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
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

// Use this for testing the scoreboard
//#define USE_DUMMY_PLAYER_SCORES

#include "UIGame.h"

#include "ChatHelper.h"
#include "quickChatHelper.h"
#include "loadoutHelper.h"
#include "engineerHelper.h"
#include "TeamShuffleHelper.h"

#include "gameConnection.h"
#include "UIMenus.h"
#include "UIInstructions.h"
#include "UIChat.h"
#include "UIMessage.h"
#include "UIDiagnostics.h"
#include "UIErrorMessage.h"
#include "EventManager.h"
#include "gameType.h"
#include "IniFile.h"             // For access to gINI functions
#include "EngineeredItem.h"      // For EngineerModuleDeployer
#include "shipItems.h"           // For EngineerBuildObjects
#include "gameObjectRender.h"
#include "input.h"
#include "loadoutHelper.h"
#include "gameNetInterface.h"
#include "SoundSystem.h"
#include "md5wrapper.h"          // For submission of passwords
#include "Console.h"             // Our console object
#include "config.h"              // for Getmap level dir
#include "ScreenInfo.h"
#include "ClientGame.h"
#include "ClientInfo.h"
#include "Colors.h"
#include "Cursor.h"
#include "CoreGame.h"
#include "ScissorsManager.h"

#include "tnlEndian.h"

#include "SDL.h"
#include "OpenglUtils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

namespace Zap
{

// Sizes and other things to help with positioning
static const S32 SRV_MSG_FONT_SIZE = 14;
static const S32 SRV_MSG_FONT_GAP = 4;
static const S32 CHAT_FONT_SIZE = 12;
static const S32 CHAT_FONT_GAP = 3;
static const S32 CHAT_WRAP_WIDTH = 700;            // Max width of chat messages displayed in-game
static const S32 SRV_MSG_WRAP_WIDTH = 750;


// Constructor
GameUserInterface::GameUserInterface(ClientGame *game) : 
                  Parent(game), 
                  mVoiceRecorder(game),  //   lines expr  topdown   wrap width          font size          line gap
                  mServerMessageDisplayer(game,  6, true,  true,  SRV_MSG_WRAP_WIDTH, SRV_MSG_FONT_SIZE, SRV_MSG_FONT_GAP),
                  mChatMessageDisplayer1 (game,  5, true,  false, CHAT_WRAP_WIDTH,    CHAT_FONT_SIZE,    CHAT_FONT_GAP),
                  mChatMessageDisplayer2 (game,  5, false, false, CHAT_WRAP_WIDTH,    CHAT_FONT_SIZE,    CHAT_FONT_GAP),
                  mChatMessageDisplayer3 (game, 24, false, false, CHAT_WRAP_WIDTH,    CHAT_FONT_SIZE,    CHAT_FONT_GAP),
                  mFpsRenderer(game)
{
   mInScoreboardMode = false;
   displayInputModeChangeAlert = false;
   mMissionOverlayActive = false;

   mHelperManager.initialize(game);

   mMessageDisplayMode = ShortTimeout;

   setMenuID(GameUI);
   mInScoreboardMode = false;

   mGotControlUpdate = false;
   
   mFiring = false;
   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
   {
      mModPrimaryActivated[i] = false;
      mModSecondaryActivated[i] = false;
      mModuleDoubleTapTimer[i].setPeriod(DoubleClickTimeout);
   }
   
   mAnnouncementTimer.setPeriod(15000);  // 15 seconds
   mAnnouncement = "";
}


// Destructor  -- only runs when we're exiting to the OS
GameUserInterface::~GameUserInterface()
{
   // Do nothing
}


void GameUserInterface::onPlayerJoined()     { mHelperManager.onPlayerJoined();     }
void GameUserInterface::onPlayerQuit()       { mHelperManager.onPlayerQuit();       }
void GameUserInterface::onGameOver()         { mHelperManager.onGameOver();         }
void GameUserInterface::quitEngineerHelper() { mHelperManager.quitEngineerHelper(); }  // When ship dies engineering


void GameUserInterface::setAnnouncement(string message)
{
	mAnnouncement = message;

	mAnnouncementTimer.reset();
}


void GameUserInterface::onActivate()
{
   mDisableShipKeyboardInput = false;  // Make sure our ship controls are active
   mMissionOverlayActive = false;      // Turn off the mission overlay (if it was on)
   Cursor::disableCursor();            // Turn off cursor
   onMouseMoved();                     // Make sure ship pointed is towards mouse


   // Clear out any lingering server or chat messages
   mServerMessageDisplayer.reset();
   mChatMessageDisplayer1.reset();
   mChatMessageDisplayer2.reset();
   mChatMessageDisplayer3.reset();

   mHelperManager.reset();

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      mModPrimaryActivated[i] = false;
      mModSecondaryActivated[i] = false;
   }

   mShutdownMode = None;
}


void GameUserInterface::onReactivate()
{
   getGame()->undelaySpawn();

   mDisableShipKeyboardInput = false;
   Cursor::disableCursor();    // Turn off cursor

   if(!isChatting())
      getGame()->setBusyChatting(false);

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      mModPrimaryActivated[i] = false;
      mModSecondaryActivated[i] = false;
   }

   onMouseMoved();   // Call onMouseMoved to get ship pointed at current cursor location
}


bool GameUserInterface::isShowingMissionOverlay() const
{
   return mMissionOverlayActive;
}


static char stringBuffer[256];

void GameUserInterface::displayErrorMessage(const char *format, ...)
{
   va_list args;

   va_start(args, format);
   vsnprintf(stringBuffer, sizeof(stringBuffer), format, args);
   va_end(args);

   displayMessage(Colors::cmdChatColor, stringBuffer);
}


void GameUserInterface::displaySuccessMessage(const char *format, ...)
{
   va_list args;

   va_start(args, format);
   vsnprintf(stringBuffer, sizeof(stringBuffer), format, args);
   va_end(args);

   displayMessage(Color(0.6, 1, 0.8), stringBuffer);
}


void GameUserInterface::displayMessage(const Color &msgColor, const char *message)
{
   // Ignore empty message
   if(strcmp(message, "") == 0)
      return;

   mServerMessageDisplayer.onChatMessageReceived(msgColor, message);
}


void GameUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   // Update some timers
   mShutdownTimer.update(timeDelta);
   mInputModeChangeAlertDisplayTimer.update(timeDelta);
   mWrongModeMsgDisplay.update(timeDelta);
   mProgressBarFadeTimer.update(timeDelta);

   mLevelInfoDisplayer.idle(timeDelta);

   if(mAnnouncementTimer.update(timeDelta))
      mAnnouncement = "";

   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
      mModuleDoubleTapTimer[i].update(timeDelta);

   // Messages
   mServerMessageDisplayer.idle(timeDelta);
   mChatMessageDisplayer1.idle(timeDelta);
   mChatMessageDisplayer2.idle(timeDelta);
   mChatMessageDisplayer3.idle(timeDelta);

   mFpsRenderer.idle(timeDelta);
   
   mHelperManager.idle(timeDelta);
   mVoiceRecorder.idle(timeDelta);

   // Should we move this timer over to UIGame??
   HostMenuUserInterface *ui = getUIManager()->getHostMenuUserInterface();
   if(ui->levelLoadDisplayFadeTimer.update(timeDelta))
      ui->clearLevelLoadDisplay();

   mLoadoutIndicator.idle(timeDelta);
}


void GameUserInterface::resetInputModeChangeAlertDisplayTimer(U32 timeInMs)
{
   mInputModeChangeAlertDisplayTimer.reset(timeInMs);
}


#ifdef TNL_OS_WIN32
extern void checkMousePos(S32 maxdx, S32 maxdy);
#endif


