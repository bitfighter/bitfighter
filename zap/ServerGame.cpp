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
#include "Level.h"
#include "WallItem.h"

#include "GameObjectRender.h"
#include "stringUtils.h"
#include "GeomUtils.h"

#include "GameRecorder.h"

#include "IniFile.h"


using namespace TNL;

namespace Zap
{


static bool instantiated;           // Just a little something to keep us from creating multiple ServerGames...


// Constructor -- be sure to see Game constructor too!  Lots going on there!
ServerGame::ServerGame(const Address &address, GameSettingsPtr settings, LevelSourcePtr levelSource, bool testMode, bool dedicated, bool hostOnServer) : 
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
   mHostOnServer = hostOnServer;

   // Stupid C++ spec doesn't allow ternary logic with static const if there is no definition
   // Workaround is to add '+' to force a read of the value
   // See:  http://stackoverflow.com/questions/5446005/why-dont-static-member-variables-play-well-with-the-ternary-operator
   mNextLevel = mSettings->getSetting<YesNo>(IniKey::RandomLevels) ? +RANDOM_LEVEL : +NEXT_LEVEL;

   mShuttingDown = false;

   EventManager::get()->setPaused(false);

   mInfoFlags = 0;                           // Currently used to specify test mode and debug builds
   mCurrentLevelIndex = 0;

   if(testMode)
      mInfoFlags |= TestModeFlag;

   if(hostOnServer)
   {
      mInfoFlags |= HostModeFlag;
      mOriginalServerPassword = mSettings->getServerPassword();
      mOriginalName = mSettings->getHostName();
      mOriginalDescr = mSettings->getHostDescr();

      // Other hosts might override settings so lets make it so it never saves
      GameSettings::iniFile.SetPath(string(""));
   }

#ifdef TNL_DEBUG
   mInfoFlags |= DebugModeFlag;
#endif

   mTestMode = testMode;

   mNetInterface->setAllowsConnections(true);
   mMasterUpdateTimer.reset(UpdateServerStatusTime);

   // How long will teams stay locked after last admin departs?
   mNoAdminAutoUnlockTeamsTimer.setPeriod(TeamHistoryManager::LockedTeamsNoAdminsGracePeriod);

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

   instantiated = false;

   delete mGameInfo;

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

   for(S32 i = 0; i < fillVector.size(); i++)
      delete dynamic_cast<Object *>(fillVector[i]);

   mLevelSwitchTimer.clear();
   mScopeAlwaysList.clear();

   Parent::cleanUp();
}


