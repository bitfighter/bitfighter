//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------


#include "UIManager.h"

#include "UI.h"
#include "UIGame.h"
#include "UIMenus.h"
#include "UINameEntry.h"
#include "UIMessage.h"
#include "UIQueryServers.h"
#include "UIEditor.h"            // For EditorUserInterface def
#include "UIInstructions.h"
#include "UIKeyDefMenu.h"
#include "UIDiagnostics.h"
#include "UIGameParameters.h"
#include "UICredits.h"
#include "UIEditorInstructions.h"
#include "UIErrorMessage.h"
#include "UIChat.h"
#include "UITeamDefMenu.h"
#include "UIGame.h"
#include "UIHighScores.h"
#include "ScreenInfo.h"
#include "ClientGame.h"

#include "stringUtils.h"
#include "RenderUtils.h"

#include "SoundSystem.h"

#if defined(TNL_OS_MOBILE) || defined(BF_USE_GLES)
#  include "SDL_opengles.h"
#else
#  include "SDL_opengl.h"
#endif


namespace Zap
{

const S32 UIManager::MessageBoxWrapWidth = 500;

// Constructor
UIManager::UIManager() 
{ 
   mGame = NULL;
   mSettings = NULL;

   mCurrentInterface = NULL;

   mLastUI = NULL;
   mLastWasLower = false;
   mUserHasSeenTimeoutMessage = false;

   mMenuTransitionTimer.reset(0);      // Set to 100 for a dizzying effect; doing so will cause editor to crash, so beware!
}


// Destructor
UIManager::~UIManager()
{
   // Clear out mUis

   for(UiIterator it = mUis.begin(); it != mUis.end(); it++) 
      delete it->second;

   mUis.clear();
}


void UIManager::setClientGame(ClientGame *clientGame)
{
   mGame     = clientGame;
   mSettings = clientGame->getSettings();
}


// Reactivate previous interface, going to fallback if there is none
void UIManager::reactivatePrevUI()
{
   if(mPrevUIs.size() > 0)
   {
      UserInterface *prev = mPrevUIs.last();
      mPrevUIs.pop_back();

      mLastUI = mCurrentInterface;
      mCurrentInterface = prev;
      mLastWasLower = true;
      
      mCurrentInterface->reactivate();
   }
   else
   {
      TNLAssert(false, "There has been a failure in previous UI queuing!");
      getUI<MainMenuUserInterface>()->reactivate();      // Fallback if everything else has failed
   }

   mMenuTransitionTimer.reset();
}


void UIManager::reactivate(const UserInterface *ui)
{
   // Keep discarding menus until we find the one we want
   while(mPrevUIs.size() && (mPrevUIs.last() != ui))
      mPrevUIs.pop_back();

   // Now that the next one is our target, when we reactivate, we'll be where we want to be
   reactivatePrevUI();
}


UserInterface *UIManager::getPrevUI()
{
   if(mPrevUIs.size() == 0)
      return NULL;

   return mPrevUIs.last();
}


UserInterface *UIManager::getCurrentUI()
{
   return mCurrentInterface;
}


#ifndef BF_TEST

bool UIManager::hasPrevUI()
{
   return mPrevUIs.size() > 0;
}


void UIManager::clearPrevUIs()
{
   mPrevUIs.clear();
}


// Have to pass ui to avoid stack overflow when trying to render UIs two-levels deep
void UIManager::renderPrevUI(const UserInterface *ui)
{
   // This will cause stack overflows, as current can be UI is last on stack!
   //if(mPrevUIs.size() > 0)
   //   mPrevUIs.last()->render();

   if(mCurrentInterface == ui)
   {
      mPrevUIs.last()->render();
      return;
   }

   for(S32 i = mPrevUIs.size() - 1; i > 0; i--)    // Not >= 0, because of the [i-1] below
      if(mPrevUIs[i] == ui)
      {
         mPrevUIs[i-1]->render();
         return;
      }
}


void UIManager::activate(UserInterface *ui, bool save)  // save defaults to true
{
   if(mCurrentInterface)
   {
      if(save)
         saveUI(mCurrentInterface);
   }

   mLastUI = mCurrentInterface;
   mLastWasLower = false;
   mCurrentInterface = ui;

   // Deactivate the UI we're no longer using
   if(mLastUI)
      mLastUI->onDeactivate(ui->usesEditorScreenMode());

   mCurrentInterface->activate();

   mMenuTransitionTimer.reset();
}


void UIManager::saveUI(UserInterface *ui)
{
   TNLAssert(!ui || mPrevUIs.size() == 0 || ui != mPrevUIs.last(), "Pushing duplicate UI onto mPrevUIs stack.  Intentional?");

   if(ui)
      mPrevUIs.push_back(ui);
}


// Game connection is terminated -- reactivate the appropriate UI
void UIManager::onConnectionTerminated(const Address &serverAddress, NetConnection::TerminationReason reason, const char *reasonStr)
{
   if(cameFrom<EditorUserInterface>())
     reactivate(getUI<EditorUserInterface>());
   else if(getPrevUI() != NULL)    // Avoids Assert "There has been a failure in previous UI queuing!" in tests
     reactivate(getUI<MainMenuUserInterface>());


   // Display a context-appropriate error message

   string message;

   switch(reason)
   {
      case NetConnection::ReasonSelfDisconnect:
         // We get this when we terminate our own connection.  Since this is intentional behavior,
         // we don't want to display any message to the user.
         return;

      case NetConnection::ReasonTimedOut:
         message = "Your connection timed out.  Please try again later.";
         break;

      case NetConnection::ReasonIdle:
         message = "The server kicked you because you were idle too long.\n\n"
                   "Feel free to rejoin the game when you are ready.";
         break;

      case NetConnection::ReasonPuzzle:
         message = "Unable to connect to the server.  Disconnected for this reason:\n\n"
                   "Invalid puzzle solution\n\n"
                   "Please try a different game server, or try again later.";
         break;

      case NetConnection::ReasonKickedByAdmin:
         message = "You were kicked off the server by an admin.\n\n"
                   "You can try another server, host your own, or try the server that kicked you again later.";

         activate<NameEntryUserInterface>();
         break;

      case NetConnection::ReasonBanned:
         message = "You are banned from playing on this server.\n\n"
                   "Contact the server administrator if you think this was in error.";
         break;

      case NetConnection::ReasonFloodControl:
         message = "Your connection was rejected by the server because you sent too many connection requests.\n\n"
                   "Please try a different game server, or try again later.";

         activate<NameEntryUserInterface>();
         break;

      case NetConnection::ReasonShutdown:
         message = "Remote server shut down.\n\n"
                   "Please try a different server, or host a game of your own!";
         break;

      case NetConnection::ReasonNeedServerPassword:
      {
         // We have the wrong password, let's make sure it's not saved
         string serverName = getLastSelectedServerName();
         GameSettings::deleteServerPassword(serverName);
   
         setConnectAddressAndActivatePasswordEntryUI(Address(serverAddress));
         return;
      }

      case NetConnection::ReasonServerFull:
         message = "Could not connect to server because server is full.\n\n"

                   "Please try a different server, or try again later.";
         break;

      default:
         if(reasonStr[0])
            message = "Disconnected for this reason:\n\n" + string(reasonStr);
         else
            message = "Disconnected for unknown reason:\n\nError number: " + itos(reason);
         break;
   }

   displayMessageBox("Connection Terminated", "", message);
}


void UIManager::onConnectedToMaster()
{
   mUserHasSeenTimeoutMessage = false;     // Reset display of connection error
}


void UIManager::onConnectionToMasterTerminated(NetConnection::TerminationReason reason, const char *reasonStr, bool wasFullyConnected)
{
   string message;

   switch(reason)
   {
      case NetConnection::ReasonDuplicateId:
         message =
            "Your connection was rejected by the server because you sent a duplicate player id. Player ids are "
            "generated randomly, and collisions are extremely rare.\n\n"

            "Please restart Bitfighter and try again.  Statistically speaking, you should never see this message again!";
         break;

      case NetConnection::ReasonBadLogin:
         message = 
            "Unable to log you in with the username/password you provided. If you have an account, please verify your "
            "password. Otherwise, you chose a reserved name; please try another.\n\n"

            "Please check your credentials and try again.";

         activate<NameEntryUserInterface>();
         break;

      case NetConnection::ReasonInvalidUsername:
         message = 
            "Your connection was rejected by the server because you sent a username that contained illegal characters.\n\n"
            "Please try a different name.";

         activate<NameEntryUserInterface>();
         break;

      case NetConnection::ReasonError:
         message = 
            "Unable to connect to the server.  Received message:\n\n" +
            string(reasonStr) + "\n\n"
            "Please try a different game server, or try again later.";
         break;

      case NetConnection::ReasonTimedOut:
         // Avoid spamming the player if they are not connected to the Internet
         if(mUserHasSeenTimeoutMessage)
            return;

         if(wasFullyConnected)
            return;

         message = 
               "My attempt to connect to the Master Server failed because the server did not respond.  Either the server is down, "
               "or, more likely, you are not connected to the Internet or your firewall is blocking the connection.\n\n"

               "I will continue trying, but you will not see this message again until you successfully "
               "connect or restart Bitfighter.";

         mUserHasSeenTimeoutMessage = true;
         break;

      case NetConnection::ReasonSelfDisconnect:
         // No errors when client disconnect (this happens when quitting bitfighter normally)
         return;

      case NetConnection::ReasonAnonymous:
         // Anonymous connections are disconnected quickly, usually after retrieving some data
         return;

      default:  // Not handled
         message = "Unable to connect to the master server, with error code:\n\n";

         if(reasonStr[0])
            message += itos(reason) + " " + reasonStr;
         else
            message += "MasterServer Error #" + itos(reason);

         message += "\n\nCheck your Internet Connection and firewall settings.\n\n"
                    "Please report this error code to the Bitfighter developers.";
         break;
   }

   displayMessageBox("Connection Terminated", "Press [[Esc]] to continue", message);
}


void UIManager::onConnectionToServerRejected(const char *reason)
{
   activate<MainMenuUserInterface>();

   string message;

   message = "Error when trying to punch through firewall.\n\n"
             "Server did not respond or rejected you.\n\n"
             "Unable to join game.  Please try a different server.";

   if(reason[0])
      message += "\n\n" + string(reason);
   
   displayMessageBox("Connection Terminated", "", message);
}


void UIManager::onPlayerJoined(const char *playerName, bool isLocalClient, bool playAlert, bool showMessage)
{
   if(showMessage)
   {
      if(isLocalClient)    
         displayMessage(Color(0.6f, 0.6f, 0.8f), "Welcome to the game!");            // SysMsg
      else   
         displayMessage(Color(0.6f, 0.6f, 0.8f), "%s joined the game.", playerName); // SysMsg
   }

   if(playAlert)
      playSoundEffect(SFXPlayerJoined, 1);

   getUI<GameUserInterface>()->onPlayerJoined();      // Notifies the helpers
}


// Another player has just left the game
void UIManager::onPlayerQuit(const char *name)
{
   displayMessage(Color(0.6f, 0.6f, 0.8f), "%s left the game.", name);     // SysMsg
   playSoundEffect(SFXPlayerLeft, 1);
}


void UIManager::onGameStarting()
{
   getUI<GameUserInterface>()->onGameStarting();
}


void UIManager::onGameOver()
{
   if(mUis[getTypeInfo<GameUserInterface>()])
      getUI<GameUserInterface>()->onGameOver();    // Closes helpers and such
}


void UIManager::displayMessage(const Color &msgColor, const char *format, ...)
{
   va_list args;
   char message[MAX_CHAT_MSG_LENGTH]; 

   va_start(args, format);
   vsnprintf(message, sizeof(message), format, args); 
   va_end(args);
    
   getUI<GameUserInterface>()->displayMessage(msgColor, message);
}


void UIManager::renderCurrent()
{
   // The viewport has been setup by the caller so, regardless of how many clients we're running, we can just render away here.
   // Each viewport should have an aspect ratio of 800x600.

   if(mMenuTransitionTimer.getCurrent() && mLastUI)
   {
      // Save viewport
      GLint viewport[4];
      glGetIntegerv(GL_VIEWPORT, viewport);    

      glViewport(viewport[0] + GLint((mLastWasLower ? 1 : -1) * viewport[2] * (1 - mMenuTransitionTimer.getFraction())), 0, viewport[2], viewport[3]);
      mLastUI->render();

      glViewport(viewport[0] - GLint((mLastWasLower ? 1 : -1) * viewport[2] * mMenuTransitionTimer.getFraction()), 0, viewport[2], viewport[3]);
      mCurrentInterface->render();

      // Restore viewport for posterity
      glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

      return;
   }

   TNLAssert(mCurrentInterface, "NULL mCurrentInterface");

   // Run the active UI renderer
   mCurrentInterface->render();
   UserInterface::renderDiagnosticKeysOverlay();    // By putting this here, it will always get rendered, regardless of active UI
   mCurrentInterface->renderMasterStatus();
}


void UIManager::idle(U32 timeDelta)
{
   mMenuTransitionTimer.update(timeDelta);
   processAudio(timeDelta);

   // Idle the currently active UI
   if(getCurrentUI())
      getCurrentUI()->idle(timeDelta);

   // If we're in a UI and GameUI is still running, we need to idle that too
   if(cameFrom<GameUserInterface>())
      getUI<GameUserInterface>()->idle(timeDelta);
}


// Select music based on where we are
MusicLocation UIManager::selectMusic()
{
   // In game (or one of its submenus)...
   if(isCurrentUI<GameUserInterface>() || cameFrom<GameUserInterface>())
      return MusicLocationGame;

   // In editor...
   if(isCurrentUI<EditorUserInterface>() || cameFrom<EditorUserInterface>())
      return MusicLocationEditor;

   // In credits...
   if(isCurrentUI<CreditsUserInterface>() || cameFrom<CreditsUserInterface>())
      return MusicLocationCredits;

   // Otherwise, we must be in the menus...
   return MusicLocationMenus;
}


void UIManager::processAudio(U32 timeDelta)
{
   SoundSystem::processAudio(timeDelta, 
                             mSettings->getIniSettings()->sfxVolLevel,
                             mSettings->getIniSettings()->getMusicVolLevel(),
                             mSettings->getIniSettings()->voiceChatVolLevel,
                             selectMusic());  
}


SFXHandle UIManager::playSoundEffect(U32 profileIndex, F32 gain) const
{
   return SoundSystem::playSoundEffect(profileIndex, gain);
}


SFXHandle UIManager::playSoundEffect(U32 profileIndex, const Point &position) const
{
   return SoundSystem::playSoundEffect(profileIndex, position);
}


SFXHandle UIManager::playSoundEffect(U32 profileIndex, const Point &position, const Point &velocity, F32 gain) const
{
   return SoundSystem::playSoundEffect(profileIndex, position, velocity, gain);
}


void UIManager::setMovementParams(SFXHandle &effect, const Point &position, const Point &velocity) const
{
   SoundSystem::setMovementParams(effect, position, velocity);
}


void UIManager::stopSoundEffect(SFXHandle &effect) const
{
   SoundSystem::stopSoundEffect(effect);
}


void UIManager::setListenerParams(const Point &position, const Point &velocity) const
{
   SoundSystem::setListenerParams(position, velocity);
}


void UIManager::playNextTrack() const
{
   SoundSystem::playNextTrack();
}


void UIManager::playPrevTrack() const
{
   SoundSystem::playPrevTrack();
}


void UIManager::queueVoiceChatBuffer(const SFXHandle &effect, const ByteBufferPtr &p) const
{
   SoundSystem::queueVoiceChatBuffer(effect, p);
}


void UIManager::renderAndDimGameUserInterface()
{
   getUI<GameUserInterface>()->render();
   UserInterface::dimUnderlyingUI();
}


void UIManager::setHighScores(const Vector<StringTableEntry> &groupNames, const Vector<string> &names, const Vector<string> &scores)
{
   getUI<HighScoresUserInterface>()->setHighScores(groupNames, names, scores);
}


// Message relayed through master -- global chat system
void UIManager::gotGlobalChatMessage(const string &from, const string &message, bool isPrivate, bool isSystem, bool fromSelf)
{
   getUI<ChatUserInterface>()->newMessage(from, message, isPrivate, isSystem, fromSelf);
}


void UIManager::gotServerListFromMaster(const Vector<IPAddress> &serverList)
{
   getUI<QueryServersUserInterface>()->gotServerListFromMaster(serverList);
}


void UIManager::setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks)
{
   getUI<ChatUserInterface>()->setPlayersInGlobalChat(playerNicks);
}