// Draw main game screen (client only)
void GameUserInterface::render()
{
   if(!getGame()->isConnectedToServer())
   {
      glColor(Colors::white);
      drawCenteredString(260, 30, "Connecting to server...");

      glColor(Colors::green);
      if(getGame()->getConnectionToServer())
         drawCenteredString(310, 16, GameConnection::getConnectionStateString(getGame()->getConnectionToServer()->getConnectionState()));

      glColor(Colors::white);
      drawCenteredString(346, 20, "Press <ESC> to abort");
   }

   getGame()->render();

   if(getGame()->isSpawnDelayed())
      renderSuspendedMessage();

   {
      renderReticle();                       // Draw crosshairs if using mouse
      renderChatMsgs();                      // Render incoming chat and server msgs
      mLoadoutIndicator.render(getGame());   // Draw indicators for the various loadout items

      getUIManager()->getHostMenuUserInterface()->renderProgressListItems();  // This is the list of levels loaded while hosting

      renderProgressBar();          // This is the status bar that shows progress of loading this level

      mVoiceRecorder.render();      // This is the indicator that someone is sending a voice msg

      mFpsRenderer.render();        // Display running average FPS

      mHelperManager.render();

      GameType *gameType = getGame()->getGameType();

      if(gameType)
         gameType->renderInterfaceOverlay(mInScoreboardMode);

      renderLostConnectionMessage();      // Renders message overlay if we're losing our connection to the server
   }

   renderShutdownMessage();

   renderConsole();  // Rendered last, so it's always on top

#if 0
// Some code for outputting the position of the ship for finding good spawns
GameConnection *con = getGame()->getConnectionToServer();

if(con)
{
   BfObject *co = con->getControlObject();
   if(co)
   {
      Point pos = co->getActualPos() * F32(1 / 300.0f);
      drawStringf(10, 550, 30, "%0.2g, %0.2g", pos.x, pos.y);
   }
}

if(mGotControlUpdate)
   drawString(710, 10, 30, "CU");
#endif
}


// Returns true if player is composing a chat message
bool GameUserInterface::isChatting()
{
   return mHelperManager.isHelperActive(HelperMenu::ChatHelperType);
}


void GameUserInterface::renderSuspendedMessage()
{
   static string waitMsg[] = { "", 
                               "WILL RESPAWN",
                               "IN BLAH BLAH SECONDS",
                               "",
                               "" };

   static string readyMsg[] = { "", 
                                "PRESS ANY",
                                "KEY TO",
                                "RESPAWN",
                                "" };

   static const S32 DisplayStyle = 2;
   static const S32 VertOffset = -30;

   if(getGame()->getReturnToGameDelay() != 0)
   {
      waitMsg[2] = "IN " + ftos(ceil(F32(getGame()->getReturnToGameDelay()) / 1000.0f)) + " SECONDS";
      renderMessageBox("", "", waitMsg,  ARRAYSIZE(waitMsg),  VertOffset, DisplayStyle);
   }
   else
      renderMessageBox("", "", readyMsg, ARRAYSIZE(readyMsg), VertOffset, DisplayStyle);
}


void GameUserInterface::renderLostConnectionMessage()
{
   GameConnection *connection = getGame()->getConnectionToServer();

   if(connection && connection->lostContact())
   {
      static string msg[] = { "", 
                              "We may have lost contact with the server...", 
                              "",
                              " You can't play until the connection has been re-established ", 
                              "",
                              "Trying to reconnect now..."};

      renderMessageBox("SERVER CONNECTION PROBLEMS", "", msg, 5, -30);
   }
}


void GameUserInterface::renderShutdownMessage()
{
   if(mShutdownMode == None)
      return;

   else if(mShutdownMode == ShuttingDown)
   {
      char timemsg[255];
      dSprintf(timemsg, sizeof(timemsg), "Server is shutting down in %d seconds.", (S32) (mShutdownTimer.getCurrent() / 1000));

      if(mShutdownInitiator)     // Local client intitiated the shutdown
      {
         string msg[] = { "", timemsg, "", "Shutdown sequence intitated by you.", "", mShutdownReason.getString(), "" };
         renderMessageBox("SERVER SHUTDOWN INITIATED", "Press <ESC> to cancel shutdown", msg, 7);
      }
      else                       // Remote user intiated the shutdown
      {
         char whomsg[255];
         dSprintf(whomsg, sizeof(whomsg), "Shutdown sequence initiated by %s.", mShutdownName.getString());

         string msg[] = { "", timemsg, "", whomsg, "", mShutdownReason.getString(), "" };
         renderMessageBox("SHUTDOWN INITIATED", "Press <ESC> to dismiss", msg, 7);
      }
   }
   else if(mShutdownMode == Canceled)
   {
      // Keep same number of messages as above, so if message changes, it will be a smooth transition
      string msg[] = { "", "", "Server shutdown sequence canceled.", "", "Play on!", "", "" };     

      renderMessageBox("SHUTDOWN CANCELED", "Press <ESC> to dismiss", msg, 7);
   }
}


void GameUserInterface::shutdownInitiated(U16 time, const StringTableEntry &who, const StringPtr &why, bool initiator)
{
   mShutdownMode = ShuttingDown;
   mShutdownName = who;
   mShutdownReason = why;
   mShutdownInitiator = initiator;
   mShutdownTimer.reset(time * 1000);
}


void GameUserInterface::cancelShutdown()
{
   mShutdownMode = Canceled;
}


// Draws level-load progress bar across the bottom of the screen
void GameUserInterface::renderProgressBar()
{
   GameType *gt = getGame()->getGameType();
   if((mShowProgressBar || mProgressBarFadeTimer.getCurrent() > 0) && gt && gt->mObjectsExpected > 0)
   {
      TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

      glColor(Colors::green, mShowProgressBar ? 1 : mProgressBarFadeTimer.getFraction());

      // Outline
      const S32 left = 200;
      const S32 width = gScreenInfo.getGameCanvasWidth() - 2 * left;
      const S32 height = 10;

      // For some reason, there are occasions where the status bar doesn't progress all the way over during the load process.
      // The problem is that, for some reason, some objects do not add themselves to the loaded object counter, and this creates
      // a disconcerting effect, as if the level did not fully load.  Rather than waste any more time on this problem, we'll just
      // fill in the status bar while it's fading, to make it look like the level fully loaded.  Since the only thing that this
      // whole mechanism is used for is to display something to the user, this should work fine.
      S32 barWidth = mShowProgressBar ? S32((F32) width * (F32) getGame()->mObjectsLoaded / (F32) gt->mObjectsExpected) : width;

      for(S32 i = 1; i >= 0; i--)
      {
         S32 w = i ? width : barWidth;

         F32 vertices[] = {
               left,     gScreenInfo.getGameCanvasHeight() - vertMargin,
               left + w, gScreenInfo.getGameCanvasHeight() - vertMargin,
               left + w, gScreenInfo.getGameCanvasHeight() - vertMargin - height,
               left,     gScreenInfo.getGameCanvasHeight() - vertMargin - height
         };
         renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, i ? GL_LINE_LOOP : GL_TRIANGLE_FAN);
      }
   }
}


// Draw the reticle (i.e. the mouse cursor) if we are using keyboard/mouse
void GameUserInterface::renderReticle()
{
   bool shouldRender = getGame()->getSettings()->getInputCodeManager()->getInputMode() == InputModeKeyboard &&   // Reticle in keyboard mode only
                       getUIManager()->getCurrentUI()->getMenuID() == GameUI;                                    // And not when a menu is active
   if(shouldRender)
   {
      Point offsetMouse = mMousePoint + Point(gScreenInfo.getGameCanvasWidth() / 2, gScreenInfo.getGameCanvasHeight() / 2);

      F32 vertices[] = {
            // Center cross-hairs
            offsetMouse.x - 15, offsetMouse.y,
            offsetMouse.x + 15, offsetMouse.y,
            offsetMouse.x, offsetMouse.y - 15,
            offsetMouse.x, offsetMouse.y + 15,

            // Large axes lines
            0, offsetMouse.y,
            offsetMouse.x - 30, offsetMouse.y,

            offsetMouse.x + 30, offsetMouse.y,
            (F32)gScreenInfo.getGameCanvasWidth(), offsetMouse.y,

            offsetMouse.x, 0,
            offsetMouse.x, offsetMouse.y - 30,

            offsetMouse.x, offsetMouse.y + 30,
            offsetMouse.x, (F32)gScreenInfo.getGameCanvasHeight(),
      };

      static F32 colors[] = {
            0, 1, 0, 0.7,  //Colors::green
            0, 1, 0, 0.7,
            0, 1, 0, 0.7,
            0, 1, 0, 0.7,

            0, 1, 0, 0,
            0, 1, 0, 0.7,

            0, 1, 0, 0.7,
            0, 1, 0, 0,

            0, 1, 0, 0,
            0, 1, 0, 0.7,

            0, 1, 0, 0.7,
            0, 1, 0, 0,
      };

      renderColorVertexArray(vertices, colors, ARRAYSIZE(vertices) / 2, GL_LINES);
   }

   if(mWrongModeMsgDisplay.getCurrent())
   {
      glColor(Colors::paleRed);
      drawCenteredString(225, 20, "You are in joystick mode.");
      drawCenteredString(250, 20, "You can change to Keyboard input with the Options menu.");
   }
}


void GameUserInterface::onMouseDragged()
{
   TNLAssert(false, "Is this ever called?");  // Probably not!
   onMouseMoved();
}