// Return true when handled
bool ServerGame::voteStart(ClientInfo *clientInfo, VoteType type, S32 number)
{
   GameConnection *conn = clientInfo->getConnection();

   if(!mSettings->getSetting<YesNo>(IniKey::VotingEnabled))
      return false;

   U32 voteTimer;
   if(type == VoteChangeTeam)
      voteTimer = mSettings->getSetting<U32>(IniKey::VoteLengthToChangeTeam) * 1000;
   else
      voteTimer = mSettings->getSetting<YesNo>(IniKey::VoteLength) * 1000;

   if(voteTimer == 0)
      return false;

   if(type != VoteLevelChange)
   {
      if(getGameType()->isGameOver())
         return true;   // Don't allow trying to start votes during game over, except level changing

      if((U32)getGameType()->getRemainingGameTimeInMs() - 1 < voteTimer)  // Handles unlimited GameType time, by forcing the U32 range
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
      conn->s2cDisplayMessageESI(GameConnection::ColorRed, SFXNone, "Can't start vote, try again in %i0 seconds.", e, s, i);

      return true;
   }

   mVoteTimer = voteTimer;
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
   // Last level to process?
   if(mLevelLoadIndex == mLevelSource->getLevelCount())
   {
      TNLAssert(mHostOnServer, "Shouldn't be empty if not using -hostonserver");
      GameManager::setHostingModePhase(GameManager::DoneLoadingLevels);
      return string("No levels loaded");
   }

   // populateLevelInfoFromSource() will return true if the level was processed successfully
   string levelName;
   if(mLevelSource->populateLevelInfoFromSourceByIndex(mLevelLoadIndex))
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


void ServerGame::receivedLevelFromHoster(S32 levelIndex, const string &filename)
{
   if(levelIndex >= mLevelSource->getLevelCount())
      return; // out of range
   mLevelSource->setLevelFileName(levelIndex, filename);
   cycleLevel(levelIndex);
}


void ServerGame::makeEmptyLevelIfNoGameType()
{
   if(!getGameType())
   {
      GameType *gameType = new GameType(getLevel());
      gameType->addToGame(this, getLevel());
   }
   
   mLevel->makeSureTeamCountIsNotZero();
}


// Clear, prepare, and load the level given by the index \nextLevel. This
// function respects meta-indices, and otherwise expects an absolute index.
void ServerGame::cycleLevel(S32 nextLevel)
{
   if(mHostOnServer)
   {
      if(mHoster.isValid())
      {
         if(mLevelSource->getLevelCount() == 0)
         {
            if(getGameType()->isGameOver())
            {
               mShutdownTimer.reset(1); 
               mShuttingDown = true;
               mShutdownReason = "Host failed to send level list";
            }
            return; // we haven't cleared anything so its like level never changed, yet.
         }
         mCurrentLevelIndex = getAbsoluteLevelIndex(nextLevel);
         nextLevel = mCurrentLevelIndex;
         S32 hostLevelIndex = mLevelSource->getLevelInfo(mCurrentLevelIndex).mHosterLevelIndex;
         if(mLevelSource->getLevelFileName(mLevelSource->getLevelInfo(mCurrentLevelIndex).mHosterLevelIndex).length() == 0 && hostLevelIndex >= 0)
         {
            mHoster->s2cRequestLevel(hostLevelIndex);
            return;
         }
      }
      else if(getPlayerCount() == 0)
      {
         makeEmptyLevelIfNoGameType();
         return;
      }
      else
      {
         mShutdownTimer.reset(1); 
         mShuttingDown = true;
         mShutdownReason = "Host left game";
         return;
      }
   }

   delete mGameRecorderServer;
   mGameRecorderServer = NULL;

   // If mLevel is NULL, it's our first time here, and there won't be anything to clean up
   if(mLevel)
      cleanUp();

   // We moved the clearing code into cleanup()... I think it will always be run.  But better check!
   TNLAssert(mLevelSwitchTimer.getCurrent() == 0, "Expected this to be clear!");
   TNLAssert(mScopeAlwaysList.size() == 0,        "Expected this to be empty!");


   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);
      GameConnection *conn = clientInfo->getConnection();

      conn->resetGhosting();
      conn->switchedTeamCount = 0;

      clientInfo->setScore(0);         // Reset player scores, for non team game types
      clientInfo->clearKillStreak();   // Clear any rampage the players have going... sorry, lads!
   }

   // Beginning of next level...

   mRobotManager.onLevelChanged();

   if(!loadNextLevel(nextLevel))
      return;

   if(!mGameRecorderServer && !mShuttingDown && getSettings()->getSetting<YesNo>(IniKey::GameRecording))
      mGameRecorderServer = new GameRecorderServer(this);


   ////// This block could easily be moved off somewhere else   
   fillVector.clear();
   mLevel->findObjects(TeleporterTypeNumber, fillVector);

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
   getLevel()->findObjects((TestFunc)isWallType, barrierList, *getWorldExtents());

   Vector<DatabaseObject *> turretList;
   getLevel()->findObjects(TurretTypeNumber, turretList, *getWorldExtents());

   Vector<DatabaseObject *> forceFieldProjectorList;
   getLevel()->findObjects(ForceFieldProjectorTypeNumber, forceFieldProjectorList, *getWorldExtents());

   bool triangulate;

   // Try and load Bot Zones for this level, set flag if failed
   // We need to run buildBotMeshZones in order to set mAllZones properly, which is why I (sort of) disabled the use of hand-built zones in level files
#ifdef ZAP_DEDICATED
   triangulate = false;
#else
   triangulate = !isDedicated();
