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

#include "masterConnection.h"
#include "gameNetInterface.h"
#include "IniFile.h"             // For CIniFile def

#include "barrier.h"
#include "gameType.h"

#include "UIManager.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "UIErrorMessage.h"

#include "EditorTeam.h"

#include "Colors.h"

#include "stringUtils.h"

#include <boost/shared_ptr.hpp>
#include <sys/stat.h>
#include <cmath>

using namespace TNL;

namespace Zap
{

// Global Game objects
Vector<ClientGame *> gClientGames;


////////////////////////////////////////
////////////////////////////////////////

// Constructor
ClientGame::ClientGame(const Address &bindAddress, GameSettings *settings) : Game(bindAddress, settings)
{
   mUserInterfaceData = new UserInterfaceData();

   mInCommanderMap        = false;
   mGameIsRunning         = true;      // Only matters when game is suspended
   mSeenTimeOutMessage    = false;

   // Transition time between regular map and commander's map; in ms, higher = slower
   mCommanderZoomDelta.setPeriod(350);

   mRemoteLevelDownloadFilename = "downloaded.level";

   mUIManager = new UIManager(this);               // Gets deleted in destructor

   mClientInfo = new FullClientInfo(this, NULL, mSettings->getPlayerName(), false);  // Will be deleted in destructor

   mScreenSaverTimer.reset(59 * 1000);         // Fire screen saver supression every 59 seconds

   mUi = mUIManager->getGameUserInterface();

   for(S32 i = 0; i < JoystickAxesDirectionCount; i++)
      mJoystickInputs[i] = 0;
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


void ClientGame::joinLocalGame(GameNetInterface *remoteInterface)
{
   // Much of the time, this may seem pointless, but if we arrive here via the editor, we need to swap out the editor's team manager for
   // the one used by the game.  If we don't we'll clobber the editor's copy, and we'll get crashes in the team definition (F2) menu.
   setActiveTeamManager(&mTeamManager);

   mClientInfo->setRole(ClientInfo::RoleOwner);       // Local connection is always owner

   getUIManager()->activate(GameUI);
   GameConnection *gameConnection = new GameConnection(this);
 
   setConnectionToServer(gameConnection);

   gameConnection->connectLocal(getNetInterface(), remoteInterface);

   // Note that gc and gameConnection aren't the same, nor are gc->getClientInfo() and mClientInfo the same.
   // I _think_ gc is the server view of the local connection, where as gameConnection is the client's view.
   // Likewise with the clientInfos.  A little confusing, as they really represent the same thing in a way.  But different.
   TNLAssert(dynamic_cast<GameConnection *>(gameConnection->getRemoteConnectionObject()), 
               "This should never be NULL here -- if it is, it means our connection to ourselves has failed for some reason");

   GameConnection *gc = static_cast<GameConnection *>(gameConnection->getRemoteConnectionObject()); 
   gc->onLocalConnection();
}


// Player has selected a game from the QueryServersUserInterface, and is ready to join
// Also get here when hosting a game
void ClientGame::joinRemoteGame(Address remoteAddress, bool isFromMaster)
{
   // Much of the time, this may seem pointless, but if we arrive here via the editor, we need to swap out the editor's team manager for
   // the one used by the game.  If we don't we'll clobber the editor's copy, and we'll get crashes in the team definition (F2) menu.
   // Do we need this for joining a remote game?
   setActiveTeamManager(&mTeamManager);    

   mClientInfo->setRole(ClientInfo::RoleNone);        // Start out with no permissions, server will upgrade if the proper pws are provided
   
   MasterServerConnection *connToMaster = getConnectionToMaster();

   bool useArrangedConnection = isFromMaster && connToMaster && connToMaster->getConnectionState() == NetConnection::Connected;

   getUIManager()->activate(GameUI);

   if(useArrangedConnection)  // Request arranged connection
      connToMaster->requestArrangedConnection(remoteAddress);

   else                       // Try a direct connection
   {
      GameConnection *gameConnection = new GameConnection(this);

      setConnectionToServer(gameConnection);

      // Connect to a remote server, but not via the master server
      gameConnection->connect(getNetInterface(), remoteAddress);  
   }
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

   getUIManager()->disableLevelLoadDisplay(false);

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


bool ClientGame::isConnectedToServer() const
{
   return mConnectionToServer.isValid() && mConnectionToServer->getConnectionState() == NetConnection::Connected;
}


GameConnection *ClientGame::getConnectionToServer() const
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


void ClientGame::startLoadingLevel(F32 lx, F32 ly, F32 ux, F32 uy, bool engineerEnabled)
{
   mObjectsLoaded = 0;              // Reset item counter
   clearSparks();
   
   mRobots.clear();

   mUi->startLoadingLevel(lx, ly, ux, uy, engineerEnabled);
}


void ClientGame::doneLoadingLevel()
{
   computeWorldObjectExtents();              // Make sure our world extents reflect all the objects we've loaded
   Barrier::prepareRenderingGeometry(this);  // Get walls ready to render

   mUi->doneLoadingLevel();
}


ClientInfo *ClientGame::getClientInfo() const
{
   return mClientInfo;
}


ClientInfo *ClientGame::getLocalRemoteClientInfo() const
{
   return mLocalRemoteClientInfo;
}


void ClientGame::setSpawnDelayed(bool spawnDelayed)
{
   if(getClientInfo()->isSpawnDelayed() != spawnDelayed)    // Yes, yes, we heard you the first time!
      return;

   if(!spawnDelayed)
   {
      unsuspendGame();

      clearSparks();
   }
}


void ClientGame::clearSparks()
{
   mUi->clearSparks();
}


void ClientGame::emitBlast(const Point &pos, U32 size)
{
   mUi->emitBlast(pos, size);
}


void ClientGame::emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2)
{
   mUi->emitBurst(pos, scale, color1, color2);
}


void ClientGame::emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation)
{
   mUi->emitDebrisChunk(points, color, pos, vel, ttl, angle, rotation);
}


void ClientGame::emitTextEffect(const string &text, const Color &color, const Point &pos)
{
   mUi->emitTextEffect(text, color, pos);
}


void ClientGame::emitSpark(const Point &pos, const Point &vel, const Color &color, S32 ttl, UI::SparkType sparkType)
{
   mUi->emitSpark(pos, vel, color, ttl, sparkType);
}


void ClientGame::emitExplosion(const Point &pos, F32 size, const Color *colorArray, U32 numColors)
{
   mUi->emitExplosion(pos, size, colorArray, numColors);
}


void ClientGame::emitTeleportInEffect(const Point &pos, U32 type)
{
   mUi->emitTeleportInEffect(pos, type);
}


Color ShipExplosionColors[] = {
   Colors::red,  Color(0.9, 0.5, 0),  Colors::white,     Colors::yellow,
   Colors::red,  Color(0.8, 1.0, 0),  Color(1, 0.5, 0),  Colors::white,
   Colors::red,  Color(0.9, 0.5, 0),  Colors::white,     Colors::yellow,
};


void ClientGame::emitShipExplosion(const Point &pos)
 {
   playSoundEffect(SFXShipExplode, pos);

   F32 a = TNL::Random::readF() * 0.4f + 0.5f;
   F32 b = TNL::Random::readF() * 0.2f + 0.9f;

   F32 c = TNL::Random::readF() * 0.15f + 0.125f;
   F32 d = TNL::Random::readF() * 0.2f + 0.9f;

   emitExplosion(pos, 0.9f, ShipExplosionColors, ARRAYSIZE(ShipExplosionColors));
   emitBurst(pos, Point(a,c), Color(1,1,0.25), Colors::red);
   emitBurst(pos, Point(b,d), Colors::yellow, Color(0,0.75,0));
 }



SFXHandle ClientGame::playSoundEffect(U32 profileIndex, F32 gain) const
{
   return mUi->playSoundEffect(profileIndex, gain);
}


SFXHandle ClientGame::playSoundEffect(U32 profileIndex, const Point &position) const
{
   return mUi->playSoundEffect(profileIndex, position);
}


SFXHandle ClientGame::playSoundEffect(U32 profileIndex, const Point &position, const Point &velocity, F32 gain) const
{
   return mUi->playSoundEffect(profileIndex, position, velocity, gain);
}


void ClientGame::playNextTrack() const
{
   mUi->playNextTrack();
}


void ClientGame::playPrevTrack() const
{
   mUi->playPrevTrack();
}


void ClientGame::queueVoiceChatBuffer(const SFXHandle &effect, const ByteBufferPtr &p) const
{
   mUi->queueVoiceChatBuffer(effect, p);
}


void ClientGame::updateModuleSounds(const Point &pos, const Point &vel, const LoadoutTracker &loadout)
{
   const S32 moduleSFXs[ModuleCount] =
   {
      SFXShieldActive,
      SFXShipBoost,
      SFXNone,       // No more sensor
      SFXRepairActive,
      SFXUIBoop,     // Need better sound...
      SFXCloakActive,
      SFXNone,       // Armor... tough, but he don't say much
   };
   
   for(U32 i = 0; i < ModuleCount; i++)
   {
      if(loadout.isModulePrimaryActive(ShipModule(i)) && moduleSFXs[i] != SFXNone)
      {
         if(mModuleSound[i].isValid())
            mUi->setMovementParams(mModuleSound[i], pos, vel);
         else if(moduleSFXs[i] != -1)
            mModuleSound[i] = playSoundEffect(moduleSFXs[i], pos, vel);
      }
      else
      {
         if(mModuleSound[i].isValid())
         {
            mUi->stopSoundEffect(mModuleSound[i]);
            mModuleSound[i] = NULL;
         }
      }
   }
}


// User selected Switch Teams meunu item
void ClientGame::switchTeams()
{
   if(!mGameType)    // I think these GameType checks are not needed
      return;

   // If there are only two teams, just switch teams and skip the rigamarole
   if(getTeamCount() == 2)
   {
      BfObject *controlObject = getConnectionToServer()->getControlObject();
      if(!controlObject || !isShipType(controlObject->getObjectTypeNumber()))
         return;

      Ship *ship = static_cast<Ship *>(controlObject);   // Returns player's ship...

      changeOwnTeam(1 - ship->getTeam());    // If two teams, team will either be 0 or 1, so "1 - " will toggle
      getUIManager()->reactivate(GameUI);    // Jump back into the game (this option takes place immediately)
   }
   else
   {
      // Show menu to let player select a new team... future home of mUi->showTeamMenu()
      TeamMenuUserInterface *ui = getUIManager()->getTeamMenuUserInterface();
      ui->nameToChange = getPlayerName();
      getUIManager()->activate(ui);                  
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
}


F32 ClientGame::getUIFadeFactor() const
{
   return 1 - mTimeToSuspend.getFraction();     
}


// Provide access to these annoying bools
bool ClientGame::isSpawnDelayed() const
{ 
   return getClientInfo()->isSpawnDelayed();
}


// Tells the server to spawn delay us... server may incur a penalty when we unspawn
void ClientGame::requestSpawnDelayed()
{
   getConnectionToServer()->c2sPlayerRequestSpawnDelayed();
}


U32 ClientGame::getReturnToGameDelay() const
{
   return mClientInfo->getReturnToGameTime();
}


string ClientGame::getPlayerName()     const { return mSettings->getPlayerName();     }
string ClientGame::getPlayerPassword() const { return mSettings->getPlayerPassword(); }


void ClientGame::userEnteredLoginCredentials(const string &name, const string &password, bool savePassword)
{
   getClientInfo()->setName(name);
   getSettings()->setLoginCredentials(name, password, savePassword);      // Saves to INI

   mNextMasterTryTime = 0;                   // Triggers connection attempt with master

   setReadyToConnectToMaster(true);

   seedRandomNumberGenerator(name);

   if(getConnectionToServer())               // Rename while in game server, if connected
      getConnectionToServer()->c2sRenameClient(name.c_str());
}


// When server corrects capitalization of name or similar
void ClientGame::correctPlayerName(const string &name)
{
   getClientInfo()->setName(name);
   getSettings()->updatePlayerName(name);    // Saves to INI
}


void ClientGame::displayShipDesignChangedMessage(const LoadoutTracker &loadout, const char *msgToShowIfLoadoutsAreTheSame)
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
                              gt->levelHasLoadoutZone() ? "enter Loadout Zone to activate changes" : 
                                                          "changes will be activated when you respawn");
      }
   }
}