void GameUserInterface::onMouseMoved()
{
   Parent::onMouseMoved();

   mMousePoint.set(gScreenInfo.getMousePos()->x - gScreenInfo.getGameCanvasWidth()  / 2,
                   gScreenInfo.getMousePos()->y - gScreenInfo.getGameCanvasHeight() / 2);

   if(getGame()->getInCommanderMap())     // Ship not in center of the screen in cmdrs map.  Where is it?
   {
      // If we join a server while in commander's map, we'll be here without a gameConnection and we'll get a crash without this check
      GameConnection *gameConnection = getGame()->getConnectionToServer();
      if(!gameConnection)
         return;

      // Here's our ship...
      Ship *ship = dynamic_cast<Ship *>(gameConnection->getControlObject());
      if(!ship)      // Can sometimes happen when switching levels. This will stop the ensuing crashing.
         return;

      Point o = ship->getRenderPos();  // To avoid taking address of temporary
      Point p = getGame()->worldToScreenPoint( &o );

      mCurrentMove.angle = atan2(mMousePoint.y + gScreenInfo.getGameCanvasHeight() / 2 - p.y, 
                                 mMousePoint.x + gScreenInfo.getGameCanvasWidth()  / 2 - p.x);
   }

   else     // Ship is at center of the screen
      mCurrentMove.angle = atan2(mMousePoint.y, mMousePoint.x);
}


// Is engineer enabled on this level?  Only set at beginning of level, not changed during game
void GameUserInterface::pregameSetup(bool engineerEnabled)
{
   mHelperManager.pregameSetup(engineerEnabled);
}


void GameUserInterface::setSelectedEngineeredObject(U32 objectType)
{
   mHelperManager.setSelectedEngineeredObject(objectType);
}


void GameUserInterface::activateHelper(HelperMenu::HelperMenuType helperType, bool activatedWithChatCmd)
{
   mHelperManager.activateHelper(helperType, activatedWithChatCmd);
   playBoop();
}


void GameUserInterface::exitHelper()
{
   mHelperManager.exitHelper();
}


void GameUserInterface::renderEngineeredItemDeploymentMarker(Ship *ship)
{
   mHelperManager.renderEngineeredItemDeploymentMarker(ship);
}


// Runs on client
void GameUserInterface::dropItem()
{
   if(!getGame()->getConnectionToServer())
      return;

   Ship *ship = dynamic_cast<Ship *>(getGame()->getConnectionToServer()->getControlObject());
   GameType *gt = getGame()->getGameType();
   if(!ship || !gt)
      return;

   if(!gt->isCarryingItems(ship))
   {
      displayMessage(Colors::paleRed, "You don't have any items to drop!");
      return;
   }

   gt->c2sDropItem();
}


// Select next weapon
void GameUserInterface::chooseNextWeapon()
{
   GameType *gameType = getGame()->getGameType();
   if(gameType)
      gameType->c2sChooseNextWeapon();
}


void GameUserInterface::choosePrevWeapon()
{
   GameType *gameType = getGame()->getGameType();
   if(gameType)
      gameType->c2sChoosePrevWeapon();
}


// Select a weapon by its index
void GameUserInterface::selectWeapon(U32 indx)
{
   GameType *gameType = getGame()->getGameType();
   if(gameType)
      gameType->c2sSelectWeapon(indx);
}


void GameUserInterface::activateModule(S32 index)
{
   // Still active, just return
   if(mModPrimaryActivated[index])
      return;

   // Activate module primary component
   mModPrimaryActivated[index] = true;

   // If the module secondary double-tap timer hasn't run out, activate the secondary component
   if(mModuleDoubleTapTimer[index].getCurrent() != 0)
      mModSecondaryActivated[index] = true;

   // Now reset the double-tap timer since we've just activate this module
   mModuleDoubleTapTimer[index].reset();
}


// A new loadout has arrived
void GameUserInterface::newLoadoutHasArrived(const LoadoutTracker &loadout)
{
   mLoadoutIndicator.newLoadoutHasArrived(loadout);
}


void GameUserInterface::setActiveWeapon(U32 weaponIndex)
{
   mLoadoutIndicator.setActiveWeapon(weaponIndex);
}


void GameUserInterface::setModulePrimary(ShipModule module, bool isActive)
{
   mLoadoutIndicator.setModulePrimary(module, isActive);
}


void GameUserInterface::setModuleSecondary(ShipModule module, bool isActive)
{
   mLoadoutIndicator.setModuleSecondary(module, isActive);
}


// Key pressed --> take action!
// Handles all keypress events, including mouse clicks and controller button presses
bool GameUserInterface::onKeyDown(InputCode inputCode)
{
   GameSettings *settings = getGame()->getSettings();

   // Kind of hacky, but this will unsuspend and swallow the keystroke, which is what we want
   if(!mHelperManager.isHelperActive() && getGame()->isSpawnDelayed())
   {
      getGame()->undelaySpawn();
      if(inputCode != KEY_ESCAPE)  // Lagged out and can't un-idle to bring up the menu?
         return true;
   }

   if(checkInputCode(settings, InputCodeManager::BINDING_OUTGAMECHAT, inputCode))
      getGame()->setBusyChatting(true);

   if(!mHelperManager.isHelperActive()) 
      getGame()->undelaySpawn();

   if(Parent::onKeyDown(inputCode))    // Let parent try handling the key
      return true;

   if(gConsole.onKeyDown(inputCode))   // Pass the key on to the console for processing
      return true;

   if(checkInputCode(settings, InputCodeManager::BINDING_HELP, inputCode))   // Turn on help screen
   {
      playBoop();
      getGame()->setBusyChatting(true);

      // If we have a helper, let that determine what happens when the help key is pressed.  Otherwise, show help normally.
      if(mHelperManager.isHelperActive())
         mHelperManager.activateHelp(getUIManager());
      else
         getUIManager()->activate(InstructionsUI);

      return true;
   }

   // Ctrl-/ toggles console window for the moment
   // Only open when there are no active helpers
   if(!mHelperManager.isHelperActive() && inputCode == KEY_SLASH && InputCodeManager::checkModifier(KEY_CTRL))
   {
      if(gConsole.isOk())                 // Console is only not Ok if something bad has happened somewhere
         gConsole.toggleVisibility();

      return true;
   }

   if(checkInputCode(settings, InputCodeManager::BINDING_MISSION, inputCode)) // F2
   {
      if(!mMissionOverlayActive)
      {
         mMissionOverlayActive = true;
         if(!mLevelInfoDisplayer.isDisplayTimerActive())
            mLevelInfoDisplayer.onActivated();

         mLevelInfoDisplayer.clearDisplayTimer();   // Clear level-start display timer so releasing F2 always hides display
      }

      return true;
   }

   if(inputCode == KEY_M && InputCodeManager::checkModifier(KEY_CTRL))        // Ctrl-M, for now, to cycle through message dispaly modes
   {
      toggleChatDisplayMode();
      return true;
   }

   if(mHelperManager.processInputCode(inputCode))   // Will return true if key was processed
   {
      // Experimental, to keep ship from moving after entering a quick chat that has the same shortcut as a movement key
      InputCodeManager::setState(inputCode, false);
      return true;
   }

   // If we're not in a helper, and we apply the engineer module, then we can handle that locally by displaying a menu or message
   if(!mHelperManager.isHelperActive())
   {
      Ship *ship = NULL;
      if(getGame()->getConnectionToServer())   // Prevents errors, getConnectionToServer() might be NULL, and getControlObject may crash if NULL
         ship = dynamic_cast<Ship *>(getGame()->getConnectionToServer()->getControlObject());
         
      if(ship)
      {
         if((checkInputCode(settings, InputCodeManager::BINDING_MOD1, inputCode) && ship->getModule(0) == ModuleEngineer) ||
            (checkInputCode(settings, InputCodeManager::BINDING_MOD2, inputCode) && ship->getModule(1) == ModuleEngineer))
         {
            string msg = EngineerModuleDeployer::checkResourcesAndEnergy(ship);      // Returns "" if ok, error message otherwise

            if(msg != "")
               displayErrorMessage(msg.c_str());
            else
               activateHelper(HelperMenu::EngineerHelperType);

            return true;
         }
      }
   }

   if(!gConsole.isVisible())
   {
      if(!isChatting())
         return processPlayModeKey(inputCode);
   }

   return false;
}


void GameUserInterface::onTextInput(char ascii)
{
   mHelperManager.onTextInput(ascii);
}


// Helper function...
static void saveLoadoutPreset(ClientGame *game, const LoadoutTracker *loadout, S32 slot)
{
   game->getSettings()->setLoadoutPreset(loadout, slot);
   game->displaySuccessMessage(("Current loadout saved as preset " + itos(slot + 1)).c_str());
}


