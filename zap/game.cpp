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

#include "barrier.h"
#include "config.h"
#include "game.h"
#include "gameLoader.h"
#include "gameNetInterface.h"
#include "gameObject.h"
#include "gameObjectRender.h"
#include "gameType.h"
#include "masterConnection.h"
#include "move.h"
#include "moveObject.h"
#include "projectile.h"          // For SpyBug class
#include "sfx.h"
#include "ship.h"
#include "sparkManager.h"
#include "SweptEllipsoid.h"
#include "UIGame.h"
#include "UIMenus.h"
#include "UINameEntry.h"
#include "luaLevelGenerator.h"

//#include "UIChat.h"

#include "BotNavMeshZone.h"      // For zone clearing code

#include "../glut/glutInclude.h"

#include "tnl.h"
#include "tnlRandom.h"
#include "tnlGhostConnection.h"
#include "tnlNetInterface.h"

#include <sys/stat.h>
using namespace TNL;

namespace Zap
{

// Global Game objects
ServerGame *gServerGame = NULL;
ClientGame *gClientGame = NULL;

Rect gServerWorldBounds = Rect();

extern const S32 gScreenHeight;
extern const S32 gScreenWidth;

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

// Constructor
Game::Game(const Address &theBindAddress) : mDatabase(GridDatabase(256))
{
   mNextMasterTryTime = 0;
   mReadyToConnectToMaster = false;

   mCurrentTime = 0;
   mGameSuspended = false;

   mNetInterface = new GameNetInterface(theBindAddress, this);
}


void Game::setScopeAlwaysObject(GameObject *theObject)
{
   mScopeAlwaysList.push_back(theObject);
}


GameNetInterface *Game::getNetInterface()
{
   return mNetInterface;
}


MasterServerConnection *Game::getConnectionToMaster()
{
   return mConnectionToMaster;
}


GameType *Game::getGameType()
{
   return mGameType;    // This is a safePtr, so it can be NULL, but will never point off into space
}


U32 Game::getTeamCount()
{
   return(mGameType.isValid() ? mGameType->mTeams.size() : 0);
}


void Game::setGameType(GameType *theGameType)
{
   mGameType = theGameType;
}

extern bool gReadyToConnectToMaster;

// If there is no valid connection to master server, perodically try to create one.
// If user is playing a game they're hosting, they should get one master connection
// for the client and one for the server.
void Game::checkConnectionToMaster(U32 timeDelta)
{
   if(!mConnectionToMaster.isValid())
   {
      if(gMasterAddress == Address())     // Check for a valid address
         return;

      if(mNextMasterTryTime < timeDelta && mReadyToConnectToMaster)
      {
          logprintf(LogConsumer::LogConnection, "%s connecting to master [%s]", isServer() ? "Server" : "Client", 
                    gMasterAddress.toString());

          mConnectionToMaster = new MasterServerConnection(isServer(), 0);
          mConnectionToMaster->connect(mNetInterface, gMasterAddress);

         mNextMasterTryTime = MASTER_SERVER_FAILURE_RETRY;     // 10 secs, just in case this attempt fails
      }
      else if(!mReadyToConnectToMaster)
         mNextMasterTryTime = 0;
      else
         mNextMasterTryTime -= timeDelta;
   }
}

Game::DeleteRef::DeleteRef(GameObject *o, U32 d)
{
   theObject = o;
   delay = d;
}

void Game::addToDeleteList(GameObject *theObject, U32 delay)
{
   mPendingDeleteObjects.push_back(DeleteRef(theObject, delay));
}

// Cycle through our pending delete list, and either delete an object or update its timer
void Game::processDeleteList(U32 timeDelta)
{
   for(S32 i = 0; i < mPendingDeleteObjects.size(); )    // no i++
   {     // braces required
      if(timeDelta > mPendingDeleteObjects[i].delay)
      {
         GameObject *g = mPendingDeleteObjects[i].theObject;
         delete g;
         mPendingDeleteObjects.erase_fast(i);
      }
      else
      {
         mPendingDeleteObjects[i].delay -= timeDelta;
         i++;
      }
   }
}

void Game::addToGameObjectList(GameObject *theObject)
{
   mGameObjects.push_back(theObject);
}

void Game::removeFromGameObjectList(GameObject *theObject)
{
   for(S32 i = 0; i < mGameObjects.size(); i++)
   {
      if(mGameObjects[i] == theObject)
      {
         mGameObjects.erase_fast(i);
         return;
      }
   }
   TNLAssert(0, "Object not in game's list!");
}


// Delete all objects of specified type
void Game::deleteObjects(U32 typeMask)
{
   for(S32 i = 0; i < mGameObjects.size(); i++)
      if(mGameObjects[i]->getObjectTypeMask() & typeMask)
         mGameObjects[i]->deleteObject(0);
}


Rect Game::computeWorldObjectExtents()
{
    if(!mGameObjects.size())
      return Rect();

   // All this rigamarole is to make world extent correct for levels that do not overlap (0,0)
   // The problem is that the GameType is treated as an object, and has the extent (0,0), and
   // a mask of UnknownType.  Fortunately, the GameType tends to be first, so what we do is skip
   // all objects until we find an UnknownType object, then start creating our extent from there.
   // We have to assign theRect to an extent object initially to avoid getting the default coords
   // of (0,0) that are assigned by the constructor.
   Rect theRect;

   S32 first = -1;

   // Look for first non-UnknownType object
   for(S32 i = 0; i < mGameObjects.size() && first == -1; i++)
      if(mGameObjects[i]->getObjectTypeMask() != UnknownType)
      {
         theRect = mGameObjects[i]->getExtent();
         first = i;
      }

   if(first == -1)      // No suitable objects found, return empty extents
      return Rect();

   // Now start unioning the extents of remaining objects.  Should be all of them.
   for(S32 i = first + 1; i < mGameObjects.size(); i++)
      theRect.unionRect(mGameObjects[i]->getExtent());

   return theRect;
}


Rect Game::computeBarrierExtents()
{
   Rect theRect;

   for(S32 i = 0; i < mGameObjects.size(); i++)
      if(mGameObjects[i]->getObjectTypeMask() & BarrierType)
         theRect.unionRect(mGameObjects[i]->getExtent());

   return theRect;
}


Point Game::computePlayerVisArea(Ship *ship)
{
   F32 fraction = ship->getSensorZoomFraction();

   Point regVis(PlayerHorizVisDistance, PlayerVertVisDistance);
   Point sensVis(PlayerSensorHorizVisDistance, PlayerSensorVertVisDistance);

   if(ship->isModuleActive(ModuleSensor))
      return regVis + (sensVis - regVis) * fraction;
   else
      return sensVis + (regVis - sensVis) * fraction;
}


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

extern string gHostName;
extern string gHostDescr;

// Constructor
ServerGame::ServerGame(const Address &theBindAddress, U32 maxPlayers, const char *hostName, bool testMode) : Game(theBindAddress)
{
   mPlayerCount = 0;
   mMaxPlayers = maxPlayers;
   mHostName = gHostName;
   mHostDescr = gHostDescr;
   mShuttingDown = false;

   hostingModePhase = ServerGame::NotHosting;

   mInfoFlags = 0;                  // Not used for much at the moment, but who knows --> propagates to master
   mCurrentLevelIndex = 0;

   if(testMode)
      mInfoFlags = 1;

   mTestMode = testMode;

   mNetInterface->setAllowsConnections(true);
   mMasterUpdateTimer.reset(UpdateServerStatusTime);

   mSuspendor = NULL;
}


// Destructor
ServerGame::~ServerGame()
{
   // Delete any objects on the delete list
   processDeleteList(0xFFFFFFFF);

   // Delete any game objects that may exist
   while(mGameObjects.size())
      delete mGameObjects[0];
}


S32 ServerGame::getLevelNameCount()
{
   return mLevelInfos.size();
}


class Robot;

S32 ServerGame::getRobotCount()
{
   return Robot::getRobotCount();
}


// This gets called when you first host a game
void ServerGame::setLevelList(Vector<StringTableEntry> levelList)
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
   if(getLevelNameCount() == 0)     // Could happen if there are no valid levels specified wtih -levels param, for example
      return "";
   else
      return mLevelInfos[mLevelLoadIndex - 1].levelName.getString();
}