#endif

   TNLAssert(getGameType(), "Expect to have a GameType here!");
   getGameType()->mBotZoneCreationFailed = !BotNavMeshZone::buildBotMeshZones(mLevel->getBotZoneDatabase(), mLevel->getBotZoneList(),
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

   Vector<S32> unassigned;

   if(areTeamsLocked())
   {
      // Two passes!  All pre-assigned players get added first, then unassigned ones
      for(S32 i = 0; i < getClientCount(); i++)
      {
         ClientInfo *clientInfo = getClientInfo(i);

         if(mTeamHistoryManager.getTeam(clientInfo->getName().getString(), mLevel->getTeamCount()) != NO_TEAM)
            getGameType()->serverAddClient(clientInfo, &mTeamHistoryManager);
         else
            unassigned.push_back(i);
      }

      // Now assign any still-unassigned teams
      for(S32 i = 0; i < unassigned.size(); i++)
      {
         ClientInfo *clientInfo = getClientInfo(unassigned[i]);  

         getGameType()->serverAddClient(clientInfo, &mTeamHistoryManager);
      }
   }
   else  // Teams not locked, just add all the clients
   {
      // Backwards!  So the lowest scorer goes on the larger team (if there is uneven teams)
      for(S32 i = getClientCount() - 1; i > -1; i--)
         getGameType()->serverAddClient(getClientInfo(i), NULL);
   }

   // Initialize all our connections
   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);

      GameConnection *connection = clientInfo->getConnection();
      //TNLAssert(connection, "Why no connection here?");  // If this trips, comment circumstances and delete 3/10/2015 CE (trips during certain tests)
      if(connection)
      {
         connection->setObjectMovedThisGame(false);
         connection->activateGhosting();                 // Tell clients we're done sending objects and are ready to start playing
      }
   }

   // Fire onPlayerJoined event for any players already on the server
   for(S32 i = 0; i < getClientCount(); i++)
      EventManager::get()->fireEvent(NULL, EventManager::PlayerJoinedEvent, getClientInfo(i)->getPlayerInfo());

   mRobotManager.balanceTeams();

   sendLevelStatsToMaster();     // Give the master some information about this level for its database

   suspendIfNoActivePlayers();   // Does nothing if we're already suspended
}


// Find the next valid level, and load it with loadLevel()
bool ServerGame::loadNextLevel(S32 nextLevel)
{
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

         if(mHostOnServer)
         {
            makeEmptyLevelIfNoGameType();
            loaded = true;
         }
         else if(mLevelSource->getLevelCount() > 1)
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
            makeEmptyLevelIfNoGameType();

            return false;
         }
      }
   }

   computeWorldObjectExtents();                       // Compute world Extents nice and early
   return true;
}


// Returns true if the level is successfully loaded, false if it wasn't
bool ServerGame::loadLevel()
{
   mLevel = boost::shared_ptr<Level>(mLevelSource->getLevel(mCurrentLevelIndex));

   TNLAssert(!mLevel->getAddedToGame(), "Can't reuse Levels!");

   // NULL level means file was not loaded.  Danger Will Robinson!
   if(!mLevel)
   {
      logprintf(LogConsumer::LogError, "Error: Cannot load %s",
         mLevelSource->getLevelFileDescriptor(mCurrentLevelIndex).c_str());
      return false;
   }

   mLevel->onAddedToGame(this);     // Gets the TeamManager up and running and populated, adds bots


   // Add walls first, so engineered items will have something to snap to
   Vector<DatabaseObject *> walls;
   mLevel->findObjects(WallItemTypeNumber, walls);

   for(S32 i = 0; i < walls.size(); i++)
      addWallItem(static_cast<WallItem *>(walls[i]), NULL);        // Just does this --> Barrier::constructBarriers(this, *wallItem->getOutline(), false, wallItem->getWidth());


   Vector<Point> points;
   mLevel->buildWallEdgeGeometry(points);


   const Vector<DatabaseObject *> objects = *mLevel->findObjects_fast();
   for(S32 i = 0; i < objects.size(); i++)
   {
      // Walls have already been handled in addWallItem call above
      if(isWallType(objects[i]->getObjectTypeNumber()))
         continue;

      BfObject *object = static_cast<BfObject *>(objects[i]);

      // Mark the item as being a ghost (client copy of a server object) so that the object will not trigger server-side tests
      // The only time this code is run on the client is when loading into the editor.
      if(!isServer())
         object->markAsGhost();

      object->addToGame(this, NULL);
   }

   mLevel->addBots(this);

   // Levelgens:
   // Run level's levelgen script (if any)
   runLevelGenScript(getGameType()->getScriptName());

   // Global levelgens are run on every level.  Run any that are defined.
   Vector<string> scriptList;
   parseString(getSettings()->getSetting<string>(IniKey::GlobalLevelScript), scriptList, '|');

   for(S32 i = 0; i < scriptList.size(); i++)
      runLevelGenScript(scriptList[i]);

   // Fire an update to make sure certain events run on level start (like onShipSpawned)
   EventManager::get()->update();

   // Check after script, script might add or delete Teams
   if(mLevel->makeSureTeamCountIsNotZero())
      logprintf(LogConsumer::LogLevelError, "Warning: Missing Team in %s",
      mLevelSource->getLevelFileDescriptor(mCurrentLevelIndex).c_str());

   getGameType()->onLevelLoaded();

   return true;
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
   mSendLevelInfoDelayNetInfo = masterConn->s2mSendLevelInfo_construct(mLevelFileHash, mLevel->getLevelName(), 
                                mLevel->getLevelCredits(), 
                                getCurrentLevelTypeName(),
                                hasLevelGen, 
                                (U8)teamCountU8, 
                                mLevel->getWinningScore(), 
                                getRemainingGameTime());

   mSendLevelInfoDelayCount.reset(SIX_SECONDS);  // Set time left to send

   mSentHashes.push_back(mLevelFileHash);
}