// We might get here if a script ran amok in the editor...  nothing to do, really
void ClientGame::deleteLevelGen(LuaLevelGenerator *levelgen)
{
   // Do nothing
}


static void joystickUpdateMove(ClientGame *game, GameSettings *settings, Move *theMove)
{
   // One of each of left/right axis and up/down axis should be 0 by this point
   // but let's guarantee it..   why?
   theMove->x = game->mJoystickInputs[JoystickMoveAxesRight] - 
                game->mJoystickInputs[JoystickMoveAxesLeft];
   theMove->x = MAX(theMove->x, -1);
   theMove->x = MIN(theMove->x, 1);
   theMove->y =  game->mJoystickInputs[JoystickMoveAxesDown] - 
                 game->mJoystickInputs[JoystickMoveAxesUp];
   theMove->y = MAX(theMove->y, -1);
   theMove->y = MIN(theMove->y, 1);

   //logprintf(
   //      "Joystick axis values. Move: Left: %f, Right: %f, Up: %f, Down: %f\nShoot: Left: %f, Right: %f, Up: %f, Down: %f ",
   //      mJoystickInputs[MoveAxesLeft],  mJoystickInputs[MoveAxesRight],
   //      mJoystickInputs[MoveAxesUp],    mJoystickInputs[MoveAxesDown],
   //      mJoystickInputs[ShootAxesLeft], mJoystickInputs[ShootAxesRight],
   //      mJoystickInputs[ShootAxesUp],   mJoystickInputs[ShootAxesDown]
   //      );

   //logprintf(
   //         "Move values. Move: Left: %f, Right: %f, Up: %f, Down: %f",
   //         theMove->left, theMove->right,
   //         theMove->up, theMove->down
   //         );


   //logprintf("XY from shoot axes. x: %f, y: %f", x, y);


   Point p(game->mJoystickInputs[JoystickShootAxesRight] - 
           game->mJoystickInputs[JoystickShootAxesLeft], 
                             game->mJoystickInputs[JoystickShootAxesDown]  - 
                             game->mJoystickInputs[JoystickShootAxesUp]);

   F32 fact =  p.len();

   if(fact > 0.66f)        // It requires a large movement to actually fire...
   {
      theMove->angle = atan2(p.y, p.x);
      theMove->fire = true;
   }
   else if(fact > 0.25)    // ...but you can change aim with a smaller one
   {
      theMove->angle = atan2(p.y, p.x);
      theMove->fire = false;
   }
   else
      theMove->fire = false;
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

   mCurrentTime += timeDelta;

   computeWorldObjectExtents();

   if(mCommanderZoomDelta.getCurrent() > 0)               
      mUi->onMouseMoved();     // Keep ship pointed towards mouse cmdrs map zoom transition

   mCommanderZoomDelta.update(timeDelta);


   Move *theMove = mUi->getCurrentMove();       // Get move from keyboard input

   // Overwrite theMove if we're using joystick (also does some other essential joystick stuff)
   // We'll also run this while in the menus so if we enter keyboard mode accidentally, it won't
   // kill the joystick.  The design of combining joystick input and move updating really sucks.
   if(mSettings->getInputCodeManager()->getInputMode() == InputModeJoystick || 
      getUIManager()->getCurrentUI() == getUIManager()->getOptionsMenuUserInterface())
         joystickUpdateMove(this, mSettings, theMove);

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
      for(BfObject *obj = idlingObjects.nextList, *objNext; obj != NULL; obj = objNext)
      {
         objNext = obj->nextList;      // Just in case this object is deleted inside idle function

         if(obj->isDeleted())
            continue;

         if(obj == controlObject)      // That is, if we are idling the local ship
         {
            obj->setCurrentMove(*theMove);
            obj->idle(BfObject::ClientIdlingLocalShip);  // on client, object is our control object
         }
         else
         {
            Move m = obj->getCurrentMove();
            m.time = timeDelta;
            obj->setCurrentMove(m);
            obj->idle(BfObject::ClientIdlingNotLocalShip);    // on client, object is not our control object
         }
      }

      if(mGameType)
         mGameType->idle(BfObject::ClientIdlingNotLocalShip, timeDelta);

      if(controlObject)
         mUi->setListenerParams(controlObject->getPos(), controlObject->getVel());


      // Check to see if there are any items near the ship we need to display help for
      bool mShowingHelp = true;     // TODO: Set elsewhere
      if(mShowingHelp && controlObject)
      {
         Rect searchRect = Rect(controlObject->getPos(), 400);
         fillVector.clear();
         mGameObjDatabase->findObjects(RepairItemTypeNumber,   fillVector, searchRect);
         mGameObjDatabase->findObjects(TestItemTypeNumber,     fillVector, searchRect);
         mGameObjDatabase->findObjects(ResourceItemTypeNumber, fillVector, searchRect);
      }

      for(S32 i = 0; i < fillVector.size(); i++)
      {
         if(fillVector[i]->getObjectTypeNumber() == RepairItemTypeNumber)
            addHelpItem(RepairItemSpottedItem);
         else if(fillVector[i]->getObjectTypeNumber() == TestItemTypeNumber)
            addHelpItem(TestItemSpottedItem);
         else if(fillVector[i]->getObjectTypeNumber() == ResourceItemTypeNumber)
            addHelpItem(ResourceItemSpottedItem);
      }
   }

   processDeleteList(timeDelta);                         // Delete any objects marked for deletion
   
   mUi->processAudio(timeDelta, mSettings->getIniSettings()->sfxVolLevel,
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


void ClientGame::addHelpItem(HelpItem item)
{
   mUi->addHelpItem(item);
}


void ClientGame::gotServerListFromMaster(const Vector<IPAddress> &serverList)
{
   getUIManager()->gotServerListFromMaster(serverList);
}


// Message relayed through master -- global chat system
void ClientGame::gotGlobalChatMessage(const char *playerNick, const char *message, bool isPrivate)
{
   getUIManager()->gotGlobalChatMessage(playerNick, message, isPrivate, false, false);
}


// Message relayed through server -- normal chat system
void ClientGame::gotChatMessage(const StringTableEntry &clientName, const StringPtr &message, bool global)
{
   if(isOnMuteList(clientName.getString()))
      return;

   const Color *color = global ? &Colors::globalChatColor : &Colors::teamChatColor;
   mUi->onChatMessageReceived(*color, "%s: %s", clientName.getString(), message.getString());
}


void ClientGame::gotChatPM(const StringTableEntry &fromName, const StringTableEntry &toName, const StringPtr &message)
{
   ClientInfo *fullClientInfo = getClientInfo();

   Color color = Colors::yellow;

   if(fullClientInfo->getName() == toName && toName == fromName)      // Message sent to self
      mUi->onChatMessageReceived(color, "%s: %s", toName.getString(), message.getString());

   else if(fullClientInfo->getName() == toName)                       // To this player
      mUi->onChatMessageReceived(color, "from %s: %s", fromName.getString(), message.getString());

   else if(fullClientInfo->getName() == fromName)                     // From this player
      mUi->onChatMessageReceived(color, "to %s: %s", toName.getString(), message.getString());

   else  
      TNLAssert(false, "Should never get here... shouldn't be able to see PM that is not from or not to you"); 
}


void ClientGame::gotAnnouncement(const string &announcement)
{
	mUi->setAnnouncement(announcement);
}


void ClientGame::gotVoiceChat(const StringTableEntry &from, const ByteBufferPtr &voiceBuffer)
{
   ClientInfo *clientInfo = findClientInfo(from);

   if(!clientInfo)
      return;

   if(isOnVoiceMuteList(from.getString()))
      return;

   clientInfo->playVoiceChat(voiceBuffer);
}


void ClientGame::activatePlayerMenuUi()
{
   PlayerMenuUserInterface *ui = mUIManager->getPlayerMenuUserInterface();

   ui->action = PlayerMenuUserInterface::ChangeTeam;
   mUIManager->activate(ui);
}


void ClientGame::renderBasicInterfaceOverlay(bool scoreboardVisible)
{
   mUi->renderBasicInterfaceOverlay(scoreboardVisible);
}


void ClientGame::gameTypeIsAboutToBeDeleted()
{
   // Quit EngineerHelper when level changes, or when current GameType get removed
   mUi->quitEngineerHelper();
}


void ClientGame::setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks)
{
   getUIManager()->setPlayersInGlobalChat(playerNicks);
}