void UIManager::playerJoinedGlobalChat(const StringTableEntry &playerNick)
{
   getUI<ChatUserInterface>()->playerJoinedGlobalChat(playerNick);
}


void UIManager::playerLeftGlobalChat(const StringTableEntry &playerNick)
{
   getUI<ChatUserInterface>()->playerLeftGlobalChat(playerNick);
}


void UIManager::gotPingResponse(const Address &address, const Nonce &nonce, U32 clientIdentityToken)
{
   getUI<QueryServersUserInterface>()->gotPingResponse(address, nonce, clientIdentityToken);
}


void UIManager::gotQueryResponse(const Address &address, const Nonce &nonce, const char *serverName, const char *serverDescr, 
                                 U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired)
{
   getUI<QueryServersUserInterface>()->gotQueryResponse(address, nonce, serverName, serverDescr, playerCount, 
                                                        maxPlayers, botCount, dedicated, test, passwordRequired);
}


string UIManager::getLastSelectedServerName()
{
   return getUI<QueryServersUserInterface>()->getLastSelectedServerName();
}


void UIManager::setConnectAddressAndActivatePasswordEntryUI(const Address &serverAddress)
{
   ServerPasswordEntryUserInterface *ui = getUI<ServerPasswordEntryUserInterface>();
   ui->setAddressToConnectTo(serverAddress);

   activate(ui);
}