static void loadLoadoutPreset(ClientGame *game, S32 slot)
{
   LoadoutTracker loadout = game->getSettings()->getLoadoutPreset(slot);

   if(!loadout.isValid())
   {
      string msg = "Preset " + itos(slot + 1) + " is undefined -- to define it, try Ctrl+" + itos(slot + 1);
      game->displayErrorMessage(msg.c_str());
      return;
   }

   GameType *gameType = game->getGameType();
   if(!gameType)
      return;
   

   GameConnection *conn = game->getConnectionToServer();
   if(!conn)
      return;

   if(game->getSettings()->getIniSettings()->verboseHelpMessages)
      game->displayShipDesignChangedMessage(loadout, "Preset same as the current design");

   // Request loadout even if it was the same -- if I have loadout A, with on-deck loadout B, and I enter a new loadout
   // that matches A, it would be better to have loadout remain unchanged if I entered a loadout zone.
   // Tell server loadout has changed.  Server will activate it when we enter a loadout zone.
   conn->c2sRequestLoadout(loadout.toU8Vector());    
}


bool checkInputCode(InputCode codeUserEntered, InputCode codeToActivateCommand)
{
   // Look for some common synonyms, like keypad_4 and 4
   return codeUserEntered == codeToActivateCommand;
}


// Helper function -- checks input keys and sees if we should start chatting.  Returns true if entered chat mode, false if not.
bool GameUserInterface::checkEnterChatInputCode(InputCode inputCode)
{
   GameSettings *settings = getGame()->getSettings();

   if(checkInputCode(settings, InputCodeManager::BINDING_TEAMCHAT, inputCode))          // Start entering a team chat msg
      mHelperManager.activateHelper(ChatHelper::TeamChat);
   else if(checkInputCode(settings, InputCodeManager::BINDING_GLOBCHAT, inputCode))     // Start entering a global chat msg
      mHelperManager.activateHelper(ChatHelper::GlobalChat);
   else if(checkInputCode(settings, InputCodeManager::BINDING_CMDCHAT, inputCode))      // Start entering a command
      mHelperManager.activateHelper(ChatHelper::CmdChat);
   else
      return false;

   return true;
}


// Can only get here if we're not in chat mode
bool GameUserInterface::processPlayModeKey(InputCode inputCode)
{
   GameSettings *settings = getGame()->getSettings();

   // The following keys are allowed in both play mode and in loadout or
   // engineering menu modes if not used in the loadout menu above
   if(inputCode == KEY_CLOSEBRACKET && InputCodeManager::checkModifier(KEY_ALT))          // Alt+] advances bots by one step if frozen
      EventManager::get()->addSteps(1);
   else if(inputCode == KEY_CLOSEBRACKET && InputCodeManager::checkModifier(KEY_CTRL))    // Ctrl+] advances bots by 10 steps if frozen
      EventManager::get()->addSteps(10);
   else if(inputCode == KEY_1 && InputCodeManager::checkModifier(KEY_CTRL))               // Ctrl+1 saves loadout preset in first slot
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 0);
   else if(inputCode == KEY_1 && InputCodeManager::checkModifier(KEY_ALT))                // Alt+1 loads preset from first slot
      loadLoadoutPreset(getGame(), 0);
   else if(inputCode == KEY_2 && InputCodeManager::checkModifier(KEY_CTRL))               // Ctrl+2
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 1);
   else if(inputCode == KEY_2 && InputCodeManager::checkModifier(KEY_ALT))                // Alt+2
      loadLoadoutPreset(getGame(), 1);
   else if(inputCode == KEY_3 && InputCodeManager::checkModifier(KEY_CTRL))               // Ctrl+3
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 2);
   else if(inputCode == KEY_3 && InputCodeManager::checkModifier(KEY_ALT))                // Alt+3
      loadLoadoutPreset(getGame(), 2);

   else if(checkInputCode(settings, InputCodeManager::BINDING_MOD1, inputCode))       
      activateModule(0);
   else if(checkInputCode(settings, InputCodeManager::BINDING_MOD2, inputCode))       
      activateModule(1);
   else if(checkInputCode(settings, InputCodeManager::BINDING_FIRE, inputCode))       
      mFiring = true;
   else if(checkInputCode(settings, InputCodeManager::BINDING_SELWEAP1, inputCode))
      selectWeapon(0);
   else if(checkInputCode(settings, InputCodeManager::BINDING_SELWEAP2, inputCode))
      selectWeapon(1);
   else if(checkInputCode(settings, InputCodeManager::BINDING_SELWEAP3, inputCode))
      selectWeapon(2);
   else if(checkInputCode(settings, InputCodeManager::BINDING_FPS, inputCode))
      mFpsRenderer.toggleVisibility();
   else if(checkInputCode(settings, InputCodeManager::BINDING_ADVWEAP, inputCode))
      chooseNextWeapon();

   // By default, Handle mouse wheel. Users can change it in "Define Keys" option
   else if(checkInputCode(settings, InputCodeManager::BINDING_ADVWEAP2, inputCode))
      chooseNextWeapon();
   else if(checkInputCode(settings, InputCodeManager::BINDING_PREVWEAP, inputCode))
      choosePrevWeapon();

   else if(inputCode == KEY_ESCAPE || inputCode == BUTTON_BACK)
   {
      if(mShutdownMode == ShuttingDown)
      {
         if(mShutdownInitiator)
         {
            getGame()->getConnectionToServer()->c2sRequestCancelShutdown();
            mShutdownMode = Canceled;
         }
         else
            mShutdownMode = None;

         return true;
      }
      else if(mShutdownMode == Canceled)
      {
         mShutdownMode = None;
         return true;
      }

      playBoop();

      if(!getGame()->isConnectedToServer())     // Perhaps we're still joining?
      {
         getGame()->closeConnectionToGameServer();
         getUIManager()->activate(MainUI);      // Back to main menu
      }
      else
      {
         getGame()->setBusyChatting(true);
         getUIManager()->activate(GameMenuUI);
      }
   }     
   else if(checkInputCode(settings, InputCodeManager::BINDING_CMDRMAP, inputCode))
      getGame()->zoomCommanderMap();

   else if(checkInputCode(settings, InputCodeManager::BINDING_SCRBRD, inputCode))
   {     // (braces needed)
      if(!mInScoreboardMode)    // We're activating the scoreboard
      {
         mInScoreboardMode = true;
         GameType *gameType = getGame()->getGameType();
         if(gameType)
            gameType->c2sRequestScoreboardUpdates(true);
      }
   }
   else if(checkInputCode(settings, InputCodeManager::BINDING_TOGVOICE, inputCode))
   {     // (braces needed)
      if(!mVoiceRecorder.mRecordingAudio)  // Turning recorder on
         mVoiceRecorder.start();
   }

   // The following keys are only allowed when there are no helpers or when the top helper permits
   else if(mHelperManager.isChatAllowed())    
   {
      if(checkEnterChatInputCode(inputCode))
         return true;

      // These keys are only available when there is no helper active
      if(!mHelperManager.isHelperActive())
      {
         if(checkInputCode(settings, InputCodeManager::BINDING_QUICKCHAT, inputCode))
            activateHelper(HelperMenu::QuickChatHelperType);
         else if(checkInputCode(settings, InputCodeManager::BINDING_LOADOUT, inputCode))
            activateHelper(HelperMenu::LoadoutHelperType);
         else if(checkInputCode(settings, InputCodeManager::BINDING_DROPITEM, inputCode))
            dropItem();
         // Check if the user is trying to use keyboard to move when in joystick mode
         else if(settings->getInputCodeManager()->getInputMode() == InputModeJoystick)      
            checkForKeyboardMovementKeysInJoystickMode(inputCode);
      }
   }
   else
      return false;

   return true;
}


// Show a message if the user starts trying to play with keyboard in joystick mode
void GameUserInterface::checkForKeyboardMovementKeysInJoystickMode(InputCode inputCode)
{
   GameSettings *settings = getGame()->getSettings();

   if(checkInputCode(settings, InputCodeManager::BINDING_UP,    inputCode) ||
      checkInputCode(settings, InputCodeManager::BINDING_DOWN,  inputCode) ||
      checkInputCode(settings, InputCodeManager::BINDING_LEFT,  inputCode) ||
      checkInputCode(settings, InputCodeManager::BINDING_RIGHT, inputCode))
         mWrongModeMsgDisplay.reset(WRONG_MODE_MSG_DISPLAY_TIME);
}


