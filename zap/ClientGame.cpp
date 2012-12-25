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

#include "ClientGame.h"
#include "ServerGame.h"    // For gServerGame refs

#include "ClientInfo.h"
#include "barrier.h"
#include "config.h"
#include "EngineeredItem.h"      // For EngineerModuleDeployer
#include "engineerHelper.h"
#include "GameSettings.h"
#include "gameLoader.h"
#include "gameNetInterface.h"
#include "BfObject.h"
#include "gameObjectRender.h"
#include "masterConnection.h"
#include "move.h"
#include "moveObject.h"
#include "projectile.h"          // For SpyBug class
#include "SharedConstants.h"     // For ServerInfoFlags enum
#include "sparkManager.h"
#include "GeomUtils.h"
#include "luaLevelGenerator.h"
#include "robot.h"
#include "shipItems.h"           // For moduleInfos
#include "NexusGame.h"
#include "Zone.h"                // For instantiation

#include "soccerGame.h"

#include "UI.h"                  // For renderRect

#include "stringUtils.h"

#include "IniFile.h"             // For CIniFile def

#include "BotNavMeshZone.h"      // For zone clearing code
#include "ScreenInfo.h"
#include "Joystick.h"

#include "UIGame.h"
#include "UIEditor.h"
#include "UINameEntry.h"
#include "UIQueryServers.h"
#include "UIErrorMessage.h"
#include "UIHighScores.h"

#include "TeamShuffleHelper.h"   // For... wait for it... TeamShuffleHelper class!!!

#include "tnl.h"
#include "tnlRandom.h"
#include "tnlGhostConnection.h"
#include "tnlNetInterface.h"

#include "md5wrapper.h"

#include "OpenglUtils.h"

#include <boost/shared_ptr.hpp>
#include <sys/stat.h>
#include <cmath>



using namespace TNL;