void UIManager::enableLevelLoadDisplay()
{
   getUI<GameUserInterface>()->showLevelLoadDisplay(true, false);
}


void UIManager::serverLoadedLevel(const string &levelName)
{
   getUI<GameUserInterface>()->serverLoadedLevel(levelName);
}


void UIManager::disableLevelLoadDisplay(bool fade)
{
   if(mUis[getTypeInfo<GameUserInterface>()])
      getUI<GameUserInterface>()->showLevelLoadDisplay(false, fade);
}


// We need this to hide the GameUserInterface class from ClientGame so tests will compile nicely
void UIManager::activateGameUserInterface()
{
   activate<GameUserInterface>();
}


void UIManager::renderLevelListDisplayer()
{
   getUI<GameUserInterface>()->renderLevelListDisplayer();
}


void UIManager::setMOTD(const char *motd)
{
   getUI<MainMenuUserInterface>()->setMOTD(motd); 
}


void UIManager::setNeedToUpgrade(bool needToUpgrade)
{
   getUI<MainMenuUserInterface>()->setNeedToUpgrade(needToUpgrade);
}


void UIManager::gotPasswordOrPermissionsReply(const ClientGame *game, const char *message)
{
   // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
   if(isCurrentUI<GameMenuUserInterface>())
      getUI<GameMenuUserInterface>()->mMenuSubTitle = message;
   else
      game->displayCmdChatMessage(message);     
}


