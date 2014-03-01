//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ServerGame.h"

#include "GameManager.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "masterConnection.h"
#include "SoundSystem.h"
#include "luaGameInfo.h"
#include "luaLevelGenerator.h"
#include "robot.h"
#include "Teleporter.h"
#include "BanList.h"             // For banList kick duration
#include "BotNavMeshZone.h"      // For zone clearing code
#include "LevelSource.h"
#include "LevelDatabase.h"

#include "gameObjectRender.h"
#include "stringUtils.h"
#include "GeomUtils.h"

#include "GameRecorder.h"


using namespace TNL;

namespace Zap
{


static bool instantiated;           // Just a little something to keep us from creating multiple ServerGames...


// Constructor -- be sure to see Game constructor too!  Lots going on there!
ServerGame::ServerGame(const Address &address, GameSettingsPtr settings, LevelSourcePtr levelSource, bool testMode, bool dedicated) : 
      Game(address, settings),
      mRobotManager(this, settings)
{
   TNLAssert(!instantiated, "Only one ServerGame at a time, please!  If this trips while testing, "
      "it is probably because a test failed before another instance could be deleted.  Try disabling "
      "this assert, see what test fails, and fix it.  Then re-enable it, please!");
   instantiated = true;

   mLevelSource = levelSource;

   mVoteTimer = 0;
   mVoteYes = 0;
   mVoteNo = 0;
   mVoteNumber = 0;
   mVoteType = VoteLevelChange;  // Arbitrary
   mLevelLoadIndex = 0;
   mShutdownOriginator = NULL;

   setAddTarget();               // When we do an addToGame, objects should be added to ServerGame


   // Stupid C++ spec doesn't allow ternary logic with static const if there is no definition
   // Workaround is to add '+' to force a read of the value
   // See:  http://stackoverflow.com/questions/5446005/why-dont-static-member-variables-play-well-with-the-ternary-operator
   mNextLevel = settings->getIniSettings()->randomLevels ? +RANDOM_LEVEL : +NEXT_LEVEL;

   mShuttingDown = false;

   EventManager::get()->setPaused(false);

   mInfoFlags = 0;                           // Currently used to specify test mode and debug builds
   mCurrentLevelIndex = 0;

   mBotZoneDatabase = new GridDatabase();    // Deleted in destructor

   if(testMode)
      mInfoFlags |= TestModeFlag;

#ifdef TNL_DEBUG
   mInfoFlags |= DebugModeFlag;
#endif

   mTestMode = testMode;

   mNetInterface->setAllowsConnections(true);
   mMasterUpdateTimer.reset(UpdateServerStatusTime);

   mSuspendor = NULL;

   mGameInfo = NULL;

#ifdef ZAP_DEDICATED
   TNLAssert(dedicated, "Dedicated should be true here!");
#endif

   mDedicated = dedicated;

   mGameSuspended = true;                 // Server starts with zero players

   U32 stutter = mSettings->getSimulatedStutter();

   mStutterTimer.reset(1001 - stutter);   // Use 1001 to ensure timer is never set to 0
   mStutterSleepTimer.reset(stutter);
   mAccumulatedSleepTime = 0;

   botControlTickTimer.reset(BotControlTickInterval);

   mLevelSwitchTimer.setPeriod(LevelSwitchTime);
   GameManager::setHostingModePhase(GameManager::NotHosting);

   mGameRecorderServer = NULL;
}


// Destructor
ServerGame::~ServerGame()
{
   if(getConnectionToMaster())   // Prevents errors when ServerGame is gone too soon
      getConnectionToMaster()->disconnect(NetConnection::ReasonSelfDisconnect, "");

   cleanUp();

   clearAddTarget();

   instantiated = false;

   delete mGameInfo;
   delete mBotZoneDatabase;

   GameManager::setHostingModePhase(GameManager::NotHosting);

   if(mGameRecorderServer)
      delete mGameRecorderServer;
}


// Called before we load a new level, or when we shut the server down
void ServerGame::cleanUp()
{
   fillVector.clear();
   mDatabaseForBotZones.findObjects(fillVector);

   mLevelGens.deleteAndClear();
   LuaScriptRunner::resetTimer();

   for(S32 i = 0; i < fillVector.size(); i++)
      delete dynamic_cast<Object *>(fillVector[i]);

   Parent::cleanUp();
}


// Return true when handled
bool ServerGame::voteStart(ClientInfo *clientInfo, VoteType type, S32 number)
{
   GameConnection *conn = clientInfo->getConnection();

   if(!mSettings->getIniSettings()->voteEnable)
      return false;

   U32 VoteTimer;
   if(type == VoteChangeTeam)
      VoteTimer = mSettings->getIniSettings()->voteLengthToChangeTeam * 1000;
   else
      VoteTimer = mSettings->getIniSettings()->voteLength * 1000;
   if(VoteTimer == 0)
      return false;

   if(type != VoteLevelChange)
   {
      if(getGameType()->isGameOver())
         return true;   // Don't allow trying to start votes during game over, except level changing

      if((U32)getGameType()->getRemainingGameTimeInMs() - 1 < VoteTimer)  // Handles unlimited GameType time, by forcing the U32 range
      {
         conn->s2cDisplayErrorMessage("Not enough time");
         return true;
      }
   }

   if(mVoteTimer != 0)
   {
      conn->s2cDisplayErrorMessage("Can't start a new vote when there is one pending.");
      return true;
   }

   if(conn->mVoteTime != 0)
   {
      Vector<StringTableEntry> e;
      Vector<StringPtr> s;
      Vector<S32> i;

      i.push_back(conn->mVoteTime / 1000);
      conn->s2cDisplayMessageESI(GameConnection::ColorRed, SFXNone, "Can't start vote, try again %i0 seconds later.", e, s, i);

      return true;
   }

   mVoteTimer = VoteTimer;
   mVoteType = type;
   mVoteNumber = number;
   mVoteClientName = clientInfo->getName();

   for(S32 i = 0; i < getClientCount(); i++)
      if(getClientInfo(i)->getConnection())  // Robots don't have GameConnection
         getClientInfo(i)->getConnection()->mVote = 0;

   conn->mVote = 1;
   conn->s2cDisplayMessage(GameConnection::ColorInfo, SFXNone, "Vote started, waiting for others to vote.");
   return true;
}


void ServerGame::voteClient(ClientInfo *clientInfo, bool voteYes)
{
   GameConnection *conn = clientInfo->getConnection();

   if(mVoteTimer == 0)
      conn->s2cDisplayErrorMessage("!!! Nothing to vote on");
   else if(conn->mVote == (voteYes ? 1 : 2))
      conn->s2cDisplayErrorMessage("!!! Already voted");
   else if(conn->mVote == 0)
      conn->s2cDisplayMessage(GameConnection::ColorGreen, SFXNone, voteYes ? "Voted Yes" : "Voted No");
   else
      conn->s2cDisplayMessage(GameConnection::ColorGreen, SFXNone, voteYes ? "Changed vote to Yes" : "Changed vote to No");

   conn->mVote = voteYes ? 1 : 2;
}


S32 ServerGame::getCurrentLevelIndex()
{
   return mCurrentLevelIndex;
}


S32 ServerGame::getLevelCount()
{
   return mLevelSource->getLevelCount();
}


LevelInfo ServerGame::getLevelInfo(S32 index)
{
   return mLevelSource->getLevelInfo(index);
}


// Creates a set of LevelInfos that are empty except for the filename.  They will be fleshed out later.
// This gets called when you first load the host menu
//void ServerGame::buildBasicLevelInfoList(const Vector<string> &levelList)
//{
//   mLevelInfos.clear();
//
//   for(S32 i = 0; i < levelList.size(); i++)
//      mLevelInfos.push_back(LevelInfo(levelList[i]));
//}
//
//void ServerGame::clearLevelInfos()
//{
//   mLevelInfos.clear();
//}


void ServerGame::sendLevelListToLevelChangers(const string &message)
{
   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);
      GameConnection *conn = clientInfo->getConnection();