// Display proper chat queue based on mMessageDisplayMode.  These displayers are configured in the constructor. 
void GameUserInterface::renderChatMsgs()
{
   bool chatDisabled = !mHelperManager.isChatAllowed();
   bool announcementActive = (mAnnouncementTimer.getCurrent() != 0);

   F32 alpha = mHelperManager.getDimFactor();

   if(mMessageDisplayMode == ShortTimeout)
      mChatMessageDisplayer1.render(IN_GAME_CHAT_DISPLAY_POS, chatDisabled, announcementActive, alpha);
   else if(mMessageDisplayMode == ShortFixed)
      mChatMessageDisplayer2.render(IN_GAME_CHAT_DISPLAY_POS, chatDisabled, announcementActive, alpha);
   else
      mChatMessageDisplayer3.render(IN_GAME_CHAT_DISPLAY_POS, chatDisabled, announcementActive, alpha);

   F32 showIndicators = getGame()->getSettings()->getIniSettings()->showWeaponIndicators;
   mServerMessageDisplayer.render(showIndicators ? messageMargin : vertMargin, chatDisabled, false, alpha);

   if(announcementActive)
      renderAnnouncement(IN_GAME_CHAT_DISPLAY_POS);
}


void GameUserInterface::renderAnnouncement(S32 pos)
{
   glColor(Colors::red);
   glLineWidth(gLineWidth4);

   S32 x = drawStringAndGetWidth(UserInterface::horizMargin, pos, 16, "*** ");
   x += drawStringAndGetWidth(UserInterface::horizMargin + x, pos, 16, mAnnouncement.c_str());
   drawString(UserInterface::horizMargin + x, pos, 16, " ***");

   glLineWidth(gDefaultLineWidth);
}


void GameUserInterface::onKeyUp(InputCode inputCode)
{
   // These keys works in any mode!  And why not??

   GameSettings *settings = getGame()->getSettings();

   if(checkInputCode(settings, InputCodeManager::BINDING_MISSION, inputCode))
   {
      mMissionOverlayActive = false;
      mLevelInfoDisplayer.onDeactivated();
   }
   else if(checkInputCode(settings, InputCodeManager::BINDING_MOD1, inputCode))
   {
      mModPrimaryActivated[0] = false;
      mModSecondaryActivated[0] = false;
   }
   else if(checkInputCode(settings, InputCodeManager::BINDING_MOD2, inputCode))
   {
      mModPrimaryActivated[1] = false;
      mModSecondaryActivated[1] = false;
   }
   else if(checkInputCode(settings, InputCodeManager::BINDING_FIRE, inputCode))
      mFiring = false;
   else if(checkInputCode(settings, InputCodeManager::BINDING_SCRBRD, inputCode))
   {     // (braces required)
      if(mInScoreboardMode)     // We're turning scoreboard off
      {
         mInScoreboardMode = false;
         GameType *gameType = getGame()->getGameType();
         if(gameType)
            gameType->c2sRequestScoreboardUpdates(false);
      }
   }
   else if(checkInputCode(settings, InputCodeManager::BINDING_TOGVOICE, inputCode))
   {     // (braces required)
      if(mVoiceRecorder.mRecordingAudio)  // Turning recorder off
         mVoiceRecorder.stop();
   }
}


void GameUserInterface::receivedControlUpdate(bool recvd)
{
   mGotControlUpdate = recvd;
}


bool GameUserInterface::isInScoreboardMode()
{
   return mInScoreboardMode;
}


// Return current move (actual move processing in ship.cpp)
// Will also transform move into "relative" mode if needed
// Note that all input supplied here will be overwritten if
// we are using a game controller. 
// Runs only on client
Move *GameUserInterface::getCurrentMove()
{
   if(!mDisableShipKeyboardInput && !gConsole.isVisible())
   {
      // Some helpers (like TeamShuffle) like to disable movement when they are active
      if(mHelperManager.isMovementDisabled())
      {
         mCurrentMove.x = 0;
         mCurrentMove.y = 0;
      }
      else
      {
         GameSettings *settings = getGame()->getSettings();

         mCurrentMove.x = F32((InputCodeManager::getState(getInputCode(settings, InputCodeManager::BINDING_RIGHT)) ? 1 : 0) - 
                              (InputCodeManager::getState(getInputCode(settings, InputCodeManager::BINDING_LEFT))  ? 1 : 0));

         mCurrentMove.y = F32((InputCodeManager::getState(getInputCode(settings, InputCodeManager::BINDING_DOWN))  ? 1 : 0) -  
                              (InputCodeManager::getState(getInputCode(settings, InputCodeManager::BINDING_UP))    ? 1 : 0));
      }

      mCurrentMove.fire = mFiring;

      for(U32 i = 0; i < (U32)ShipModuleCount; i++)
      {
         mCurrentMove.modulePrimary[i] = mModPrimaryActivated[i];
         mCurrentMove.moduleSecondary[i] = mModSecondaryActivated[i];
      }
   }
   else
   {
      mCurrentMove.x = 0;
      mCurrentMove.y = 0;

      mCurrentMove.fire = mFiring;     // should be false?

      for(U32 i = 0; i < (U32)ShipModuleCount; i++)
      {
         mCurrentMove.modulePrimary[i] = false;
         mCurrentMove.moduleSecondary[i] = false;
      }
   }

   if(!getGame()->getSettings()->getIniSettings()->controlsRelative)
      return &mCurrentMove;

   else     // Using relative controls -- all turning is done relative to the direction of the ship.
   {
      mTransformedMove = mCurrentMove;

      Point moveDir(mCurrentMove.x, -mCurrentMove.y);

      Point angleDir(cos(mCurrentMove.angle), sin(mCurrentMove.angle));

      Point rightAngleDir(-angleDir.y, angleDir.x);
      Point newMoveDir = angleDir * moveDir.y + rightAngleDir * moveDir.x;

      mTransformedMove.x = newMoveDir.x;
      mTransformedMove.y = newMoveDir.y;

      // Sanity checks
      mTransformedMove.x = min(1.0f, mTransformedMove.x);
      mTransformedMove.y = min(1.0f, mTransformedMove.y);
      mTransformedMove.x = max(-1.0f, mTransformedMove.x);
      mTransformedMove.y = max(-1.0f, mTransformedMove.y);

      return &mTransformedMove;
   }
}


void GameUserInterface::resetLevelInfoDisplayTimer()
{
   mLevelInfoDisplayer.onActivated();
   mLevelInfoDisplayer.resetDisplayTimer();
}


// Constructor
GameUserInterface::VoiceRecorder::VoiceRecorder(ClientGame *game)
{
   mRecordingAudio = false;
   mMaxAudioSample = 0;
   mMaxForGain = 0;
   mVoiceEncoder = new SpeexVoiceEncoder;

   mGame = game;

   mWantToStopRecordingAudio = false;
}


GameUserInterface::VoiceRecorder::~VoiceRecorder()
{
   stopNow();
}


void GameUserInterface::VoiceRecorder::idle(U32 timeDelta)
{
   if(mRecordingAudio)
   {
      if(mVoiceAudioTimer.update(timeDelta))
      {
         mVoiceAudioTimer.reset(VoiceAudioSampleTime);
         process();
      }
   }
}


void GameUserInterface::VoiceRecorder::render()
{
   if(mRecordingAudio)
   {
      F32 amt = mMaxAudioSample / F32(0x7FFF);
      U32 totalLineCount = 50;

      // Render low/high volume lines
      glColor(1, 1 ,1);
      F32 vertices[] = {
            10, 130,
            10, 145,
            10 + totalLineCount * 2, 130,
            10 + totalLineCount * 2, 145
      };
      renderVertexArray(vertices, ARRAYSIZE(vertices)/2, GL_LINES);

      F32 halfway = totalLineCount * 0.5f;
      F32 full = amt * totalLineCount;

      // Total items possible is totalLineCount (50)
      static F32 colorArray[400];   // 2 * 4 color components per item
      static F32 vertexArray[200];  // 2 * 2 vertex components per item

      // Render recording volume
      for(U32 i = 1; i < full; i++)  // start at 1 to not show
      {
         if(i < halfway)
         {
            colorArray[8*(i-1)]     = i / halfway;
            colorArray[(8*(i-1))+1] = 1;
            colorArray[(8*(i-1))+2] = 0;
            colorArray[(8*(i-1))+3] = 1;
            colorArray[(8*(i-1))+4] = i / halfway;
            colorArray[(8*(i-1))+5] = 1;
            colorArray[(8*(i-1))+6] = 0;
            colorArray[(8*(i-1))+7] = 1;
         }
         else
         {
            colorArray[8*(i-1)]     = 1;
            colorArray[(8*(i-1))+1] = 1 - (i - halfway) / halfway;
            colorArray[(8*(i-1))+2] = 0;
            colorArray[(8*(i-1))+3] = 1;
            colorArray[(8*(i-1))+4] = 1;
            colorArray[(8*(i-1))+5] = 1 - (i - halfway) / halfway;
            colorArray[(8*(i-1))+6] = 0;
            colorArray[(8*(i-1))+7] = 1;
         }

         vertexArray[4*(i-1)]     = 10 + i * 2;
         vertexArray[(4*(i-1))+1] = 130;
         vertexArray[(4*(i-1))+2] = 10 + i * 2;
         vertexArray[(4*(i-1))+3] = 145;
      }

      renderColorVertexArray(vertexArray, colorArray, S32(full*2), GL_LINES);
   }
}


