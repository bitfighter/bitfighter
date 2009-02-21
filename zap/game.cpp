//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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
#include "projectile.h"       // For SpyBug class
#include "sfx.h"
#include "ship.h"
#include "sparkManager.h"
#include "SweptEllipsoid.h"
#include "UIGame.h"
#include "UIMenus.h"
#include "UINameEntry.h"

#include "BotNavMeshZone.h"      // For misplaced zone clearing code

#include "../glut/glutInclude.h"

#include "../tnl/tnl.h"
#include "../tnl/tnlRandom.h"
#include "../tnl/tnlGhostConnection.h"
#include "../tnl/tnlNetInterface.h"

#include <sys/stat.h>
using namespace TNL;

namespace Zap
{

// global Game objects
ServerGame *gServerGame = NULL;
ClientGame *gClientGame = NULL;

Rect gServerWorldBounds;

extern const S32 gScreenHeight;
extern const S32 gScreenWidth;

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

// Constructor
Game::Game(const Address &theBindAddress)
{
   mNextMasterTryTime = 0;
   mCurrentTime = 0;

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
   return mGameType;
}

U32 Game::getTeamCount()
{
   if(mGameType.isValid())
      return mGameType->mTeams.size();
   return 0;
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
      if(gMasterAddress == Address())
         return;
      if(mNextMasterTryTime < timeDelta && gReadyToConnectToMaster)
      {
         mConnectionToMaster = new MasterServerConnection(isServer(), 0);
         mConnectionToMaster->connect(mNetInterface, gMasterAddress);
         mNextMasterTryTime = MasterServerConnectAttemptDelay;

         TNL::logprintf("%s connecting to master...", isServer() ? "Server" : "Client");
      }
      else if (!gReadyToConnectToMaster)
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

   Rect theRect = mGameObjects[0]->getExtent();
   for(S32 i = 0; i < mGameObjects.size(); i++)
      theRect.unionRect(mGameObjects[i]->getExtent());
   return theRect;
}


Point Game::computePlayerVisArea(Ship *ship)
{
   F32 fraction = ship->getSensorZoomFraction();

   Point ret;
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
ServerGame::ServerGame(const Address &theBindAddress, U32 maxPlayers, const char *hostName) : Game(theBindAddress)
{
   mPlayerCount = 0;
   mMaxPlayers = maxPlayers;
   mHostName = gHostName.c_str();
   mHostDescr = gHostDescr.c_str();
   mInfoFlags = 0;                  // Not used for much at the moment, but who knows --> propigates to master

   mNetInterface->setAllowsConnections(true);
   mMasterUpdateTimer.reset(UpdateServerStatusTime);
}

S32 ServerGame::getLevelNameCount()
{
   return mLevelNames.size();
}


extern CmdLineSettings gCmdLineSettings;
extern S32 gHostingModePhase;
 
// This gets called when you first host a game.
void ServerGame::setLevelList(Vector<StringTableEntry> levelList)
{
   mLevelList = levelList;
}

void ServerGame::resetLevelLoadIndex()
{
   mLevelLoadIndex = 0;
}

string ServerGame::getCurrentLevelLoadName()
{
   if(getLevelNameCount() == 0)     // Could happen if there are no valid levels specified wtih -levels param, for example
      return "";
   else
      return mLevelNames.last().getString();
}

void ServerGame::loadNextLevel()
{
   // Here we cycle through all the levels, reading them in, and grabbing their names for the level list
   // Seems kind of wasteful...  could we quit after we found the name? (probably not easily, and we get the benefit of learning early on which levels are b0rked)
   // How about storing them in memory for rapid recall? People sometimes load hundreds of levels, so it's probably not feasible.
   if(mLevelLoadIndex < mLevelList.size())
   {
      string levelName = getLevelFileName(mLevelList[mLevelLoadIndex].getString());
      if(loadLevel(levelName))    // loadLevel returns true if the load was successful
      {
         string lname = trim(getGameType()->mLevelName.getString());
         StringTableEntry name;
         if(lname == "")
            name = mLevelList[mLevelLoadIndex];      // Use filename if level name is blank
         else
            name = lname.c_str();

         StringTableEntry type(getGameType()->getGameTypeString());
         
         // Save some key level parameters
         mLevelNames.push_back(name);
         mLevelTypes.push_back(type);
         mMinRecPlayers.push_back(getGameType()->minRecPlayers);
         mMaxRecPlayers.push_back(getGameType()->maxRecPlayers);

         // We got what we need, so get rid of this level.  Delete any objects that may exist
         while(mGameObjects.size())    // Don't just to a .clear() because we want to make sure destructors run and memory gets cleared.
            delete mGameObjects[0];    // But would they anyway?  Apparently not...  using .clear() causes all kinds of hell.
         mScopeAlwaysList.clear();

         // If we're loading lots of levels, try not to kill the computer!
         // Sleep now handled by idle loop
         //if(mLevelList.size() > 10)
         //   Platform::sleep(1);

         TNL::logprintf ("Loaded level %s of type %s [%s]", name.getString(), type.getString(), levelName.c_str());
      }
      else     // Level could not be loaded -- it's either missing or invalid.  Remove it from our level list.
      {
         logprintf("Could not load level %s.  Skipping...", levelName.c_str());
         mLevelList.erase(mLevelLoadIndex);
         mLevelLoadIndex--;
      }
      mLevelLoadIndex++;
   }

   if(mLevelLoadIndex == mLevelList.size())
      gHostingModePhase = 2;     // DoneLoadingLevels
}


// Get the level name, as defined in the level file
StringTableEntry ServerGame::getLevelNameFromIndex(S32 indx)
{
   if(indx < 0 || indx >= mLevelNames.size())
      return StringTableEntry();
   else
      return StringTableEntry( mLevelNames[indx].getString() );
}

// Get the filename the level is saved under
string ServerGame::getLevelFileNameFromIndex(S32 indx)
{
   if(indx < 0 || indx >= mLevelList.size())
      return "";
   else
      return getLevelFileName(mLevelList[indx].getString());
}


string ServerGame::getLevelFileName(string base)
{

#ifdef TNL_OS_XBOX         // This logic completely untested for OS_XBOX... basically disables -leveldir param
      return = "d:\\media\\levels\\" + base;
#endif

      return gLevelDir + (gLevelDir != "" ? "/" : "") + base;
}

// Return name of level currently in play
StringTableEntry ServerGame::getCurrentLevelName()
{
   return mLevelNames[mCurrentLevelIndex];
}

// Return type of level currently in play
StringTableEntry ServerGame::getCurrentLevelType()
{
   return mLevelTypes[mCurrentLevelIndex];
}


static S32 QSORT_CALLBACK RatingSort(GameConnection **a, GameConnection **b)
{
   return ( ((*a)->mTotalScore == 0) ? .5 : (*a)->mCumScore / (*a)->mTotalScore ) <  
          ( ((*b)->mTotalScore == 0) ? .5 : (*b)->mCumScore / (*b)->mTotalScore );
}


extern void buildBotNavMeshZoneConnections();


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

   if(nextLevel >= 0)
      mCurrentLevelIndex = nextLevel;
   else
      mCurrentLevelIndex++;

   if(S32(mCurrentLevelIndex) >= mLevelList.size())
      mCurrentLevelIndex = 0;

   // Load the level for real this time (we loaded it once before, when we started the server, but only to grab a few params)
   loadLevel(getLevelFileNameFromIndex(mCurrentLevelIndex));

   // Analyze zone connections
   buildBotNavMeshZoneConnections();

   // Build a list of our current connections
   Vector<GameConnection *> connectionList;
   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
      connectionList.push_back(walk);

   // Now add the connections to the game type, in a random order --> will create random teams
   //while(connectionList.size())
   //{
   //   U32 index = TNL::Random::readI() % connectionList.size();
   //   GameConnection *gc = connectionList[index];
   //   connectionList.erase(index);     // Remove the connection from our list

   //   if(mGameType.isValid())
   //      mGameType->serverAddClient(gc);
   //   gc->activateGhosting();
   //}

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


bool ServerGame::loadLevel(string filename)
{
   mGridSize = DefaultGridSize;

   if(!initLevelFromFile(filename.c_str()))
   {
      // Try appending a ".level" to the filename, and see if that helps
      filename += ".level";
      if(!initLevelFromFile(filename.c_str()))
         return false;
   }

   if(!getGameType())
   {
      GameType *g = new GameType;
      g->addToGame(this);
   }

   return true;
}

// Process a single line of a level file, loaded in gameLoader.cpp
// argc is the number of parameters on the line, argv is the params themselves
void ServerGame::processLevelLoadLine(int argc, const char **argv)
{
   // Since we don't yet have bots, we'll just read and ignore any BotsPerTeam lines.  For now.
   if(!stricmp(argv[0], "BotsPerTeam"))
      return;

   if(!stricmp(argv[0], "GridSize"))      // GridSize requires a single parameter (an int
   {                                      // specifiying how many pixels in a grid cell)
      if(argc < 2)
      {
         TNL::logprintf("Improperly formed GridSize parameter -- using default");
         return;
      }
      mGridSize = max(min((F32) atof(argv[1]), (F32) maxGridSize), (F32) minGridSize);
   }
   else if(mGameType.isNull() || !mGameType->processLevelItem(argc, argv))
   {
      TNL::Object *theObject = TNL::Object::create(argv[0]);      // Create an object of the type specified on the line
      GameObject *object = dynamic_cast<GameObject*>(theObject);  // Force our new object to be a GameObject
      if(!object)    // Well... that was a bad idea!
      {
         TNL::logprintf("Invalid object type: %s -- ignoring", argv[0]);
         delete theObject;
      }
      else
      {
         object->addToGame(this);                           // And add it to the list of game objects
         object->processArguments(argc - 1, argv + 1);      //    with the rest of the line as parameters
      }                                                     //    (see GameObject.cpp, & various game-specific subtypes)
   }
}

extern bool gDedicatedServer;

void ServerGame::addClient(GameConnection *theConnection)
{
   // Send our list of levels and their types to the connecting client
   for(S32 i = 0; i < mLevelList.size();i++)
      theConnection->s2cAddLevel(mLevelNames[i], mLevelTypes[i]);

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

// Top-level idle loop for server
void ServerGame::idle(U32 timeDelta)
{
   mCurrentTime += timeDelta;
   mNetInterface->checkBanlistTimeouts(timeDelta);
   mNetInterface->checkIncomingPackets();
   checkConnectionToMaster(timeDelta);    // Connect to master server if not connected
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

      Move thisMove = mGameObjects[i]->getCurrentMove();
      thisMove.time = timeDelta;
      mGameObjects[i]->setCurrentMove(thisMove);
      mGameObjects[i]->idle(GameObject::ServerIdleMainLoop);
   }

   processDeleteList(timeDelta);
   mNetInterface->processConnections();

   // Load a new level if the time is out on the current one
   if(mLevelSwitchTimer.update(timeDelta))
      cycleLevel();

   // Periodically update our status on the master, so they know what we're doing...
   if(mMasterUpdateTimer.update(timeDelta))
   {
      MasterServerConnection *masterConn = gServerGame->getConnectionToMaster();
      if(masterConn && masterConn->isEstablished())
         masterConn->updateServerStatus(getCurrentLevelName(), getCurrentLevelType(), 0, mPlayerCount, mMaxPlayers, mInfoFlags);
      mMasterUpdateTimer.reset(UpdateServerStatusTime);
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
   TNLAssert(mConnectionToServer.isNull(), "Error, a connection already exists here.");
   mConnectionToServer = theConnection;
}

extern void JoystickUpdateMove(Move *theMove);
extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;
extern Point gMousePos;


void ClientGame::idle(U32 timeDelta)
{
   mCurrentTime += timeDelta;
   mNetInterface->checkIncomingPackets();

   checkConnectionToMaster(timeDelta);   // If no current connection to master, create (or recreate) one

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
      // Disable controls if we are going too fast (usually by being blasted around by a SpeedZone or mine)
      GameObject *controlObject = mConnectionToServer->getControlObject();
      Ship *ship = dynamic_cast<Ship *>(controlObject);

      if(ship && ship->getActualVel().len() > 1000)      // 1000 is completely arbitrary, but it seems to work well...
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
            mGameObjects[i]->idle(GameObject::ClientIdleControlMain);
         }
         else
         {
            Move m = mGameObjects[i]->getCurrentMove();
            m.time = timeDelta;
            mGameObjects[i]->setCurrentMove(m);
            mGameObjects[i]->idle(GameObject::ClientIdleMainRemote);
         }
      }

      if(controlObject)
         SFXObject::setListenerParams(controlObject->getRenderPos(),controlObject->getRenderVel());
   }