void UIManager::showPlayerActionMenu(PlayerAction action)
{
   PlayerMenuUserInterface *ui = getUI<PlayerMenuUserInterface>();
   ui->action = action;

   activate(ui);
};


void UIManager::showMenuToChangeTeamForPlayer(const string &playerName)
{
   TeamMenuUserInterface *ui = getUI<TeamMenuUserInterface>();
   ui->nameToChange = playerName;
   activate(ui);                  
}


void UIManager::activateGameUI()
{
   activate(getUI<GameUserInterface>());
}


void UIManager::reactivateGameUI()
{
   reactivate(getUI<GameUserInterface>());
}


void UIManager::displayMessageBox(const StringTableEntry &title, const StringTableEntry &instr, 
                                  const Vector<StringTableEntry> &messages)
{
   string msg = "";
   for(S32 i = 0; i < messages.size(); i++)
      msg += string(messages[i].getString()) + "\n";

   displayMessageBox(title.getString(), instr.getString(), msg);
}


void UIManager::displayMessageBox(const string &title, const string &instr, const Vector<string> &messages)
{
   string msg = "";

   for(S32 i = 0; i < messages.size(); i++)
      msg += messages[i] + "\n";   
   
   displayMessageBox(title, instr, msg);
}


void UIManager::displayMessageBox(const string &title, const string &instr, const string &message)
{
   ErrorMessageUserInterface *ui = getUI<ErrorMessageUserInterface>();

   ui->reset();
   ui->setTitle(title);
   ui->setInstr(instr);
   ui->setMessage(message);

   activate(ui);
}