// Resets all player team assignments
void ServerGame::resetAllClientTeams()
{
   for(S32 i = 0; i < getClientCount(); i++)
      getClientInfo(i)->setTeamIndex(NO_TEAM);
}


// Because team locking will last longer than one game, and because our GameType object goes 
// away at the end of each game, we need to track locking here.  Since we are tracking it here,
// we might as well do our client-side annoucements here as well.
void ServerGame::setTeamsLocked(bool locked)
{
   if(locked == areTeamsLocked())
      return;

   Parent::setTeamsLocked(locked);

   getGameType()->announceTeamsLocked(locked);

   if(locked)
   { 
      // Save current team configuration
      for(S32 i = 0; i < mClientInfos.size(); i++)
         mTeamHistoryManager.addPlayer(mClientInfos[i]->getName().getString(), 
                                       getTeamCount(), 
                                       mClientInfos[i]->getTeamIndex());
   }
   else
      mTeamHistoryManager.onTeamsUnlocked();
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

   if(skipUploads)   // Skip levels starting with UploadPrefix (currently "upload_")
      if(strncmp(levelInfo.filename.c_str(), UploadPrefix.c_str(), UploadPrefix.length()) == 0)
         return false;

   return true;
}


// Helps resolve pseudo-indices such as NEXT_LEVEL.  If you pass it a normal level index, you'll just get that back.
S32 ServerGame::getAbsoluteLevelIndex(S32 nextLevel)
{
   S32 currentLevelIndex = mCurrentLevelIndex;
   S32 levelCount = mLevelSource->getLevelCount();
   bool skipUploads = getSettings()->getSetting<YesNo>(IniKey::SkipUploads);

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


// ClientInfos will be stored as RefPtrs, so they will be deleted when all refs are removed... 
// In other words, ServerGame will manage cleanup.
// Called from onConnectionEstablished_server()
// onClientJoined // onPlayerJoined
void ServerGame::addClient(ClientInfo *clientInfo)
{
   TNLAssert(!clientInfo->isRobot(), "This only gets called for players");

   GameConnection *conn = clientInfo->getConnection();

   // If client has level change send a list of levels and their types to the connecting client
   if(clientInfo->isLevelChanger())
      conn->sendLevelList();

   // Let client know that teams are locked... if in fact they are locked.  
   // Also tell the TeamHistoryManager that we're here.
   if(areTeamsLocked())
   {
      conn->s2cTeamsLocked(true);
      mTeamHistoryManager.onPlayerJoined(clientInfo->getName().getString());
   }

   // If we're shutting down, display a notice to the user... but still let them connect normally
   if(mShuttingDown)
      conn->s2cInitiateShutdown(mShutdownTimer.getCurrent() / 1000,
            mShutdownOriginator.isNull() ? StringTableEntry() : mShutdownOriginator->getClientInfo()->getName(), 
            "Sorry -- server shutting down", false);

   TNLAssert(getGameType(), "No gametype?");     // Added 12/20/2013 by wat to try to understand if this ever happens

   getGameType()->serverAddClient(clientInfo, areTeamsLocked() ? &mTeamHistoryManager : NULL);
   addToClientList(clientInfo);

   if(anyAdminsInGame())
      mNoAdminAutoUnlockTeamsTimer.clear();

   mRobotManager.balanceTeams();
   
   // When a new player joins, game is always unsuspended!
   unsuspendIfActivePlayers();

   if(mDedicated)
      SoundSystem::playSoundEffect(SFXPlayerJoined, 1);
}


bool ServerGame::anyAdminsInGame() const
{
   for(S32 i = 0; i < mClientInfos.size(); i++)
      if(mClientInfos[i]->isAdmin())
         return true;

   return false;
}


// onClientQuit // onPlayerQuit
void ServerGame::removeClient(ClientInfo *clientInfo)
{
   TNLAssert(getGameType(), "Expect GameType here!");
   getGameType()->removeClient(clientInfo);

   if(getPlayerCount() == 0)     // Last player just quit, bummer!
      setTeamsLocked(false);     // Unlock now!
   else
   {
      if(!anyAdminsInGame())
         mNoAdminAutoUnlockTeamsTimer.reset();

      if(areTeamsLocked())
         mTeamHistoryManager.onPlayerQuit(clientInfo->getName().getString());
   }

   if(mDedicated)
      SoundSystem::playSoundEffect(SFXPlayerLeft, 1);

   if(mHostOnServer)
   {
      if(getPlayerCount() == 0)
      {
         delete mGameRecorderServer;
         mGameRecorderServer = NULL;
         cleanUp();

         removeLevel(-1);
         mCurrentLevelIndex = FIRST_LEVEL;
         mInfoFlags |= HostModeFlag;
         makeEmptyLevelIfNoGameType();

         mSettings->setServerPassword(mOriginalServerPassword, false);
         mSettings->setHostName(mOriginalName, false);
         mSettings->setHostDescr(mOriginalDescr, false);

         if(mMasterUpdateTimer.getCurrent() > UpdateServerWhenHostGoesEmpty)
            mMasterUpdateTimer.setPeriod(UpdateServerWhenHostGoesEmpty);
      }
   }
   else if(getPlayerCount() == 0 && !mShuttingDown && isDedicated())  // Only dedicated server can have zero players
      cycleLevel(mSettings->getSetting<YesNo>(IniKey::RandomLevels) ? +RANDOM_LEVEL : +NEXT_LEVEL);    // Advance to beginning of next level
   else
      mRobotManager.balanceTeams();
}


// Loop through all our bots and start their interpreters, delete those that sqawk
// Only called from GameType::onLevelLoaded()
void ServerGame::startAllBots()
{   
   // Do nothing -- remove this fn!
}


string ServerGame::addBot(const Vector<string> &args, ClientInfo::ClientClass clientClass)
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
   if(mHostOnServer && mShuttingDown)
   {
      if(mShutdownTimer.update(timeDelta) || onlyClientIs(mShutdownOriginator))
      {
         // Disconnect all clients, and  then this server will
         // move from main server list into "Host from server" list
         // Loop backwards to avoid skipping clients

         for(S32 i = getClientCount() - 1; i >= 0; i--)
         {
            ClientInfo *clientInfo = getClientInfo(i);
   
            if(!clientInfo->isRobot())
            {
               GameConnection *connection = clientInfo->getConnection();
               if(connection != NULL)
                  connection->disconnect(NetConnection::ReasonShutdown, mShutdownReason.c_str());
            }
         }
         mShuttingDown = false;
      }
      return false;
   }

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
   // (No music or voice on server!)
   //
   // Here, we process alerts for dedicated servers; Non-dedicated servers will emit sound from the client
   if(isDedicated())   
   {
      // Save volume here to avoid repeated lookup; it can't change without a restart, so this will work
      static const F32 volume = mSettings->getSetting<F32>(IniKey::AlertsVolume);
      SoundSystem::processAudio(volume);    
   }

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

   // Tick levelgen timers
   for(S32 i = 0; i < mLevelGens.size(); i++)
      mLevelGens[i]->tickTimer<LuaLevelGenerator>(timeDelta);

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
   
   const Vector<DatabaseObject *> *gameObjects = mLevel->findObjects_fast();

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

   TNLAssert(getGameType(), "Expect a GameType here!");
   getGameType()->idle(BfObject::ServerIdleMainLoop, timeDelta);

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

            if(!connection->getObjectMovedThisGame() &&        // Player hasn't moved in this level
                  !connection->isLocalConnection()   &&        // Don't kick the host, please!
                  connection->getBusyTime() > THIRTY_SECONDS)  // Player hasn't been busy for less than 30 seconds
            {
               connection->disconnect(NetConnection::ReasonIdle, "");
            }
         }
      }

      // Normalize ratings for this game
      getGameType()->updateRatings();
      cycleLevel(mNextLevel);
      mNextLevel = getSettings()->getSetting<YesNo>(IniKey::RandomLevels) ? +RANDOM_LEVEL : +NEXT_LEVEL;
   }

   // The host could leave the game in a middle of next level upload, then we have to shut down
   if(mHostOnServer && getGameType()->isGameOver() && mLevelSwitchTimer.getCurrent() == 0 && mHoster.isNull())
   {
      mShutdownTimer.reset(1);
      mShuttingDown = true;
      mShutdownReason = "Host left game";
      return;
   }

   if(mGameRecorderServer)
      mGameRecorderServer->idle(timeDelta);

   if(mNoAdminAutoUnlockTeamsTimer.update(timeDelta))
      setTeamsLocked(false);

   mTeamHistoryManager.idle(timeDelta);

   // Update to other clients right after idling everything else, so clients get more up to date information
   mNetInterface->processConnections(); 
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
         if((mVoteTimer - timeDelta) % 1000 > mVoteTimer % 1000) // show message
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
               if(mVoteNumber == 0)
                  msg = "/YES or /NO : %i0 : Set time to unlimited";
               else
               {
                  msg = "/YES or /NO : %i0 : Set time %i1 minutes %i2 seconds";
                  i.push_back(mVoteNumber / 60000);
                  i.push_back((mVoteNumber / 1000) % 60);
               }
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
               mVoteTimer = timeDelta + 1;   // No more waiting when everyone have voted
         }
         mVoteTimer -= timeDelta;
      }
      else                                   // Vote ends
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

         Settings<IniKey::SettingsItem> &settings = getSettings()->getIniSettings()->mSettings;

         bool votePass = (voteYes     * settings.getVal<YesNo>(IniKey::VoteYesStrength) + 
                          voteNo      * settings.getVal<YesNo>(IniKey::VoteNoStrength)  + 
                          voteNothing * settings.getVal<YesNo>(IniKey::VoteNothingStrength)) > 0;

         TNLAssert(getGameType(), "Expect GameType here!");

         if(votePass)
         {
            mVoteTimer = 0;
            switch(mVoteType)
            {
               case VoteLevelChange:
                  mNextLevel = mVoteNumber;
                  getGameType()->gameOverManGameOver();
                  break;

               case VoteAddTime:
                  getGameType()->extendGameTime(mVoteNumber);      // Increase "official time"
                  getGameType()->broadcastNewRemainingTime();   
                  break;   

               case VoteSetTime:
                  getGameType()->setGameTime(mVoteNumber);
                  getGameType()->broadcastNewRemainingTime();                                   
                  break;

               case VoteSetScore:
                  getGameType()->setWinningScore(mVoteNumber);
                  getGameType()->s2cChangeScoreToWin(mVoteNumber, mVoteClientName);     // Broadcast score to clients
                  break;

               case VoteChangeTeam:
                  for(S32 i = 0; i < getClientCount(); i++)
                  {
                     ClientInfo *clientInfo = getClientInfo(i);

                     if(clientInfo->getName() == mVoteClientName)
                        getGameType()->changeClientTeam(clientInfo, mVoteNumber);
                  }
                  break;

               case VoteResetScore:
                  if(getGameType()->getGameTypeId() != CoreGame)  // No changing score in Core
                  {
                     // Reset player scores
                     for(S32 i = 0; i < getClientCount(); i++)
                        // Broadcast any updated scores to the clients, and reset them to 0
                        if(getPlayerScore(i) != 0)
                        {
                           getGameType()->s2cSetPlayerScore(i, 0);
                           setPlayerScore(i, 0);
                        }

                     // Reset team scores
                     for(S32 i = 0; i < getTeamCount(); i++)
                        // Broadcast any updated scores to the clients, and reset them to 0
                        if(getTeam(i)->getScore() != 0)
                        {
                           getGameType()->s2cSetTeamScore(i, 0);
                           getTeam(i)->setScore(0);
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
                  conn->mVoteTime = settings.getVal<YesNo>(IniKey::VoteRetryLength) * 1000;
            }
         }
      }
   }
}