// Control whether we're in shut down mode or not
void ServerGame::setShuttingDown(bool shuttingDown, U16 time, ClientRef *who)
{
   mShuttingDown = shuttingDown;
   if(shuttingDown)
   {
      mShutdownOriginator = who->clientConnection;

      // If there's no other clients, then just shutdown now
      if(GameConnection::onlyClientIs(mShutdownOriginator))
      {
         logprintf(LogConsumer::ServerFilter, "Server shutdown requested by %s.  No other players, so shutting down now.", 
                                                   mShutdownOriginator->getClientName().getString());
         mShutdownTimer.reset(1);
      }
      else
      {
         logprintf(LogConsumer::ServerFilter, "Server shutdown in %d seconds, requested by %s.", 
                                                   time, mShutdownOriginator->getClientName().getString());
         mShutdownTimer.reset(time * 1000);
      }
   }
   else
      logprintf(LogConsumer::ServerFilter, "Server shutdown canceled.");
}


void ServerGame::loadNextLevel()
{
   // Here we cycle through all the levels, reading them in, and grabbing their names for the level list
   // Seems kind of wasteful...  could we quit after we found the name? (probably not easily, and we get the benefit of learning early on which levels are b0rked)
   // How about storing them in memory for rapid recall? People sometimes load hundreds of levels, so it's probably not feasible.
   if(mLevelLoadIndex < mLevelInfos.size())
   {
      string levelName = getLevelFileName(mLevelInfos[mLevelLoadIndex].levelFileName.getString());

      if(loadLevel(levelName))    // loadLevel returns true if the load was successful
      {
         string lname = trim(getGameType()->mLevelName.getString());
         StringTableEntry name;
         if(lname == "")
            name = mLevelInfos[mLevelLoadIndex].levelFileName;      // Use filename if level name is blank
         else
            name = lname.c_str();

         StringTableEntry type(getGameType()->getGameTypeString());

         // Save some key level parameters
         mLevelInfos[mLevelLoadIndex].setInfo(name, type, getGameType()->minRecPlayers, getGameType()->maxRecPlayers);

         // We got what we need, so get rid of this level.  Delete any objects that may exist
         while(mGameObjects.size())    // Don't just to a .clear() because we want to make sure destructors run and memory gets cleared.
            delete mGameObjects[0];    // But would they anyway?  Apparently not...  using .clear() causes all kinds of hell.

         mScopeAlwaysList.clear();

         logprintf(LogConsumer::LogLevelLoaded, "Loaded level %s of type %s [%s]", name.getString(), type.getString(), levelName.c_str());
      }
      else     // Level could not be loaded -- it's either missing or invalid.  Remove it from our level list.
      {
         logprintf(LogConsumer::LogWarning, "Could not load level %s.  Skipping...", levelName.c_str());
         mLevelInfos.erase(mLevelLoadIndex);
         mLevelLoadIndex--;
      }
      mLevelLoadIndex++;
   }

   if(mLevelLoadIndex == mLevelInfos.size())
      ServerGame::hostingModePhase = DoneLoadingLevels;
}


S32 ServerGame::getAbsoluteLevelIndex(S32 indx)
{
   if(indx == NEXT_LEVEL)
      return (mCurrentLevelIndex + 1) % mLevelInfos.size();

   else if(indx == PREVIOUS_LEVEL)
   {
      indx = mCurrentLevelIndex - 1;
      return (indx >= 0) ? indx : mLevelInfos.size() - 1;
   }

   else if(indx == REPLAY_LEVEL)
      return mCurrentLevelIndex;

   else if(indx < 0 || indx >= mLevelInfos.size())    // Out of bounds index specified
      return 0;

   return indx;
}

// Get the level name, as defined in the level file
StringTableEntry ServerGame::getLevelNameFromIndex(S32 indx)
{
   return StringTableEntry( mLevelInfos[getAbsoluteLevelIndex(indx)].levelName.getString() );
}


// Get the filename the level is saved under
string ServerGame::getLevelFileNameFromIndex(S32 indx)
{
   if(indx < 0 || indx >= mLevelInfos.size())
      return "";
   else
      return getLevelFileName(mLevelInfos[indx].levelFileName.getString());
}


extern ConfigDirectories gConfigDirs;
extern string joindir(const string &path, const string &filename);

string ServerGame::getLevelFileName(string base)
{
   #ifdef TNL_OS_XBOX         // This logic completely untested for OS_XBOX... basically disables -leveldir param
      return = "d:\\media\\levels\\" + base;
   #endif

   return joindir(gConfigDirs.levelDir, base);
}


