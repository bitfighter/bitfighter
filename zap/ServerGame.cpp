//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "ServerGame.h"
#include "barrier.h"
#include "config.h"
#include "EngineeredItem.h"      // For EngineerModuleDeployer
#include "gameLoader.h"
#include "gameNetInterface.h"
#include "BfObject.h"
#include "gameObjectRender.h"
#include "gameType.h"
#include "masterConnection.h"
#include "move.h"
#include "moveObject.h"
#include "projectile.h"          // For SpyBug class
#include "SoundSystem.h"
#include "SharedConstants.h"     // For ServerInfoFlags enum
#include "ship.h"
#include "GeomUtils.h"
#include "luaLevelGenerator.h"
#include "robot.h"
#include "shipItems.h"           // For moduleInfos
#include "stringUtils.h"
#include "NexusGame.h"           // For creating new NexusFlagItem
#include "teamInfo.h"            // For TeamManager def
#include "playerInfo.h"
#include "Zone.h"                // For instantiating zones

#include "ClientInfo.h"

#include "IniFile.h"             // For CIniFile def
#include "BanList.h"             // For banList kick duration

#include "BotNavMeshZone.h"      // For zone clearing code

#include "WallSegmentManager.h"

#include "tnl.h"
#include "tnlRandom.h"
#include "tnlGhostConnection.h"
#include "tnlNetInterface.h"

#include "md5wrapper.h"

#include <boost/shared_ptr.hpp>
#include <sys/stat.h>
#include <cmath>


#include "soccerGame.h"


#ifdef PRINT_SOMETHING
#include "ClientGame.h"  // only used to print some variables in ClientGame...
#endif


using namespace TNL;