      StringTableEntry msg(message);

      if(clientInfo->isLevelChanger() && conn)
      {
        conn->sendLevelList();
        if(message != "")
           conn->s2cDisplayMessage(GameConnection::ColorInfo, SFXNone, message);
      }
   }
}


bool ServerGame::isTestServer() const
{
   return mTestMode;
}


AbstractTeam *ServerGame::getNewTeam()
{
   return new Team;
}


void ServerGame::resetLevelLoadIndex()
{
   mLevelLoadIndex = 0;
}


// Return true if the only client connected is the one we passed; don't consider bots
bool ServerGame::onlyClientIs(GameConnection *client)
{
   GameType *gameType = getGameType();

   if(!gameType)
      return false;

   for(S32 i = 0; i < getClientCount(); i++)
      if(!mClientInfos[i]->isRobot() && mClientInfos[i]->getConnection() != client)
         return false;

   return true;
}


// Control whether we're in shut down mode or not
void ServerGame::setShuttingDown(bool shuttingDown, U16 time, GameConnection *who, StringPtr reason)
{
   mShuttingDown = shuttingDown;
   if(shuttingDown)
   {
      mShutdownOriginator = who;
      const char *name = mShutdownOriginator->getClientInfo()->getName().getString();

      // If there's no other clients, then just shutdown now
      if(onlyClientIs(mShutdownOriginator))
      {
         logprintf(LogConsumer::ServerFilter, "Server shutdown requested by %s.  No other players, so shutting down now.", name);
         mShutdownTimer.reset(1);
      }
      else
      {
         logprintf(LogConsumer::ServerFilter, "Server shutdown in %d seconds, requested by %s, for reason [%s].", time, name, reason.getString());
         mShutdownTimer.reset(time * 1000);
      }
   }
   else
      logprintf(LogConsumer::ServerFilter, "Server shutdown canceled.");
}


bool ServerGame::populateLevelInfoFromSource(const string &fullFilename, LevelInfo &levelInfo)
{
   return mLevelSource->populateLevelInfoFromSource(fullFilename, levelInfo);
}


// Returns name of level loaded, which will be displayed in the client window during level loading phase of hosting.
// Can return "" if there was a problem with the level.
string ServerGame::loadNextLevelInfo()
{
   FolderManager *folderManager = getSettings()->getFolderManager();

   string filename = folderManager->findLevelFile(mLevelSource->getLevelFileName(mLevelLoadIndex));
   TNLAssert(filename != "", "Expected a filename here!");

   // populateLevelInfoFromSource() will return true if the level was processed successfully
   string levelName;
   if(mLevelSource->populateLevelInfoFromSource(filename, mLevelLoadIndex))
   {
      levelName = mLevelSource->getLevelName(mLevelLoadIndex);    // This will be the name specified in the level file we just populated
      mLevelLoadIndex++;
   }
   else     // Failed to process level; remove it from the list
      mLevelSource->remove(mLevelLoadIndex);

   // Last level to process?
   if(mLevelLoadIndex == mLevelSource->getLevelCount())
      GameManager::setHostingModePhase(GameManager::DoneLoadingLevels);

   return levelName;
}


// Get the level name, as defined in the level file
StringTableEntry ServerGame::getLevelNameFromIndex(S32 index)
{
   return mLevelSource->getLevelInfo(getAbsoluteLevelIndex(index)).mLevelName;
}


// Return filename of level currently in play
string ServerGame::getCurrentLevelFileName() const
{
   return mLevelSource->getLevelFileName(mCurrentLevelIndex);
}


// Return name of level currently in play
StringTableEntry ServerGame::getCurrentLevelName() const
{
   return mLevelSource->getLevelInfo(mCurrentLevelIndex).mLevelName;
}


// Return type of level currently in play
GameTypeId ServerGame::getCurrentLevelType()
{
   return mLevelSource->getLevelType(mCurrentLevelIndex);
}


StringTableEntry ServerGame::getCurrentLevelTypeName()
{
   return GameType::getGameTypeName(getCurrentLevelType());
}


bool ServerGame::processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database, S32 id)
{
   if(!stricmp(argv[0], "BarrierMaker"))
   {
      // Use WallItem's ProcessGeometry method to read the points; this will let us put us all our error handling
      // and geom processing in our place.
      WallItem wallItem;
      if(wallItem.processArguments(argc, argv, this))    // Returns true if wall was successfully processed
         addWallItem(&wallItem, NULL);
   }
   else if(!stricmp(argv[0], "BarrierMakerS") || !stricmp(argv[0], "PolyWall"))
   {
      PolyWall polywall;
      if(polywall.processArguments(argc, argv, this))    // Returns true if wall was successfully processed
         addPolyWall(&polywall, NULL);

   }

   else 
      return false;

   return true;
}


void ServerGame::addPolyWall(BfObject *polyWall, GridDatabase *unused)
{
   Parent::addPolyWall(polyWall, getGameObjDatabase());

   // Convert the wallItem in to a wallRec, an abbreviated form of wall that represents both regular walls and polywalls, and 
   // is convenient to transmit to the clients
   //WallRec wallRec(polyWall);
   //getGameType()->addWall(wallRec, this);
}


void ServerGame::addWallItem(BfObject *wallItem, GridDatabase *unused)
{
   Parent::addWallItem(wallItem, getGameObjDatabase());

   // Convert the wallItem in to a wallRec, an abbreviated form of wall that represents both regular walls and polywalls, and 
   // is convenient to transmit to the clients
   //WallRec wallRec(wallItem);
   //getGameType()->addWall(wallRec, this);
}


// Sort by order in which players should be added to teams
// Highest ratings first -- runs on server only, so these should be FullClientInfos
// Return 1 if a should be added before b, -1 if b should be added before a, and 0 if it doesn't matter
static S32 QSORT_CALLBACK AddOrderSort(RefPtr<ClientInfo> *a, RefPtr<ClientInfo> *b)
{
   // Always add level-specified bots first
   if((*a)->getClientClass() == ClientInfo::ClassRobotAddedByLevel)
      return 1;
   if((*b)->getClientClass() == ClientInfo::ClassRobotAddedByLevel)
      return -1;

   // Always add other bots last
   if((*a)->getClientClass() != ClientInfo::ClassHuman)
      return -1;
   if((*b)->getClientClass() != ClientInfo::ClassHuman)
      return 1;

   bool aIsIdle = !(*a)->getConnection() || !(*a)->getConnection()->getObjectMovedThisGame();
   bool bIsIdle = !(*b)->getConnection() || !(*b)->getConnection()->getObjectMovedThisGame();

   // If either player is idle, put them at the bottom of the list
   if(aIsIdle && !bIsIdle)
      return -1;

   if(!aIsIdle && bIsIdle)
      return 1;

   // Add higher-rated players first
   if((*a)->getCalculatedRating() > (*b)->getCalculatedRating())
      return 1;
   else if((*a)->getCalculatedRating() < (*b)->getCalculatedRating())
      return -1;
   else
      return 0;
}