// Return filename of level currently in play
StringTableEntry ServerGame::getCurrentLevelFileName()
{
   return mLevelInfos[mCurrentLevelIndex].levelFileName;
}


// Return name of level currently in play
StringTableEntry ServerGame::getCurrentLevelName()
{
   return mLevelInfos[mCurrentLevelIndex].levelName;
}


// Return type of level currently in play
StringTableEntry ServerGame::getCurrentLevelType()
{
   return mLevelInfos[mCurrentLevelIndex].levelType;
}


// Should return a number between -1 and 1
F32 getCurrentRating(GameConnection *conn)
{
   if(conn->mTotalScore == 0 && conn->mGamesPlayed == 0)
      return .5;
   else if(conn->mTotalScore == 0)
      return (((conn->mGamesPlayed) * conn->mRating)) / (conn->mGamesPlayed);
   else
      return ((conn->mGamesPlayed * conn->mRating) + ((F32) conn->mScore / (F32) conn->mTotalScore)) / (conn->mGamesPlayed + 1);
}


// Highest ratings first
static S32 QSORT_CALLBACK RatingSort(GameConnection **a, GameConnection **b)
{
   return getCurrentRating(*a) < getCurrentRating(*b);
}


//extern void buildBotNavMeshZoneConnections();
extern void testBotNavMeshZoneConnections();


// Pass -1 to go to next level, otherwise pass an absolute level number
void ServerGame::cycleLevel(S32 nextLevel)
{
   // Delete any objects on the delete list
   processDeleteList(0xFFFFFFFF);

   // Delete any game objects that may exist
   while(mGameObjects.size())
      delete mGameObjects[0];

   mScopeAlwaysList.clear();

   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
      walk->resetGhosting();

   if(nextLevel >= FIRST_LEVEL)          // Go to specified level
      mCurrentLevelIndex = (nextLevel < mLevelInfos.size()) ? nextLevel : FIRST_LEVEL;

   else if(nextLevel == NEXT_LEVEL)      // Next level
   {
      S32 players = mPlayerCount;
       
      // If game is supended, then we are waiting for another player to join.  That means that (probably)
      // there are either 0 or 1 players, so the next game will need to be good for 1 or 2 players.
      if(mGameSuspended)
         players++;

      bool first = true;
      bool found = false;

      mCurrentLevelIndex = (mCurrentLevelIndex + 1) % mLevelInfos.size();
      S32 currLevel = mCurrentLevelIndex;

      // Cycle through the levels looking for one that matches our player counts
      while(first || mCurrentLevelIndex != currLevel)
      {
         S32 minPlayers = mLevelInfos[mCurrentLevelIndex].minRecPlayers;
         S32 maxPlayers = mLevelInfos[mCurrentLevelIndex].maxRecPlayers;

         if(maxPlayers == 0)        // i.e. limit doesn't apply (note if limit doesn't apply on the minPlayers, then 
            maxPlayers = S32_MAX;   // it works out because the smallest number of players is 1).

         if(players >= minPlayers && players <= maxPlayers)
         {
            found = true;
            break;
         }

         // else do nothing to play the next level, which we found above
         first = false;
      }

      // We didn't find a suitable level... just proceed to the next one
      if(!found)
      {
         mCurrentLevelIndex++;
         if(S32(mCurrentLevelIndex) >= mLevelInfos.size())
            mCurrentLevelIndex = FIRST_LEVEL;
      }
   } 
   else if(nextLevel == PREVIOUS_LEVEL)
      mCurrentLevelIndex = mCurrentLevelIndex > 0 ? mCurrentLevelIndex - 1 : mLevelInfos.size() - 1;

   //else if(nextLevel == REPLAY_LEVEL)    // Replay level, do nothing
   //   mCurrentLevelIndex += 0;


   gBotNavMeshZones.clear();

   logprintf(LogConsumer::ServerFilter, "Loading %s [%s]... \\", gServerGame->getLevelNameFromIndex(mCurrentLevelIndex).getString(), 
                                             gServerGame->getLevelFileNameFromIndex(mCurrentLevelIndex).c_str());

   // Load the level for real this time (we loaded it once before, when we started the server, but only to grab a few params)
   loadLevel(getLevelFileNameFromIndex(mCurrentLevelIndex));

   logprintf(LogConsumer::ServerFilter, "Done. [%s]", getTimeStamp().c_str());


   // Analyze zone connections
   BotNavMeshZone::buildBotNavMeshZoneConnections();     // Does nothing if there are no botNavMeshZones defined


   // Build a list of our current connections
   Vector<GameConnection *> connectionList;
   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
      connectionList.push_back(walk);

   // Now add the connections to the game type, from highest rating to lowest --> will create ratings-based teams
   connectionList.sort(RatingSort);

   for(S32 i = 0; i < connectionList.size(); i++)
   {
      GameConnection *gc = connectionList[i];

      if(mGameType.isValid())
         mGameType->serverAddClient(gc);
      gc->activateGhosting();
   }
}


// Enter suspended animation mode
void ServerGame::suspendGame()
{
   mGameSuspended = true;
   cycleLevel(NEXT_LEVEL);    // Advance to beginning of next level
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
   mGameSuspended = false;
   if(mSuspendor && !remoteRequest)     // If the request is from remote server, don't need to alert that server!
      mSuspendor->s2cUnsuspend();

   mSuspendor = NULL;
}


// Need to handle both forward and backward slashes... will return pathname with trailing delimeter.
inline string getPathFromFilename( const string& filename )
{
   size_t pos1 = filename.rfind("/");
   size_t pos2 = filename.rfind("\\");

   if(pos1 == string::npos)
      pos1 = 0;

   if(pos2 == string::npos)
      pos2 = 0;

   return filename.substr( 0, max(pos1, pos2) + 1 );
}

static string origFilename;      // Name of file we're trying to load