void UIManager::startLoadingLevel(bool engineerEnabled)
{
   clearSparks();
   getUI<EditorUserInterface>()->clearRobotLines();
   getUI<GameUserInterface>()->startLoadingLevel(engineerEnabled);
}


void UIManager::readRobotLine(const string &robotLine)
{
   getUI<EditorUserInterface>()->addRobotLine(robotLine);
}


void UIManager::doneLoadingLevel()
{
   getUI<GameUserInterface>()->doneLoadingLevel();
}


void UIManager::clearSparks()
{
   getUI<GameUserInterface>()->clearSparks();
}


void UIManager::emitBlast(const Point &pos, U32 size)
{
   getUI<GameUserInterface>()->emitBlast(pos, size);
}


void UIManager::emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2)
{
   getUI<GameUserInterface>()->emitBurst(pos, scale, color1, color2);
}


void UIManager::emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation)
{
   getUI<GameUserInterface>()->emitDebrisChunk(points, color, pos, vel, ttl, angle, rotation);
}


void UIManager::emitTextEffect(const string &text, const Color &color, const Point &pos)
{
   getUI<GameUserInterface>()->emitTextEffect(text, color, pos);
}


void UIManager::emitSpark(const Point &pos, const Point &vel, const Color &color, S32 ttl, UI::SparkType sparkType)
{
   getUI<GameUserInterface>()->emitSpark(pos, vel, color, ttl, sparkType);
}