// Clear, prepare, and load the level given by the index \nextLevel. This
// function respects meta-indices, and otherwise expects an absolute index.
void ServerGame::cycleLevel(S32 nextLevel)
{
   if(mGameRecorderServer)
   {
      delete mGameRecorderServer;
      mGameRecorderServer = NULL;
   }

   cleanUp();
   mLevelSwitchTimer.clear();
   mScopeAlwaysList.clear();

   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);
      GameConnection *conn = clientInfo->getConnection();

      conn->resetGhosting();
      conn->switchedTeamCount = 0;

      clientInfo->setScore(0);         // Reset player scores, for non team game types
      clientInfo->clearKillStreak();   // Clear any rampage the players have going... sorry, lads!
   }

   mRobotManager.onLevelChanged();


   bool loaded = false;

   while(!loaded)
   {
      mCurrentLevelIndex = getAbsoluteLevelIndex(nextLevel); // Set mCurrentLevelIndex to refer to the next level we'll play

      logprintf(LogConsumer::ServerFilter, "Loading %s [%s]... \\", getLevelNameFromIndex(mCurrentLevelIndex).getString(), 
                                                                    mLevelSource->getLevelFileDescriptor(mCurrentLevelIndex).c_str());

      // Load the level for real this time (we loaded it once before, when we started the server, but only to grab a few params)
      if(loadLevel())
      {
         loaded = true;
         logprintf(LogConsumer::ServerFilter, "Done. [%s]", getTimeStamp().c_str());
      }
      else
      {
         logprintf(LogConsumer::ServerFilter, "FAILED!");

         if(mLevelSource->getLevelCount() > 1)
            removeLevel(mCurrentLevelIndex);
         else
         {
            // No more working levels to load...  quit?
            logprintf(LogConsumer::LogError, "All the levels I was asked to load are corrupt.  Exiting!");

            mShutdownTimer.reset(1); 
            mShuttingDown = true;
            mShutdownReason = "All the levels I was asked to load are corrupt or missing; "
                              "Sorry dude -- hosting mode shutting down.";

            // To avoid crashing...
            if(!getGameType())
            {
               GameType *gameType = new GameType();
               gameType->addToGame(this, getGameObjDatabase());
            }
            getGameType()->makeSureTeamCountIsNotZero();

            return;
         }
      }
   }

   computeWorldObjectExtents();                       // Compute world Extents nice and early

   if(!mGameRecorderServer && !mShuttingDown && getSettings()->getIniSettings()->enableGameRecording)
      mGameRecorderServer = new GameRecorderServer(this);


   ////// This block could easily be moved off somewhere else   
   fillVector.clear();
   getGameObjDatabase()->findObjects(TeleporterTypeNumber, fillVector);

   Vector<pair<Point, const Vector<Point> *> > teleporterData(fillVector.size());
   pair<Point, const Vector<Point> *> teldat;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      Teleporter *teleporter = static_cast<Teleporter *>(fillVector[i]);

      teldat.first  = teleporter->getPos();
      teldat.second = teleporter->getDestList();

      teleporterData.push_back(teldat);
   }

   // Get our parameters together
   Vector<DatabaseObject *> barrierList;
   getGameObjDatabase()->findObjects((TestFunc)isWallType, barrierList, *getWorldExtents());

   Vector<DatabaseObject *> turretList;
   getGameObjDatabase()->findObjects(TurretTypeNumber, turretList, *getWorldExtents());

   Vector<DatabaseObject *> forceFieldProjectorList;
   getGameObjDatabase()->findObjects(ForceFieldProjectorTypeNumber, forceFieldProjectorList, *getWorldExtents());

   bool triangulate;

   // Try and load Bot Zones for this level, set flag if failed
   // We need to run buildBotMeshZones in order to set mAllZones properly, which is why I (sort of) disabled the use of hand-built zones in level files
#ifdef ZAP_DEDICATED
   triangulate = false;
#else
   triangulate = !isDedicated();
#endif

   mGameType->mBotZoneCreationFailed = !BotNavMeshZone::buildBotMeshZones(mBotZoneDatabase, &mAllZones,
                                                                          getWorldExtents(), barrierList, turretList,
                                                                          forceFieldProjectorList, teleporterData, triangulate);
   // Clear team info for all clients
   resetAllClientTeams();

   // Reset loadouts now that we have GameType set up
   for(S32 i = 0; i < getClientCount(); i++)
      getClientInfo(i)->resetLoadout(levelHasLoadoutZone());
  

   // Now add players to the gameType, from highest rating to lowest in an attempt to create ratings-based teams
   // Sorting also puts idle players at the end of the list, regardless of their rating
   mClientInfos.sort(AddOrderSort);

   if(mGameType.isValid())
   {
      // Backwards!  So the lowest scorer goes on the larger team (if there is uneven teams)
      for(S32 i = getClientCount() - 1; i > -1; i--)
      {
         ClientInfo *clientInfo = getClientInfo(i);   // Could be a robot when level have "Robot" line, or a levelgen adds one
         
         mGameType->serverAddClient(clientInfo);

         GameConnection *connection = clientInfo->getConnection();
         if(connection)
         {
            connection->setObjectMovedThisGame(false);
            connection->activateGhosting();                 // Tell clients we're done sending objects and are ready to start playing
         }
      }
   }

   // Fire onPlayerJoined event for any players already on the server
   for(S32 i = 0; i < getClientCount(); i++)
      EventManager::get()->fireEvent(NULL, EventManager::PlayerJoinedEvent, getClientInfo(i)->getPlayerInfo());


   mRobotManager.balanceTeams();

   sendLevelStatsToMaster();     // Give the master some information about this level for its database

   suspendIfNoActivePlayers();   // Does nothing if we're already suspended
}


void ServerGame::onConnectedToMaster()
{
   Parent::onConnectedToMaster();

   // Check if we have any clients that need to have their authentication status checked; might happen if we've lost touch with master 
   // and clients have connected in the meantime.  In some rare circumstances, could lead to double-verification, but I don't think this
   // would be a real problem
   for(S32 i = 0; i < getClientCount(); i++)
      if(!getClientInfo(i)->isRobot())
         getClientInfo(i)->getConnection()->requestAuthenticationVerificationFromMaster();

   sendLevelStatsToMaster();    // We're probably in a game, and we should send the level details to the master

   logprintf(LogConsumer::MsgType(LogConsumer::LogConnection | LogConsumer::ServerFilter), 
             "Server established connection with Master Server");
}