namespace Zap
{
bool showDebugBots = false;


// Global Game objects
// TODO: Replace this rigamarole with something like: Vector<ClientGame *> gClientGames;
//ClientGame *gClientGame = NULL;
//ClientGame *gClientGame1 = NULL;
//ClientGame *gClientGame2 = NULL;

Vector<ClientGame *> gClientGames;



extern ScreenInfo gScreenInfo;

////////////////////////////////////////
////////////////////////////////////////

// Constructor
ClientGame::ClientGame(const Address &bindAddress, GameSettings *settings) : Game(bindAddress, settings)
{
   mUserInterfaceData = new UserInterfaceData();

   mInCommanderMap        = false;
   mRequestedSpawnDelayed = false;
   mIsWaitingForSpawn     = false;
   mSpawnDelayed          = false;
   mGameIsRunning         = true;      // Only matters when game is suspended
   mSeenTimeOutMessage    = false;

   // Some debugging settings
   mDebugShowShipCoords   = false;
   mDebugShowMeshZones    = false;

   mCommanderZoomDelta = 0;

   mRemoteLevelDownloadFilename = "downloaded.level";

   mUIManager = new UIManager(this);               // Gets deleted in destructor

   mClientInfo = new FullClientInfo(this, NULL, false);  // Will be deleted in destructor


   // Create some random stars
   for(U32 i = 0; i < NumStars; i++)
   {
      mStars[i].x = TNL::Random::readF();    // Between 0 and 1
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

   mScreenSaverTimer.reset(59 * 1000);         // Fire screen saver supression every 59 seconds

}


// Destructor
ClientGame::~ClientGame()
{
   if(getConnectionToMaster()) // Prevents errors when ClientGame is gone too soon.
      getConnectionToMaster()->disconnect(NetConnection::ReasonSelfDisconnect, "");
   closeConnectionToGameServer();      // I just added this for good measure... not really sure it's needed
   cleanUp();

   delete mUserInterfaceData;
   delete mUIManager; 
   delete mConnectionToServer.getPointer();
}


// Player has selected a game from the QueryServersUserInterface, and is ready to join
// Also get here when hosting a game
void ClientGame::joinGame(Address remoteAddress, bool isFromMaster, bool local)
{
   // Much of the time, this may seem pointless, but if we arrive here via the editor, we need to swap out the editor's team manager for
   // the one used by the game.  If we don't we'll clobber the editor's copy, and we'll get crashes in the team definition (F2) menu.
   setActiveTeamManager(&mTeamManager);    

   mClientInfo->setIsAdmin(false); // Always start out with no permission
   mClientInfo->setIsLevelChanger(false);

   mSpawnDelayed = false;

   MasterServerConnection *connToMaster = getConnectionToMaster();
   
   if(isFromMaster && connToMaster && connToMaster->getConnectionState() == NetConnection::Connected)     // Request arranged connection
   {
      connToMaster->requestArrangedConnection(remoteAddress);
      getUIManager()->activate(GameUI);
   }
   else                                                         // Try a direct connection
   {
      getUIManager()->activate(GameUI);
      GameConnection *gameConnection = new GameConnection(this);

      setConnectionToServer(gameConnection);

      if(local)   // We're a local client, running in the same process as the server... connect to that server
      {
         // Stuff on client side, so interface will offer the correct options.
         // Note that if we're local, the passed address is probably a dummy; check caller if important.
         gameConnection->connectLocal(getNetInterface(), gServerGame->getNetInterface());
         mClientInfo->setIsAdmin(true);              // Local connection is always admin
         mClientInfo->setIsLevelChanger(true);       // Local connection can always change levels

         // Note that gc and gameConnection aren't the same, nor are gc->getClientInfo() and mClientInfo the same.
         // I _think_ gc is the server view of the local connection, where as gameConnection is the client's view.
         // Likewise with the clientInfos.  A little confusing, as they really represent the same thing in a way.  But different.
         GameConnection *gc = dynamic_cast<GameConnection *>(gameConnection->getRemoteConnectionObject()); 
         TNLAssert(gc, "gc should never be NULL here -- if it is, it means our connection to ourselves has failed for some reason");

         // Stuff on server side
         if(gc)                              
         {
            gc->getClientInfo()->setIsAdmin(true);                         // Set isAdmin on server
            gc->getClientInfo()->setIsLevelChanger(true);                  // Set isLevelChanger on server
            gc->sendLevelList();

            gc->s2cSetIsAdmin(true);                                       // Set isAdmin on the client
            gc->s2cSetIsLevelChanger(true, false);                         // Set isLevelChanger on the client
            gc->setServerName(gServerGame->getSettings()->getHostName());  // Server name is whatever we've set locally

            // Tell local host if we're authenticated... no need to verify
            gc->getClientInfo()->setAuthenticated(getClientInfo()->isAuthenticated(), getClientInfo()->getBadges());  
         }
      }
      else        // Connect to a remote server, but not via the master server
      {
         gameConnection->connect(getNetInterface(), remoteAddress);  
      }

   }

   //if(gClientGame2 && gClientGame != gClientGame2)  // make both client connect for now, until menus works in both clients.
   //{
   //   gClientGame = gClientGame2;
   //   joinGame(remoteAddress, isFromMaster, local);
   //   gClientGame = gClientGame1;
   //}
}


// Called when the local connection to game server is terminated for one reason or another
void ClientGame::closeConnectionToGameServer()
{
    // Cancel any in-progress attempts to connect
   if(getConnectionToMaster())
      getConnectionToMaster()->cancelArrangedConnectionAttempt();

   // Disconnect from game server
   if(getConnectionToServer())
      getConnectionToServer()->disconnect(NetConnection::ReasonSelfDisconnect, "");

   getUIManager()->getHostMenuUserInterface()->levelLoadDisplayDisplay = false;

   onGameOver();  
}


void ClientGame::onConnectedToMaster()
{
   Parent::onConnectedToMaster();

   // Clear old player list that might be there from client's lost connection to master while in game lobby
   Vector<StringTableEntry> emptyPlayerList;
   setPlayersInGlobalChat(emptyPlayerList);

   mSeenTimeOutMessage = false;     // Reset display of connection error

   logprintf(LogConsumer::LogConnection, "Client established connection with Master Server");
}


UIMode ClientGame::getUIMode()
{
   return mUIManager->getGameUserInterface()->getUIMode();
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
   theConnection->setClientGame(this);
}


ClientInfo *ClientGame::getClientInfo()
{
   return mClientInfo;
}


ClientInfo *ClientGame::getLocalRemoteClientInfo()
{
   return mLocalRemoteClientInfo;
}


void ClientGame::setSpawnDelayed(bool spawnDelayed)
{
   if(mSpawnDelayed && spawnDelayed)    // Yes, yes, we heard you the first time!
      return;

   mSpawnDelayed = spawnDelayed;

   if(spawnDelayed)
      mTimeToSuspend.reset();
   else
   {
      unsuspendGame();

      mRequestedSpawnDelayed = false;
      mIsWaitingForSpawn     = false;

      FXManager::clearSparks();
   }
}


// User has pressed a key, finished composing that most eloquent of chat messages, or taken some other action to undelay their spawn
void ClientGame::undelaySpawn()
{
   if(!isSpawnDelayed())                        // Already undelayed, nothing to do
      return;

   if(mClientInfo->getReturnToGameTime() > 0)   // Waiting for post /idle rejoin timer to wind down, nothing to do
      return;

   getConnectionToServer()->c2sPlayerSpawnUndelayed();
   mIsWaitingForSpawn = true;

   if(mRequestedSpawnDelayed)
      mClientInfo->resetReturnToGameTimer();
}


F32 ClientGame::getUIFadeFactor()
{
   return 1 - mTimeToSuspend.getFraction();     
}



// Provide access to these annoying bools
bool ClientGame::requestedSpawnDelayed()  { return mRequestedSpawnDelayed; }
bool ClientGame::isWaitingForSpawn()      { return mIsWaitingForSpawn;     }
bool ClientGame::isSpawnDelayed()         { return mSpawnDelayed;          }


// Tells the server to spawn delay us... server may incur a penalty when we unspawn
void ClientGame::requestSpawnDelayed()
{
   getConnectionToServer()->c2sPlayerRequestSpawnDelayed();
   mRequestedSpawnDelayed = true;
}


U32 ClientGame::getReturnToGameDelay()
{
   return mClientInfo->getReturnToGameTime();
}


string ClientGame::getLoginPassword() const
{
   return mLoginPassword;
}


void ClientGame::setLoginPassword(const string &loginPassword)
{
   mLoginPassword = loginPassword;
}


// When server corrects capitalization of name or similar
void ClientGame::correctPlayerName(const string &name)
{
   getClientInfo()->setName(name);
   getSettings()->setPlayerName(name, false);                  // Save to INI
}


// When user enters new name and password on NameEntryUI
void ClientGame::updatePlayerNameAndPassword(const string &name, const string &password)
{
   getClientInfo()->setName(name);
   getSettings()->setPlayerNameAndPassword(name, password);    // Save to INI
}


void ClientGame::displayShipDesignChangedMessage(const Vector<U8> &loadout, const char *msgToShowIfLoadoutsAreTheSame)
{
   if(!getConnectionToServer())
      return;

   BfObject *object = getConnectionToServer()->getControlObject();
   if(!object || object->getObjectTypeNumber() != PlayerShipTypeNumber)
      return;

   Ship *ship = static_cast<Ship *>(object);

   // If we're in a loadout zone, don't show any message -- new loadout will become active immediately, 
   // and we'll get a different msg from the server.  Avoids unnecessary messages.
   if(ship->isInZone(LoadoutZoneTypeNumber))
      return;

   if(getSettings()->getIniSettings()->verboseHelpMessages)
   {
      if(ship->isLoadoutSameAsCurrent(loadout))
         displayErrorMessage(msgToShowIfLoadoutsAreTheSame);
      else
      {
         GameType *gt = getGameType();

         displaySuccessMessage("Ship design changed -- %s", 
                              gt->levelHasLoadoutZone() ? "enter Loadout Zone to activate changes" : "changes will be activated when you respawn");
      }
   }
}


extern bool gShowAimVector;

static void joystickUpdateMove(GameSettings *settings, Move *theMove)
{
   // One of each of left/right axis and up/down axis should be 0 by this point
   // but let's guarantee it..   why?
   theMove->x = Joystick::JoystickInputData[MoveAxesRight].value - Joystick::JoystickInputData[MoveAxesLeft].value;
   theMove->x = MAX(theMove->x, -1);
   theMove->x = MIN(theMove->x, 1);
   theMove->y = Joystick::JoystickInputData[MoveAxesDown].value - Joystick::JoystickInputData[MoveAxesUp].value;
   theMove->y = MAX(theMove->y, -1);
   theMove->y = MIN(theMove->y, 1);

   //logprintf(
   //      "Joystick axis values. Move: Left: %f, Right: %f, Up: %f, Down: %f\nShoot: Left: %f, Right: %f, Up: %f, Down: %f ",
   //      Joystick::JoystickInputData[MoveAxesLeft].value, Joystick::JoystickInputData[MoveAxesRight].value,
   //      Joystick::JoystickInputData[MoveAxesUp].value, Joystick::JoystickInputData[MoveAxesDown].value,
   //      Joystick::JoystickInputData[ShootAxesLeft].value, Joystick::JoystickInputData[ShootAxesRight].value,
   //      Joystick::JoystickInputData[ShootAxesUp].value, Joystick::JoystickInputData[ShootAxesDown].value
   //      );

   //logprintf(
   //         "Move values. Move: Left: %f, Right: %f, Up: %f, Down: %f",
   //         theMove->left, theMove->right,
   //         theMove->up, theMove->down
   //         );


   // Goofball implementation of enableExperimentalAimMode here replicates old behavior when setting is disabled

   //logprintf("XY from shoot axes. x: %f, y: %f", x, y);

   Point p(Joystick::JoystickInputData[ShootAxesRight].value - Joystick::JoystickInputData[ShootAxesLeft].value, Joystick::JoystickInputData[ShootAxesDown].value - Joystick::JoystickInputData[ShootAxesUp].value);
   F32 plen = p.len();

   F32 maxplen = max(fabs(p.x), fabs(p.y));

   F32 fact = settings->getEnableExperimentalAimMode() ? maxplen : plen;

   if(fact > (settings->getEnableExperimentalAimMode() ? 0.95 : 0.66f))    // It requires a large movement to actually fire...
   {
      theMove->angle = atan2(p.y, p.x);
      theMove->fire = true;
      gShowAimVector = true;
   }
   else if(fact > 0.25)   // ...but you can change aim with a smaller one
   {
      theMove->angle = atan2(p.y, p.x);
      theMove->fire = false;
      gShowAimVector = true;
   }
   else
   {
      theMove->fire = false;
      gShowAimVector = false;
   }
}


bool ClientGame::isServer()
{
   return false;
}


U32 prevTimeDelta = 0;

void ClientGame::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   mNetInterface->checkIncomingPackets();

   checkConnectionToMaster(timeDelta);   // If no current connection to master, create (or recreate) one

   mClientInfo->updateReturnToGameTimer(timeDelta);

   if(isSuspended())
   {
      mNetInterface->processConnections();
      SoundSystem::processAudio(timeDelta, mSettings->getIniSettings()->sfxVolLevel,
                                           mSettings->getIniSettings()->getMusicVolLevel(),
                                           mSettings->getIniSettings()->voiceChatVolLevel,
                                           getUIManager()); 

      // Need to update the game clock to keep it in sync with the clients
      if(mGameIsRunning && getGameType())
         getGameType()->advanceGameClock(timeDelta);

      return;
   }


   mCurrentTime += timeDelta;

   computeWorldObjectExtents();

   if(!mInCommanderMap && mCommanderZoomDelta != 0)               // Zooming into normal view
   {
      if(timeDelta > mCommanderZoomDelta)
         mCommanderZoomDelta = 0;
      else
         mCommanderZoomDelta -= timeDelta;

      getUIManager()->getGameUserInterface()->onMouseMoved();     // Keep ship pointed towards mouse
   }
   else if(mInCommanderMap && mCommanderZoomDelta != CommanderMapZoomTime)    // Zooming out to commander's map
   {
      mCommanderZoomDelta += timeDelta;

      if(mCommanderZoomDelta > CommanderMapZoomTime)
         mCommanderZoomDelta = CommanderMapZoomTime;

      getUIManager()->getGameUserInterface()->onMouseMoved();  // Keep ship pointed towards mouse
   }
   // else we're not zooming in or out, which is most of the time


   Move *theMove = getUIManager()->getGameUserInterface()->getCurrentMove();       // Get move from keyboard input

   // Overwrite theMove if we're using joystick (also does some other essential joystick stuff)
   // We'll also run this while in the menus so if we enter keyboard mode accidentally, it won't
   // kill the joystick.  The design of combining joystick input and move updating really sucks.
   if(mSettings->getInputCodeManager()->getInputMode() == InputModeJoystick || 
      getUIManager()->getCurrentUI() == getUIManager()->getOptionsMenuUserInterface())
         joystickUpdateMove(mSettings, theMove);

   theMove->time = timeDelta + prevTimeDelta;

   if(mConnectionToServer.isValid())
   {
      // Disable controls if we are going too fast (usually by being blasted around by a GoFast or mine or whatever)
      BfObject *controlObject = mConnectionToServer->getControlObject();

      // Don't saturate server with moves...
      if(theMove->time < 6)     // Why 6?  Can this be related to some other factor?
         prevTimeDelta += timeDelta;
      else
      { 
         // Limited MaxPendingMoves only allows sending a few moves at a time. 
         // Changing MaxPendingMoves may break compatibility with old version server/client.
         // If running at 1000 FPS and 300 ping it will try to add more then MaxPendingMoves, losing control horribly.
         // Without the unlimited shield fix, the ship would also go super slow motion with over the limit MaxPendingMoves.
         // With 100 FPS limit, time is never less then 10 milliseconds. (1000 / millisecs = FPS), may be changed in .INI [Settings] MinClientDelay
         mConnectionToServer->addPendingMove(theMove);
         prevTimeDelta = 0;
      }
     
      theMove->time = timeDelta;
      theMove->prepare();           // Pack and unpack the move for consistent rounding errors

      // Visit each game object, handling moves and running its idle method
      BfObject *currentObject = idlingObjects.nextList;
      for(BfObject *obj = idlingObjects.nextList, *objNext; obj != NULL; obj = objNext)
      {
         objNext = obj->nextList; // Just in case this object is deleted inside idle function

         if(obj->isDeleted())
            continue;

         if(obj == controlObject)
         {
            obj->setCurrentMove(*theMove);
            obj->idle(BfObject::ClientIdleControlMain);  // on client, object is our control object
         }
         else
         {
            Move m = obj->getCurrentMove();
            m.time = timeDelta;
            obj->setCurrentMove(m);
            obj->idle(BfObject::ClientIdleMainRemote);    // on client, object is not our control object
         }
      }
      if(mGameType)
         mGameType->idle(BfObject::ClientIdleMainRemote, timeDelta);

      if(controlObject)
         SoundSystem::setListenerParams(controlObject->getPos(), controlObject->getVel());
   }

   processDeleteList(timeDelta);                         // Delete any objects marked for deletion
   FXManager::idle(timeDelta);                           // Processes sparks and teleporter effects
   SoundSystem::processAudio(timeDelta, mSettings->getIniSettings()->sfxVolLevel,
         mSettings->getIniSettings()->getMusicVolLevel(),
         mSettings->getIniSettings()->voiceChatVolLevel,
         getUIManager());  

   mNetInterface->processConnections();                  // Pass updated ship info to the server

   if(mScreenSaverTimer.update(timeDelta))               // Attempt, vainly, I'm afraid, to suppress screensavers
   {
      supressScreensaver();
      mScreenSaverTimer.reset();
   }

   mUIManager->idle(timeDelta);
}


void ClientGame::gotServerListFromMaster(const Vector<IPAddress> &serverList)
{
   getUIManager()->getQueryServersUserInterface()->gotServerListFromMaster(serverList);
}


void ClientGame::gotChatMessage(const char *playerNick, const char *message, bool isPrivate, bool isSystem)
{
   getUIManager()->getChatUserInterface()->newMessage(playerNick, message, isPrivate, isSystem, false);
}


void ClientGame::setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks)
{
   getUIManager()->getChatUserInterface()->setPlayersInGlobalChat(playerNicks);
}