bool ServerGame::loadLevel(string filename)
{
   origFilename = filename;
   mGridSize = DefaultGridSize;

   mObjectsLoaded = 0;
   if(!initLevelFromFile(filename.c_str()))
   {
      // Try appending a ".level" to the filename, and see if that helps
      filename += ".level";
      if(!initLevelFromFile(filename.c_str()))
      {
         logprintf("Unable to open level file \"%s\".  Skipping...", origFilename.c_str());
         return false;
      }
   }

   // We should have a gameType by the time we get here... but in case we don't, we'll add a default one now
   if(!getGameType())
   {
      logprintf(LogConsumer::LogWarning, "Warning: Missing game type parameter in level \"%s\"", origFilename.c_str());
      GameType *g = new GameType;
      g->addToGame(this);
   }

   // If there was a script specified in the level file, now might be a fine time to try running it!
   if(getGameType()->mScriptArgs.size() > 0)
   {
      // The script file will be the first argument, subsequent args will be passed on to the script.
      // We'll assume that the script lives in the same folder as the level file.  Hope we don't need to get too fancy here...
      // Now we've crammed all our action into the constructor... is this ok design?
      LuaLevelGenerator levelgen = LuaLevelGenerator(getPathFromFilename(filename), getGameType()->mScriptArgs, gServerGame->getGridSize(), this);
   }

   getGameType()->onLevelLoaded();

   return true;
}


// Process a single line of a level file, loaded in gameLoader.cpp
// argc is the number of parameters on the line, argv is the params themselves
void ServerGame::processLevelLoadLine(U32 argc, U32 id, const char **argv)
{
   // This is a legacy from the old Zap! days... we do bots differently in Bitfighter, so we'll just ignore this line if we find it.
   if(!stricmp(argv[0], "BotsPerTeam"))
      return;

   if(!stricmp(argv[0], "GridSize"))      // GridSize requires a single parameter (an int
   {                                      //    specifiying how many pixels in a grid cell)
      if(argc < 2)
      {
         logprintf(LogConsumer::LogWarning, "Improperly formed GridSize parameter in level \"%s\"", origFilename.c_str());
         return;
      }
      mGridSize = max(min((F32) atof(argv[1]), (F32) MAX_GRID_SIZE), (F32) MIN_GRID_SIZE);
   }
   // True if we haven't yet created a gameType || false if processLevelItem can't do anything with the line
   else if(mGameType.isNull() || !mGameType->processLevelItem(argc, argv))    
   {
      char obj[LevelLoader::MaxArgLen + 1];

      // Kind of hacky, but if we encounter a FlagItem in a Nexus game, we'll convert it to a NexusFlag item.  This seems to make more sense.
      // This will work so long as FlagItem and HuntersFlagItem share a common attribute list.
      if(!stricmp(argv[0], "FlagItem") && !mGameType.isNull() && mGameType->getGameType() == GameType::NexusGame)
         strcpy(obj, "HuntersFlagItem");
      // Also, while we're here, we'll add advanced support for the NexusFlagItem, which HuntersFlagItem will eventually be renamed to...
      else if(!stricmp(argv[0], "NexusFlagItem"))
         strcpy(obj, "HuntersFlagItem");
      else
      {
         strncpy(obj, argv[0], LevelLoader::MaxArgLen);
         obj[LevelLoader::MaxArgLen] = '\0';
      }

      TNL::Object *theObject = TNL::Object::create(obj);          // Create an object of the type specified on the line
      GameObject *object = dynamic_cast<GameObject*>(theObject);  // Force our new object to be a GameObject


      if(!object)    // Well... that was a bad idea!
      {
         logprintf(LogConsumer::LogWarning, "Unknown object type \"%s\" in level \"%s\"", obj, origFilename.c_str());
         delete theObject;
      }
      else  // object was valid
      {
         gServerWorldBounds = gServerGame->computeWorldObjectExtents();    // Make sure this is current if we process a robot that needs this for intro code
         object->addToGame(this);

         bool validArgs = object->processArguments(argc - 1, argv + 1);

         if(!validArgs)
         {
            logprintf(LogConsumer::LogWarning, "Object \"%s\" in level \"%s\"", obj, origFilename.c_str());
            object->removeFromGame();
            object->destroySelf();
         }
      }
   }
}


extern bool gDedicatedServer;

void ServerGame::addClient(GameConnection *theConnection)
{
   // Send our list of levels and their types to the connecting client
   for(S32 i = 0; i < mLevelInfos.size();i++)
      theConnection->s2cAddLevel(mLevelInfos[i].levelName, mLevelInfos[i].levelType);

   // If we're shutting down, display a notice to the user
   if(mShuttingDown)
      theConnection->s2cInitiateShutdown(mShutdownTimer.getCurrent() / 1000, mShutdownOriginator->getClientName(), false);

   if(mGameType.isValid())
      mGameType->serverAddClient(theConnection);

   mPlayerCount++;

   if(gDedicatedServer)
      SFXObject::play(SFXPlayerJoined, 1);
}


void ServerGame::removeClient(GameConnection *theConnection)
{
   if(mGameType.isValid())
      mGameType->serverRemoveClient(theConnection);
   mPlayerCount--;

   if(gDedicatedServer)
      SFXObject::play(SFXPlayerLeft, 1);
}


// Top-level idle loop for server, runs only on the server by definition
void ServerGame::idle(U32 timeDelta)
{
   if( mShuttingDown && (mShutdownTimer.update(timeDelta) || GameConnection::onlyClientIs(mShutdownOriginator)) )
   {
      endGame();
      return;
   }

   // If there are no players on the server, we can enter "suspended animation" mode, but not during the first half-second of hosting.
   // This will prevent locally hosted game from immediately suspending for a frame, giving the local client a chance to 
   // connect.  A little hacky, but works!
   if(mPlayerCount == 0 && !mGameSuspended && mCurrentTime != 0)
      suspendGame();
   else if( mGameSuspended && ((mPlayerCount > 0 && !mSuspendor) || mPlayerCount > 1) )
      unsuspendGame(false);

   mCurrentTime += timeDelta;
   mNetInterface->checkIncomingPackets();
   checkConnectionToMaster(timeDelta);    // Connect to master server if not connected

   
   mNetInterface->checkBanlistTimeouts(timeDelta);    // Unban players who's bans have expired

   // Periodically update our status on the master, so they know what we're doing...
   if(mMasterUpdateTimer.update(timeDelta))
   {
      MasterServerConnection *masterConn = gServerGame->getConnectionToMaster();
      if(masterConn && masterConn->isEstablished())
         masterConn->updateServerStatus(getCurrentLevelName(), getCurrentLevelType(), getRobotCount(), mPlayerCount, mMaxPlayers, mInfoFlags);
      mMasterUpdateTimer.reset(UpdateServerStatusTime);
   }

   mNetInterface->processConnections();

   if(mGameSuspended)     // If game is suspended, we need do nothing more
      return;

   for(GameConnection *walk = GameConnection::getClientList(); walk ; walk = walk->getNextClient())
      walk->addToTimeCredit(timeDelta);

   // Compute new world extents -- these might change if a ship flies far away, for example...
   // In practice, we could probably just set it and forget it when we load a level.
   // Compute it here to save recomputing it for every robot and other method that relies on it.
   gServerWorldBounds = gServerGame->computeWorldObjectExtents();

   // Visit each game object, handling moves and running its idle method
   for(S32 i = 0; i < mGameObjects.size(); i++)
   {
      if(mGameObjects[i]->getObjectTypeMask() & DeletedType)
         continue;

      // Here is where the time gets set for all the various object moves
      Move thisMove = mGameObjects[i]->getCurrentMove();
      thisMove.time = timeDelta;

      // Give the object its move, then have it idle
      mGameObjects[i]->setCurrentMove(thisMove);
      mGameObjects[i]->idle(GameObject::ServerIdleMainLoop);
   }

   processDeleteList(timeDelta);


   // Load a new level if the time is out on the current one
   if(mLevelSwitchTimer.update(timeDelta))
   {
      // Normalize ratings for this game
      getGameType()->updateRatings();
      cycleLevel(NEXT_LEVEL);
   }


   // Lastly, play any sounds server might have made...
   SFXObject::process();
}