void GameUserInterface::VoiceRecorder::start()
{
   if(!(mGame->getConnectionToServer() && mGame->getConnectionToServer()->mVoiceChatEnabled))
   {
      mGame->displayErrorMessage("!!! Voice chat not allowed on this server");
      return;
   }

   mWantToStopRecordingAudio = 0; // linux repeadedly sends key-up / key-down when only holding key down (that was in GLUT, may )
   if(!mRecordingAudio)
   {
      mRecordingAudio = SoundSystem::startRecording();
      if(!mRecordingAudio)
         return;

      mUnusedAudio = new ByteBuffer(0);
      mRecordingAudio = true;
      mMaxAudioSample = 0;
      mVoiceAudioTimer.reset(FirstVoiceAudioSampleTime);

      // trim the start of the capture buffer:
      SoundSystem::captureSamples(mUnusedAudio);
      mUnusedAudio->resize(0);
   }
}

void GameUserInterface::VoiceRecorder::stopNow()
{
   if(mRecordingAudio)
   {
      process();

      mRecordingAudio = false;
      SoundSystem::stopRecording();
      mVoiceSfx = NULL;
      mUnusedAudio = NULL;
   }
}
void GameUserInterface::VoiceRecorder::stop()
{
   if(mWantToStopRecordingAudio == 0)
      mWantToStopRecordingAudio = 2;
}


void GameUserInterface::VoiceRecorder::process()
{
   if(!(mGame->getConnectionToServer() && mGame->getConnectionToServer()->mVoiceChatEnabled))
      stop();

   if(mWantToStopRecordingAudio != 0)
   {
      mWantToStopRecordingAudio--;
      if(mWantToStopRecordingAudio == 0)
      {
         stopNow();
         return;
      }
   }
   U32 preSampleCount = mUnusedAudio->getBufferSize() / 2;
   SoundSystem::captureSamples(mUnusedAudio);

   U32 sampleCount = mUnusedAudio->getBufferSize() / 2;
   if(sampleCount == preSampleCount)
      return;

   S16 *samplePtr = (S16 *) mUnusedAudio->getBuffer();
   mMaxAudioSample = 0;

   for(U32 i = preSampleCount; i < sampleCount; i++)
   {
      if(samplePtr[i] > mMaxAudioSample)
         mMaxAudioSample = samplePtr[i];
      else if(-samplePtr[i] > mMaxAudioSample)
         mMaxAudioSample = -samplePtr[i];
   }
   mMaxForGain = U32(mMaxForGain * 0.95f);
   S32 boostedMax = mMaxAudioSample + 2048;

   if(boostedMax > mMaxForGain)
      mMaxForGain = boostedMax;

   if(mMaxForGain > MaxDetectionThreshold)
   {
      // Apply some gain to the buffer:
      F32 gain = 0x7FFF / F32(mMaxForGain);
      for(U32 i = preSampleCount; i < sampleCount; i++)
      {
         F32 sample = gain * samplePtr[i];
         if(sample > 0x7FFF)
            samplePtr[i] = 0x7FFF;
         else if(sample < -0x7FFF)
            samplePtr[i] = -0x7FFF;
         else
            samplePtr[i] = S16(sample);
      }
      mMaxAudioSample = U32(mMaxAudioSample * gain);
   }

   ByteBufferPtr sendBuffer = mVoiceEncoder->compressBuffer(mUnusedAudio);

   if(sendBuffer.isValid())
   {
      GameType *gameType = mGame->getGameType();

      if(gameType && sendBuffer->getBufferSize() < 1024)      // Don't try to send too big
         gameType->c2sVoiceChat(mGame->getSettings()->getIniSettings()->echoVoice, sendBuffer);
   }
}


#ifdef USE_DUMMY_PLAYER_SCORES

S32 getDummyTeamCount() { return 2; }     // Teams
S32 getDummyMaxPlayers() { return 5; }    // Players per team

// Create a set of fake player scores for testing the scoreboard -- fill scores
void getDummyPlayerScores(Vector<ClientInfo *> &scores)
{
   ClientInfo *clientInfo;

   S32 teams = getDummyTeamCount();

   for(S32 i = 0; i < getDummyMaxPlayers(); i++)
   {
      string name = "PlayerName-" + itos(i);

      clientInfo = new RemoteClientInfo(name, false, ((i+1) % 4) > 0);

      clientInfo->setScore(i * 3);
      clientInfo->setAuthenticated((i % 3) > 0);
      clientInfo->setIsLevelChanger(((i+1) % 2) > 0);
      clientInfo->setPing(100 * i + 10);
      clientInfo->setTeamIndex(i % teams);

      scores.push_back(clientInfo);
   }
}
#endif