void ClientGame::playerJoinedGlobalChat(const StringTableEntry &playerNick)
{
   getUIManager()->getChatUserInterface()->playerJoinedGlobalChat(playerNick);
}


void ClientGame::playerLeftGlobalChat(const StringTableEntry &playerNick)
{
   getUIManager()->getChatUserInterface()->playerLeftGlobalChat(playerNick);
}


// A new player has just joined the game; if isLocalClient is true, that new player is us!
// ClientInfo will be a RemoteClientInfo
void ClientGame::onPlayerJoined(ClientInfo *clientInfo, bool isLocalClient, bool playAlert, bool showMessage)
{
   addToClientList(clientInfo);

   // Find which client is us
   mLocalRemoteClientInfo = findClientInfo(mClientInfo->getName());

   if(isLocalClient)    // i.e. if this is us
   {
      // Now we'll check if we need an updated scoreboard... this only needed to handle use case of user
      // holding Tab while one game transitions to the next.  Without it, ratings will be reported as 0.
      if(getUIManager()->getGameUserInterface()->isInScoreboardMode())
         mGameType->c2sRequestScoreboardUpdates(true);

      if(showMessage)
         displayMessage(Color(0.6f, 0.6f, 0.8f), "Welcome to the game!");
   }
   else     // A remote player has joined the fray
   {
      if(showMessage) // Might as well not display when no sound being played, prevents message flood of many players join loading new GameType
         displayMessage(Color(0.6f, 0.6f, 0.8f), "%s joined the game.", clientInfo->getName().getString());

   }
   if(playAlert)
      SoundSystem::playSoundEffect(SFXPlayerJoined, 1);

   if(getUIMode() == TeamShuffleMode)
      mUIManager->getGameUserInterface()->getTeamShuffleHelper(this)->onPlayerJoined();

   mGameType->updateLeadingPlayerAndScore();
}


// Another player has just left the game
void ClientGame::onPlayerQuit(const StringTableEntry &name)
{
   removeFromClientList(name);

   displayMessage(Color(0.6f, 0.6f, 0.8f), "%s left the game.", name.getString());
   SoundSystem::playSoundEffect(SFXPlayerLeft, 1);

   if(getUIMode() == TeamShuffleMode)
      mUIManager->getGameUserInterface()->getTeamShuffleHelper(this)->onPlayerQuit();
}


// Gets run when game is really and truly over, after post-game scoreboard is displayed.  Over.
void ClientGame::onGameOver()
{
   clearClientList();                   // Erase all info we have about fellow clients

   if(getUIMode() == TeamShuffleMode)   // Exit Shuffle helper to keep things from getting too crashy
      enterMode(PlayMode);             

   // Kill any objects lingering in the database, such as forcefields
   getGameObjDatabase()->removeEverythingFromDatabase();    
}


// Only happens when using connectArranged, part of punching through firewall
void ClientGame::connectionToServerRejected(const char *reason)
{
   UIManager *uiManager = getUIManager();

   uiManager->activate(MainUI);

   ErrorMessageUserInterface *ui = static_cast<ErrorMessageUserInterface *>(uiManager->getUI(ErrorMessageUI));

   ui->reset();
   ui->setTitle("Connection Terminated");
   ui->setMessage(2, "Server did not respond or rejected");
   ui->setMessage(3, "when trying to punch through firewall");
   ui->setMessage(4, "Unable to join game.  Please try a different server.");

   if(reason[0])
      ui->setMessage(5, string(reason));

   uiManager->activate(ErrorMessageUI);

   closeConnectionToGameServer();
}


void ClientGame::setMOTD(const char *motd)
{
   getUIManager()->getMainMenuUserInterface()->setMOTD(motd); 
}