   processDeleteList(timeDelta);                // Delete any objects marked for deletion
   FXManager::tick((F32)timeDelta * 0.001f);    // Processes sparks and teleporter effects
   SFXObject::process();                        // Process sound effects (SFX)

   mNetInterface->processConnections();

   if(mScreenSaverTimer.update(timeDelta))
   {
      supressScreensaver();
      mScreenSaverTimer.reset();
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
   S32 fx2 = gIniSettings.starsInDistance ? 1 - ((S32) (cameraPos.x / starDist)) : 0;
   S32 fy1 = gIniSettings.starsInDistance ? -1 - ((S32) (cameraPos.y / starDist)) : 0;
   S32 fy2 = gIniSettings.starsInDistance ? 1 - ((S32) (cameraPos.y / starDist)) : 0;

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
   Ship *u = dynamic_cast<Ship *>(controlObject);
   Point position = u->getRenderPos();    // Ship's location (which will be coords of screen's center)

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
      Point visSize = computePlayerVisArea(u) * 2;
      Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;

      Point visScale(UserInterface::canvasWidth / modVisSize.x,
         UserInterface::canvasHeight / modVisSize.y );

      Point ret = (p - offset) * visScale + Point((gScreenWidth / 2), (gScreenHeight / 2));
      return ret;
   }
   else                       // Normal map view
   {
      Point visExt = computePlayerVisArea(dynamic_cast<Ship *>(u));
      Point scaleFactor((gScreenWidth / 2) / visExt.x, (gScreenHeight / 2) / visExt.y);
      Point ret = (p - position) * scaleFactor + Point((gScreenWidth / 2), (gScreenHeight / 2));
      return ret;
   }
}