void UIManager::emitExplosion(const Point &pos, F32 size, const Color *colorArray, U32 numColors)
{
   getUI<GameUserInterface>()->emitExplosion(pos, size, colorArray, numColors);
}


void UIManager::emitTeleportInEffect(const Point &pos, U32 type)
{
   getUI<GameUserInterface>()->emitTeleportInEffect(pos, type);
}


void UIManager::addInlineHelpItem(HelpItem item)
{
   getUI<GameUserInterface>()->addInlineHelpItem(item);
}


void UIManager::addInlineHelpItem(U8 objectType, S32 objectTeam, S32 playerTeam)
{
   getUI<GameUserInterface>()->addInlineHelpItem(objectType, objectTeam, playerTeam);
}


void UIManager::removeInlineHelpItem(HelpItem item, bool markAsSeen)
{
   getUI<GameUserInterface>()->removeInlineHelpItem(item, markAsSeen);
}


F32 UIManager::getObjectiveArrowHighlightAlpha()
{
   return getUI<GameUserInterface>()->getObjectiveArrowHighlightAlpha();
}


bool UIManager::isShowingInGameHelp()
{
   return getUI<GameUserInterface>()->isShowingInGameHelp();
}


void UIManager::onChatMessageReceived(const Color &msgColor, const char *format, ...)
{
   static char buffer[MAX_CHAT_MSG_LENGTH];

   va_list args;

   va_start(args, format);
   vsnprintf(buffer, sizeof(buffer), format, args);
   va_end(args);

   getUI<GameUserInterface>()->onChatMessageReceived(msgColor, "%s", buffer);
}


void UIManager::gotAnnouncement(const string &announcement)
{
   getUI<GameUserInterface>()->setAnnouncement(announcement);
}


bool UIManager::isInScoreboardMode()
{
   return getUI<GameUserInterface>()->isInScoreboardMode();
}


// Called by Ship::unpack() -- loadouts are transmitted via the ship object
// Data flow: Ship->ClientGame->UIManager->GameUserInterface->LoadoutIndicator
void UIManager::newLoadoutHasArrived(const LoadoutTracker &loadout)
{
   getUI<GameUserInterface>()->newLoadoutHasArrived(loadout);
}


void UIManager::setActiveWeapon(U32 weaponIndex)
{
   getUI<GameUserInterface>()->setActiveWeapon(weaponIndex);
}


bool UIManager::isShowingDebugShipCoords()
{
   return getUI<GameUserInterface>()->isShowingDebugShipCoords();
}


// Called from renderObjectiveArrow() & ship's onMouseMoved() when in commander's map
Point UIManager::worldToScreenPoint(const Point *point, S32 canvasWidth, S32 canvasHeight)
{
   return getUI<GameUserInterface>()->worldToScreenPoint(point, canvasWidth, canvasHeight);
}