void ClientGame::setHighScores(Vector<StringTableEntry> groupNames, Vector<string> names, Vector<string> scores)
{
   getUIManager()->getHighScoresUserInterface()->setHighScores(groupNames, names, scores);
}


void ClientGame::setNeedToUpgrade(bool needToUpgrade)
{
  getUIManager()->getMainMenuUserInterface()->setNeedToUpgrade(needToUpgrade);
}


void ClientGame::displayMessage(const Color &msgColor, const char *format, ...)
{
   va_list args;
   char message[MAX_CHAT_MSG_LENGTH]; 

   va_start(args, format);
   vsnprintf(message, sizeof(message), format, args); 
   va_end(args);
    
   getUIManager()->getGameUserInterface()->displayMessage(msgColor, message);
}


extern Color gCmdChatColor;

void ClientGame::gotAdminPermissionsReply(bool granted)
{
   static const char *adminPassSuccessMsg = "You've been granted permission to manage players and change levels";
   static const char *adminPassFailureMsg = "Incorrect password: Admin access denied";

   // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
   if(getUIManager()->getCurrentUI()->getMenuID() == GameMenuUI)
      getUIManager()->getGameMenuUserInterface()->mMenuSubTitle = granted ? adminPassSuccessMsg : adminPassFailureMsg;
   else
      displayMessage(gCmdChatColor, granted ? adminPassSuccessMsg : adminPassFailureMsg);
}


void ClientGame::gotLevelChangePermissionsReply(bool granted)
{
   static const char *levelPassSuccessMsg = "You've been granted permission to change levels";
   static const char *levelPassFailureMsg = "Incorrect password: Level changing permissions denied";

   // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
   if(getUIManager()->getCurrentUI()->getMenuID() == GameMenuUI)
      getUIManager()->getGameMenuUserInterface()->mMenuSubTitle = granted ? levelPassSuccessMsg : levelPassFailureMsg;
   else
      displayMessage(gCmdChatColor, granted ? levelPassSuccessMsg : levelPassFailureMsg);
}

void ClientGame::gotWrongPassword()
{
   static const char *levelPassFailureMsg = "Incorrect password";

   // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
   if(getUIManager()->getCurrentUI()->getMenuID() == GameMenuUI)
      getUIManager()->getGameMenuUserInterface()->mMenuSubTitle = levelPassFailureMsg;
   else
      displayMessage(gCmdChatColor, levelPassFailureMsg);
}

void ClientGame::gotPingResponse(const Address &address, const Nonce &nonce, U32 clientIdentityToken)
{
   getUIManager()->getQueryServersUserInterface()->gotPingResponse(address, nonce, clientIdentityToken);
}


void ClientGame::gotQueryResponse(const Address &address, const Nonce &nonce, const char *serverName, const char *serverDescr, 
                                   U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired)
{
   getUIManager()->getQueryServersUserInterface()->gotQueryResponse(address, nonce, serverName, serverDescr, playerCount, 
                                                                    maxPlayers, botCount, dedicated, test, passwordRequired);
}


void ClientGame::shutdownInitiated(U16 time, const StringTableEntry &name, const StringPtr &reason, bool originator)
{
   getUIManager()->getGameUserInterface()->shutdownInitiated(time, name, reason, originator);
}


void ClientGame::cancelShutdown() 
{ 
   getUIManager()->getGameUserInterface()->cancelShutdown(); 
}


// Returns true if we have admin privs, displays error message and returns false if not
bool ClientGame::hasAdmin(const char *failureMessage)
{
   if(mClientInfo->isAdmin())
      return true;
   
   displayErrorMessage(failureMessage);
   return false;
}


// Returns true if we have level change privs, displays error message and returns false if not
bool ClientGame::hasLevelChange(const char *failureMessage)
{
   if(mClientInfo->isLevelChanger())
      return true;
   
   displayErrorMessage(failureMessage);
   return false;
}


void ClientGame::enterMode(UIMode mode)
{
   getUIManager()->getGameUserInterface()->enterMode(mode); 
}


void ClientGame::addToMuteList(const string &name)
{
   mMuteList.push_back(name);
}


void ClientGame::removeFromMuteList(const string &name)
{

   for(S32 i = 0; i < mMuteList.size(); i++)
      if(mMuteList[i] == name)
      {
         mMuteList.erase_fast(i);
         return;
      }
}


bool ClientGame::isOnMuteList(const string &name)
{
   for(S32 i = 0; i < mMuteList.size(); i++)
      if(mMuteList[i] == name)
         return true;
   
   return false;
}


void ClientGame::addToVoiceMuteList(const string &name)
{
   mVoiceMuteList.push_back(name);
}


void ClientGame::removeFromVoiceMuteList(const string &name)
{

   for(S32 i = 0; i < mVoiceMuteList.size(); i++)
      if(mVoiceMuteList[i] == name)
      {
         mVoiceMuteList.erase_fast(i);
         return;
      }
}


bool ClientGame::isOnVoiceMuteList(const string &name)
{
   for(S32 i = 0; i < mVoiceMuteList.size(); i++)
      if(mVoiceMuteList[i] == name)
         return true;

   return false;
}


string ClientGame::getRemoteLevelDownloadFilename() const
{
   return mRemoteLevelDownloadFilename;
}


void ClientGame::setRemoteLevelDownloadFilename(const string &filename)
{
   mRemoteLevelDownloadFilename = filename;
}


void ClientGame::changePassword(GameConnection::ParamType type, const Vector<string> &words, bool required)
{
   GameConnection *gc = getConnectionToServer();

   bool blankPassword = words.size() < 2 || words[1] == "";

   // First, send a message to the server to tell it we want to change a password
   if(required && blankPassword)
   {
      displayErrorMessage("!!! Need to supply a password");
      return;
   }

   gc->changeParam(blankPassword ? "" : words[1].c_str(), type);


   // Now make any changes we need locally -- writing (or erasing) the pw in the INI, etc.

   if(blankPassword)       // Empty password
   {
      // Clear any saved passwords for this server
      if(type == GameConnection::LevelChangePassword)
         mSettings->forgetLevelChangePassword(gc->getServerName());

      else if(type == GameConnection::AdminPassword)
         mSettings->forgetAdminPassword(gc->getServerName());
   }
   else                    // Non-empty password
   {
      // Save the password so the user need not enter it again the next time they're on this server
      if(type == GameConnection::LevelChangePassword)
         mSettings->saveLevelChangePassword(gc->getServerName(), words[1]);
         
      else if(type == GameConnection::AdminPassword)
         mSettings->saveAdminPassword(gc->getServerName(), words[1]);
   }
}


void ClientGame::changeServerParam(GameConnection::ParamType type, const Vector<string> &words)
{
   // Concatenate all params into a single string
   string allWords = concatenate(words, 1);

   // Did the user provide a name/description?
   if(type != GameConnection::DeleteLevel && allWords == "")
   {
      if(type == GameConnection::LevelDir)
         displayErrorMessage("!!! Need to supply a folder name");
      else if(type == GameConnection::ServerName)
         displayErrorMessage("!!! Need to supply a name");
      else
         displayErrorMessage("!!! Need to supply a description");
      return;
   }

   getConnectionToServer()->changeParam(allWords.c_str(), type);
}


// Returns true if it finds a case-sensitive match, or only 1 case-insensitive matches, false otherwise
bool ClientGame::checkName(const string &name)
{
   S32 potentials = 0;

   for(S32 i = 0; i < getClientCount(); i++)
   {
      StringTableEntry n = Parent::getClientInfo(i)->getName();

      if(n == name)           // Exact match
         return true;

      else if(!stricmp(n.getString(), name.c_str()))     // Case insensitive match
         potentials++;
   }

   return(potentials == 1);      // Return true if we only found exactly one potential match, false otherwise
}


void ClientGame::displayMessageBox(const StringTableEntry &title, const StringTableEntry &instr, 
                                   const Vector<StringTableEntry> &message) const
{
   ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

   ui->reset();
   ui->setTitle(title.getString());
   ui->setInstr(instr.getString());

   for(S32 i = 0; i < message.size(); i++)
      ui->setMessage(i+1, message[i].getString());      // UIErrorMsgInterface ==> first line = 1

   getUIManager()->activate(ui);
}