void ClientGame::renderCommander()
{
   GameObject *controlObject = mConnectionToServer->getControlObject();
   Ship *u = dynamic_cast<Ship *>(controlObject);      // This is the local player's ship

   Point position = u->getRenderPos();

   F32 zoomFrac = getCommanderZoomFraction();

   // Set up the view to show the whole level.
   mWorldBounds = computeWorldObjectExtents(); // TODO: Cache this value!  ?  Or not?

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
   Point visSize = computePlayerVisArea(u) * 2;
   Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;

   Point visScale(UserInterface::canvasWidth / modVisSize.x,
                  UserInterface::canvasHeight / modVisSize.y );

   glPushMatrix();
   glTranslatef(gScreenWidth / 2, gScreenHeight / 2, 0);    // Put (0,0) at the center of the screen

   glScalef(visScale.x, visScale.y, 1);
   glTranslatef(-offset.x, -offset.y, 0);

   if(zoomFrac < 0.95)
      drawStars(1 - zoomFrac, offset, modVisSize);

   // Render the objects.  Start by putting all command-map-visible objects into renderObjects
   Vector<GameObject *> renderObjects;
   mDatabase.findObjects(CommandMapVisType, renderObjects, mWorldBounds);

   // Get info about the current player
   GameType *gt = gClientGame->getGameType();
   S32 playerTeam = -1;

   if(gt)
   {
      playerTeam = u->getTeam();
      Color teamColor = gt->getTeamColor(playerTeam);

      F32 colorFactor = zoomFrac * 0.35;
      glColor(teamColor * colorFactor);
      for(S32 i = 0; i < renderObjects.size(); i++)
      {
         // Render ship visibility range, and that of our teammates
         if(renderObjects[i]->getObjectTypeMask() & ShipType)
         {
            Ship *ship = (Ship*)(renderObjects[i]);

            // Get team of this object
            S32 ourTeam = ship->getTeam();
            if(ourTeam == playerTeam && getGameType()->isTeamGame() || ship == u)  // On our team (in team game) || the ship is us
            {
               Point p = ship->getRenderPos();
               Point visExt = computePlayerVisArea(ship);

               glBegin(GL_POLYGON);
                  glVertex2f(p.x - visExt.x, p.y - visExt.y);
                  glVertex2f(p.x + visExt.x, p.y - visExt.y);
                  glVertex2f(p.x + visExt.x, p.y + visExt.y);
                  glVertex2f(p.x - visExt.x, p.y + visExt.y);
               glEnd();
            }
         }
      }

      Vector<GameObject *> spyBugObjects;
      mDatabase.findObjects(SpyBugType, spyBugObjects, mWorldBounds);

      // Render spy bug visibility range second, so ranges appear above ship scanner range
      colorFactor = zoomFrac * 0.45;     // Slightly different color than that used for ships
      for(S32 i = 0; i < spyBugObjects.size(); i++)
      {
         if(spyBugObjects[i]->getObjectTypeMask() & SpyBugType)
         {
            S32 spyBugTeam = spyBugObjects[i]->getTeam();
            Zap::SpyBug *sb = dynamic_cast<Zap::SpyBug *>(spyBugObjects[i]);     // Why do we need to prefix SpyBug with Zap::??

            // On our team (in a team game) || was set by us (in any game) || is neutral (in any game)
            if(spyBugTeam == playerTeam && getGameType()->isTeamGame() || (getGameType()->mLocalClient && getGameType()->mLocalClient->name == sb->mSetBy) || spyBugTeam == -1)
            {
               Point p = spyBugObjects[i]->getRenderPos();
               Point visExt(gSpyBugRange, gSpyBugRange);
               glColor(teamColor * colorFactor);

               glBegin(GL_POLYGON);
                  glVertex2f(p.x - visExt.x, p.y - visExt.y);
                  glVertex2f(p.x + visExt.x, p.y - visExt.y);
                  glVertex2f(p.x + visExt.x, p.y + visExt.y);
                  glVertex2f(p.x - visExt.x, p.y + visExt.y);
               glEnd();

               glColor(teamColor * 0.8);     // Draw a marker in the middle
               drawCircle(position, 2);
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

   F32 zoomFrac = getCommanderZoomFraction();

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
   Vector<GameObject *> renderObjects;
   mDatabase.findObjects(CommandMapVisType, renderObjects, mapBounds);

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
   GameObject *u = mConnectionToServer->getControlObject();    // Returns player's ship...
   Point position = u->getRenderPos();                         // ...and it's location

   glPushMatrix();
   glTranslatef(gScreenWidth / 2, gScreenHeight / 2, 0);       // Put (0,0) at the center of the screen

   Point visExt = computePlayerVisArea(dynamic_cast<Ship *>(u));
   glScalef((gScreenWidth / 2) / visExt.x, (gScreenHeight / 2) / visExt.y, 1);

   glTranslatef(-position.x, -position.y, 0);

   drawStars(1.0, position, visExt * 2);

   // Render all the objects the player can see
   Vector<GameObject *> renderObjects;

   Point screenSize = visExt;
   Rect extentRect(position - screenSize, position + screenSize);
   mDatabase.findObjects(AllObjectTypes, renderObjects, extentRect);    // Use extent rects to quickly find objects in visual range

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

   // Render current ship's energy, too.
   if(mConnectionToServer.isValid())
   {
      Ship *s = dynamic_cast<Ship *>(mConnectionToServer->getControlObject());
      if(s)
      {
         //static F32 curEnergy = 0.f;

         //curEnergy = (curEnergy + s->mEnergy) * 0.5f;
         F32 energy = s->mEnergy / F32(Ship::EnergyMax);
         U32 totalLineCount = 100;

         F32 full = energy * totalLineCount;
         glBegin(GL_POLYGON);
         glColor3f(0, 0, 1);
         glVertex2f(UserInterface::horizMargin, UserInterface::canvasHeight - UserInterface::vertMargin - 20);
         glVertex2f(UserInterface::horizMargin, UserInterface::canvasHeight - UserInterface::vertMargin);
         glColor3f(0, 1, 1);
         glVertex2f(UserInterface::horizMargin + full * 2, UserInterface::canvasHeight - UserInterface::vertMargin);
         glVertex2f(UserInterface::horizMargin + full * 2, UserInterface::canvasHeight - UserInterface::vertMargin - 20);
         glEnd();

         glColor3f(1, 1 ,1);
         glBegin(GL_LINES);
            glVertex2f(UserInterface::horizMargin, UserInterface::canvasHeight - UserInterface::vertMargin - 20);
            glVertex2f(UserInterface::horizMargin, UserInterface::canvasHeight - UserInterface::vertMargin);
            glVertex2f(UserInterface::horizMargin + totalLineCount * 2, UserInterface::canvasHeight - UserInterface::vertMargin - 20);
            glVertex2f(UserInterface::horizMargin + totalLineCount * 2, UserInterface::canvasHeight - UserInterface::vertMargin);
         // Show safety line.
         glColor3f(1, 1, 0);
         F32 cutoffx = (Ship::EnergyCooldownThreshold * totalLineCount * 2) / Ship::EnergyMax;
         glVertex2f(UserInterface::horizMargin + cutoffx, UserInterface::canvasHeight - UserInterface::vertMargin - 23);
         glVertex2f(UserInterface::horizMargin + cutoffx, UserInterface::canvasHeight - UserInterface::vertMargin + 4);
         glEnd();
      }
   }

   //renderOverlayMap();     // Draw a floating overlay map
}

void ClientGame::render()
{
   if(!hasValidControlObject())
      return;

   if(mCommanderZoomDelta)
      renderCommander();
   else
      renderNormal();
}


};