void GameUserInterface::renderScoreboard()
{
   // This is probably not needed... if gameType were NULL, we'd have crashed and burned long ago
   GameType *gameType = getGame()->getGameType();
   if(!gameType)
      return;

   const bool isTeamGame = gameType->isTeamGame();

#ifdef USE_DUMMY_PLAYER_SCORES
   S32 maxTeamPlayers = getDummyMaxPlayers();
   S32 teams = isTeamGame ? getDummyTeamCount() : 1;
#else
   getGame()->countTeamPlayers();

   const S32 teams = isTeamGame ? getGame()->getTeamCount() : 1;
   S32 maxTeamPlayers = 0;

   // Check to make sure at least one team has at least one player...
   for(S32 i = 0; i < teams; i++)
   {
      Team *team = (Team *)getGame()->getTeam(i);

      if(!isTeamGame)
         maxTeamPlayers += team->getPlayerBotCount();

      else if(team->getPlayerBotCount() > maxTeamPlayers)
         maxTeamPlayers = team->getPlayerBotCount();
   }
#endif
   // ...if not, then go home!
   if(maxTeamPlayers == 0)
      return;


   const U32 drawableWidth = gScreenInfo.getGameCanvasWidth() - horizMargin * 2;
   const U32 columnCount = min(teams, 2);
   const U32 teamWidth = drawableWidth / columnCount;
   const U32 teamAreaHeight = isTeamGame ? 40 : 0;

   const U32 numTeamRows = (teams + 1) >> 1;
   const U32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   const U32 desiredHeight = (canvasHeight - vertMargin * 2) / numTeamRows - (numTeamRows - 1) * 2;
   const U32 maxHeight = MIN(30, (desiredHeight - teamAreaHeight) / maxTeamPlayers);

   const U32 sectionHeight = (teamAreaHeight + maxHeight * maxTeamPlayers);
   const U32 totalHeight = sectionHeight * numTeamRows + (numTeamRows - 1) * 2;

   // Vertical scale ratio to maximum line height
   const F32 scaleRatio = ((F32)maxHeight) / 30.f;

   const char *botSymbol = "B ";
   const char *levelChangerSymbol = "+ ";
   const char *adminSymbol = "@ ";


   for(S32 i = 0; i < teams; i++)
   {
      const S32 yt = (canvasHeight - totalHeight) / 2 + (i >> 1) * (sectionHeight + 2);  // y-top
      const S32 yb = yt + sectionHeight;     // y-bottom
      const S32 xl = 10 + (i & 1) * teamWidth;
      const S32 xr = xl + teamWidth - 2;

      const Color *teamColor = getGame()->getTeamColor(i);

      TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

      glColor(teamColor, 0.6f);
      drawRect(xl, yt, xr, yb, GL_TRIANGLE_FAN);

      //// Render team scores
      glColor(Colors::white);
      if(isTeamGame)     
      {
         renderFlag(F32(xl + 20), F32(yt + 18), teamColor);
         renderFlag(F32(xr - 20), F32(yt + 18), teamColor);

         drawHorizLine(xl, xr, yt + S32(teamAreaHeight));

         drawString(xl + 40, yt + 2, 30, getGame()->getTeamName(i).getString());
         drawStringf(xr - 140, yt + 2, 30, "%d", ((Team *)(getGame()->getTeam(i)))->getScore());
      }

      // Now for player scores.  First build a list, then sort it, then display it.
      Vector<ClientInfo *> playerScores;

#ifdef USE_DUMMY_PLAYER_SCORES
      getDummyPlayerScores(playerScores);
#else
      gameType->getSortedPlayerScores(i, playerScores);     // Fills playerScores for team i
#endif

      const S32 fontSize = U32(maxHeight * 0.85f);

      S32 curRowY = yt + teamAreaHeight + 1;

      // Use any symbol for an offset
      const S32 symbolFontSize = S32(fontSize * 0.8f);
      const S32 symbolSize = getStringWidth(symbolFontSize, botSymbol);

      for(S32 j = 0; j < playerScores.size(); j++)
      {
         S32 x = xl + 40;
         S32 vertAdjustFact = (fontSize - symbolFontSize) / 2 - 1;

         bool isDelayed = playerScores[j]->isSpawnDelayed();
         const Color *nameColor = isDelayed ? &Colors::idlePlayerScoreboardColor : &Colors::standardPlayerScoreboardColor;


         glColor(nameColor);

         // Add the mark of the bot
         if(playerScores[j]->isRobot())
            drawString(x - symbolSize, curRowY + vertAdjustFact + 2, symbolFontSize, botSymbol);

         // Add level changer mark
         if(playerScores[j]->isLevelChanger() && !playerScores[j]->isAdmin())
            drawString(x - symbolSize, curRowY + vertAdjustFact + 2, symbolFontSize, levelChangerSymbol);

         // Add admin mark
         if(playerScores[j]->isAdmin())
            drawString(x - symbolSize, curRowY + vertAdjustFact + 2, symbolFontSize, adminSymbol);

         S32 nameWidth = drawStringAndGetWidth(x - 8, curRowY, fontSize, playerScores[j]->getName().getString());

         renderBadges(playerScores[j], x + nameWidth + 8, curRowY + (maxHeight / 2), scaleRatio);
         
         glColor(nameColor);
         static char buff[255] = "";

         if(isTeamGame)
            dSprintf(buff, sizeof(buff), "%2.2f", playerScores[j]->getRating());
         else
         {
            if(playerScores[j]->getRating() < 0)
               dSprintf(buff, sizeof(buff), "%d %2.2f", playerScores[j]->getScore(), playerScores[j]->getRating());
            else
               dSprintf(buff, sizeof(buff), "%d  %2.2f", playerScores[j]->getScore(), playerScores[j]->getRating());
         }

         drawString(xr - (85 + S32(getStringWidth(F32(fontSize), buff))), curRowY, fontSize, buff);
         drawStringf(xr - 60, curRowY, fontSize, "%d", playerScores[j]->getPing());
         curRowY += maxHeight;
      }

#ifdef USE_DUMMY_PLAYER_SCORES
      playerScores.deleteAndClear();      // Clean up
#endif
   }

   // Render symbol legend 
   const S32 legendSize = 12;     
   const S32 legendGap  =  3;    // Space between scoreboard and legend

   const S32 humans     = getGame()->getPlayerCount();
   const S32 legendPos  = (canvasHeight - totalHeight) / 2 + totalHeight + legendGap;   

   string legend = itos(humans) + " Human" + (humans != 1 ? "s" : "") + " | " + adminSymbol + "= Admin | " + 
                   levelChangerSymbol + "= Can Change Levels | " + botSymbol + "= Bot |";

   // Not quite the function's intended purpose, but it does the job
   drawCenteredStringPair(legendPos, legendSize, Colors::standardPlayerScoreboardColor, 
                          Colors::idlePlayerScoreboardColor, legend.c_str(), "Idle Player");
}


void GameUserInterface::renderBadges(ClientInfo *clientInfo, S32 x, S32 y, F32 scaleRatio)
{
   F32 badgeRadius = 10.f * scaleRatio;
   S32 badgeOffset = S32(2 * badgeRadius) + 5;
   F32 badgeBackgroundEdgeSize = 2 * badgeRadius + 2.f;

   bool hasBBBBadge = false;

   for(S32 i = 0; i < BADGE_COUNT; i++)
   {
      MeritBadges badge = MeritBadges(i);    // C++ enums can be rather tedious...

      if(clientInfo->hasBadge(badge))
      {
         // Test for BBB badges.  We're only going to show the most valued one
         if(badge == BADGE_BBB_GOLD || badge == BADGE_BBB_SILVER || badge == BADGE_BBB_BRONZE || badge == BADGE_BBB_PARTICIPATION)
         {
            // If we've already got one, don't draw this badge.  This assumes the value of the badges decrease
            // with each iteration
            if(hasBBBBadge)
               continue;

            hasBBBBadge = true;
         }

         drawFilledRoundedRect(Point(x,y), badgeBackgroundEdgeSize, badgeBackgroundEdgeSize, Colors::black, Colors::black, 3.f);
         renderBadge((F32)x, (F32)y, badgeRadius, badge);
         x += badgeOffset;
      }
   }
}


void GameUserInterface::renderBasicInterfaceOverlay(const GameType *gameType, bool scoreboardVisible)
{
   if(!mLevelInfoDisplayer.isActive())
      int x = 0;

   if(mLevelInfoDisplayer.isActive() || mMissionOverlayActive)
   {
      mLevelInfoDisplayer.render(gameType, getGame()->getTeamCount(), getInputCodeString(getGame()->getSettings(), 
                                 InputCodeManager::BINDING_MISSION), mMissionOverlayActive);
      mInputModeChangeAlertDisplayTimer.reset(0);     // Supress mode change alert if this message is displayed...
   }

   if(mInputModeChangeAlertDisplayTimer.getCurrent() != 0)
      renderInputModeChangeAlert();

   Game *game = getGame();
   S32 teamCount = game->getTeamCount();

   bool showScore = gameType->isGameOver() || scoreboardVisible;

   if(showScore && teamCount > 0)      // How could teamCount be 0?
      renderScoreboard();
   
   // Render timer and associated doodads in the lower-right corner
   mTimeLeftRenderer.render(gameType, showScore);

   renderTalkingClients();
   renderDebugStatus();
}


// Display alert about input mode changing
void GameUserInterface::renderInputModeChangeAlert()
{
   F32 alpha = 1;

   if(mInputModeChangeAlertDisplayTimer.getCurrent() < 1000)
      alpha = mInputModeChangeAlertDisplayTimer.getCurrent() * 0.001f;

   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   glColor(Colors::paleRed, alpha);
   drawCenteredStringf(vertMargin + 130, 20, "Input mode changed to %s", 
                       getGame()->getSettings()->getInputCodeManager()->getInputMode() == InputModeJoystick ? "Joystick" : "Keyboard");
}


void GameUserInterface::renderTalkingClients()
{
   S32 y = 150;

   for(S32 i = 0; i < getGame()->getClientCount(); i++)
   {
      ClientInfo *client = ((Game *)getGame())->getClientInfo(i);

      if(client->getVoiceSFX()->isPlaying())
      {
         const S32 TEXT_HEIGHT = 20;

         glColor( getGame()->getTeamColor(client->getTeamIndex()) );
         drawString(10, y, TEXT_HEIGHT, client->getName().getString());
         y += TEXT_HEIGHT + 5;
      }
   }
}


void GameUserInterface::renderDebugStatus()
{
   // When bots are frozen, render large pause icon in lower left
   if(EventManager::get()->isPaused())
   {
      glColor(Colors::white);

      const S32 PAUSE_HEIGHT = 30;
      const S32 PAUSE_WIDTH = 10;
      const S32 PAUSE_GAP = 6;
      const S32 BOX_INSET = 5;

      const S32 TEXT_SIZE = 15;
      const char *TEXT = "STEP: Alt-], Ctrl-]";

      S32 x, y;

      // Draw box
      x = gScreenInfo.getGameCanvasWidth() - horizMargin - 2 * (PAUSE_WIDTH + PAUSE_GAP) - BOX_INSET - getStringWidth(TEXT_SIZE, TEXT);
      y = vertMargin + PAUSE_HEIGHT;

      // Draw Pause symbol
      drawFilledRect(x, y, x + PAUSE_WIDTH, y - PAUSE_HEIGHT, Colors::black, Colors::white);

      x += PAUSE_WIDTH + PAUSE_GAP;
      drawFilledRect(x, y, x + PAUSE_WIDTH, y - PAUSE_HEIGHT, Colors::black, Colors::white);

      x += PAUSE_WIDTH + PAUSE_GAP + BOX_INSET;

      y -= TEXT_SIZE + (PAUSE_HEIGHT - TEXT_SIZE) / 2 + 1;
      drawString(x, y, TEXT_SIZE, TEXT);
   }
}