namespace Zap
{


// Constructor
ServerGame::ServerGame(const Address &address, GameSettings *settings, bool testMode, bool dedicated) : 
      Game(address, settings)
{
   mVoteTimer = 0;

   setAddTarget();         // When we do an addToGame, objects should be added to ServerGame


   // Stupid c++ spec doesn't allow ternary logic with static const if there is no definition
   // Workaround is to add '+' to force a read of the value
   // See:  http://stackoverflow.com/questions/5446005/why-dont-static-member-variables-play-well-with-the-ternary-operator
   mNextLevel = settings->getIniSettings()->randomLevels ? +RANDOM_LEVEL : +NEXT_LEVEL;

   mShuttingDown = false;

   EventManager::get()->setPaused(false);

   hostingModePhase = ServerGame::NotHosting;

   mInfoFlags = 0;                  // Currently only used to specify test mode
   mCurrentLevelIndex = 0;

   if(testMode)
      mInfoFlags = TestModeFlag;

   mTestMode = testMode;

   mNetInterface->setAllowsConnections(true);
   mMasterUpdateTimer.reset(UpdateServerStatusTime);

   mSuspendor = NULL;

#ifdef ZAP_DEDICATED
   TNLAssert(dedicated, "Dedicated should be true here!");
#endif

   mDedicated = dedicated;

   mGameSuspended = true; // server starts at zero players

   U32 stutter = mSettings->getSimulatedStutter();

   mStutterTimer.reset(1001 - stutter);    // Use 1001 to ensure timer is never set to 0
   mStutterSleepTimer.reset(stutter);
   mAccumulatedSleepTime = 0;

   botControlTickTimer.reset(BotControlTickInterval);
}


// Destructor
ServerGame::~ServerGame()
{
   if(getConnectionToMaster()) // Prevents errors when ServerGame is gone too soon.
      getConnectionToMaster()->disconnect(NetConnection::ReasonSelfDisconnect, "");
   cleanUp();

   clearAddTarget();
}


void ServerGame::cleanUp()
{
   fillVector.clear();
   mDatabaseForBotZones.findObjects(fillVector);

   mLevelGens.deleteAndClear();

   for(S32 i = 0; i < fillVector.size(); i++)
      delete dynamic_cast<Object *>(fillVector[i]);

   Parent::cleanUp();
}


// Return true when handled
bool ServerGame::voteStart(ClientInfo *clientInfo, S32 type, S32 number)
{
   GameConnection *conn = clientInfo->getConnection();

   if(!mSettings->getIniSettings()->voteEnable)
      return false;

   U32 VoteTimer;
   if(type == 4) // change team
      VoteTimer = mSettings->getIniSettings()->voteLengthToChangeTeam * 1000;
   else
      VoteTimer = mSettings->getIniSettings()->voteLength * 1000;
   if(VoteTimer == 0)
      return false;

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
      if(getClientInfo(i)->getConnection())  // robots don't have GameConnection
         getClientInfo(i)->getConnection()->mVote = 0;

   conn->mVote = 1;
   conn->s2cDisplayMessage(GameConnection::ColorAqua, SFXNone, "Vote started, waiting for others to vote.");
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


S32 ServerGame::getLevelNameCount()
{
   return mLevelInfos.size();
}


S32 ServerGame::getCurrentLevelIndex()
{
   return mCurrentLevelIndex;
}


S32 ServerGame::getLevelCount()
{
   return mLevelInfos.size();
}


LevelInfo ServerGame::getLevelInfo(S32 index)
{
   return mLevelInfos[index];
}


void ServerGame::clearLevelInfos()
{
   mLevelInfos.clear();
}


void ServerGame::addLevelInfo(const LevelInfo &levelInfo)
{
   mLevelInfos.push_back(levelInfo);
}


bool ServerGame::isTestServer()
{
   return mTestMode;
}


AbstractTeam *ServerGame::getNewTeam()
{
   return new Team;
}


// Creates a set of LevelInfos that are empty except for the filename.  They will be fleshed out later.
// This gets called when you first load the host menu
void ServerGame::buildBasicLevelInfoList(const Vector<string> &levelList)
{
   mLevelInfos.clear();

   for(S32 i = 0; i < levelList.size(); i++)
      mLevelInfos.push_back(LevelInfo(levelList[i]));
}


void ServerGame::resetLevelLoadIndex()
{
   mLevelLoadIndex = 0;
}


// This is only used while we're building a list of levels to display on the host during loading.
string ServerGame::getLastLevelLoadName()
{
   if(mLevelInfos.size() == 0)     // Could happen if there are no valid levels specified wtih -levels param, for example
      return "";
   else if(mLevelLoadIndex == 0)    // Still not sure when this would happen
      return "";
   else
      return mLevelInfos[mLevelLoadIndex - 1].mLevelName.getString();
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


// Parse through the chunk of data passed in and find parameters to populate levelInfo with
// This is only used on the server to provide quick level information without having to load the level
// (like with playlists or menus)
// Warning: Mungs chunk!
LevelInfo getLevelInfoFromFileChunk(char *chunk, S32 size, LevelInfo &levelInfo)
{
   S32 cur = 0;
   S32 startingCur = 0;

   bool foundGameType = false;
   bool foundLevelName = false;
   bool foundMinPlayers = false;
   bool foundMaxPlayers = false;

   while(cur < size && !(foundGameType && foundLevelName && foundMinPlayers && foundMaxPlayers))
   {
      if(chunk[cur] < 32)
      {
         if(cur - startingCur > 5)
         {
            char c = chunk[cur];
            chunk[cur] = 0;
            Vector<string> list = parseString(&chunk[startingCur]);
            chunk[cur] = c;

            if(list.size() >= 1 && list[0].find("GameType") != string::npos)
            {
               // Convert legacy Hunters game
               if(!stricmp(list[0].c_str(), "HuntersGameType"))
                  list[0] = "NexusGameType";

               TNL::Object *theObject = TNL::Object::create(list[0].c_str());  // Instantiate a gameType object
               GameType *gt = dynamic_cast<GameType*>(theObject);              // and cast it
               if(gt)
               {
                  levelInfo.mLevelType = gt->getGameTypeId();
                  foundGameType = true;
               }

               delete theObject;
            }
            else if(list.size() >= 2 && list[0] == "LevelName")
            {
               string levelName = list[1];

               for(S32 i = 2; i < list.size(); i++)
                  levelName += " " + list[i];

               levelInfo.mLevelName = levelName;

               foundLevelName = true;
            }
            else if(list.size() >= 2 && list[0] == "MinPlayers")
            {
               levelInfo.minRecPlayers = atoi(list[1].c_str());
               foundMinPlayers = true;
            }
            else if(list.size() >= 2 && list[0] == "MaxPlayers")
            {
               levelInfo.maxRecPlayers = atoi(list[1].c_str());
               foundMaxPlayers = true;
            }
         }
         startingCur = cur + 1;
      }
      cur++;
   }
   return levelInfo;
}


void ServerGame::loadNextLevelInfo()
{
   FolderManager *folderManager = getSettings()->getFolderManager();

   string levelFile = folderManager->findLevelFile(mLevelInfos[mLevelLoadIndex].mLevelFileName.getString());

   if(getLevelInfo(levelFile, mLevelInfos[mLevelLoadIndex]))    // Populate mLevelInfos[i] with data from levelFile
      mLevelLoadIndex++;
   else
      mLevelInfos.erase(mLevelLoadIndex);

   if(mLevelLoadIndex == mLevelInfos.size())
      hostingModePhase = DoneLoadingLevels;
}


// Populates levelInfo with data from fullFilename
bool ServerGame::getLevelInfo(const string &fullFilename, LevelInfo &levelInfo)
{
   TNLAssert(levelInfo.mLevelFileName != "", "Invalid assumption");

   FILE *f = fopen(fullFilename.c_str(), "rb");

   if(f)
   {
      char data[1024 * 4];  // 4 kb should be enough to fit all parameters at the beginning of level; we don't need to read everything
      S32 size = (S32)fread(data, 1, sizeof(data), f);
      fclose(f);

      getLevelInfoFromFileChunk(data, size, levelInfo);

      // Provide a default levelname
      if(levelInfo.mLevelName == "")
         levelInfo.mLevelName = levelInfo.mLevelFileName;   

      return true;
   }
   else
   {
      // was mLevelInfos[mLevelLoadIndex].levelFileName.getString()
      logprintf(LogConsumer::LogWarning, "Could not load level %s [%s].  Skipping...", levelInfo.mLevelFileName.getString(), fullFilename.c_str());
      return false;
   }
}


// Get the level name, as defined in the level file
StringTableEntry ServerGame::getLevelNameFromIndex(S32 indx)
{
   return mLevelInfos[getAbsoluteLevelIndex(indx)].mLevelName;
}


// Get the filename the level is saved under
string ServerGame::getLevelFileNameFromIndex(S32 indx)
{
   if(indx < 0 || indx >= mLevelInfos.size())
      return "";
   else
      return mLevelInfos[indx].mLevelFileName.getString();
}


// Return filename of level currently in play
StringTableEntry ServerGame::getCurrentLevelFileName()
{
   return mLevelInfos[mCurrentLevelIndex].mLevelFileName;
}


// Return name of level currently in play
StringTableEntry ServerGame::getCurrentLevelName()
{
   return mLevelInfos[mCurrentLevelIndex].mLevelName;
}


// Return type of level currently in play
GameTypeId ServerGame::getCurrentLevelType()
{
   return mLevelInfos[mCurrentLevelIndex].mLevelType;
}


StringTableEntry ServerGame::getCurrentLevelTypeName()
{
   return GameType::getGameTypeName(getCurrentLevelType());
}


bool ServerGame::processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database)
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
   else if(!stricmp(argv[0], "Zone")) 
   {
      Zone *zone = new Zone();

      if(zone->processArguments(argc - 1, argv + 1, this))
         getGameType()->addZone(zone);
   }

   else 
      return false;

   return true;
}


void ServerGame::addPolyWall(PolyWall *polyWall, GridDatabase *unused)
{
   polyWall->addToGame(this, this->getGameObjDatabase());
   // Convert the wallItem in to a wallRec, an abbreviated form of wall that represents both regular walls and polywalls, and 
   // is convenient to transmit to the clients
   //WallRec wallRec(polyWall);
   //getGameType()->addWall(wallRec, this);
}


void ServerGame::addWallItem(WallItem *wallItem, GridDatabase *unused)
{
   wallItem->addToGame(this, this->getGameObjDatabase());

   // Convert the wallItem in to a wallRec, an abbreviated form of wall that represents both regular walls and polywalls, and 
   // is convenient to transmit to the clients
   //WallRec wallRec(wallItem);
   //getGameType()->addWall(wallRec, this);
}


// Sort by order in which players should be added to teams
// Highest ratings first -- runs on server only, so these should be FullClientInfos
// Return 1 if a is above b, -1 if b is above a, and 0 if they are equal
static S32 QSORT_CALLBACK AddOrderSort(RefPtr<ClientInfo> *a, RefPtr<ClientInfo> *b)
{
   bool aIsIdle = !(*a)->getConnection() || !(*a)->getConnection()->getObjectMovedThisGame();
   bool bIsIdle = !(*b)->getConnection() || !(*b)->getConnection()->getObjectMovedThisGame();

   // If either player is idle, put them at the bottom of the list
   if(aIsIdle && !bIsIdle)
      return -1;

   if(!aIsIdle && bIsIdle)
      return 1;

   // If neither (or both) are idle, sort by ranking
   F32 diff = (*a)->getCalculatedRating() - (*b)->getCalculatedRating();

   if(diff == 0) 
      return 0;

   return diff > 0 ? 1 : -1;
}


// Pass -1 to go to next level, otherwise pass an absolute level number
void ServerGame::cycleLevel(S32 nextLevel)
{
   cleanUp();
   mLevelSwitchTimer.clear();
   mScopeAlwaysList.clear();

   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);
      GameConnection *conn = clientInfo->getConnection();