void ClientGame::playerJoinedGlobalChat(const StringTableEntry &playerNick)
{
   getUIManager()->playerJoinedGlobalChat(playerNick);
}


void ClientGame::playerLeftGlobalChat(const StringTableEntry &playerNick)
{
   getUIManager()->playerLeftGlobalChat(playerNick);
}


void ClientGame::sendChat(bool isGlobal, const StringPtr &message)
{
   if(getGameType())
      getGameType()->c2sSendChat(isGlobal, message);
}


void ClientGame::sendChatSTE(bool global, const StringTableEntry &message) const
{
   if(getGameType())
      getGameType()->c2sSendChatSTE(global, message);;
}


void ClientGame::sendCommand(const StringTableEntry &cmd, const Vector<StringPtr> &args)
{
   if(getGameType())
      getGameType()->c2sSendCommand(cmd, args);
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
      if(mUi->isInScoreboardMode())
         mGameType->c2sRequestScoreboardUpdates(true);

      if(showMessage)
         displayMessage(Color(0.6f, 0.6f, 0.8f), "Welcome to the game!");     // SysMsg
   }
   else     // A remote player has joined the fray
   {
      if(showMessage) // Might as well not display when no sound being played, prevents message flood of many players join loading new GameType
         displayMessage(Color(0.6f, 0.6f, 0.8f), "%s joined the game.", clientInfo->getName().getString()); // SysMsg

   }
   if(playAlert)
      playSoundEffect(SFXPlayerJoined, 1);

   mUIManager->getGameUserInterface()->onPlayerJoined();

   mGameType->updateLeadingPlayerAndScore();
}