// Only called my gameConnection when connection to game server is established
void UIManager::resetCommandersMap()
{
   getUI<GameUserInterface>()->resetCommandersMap();
}


F32 UIManager::getCommanderZoomFraction()
{
   return getUI<GameUserInterface>()->getCommanderZoomFraction();
}


void UIManager::renderBasicInterfaceOverlay()
{
   getUI<GameUserInterface>()->renderBasicInterfaceOverlay();
}


void UIManager::quitEngineerHelper()
{
   getUI<GameUserInterface>()->quitEngineerHelper();
}


void UIManager::exitHelper()
{
   getUI<GameUserInterface>()->exitHelper();
}


void UIManager::shutdownInitiated(U16 time, const StringTableEntry &name, const StringPtr &reason, bool originator)
{
   getUI<GameUserInterface>()->shutdownInitiated(time, name, reason, originator);
}


void UIManager::cancelShutdown() 
{ 
   getUI<GameUserInterface>()->cancelShutdown(); 
}


void UIManager::setSelectedEngineeredObject(U32 objectType)
{
   getUI<GameUserInterface>()->setSelectedEngineeredObject(objectType);
}


Move *UIManager::getCurrentMove()
{
   return getUI<GameUserInterface>()->getCurrentMove();
}


void UIManager::displayErrorMessage(const char *message)
{
   getUI<GameUserInterface>()->displayErrorMessage(message);
}


void UIManager::displaySuccessMessage(const char *message)
{
   getUI<GameUserInterface>()->displaySuccessMessage(message);
}


void UIManager::setShowingInGameHelp(bool showing)
{
   if(mUis[getTypeInfo<GameUserInterface>()])
      getUI<GameUserInterface>()->setShowingInGameHelp(showing);
}


void UIManager::resetInGameHelpMessages()
{
   getUI<GameUserInterface>()->resetInGameHelpMessages();
}

#else

// Constructor
bool UIManager::hasPrevUI() { return true; }
void UIManager::clearPrevUIs() { }
void UIManager::renderPrevUI(const UserInterface *ui) { }
void UIManager::activate(UserInterface *ui, bool save)  { }
void UIManager::saveUI(UserInterface *ui) { }
void UIManager::onConnectionTerminated(const Address &serverAddress, NetConnection::TerminationReason reason, const char *reasonStr) { }
void UIManager::onConnectedToMaster() { }
void UIManager::onConnectionToMasterTerminated(NetConnection::TerminationReason reason, const char *reasonStr, bool wasFullyConnected) { }
void UIManager::onConnectionToServerRejected(const char *reason) { }
void UIManager::onPlayerJoined(const char *playerName, bool isLocalClient, bool playAlert, bool showMessage) { }
void UIManager::onPlayerQuit(const char *name)  { }
void UIManager::onGameStarting() { }
void UIManager::onGameOver() { }
void UIManager::displayMessage(const Color &msgColor, const char *format, ...) { }
void UIManager::renderCurrent() { }
void UIManager::idle(U32 timeDelta) { }
MusicLocation UIManager::selectMusic() { return MusicLocationNone; }
void UIManager::processAudio(U32 timeDelta) { }
SFXHandle UIManager::playSoundEffect(U32 profileIndex, F32 gain) const { return NULL; }
SFXHandle UIManager::playSoundEffect(U32 profileIndex, const Point &position) const { return NULL; }
SFXHandle UIManager::playSoundEffect(U32 profileIndex, const Point &position, const Point &velocity, F32 gain) const { return NULL; }
void UIManager::setMovementParams(SFXHandle &effect, const Point &position, const Point &velocity) const { }
void UIManager::stopSoundEffect(SFXHandle &effect) const { }
void UIManager::setListenerParams(const Point &position, const Point &velocity) const { }
void UIManager::playNextTrack() const { }
void UIManager::playPrevTrack() const { }
void UIManager::queueVoiceChatBuffer(const SFXHandle &effect, const ByteBufferPtr &p) const { }
void UIManager::renderAndDimGameUserInterface() { }
void UIManager::setHighScores(const Vector<StringTableEntry> &groupNames, const Vector<string> &names, const Vector<string> &scores) { }
void UIManager::gotGlobalChatMessage(const string &from, const string &message, bool isPrivate, bool isSystem, bool fromSelf) { }
void UIManager::gotServerListFromMaster(const Vector<IPAddress> &serverList) { }
void UIManager::setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks) { }
void UIManager::playerJoinedGlobalChat(const StringTableEntry &playerNick) { }
void UIManager::playerLeftGlobalChat(const StringTableEntry &playerNick) { }
void UIManager::gotPingResponse(const Address &address, const Nonce &nonce, U32 clientIdentityToken) { }
void UIManager::gotQueryResponse(const Address &address, const Nonce &nonce, const char *serverName, const char *serverDescr, 
                                 U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired) { }