      conn->resetGhosting();
      conn->switchedTeamCount = 0;

      clientInfo->setScore(0); // Reset player scores, for non team game types
   }

   mCurrentLevelIndex = getAbsoluteLevelIndex(nextLevel); // Set mCurrentLevelIndex to refer to the next level we'll play

   string levelFile = getLevelFileNameFromIndex(mCurrentLevelIndex);

   logprintf(LogConsumer::ServerFilter, "Loading %s [%s]... \\", getLevelNameFromIndex(mCurrentLevelIndex).getString(), levelFile.c_str());

   // Load the level for real this time (we loaded it once before, when we started the server, but only to grab a few params)
   loadLevel(levelFile);

   // Make sure we have a gameType... if we don't we'll add a default one here
   if(!getGameType())   // loadLevel can fail (missing file) and not create GameType
   {
      logprintf(LogConsumer::LogLevelError, "Warning: Missing game type parameter in level \"%s\"", levelFile.c_str());

      GameType *gameType = new GameType;
      gameType->addToGame(this, getGameObjDatabase());
   }

   if(getGameType()->makeSureTeamCountIsNotZero())
      logprintf(LogConsumer::LogLevelError, "Warning: Missing team in level \"%s\"", levelFile.c_str());

   logprintf(LogConsumer::ServerFilter, "Done. [%s]", getTimeStamp().c_str());

   computeWorldObjectExtents();                       // Compute world Extents nice and early

   // Try and load Bot Zones for this level, set flag if failed
   // We need to run buildBotMeshZones in order to set mAllZones properly, which is why I (sort of) disabled the use of hand-built zones in level files