// Another player has just left the game
void ClientGame::onPlayerQuit(const StringTableEntry &name)
{
   removeFromClientList(name);

   displayMessage(Color(0.6f, 0.6f, 0.8f), "%s left the game.", name.getString());     // SysMsg
   playSoundEffect(SFXPlayerLeft, 1);

   mUIManager->getGameUserInterface()->onPlayerQuit();
}


// Server tells the GameType that the game is now over.  We in turn tell the UI, which in turn notifies its helpers.
// This begins the phase of showing the post-game scoreboard.
void ClientGame::setGameOver()
{
   mUi->onGameOver();
}


// Gets run when game is really and truly over, after post-game scoreboard is displayed.  Over.
void ClientGame::onGameOver()
{
   clearClientList();                   // Erase all info we have about fellow clients

   mUIManager->getGameUserInterface()->onGameOver();

   // Kill any objects lingering in the database, such as forcefields
   getGameObjDatabase()->removeEverythingFromDatabase();    
}


void ClientGame::quitEngineerHelper()
{
   mUi->quitEngineerHelper();
}


// Called by Ship::unpack() -- loadouts are transmitted via the ship object
// Data flow: Ship->ClientGame->GameUserInterface->LoadoutIndicator
void ClientGame::newLoadoutHasArrived(const LoadoutTracker &loadout)
{
   mUi->newLoadoutHasArrived(loadout);
}