// Established connection is terminated.  Compare to onConnectTerminate() below.
void ClientGame::onConnectionTerminated(const Address &serverAddress, NetConnection::TerminationReason reason, const char *reasonStr, bool wasFullyConnected)
{
   clearClientList();  // this can fix all cases of extra names appearing on score board when connecting to server after getting disconnected other then "SelfDisconnect"

   if(getUIManager()->cameFrom(EditorUI))
     getUIManager()->reactivate(EditorUI);
   else
     getUIManager()->reactivate(MainUI);

   unsuspendGame();

   // Display a context-appropriate error message
   ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

   ui->reset();
   ui->setTitle("Connection Terminated");

   switch(reason)
   {
      case NetConnection::ReasonTimedOut:
         ui->setMessage(2, "Your connection timed out.  Please try again later.");

         getUIManager()->activate(ui);
         break;

      case NetConnection::ReasonIdle:
         ui->setMessage(2, "The server kicked you because you were idle too long.");
         ui->setMessage(4, "Feel free to rejoin the game when you are ready.");

         getUIManager()->activate(ui);
         break;

      case NetConnection::ReasonPuzzle:
         ui->setMessage(2, "Unable to connect to the server.  Recieved message:");
         ui->setMessage(3, "Invalid puzzle solution");
         ui->setMessage(5, "Please try a different game server, or try again later.");

         getUIManager()->activate(ui);
         break;

      case NetConnection::ReasonKickedByAdmin:
         ui->setMessage(2, "You were kicked off the server by an admin.");
         ui->setMessage(4, "You can try another server, host your own,");
         ui->setMessage(5, "or try the server that kicked you again later.");

         getUIManager()->activate(NameEntryUI);
         getUIManager()->activate(ui);
         break;

      case NetConnection::ReasonBanned:
         ui->setMessage(2, "You are banned from playing on this server");
         ui->setMessage(3, "Contact the server administrator if you think");
         ui->setMessage(4, "this was in error.");

         getUIManager()->activate(ui);
         break;

      case NetConnection::ReasonFloodControl:
         ui->setMessage(2, "Your connection was rejected by the server");
         ui->setMessage(3, "because you sent too many connection requests.");
         ui->setMessage(5, "Please try a different game server, or try again later.");

         getUIManager()->activate(NameEntryUI);
         getUIManager()->activate(ui);
         break;

      case NetConnection::ReasonShutdown:
         ui->setMessage(2, "Remote server shut down.");
         ui->setMessage(4, "Please try a different server,");
         ui->setMessage(5, "or host a game of your own!");

         getUIManager()->activate(ui);
         break;

      case NetConnection::ReasonNeedServerPassword:
      {
         // We have the wrong password, let's make sure it's not saved
         string serverName = getUIManager()->getQueryServersUserInterface()->getLastSelectedServerName();
         gINI.deleteKey("SavedServerPasswords", serverName);
   
         ServerPasswordEntryUserInterface *ui = getUIManager()->getServerPasswordEntryUserInterface();
         ui->setConnectServer(serverAddress);

         getUIManager()->activate(ui);
         break;
      }

      case NetConnection::ReasonServerFull:
         // Display a context-appropriate error message
         ui->reset();
         ui->setTitle("Connection Terminated");

         ui->setMessage(2, "Could not connect to server");
         ui->setMessage(3, "because server is full.");
         ui->setMessage(5, "Please try a different server, or try again later.");

         getUIManager()->activate(ui);
         break;

      case NetConnection::ReasonSelfDisconnect:
            // We get this when we terminate our own connection.  Since this is intentional behavior,
            // we don't want to display any message to the user.
         break;

      default:
         if(reasonStr[0])
         {
            ui->setMessage(1, "Disconnected for this reason:");
            ui->setMessage(2, string(reasonStr));
         }
         else
         {
            ui->setMessage(1, "Disconnected for unknown reason:");
            ui->setMessage(1, "Error number: " + itos(reason));
         }

         getUIManager()->activate(ui);
         break;
   }

}


void ClientGame::onConnectionToMasterTerminated(NetConnection::TerminationReason reason, const char *reasonStr, bool wasFullyConnected)
{
   ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

   ui->reset();

   switch(reason)
   {
      case NetConnection::ReasonDuplicateId:
         ui->setMessage(2, "Your connection was rejected by the server");
         ui->setMessage(3, "because you sent a duplicate player id. Player ids are");
         ui->setMessage(4, "generated randomly, and collisions are extremely rare.");
         ui->setMessage(5, "Please restart Bitfighter and try again.  Statistically");
         ui->setMessage(6, "speaking, you should never see this message again!");
         getUIManager()->activate(ui);

         getClientInfo()->getId()->getRandom();        // Get a different ID and retry to successfully connect to master
         break;

      case NetConnection::ReasonBadLogin:
         ui->setMessage(2, "Unable to log you in with the username/password you");
         ui->setMessage(3, "provided. If you have an account, please verify your");
         ui->setMessage(4, "password. Otherwise, you chose a reserved name; please");
         ui->setMessage(5, "try another.");
         ui->setMessage(7, "Please check your credentials and try again.");

         getUIManager()->activate(NameEntryUI);
         getUIManager()->activate(ui);
         break;

      case NetConnection::ReasonInvalidUsername:
         ui->setMessage(2, "Your connection was rejected by the server because");
         ui->setMessage(3, "you sent an username that contained illegal characters.");
         ui->setMessage(5, "Please try a different name.");

         getUIManager()->activate(NameEntryUI);
         getUIManager()->activate(ui);
         break;

      case NetConnection::ReasonError:
         ui->setMessage(2, "Unable to connect to the server.  Recieved message:");
         ui->setMessage(3, string(reasonStr));
         ui->setMessage(5, "Please try a different game server, or try again later.");

         getUIManager()->activate(ui);
         break;

      case NetConnection::ReasonTimedOut:
         // Avoid spamming the player if they are not connected to the Internet
         if(reason == NetConnection::ReasonTimedOut && mSeenTimeOutMessage)
            break;
         if(wasFullyConnected)
            break;

         ui->setMessage(2, "My attempt to connect to the Master Server failed because");
         ui->setMessage(3, "the server did not respond.  Either the server is down,");
         ui->setMessage(4, "or, more likely, you are either not connected to the internet");
         ui->setMessage(5, "or your firewall is blocking the connection.");
         ui->setMessage(7, "I will continue to try connecting, but you will not see this");
         ui->setMessage(8, "message again until you successfully connect or restart Bitfighter.");

         getUIManager()->activate(ui);

         mSeenTimeOutMessage = true;
         break;
      case NetConnection::ReasonSelfDisconnect:
         break;  // no errors when client disconnect (when quitting bitfighter)

      default:  // Not handled
         ui->setMessage(2, "Unable to connect to the master server, with error code:");

         if(reasonStr[0])
            ui->setMessage(3, itos(reason) + " " + reasonStr);
         else
            ui->setMessage(3, "MasterServer Error #" + itos(reason));

         ui->setMessage(5, "Check your Internet Connection and firewall settings.");
         ui->setMessage(7, "Please report this error code to the");
         ui->setMessage(8, "Bitfighter developers.");

         getUIManager()->activate(ui);
         break;
   }
}

extern CIniFile gINI;


void ClientGame::runCommand(const char *command)
{
   getUIManager()->getGameUserInterface()->runCommand(command);
}


string ClientGame::getRequestedServerName()
{
   return getUIManager()->getQueryServersUserInterface()->getLastSelectedServerName();
}


string ClientGame::getServerPassword()
{
   return getUIManager()->getServerPasswordEntryUserInterface()->getText();
}


string ClientGame::getHashedServerPassword()
{
   return getUIManager()->getServerPasswordEntryUserInterface()->getSaltedHashText();
}