#ifdef ZAP_DEDICATED
   mGameType->mBotZoneCreationFailed = !BotNavMeshZone::buildBotMeshZones(this, false);
#else
   mGameType->mBotZoneCreationFailed = !BotNavMeshZone::buildBotMeshZones(this, !isDedicated());
#endif

   // Clear team info for all clients
   resetAllClientTeams();

   // Reset loadouts now that we have GameType set up
   bool levelHasLoadoutZone = getGameType()->levelHasLoadoutZone();
   for(S32 i = 0; i < getClientCount(); i++)
      getClientInfo(i)->resetLoadout(levelHasLoadoutZone);
  

   // Now add players to the gameType, from highest rating to lowest in an attempt to create ratings-based teams
   // Sorting also puts idle players at the end of the list, regardless of their rating
   mClientInfos.sort(AddOrderSort);

   if(mGameType.isValid())
      for(S32 i = 0; i < getClientCount(); i++)
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

   logprintf(LogConsumer::MsgType(LogConsumer::LogConnection | LogConsumer::ServerFilter), "Server established connection with Master Server");
}


void ServerGame::sendLevelStatsToMaster()
{
   // Send level stats to master, but don't bother in test mode -- don't want to gum things up with a bunch of one-off levels
   if(mTestMode)
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
   mSendLevelInfoDelayNetInfo = masterConn->s2mSendLevelInfo_construct(mLevelFileHash, mGameType->getLevelName()->getString(), 
                                mGameType->getLevelCredits()->getString(), 
                                getCurrentLevelTypeName(),
                                hasLevelGen, 
                                (U8)teamCountU8, 
                                mGameType->getWinningScore(), 
                                mGameType->getRemainingGameTime());

   mSendLevelInfoDelayCount.reset(6000);  // set time left to send

   mSentHashes.push_back(mLevelFileHash);
}