void ClientGame::setActiveWeapon(U32 weaponIndex)
{
   mUi->setActiveWeapon(weaponIndex);
}


bool ClientGame::isShowingDebugShipCoords()
{
   return mUi->isShowingDebugShipCoords();
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


void ClientGame::setHighScores(const Vector<StringTableEntry> &groupNames, const Vector<string> &names, const Vector<string> &scores) const
{
   getUIManager()->setHighScores(groupNames, names, scores);
}


void ClientGame::setNeedToUpgrade(bool needToUpgrade)
{
  getUIManager()->getMainMenuUserInterface()->setNeedToUpgrade(needToUpgrade);
}


void ClientGame::displayMessage(const Color &msgColor, const char *format, ...) const
{
   va_list args;
   char message[MAX_CHAT_MSG_LENGTH]; 

   va_start(args, format);
   vsnprintf(message, sizeof(message), format, args); 
   va_end(args);
    
   mUi->displayMessage(msgColor, message);
}


void ClientGame::gotPermissionsReply(ClientInfo::ClientRole role)
{
   static string ownerPassSuccessMsg = "You've been granted ownership permissions of this server";
   static string adminPassSuccessMsg = "You've been granted permission to manage players and change levels";
   static string levelPassSuccessMsg = "You've been granted permission to change levels";

   string *message;
   if(role == ClientInfo::RoleOwner)
      message = &ownerPassSuccessMsg;
   else if(role == ClientInfo::RoleAdmin)
      message = &adminPassSuccessMsg;
   else if(role == ClientInfo::RoleLevelChanger)
      message = &levelPassSuccessMsg;

   // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
   if(getUIManager()->getCurrentUI()->getMenuID() == GameMenuUI)
      getUIManager()->getGameMenuUserInterface()->mMenuSubTitle = *message;
   else
      displayMessage(Colors::cmdChatColor, (*message).c_str());      // ChatCmd
}


void ClientGame::gotWrongPassword()
{
   static const char *levelPassFailureMsg = "Incorrect password";

   // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
   if(getUIManager()->getCurrentUI()->getMenuID() == GameMenuUI)
      getUIManager()->getGameMenuUserInterface()->mMenuSubTitle = levelPassFailureMsg;
   else
      displayMessage(Colors::cmdChatColor, levelPassFailureMsg);     // ChatCmd
}

void ClientGame::gotPingResponse(const Address &address, const Nonce &nonce, U32 clientIdentityToken)
{
   getUIManager()->gotPingResponse(address, nonce, clientIdentityToken);
}


void ClientGame::gotQueryResponse(const Address &address, const Nonce &nonce, const char *serverName, const char *serverDescr, 
                                  U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired)
{
   getUIManager()->gotQueryResponse(address, nonce, serverName, serverDescr, playerCount, 
                                    maxPlayers, botCount, dedicated, test, passwordRequired);
}


void ClientGame::shutdownInitiated(U16 time, const StringTableEntry &name, const StringPtr &reason, bool originator)
{
   mUi->shutdownInitiated(time, name, reason, originator);
}


void ClientGame::cancelShutdown() 
{ 
   mUi->cancelShutdown(); 
}


// Returns true if we have owner privs, displays error message and returns false if not
bool ClientGame::hasOwner(const char *failureMessage)
{
   if(mClientInfo->isOwner())
      return true;

   displayErrorMessage(failureMessage);
   return false;
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


void ClientGame::gotEngineerResponseEvent(EngineerResponseEvent event)
{
   switch(event)
   {
      case EngineerEventTurretBuilt:         // fallthrough ok
      case EngineerEventForceFieldBuilt:
      case EngineerEventTeleporterExitBuilt:
         mUi->exitHelper();
         break;

      case EngineerEventTeleporterEntranceBuilt:
         setSelectedEngineeredObject(EngineeredTeleporterExit);
         break;

      default:
         break;
   }
}


// Send a message to the server that we are (or are not) busy chatting
void ClientGame::setBusyChatting(bool isBusy)
{
   GameConnection *conn = getConnectionToServer();
   if(conn)
      conn->c2sSetIsBusy(isBusy);
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

      else if(type == GameConnection::OwnerPassword)
         mSettings->forgetOwnerPassword(gc->getServerName());
   }
   else                    // Non-empty password
   {
      // Save the password so the user need not enter it again the next time they're on this server
      if(type == GameConnection::LevelChangePassword)
         mSettings->saveLevelChangePassword(gc->getServerName(), words[1]);
         
      else if(type == GameConnection::AdminPassword)
         mSettings->saveAdminPassword(gc->getServerName(), words[1]);

      else if(type == GameConnection::OwnerPassword)
         mSettings->saveAdminPassword(gc->getServerName(), words[1]);
   }
}


// User entered password for joining a game
void ClientGame::submitServerAccessPassword(const Address &connectAddress, const char *password)
{
   mEnteredServerAccessPassword = password;
   joinRemoteGame(connectAddress, false);  // false: Not from master
}


// User entered password to get permissions on the server
bool ClientGame::submitServerPermissionsPassword(const char *password)
{
   mEnteredServerPermsPassword = password;

   GameConnection *gameConnection = getConnectionToServer();

   if(gameConnection)
   {
      gameConnection->submitPassword(password);
      return true;
   }

   return false;
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
         ui->setMessage(2, "Unable to connect to the server.  Received message:");
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
         string serverName = getUIManager()->getLastSelectedServerName();
         GameSettings::deleteServerPassword(serverName);
   
         getUIManager()->setConnectAddressAndActivatePasswordEntryUI(Address(serverAddress));

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
            ui->setMessage(3, "Error number: " + itos(reason));
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
         ui->setMessage(2, "Unable to connect to the server.  Received message:");
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
         // no errors when client disconnect (when quitting bitfighter)
      case NetConnection::ReasonAnonymous:
         // Anonymous connections are disconnected quickly, usually after retrieving some data
         break;

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


void ClientGame::runCommand(const char *command)
{
   ChatHelper::runCommand(this, command);
}


string ClientGame::getRequestedServerName()
{
   return getUIManager()->getLastSelectedServerName();
}


string ClientGame::getEnteredServerAccessPassword()
{
   return mEnteredServerAccessPassword;
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

   mUi->displayErrorMessage(message);
}


void ClientGame::displaySuccessMessage(const char *format, ...)
{
   static char message[256];

   va_list args;

   va_start(args, format);
   vsnprintf(message, sizeof(message), format, args);
   va_end(args);

   mUi->displaySuccessMessage(message);
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


Ship *ClientGame::findShip(const StringTableEntry &clientName)
{
   fillVector.clear();

   getGameObjDatabase()->findObjects(PlayerShipTypeNumber, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      Ship *ship = static_cast<Ship *>(fillVector[i]);
      ClientInfo *clientInfo = ship->getClientInfo();
      // Due to spybug scoping ships when not ready yet, we might not have ClientInfo yet
      // Also clientInfo->getName() can be NULL here somehow?  Player leaves at the right moment?
      if(clientInfo && clientInfo->getName() && clientInfo->getName() == clientName)
         return ship;
   }

   return NULL;
}


// Toggles commander's map activation status
void ClientGame::toggleCommanderMap()
{
   mInCommanderMap = !mInCommanderMap;
   mCommanderZoomDelta.invert();

   if(mInCommanderMap)
      playSoundEffect(SFXUICommUp);
   else
      playSoundEffect(SFXUICommDown);


   GameConnection *conn = getConnectionToServer();

   if(conn)
   {
      if(mInCommanderMap)
         conn->c2sRequestCommanderMap();
      else
         conn->c2sReleaseCommanderMap();
   }
}


UIManager *ClientGame::getUIManager() const
{
   return mUIManager;
}


GameUserInterface *ClientGame::getUi() const
{
   return mUi;
}


bool ClientGame::getInCommanderMap()
{
   return mInCommanderMap;
}


void ClientGame::setInCommanderMap(bool inCommanderMap)
{
   mInCommanderMap = inCommanderMap;
   mCommanderZoomDelta.clear();
}


F32 ClientGame::getCommanderZoomFraction() const
{
   return mInCommanderMap ? 1 - mCommanderZoomDelta.getFraction() : mCommanderZoomDelta.getFraction();
}


// Called from renderObjectiveArrow() & ship's onMouseMoved() when in commander's map
Point ClientGame::worldToScreenPoint(const Point *point,  S32 canvasWidth, S32 canvasHeight) const
{
   Ship *ship = getLocalPlayerShip();

   if(!ship)
      return Point(0,0);

   Point position = ship->getRenderPos();    // Ship's location (which will be coords of screen's center)
   
   if(mInCommanderMap || mCommanderZoomDelta.getCurrent() > 0)    // In commander's map, or zooming in/out
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

      Point ret = (*point - offset) * visScale + Point((canvasWidth / 2), (canvasHeight / 2));
      return ret;
   }
   else                       // Normal map view
   {
      Point visExt = computePlayerVisArea(ship);
      Point scaleFactor((canvasWidth / 2) / visExt.x, (canvasHeight / 2) / visExt.y);

      Point ret = (*point - position) * scaleFactor + Point((canvasWidth / 2), (canvasHeight / 2));
      return ret;
   }
}


void ClientGame::render()
{
   UIID currentUI = mUIManager->getCurrentUI()->getMenuID();

   // Not in the Game UI (or one of its submenus)...
   if(currentUI != GameUI && !mUIManager->cameFrom(GameUI))
      return;

   // We render the cmdrs map during the transition
   if(mInCommanderMap || mCommanderZoomDelta.getCurrent() > 0)
      mUi->renderCommander(this);
   else
      mUi->renderNormal(this);
}


////////////////////////////////////////
////////////////////////////////////////

bool ClientGame::processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database, S32 id)
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

      mRobots.push_back(robot);
   }
      
   else 
      return false;

   return true;
}