void GameUserInterface::onChatMessageReceived(const Color &msgColor, const char *format, ...)
{
   // Ignore empty message
   if(!strcmp(format, ""))
      return;

   static char buffer[MAX_CHAT_MSG_LENGTH];

   va_list args;

   va_start(args, format);
   vsnprintf(buffer, sizeof(buffer), format, args);
   va_end(args);

   mChatMessageDisplayer1.onChatMessageReceived(msgColor, buffer);      // Standard chat stream
   mChatMessageDisplayer2.onChatMessageReceived(msgColor, buffer);      // Short, non-expiring chat stream
   mChatMessageDisplayer3.onChatMessageReceived(msgColor, buffer);      // Long, non-expiring chat stream
}


// Set which chat message display mode we're in (Ctrl-M)
void GameUserInterface::toggleChatDisplayMode()
{
   S32 m = mMessageDisplayMode + 1;

   if(m >= MessageDisplayModes)
      m = 0;

   mMessageDisplayMode = MessageDisplayMode(m);
}


// Return message being composed in in-game chat
const char *GameUserInterface::getChatMessage()
{
   return mHelperManager.getChatMessage();
}


////////////////////////////////////////
////////////////////////////////////////

void ColorString::set(const string &s, const Color &c, U32 id)    // id defaults to 0
{
   str = s;
   color = c;
   groupId = id;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
ChatMessageDisplayer::ChatMessageDisplayer(ClientGame *game, S32 msgCount, bool expire, bool topDown, S32 wrapWidth, S32 fontSize, S32 fontWidth)
{
   mDisplayChatMessageTimer.setPeriod(5000);    // How long messages stay visible (ms)
   mChatScrollTimer.setPeriod(100);             // Transition time when new msg arrives (ms) 

   mMessages.resize(msgCount + 1);              // Have an extra message for scrolling effect.  Will only display msgCount messages.

   reset();

   mGame      = game;
   mExpire    = expire;
   mTopDown   = topDown;
   mWrapWidth = wrapWidth;
   mFontSize  = fontSize;
   mFontGap   = fontWidth;
   
   mNextGroupId = 0;
}


// Effectivley clears all messages
void ChatMessageDisplayer::reset()
{
   mFirst = mLast = 0;
   mFull = false;
}


void ChatMessageDisplayer::idle(U32 timeDelta)
{
   mChatScrollTimer.update(timeDelta);

   // Clear out any expired messages
   if(mExpire && mDisplayChatMessageTimer.update(timeDelta))
   {
      mDisplayChatMessageTimer.reset();

      if(mFirst > mLast)
      {
         if(mTopDown)
            mChatScrollTimer.reset();

         advanceLast();         
      }
   }
}


// Make room for a new message at the head of our list
void ChatMessageDisplayer::advanceFirst()
{
   mFirst++;

   if(mLast % mMessages.size() == mFirst % mMessages.size())
   {
      mLast++;
      mFull = true;
   }
}


// Clear out messages from the back of our list; expire all messages with same id together.
void ChatMessageDisplayer::advanceLast()
{
   mLast++;

   U32 id = mMessages[mLast % mMessages.size()].groupId;

   while(mMessages[(mLast + 1) % mMessages.size()].groupId == id && mFirst > mLast)
      mLast++;

   mFull = false;

   TNLAssert(mLast <= mFirst, "index error! -- add check to correct this!");
}


// Replace %vars% in chat messages 
// Currently only evaluates names of keybindings (as used in the INI file), and %playerName%
// Vars are case insensitive
static string getSubstVarVal(ClientGame *game, const string &var)
{
   // %keybinding%
   InputCode inputCode = game->getSettings()->getInputCodeManager()->getKeyBoundToBindingCodeName(var);
   if(inputCode != KEY_UNKNOWN)
      return string("[") + InputCodeManager::inputCodeToString(inputCode) + "]";
   
   // %playerName%
   if(caseInsensitiveStringCompare(var, "playerName"))
      return game->getClientInfo()->getName().getString();

   // Not a variable... preserve formatting
   return "%" + var + "%";
}


// Add it to the list, will be displayed in render()
void ChatMessageDisplayer::onChatMessageReceived(const Color &msgColor, const string &msg)
{
   Vector<string> lines = wrapString(substitueVars(msg), mWrapWidth, mFontSize, "      ");

   // All lines from this message will share a groupId.  We'll use that to expire the group as a whole.
   for(S32 i = 0; i < lines.size(); i++)
   {
      advanceFirst();
      mMessages[mFirst % mMessages.size()].set(lines[i], msgColor, mNextGroupId); 
   }

   mNextGroupId++;

   // When displaying messages from the top of the screen, the animation happens when we expire messages
   mDisplayChatMessageTimer.reset();

   if(!mTopDown)
      mChatScrollTimer.reset();
}


// Check if we have any %variables% that need substituting
string ChatMessageDisplayer::substitueVars(const string &str)
{
   string s = str;      // Make working copy

   bool inside = false;

   std::size_t startPos, endPos;

   inside = false;

   for(std::size_t i = 0; i < s.length(); i++)
   {
      if(s[i] == '%')
      {
         if(!inside)    // Found beginning of variable
         {
            startPos = i + 1;
            inside = true;
         }
         else           // Found end of variable
         {
            endPos = i - startPos;
            inside = false;

            string var = s.substr(startPos, endPos);
            string val = getSubstVarVal(mGame, var);

            s.replace(startPos - 1, endPos + 2, val);

            i += val.length() - var.length() - 2;     // Make sure we don't evaluate the contents of val; i.e. no recursion
         }
      }
   }

   return s;
}


// Render any incoming player chat msgs
void ChatMessageDisplayer::render(S32 anchorPos, bool helperVisible, bool anouncementActive, F32 alpha)
{
   // Are we in the act of transitioning between one message and another?
   bool isScrolling = (mChatScrollTimer.getCurrent() > 0);  

   // Check if there any messages to display... if not, bail
   if(mFirst == mLast && !(mTopDown && isScrolling))
      return;

   S32 lineHeight = mFontSize + mFontGap;


   // Reuse this to avoid startup and breakdown costs
   static ScissorsManager scissorsManager;

   // Only need to set scissors if we're scrolling.  When not scrolling, we control the display by only showing
   // the specified number of lines; there are normally no partial lines that need vertical clipping as 
   // there are when we're scrolling.  Note also that we only clip vertically, and can ignore the horizontal.
   if(isScrolling)    
   {
      // Remember that our message list contains an extra entry that exists only for scrolling purposes.
      // We want the height of the clip window to omit this line, so we subtract 1 below.  
      S32 displayAreaHeight = (mMessages.size() - 1) * lineHeight;     
      S32 displayAreaYPos = anchorPos + (mTopDown ? displayAreaHeight : lineHeight);

      scissorsManager.enable(true, mGame, 0, displayAreaYPos - displayAreaHeight, gScreenInfo.getGameCanvasWidth(), displayAreaHeight);
   }

   // Initialize the starting rendering position.  This represents the bottom of the message rendering area, and
   // we'll work our way up as we go.  In all cases, newest messages will appear on the bottom, older ones on top.
   // Note that anchorPos reflects something different (i.e. the top or the bottom of the area) in each case.
   S32 y = anchorPos + mChatScrollTimer.getFraction() * lineHeight;

   // Advance anchor from top to the bottom of the render area.  When we are rendering at the bottom, anchorPos
   // already represents the bottom, so no additional adjustment is necessary.
   if(mTopDown)
      y += (mFirst - mLast - 1) * lineHeight;

   // Render an extra message while we're scrolling (in some cases).  Scissors will control the total vertical height.
   S32 renderExtra = 0;
   if(isScrolling)
   {
      if(mTopDown)
         renderExtra = 1;
      else if(mFull)    // Only render extra item on bottom-up if list is fully occupied
         renderExtra = 1;
   }

   // Adjust our last line if we have an announcement
   U32 last = mLast;
   if(anouncementActive)
   {
      // Render one less line if we're past the size threshold for this displayer
      if(mFirst >= (U32)mMessages.size() - 1)
         last++;

      y -= lineHeight;
   }

   // Draw message lines
   for(U32 i = mFirst; i != last - renderExtra; i--)
   {
      U32 index = i % (U32)mMessages.size();    // Handle wrapping in our message list

      glColor(mMessages[index].color, alpha); 

      drawString(UserInterface::horizMargin, y, mFontSize, mMessages[index].str.c_str());

      y -= lineHeight;
   }


   // Restore scissors settings -- only used during scrolling
   scissorsManager.disable();
}


};