// Resets all player team assignments
void ServerGame::resetAllClientTeams()
{
   for(S32 i = 0; i < getClientCount(); i++)
      getClientInfo(i)->setTeamIndex(NO_TEAM);
}


static bool checkIfLevelIsOk(ServerGame *game, const LevelInfo &levelInfo, S32 playerCount)
{
   S32 minPlayers = levelInfo.minRecPlayers;
   S32 maxPlayers = levelInfo.maxRecPlayers;

   if(maxPlayers <= 0)        // i.e. limit doesn't apply or is invalid (note if limit doesn't apply on the minPlayers, 
      maxPlayers = S32_MAX;   // then it works out because the smallest number of players is 1).

   if(playerCount < minPlayers)
      return false;
   if(playerCount > maxPlayers)
      return false;

   if(game->getSettings()->getIniSettings()->skipUploads)
      if(!strncmp(levelInfo.mLevelFileName.getString(), "upload_", 7))
         return false;

   return true;

}


// Helps resolve meta-indices such as NEXT_LEVEL.  If you pass it a normal level index, you'll just get that back.
S32 ServerGame::getAbsoluteLevelIndex(S32 nextLevel)
{
   S32 CurrentLevelIndex = mCurrentLevelIndex;
   if(nextLevel >= FIRST_LEVEL && nextLevel < mLevelInfos.size())          // Go to specified level
      CurrentLevelIndex = (nextLevel < mLevelInfos.size()) ? nextLevel : FIRST_LEVEL;

   else if(nextLevel == NEXT_LEVEL)      // Next level
   {
      // If game is supended, then we are waiting for another player to join.  That means that (probably)
      // there are either 0 or 1 players, so the next game will need to be good for 1 or 2 players.
      S32 playerCount = getPlayerCount();
      if(mGameSuspended)
         playerCount++;

      bool first = true;
      bool found = false;

      S32 currLevel = CurrentLevelIndex;

      // Cycle through the levels looking for one that matches our player counts
      while(first || CurrentLevelIndex != currLevel)
      {
         CurrentLevelIndex = (CurrentLevelIndex + 1) % mLevelInfos.size();

         if(checkIfLevelIsOk(this, mLevelInfos[CurrentLevelIndex], playerCount))
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
         CurrentLevelIndex++;
         if(S32(CurrentLevelIndex) >= mLevelInfos.size())
            CurrentLevelIndex = FIRST_LEVEL;
      }
   } 
   else if(nextLevel == PREVIOUS_LEVEL)
   {
      CurrentLevelIndex--;
      if(CurrentLevelIndex < 0)
         CurrentLevelIndex = mLevelInfos.size() - 1;
   }
   else if(nextLevel == RANDOM_LEVEL)
   {
      S32 newLevel;
      U32 RetryLeft = 200;
      S32 playerCount = getPlayerCount();
      if(mGameSuspended)
         playerCount++;

      do
      {
         newLevel = TNL::Random::readI(0, mLevelInfos.size() - 1);
         RetryLeft--;  // once we hit zero, this loop should exit to prevent endless loop.

      } while(RetryLeft != 0 && (CurrentLevelIndex == newLevel || !checkIfLevelIsOk(this, mLevelInfos[CurrentLevelIndex], playerCount)));
      CurrentLevelIndex = newLevel;
   }

   //else if(nextLevel == REPLAY_LEVEL)    // Replay level, do nothing
   //   CurrentLevelIndex += 0;

   if(CurrentLevelIndex >= mLevelInfos.size())  // some safety check in case of trying to replay the level that have just deleted
      CurrentLevelIndex = 0;
   return CurrentLevelIndex;
}