string UIManager::getLastSelectedServerName() { return ""; }
void UIManager::setConnectAddressAndActivatePasswordEntryUI(const Address &serverAddress) { }
void UIManager::enableLevelLoadDisplay() { }
void UIManager::serverLoadedLevel(const string &levelName) { }
void UIManager::disableLevelLoadDisplay(bool fade) { }
void UIManager::activateGameUserInterface() { }
void UIManager::renderLevelListDisplayer() { }
void UIManager::setMOTD(const char *motd) { }
void UIManager::setNeedToUpgrade(bool needToUpgrade) { }
void UIManager::gotPasswordOrPermissionsReply(const ClientGame *game, const char *message) { }
void UIManager::showPlayerActionMenu(PlayerAction action) { }
void UIManager::showMenuToChangeTeamForPlayer(const string &playerName) { }
void UIManager::activateGameUI() { }
void UIManager::reactivateGameUI() { }
void UIManager::displayMessageBox(const StringTableEntry &title, const StringTableEntry &instr, 
                                  const Vector<StringTableEntry> &messages) { }
void UIManager::displayMessageBox(const char *title, const char *instr, const Vector<string> &messages) { }
void UIManager::startLoadingLevel(bool engineerEnabled) { }
void UIManager::readRobotLine(const string &robotLine) { }
void UIManager::doneLoadingLevel() { }
void UIManager::clearSparks() { }
void UIManager::emitBlast(const Point &pos, U32 size) { }
void UIManager::emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2) { }
void UIManager::emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation) { }
void UIManager::emitTextEffect(const string &text, const Color &color, const Point &pos) { }
void UIManager::emitSpark(const Point &pos, const Point &vel, const Color &color, S32 ttl, UI::SparkType sparkType) { }
void UIManager::emitExplosion(const Point &pos, F32 size, const Color *colorArray, U32 numColors) { }
void UIManager::emitTeleportInEffect(const Point &pos, U32 type) { }
void UIManager::addInlineHelpItem(HelpItem item) { }
void UIManager::addInlineHelpItem(U8 objectType, S32 objectTeam, S32 playerTeam) { }
void UIManager::removeInlineHelpItem(HelpItem item, bool markAsSeen) { }
F32 UIManager::getObjectiveArrowHighlightAlpha() { return 0; }
bool UIManager::isShowingInGameHelp() { return false; }
void UIManager::onChatMessageReceived(const Color &msgColor, const char *format, ...) { }
void UIManager::gotAnnouncement(const string &announcement) { }
bool UIManager::isInScoreboardMode() { return false;}
void UIManager::newLoadoutHasArrived(const LoadoutTracker &loadout) { }
void UIManager::setActiveWeapon(U32 weaponIndex) { }
bool UIManager::isShowingDebugShipCoords() { return false; }
Point UIManager::worldToScreenPoint(const Point *point, S32 canvasWidth, S32 canvasHeight) { return Point(); }
void UIManager::resetCommandersMap() { }
F32 UIManager::getCommanderZoomFraction() { return 0; }
void UIManager::renderBasicInterfaceOverlay() { }
void UIManager::quitEngineerHelper() { }
void UIManager::exitHelper() { }
void UIManager::shutdownInitiated(U16 time, const StringTableEntry &name, const StringPtr &reason, bool originator) { }
void UIManager::cancelShutdown()  { }
void UIManager::setSelectedEngineeredObject(U32 objectType) { }
Move *UIManager::getCurrentMove() { return NULL; }      
void UIManager::displayErrorMessage(const char *message) { }
void UIManager::displaySuccessMessage(const char *message) { }
void UIManager::setShowingInGameHelp(bool showing) { }
void UIManager::resetInGameHelpMessages() { }

#endif


};