void ServerGame::sendLevelStatsToMaster()
{
   // Send level stats to master, but don't bother in test mode -- don't want to gum things up with a bunch of one-off levels
   if(mTestMode)
      return;

   // Also don't bother if we are not yet in full-on hosting mode
   if(GameManager::getHostingModePhase() != GameManager::Hosting)
      return;

   MasterServerConnection *masterConn = getConnectionToMaster();

   if(!(masterConn && masterConn->isEstablished()))
      return;

   // Check if we've already sent these stats... if so, no need to waste bandwidth and resend
   // TODO: Is there a standard container that would make this process simpler?  like a sorted hash or whatnot?
   for(S32 i = 0; i < mSentHashes.size(); i++)
      if(mSentHashes[i] == mLevelFileHash)
         return;

   S32 teamCountU8 = getTeamCount();

   if(teamCountU8 > U8_MAX)            // Should never happen!
      teamCountU8 = U8_MAX;

   bool hasLevelGen = getGameType()->getScriptName() != "";

   // Construct the info now, to be later sent, sending later avoids overloading the master with too much data
   mSendLevelInfoDelayNetInfo = masterConn->s2mSendLevelInfo_construct(mLevelFileHash, mGameType->getLevelName(), 
                                mGameType->getLevelCredits()->getString(), 
                                getCurrentLevelTypeName(),
                                hasLevelGen, 
                                (U8)teamCountU8, 
                                mGameType->getWinningScore(), 
                                getRemainingGameTime());

   mSendLevelInfoDelayCount.reset(6000);  // set time left to send

   mSentHashes.push_back(mLevelFileHash);
}


// Resets all player team assignments
void ServerGame::resetAllClientTeams()
{
   for(S32 i = 0; i < getClientCount(); i++)
      getClientInfo(i)->setTeamIndex(NO_TEAM);
}


// Currently only used by tests to temporarily disable bot leveling while setting up various team configurations
bool ServerGame::getAutoLevelingEnabled() const
{
   return mRobotManager.getAutoLevelingEnabled();
}


// Currently only used by tests to temporarily disable bot leveling while setting up various team configurations
void ServerGame::setAutoLeveling(bool enabled)
{
   mRobotManager.setAutoLeveling(enabled);
}


// Make sure level metadata fits with our current game situation; i.e. check playerCount against min/max players,
// skip uploaded levels if the settings tell us to, etc.  Can expand this to incorporate other metadata as we 
// develop it.
static bool checkIfLevelIsOk(bool skipUploads, const LevelInfo &levelInfo, S32 playerCount)
{
   S32 minPlayers = levelInfo.minRecPlayers;
   S32 maxPlayers = levelInfo.maxRecPlayers;

   if(maxPlayers <= 0)        // i.e. limit doesn't apply or is invalid (note if limit doesn't apply on the minPlayers, 
      maxPlayers = S32_MAX;   // then it works out because the smallest number of players is 1).

   if(playerCount < minPlayers)
      return false;
   if(playerCount > maxPlayers)
      return false;

   if(skipUploads)   // Skip levels starting with our upload prefix (currently "upload_")
      if(!strncmp(levelInfo.filename.c_str(), UploadPrefix.c_str(), UploadPrefix.length()))
         return false;

   return true;
}


// Helps resolve pseudo-indices such as NEXT_LEVEL.  If you pass it a normal level index, you'll just get that back.
S32 ServerGame::getAbsoluteLevelIndex(S32 nextLevel)
{
   S32 currentLevelIndex = mCurrentLevelIndex;
   S32 levelCount = mLevelSource->getLevelCount();
   bool skipUploads = getSettings()->getIniSettings()->skipUploads;

   if(levelCount == 1)
      nextLevel = FIRST_LEVEL;

   else if(nextLevel >= FIRST_LEVEL && nextLevel < levelCount)          // Go to specified level
      currentLevelIndex = (nextLevel < levelCount) ? nextLevel : FIRST_LEVEL;

   else if(nextLevel == NEXT_LEVEL)      // Next level
   {
      // If game is supended, then we are waiting for another player to join.  That means that (probably)
      // there are either 0 or 1 players, so the next game will need to be good for 1 or 2 players.
      S32 playerCount = getPlayerCount();
      if(mGameSuspended)
         playerCount++;

      bool first = true;
      bool found = false;

      S32 currLevel = currentLevelIndex;

      // Cycle through the levels looking for one that matches our player counts
      while(first || currentLevelIndex != currLevel)
      {
         currentLevelIndex = (currentLevelIndex + 1) % levelCount;

         if(checkIfLevelIsOk(skipUploads, mLevelSource->getLevelInfo(currentLevelIndex), playerCount))
         {
            found = true;
            break;
         }

         // else do nothing to play the next level, which we found above
         first = false;
      }

      // We didn't find a suitable level... just proceed to the next one in the list
      if(!found)
      {
         currentLevelIndex++;
         if(S32(currentLevelIndex) >= levelCount)
            currentLevelIndex = FIRST_LEVEL;
      }
   } 
   else if(nextLevel == PREVIOUS_LEVEL)
   {
      currentLevelIndex--;
      if(currentLevelIndex < 0)
         currentLevelIndex = levelCount - 1;
   }
   else if(nextLevel == RANDOM_LEVEL)
   {
      S32 newLevel;
      U32 retriesLeft = 200;
      S32 playerCount = getPlayerCount();

      if(mGameSuspended)
         playerCount++;

      do
      {
         newLevel = TNL::Random::readI(0, levelCount - 1);
         retriesLeft--;  // Prevent endless loop

         if(retriesLeft == 0)
            break;

      } while(!(newLevel != currentLevelIndex && 
                (retriesLeft < 100 || checkIfLevelIsOk(skipUploads, mLevelSource->getLevelInfo(currentLevelIndex), playerCount))));
      // until (( level is not the current one ) && 
      //          we're desperate  || level is ok ))

      currentLevelIndex = newLevel;
   }

   //else if(nextLevel == REPLAY_LEVEL)    // Replay level, do nothing
   //   currentLevelIndex += 0;

   if(currentLevelIndex >= levelCount)  // Safety check in case of trying to replay level that was just deleted
      currentLevelIndex = 0;

   return currentLevelIndex;
}

bool ServerGame::isOrIsAboutToBeSuspended()
{
   return mGameSuspended || mTimeToSuspend.getCurrent() > 0;
}

bool ServerGame::clientCanSuspend(ClientInfo *info)
{
   if(info->isAdmin())
      return true;

   if(info->isSpawnDelayed())
      return false;

   U32 activePlayers = 0;
   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);
      if(!clientInfo->isRobot() && !clientInfo->isSpawnDelayed())
         activePlayers++;
   }
   return activePlayers <= 1; // If only one player active, allow suspend.
}

// Enter suspended animation mode
void ServerGame::suspendGame()
{
   if(mGameSuspended) // Already suspended
      return;

   for(S32 i = 0; i < getClientCount(); i++)
      if(getClientInfo(i)->getConnection())
         getClientInfo(i)->getConnection()->s2rSetSuspendGame(true);

   mGameSuspended = true;
}


void ServerGame::suspendGame(GameConnection *gc)
{
   if(mGameSuspended) // Already suspended
      return;
   mSuspendor = gc;
   suspendGame();
}

 
// Resume game after it is no longer suspended
void ServerGame::unsuspendGame(bool remoteRequest)
{
   mTimeToSuspend.clear();
   if(!mGameSuspended)
      return;

   mGameSuspended = false;

   for(S32 i = 0; i < getClientCount(); i++)
      if(getClientInfo(i)->getConnection())
         getClientInfo(i)->getConnection()->s2rSetSuspendGame(false);

   mSuspendor = NULL;
}