// Inform master of how things are hanging on this game server
void ServerGame::updateStatusOnMaster()
{
   MasterServerConnection *masterConn = getConnectionToMaster();

   static string prevCurrentLevelName;      // Using static, so it holds the value when it comes back here
   static GameTypeId prevCurrentLevelType;
   static S32 prevRobotCount;
   static S32 prevPlayerCount;

   if(masterConn && masterConn->isEstablished())
   {
      // Only update if something is different
      if(prevCurrentLevelName != getGameType()->getLevelName() ||
         prevCurrentLevelType != getGameType()->getGameTypeId() ||
         prevRobotCount       != getRobotCount() ||
         prevPlayerCount      != getPlayerCount())
      {
         prevCurrentLevelName = getGameType()->getLevelName();
         prevCurrentLevelType = getGameType()->getGameTypeId();
         prevRobotCount       = getRobotCount();
         prevPlayerCount      = getPlayerCount();

         masterConn->updateServerStatus(StringTableEntry(prevCurrentLevelName.c_str()), 
                                        GameType::getGameTypeName(prevCurrentLevelType), 
                                        prevRobotCount, 
                                        prevPlayerCount, 
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


static bool missingLevelDir(const LevelSource *levelSource, const GameSettings *settings)
{
   return !levelSource->isEmptyLevelDirOk() && settings->getFolderManager()->getLevelDir().empty();
}


// Returns true if things went well, false if we couldn't find any levels to host
bool ServerGame::startHosting()
{
   if(missingLevelDir(mLevelSource.get(), mSettings.get()))   // No leveldir, no hosting!
      return false;

   if(mHostOnServer)
   {
      GameManager::setHostingModePhase(GameManager::NotHosting);
      cycleLevel(FIRST_LEVEL);   // Start with the first level
      return true;
   }

   S32 levelCount = mLevelSource->getLevelCount();

   for(S32 i = 0; i < levelCount; i++)
      logprintf(LogConsumer::ServerFilter, "\t%s [%s]", getLevelNameFromIndex(i).getString(), 
                mLevelSource->getLevelFileName(i).c_str());

   if(levelCount == 0)        // No levels loaded... we'll crash if we try to start a game       
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

void ServerGame::levelAddedNotifyClients(const LevelInfo &levelInfo)
{
   // Let levelChangers know about the new level if it was just added
   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);
      GameConnection *conn = clientInfo->getConnection();

      if(clientInfo->isLevelChanger() && conn)
         conn->s2cAddLevel(levelInfo.mLevelName, levelInfo.mLevelType);
   }
}

// levelInfo should arrive fully populated
S32 ServerGame::addLevel(const LevelInfo &levelInfo)
{
   pair<S32, bool> ret = mLevelSource->addLevel(levelInfo);

   // ret.first is index of level, ret.second is true if the level was added, false if it already existed.
   // Level won't always be last in the list; if the level was already in the list, the index could be different.

   // Let levelChangers know about the new level if it was just added
   if(ret.second)
      levelAddedNotifyClients(levelInfo);

   return ret.first;
}

void ServerGame::addNewLevel(const LevelInfo &levelInfo)
{
   mLevelSource->addNewLevel(levelInfo);
   levelAddedNotifyClients(levelInfo);
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

const Vector<BotNavMeshZone *> &ServerGame::getBotZoneList() const
{
   return mLevel->getBotZoneList();
}


GridDatabase &ServerGame::getBotZoneDatabase() const
{
   return mLevel->getBotZoneDatabase();
}


// Returns ID of zone containing specified point
U16 ServerGame::findZoneContaining(const Point &p) const
{
   fillVector.clear();
   mLevel->getBotZoneDatabase().findObjects(BotNavMeshZoneTypeNumber, fillVector,
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


// We get alerted whenever a client has changed roles.  Neat!
void ServerGame::onClientChangedRoles(ClientInfo *clientInfo)
{
   // Though unlikely, it is possible that player was demoted -- so don't make
   // assumptions about his new role, or whether there are other admins in the game.
   if(anyAdminsInGame())
      mNoAdminAutoUnlockTeamsTimer.clear();     // Stop timer
   else if(mNoAdminAutoUnlockTeamsTimer.getCurrent() == 0)
      mNoAdminAutoUnlockTeamsTimer.reset();     // Start timer
}


GameRecorderServer *ServerGame::getGameRecorder()
{
   return mGameRecorderServer;
}


};