void ServerGame::gameEnded()
{
   mLevelSwitchTimer.reset(LevelSwitchTime);
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------


// Constructor
ClientGame::ClientGame(const Address &bindAddress) : Game(bindAddress)
{
   mInCommanderMap = false;
   mCommanderZoomDelta = 0;

   // Create some random stars
   for(U32 i = 0; i < NumStars; i++)
   {
      mStars[i].x = TNL::Random::readF();      // Between 0 and 1
      mStars[i].y = TNL::Random::readF();
   }

   // //Create some random hexagons
   //for(U32 i = 0; i < NumStars; i++)
   //{
   //   F32 x = TNL::Random::readF();
   //   F32 y = TNL::Random::readF();
   //   F32 ang = TNL::Random::readF() * Float2Pi;
   //   F32 size = TNL::Random::readF() * .1;


   //   for(S32 j = 0; j < 6; j++)
   //   {
   //      mStars[i * 6 + j].x = x + sin(ang + Float2Pi / 6 * j) * size;      // Between 0 and 1
   //      mStars[i * 6 + j].y = y + cos(ang + Float2Pi / 6 * j) * size;
   //   }
   //}


   mScreenSaverTimer.reset(59000);         // Fire screen saver supression every 59 seconds
}

bool ClientGame::hasValidControlObject()
{
   return mConnectionToServer.isValid() && mConnectionToServer->getControlObject();
}

bool ClientGame::isConnectedToServer()
{
   return mConnectionToServer.isValid() && mConnectionToServer->getConnectionState() == NetConnection::Connected;
}

GameConnection *ClientGame::getConnectionToServer()
{
   return mConnectionToServer;
}

void ClientGame::setConnectionToServer(GameConnection *theConnection)
{
   TNLAssert(theConnection, "Passing null connection.  Bah!");
   TNLAssert(mConnectionToServer.isNull(), "Error, a connection already exists here.");
   mConnectionToServer = theConnection;
}

extern void JoystickUpdateMove(Move *theMove);
extern IniSettings gIniSettings;
extern Point gMousePos;


void ClientGame::idle(U32 timeDelta)
{
   mCurrentTime += timeDelta;
   mNetInterface->checkIncomingPackets();

   checkConnectionToMaster(timeDelta);   // If no current connection to master, create (or recreate) one

   if(isSuspended())
   {
      mNetInterface->processConnections();
      return;
   }

   // Only update at most MaxMoveTime milliseconds
   if(timeDelta > Move::MaxMoveTime)
      timeDelta = Move::MaxMoveTime;

   if(!mInCommanderMap && mCommanderZoomDelta != 0)                        // Zooming into normal view
   {
      if(timeDelta > mCommanderZoomDelta)
         mCommanderZoomDelta = 0;
      else
         mCommanderZoomDelta -= timeDelta;

      gGameUserInterface.onMouseMoved((S32)gMousePos.x, (S32)gMousePos.y); // Keep ship pointed towards mouse
   }
   else if(mInCommanderMap && mCommanderZoomDelta != CommanderMapZoomTime) // Zooming out to commander's map
   {
      mCommanderZoomDelta += timeDelta;
      if(mCommanderZoomDelta > CommanderMapZoomTime)
         mCommanderZoomDelta = CommanderMapZoomTime;

      gGameUserInterface.onMouseMoved((S32)gMousePos.x, (S32)gMousePos.y); // Keep ship pointed towards mouse

   }

   Move *theMove = gGameUserInterface.getCurrentMove();       // Get move from keyboard input

   // Overwrite theMove if we're using joystick (also does some other essential joystick stuff)
   // We'll also run this while in the menus so if we enter keyboard mode accidentally, it won't
   // kill the joystick.  The design of combining joystick input and move updating really sucks.
   if(gIniSettings.inputMode == Joystick || UserInterface::current == &gOptionsMenuUserInterface)
      JoystickUpdateMove(theMove);

   //gGameUserInterface.checkCurrentMove(theMove);     // In ChatMode and QuickChatMode, stop all action by overwriting the current move with zeros


   theMove->time = timeDelta;

   if(mConnectionToServer.isValid())
   {
      // Disable controls if we are going too fast (usually by being blasted around by a GoFast or mine or whatever)
      GameObject *controlObject = mConnectionToServer->getControlObject();
      Ship *ship = dynamic_cast<Ship *>(controlObject);

      const S32 MAX_CONTROLLABLE_SPEED = 1000;     // 1000 is completely arbitrary, but it seems to work well...
      if(ship && ship->getActualVel().len() > MAX_CONTROLLABLE_SPEED)     
         theMove->left = theMove->right = theMove->up = theMove->down = 0;

      mConnectionToServer->addPendingMove(theMove);

      theMove->prepare();     // Pack and unpack the move for consistent rounding errors

      for(S32 i = 0; i < mGameObjects.size(); i++)
      {
         if(mGameObjects[i]->getObjectTypeMask() & DeletedType)
            continue;

         if(mGameObjects[i] == controlObject)
         {
            mGameObjects[i]->setCurrentMove(*theMove);
            mGameObjects[i]->idle(GameObject::ClientIdleControlMain);  // on client, object is our control object
         }
         else
         {
            Move m = mGameObjects[i]->getCurrentMove();
            m.time = timeDelta;
            mGameObjects[i]->setCurrentMove(m);
            mGameObjects[i]->idle(GameObject::ClientIdleMainRemote);    // on client, object is not our control object
         }
      }

      if(controlObject)
         SFXObject::setListenerParams(controlObject->getRenderPos(),controlObject->getRenderVel());
   }

   processDeleteList(timeDelta);                // Delete any objects marked for deletion
   FXManager::tick((F32)timeDelta * 0.001f);    // Processes sparks and teleporter effects
   SFXObject::process();                        // Process sound effects (SFX)

   mNetInterface->processConnections();         // Here we can pass on our updated ship info to the server

   if(mScreenSaverTimer.update(timeDelta))
   {
      supressScreensaver();
      mScreenSaverTimer.reset();
   }
}


// Client only
void ClientGame::prepareBarrierRenderingGeometry()
{
   for(S32 i = 0; i < mGameObjects.size(); i++)
      if(mGameObjects[i]->getObjectTypeMask() & BarrierType)
      {
         Barrier *barrier = dynamic_cast<Barrier *>(mGameObjects[i]);  
         if(barrier)
            barrier->prepareRenderingGeometry();
      }
}


// Fire keyboard event to suppress screen saver
void ClientGame::supressScreensaver()
{

#ifdef TNL_OS_WIN32     // Windows only for now, sadly...
   // Code from Tom Revell's Caffeine screen saver suppression product

   // Build keypress
   tagKEYBDINPUT keyup;
   keyup.wVk = VK_MENU;     // Some key they GLUT doesn't recognize
   keyup.wScan = NULL;
   keyup.dwFlags = KEYEVENTF_KEYUP;
   keyup.time = NULL;
   keyup.dwExtraInfo = NULL;

   tagINPUT array[1];
   array[0].type = INPUT_KEYBOARD;
   array[0].ki = keyup;
   SendInput(1, array, sizeof(INPUT));
#endif
}


void ClientGame::zoomCommanderMap()
{
   mInCommanderMap = !mInCommanderMap;
   if(mInCommanderMap)
      SFXObject::play(SFXUICommUp);
   else
      SFXObject::play(SFXUICommDown);


   GameConnection *conn = getConnectionToServer();

   if(conn)
   {
      if(mInCommanderMap)
         conn->c2sRequestCommanderMap();
      else
         conn->c2sReleaseCommanderMap();
   }
}


// Unused
U32 ClientGame::getPlayerAndRobotCount() 
{ 
   return mGameType ? mGameType->mClientList.size() : (U32)PLAYER_COUNT_UNAVAILABLE; 
}


U32 ClientGame::getPlayerCount()
{
   if(!mGameType)
      return (U32)PLAYER_COUNT_UNAVAILABLE;

   U32 players = 0;

   for(S32 i = 0; i < mGameType->mClientList.size(); i++)
   {
      if(!mGameType->mClientList[i]->isRobot)
         players++;
   }

   return players;
}


extern OptionsMenuUserInterface gOptionsMenuUserInterface;

void ClientGame::drawStars(F32 alphaFrac, Point cameraPos, Point visibleExtent)
{
   const F32 starChunkSize = 1024;        // Smaller numbers = more dense stars
   const F32 starDist = 3500;             // Bigger value = slower moving stars

   Point upperLeft = cameraPos - visibleExtent * 0.5f;   // UL corner of screen in "world" coords
   Point lowerRight = cameraPos + visibleExtent * 0.5f;  // LR corner of screen in "world" coords

   upperLeft *= 1 / starChunkSize;
   lowerRight *= 1 / starChunkSize;

   upperLeft.x = floor(upperLeft.x);      // Round to ints, slightly enlarging the corners to ensureloadle
   upperLeft.y = floor(upperLeft.y);      // the entire screen is "covered" by the bounding box

   lowerRight.x = floor(lowerRight.x) + 0.5;
   lowerRight.y = floor(lowerRight.y) + 0.5;

   // Render some stars
   glPointSize( 1.0f );
   glColor3f(0.8 * alphaFrac, 0.8 * alphaFrac, 1.0 * alphaFrac);

   glPointSize(1);
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(2, GL_FLOAT, sizeof(Point), &mStars[0]);    // Each star is a pair of floats between 0 and 1

   S32 fx1 = gIniSettings.starsInDistance ? -1 - ((S32) (cameraPos.x / starDist)) : 0;
   S32 fx2 = gIniSettings.starsInDistance ?  1 - ((S32) (cameraPos.x / starDist)) : 0;
   S32 fy1 = gIniSettings.starsInDistance ? -1 - ((S32) (cameraPos.y / starDist)) : 0;
   S32 fy2 = gIniSettings.starsInDistance ?  1 - ((S32) (cameraPos.y / starDist)) : 0;

   for(F32 xPage = upperLeft.x + fx1; xPage < lowerRight.x + fx2; xPage++)
      for(F32 yPage = upperLeft.y + fy1; yPage < lowerRight.y + fy2; yPage++)
      {
         glPushMatrix();
         glScalef(starChunkSize, starChunkSize, 1);   // Creates points with coords btwn 0 and starChunkSize

         if (gIniSettings.starsInDistance)
            glTranslatef(xPage + (cameraPos.x / starDist), yPage + (cameraPos.y / starDist), 0);
         else
            glTranslatef(xPage, yPage, 0);

         glDrawArrays(GL_POINTS, 0, NumStars);
         
         //glColor3f(.1,.1,.1);
         // for(S32 i = 0; i < 50; i++)
         //   glDrawArrays(GL_LINE_LOOP, i * 6, 6);

         glPopMatrix();
      }

   glDisableClientState(GL_VERTEX_ARRAY);
}

S32 QSORT_CALLBACK renderSortCompare(GameObject **a, GameObject **b)
{
   return (*a)->getRenderSortValue() - (*b)->getRenderSortValue();
}


Point ClientGame::worldToScreenPoint(Point p)
{
   GameObject *controlObject = mConnectionToServer->getControlObject();

   Ship *ship = dynamic_cast<Ship *>(controlObject);
   if(!ship)
      return Point(0,0);

   Point position = ship->getRenderPos();    // Ship's location (which will be coords of screen's center)

   if(mCommanderZoomDelta)    // In commander's map, or zooming in/out
   {
      F32 zoomFrac = getCommanderZoomFraction();
      Point worldCenter = mWorldBounds.getCenter();
      Point worldExtents = mWorldBounds.getExtents();
      worldExtents.x *= UserInterface::canvasWidth / F32(UserInterface::canvasWidth - (UserInterface::horizMargin * 2));
      worldExtents.y *= UserInterface::canvasHeight / F32(UserInterface::canvasHeight - (UserInterface::vertMargin * 2));

      F32 aspectRatio = worldExtents.x / worldExtents.y;
      F32 screenAspectRatio = UserInterface::canvasWidth / F32(UserInterface::canvasHeight);
      if(aspectRatio > screenAspectRatio)
         worldExtents.y *= aspectRatio / screenAspectRatio;
      else
         worldExtents.x *= screenAspectRatio / aspectRatio;

      Point offset = (worldCenter - position) * zoomFrac + position;
      Point visSize = computePlayerVisArea(ship) * 2;
      Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;

      Point visScale(UserInterface::canvasWidth / modVisSize.x,
         UserInterface::canvasHeight / modVisSize.y );

      Point ret = (p - offset) * visScale + Point((gScreenWidth / 2), (gScreenHeight / 2));
      return ret;
   }
   else                       // Normal map view
   {
      Point visExt = computePlayerVisArea(ship);
      Point scaleFactor((gScreenWidth / 2) / visExt.x, (gScreenHeight / 2) / visExt.y);
      Point ret = (p - position) * scaleFactor + Point((gScreenWidth / 2), (gScreenHeight / 2));
      return ret;
   }
}


void ClientGame::renderSuspended()
{
   glColor3f(1,1,0);
   S32 textHeight = 20;
   S32 textGap = 5;
   S32 ypos = gScreenHeight / 2 - 3 * (textHeight + textGap);

   UserInterface::drawCenteredString(ypos, textHeight, "==> Game is currently suspended, waiting for other players <==");
   ypos += textHeight + textGap;
   UserInterface::drawCenteredString(ypos, textHeight, "When another player joins, the game will start automatically.");
   ypos += textHeight + textGap;
   UserInterface::drawCenteredString(ypos, textHeight, "When the game restarts, the level will be reset.");
   ypos += 2 * (textHeight + textGap);
   UserInterface::drawCenteredString(ypos, textHeight, "Press <SPACE> to resume playing now");
}


static Vector<DatabaseObject *> rawRenderObjects;
static Vector<GameObject *> renderObjects;

void ClientGame::renderCommander()
{
   F32 zoomFrac = getCommanderZoomFraction();

   // Set up the view to show the whole level
   mWorldBounds = gGameUserInterface.mShowProgressBar ? getGameType()->mViewBoundsWhileLoading : computeWorldObjectExtents(); 

   Point worldCenter = mWorldBounds.getCenter();
   Point worldExtents = mWorldBounds.getExtents();
   worldExtents.x *= UserInterface::canvasWidth / F32(UserInterface::canvasWidth - (UserInterface::horizMargin * 2));
   worldExtents.y *= UserInterface::canvasHeight / F32(UserInterface::canvasHeight - (UserInterface::vertMargin * 2));

   F32 aspectRatio = worldExtents.x / worldExtents.y;
   F32 screenAspectRatio = UserInterface::canvasWidth / F32(UserInterface::canvasHeight);
   if(aspectRatio > screenAspectRatio)
      worldExtents.y *= aspectRatio / screenAspectRatio;
   else
      worldExtents.x *= screenAspectRatio / aspectRatio;

   glPushMatrix();

   GameObject *controlObject = mConnectionToServer->getControlObject();
   Ship *u = dynamic_cast<Ship *>(controlObject);      // This is the local player's ship
   
   Point position = u ? u->getRenderPos() : Point(0,0);

   Point visSize = u ? computePlayerVisArea(u) * 2 : worldExtents;
   Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;

   glTranslatef(gScreenWidth / 2, gScreenHeight / 2, 0);    // Put (0,0) at the center of the screen

   Point visScale(UserInterface::canvasWidth / modVisSize.x, UserInterface::canvasHeight / modVisSize.y );
   glScalef(visScale.x, visScale.y, 1);

   Point offset = (worldCenter - position) * zoomFrac + position;
   glTranslatef(-offset.x, -offset.y, 0);

   if(zoomFrac < 0.95)
      drawStars(1 - zoomFrac, offset, modVisSize);
 

   // Render the objects.  Start by putting all command-map-visible objects into renderObjects
   rawRenderObjects.clear();
   mDatabase.findObjects(CommandMapVisType, rawRenderObjects, mWorldBounds);
   
   renderObjects.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(dynamic_cast<GameObject *>(rawRenderObjects[i]));

   if(u)
   {
      // Get info about the current player
      GameType *gt = gClientGame->getGameType();
      S32 playerTeam = -1;

      if(gt)
      {
         playerTeam = u->getTeam();
         Color teamColor = gt->getTeamColor(playerTeam);

         for(S32 i = 0; i < renderObjects.size(); i++)
         {
            // Render ship visibility range, and that of our teammates
            if(renderObjects[i]->getObjectTypeMask() & (ShipType | RobotType))
            {
               Ship *ship = dynamic_cast<Ship *>(renderObjects[i]);

               // Get team of this object
               S32 ourTeam = ship->getTeam();
               if((ourTeam == playerTeam && getGameType()->isTeamGame()) || ship == u)  // On our team (in team game) || the ship is us
               {
                  Point p = ship->getRenderPos();
                  Point visExt = computePlayerVisArea(ship);

                  glColor(teamColor * zoomFrac * 0.35);

                  glBegin(GL_POLYGON);
                     glVertex2f(p.x - visExt.x, p.y - visExt.y);
                     glVertex2f(p.x + visExt.x, p.y - visExt.y);
                     glVertex2f(p.x + visExt.x, p.y + visExt.y);
                     glVertex2f(p.x - visExt.x, p.y + visExt.y);
                  glEnd();
               }
            }
         }

         Vector<DatabaseObject *> spyBugObjects;
         mDatabase.findObjects(SpyBugType, spyBugObjects, mWorldBounds);

         // Render spy bug visibility range second, so ranges appear above ship scanner range
         for(S32 i = 0; i < spyBugObjects.size(); i++)
         {
            if(spyBugObjects[i]->getObjectTypeMask() & SpyBugType)
            {
               SpyBug *sb = dynamic_cast<SpyBug *>(spyBugObjects[i]);

               if(sb->isVisibleToPlayer(playerTeam, getGameType()->mLocalClient ? getGameType()->mLocalClient->name : 
                                                                                  StringTableEntry(""), getGameType()->isTeamGame()))
               {
                  const Point &p = sb->getRenderPos();
                  Point visExt(gSpyBugRange, gSpyBugRange);
                  glColor(teamColor * zoomFrac * 0.45);     // Slightly different color than that used for ships

                  glBegin(GL_POLYGON);
                     glVertex2f(p.x - visExt.x, p.y - visExt.y);
                     glVertex2f(p.x + visExt.x, p.y - visExt.y);
                     glVertex2f(p.x + visExt.x, p.y + visExt.y);
                     glVertex2f(p.x - visExt.x, p.y + visExt.y);
                  glEnd();

                  glColor(teamColor * 0.8);     // Draw a marker in the middle
                  drawCircle(u->getRenderPos(), 2);
               }
            }
         }
      }
   }

   renderObjects.sort(renderSortCompare);

   for(S32 i = 0; i < renderObjects.size(); i++)
      renderObjects[i]->render(0);

   for(S32 i = 0; i < renderObjects.size(); i++)
      // Keep our spy bugs from showing up on enemy commander maps, even if they're known
 //     if(!(renderObjects[i]->getObjectTypeMask() & SpyBugType && playerTeam != renderObjects[i]->getTeam()))
         renderObjects[i]->render(1);

   glPopMatrix();
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// This is a test of a partial map overlay to assist in navigation
// still needs work, and early indications are that it is not
// a beneficial addition to the game.

void ClientGame::renderOverlayMap()
{
   GameObject *controlObject = mConnectionToServer->getControlObject();
   Ship *u = dynamic_cast<Ship *>(controlObject);

   Point position = u->getRenderPos();

   S32 mapWidth = UserInterface::canvasWidth / 4;
   S32 mapHeight = UserInterface::canvasHeight / 4;
   S32 mapX = UserInterface::horizMargin;        // This may need to the the UL corner, rather than the LL one
   S32 mapY = UserInterface::canvasHeight - UserInterface::vertMargin - mapHeight;
   F32 mapScale = 0.10;

   glBegin(GL_LINE_LOOP);
      glVertex2f(mapX, mapY);
      glVertex2f(mapX, mapY + mapHeight);
      glVertex2f(mapX + mapWidth, mapY + mapHeight);
      glVertex2f(mapX + mapWidth, mapY);
   glEnd();

   glEnable(GL_SCISSOR_BOX);                    // Crop to overlay map display area
   glScissor(mapX, mapY + mapHeight, mapWidth, mapHeight);  // Set cropping window

   glPushMatrix();   // Set scaling and positioning of the overlay

   glTranslatef(mapX + mapWidth / 2, mapY + mapHeight / 2, 0);          // Move map off to the corner
   glScalef(mapScale, mapScale, 1);                                     // Scale map
   glTranslatef(-position.x, -position.y, 0);                           // Put ship at the center of our overlay map area

   // Render the objects.  Start by putting all command-map-visible objects into renderObjects
   Rect mapBounds(position, position);
   mapBounds.expand(Point(mapWidth * 2, mapHeight * 2));      //TODO: Fix

   rawRenderObjects.clear();
   mDatabase.findObjects(CommandMapVisType, rawRenderObjects, mapBounds);

   renderObjects.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(dynamic_cast<GameObject *>(rawRenderObjects[i]));


   renderObjects.sort(renderSortCompare);

   for(S32 i = 0; i < renderObjects.size(); i++)
      renderObjects[i]->render(0);

   for(S32 i = 0; i < renderObjects.size(); i++)
      // Keep our spy bugs from showing up on enemy commander maps, even if they're known
 //     if(!(renderObjects[i]->getObjectTypeMask() & SpyBugType && playerTeam != renderObjects[i]->getTeam()))
         renderObjects[i]->render(1);

   glPopMatrix();
   glDisable(GL_SCISSOR_BOX);     // Stop cropping
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


void ClientGame::renderNormal()
{
    if(!hasValidControlObject())
      return;

   GameObject *controlObject = mConnectionToServer->getControlObject();
   Ship *u = dynamic_cast<Ship *>(controlObject);      // This is the local player's ship
   if(!u)
      return;

   Point position = u->getRenderPos();

   glPushMatrix();
   glTranslatef(gScreenWidth / 2, gScreenHeight / 2, 0);       // Put (0,0) at the center of the screen

   Point visExt = computePlayerVisArea(dynamic_cast<Ship *>(u));
   glScalef((gScreenWidth / 2) / visExt.x, (gScreenHeight / 2) / visExt.y, 1);

   glTranslatef(-position.x, -position.y, 0);

   drawStars(1.0, position, visExt * 2);

   // Render all the objects the player can see
   Point screenSize = visExt;
   Rect extentRect(position - screenSize, position + screenSize);

   rawRenderObjects.clear();
   mDatabase.findObjects(AllObjectTypes, rawRenderObjects, extentRect);    // Use extent rects to quickly find objects in visual range

   renderObjects.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(dynamic_cast<GameObject *>(rawRenderObjects[i]));

   renderObjects.sort(renderSortCompare);

   // Render in three passes, to ensure some objects are drawn above others
   for(S32 j = -1; j < 2; j++)
   {
      for(S32 i = 0; i < renderObjects.size(); i++)
         renderObjects[i]->render(j);
      FXManager::render(j);
   }

   FXTrail::renderTrails();

   glPopMatrix();

   // Render current ship's energy
   if(mConnectionToServer.isValid())
   {
      Ship *s = dynamic_cast<Ship *>(mConnectionToServer->getControlObject());
      if(s)
         renderEnergyGuage(s->mEnergy, Ship::EnergyMax, Ship::EnergyCooldownThreshold);
   }

   //renderOverlayMap();     // Draw a floating overlay map
}


void ClientGame::render()
{
   bool renderObjectsWhileLoading = false;

   if(!renderObjectsWhileLoading && !hasValidControlObject())
      return;

   if(gGameUserInterface.mShowProgressBar)
      renderCommander();
   else if(mGameSuspended)
      renderCommander();
   else if(mCommanderZoomDelta > 0)
      renderCommander();
   else
      renderNormal();
}


};