void ServerGame::suspenderLeftGame()
{
   mSuspendor = NULL;
   unsuspendIfActivePlayers();
}


GameConnection *ServerGame::getSuspendor()
{
   return mSuspendor;
}

// suspend only if there are non-idling players (TestSpawnDelay.cpp needs changing if using this)
static bool shouldBeSuspended(ServerGame *game)
{
   // Check all clients and make sure they're not active
   for(S32 i = 0; i < game->getClientCount(); i++)
   {
      ClientInfo *clientInfo = game->getClientInfo(i);

      // Robots don't count
      if(!clientInfo->isRobot() && !clientInfo->isSpawnDelayed())
         return false;
   }
   return true;
}

// Check to see if there are any players who are active; suspend the game if not.  Server only.
void ServerGame::suspendIfNoActivePlayers(bool delaySuspend)
{
   if(mGameSuspended)
      return;

   if(shouldBeSuspended(this))
   {
      if(delaySuspend)
         mTimeToSuspend.reset(PreSuspendSettlingPeriod);
      else
         suspendGame();
   }
}

// Check to see if there are any players who are active; suspend the game if not.  Server only.
void ServerGame::unsuspendIfActivePlayers()
{
   if(mSuspendor && clientCanSuspend(mSuspendor->getClientInfo()))
      return; // Keep the game suspended if a player paused the game and still can pause game.

   if(!shouldBeSuspended(this))
      unsuspendGame(false);
}


// Need to handle both forward and backward slashes... will return pathname with trailing delimeter.
inline string getPathFromFilename(const string &filename)
{
   std::size_t pos1 = filename.rfind("/");
   std::size_t pos2 = filename.rfind("\\");

   if(pos1 == string::npos)
      pos1 = 0;

   if(pos2 == string::npos)
      pos2 = 0;

   return filename.substr( 0, max(pos1, pos2) + 1 );
}


bool ServerGame::loadLevel()
{
   resetLevelInfo();    // Resets info about the level, not a LevelInfo...  In case you were wondering.

   mObjectsLoaded = 0;
   setLevelDatabaseId(LevelDatabase::NOT_IN_DATABASE);

   mLevelFileHash = mLevelSource->loadLevel(mCurrentLevelIndex, this, getGameObjDatabase());

   // Empty hash means file was not loaded.  Danger Will Robinson!
   if(mLevelFileHash == "")
   {
      logprintf(LogConsumer::LogError, "Error: Cannot load %s", mLevelSource->getLevelFileDescriptor(mCurrentLevelIndex).c_str());
      return false;
   }

   // We should have a gameType by the time we get here... but in case we don't, we'll add a default one now
   if(!getGameType())
   {
      logprintf(LogConsumer::LogWarning, "Warning: Missing GameType parameter in %s (defaulting to Bitmatch)", mLevelSource->getLevelFileDescriptor(mCurrentLevelIndex).c_str());
      GameType *gameType = new GameType;
      gameType->addToGame(this, getGameObjDatabase());
   }
   
   // Levelgens:
   // Run level's levelgen script (if any)
   runLevelGenScript(getGameType()->getScriptName());

   // Run any global levelgen scripts (if defined)
   Vector<string> scriptList;
   parseString(mSettings->getIniSettings()->globalLevelScript, scriptList, '|');

   for(S32 i = 0; i < scriptList.size(); i++)
      runLevelGenScript(scriptList[i]);

   // Fire an update to make sure certain events run on level start (like onShipSpawned)
   EventManager::get()->update();

   // Check after script, script might add Teams
   if(getGameType()->makeSureTeamCountIsNotZero())
      logprintf(LogConsumer::LogLevelError, "Warning: Missing Team in %s", mLevelSource->getLevelFileDescriptor(mCurrentLevelIndex).c_str());

   getGameType()->onLevelLoaded();

   return true;
}


void ServerGame::runLevelGenScript(const string &scriptName)
{
   if(scriptName == "")    // No script specified!
      return;

   // Find full name of levelgen script -- returns "" if file not found
   string fullname = getSettings()->getFolderManager()->findLevelGenScript(scriptName);  

   if(fullname == "")
   {
      logprintf(LogConsumer::MsgType(LogConsumer::LogWarning | LogConsumer::LuaLevelGenerator), 
                "Warning: Could not find levelgen script \"%s\"", scriptName.c_str());
      return;
   }

   // The script file will be the first argument, subsequent args will be passed on to the script -- 
   // will be deleted when level ends in ServerGame::cleanUp()
   LuaLevelGenerator *levelgen = new LuaLevelGenerator(this, fullname, *getGameType()->getScriptArgs());

   if(!levelgen->runScript(!isTestServer()))    // Do not cache on test server
      delete levelgen;
   else
      mLevelGens.push_back(levelgen);
}


// Add a misbehaved levelgen to the kill list
void ServerGame::deleteLevelGen(LuaLevelGenerator *levelgen)
{
   mLevelGenDeleteList.push_back(levelgen);
}


Vector<Vector<S32> > ServerGame::getCategorizedPlayerCountsByTeam() const
{
   countTeamPlayers();

   Vector<Vector<S32> > counts;

   counts.resize(getTeamCount());
   for(S32 i = 0; i < counts.size(); i++)
   {
      counts[i].resize(ClientInfo::ClassCount);
      for(S32 j = 0; j < counts[i].size(); j++)
         counts[i][j] = 0;
   }

   for(S32 i = 0; i < mClientInfos.size(); i++)
   {
      S32 team = mClientInfos[i]->getTeamIndex();
      S32 cc   = mClientInfos[i]->getClientClass();

      if(team >= 0)
         counts[team][cc]++;
   }

   return counts;
}


void ServerGame::addClient(ClientInfo *clientInfo)
{
   TNLAssert(!clientInfo->isRobot(), "This only gets called for players");

   GameConnection *conn = clientInfo->getConnection();

   // If client has level change or admin permissions, send a list of levels and their types to the connecting client
   if(clientInfo->isLevelChanger() || clientInfo->isAdmin())
      conn->sendLevelList();

   // If we're shutting down, display a notice to the user... but still let them connect normally
   if(mShuttingDown)
      conn->s2cInitiateShutdown(mShutdownTimer.getCurrent() / 1000,
            mShutdownOriginator.isNull() ? StringTableEntry() : mShutdownOriginator->getClientInfo()->getName(), 
            "Sorry -- server shutting down", false);

   TNLAssert(mGameType, "No gametype?");     // Added 12/20/2013 by wat to try to understand if this ever happens
   if(mGameType.isValid())
   {
      mGameType->serverAddClient(clientInfo);
      addToClientList(clientInfo);
   }
   // Else... what?

   mRobotManager.balanceTeams();
   
   // When a new player joins, game is always unsuspended!
   unsuspendIfActivePlayers();

   if(mDedicated)
      SoundSystem::playSoundEffect(SFXPlayerJoined, 1);
}