void ClientGame::suspendGame(bool gameIsRunning)
{
   mGameIsRunning = gameIsRunning;
   mTimeToSuspend.reset();
}


void ClientGame::unsuspendGame()
{
   mGameSuspended = false;
   mTimeToSuspend.clear();
}


void ClientGame::displayErrorMessage(const char *format, ...)
{
   static char message[256];

   va_list args;

   va_start(args, format);
   vsnprintf(message, sizeof(message), format, args);
   va_end(args);

   getUIManager()->getGameUserInterface()->displayErrorMessage(message);
}



void ClientGame::displaySuccessMessage(const char *format, ...)
{
   static char message[256];

   va_list args;

   va_start(args, format);
   vsnprintf(message, sizeof(message), format, args);
   va_end(args);

   getUIManager()->getGameUserInterface()->displaySuccessMessage(message);
}

// Fire keyboard event to suppress screen saver
void ClientGame::supressScreensaver()
{

#if defined(TNL_OS_WIN32) && (_WIN32_WINNT > 0x0400)     // Windows only for now, sadly...
   // _WIN32_WINNT is needed in case of compiling for old windows 98 (this code won't work for windows 98)

   // Code from Tom Revell's Caffeine screen saver suppression product

   // Build keypress
   tagKEYBDINPUT keyup;
   keyup.wVk = VK_MENU;     // Some key that GLUT doesn't recognize
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


bool ClientGame::isShowingDebugShipCoords() const
{
   return mDebugShowShipCoords;
}


void ClientGame::toggleShowingShipCoords()
{
   mDebugShowShipCoords = !mDebugShowShipCoords;
}


bool ClientGame::isShowingDebugMeshZones() const
{
   return mDebugShowMeshZones;
}


void ClientGame::toggleShowingMeshZones()
{
   mDebugShowMeshZones = !mDebugShowMeshZones;
}


Ship *ClientGame::findShip(const StringTableEntry &clientName)
{
   fillVector.clear();

   getGameObjDatabase()->findObjects(PlayerShipTypeNumber, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      Ship *ship = static_cast<Ship *>(fillVector[i]);
      if(ship->getClientInfo()->getName() == clientName)
         return ship;
   }

   return NULL;
}



void ClientGame::zoomCommanderMap()
{
   mInCommanderMap = !mInCommanderMap;
   if(mInCommanderMap)
      SoundSystem::playSoundEffect(SFXUICommUp);
   else
      SoundSystem::playSoundEffect(SFXUICommDown);


   GameConnection *conn = getConnectionToServer();

   if(conn)
   {
      if(mInCommanderMap)
         conn->c2sRequestCommanderMap();
      else
         conn->c2sReleaseCommanderMap();
   }
}


void ClientGame::drawStars(F32 alphaFrac, Point cameraPos, Point visibleExtent)
{
   const F32 starChunkSize = 1024;        // Smaller numbers = more dense stars
   const F32 starDist = 3500;             // Bigger value = slower moving stars

   Point upperLeft = cameraPos - visibleExtent * 0.5f;   // UL corner of screen in "world" coords
   Point lowerRight = cameraPos + visibleExtent * 0.5f;  // LR corner of screen in "world" coords

   // When zooming out to commander's view, visibleExtent will grow larger and larger.  At some point, drawing all the stars
   // needed to fill the zoomed out screen becomes overwhelming, and bogs the computer down.  So we really need to set some
   // rational limit on where we stop showing stars during the zoom process (recall that stars are hidden when we are done zooming,
   // so this effect should be transparent to the user except at the most extreme of scales, and then, the alternative is slowing 
   // the computer greatly).  Note that 10000 is probably irrationally high.
   if(visibleExtent.x > 10000 || visibleExtent.y > 10000) 
      return;

   upperLeft  *= 1 / starChunkSize;
   lowerRight *= 1 / starChunkSize;

   upperLeft.x = floor(upperLeft.x);      // Round to ints, slightly enlarging the corners to ensure
   upperLeft.y = floor(upperLeft.y);      // the entire screen is "covered" by the bounding box

   lowerRight.x = floor(lowerRight.x) + 0.5f;
   lowerRight.y = floor(lowerRight.y) + 0.5f;

   // Render some stars
   glPointSize( gLineWidth1 );
   glColor(0.8f * alphaFrac, 0.8f * alphaFrac, alphaFrac);

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(2, GL_FLOAT, sizeof(Point), &mStars[0]);    // Each star is a pair of floats between 0 and 1

   bool starsInDistance = mSettings->getStarsInDistance();

   S32 fx1 = 0, fx2 = 0, fy1 = 0, fy2 = 0;

   if(starsInDistance)
   {
      S32 xDist = (S32) (cameraPos.x / starDist);
      S32 yDist = (S32) (cameraPos.y / starDist);

      fx1 = -1 - xDist;
      fx2 =  1 - xDist;
      fy1 = -1 - yDist;
      fy2 =  1 - yDist;
   }

   glDisable(GL_BLEND);

   for(F32 xPage = upperLeft.x + fx1; xPage < lowerRight.x + fx2; xPage++)
      for(F32 yPage = upperLeft.y + fy1; yPage < lowerRight.y + fy2; yPage++)
      {
         glPushMatrix();
         glScale(starChunkSize);   // Creates points with coords btwn 0 and starChunkSize

         if(starsInDistance)
            glTranslatef(xPage + (cameraPos.x / starDist), yPage + (cameraPos.y / starDist), 0);
         else
            glTranslatef(xPage, yPage, 0);

         glDrawArrays(GL_POINTS, 0, NumStars);
         
         //glColor(.1,.1,.1);
         // for(S32 i = 0; i < 50; i++)
         //   glDrawArrays(GL_LINE_LOOP, i * 6, 6);

         glPopMatrix();
      }

   glEnable(GL_BLEND);

   glDisableClientState(GL_VERTEX_ARRAY);
}


S32 QSORT_CALLBACK renderSortCompare(BfObject **a, BfObject **b)
{
   return (*a)->getRenderSortValue() - (*b)->getRenderSortValue();
}


UIManager *ClientGame::getUIManager() const
{
   return mUIManager;
}


bool ClientGame::getInCommanderMap()
{
   return mInCommanderMap;
}


void ClientGame::setInCommanderMap(bool inCommanderMap)
{
   mInCommanderMap = inCommanderMap;
}


F32 ClientGame::getCommanderZoomFraction() const
{
   return mCommanderZoomDelta / F32(CommanderMapZoomTime);
}


// Called from renderObjectiveArrow() & ship's onMouseMoved() when in commander's map
Point ClientGame::worldToScreenPoint(const Point *point) const
{
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   BfObject *controlObject = mConnectionToServer->getControlObject();

   if(!controlObject || controlObject->getObjectTypeNumber() != PlayerShipTypeNumber)
      return Point(0,0);

   Ship *ship = static_cast<Ship *>(controlObject);

   Point position = ship->getRenderPos();    // Ship's location (which will be coords of screen's center)
   
   if(mCommanderZoomDelta)    // In commander's map, or zooming in/out
   {
      F32 zoomFrac = getCommanderZoomFraction();
      Point worldExtents = mWorldExtents.getExtents();
      worldExtents.x *= canvasWidth / F32(canvasWidth - (UserInterface::horizMargin * 2));
      worldExtents.y *= canvasHeight / F32(canvasHeight - (UserInterface::vertMargin * 2));

      F32 aspectRatio = worldExtents.x / worldExtents.y;
      F32 screenAspectRatio = F32(canvasWidth) / F32(canvasHeight);
      if(aspectRatio > screenAspectRatio)
         worldExtents.y *= aspectRatio / screenAspectRatio;
      else
         worldExtents.x *= screenAspectRatio / aspectRatio;

      Point offset = (mWorldExtents.getCenter() - position) * zoomFrac + position;
      Point visSize = computePlayerVisArea(ship) * 2;
      Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;

      Point visScale(canvasWidth / modVisSize.x, canvasHeight / modVisSize.y );

      Point ret = (*point - offset) * visScale + Point((gScreenInfo.getGameCanvasWidth() / 2), (gScreenInfo.getGameCanvasHeight() / 2));
      return ret;
   }
   else                       // Normal map view
   {
      Point visExt = computePlayerVisArea(ship);
      Point scaleFactor((gScreenInfo.getGameCanvasWidth() / 2) / visExt.x, (gScreenInfo.getGameCanvasHeight() / 2) / visExt.y);

      Point ret = (*point - position) * scaleFactor + Point((gScreenInfo.getGameCanvasWidth() / 2), (gScreenInfo.getGameCanvasHeight() / 2));
      return ret;
   }
}


void ClientGame::renderSuspended()
{
   glColor(Colors::yellow);
   S32 textHeight = 20;
   S32 textGap = 5;
   S32 ypos = gScreenInfo.getGameCanvasHeight() / 2 - 3 * (textHeight + textGap);

   UserInterface::drawCenteredString(ypos, textHeight, "==> Game is currently suspended, waiting for other players <==");
   ypos += textHeight + textGap;
   UserInterface::drawCenteredString(ypos, textHeight, "When another player joins, the game will start automatically.");
   ypos += textHeight + textGap;
   UserInterface::drawCenteredString(ypos, textHeight, "When the game restarts, the level will be reset.");
   ypos += 2 * (textHeight + textGap);
   UserInterface::drawCenteredString(ypos, textHeight, "Press <SPACE> to resume playing now");
}


static Vector<DatabaseObject *> rawRenderObjects;
static Vector<BfObject *> renderObjects;
static Vector<BotNavMeshZone *> renderZones;


static void fillRenderZones()
{
   renderZones.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderZones.push_back(static_cast<BotNavMeshZone *>(rawRenderObjects[i]));
}

// Fills renderZones for drawing botNavMeshZones
static void populateRenderZones()
{
   rawRenderObjects.clear();
   BotNavMeshZone::getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, rawRenderObjects);
   fillRenderZones();
}