// Enter suspended animation mode
void ServerGame::suspendGame()
{
   mGameSuspended = true;
   cycleLevel(getSettings()->getIniSettings()->randomLevels ? +RANDOM_LEVEL : +NEXT_LEVEL);    // Advance to beginning of next level
}


 // Suspend at player's request
void ServerGame::suspendGame(GameConnection *requestor)
{
   if(getPlayerCount() > 1)        // Should never happen, but will protect against hacked clients
      return;

   mGameSuspended = true;    
   mSuspendor = requestor;
   //mCommanderZoomDelta = CommanderMapZoomTime;     // When suspended, we show cmdr's map, need to make sure it's fully zoomed out

   cycleLevel(REPLAY_LEVEL);  // Restart current level to make setting traps more difficult
}

 
// Resume game after it is no longer suspended
void ServerGame::unsuspendGame(bool remoteRequest)
{
   if(!mGameSuspended)
      return;

   mGameSuspended = false;
   if(mSuspendor && !remoteRequest)     // If the request is from remote server, don't need to alert that server!
      mSuspendor->s2cUnsuspend();


   // Alert any spawn-delayed clients that the game has resumed without them...
   for(S32 i = 0; i < getClientCount(); i++)
      if(getClientInfo(i)->isSpawnDelayed())
         if(mSuspendor != getClientInfo(i)->getConnection())
            getClientInfo(i)->getConnection()->s2cSuspendGame(true);

   mSuspendor = NULL;
}


void ServerGame::suspenderLeftGame()
{
   mSuspendor = NULL;
}


GameConnection *ServerGame::getSuspendor()
{
   return mSuspendor;
}


// Check to see if there are any players who are active; suspend the game if not.  Server only.
void ServerGame::suspendIfNoActivePlayers()
{
   if(mGameSuspended)
      return;

   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);

      if(!clientInfo->isRobot() && !clientInfo->isSpawnDelayed())
         return;
   }

   // No active players at the moment... mark game as suspended, and alert players
   mGameSuspended = true;

   if(getPlayerCount() == 0)
      suspendGame();
   else
   {
      // Alert any connected players
      for(S32 i = 0; i < getClientCount(); i++)
         if(!getClientInfo(i)->isRobot())  
            getClientInfo(i)->getConnection()->s2cSuspendGame(false);          
   }
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


extern md5wrapper md5;

bool ServerGame::loadLevel(const string &levelFileName)
{
   FolderManager *folderManager = getSettings()->getFolderManager();

   resetLevelInfo();

   mObjectsLoaded = 0;

   string filename = folderManager->findLevelFile(levelFileName);

   //cleanUp();
   if(filename == "")
   {
      logprintf("Unable to find level file \"%s\".  Skipping...", levelFileName.c_str());
      return false;
   }

#ifdef PRINT_SOMETHING
   logprintf("1 server: %d, client %d", gServerGame->getGameObjDatabase()->getObjectCount(),gClientGame->getGameObjDatabase()->getObjectCount());
#endif
   if(loadLevelFromFile(filename, getGameObjDatabase()))
      mLevelFileHash = md5.getHashFromFile(filename);    // TODO: Combine this with the reading of the file we're doing anyway in initLevelFromFile()
   else
   {
      logprintf("Unable to process level file \"%s\".  Skipping...", levelFileName.c_str());
      return false;
   }
#ifdef PRINT_SOMETHING
   logprintf("2 server: %d, client %d", gServerGame->getGameObjDatabase()->getObjectCount(),gClientGame->getGameObjDatabase()->getObjectCount());
#endif

   // We should have a gameType by the time we get here... but in case we don't, we'll add a default one now
   if(!getGameType())
   {
      logprintf(LogConsumer::LogWarning, "Warning: Missing game type parameter in level \"%s\"", levelFileName.c_str());
      GameType *gameType = new GameType;
      gameType->addToGame(this, getGameObjDatabase());
   }


   // Levelgens:
   runLevelGenScript(getGameType()->getScriptName());                   // Run level's levelgen script (if any)
   runLevelGenScript(mSettings->getIniSettings()->globalLevelScript);   // And our global levelgen (if defined)

   // Check after script, script might add Teams
   if(getGameType()->makeSureTeamCountIsNotZero())
      logprintf(LogConsumer::LogWarning, "Warning: Missing Team in level \"%s\"", levelFileName.c_str());

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
   LuaLevelGenerator *levelgen = new LuaLevelGenerator(fullname, *getGameType()->getScriptArgs(), getGridSize(), 
                                                       getGameObjDatabase(), this);

   if(!levelgen->runScript())
      delete levelgen;
   else
      mLevelGens.push_back(levelgen);
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
      conn->s2cInitiateShutdown(mShutdownTimer.getCurrent() / 1000, mShutdownOriginator->getClientInfo()->getName(), 
                                         "Sorry -- server shutting down", false);
   if(mGameType.isValid())
   {
      mGameType->serverAddClient(clientInfo);
      addToClientList(clientInfo);
   }
   
   // When a new player joins, game is always unsuspended!
   if(mGameSuspended)
      unsuspendGame(false);

   if(mDedicated)
      SoundSystem::playSoundEffect(SFXPlayerJoined, 1);
}