void ServerGame::removeClient(ClientInfo *clientInfo)
{
   if(mGameType.isValid())
      mGameType->serverRemoveClient(clientInfo);

   if(mDedicated)
      SoundSystem::playSoundEffect(SFXPlayerLeft, 1);

   if(getPlayerCount() == 0 && !mShuttingDown && isDedicated())  // only dedicated server can have zero players
      cycleLevel(getSettings()->getIniSettings()->randomLevels ? +RANDOM_LEVEL : +NEXT_LEVEL);    // Advance to beginning of next level
   else
      mRobotManager.balanceTeams();
}


// Loop through all our bots and start their interpreters, delete those that sqawk
// Only called from GameType::onLevelLoaded()
void ServerGame::startAllBots()
{   
   // Do nothing -- remove this fn!
}


string ServerGame::addBot(const Vector<const char *> &args, ClientInfo::ClientClass clientClass)
{
   return mRobotManager.addBot(args, clientClass);
}


void ServerGame::addBot(Robot *robot)
{
   mRobotManager.addBot(robot);
}


void ServerGame::balanceTeams()
{
   mRobotManager.balanceTeams();
}


Robot *ServerGame::getBot(S32 index)
{
   return mRobotManager.getBot(index);
}


S32 ServerGame::getBotCount() const
{
   return mRobotManager.getBotCount();
}


Robot *ServerGame::findBot(const char *id)
{
   return mRobotManager.findBot(id);
}


void ServerGame::removeBot(Robot *robot)
{
   mRobotManager.removeBot(robot);
}


void ServerGame::deleteBot(const StringTableEntry &name)
{
   mRobotManager.deleteBot(name);
}


void ServerGame::deleteBot(S32 i)
{
   mRobotManager.deleteBot(i);
}


//void ServerGame::deleteBotFromTeam(S32 teamIndex)
//{
//   mRobotManager.deleteBotFromTeam(teamIndex);
//}


void ServerGame::deleteAllBots()
{
   mRobotManager.deleteAllBots();
}


void ServerGame::moreBots()
{
   mRobotManager.moreBots();
}


void ServerGame::fewerBots()
{
   mRobotManager.fewerBots();
}


// Comes from c2sKickBot
void ServerGame::kickSingleBotFromLargestTeamWithBots()
{
    mRobotManager.deleteBotFromTeam(findLargestTeamWithBots(), ClientInfo::ClassAnyBot);
}


U32 ServerGame::getMaxPlayers() const
{
   return mSettings->getMaxPlayers();
}


bool ServerGame::isDedicated() const
{
   return mDedicated;
}


void ServerGame::setDedicated(bool dedicated)
{
   mDedicated = dedicated;
}


bool ServerGame::isFull()
{
   return (U32) getPlayerCount() >= mSettings->getMaxPlayers();
}


// Only called from outside ServerGame
bool ServerGame::isReadyToShutdown(U32 timeDelta, string &reason)
{
   reason = mShutdownReason;
   return mShuttingDown && (mShutdownTimer.update(timeDelta) || onlyClientIs(mShutdownOriginator));
}


bool ServerGame::isServer() const
{
   return true;
}


// Top-level idle loop for server, runs only on the server by definition
void ServerGame::idle(U32 timeDelta)
{
   // No idle during pre-game level loading
   if(GameManager::getHostingModePhase() == GameManager::LoadingLevels)
      return;

   Parent::idle(timeDelta);

   processSimulatedStutter(timeDelta);
   processVoting(timeDelta);

   if(mSendLevelInfoDelayCount.update(timeDelta) && mSendLevelInfoDelayNetInfo.isValid() && this->getConnectionToMaster())
   {
      this->getConnectionToMaster()->postNetEvent(mSendLevelInfoDelayNetInfo);
      mSendLevelInfoDelayNetInfo = NULL; // we can now let it free memory
   }


   // If there are no players on the server, we can enter "suspended animation" mode, but not during the first half-second of hosting.
   // This will prevent locally hosted game from immediately suspending for a frame, giving the local client a chance to 
   // connect.  A little hacky, but works!
      /*if(getPlayerCount() == 0 && !mGameSuspended && mCurrentTime != 0)
         suspendGame();
   */
   if(timeDelta > MaxTimeDelta)   // Prevents timeDelta from going too high, usually when after the server was frozen
      timeDelta = 100;

   mNetInterface->checkIncomingPackets();
   checkConnectionToMaster(timeDelta);                   // Connect to master server if not connected

   mSettings->getBanList()->updateKickList(timeDelta);   // Unban players who's bans have expired

   // Periodically update our status on the master, so they know what we're doing...
   if(mMasterUpdateTimer.update(timeDelta))
      updateStatusOnMaster();

   // If we have a data transfer going on, process it
   if(!dataSender.isDone())
      dataSender.sendNextLine();

   // Play any sounds server might have made... (this is only for special alerts such as player joined or left)
   if(isDedicated())   // Non-dedicated servers will process sound in client side
      SoundSystem::processAudio(mSettings->getIniSettings()->alertsVolLevel);    // No music or voice on server!

   if(mTimeToSuspend.update(timeDelta))
      suspendGame();

   if(mGameSuspended)     // If game is suspended, we need do nothing more
   {
      mNetInterface->processConnections();
      return;
   }


   mCurrentTime += timeDelta;

   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);

      if(!clientInfo->isRobot())
      {
         GameConnection *conn = clientInfo->getConnection();
         TNLAssert(conn, "clientInfo->getConnection() shouldn't be NULL");

         conn->updateTimers(timeDelta);
      }
   }

   // Tick Lua Timer
   LuaScriptRunner::tickTimer(timeDelta);

   // Check for any levelgens that must die
   for(S32 i = 0; i < mLevelGenDeleteList.size(); i++)
   {
      S32 index = mLevelGens.getIndex(mLevelGenDeleteList[i]);
      if(index != -1)
         mLevelGens.deleteAndErase_fast(index);
   }

   // Compute new world extents -- these might change if a ship flies far away, for example...
   // In practice, we could probably just set it and forget it when we load a level.
   // Compute it here to save recomputing it for every robot and other method that relies on it.
   computeWorldObjectExtents();

   U32 botControlTickElapsed = botControlTickTimer.getElapsed();

   if(botControlTickTimer.update(timeDelta))
   {
      // Clear all old bot moves, so that if the bot does nothing, it doesn't just continue with what it was doing before
      mRobotManager.clearMoves();

      // Fire TickEvent, in case anyone is listening
      EventManager::get()->fireEvent(EventManager::TickEvent, botControlTickElapsed + timeDelta);

      botControlTickTimer.reset();
   }
   
   const Vector<DatabaseObject *> *gameObjects = mGameObjDatabase->findObjects_fast();

   // Visit each game object, handling moves and running its idle method
   for(S32 i = gameObjects->size() - 1; i >= 0; i--)
   {
      BfObject *obj = static_cast<BfObject *>((*gameObjects)[i]);

      if(obj->isDeleted())
         continue;

      // Here is where the time gets set for all the various object moves
      Move thisMove = obj->getCurrentMove();
      thisMove.time = timeDelta;

      // Give the object its move, then have it idle
      obj->setCurrentMove(thisMove);
      obj->idle(BfObject::ServerIdleMainLoop);
   }

   if(mGameType)
      mGameType->idle(BfObject::ServerIdleMainLoop, timeDelta);

   processDeleteList(timeDelta);

   // Load a new level if the time is out on the current one
   if(mLevelSwitchTimer.update(timeDelta))
   {
      // Kick any players who were idle the entire previous game.  But DO NOT kick the hosting player!
      for(S32 i = 0; i < getClientCount(); i++)
      {
         ClientInfo *clientInfo = getClientInfo(i);

         if(!clientInfo->isRobot())
         {
            GameConnection *connection = clientInfo->getConnection();
            
            if(!connection->getObjectMovedThisGame() && !connection->isLocalConnection())    // Don't kick the host, please!
               connection->disconnect(NetConnection::ReasonIdle, "");
         }
      }

      // Normalize ratings for this game
      getGameType()->updateRatings();
      cycleLevel(mNextLevel);
      mNextLevel = getSettings()->getIniSettings()->randomLevels ? +RANDOM_LEVEL : +NEXT_LEVEL;
   }


   if(mGameRecorderServer)
      mGameRecorderServer->idle(timeDelta);

   mNetInterface->processConnections(); // Update to other clients right after idling everything else, so clients get more up to date information
}