// Add polywall item to game
void ClientGame::addPolyWall(BfObject *polyWall, GridDatabase *database)
{
   Parent::addPolyWall(polyWall, database);
   polyWall->onGeomChanged(); 
}


// Add polywall item to game
void ClientGame::addWallItem(BfObject *wallItem, GridDatabase *database)
{
   Parent::addWallItem(wallItem, database);

   // Do we want to run onGeomChanged here instead?  If so, we can combine with addPolyWall.
   static_cast<WallItem *>(wallItem)->processEndPoints();      
}


AbstractTeam *ClientGame::getNewTeam()
{
   return new EditorTeam;
}


void ClientGame::setSelectedEngineeredObject(U32 objectType)
{
   mUi->setSelectedEngineeredObject(objectType);
}


const Vector<string> *ClientGame::getLevelRobotLines() const
{
   return &mRobots;
}


Ship *ClientGame::getLocalPlayerShip() const
{
   if(!mConnectionToServer)      // Needed if we get here while joining a server in cmdrs map mode
      return NULL;

   BfObject *object = mConnectionToServer->getControlObject();

   if(!object)
      return NULL;

   // Completely unneeded at this point -- planyes can only control a ship!
   if(object->getObjectTypeNumber() != PlayerShipTypeNumber)
      return NULL;

  return static_cast<Ship *>(object);
}


void ClientGame::changePlayerTeam(const StringTableEntry &playerName, S32 teamIndex) const
{
   if(!getGameType())
      return;

   getGameType()->c2sTriggerTeamChange(playerName, teamIndex);
}


void ClientGame::changeOwnTeam(S32 teamIndex) const
{
   if(!getGameType())
      return;

   getGameType()->c2sChangeTeams(teamIndex);
}


};