void ServerGame::removeClient(ClientInfo *clientInfo)
{
   if(mGameType.isValid())
      mGameType->serverRemoveClient(clientInfo);

   if(mDedicated)
      SoundSystem::playSoundEffect(SFXPlayerLeft, 1);
}


// Loop through all our bots and start their interpreters, delete those that sqawk
// Only called from GameType::onLevelLoaded()
void ServerGame::startAllBots()
{   
   for(S32 i = 0; i < mRobots.size(); i++)
      if(!mRobots[i]->start())
      {
         mRobots.erase_fast(i);
         i--;
      }
}


U32 ServerGame::getMaxPlayers()
{
   return mSettings->getMaxPlayers();
}


bool ServerGame::isDedicated()
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
bool ServerGame::isReadyToShutdown(U32 timeDelta)
{
   return mShuttingDown && (mShutdownTimer.update(timeDelta) || onlyClientIs(mShutdownOriginator));
}


bool ServerGame::isServer()
{
   return true;
}


// Top-level idle loop for server, runs only on the server by definition
void ServerGame::idle(U32 timeDelta)
{
   // Simulate CPU stutter without impacting gClientGame
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
            case 0:
               msg = "/YES or /NO : %i0 : Change Level to %e0";
               e.push_back(getLevelNameFromIndex(mVoteNumber));
               break;
            case 1:
               msg = "/YES or /NO : %i0 : Add time %i1 minutes";
               i.push_back(mVoteNumber / 60000);
               break;
            case 2:
               msg = "/YES or /NO : %i0 : Set time %i1 minutes %i2 seconds";
               i.push_back(mVoteNumber / 60000);
               i.push_back((mVoteNumber / 1000) % 60);
               break;
            case 3:
               msg = "/YES or /NO : %i0 : Set score %i1";
               i.push_back(mVoteNumber);
               break;
            case 4:
               msg = "/YES or /NO : %i0 : Change team %e0 to %e1";
               e.push_back(mVoteClientName);
               e.push_back(getTeamName(mVoteNumber));
               break;
            case 5:
               msg = "/YES or /NO : %i0 : %s0";
               s.push_back(mVoteString);
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
                  conn->s2cDisplayMessageESI(GameConnection::ColorAqua, SFXNone, msg, e, s, i);
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
               case 0:
                  mNextLevel = mVoteNumber;
                  if(mGameType)
                     mGameType->gameOverManGameOver();
                  break;
               case 1:
                  if(mGameType)
                  {
                     mGameType->extendGameTime(mVoteNumber);                           // Increase "official time"
                     mGameType->broadcastNewRemainingTime();   
                  }
                  break;   
               case 2:
                  if(mGameType)
                  {
                     mGameType->extendGameTime(S32(mVoteNumber - mGameType->getRemainingGameTimeInMs()));
                     mGameType->broadcastNewRemainingTime();                                   
                  }
                  break;
               case 3:
                  if(mGameType)
                  {
                     mGameType->setWinningScore(mVoteNumber);
                     mGameType->s2cChangeScoreToWin(mVoteNumber, mVoteClientName);     // Broadcast score to clients
                  }
                  break;
               case 4:
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
               case 5:
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
               conn->s2cDisplayMessageESI(GameConnection::ColorAqua, SFXNone, "Vote %e0 - %i0 yes, %i1 no, %i2 did not vote", e, s, i);

               if(!votePass && clientInfo->getName() == mVoteClientName)
                  conn->mVoteTime = mSettings->getIniSettings()->voteRetryLength * 1000;
            }
         }
      }
   }


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
   if(timeDelta > 2000)   // Prevents timeDelta from going too high, usually when after the server was frozen
      timeDelta = 100;

   mNetInterface->checkIncomingPackets();
   checkConnectionToMaster(timeDelta);                   // Connect to master server if not connected

   mSettings->getBanList()->updateKickList(timeDelta);   // Unban players who's bans have expired

   // Periodically update our status on the master, so they know what we're doing...
   if(mMasterUpdateTimer.update(timeDelta))
      updateStatusOnMaster();

   mNetInterface->processConnections();

   // If we have a data transfer going on, process it
   if(!dataSender.isDone())
      dataSender.sendNextLine();

   // Play any sounds server might have made... (this is only for special alerts such as player joined or left)
   if(isDedicated())   // Non-dedicated servers will process sound in client side
      SoundSystem::processAudio(mSettings->getIniSettings()->alertsVolLevel);    // No music or voice on server!

   if(mGameSuspended)     // If game is suspended, we need do nothing more
      return;


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

   // Compute new world extents -- these might change if a ship flies far away, for example...
   // In practice, we could probably just set it and forget it when we load a level.
   // Compute it here to save recomputing it for every robot and other method that relies on it.
   computeWorldObjectExtents();

   U32 botControlTickelapsed = botControlTickTimer.getElapsed();

   if(botControlTickTimer.update(timeDelta))
   {
      // Clear all old bot moves, so that if the bot does nothing, it doesn't just continue with what it was doing before
      clearBotMoves();

      // Fire TickEvent, in case anyone is listening
      EventManager::get()->fireEvent(EventManager::TickEvent, botControlTickelapsed + timeDelta);

      botControlTickTimer.reset();
   }

   const Vector<DatabaseObject *> *gameObjects = mGameObjDatabase->findObjects_fast();

   // Visit each game object, handling moves and running its idle method
   for(S32 i = gameObjects->size() - 1; i >= 0; i--)
   {
      TNLAssert(dynamic_cast<BfObject *>((*gameObjects)[i]), "Bad cast!");
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
      // Normalize ratings for this game
      getGameType()->updateRatings();
      cycleLevel(mNextLevel);
      mNextLevel = getSettings()->getIniSettings()->randomLevels ? +RANDOM_LEVEL : +NEXT_LEVEL;
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


void ServerGame::clearBotMoves()
{
   for(S32 i = 0; i < mRobots.size(); i++)
      mRobots[i]->clearMove();
}


bool ServerGame::startHosting()
{
   if(mSettings->getFolderManager()->levelDir == "")     // Never did resolve a leveldir... no hosting for you!
      return false;

   hostingModePhase = Hosting;

   for(S32 i = 0; i < getLevelNameCount(); i++)
      logprintf(LogConsumer::ServerFilter, "\t%s [%s]", getLevelNameFromIndex(i).getString(), 
                getLevelFileNameFromIndex(i).c_str());

   if(!getLevelNameCount())      // No levels loaded... we'll crash if we try to start a game       
      return false;      

   cycleLevel(FIRST_LEVEL);      // Start with the first level

   return true;
}


void ServerGame::gameEnded()
{
   mLevelSwitchTimer.reset(LevelSwitchTime);
}


S32 ServerGame::addUploadedLevelInfo(const char *filename, LevelInfo &levelInfo)
{
   if(levelInfo.mLevelName == "")            // Make sure we have something in the name field
      levelInfo.mLevelName = filename;

   levelInfo.mLevelFileName = filename; 

   // Check if we already have this one
   for(S32 i = 0; i < mLevelInfos.size(); i++)
      if(mLevelInfos[i].mLevelFileName == levelInfo.mLevelFileName)
         return i;

   // We don't... so add it!
   mLevelInfos.push_back(levelInfo);

   // Let levelChangers know about the new level

   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);

      if(clientInfo->isLevelChanger())
         clientInfo->getConnection()->s2cAddLevel(levelInfo.mLevelName, levelInfo.mLevelType);
   }

   return mLevelInfos.size() - 1;
}


};