static void populateRenderZones(const Rect extentRect)
{
   rawRenderObjects.clear();
   BotNavMeshZone::getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, rawRenderObjects, extentRect);
   fillRenderZones();
}


void ClientGame::renderCommander()
{
   F32 zoomFrac = getCommanderZoomFraction();
   
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   Point worldExtents = (getUIManager()->getGameUserInterface()->mShowProgressBar ? getGameType()->mViewBoundsWhileLoading : 
                                                                                    mWorldExtents).getExtents();

   worldExtents.x *= canvasWidth  / F32(canvasWidth  - (UserInterface::horizMargin * 2));
   worldExtents.y *= canvasHeight / F32(canvasHeight - (UserInterface::vertMargin * 2));

   F32 aspectRatio = worldExtents.x / worldExtents.y;
   F32 screenAspectRatio = F32(canvasWidth) / F32(canvasHeight);

   if(aspectRatio > screenAspectRatio)
      worldExtents.y *= aspectRatio / screenAspectRatio;
   else
      worldExtents.x *= screenAspectRatio / aspectRatio;

   glPushMatrix();

   BfObject *controlObject = mConnectionToServer->getControlObject();
   Ship *ship = NULL;
   if(controlObject->getObjectTypeNumber() == PlayerShipTypeNumber)
      ship = static_cast<Ship *>(controlObject);      // This is the local player's ship
   
   Point position = ship ? ship->getRenderPos() : Point(0,0);

   Point visSize = ship ? computePlayerVisArea(ship) * 2 : worldExtents;
   Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;

   // Put (0,0) at the center of the screen
   glTranslatef(gScreenInfo.getGameCanvasWidth() * 0.5f, gScreenInfo.getGameCanvasHeight() * 0.5f, 0);    

   glScalef(canvasWidth / modVisSize.x, canvasHeight / modVisSize.y, 1);

   Point offset = (mWorldExtents.getCenter() - position) * zoomFrac + position;
   glTranslatef(-offset.x, -offset.y, 0);

   // zoomFrac == 1.0 when fully zoomed out to cmdr's map
   if(zoomFrac < 0.95)
      drawStars(1 - zoomFrac, offset, modVisSize);
 

   // Render the objects.  Start by putting all command-map-visible objects into renderObjects.  Note that this no longer captures
   // walls -- those will be rendered separately.
   rawRenderObjects.clear();

   if(ship->hasModule(ModuleSensor))
      mGameObjDatabase->findObjects((TestFunc)isVisibleOnCmdrsMapWithSensorType, rawRenderObjects);
   else
      mGameObjDatabase->findObjects((TestFunc)isVisibleOnCmdrsMapType, rawRenderObjects);

   renderObjects.clear();

   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(static_cast<BfObject *>(rawRenderObjects[i]));

   if(gServerGame && showDebugBots)
      for(S32 i = 0; i < getBotCount(); i++)
         renderObjects.push_back(getBot(i));

   // If we're drawing bot zones, get them now
   if(mDebugShowMeshZones)
      populateRenderZones();

   if(ship)
   {
      // Get info about the current player
      GameType *gt = getGameType();
      S32 playerTeam = -1;

      if(gt)
      {
         playerTeam = ship->getTeam();
         Color teamColor = *ship->getColor();

         for(S32 i = 0; i < renderObjects.size(); i++)
         {
            // Render ship visibility range, and that of our teammates
            if(isShipType(renderObjects[i]->getObjectTypeNumber()))
            {
               Ship *otherShip = static_cast<Ship *>(renderObjects[i]);

               // Get team of this object
               S32 otherShipTeam = otherShip->getTeam();
               if((otherShipTeam == playerTeam && getGameType()->isTeamGame()) || otherShip == ship)  // On our team (in team game) || the ship is us
               {
                  Point p = otherShip->getRenderPos();
                  Point visExt = computePlayerVisArea(otherShip);

                  glColor(teamColor * zoomFrac * 0.35f);
                  UserInterface::drawFilledRect(p.x - visExt.x, p.y - visExt.y, p.x + visExt.x, p.y + visExt.y);
               }
            }
         }

         fillVector.clear();
         mGameObjDatabase->findObjects(SpyBugTypeNumber, fillVector);

         // Render spy bug visibility range second, so ranges appear above ship scanner range
         for(S32 i = 0; i < fillVector.size(); i++)
         {
            SpyBug *sb = static_cast<SpyBug *>(fillVector[i]);

            if(sb->isVisibleToPlayer(playerTeam, getGameType()->isTeamGame()))
            {
               renderSpyBugVisibleRange(sb->getRenderPos(), teamColor);
               glColor(teamColor * 0.8f);     // Draw a marker in the middle
               drawCircle(sb->getRenderPos(), 2);
            }
         }
      }
   }

   // Now render the objects themselves
   renderObjects.sort(renderSortCompare);

   if(mDebugShowMeshZones)
      for(S32 i = 0; i < renderZones.size(); i++)
         renderZones[i]->render(0);

   // First pass
   for(S32 i = 0; i < renderObjects.size(); i++)
      renderObjects[i]->render(0);

   // Second pass
   Barrier::renderEdges(1, mSettings->getWallOutlineColor());    // Render wall edges

   if(mDebugShowMeshZones)
      for(S32 i = 0; i < renderZones.size(); i++)
         renderZones[i]->render(1);

   for(S32 i = 0; i < renderObjects.size(); i++)
      // Keep our spy bugs from showing up on enemy commander maps, even if they're known
 //     if(!(renderObjects[i]->getObjectTypeMask() & SpyBugType && playerTeam != renderObjects[i]->getTeam()))
         renderObjects[i]->render(1);

   getUIManager()->getGameUserInterface()->renderEngineeredItemDeploymentMarker(ship);

   glPopMatrix();
}


void ClientGame::resetZoomDelta()
{
   mCommanderZoomDelta = CommanderMapZoomTime;
}