void ServerGame::processSimulatedStutter(U32 timeDelta)
{
   // Simulate CPU stutter without impacting ClientGames
   if(mSettings->getSimulatedStutter() > 0)
   {
      if(mStutterTimer.getCurrent() > 0)      
      {
         if(mStutterTimer.update(timeDelta))
            mStutterSleepTimer.reset();               // Go to sleep
      }
      else     // We're sleeping
      {
         if(mStutterSleepTimer.update(timeDelta))     // Wake up!
         {
            mStutterTimer.reset();
            timeDelta += mAccumulatedSleepTime;       // Give serverGame credit for time we slept
            mAccumulatedSleepTime = 0;
         }
         else
         {
            mAccumulatedSleepTime += timeDelta;
            return;
         }
      }
   }
}


void ServerGame::processVoting(U32 timeDelta)
{
   if(mVoteTimer != 0)
   {
      if(timeDelta < mVoteTimer)  // voting continue
      {
         if((mVoteTimer-timeDelta) % 1000 > mVoteTimer % 1000) // show message
         {
            Vector<StringTableEntry> e;
            Vector<StringPtr> s;
            Vector<S32> i;
            StringTableEntry msg;
            i.push_back(mVoteTimer / 1000);
            switch(mVoteType)
            {
            case VoteLevelChange:
               msg = "/YES or /NO : %i0 : Change Level to %e0";
               e.push_back(getLevelNameFromIndex(mVoteNumber));
               break;
            case VoteAddTime:
               msg = "/YES or /NO : %i0 : Add time %i1 minutes";
               i.push_back(mVoteNumber / 60000);
               break;
            case VoteSetTime:
               msg = "/YES or /NO : %i0 : Set time %i1 minutes %i2 seconds";
               i.push_back(mVoteNumber / 60000);
               i.push_back((mVoteNumber / 1000) % 60);
               break;
            case VoteSetScore:
               msg = "/YES or /NO : %i0 : Set score %i1";
               i.push_back(mVoteNumber);
               break;
            case VoteChangeTeam:
               msg = "/YES or /NO : %i0 : Change team %e0 to %e1";
               e.push_back(mVoteClientName);
               e.push_back(getTeamName(mVoteNumber));
               break;
            case VoteResetScore:
               msg = "/YES or /NO : %i0 : Reset All Scores";
               break;
            }
            
            bool WaitingToVote = false;

            for(S32 i2 = 0; i2 < getClientCount(); i2++)
            {
               ClientInfo *clientInfo = getClientInfo(i2);
               GameConnection *conn = clientInfo->getConnection();

               if(conn && conn->mVote == 0 && !clientInfo->isRobot())
               {
                  WaitingToVote = true;
                  conn->s2cDisplayMessageESI(GameConnection::ColorInfo, SFXNone, msg, e, s, i);
               }
            }

            if(!WaitingToVote)
               mVoteTimer = timeDelta + 1;  // no more waiting when everyone have voted.
         }
         mVoteTimer -= timeDelta;
      }
      else                        // Vote ends
      {
         S32 voteYes = 0;
         S32 voteNo = 0;
         S32 voteNothing = 0;
         mVoteTimer = 0;

         for(S32 i = 0; i < getClientCount(); i++)
         {
            ClientInfo *clientInfo = getClientInfo(i);
            GameConnection *conn = clientInfo->getConnection();

            if(conn && conn->mVote == 1)
               voteYes++;
            else if(conn && conn->mVote == 2)
               voteNo++;
            else if(conn && !clientInfo->isRobot())
               voteNothing++;
         }

         bool votePass = voteYes     * mSettings->getIniSettings()->voteYesStrength + 
                         voteNo      * mSettings->getIniSettings()->voteNoStrength  + 
                         voteNothing * mSettings->getIniSettings()->voteNothingStrength > 0;
         if(votePass)
         {
            mVoteTimer = 0;
            switch(mVoteType)
            {
               case VoteLevelChange:
                  mNextLevel = mVoteNumber;
                  if(mGameType)
                     mGameType->gameOverManGameOver();
                  break;
               case VoteAddTime:
                  if(mGameType)
                  {
                     mGameType->extendGameTime(mVoteNumber);                           // Increase "official time"
                     mGameType->broadcastNewRemainingTime();   
                  }
                  break;   
               case VoteSetTime:
                  if(mGameType)
                  {
                     mGameType->extendGameTime(S32(mVoteNumber - mGameType->getRemainingGameTimeInMs()));
                     mGameType->broadcastNewRemainingTime();                                   
                  }
                  break;
               case VoteSetScore:
                  if(mGameType)
                  {
                     mGameType->setWinningScore(mVoteNumber);
                     mGameType->s2cChangeScoreToWin(mVoteNumber, mVoteClientName);     // Broadcast score to clients
                  }
                  break;
               case VoteChangeTeam:
                  if(mGameType)
                  {
                     for(S32 i = 0; i < getClientCount(); i++)
                     {
                        ClientInfo *clientInfo = getClientInfo(i);

                        if(clientInfo->getName() == mVoteClientName)
                           mGameType->changeClientTeam(clientInfo, mVoteNumber);
                     }
                  }
                  break;
               case VoteResetScore:
                  if(mGameType && mGameType->getGameTypeId() != CoreGame) // No changing score in Core
                  {
                     // Reset player scores
                     for(S32 i = 0; i < getClientCount(); i++)
                     {
                        if(getClientInfo(i)->getScore() != 0)
                           mGameType->s2cSetPlayerScore(i, 0);
                        getClientInfo(i)->setScore(0);
                     }

                     // Reset team scores
                     for(S32 i = 0; i < getTeamCount(); i++)
                     {
                        // broadcast it to the clients
                        if(((Team*)getTeam(i))->getScore() != 0)
                           mGameType->s2cSetTeamScore(i, 0);

                        // Set the score internally...
                        ((Team*)getTeam(i))->setScore(0);
                     }
                  }
                  break;
               default:
                  TNLAssert(false, "Invalid option in switch statement!");
                  break;
            } // switch
         } // if


         Vector<StringTableEntry> e;
         Vector<StringPtr> s;
         Vector<S32> i;

         i.push_back(voteYes);
         i.push_back(voteNo);
         i.push_back(voteNothing);
         e.push_back(votePass ? "Pass" : "Fail");

         for(S32 i2 = 0; i2 < getClientCount(); i2++)
         {
            ClientInfo *clientInfo = getClientInfo(i2);
            GameConnection *conn = clientInfo->getConnection();

            if(conn)
            {
               conn->s2cDisplayMessageESI(GameConnection::ColorInfo, SFXNone, "Vote %e0 - %i0 yes, %i1 no, %i2 did not vote", e, s, i);

               if(!votePass && clientInfo->getName() == mVoteClientName)
                  conn->mVoteTime = mSettings->getIniSettings()->voteRetryLength * 1000;
            }
         }
      }
   }
}