void ClientGame::clearZoomDelta()
{
   mCommanderZoomDelta = 0;
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
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   BfObject *controlObject = mConnectionToServer->getControlObject();
   Ship *ship = static_cast<Ship *>(controlObject);

   Point position = ship->getRenderPos();

   S32 mapWidth = canvasWidth / 4;
   S32 mapHeight = canvasHeight / 4;
   S32 mapX = UserInterface::horizMargin;        // This may need to the the UL corner, rather than the LL one
   S32 mapY = canvasHeight - UserInterface::vertMargin - mapHeight;
   F32 mapScale = 0.1f;

   F32 vertices[] = {
         mapX, mapY,
         mapX, mapY + mapHeight,
         mapX + mapWidth, mapY + mapHeight,
         mapX + mapWidth, mapY
   };
   renderVertexArray(vertices, 4, GL_LINE_LOOP);


   glEnable(GL_SCISSOR_BOX);                    // Crop to overlay map display area
   glScissor(mapX, mapY + mapHeight, mapWidth, mapHeight);  // Set cropping window

   glPushMatrix();   // Set scaling and positioning of the overlay

   glTranslatef(mapX + mapWidth / 2.f, mapY + mapHeight / 2.f, 0);          // Move map off to the corner
   glScalef(mapScale, mapScale, 1);                                     // Scale map
   glTranslatef(-position.x, -position.y, 0);                           // Put ship at the center of our overlay map area

   // Render the objects.  Start by putting all command-map-visible objects into renderObjects
   Rect mapBounds(position, position);
   mapBounds.expand(Point(mapWidth * 2, mapHeight * 2));      //TODO: Fix

   rawRenderObjects.clear();
   if(ship->isModulePrimaryActive(ModuleSensor))
      mGameObjDatabase->findObjects((TestFunc)isVisibleOnCmdrsMapWithSensorType, rawRenderObjects);
   else
      mGameObjDatabase->findObjects((TestFunc)isVisibleOnCmdrsMapType, rawRenderObjects);

   renderObjects.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(static_cast<BfObject *>(rawRenderObjects[i]));


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

// ==> should move render to UIGame?

static Point screenSize, position;


void ClientGame::renderNormal()
{
   if(!hasValidControlObject())
      return;

   BfObject *object = mConnectionToServer->getControlObject();
   if(!object || object->getObjectTypeNumber() != PlayerShipTypeNumber)
      return;

   Ship *ship = static_cast<Ship *>(object);  // This is the local player's ship

   position.set(ship->getRenderPos());

   glPushMatrix();

   // Put (0,0) at the center of the screen
   glTranslatef(gScreenInfo.getGameCanvasWidth() / 2.f, gScreenInfo.getGameCanvasHeight() / 2.f, 0);       

   Point visExt = computePlayerVisArea(ship);
   glScalef((gScreenInfo.getGameCanvasWidth()  / 2) / visExt.x, 
            (gScreenInfo.getGameCanvasHeight() / 2) / visExt.y, 1);

   glTranslatef(-position.x, -position.y, 0);

   drawStars(1.0, position, visExt * 2);

   // Render all the objects the player can see
   screenSize.set(visExt);
   Rect extentRect(position - screenSize, position + screenSize);

   rawRenderObjects.clear();
   mGameObjDatabase->findObjects((TestFunc)isAnyObjectType, rawRenderObjects, extentRect);    // Use extent rects to quickly find objects in visual range

   renderObjects.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(static_cast<BfObject *>(rawRenderObjects[i]));

   // Normally a big no-no, we'll access the server's bot zones directly if we are running locally so we can visualize them without bogging
   // the game down with the normal process of transmitting zones from server to client.  The result is that we can only see zones on our local
   // server.
   if(mDebugShowMeshZones)
      populateRenderZones(extentRect);


   if(gServerGame && showDebugBots)
      for(S32 i = 0; i < getBotCount(); i++)
         renderObjects.push_back(getBot(i));

   renderObjects.sort(renderSortCompare);

   // Render in three passes, to ensure some objects are drawn above others
   for(S32 j = -1; j < 2; j++)
   {
      Barrier::renderEdges(j, mSettings->getWallOutlineColor());    // Render wall edges

      if(mDebugShowMeshZones)
         for(S32 i = 0; i < renderZones.size(); i++)
            renderZones[i]->render(j);

      for(S32 i = 0; i < renderObjects.size(); i++)
         renderObjects[i]->render(j);

      FXManager::render(j);
   }

   FXTrail::renderTrails();

   getUIManager()->getGameUserInterface()->renderEngineeredItemDeploymentMarker(ship);

   glPopMatrix();

   // Render current ship's energy
   if(ship)
      renderEnergyGuage(ship->mEnergy);    

   //renderOverlayMap();     // Draw a floating overlay map
}


void ClientGame::render()
{
   bool renderObjectsWhileLoading = false;

   if(!renderObjectsWhileLoading && !hasValidControlObject())
      return;

   if(getUIManager()->getGameUserInterface()->mShowProgressBar)
      renderCommander();
   else if(mGameSuspended)
      renderCommander();
   else if(mCommanderZoomDelta > 0)
      renderCommander();
   else
      renderNormal();
}


////////////////////////////////////////
////////////////////////////////////////

bool ClientGame::processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database, U32 id)
{
   if(!stricmp(argv[0], "BarrierMaker"))
   {
      if(argc >= 2)
      {
         WallItem *wallItem = new WallItem();  
         wallItem->initializeEditor();        // Only runs unselectVerts
        
         wallItem->processArguments(argc, argv, this);
         
         if(wallItem->getVertCount() < 2)     // Too small!  Need at least 2 points for a wall!
            delete wallItem;
         else
            addWallItem(wallItem, database);
      }
   }
   // TODO: Integrate code above with code above!!  EASY!!
   else if(!stricmp(argv[0], "BarrierMakerS") || !stricmp(argv[0], "PolyWall"))
   {
      //if(width)      // BarrierMakerS still width, though we ignore it
      //   barrier.width = F32(atof(argv[1]));
      //else           // PolyWall does not have width specified
      //   barrier.width = 1;

      if(argc >= 2)
      {
         PolyWall *polywall = new PolyWall();  
         
         S32 skipArgs = 0;
         if(!stricmp(argv[0], "BarrierMakerS"))
         {
            logprintf(LogConsumer::LogLevelError, "BarrierMakerS has been deprecated.  Please use PolyWall instead.");

            skipArgs = 1;
         }

         polywall->initializeEditor();     // Only runs unselectVerts

         polywall->processArguments(argc - skipArgs, argv + skipArgs, this);
         
         if(polywall->getVertCount() >= 2)
            addPolyWall(polywall, database);
         else
            delete polywall;
      }
   }
   else if(!stricmp(argv[0], "Robot"))
   {
      // For now, we'll just make a list of robots and associated params, and write that out when saving the level.  We'll leave robots as
      // full-fledged objects on the server (instead of pseudoItems here).
      string robot = "";

      for(S32 i = 0; i < argc; i++)
      {
         if(i > 0)
            robot.append(" ");

         robot.append(argv[i]);
      }

      EditorUserInterface::robots.push_back(robot);
   }
   else if(!stricmp(argv[0], "Zone")) 
   {
      Zone *zone = new Zone();

      bool validArgs = zone->processArguments(argc - 1, argv + 1, this);

      if(validArgs)
      {
         zone->setUserAssignedId(id);
         zone->addToGame(this, database);
      }
      else
      {
         logprintf(LogConsumer::LogWarning, "Invalid arguments in object \"%s\" in level \"%s\"", argv[0], levelFileName.c_str());
         delete zone;
      }
   }
      
   else 
      return false;

   return true;
}


// Add polywall item to game
void ClientGame::addPolyWall(PolyWall *polyWall, GridDatabase *database)
{
   polyWall->addToGame(this, database);
   polyWall->onGeomChanged(); 
}


// Add polywall item to game
void ClientGame::addWallItem(WallItem *wallItem, GridDatabase *database)
{
   wallItem->addToGame(this, database);
   wallItem->processEndPoints();
}


AbstractTeam *ClientGame::getNewTeam()
{
   return new EditorTeam;
}


void ClientGame::setSelectedEngineeredObject(U32 objectType)
{
   getUIManager()->getGameUserInterface()->getEngineerHelper(this)->setSelectedEngineeredObject(objectType);
}

};