// Inform master of how things are hanging on this game server
void ServerGame::updateStatusOnMaster()
{
   MasterServerConnection *masterConn = getConnectionToMaster();

   static StringTableEntry prevCurrentLevelName;      // Using static, so it holds the value when it comes back here
   static GameTypeId prevCurrentLevelType;
   static S32 prevRobotCount;
   static S32 prevPlayerCount;

   if(masterConn && masterConn->isEstablished())
   {
      // Only update if something is different
      if(prevCurrentLevelName != getCurrentLevelName() || prevCurrentLevelType != getCurrentLevelType() || 
         prevRobotCount       != getRobotCount()       || prevPlayerCount      != getPlayerCount())
      {
         prevCurrentLevelName = getCurrentLevelName();
         prevCurrentLevelType = getCurrentLevelType();
         prevRobotCount       = getRobotCount();
         prevPlayerCount      = getPlayerCount();

         masterConn->updateServerStatus(getCurrentLevelName(), 
                                        getCurrentLevelTypeName(), 
                                        getRobotCount(), 
                                        getPlayerCount(), 
                                        mSettings->getMaxPlayers(), 
                                        mInfoFlags);

         mMasterUpdateTimer.reset(UpdateServerStatusTime);
      }
      else
         mMasterUpdateTimer.reset(CheckServerStatusTime);
   }
   else
   {
      prevPlayerCount = -1;   // Not sure if needed, but if we're disconnected, we need to update to master when we reconnect
      mMasterUpdateTimer.reset(CheckServerStatusTime);
   }
}


// Returns true if things went well, false if we couldn't find any levels to host
bool ServerGame::startHosting()
{
   if(!mLevelSource->isEmptyLevelDirOk() && mSettings->getFolderManager()->levelDir == "")   // No leveldir, no hosting!
      return false;

   S32 levelCount = mLevelSource->getLevelCount();

   for(S32 i = 0; i < levelCount; i++)
      logprintf(LogConsumer::ServerFilter, "\t%s [%s]", getLevelNameFromIndex(i).getString(), 
                mLevelSource->getLevelFileName(i).c_str());

   if(!levelCount)            // No levels loaded... we'll crash if we try to start a game       
      return false;      

   GameManager::setHostingModePhase(GameManager::NotHosting);
   cycleLevel(FIRST_LEVEL);   // Start with the first level

   return true;
}


void ServerGame::gameEnded()
{
   mLevelSwitchTimer.reset();
}


Ship *ServerGame::getLocalPlayerShip() const
{
   TNLAssert(false, "Cannot get local player's ship from a ServerGame!");
   return NULL;
}


// levelInfo should arrive fully populated
S32 ServerGame::addLevel(const LevelInfo &levelInfo)
{
   pair<S32, bool> ret = mLevelSource->addLevel(levelInfo);

   // ret.first is index of level, ret.second is true if the level was added, false if it already existed.
   // Level won't always be last in the list; if the level was already in the list, the index could be different.

   // Let levelChangers know about the new level if it was just added
   if(ret.second)
      for(S32 i = 0; i < getClientCount(); i++)
      {
         ClientInfo *clientInfo = getClientInfo(i);

         if(clientInfo->isLevelChanger())
            clientInfo->getConnection()->s2cAddLevel(levelInfo.mLevelName, levelInfo.mLevelType);
      }

   return ret.first;
}

void ServerGame::removeLevel(S32 index)
{
   if(index < 0)
   {
      while(mLevelSource->getLevelCount())
         mLevelSource->remove(0);
   }
   else
      mLevelSource->remove(index);

   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);
      if(clientInfo->isLevelChanger())
         clientInfo->getConnection()->s2cRemoveLevel(index);
   }
}


SFXHandle ServerGame::playSoundEffect(U32 profileIndex, F32 gain) const
{
   TNLAssert(false, "No sounds on a ServerGame!");
   return 0;
}


SFXHandle ServerGame::playSoundEffect(U32 profileIndex, const Point &position) const
{
   TNLAssert(false, "No sounds on a ServerGame!");
   return 0;
}


SFXHandle ServerGame::playSoundEffect(U32 profileIndex, const Point &position, const Point &velocity, F32 gain) const
{
   TNLAssert(false, "No sounds on a ServerGame!");
   return 0;
}


void ServerGame::queueVoiceChatBuffer(const SFXHandle &effect, const ByteBufferPtr &p) const
{
   TNLAssert(false, "No sounds on a ServerGame!");
}


LuaGameInfo *ServerGame::getGameInfo()
{
   // Lazily initialize
   if(!mGameInfo)
      mGameInfo = new LuaGameInfo(this);   // Deleted in destructor

   return mGameInfo;
}

///// 
//BotNavMeshZone management

GridDatabase *ServerGame::getBotZoneDatabase() const
{
   return mBotZoneDatabase;
}


const Vector<BotNavMeshZone *> *ServerGame::getBotZones() const
{
   return &mAllZones;
}


// Returns ID of zone containing specified point
U16 ServerGame::findZoneContaining(const Point &p) const
{
   fillVector.clear();
   mBotZoneDatabase->findObjects(BotNavMeshZoneTypeNumber, fillVector,
                                Rect(p - Point(0.1f, 0.1f), p + Point(0.1f, 0.1f)));  // Slightly extend Rect, it can be on the edge of zone

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      // First a quick, crude elimination check then more comprehensive one
      // Since our zones are convex, we can use the faster method!  Yay!
      // Actually, we can't, as it is not reliable... reverting to more comprehensive (and working) version.
      BotNavMeshZone *zone = static_cast<BotNavMeshZone *>(fillVector[i]);

      if(zone->getExtent().contains(p) &&
            (polygonContainsPoint(zone->getOutline()->address(), zone->getOutline()->size(), p)))
         return zone->getZoneId();
   }

   return U16_MAX;
}


void ServerGame::setGameType(GameType *gameType)
{
   Parent::setGameType(gameType);

   if(mGameRecorderServer)
      mGameRecorderServer->objectLocalScopeAlways(gameType);
}


void ServerGame::onObjectAdded(BfObject *obj)
{
   if(mGameRecorderServer && obj->isGhostable())
      mGameRecorderServer->objectLocalScopeAlways(obj);
}


void ServerGame::onObjectRemoved(BfObject *obj)
{
   if(mGameRecorderServer && obj->isGhostable())
      mGameRecorderServer->objectLocalClearAlways(obj);   
}
GameRecorderServer *ServerGame::getGameRecorder()
{
   return mGameRecorderServer;
}


};

