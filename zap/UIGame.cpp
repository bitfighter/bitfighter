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

#include "quickChatHelper.h"
#include "loadoutHelper.h"
#include "engineerHelper.h"
#include "TeamShuffleHelper.h"

#include "gameConnection.h"
#include "ServerGame.h"          // For gServerGame refs
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

#include "../tnl/tnlEndian.h"

#include "SDL.h"
#include "OpenglUtils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

namespace Zap
{

//GameUserInterface mGameUserInterface;

// TODO: Make these static like privateF5MessageDisplayedInGameColor!
Color gGlobalChatColor(0.9, 0.9, 0.9);
Color gTeamChatColor(Colors::green);
extern Color gCmdChatColor;


// Sizes and other things to help with positioning
static const S32 CHAT_Y_POS = 500;
static const S32 SRV_MSG_FONT_SIZE = 14;
static const S32 SRV_MSG_FONT_GAP = 4;
static const S32 CHAT_FONT_SIZE = 12;
static const S32 CHAT_FONT_GAP = 3;
static const S32 CHAT_WRAP_WIDTH = 700;            // Max width of chat messages displayed in-game
static const S32 SRV_MSG_WRAP_WIDTH = 750;


Color GameUserInterface::privateF5MessageDisplayedInGameColor(Colors::blue);


static void makeCommandCandidateList();      // Forward delcaration

// Constructor
GameUserInterface::GameUserInterface(ClientGame *game) : 
                  Parent(game), 
                  mVoiceRecorder(game),
                  mLineEditor(200),    //    lines expr  topdown   wrap width          font size          line gap
                  mServerMessageDisplayer(game, 6, true,  true,  SRV_MSG_WRAP_WIDTH, SRV_MSG_FONT_SIZE, SRV_MSG_FONT_GAP),
                  mChatMessageDisplayer1(game,  5, true,  false, CHAT_WRAP_WIDTH,    CHAT_FONT_SIZE,    CHAT_FONT_GAP),
                  mChatMessageDisplayer2(game,  5, false, false, CHAT_WRAP_WIDTH,    CHAT_FONT_SIZE,    CHAT_FONT_GAP),
                  mChatMessageDisplayer3(game, 24, false, false, CHAT_WRAP_WIDTH,    CHAT_FONT_SIZE,    CHAT_FONT_GAP)
{
   mInScoreboardMode = false;
   mFPSVisible = false;
   mHelper = NULL;
   displayInputModeChangeAlert = false;
   mMissionOverlayActive = false;

   mMessageDisplayMode = ShortTimeout;

   setMenuID(GameUI);
   enterMode(PlayMode);          // Also initializes mCurrentUIMode
   mInScoreboardMode = false;

   mQuickChatHelper = NULL;
   mLoadoutHelper = NULL;
   mEngineerHelper = NULL;
   mTeamShuffleHelper = NULL;


#if 0 //defined(TNL_OS_XBOX)
   mFPSVisible = true;
#else
   mFPSVisible = false;
#endif

   mFPSAvg = 0;
   mPingAvg = 0;
   mFrameIndex = 0;

   for(S32 i = 0; i < FPS_AVG_COUNT; i++)
   {
      mIdleTimeDelta[i] = 50;
      mPing[i] = 100;
   }

   mGotControlUpdate = false;
   mRecalcFPSTimer = 0;

   mFiring = false;
   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
   {
      mModPrimaryActivated[i] = false;
      mModSecondaryActivated[i] = false;
      mModuleDoubleTapTimer[i].setPeriod(DoubleClickTimeout);
   }

   makeCommandCandidateList();
}


// Destructor  -- only runs when we're exiting to the OS
GameUserInterface::~GameUserInterface()
{
   delete mQuickChatHelper;
   delete mLoadoutHelper;
   delete mEngineerHelper;
   delete mTeamShuffleHelper;
}


// Lazily initialize these...
QuickChatHelper *GameUserInterface::getQuickChatHelper(ClientGame *game)
{
   if(!mQuickChatHelper)
      mQuickChatHelper = new QuickChatHelper(game);

   return mQuickChatHelper;
}

   
LoadoutHelper *GameUserInterface::getLoadoutHelper(ClientGame *game)
{
   if(!mLoadoutHelper)
      mLoadoutHelper = new LoadoutHelper(game);

   return mLoadoutHelper;
}


EngineerHelper *GameUserInterface::getEngineerHelper(ClientGame *game)
{
   if(!mEngineerHelper)
      mEngineerHelper = new EngineerHelper(game);

   return mEngineerHelper;
}


TeamShuffleHelper *GameUserInterface::getTeamShuffleHelper(ClientGame *game)
{
   if(!mTeamShuffleHelper)
      mTeamShuffleHelper = new TeamShuffleHelper(game);

   return mTeamShuffleHelper;
}

// Used when ship dies while engineering
void GameUserInterface::quitEngineerHelper()
{
   if(mHelper == mEngineerHelper)
      mHelper = NULL;
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

   enterMode(PlayMode);                         // Make sure we're not in chat or loadout-select mode

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      mModPrimaryActivated[i] = false;
      mModSecondaryActivated[i] = false;
   }

   mShutdownMode = None;
}


void GameUserInterface::onReactivate()
{
   if(getGame()->isSuspended())
      unsuspendGame();

   getGame()->undelaySpawn();

   mDisableShipKeyboardInput = false;
   Cursor::disableCursor();    // Turn off cursor

   if(mCurrentChatType == NoChat)
      setBusyChatting(false);

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

   displayMessage(gCmdChatColor, stringBuffer);
}


void GameUserInterface::displaySuccessMessage(const char *format, ...)
{
   va_list args;

   va_start(args, format);
   vsnprintf(stringBuffer, sizeof(stringBuffer), format, args);
   va_end(args);

   displayMessage(Color(0.6, 1, 0.8), stringBuffer);
}


// A new server message is here!  We don't actually display anything here, despite the name...
// just add it to the list, will be displayed in render()
void GameUserInterface::displayMessagef(const Color &msgColor, const char *format, ...)
{
   va_list args;
   char message[MAX_CHAT_MSG_LENGTH]; 

   va_start(args, format);
   vsnprintf(message, sizeof(message), format, args); 
   va_end(args);
    
   displayMessage(msgColor, message);
}


void GameUserInterface::displayMessage(const Color &msgColor, const char *message)
{
   // Ignore empty message
   if(strcmp(message, "") == 0)
      return;

   mServerMessageDisplayer.onChatMessageRecieved(msgColor, message);
}


void GameUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   // Update some timers
   mShutdownTimer.update(timeDelta);
   mInputModeChangeAlertDisplayTimer.update(timeDelta);
   mWrongModeMsgDisplay.update(timeDelta);
   mProgressBarFadeTimer.update(timeDelta);
   mLevelInfoDisplayTimer.update(timeDelta);

   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
      mModuleDoubleTapTimer[i].update(timeDelta);

   // Messages
   mServerMessageDisplayer.idle(timeDelta);
   mChatMessageDisplayer1.idle(timeDelta);
   mChatMessageDisplayer2.idle(timeDelta);
   mChatMessageDisplayer3.idle(timeDelta);

   // Time to recalc FPS?
   if(mFPSVisible)        // Only bother if we're displaying the value...
   {
      if(timeDelta > mRecalcFPSTimer)
      {
         U32 sum = 0, sumping = 0;

         for(S32 i = 0; i < FPS_AVG_COUNT; i++)
         {
            sum += mIdleTimeDelta[i];
            sumping += mPing[i];
         }

         mFPSAvg = (1000 * FPS_AVG_COUNT) / F32(sum);
         mPingAvg = F32(sumping) / 32;
         mRecalcFPSTimer = 750;
      }
      else
         mRecalcFPSTimer -= timeDelta;
   }

   if(mHelper)
      mHelper->idle(timeDelta);

   mVoiceRecorder.idle(timeDelta);

   U32 indx = mFrameIndex % FPS_AVG_COUNT;
   mIdleTimeDelta[indx] = timeDelta;

   if(getGame()->getConnectionToServer())
      mPing[indx] = (U32)getGame()->getConnectionToServer()->getRoundTripTime();

   mFrameIndex++;

   // Should we move this timer over to UIGame??
   HostMenuUserInterface *ui = getUIManager()->getHostMenuUserInterface();
   if(ui->levelLoadDisplayFadeTimer.update(timeDelta))
      ui->clearLevelLoadDisplay();
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
   glColor(Colors::black);

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

   if(getGame()->isSuspended()  || getGame()->isSpawnDelayed())
   {
      renderChatMsgs();
      renderCurrentChat();
      renderSuspendedMessage();

      if(mHelper)
         mHelper->render();
   }
   else
   {
      renderReticle();              // Draw crosshairs if using mouse
      renderChatMsgs();             // Render incoming chat and server msgs
      renderCurrentChat();          // Render any chat msg user is composing
      renderLoadoutIndicators();    // Draw indicators for the various loadout items

      getUIManager()->getHostMenuUserInterface()->renderProgressListItems();  // This is the list of levels loaded while hosting

      renderProgressBar();          // This is the status bar that shows progress of loading this level

      mVoiceRecorder.render();      // This is the indicator that someone is sending a voice msg

      // Display running average FPS
      if(mFPSVisible)
      {
         glColor(Colors::white);
         drawStringf(gScreenInfo.getGameCanvasWidth() - horizMargin - 220, vertMargin, 20, "%4.1f fps | %1.0f ms", mFPSAvg, mPingAvg);
      }

      // Render QuickChat / Loadout menus
      if(mHelper)
         mHelper->render();

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


void GameUserInterface::renderSuspendedMessage()
{
   static string waitMsg[] = { "", 
                           "CAN RESPAWN",
                           "",
                           "",
                           "" };

   static string readyMsg[] = { "", 
                           "PRESS ANY",
                           "KEY TO",
                           "RESPAWN",
                           "" };

   if(getGame()->getReturnToGameDelay() != 0)
   {
      waitMsg[2] = "IN " +
                   ftos(ceil(F32(getGame()->getReturnToGameDelay()) / 1000.f)) +
                   " SECONDS";
      renderMessageBox("", "", waitMsg, 5, -30, 2);
   }
   else
      renderMessageBox("", "", readyMsg, 5, -30, 2);
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
#if 0 // TNL_OS_WIN32
      Point realMousePoint = mMousePoint;
      if(!getGame()->getSettings()->getIniSettings()->controlsRelative)
      {
         F32 len = mMousePoint.len();
         checkMousePos(gScreenInfo.getWindowWidth()  * 100 / canvasWidth,
                       gScreenInfo.getWindowHeight() * 100 / canvasHeight);

         if(len > 100)
            realMousePoint *= 100 / len;
      }
#endif
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
      F32 colors[] = {
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

static const S32 fontSize = 15;
static const S32 gapSize = 3;       // Gap between text and box

S32 gLoadoutIndicatorHeight = fontSize + gapSize * 2;


static S32 renderIndicator(S32 xPos, const char *name)
{
   S32 width = UserInterface::getStringWidth(fontSize, name);

   UserInterface::drawHollowRect(xPos, UserInterface::vertMargin, 
                                 xPos + width + 2 * gapSize, UserInterface::vertMargin + fontSize + 2 * gapSize + 1);

   // Add the weapon or module name
   UserInterface::drawString(xPos + gapSize, UserInterface::vertMargin + gapSize, fontSize, name);

   return width + 2 * gapSize;
}


// Draw weapon indicators at top of the screen, runs on client
void GameUserInterface::renderLoadoutIndicators()
{
   if(!getGame()->getSettings()->getIniSettings()->showWeaponIndicators)      // If we're not drawing them, we've got nothing to do
      return;

   if(!getGame()->getConnectionToServer())     // Can happen when first joining a game.  This was XelloBlue's crash...
      return;

   Ship *localShip = dynamic_cast<Ship *>(getGame()->getConnectionToServer()->getControlObject());
   if(!localShip)
      return;

   static const Color *INDICATOR_INACTIVE_COLOR = &Colors::green80;      
   static const Color *INDICATOR_ACTIVE_COLOR = &Colors::red80;        
   static const Color *INDICATOR_PASSIVE_COLOR = &Colors::yellow;

   U32 xPos = horizMargin;

   // First, the weapons
   for(U32 i = 0; i < (U32)ShipWeaponCount; i++)
   {
      glColor(i == localShip->mActiveWeaponIndx ? INDICATOR_ACTIVE_COLOR : INDICATOR_INACTIVE_COLOR);

      S32 width = renderIndicator(xPos, GameWeapon::weaponInfo[localShip->getWeapon(i)].name.getString());

      xPos += width + gapSize;
   }

   xPos += 20;    // Small horizontal gap to seperate the weapon indicators from the module indicators

   // Next, loadout modules
   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
   {
      if(gModuleInfo[localShip->getModule(i)].getPrimaryUseType() != ModulePrimaryUseActive)
      {
         if(gModuleInfo[localShip->getModule(i)].getPrimaryUseType() == ModulePrimaryUseHybrid &&
               localShip->isModulePrimaryActive(localShip->getModule(i)))
            glColor(INDICATOR_ACTIVE_COLOR);
         else
            glColor(INDICATOR_PASSIVE_COLOR);
      }
      else if(localShip->isModulePrimaryActive(localShip->getModule(i)))
         glColor(INDICATOR_ACTIVE_COLOR);
      else 
         glColor(INDICATOR_INACTIVE_COLOR);

      // Always change to orange if module secondary is fired
      if(gModuleInfo[localShip->getModule(i)].hasSecondary() &&
            localShip->isModuleSecondaryActive(localShip->getModule(i)))
         glColor(Colors::orange67);

      S32 width = renderIndicator(xPos, getGame()->getModuleInfo(localShip->getModule(i))->getName());

      xPos += width + gapSize;
   }
}


bool GameUserInterface::isCmdChat()
{
   return mLineEditor.at(0) == '/' || mCurrentChatType == CmdChat;
}


void GameUserInterface::onMouseDragged()
{
   TNLAssert(false, "Is this ever called?");
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


UIMode GameUserInterface::getUIMode()
{
   return mCurrentUIMode;
}


// Enter QuickChat, Loadout, or Engineer mode
void GameUserInterface::enterMode(UIMode mode)
{
   TNLAssert(mode != ChatMode, "Should not be called to enter chat mode!");

   playBoop();
   mCurrentChatType = NoChat;

   if(mode == QuickChatMode)
      mHelper = getQuickChatHelper(getGame());
   else if(mode == LoadoutMode)
      mHelper = getLoadoutHelper(getGame());
   else if(mode == EngineerMode)
      mHelper = getEngineerHelper(getGame());
   else if(mode == TeamShuffleMode)
      mHelper = getTeamShuffleHelper(getGame());
   else if(mode == ChatMode)
      mHelper = NULL;
   else if(mode == PlayMode)
   {
      cancelChat();
      mHelper = NULL;

      getGame()->unsuspendGame();  
   }
   else
      TNLAssert(false, "Invalid mode!");

   mCurrentUIMode = mode;

   if(mHelper)
      mHelper->onMenuShow();
}


bool GameUserInterface::isHelperActive()
{
   return mHelper != NULL;
}


bool GameUserInterface::isChatting()
{
   return mCurrentChatType != NoChat;
}


void GameUserInterface::renderEngineeredItemDeploymentMarker(Ship *ship)
{
   if(mHelper && mHelper->isEngineerHelper())
   {
      TNLAssert(mEngineerHelper, "Engineer helper does not exist!");
      mEngineerHelper->renderDeploymentMarker(ship);
   }
}


// Runs on client
void GameUserInterface::dropItem()
{
   if(!getGame()->getConnectionToServer())
      return;

   Ship *ship = dynamic_cast<Ship *>(getGame()->getConnectionToServer()->getControlObject());
   if(!ship)
      return;

   GameType *gt = getGame()->getGameType();
   if(!gt)
      return;

   if(!gt->isCarryingItems(ship))
   {
      displayMessage(Colors::paleRed, "You don't have any items to drop!");
      return;
   }

   gt->c2sDropItem();
}


// Send a message to the server that we are (or are not) busy chatting
void GameUserInterface::setBusyChatting(bool isBusy)
{
   GameConnection *conn = getGame()->getConnectionToServer();
   if(conn)
      conn->c2sSetIsBusy(isBusy);
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


void GameUserInterface::onTextInput(char ascii)
{
   // Pass the key on to the console for processing
   if(gConsole.onKeyDown(ascii))
      return;

   // Make sure we have a chat box open
   if(mCurrentChatType != NoChat)
      // Append any keys to the chat message
      if(ascii)
         // Protect against crashes while game is initializing (because we look at the ship for the player's name)
         if(getGame()->getConnectionToServer())     // getGame() cannot return NULL here
            mLineEditor.addChar(ascii);
}


// Key pressed --> take action!
// Handles all keypress events, including mouse clicks and controller button presses
bool GameUserInterface::onKeyDown(InputCode inputCode)
{
   GameSettings *settings = getGame()->getSettings();

   bool startedInHelper = mHelper || mCurrentChatType != NoChat;

   // Kind of hacky, but this will unsuspend and swallow the keystroke, which is what we want
   if(!startedInHelper && (getGame()->isSuspended() || getGame()->isSpawnDelayed()))
   {
      getGame()->undelaySpawn();
      if(inputCode != KEY_ESCAPE)  // Lagged out and can't un-idle to bring up the menu?
         return true;
   }

   if(checkInputCode(settings, InputCodeManager::BINDING_OUTGAMECHAT, inputCode))
      setBusyChatting(true);

   if(!startedInHelper || (!mHelper && mCurrentChatType == NoChat))
      getGame()->undelaySpawn();

   if(Parent::onKeyDown(inputCode))
      return true;

   else if(gConsole.onKeyDown(inputCode))   // Pass the key on to the console for processing
      return true;

   else if(checkInputCode(settings, InputCodeManager::BINDING_HELP, inputCode))   // Turn on help screen
   {
      playBoop();
      setBusyChatting(true);

      // If we have a helper, let that determine what happens when the help key is pressed.  Otherwise, show help normally.
      if(mHelper)
         mHelper->activateHelp(getUIManager());
      else if(mCurrentChatType == CmdChat)
         getUIManager()->getInstructionsUserInterface()->activatePage(InstructionsUserInterface::InstructionAdvancedCommands);
      else
         getUIManager()->activate(InstructionsUI);

      return true;
   }

   // Ctrl-/ toggles console window for the moment
   // Only open when not in any special mode.
   else if(!mHelper && inputCode == KEY_SLASH && InputCodeManager::checkModifier(KEY_CTRL))
   {
      if(gConsole.isOk())
         gConsole.toggleVisibility();
      // else... what?

      return true;
   }
   else if(checkInputCode(settings, InputCodeManager::BINDING_MISSION, inputCode))
   {
      mMissionOverlayActive = true;
      getUIManager()->getGameUserInterface()->clearLevelInfoDisplayTimer();    // Clear level-start display if user hits F2

      return true;
   }
   else if(inputCode == KEY_M && InputCodeManager::checkModifier(KEY_CTRL))    // Ctrl-M, for now, to cycle through message dispaly modes
   {
      toggleChatDisplayMode();
      return true;
   }
   else if(mHelper && mHelper->processInputCode(inputCode))   // Will return true if key was processed
   {
      // Experimental, to keep ship from moving after entering a quick chat that has the same shortcut as a movement key
      InputCodeManager::setState(inputCode, false);
      return true;
   }
   else 
   {
      // If we're in play mode, and we apply the engineer module, then we can handle that locally by throwing up a menu or message
      if(!mHelper && mCurrentChatType == NoChat)
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
                  enterMode(EngineerMode);

               return true;
            }
         }
      }

      bool handled = false;
      if(!gConsole.isVisible())
      {
         if(mCurrentChatType == NoChat)
            handled = processPlayModeKey(inputCode);
         else
            handled = processChatModeKey(inputCode);
      }

      return handled;
   }

   return false;
}


// Helper function...
static void saveLoadoutPreset(ClientGame *game, S32 slot)
{
   GameConnection *conn = game->getConnectionToServer();
   if(!conn)
      return;

   Ship *ship = dynamic_cast<Ship *>(conn->getControlObject());
   if(!ship)
      return;

   Vector<U8> loadout(ShipModuleCount + ShipWeaponCount);
   ship->getLoadout(loadout);

   game->getSettings()->setLoadoutPreset(slot, loadout);
   game->displaySuccessMessage(("Current loadout saved as preset " + itos(slot + 1)).c_str());
}


static void loadLoadoutPreset(ClientGame *game, S32 slot)
{
   Vector<U8> loadout(ShipModuleCount + ShipWeaponCount);      // Define it
   game->getSettings()->getLoadoutPreset(slot, loadout);       // Fill it

   if(loadout.size() == 0)    // Looks like the preset might be empty!
   {
      string msg = "Preset " + itos(slot + 1) + " is undefined -- to define it, try Ctrl-" + itos(slot + 1);
      game->displayErrorMessage(msg.c_str());
      return;
   }

   GameType *gameType = game->getGameType();
   if(!gameType)
      return;
   
   string err = gameType->validateLoadout(loadout);
   
   if(err != "")
   {
      game->displayErrorMessage((err + "; loadout not set").c_str());
      return;
   }

   GameConnection *conn = game->getConnectionToServer();
   if(!conn)
      return;

   if(game->getSettings()->getIniSettings()->verboseHelpMessages)
      game->displayShipDesignChangedMessage(loadout, "Preset same as the current design");

   // Request loadout even if it was the same -- if I have loadout A, with on-deck loadout B, and I enter a new loadout
   // that matches A, it would be better to have loadout remain unchanged if I entered a loadout zone.
   // Tell server loadout has changed.  Server will activate it when we enter a loadout zone.
   conn->c2sRequestLoadout(loadout);    
}


bool checkInputCode(InputCode codeUserEntered, InputCode codeToActivateCommand)
{
   // Look for some common synonyms, like keypad_4 and 4
   return codeUserEntered == codeToActivateCommand;
}


// Can only get here if we're not in chat mode
bool GameUserInterface::processPlayModeKey(InputCode inputCode)
{
   GameSettings *settings = getGame()->getSettings();
   //InputMode inputMode = getGame()->getSettings()->getIniSettings()->inputMode;

   // The following keys are allowed in both play mode and in loadout or
   // engineering menu modes if not used in the loadout menu above
   if(inputCode == KEY_CLOSEBRACKET && InputCodeManager::checkModifier(KEY_ALT))          // Alt-] advances bots by one step if frozen
      EventManager::get()->addSteps(1);
   else if(inputCode == KEY_CLOSEBRACKET && InputCodeManager::checkModifier(KEY_CTRL))    // Ctrl-] advances bots by 10 steps if frozen
      EventManager::get()->addSteps(10);
   else if(inputCode == KEY_1 && InputCodeManager::checkModifier(KEY_CTRL))               // Ctrl-1 saves loadout preset in first slot
      saveLoadoutPreset(getGame(), 0);
   else if(inputCode == KEY_1 && InputCodeManager::checkModifier(KEY_ALT))                // Alt-1 loads preset from first slot
      loadLoadoutPreset(getGame(), 0);
   else if(inputCode == KEY_2 && InputCodeManager::checkModifier(KEY_CTRL))              
      saveLoadoutPreset(getGame(), 1);
   else if(inputCode == KEY_2 && InputCodeManager::checkModifier(KEY_ALT))             
      loadLoadoutPreset(getGame(), 1);
   else if(inputCode == KEY_3 && InputCodeManager::checkModifier(KEY_CTRL))              
      saveLoadoutPreset(getGame(), 2);
   else if(inputCode == KEY_3 && InputCodeManager::checkModifier(KEY_ALT))             
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
      mFPSVisible = !mFPSVisible;
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
         setBusyChatting(true);
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
   else if(!mHelper)    // The following keys are only allowed in PlayMode
   {
      if(checkInputCode(settings, InputCodeManager::BINDING_TEAMCHAT, inputCode))          // Start entering a team chat msg
      {
         mCurrentChatType = TeamChat;
         setBusyChatting(true);
      }
      else if(checkInputCode(settings, InputCodeManager::BINDING_GLOBCHAT, inputCode))     // Start entering a global chat msg
      {
         mCurrentChatType = GlobalChat;
         setBusyChatting(true);
      }
      else if(checkInputCode(settings, InputCodeManager::BINDING_CMDCHAT, inputCode))      // Start entering a command
      {
         mCurrentChatType = CmdChat;
         setBusyChatting(true);
      }
      else if(checkInputCode(settings, InputCodeManager::BINDING_QUICKCHAT, inputCode))
         enterMode(QuickChatMode);
      else if(checkInputCode(settings, InputCodeManager::BINDING_LOADOUT, inputCode))
         enterMode(LoadoutMode);
      else if(checkInputCode(settings, InputCodeManager::BINDING_DROPITEM, inputCode))
         dropItem();
      // Check if the user is trying to use keyboard to move when in joystick mode
      else if(settings->getInputCodeManager()->getInputMode() == InputModeJoystick)      
         if(checkInputCode(settings, InputCodeManager::BINDING_UP,    inputCode) ||
            checkInputCode(settings, InputCodeManager::BINDING_DOWN,  inputCode) ||
            checkInputCode(settings, InputCodeManager::BINDING_LEFT,  inputCode) ||
            checkInputCode(settings, InputCodeManager::BINDING_RIGHT, inputCode))
               mWrongModeMsgDisplay.reset(WRONG_MODE_MSG_DISPLAY_TIME);
   }
   else
      return false;

   return true;
}


// Returns a pointer of string of chars, after "count" number of args
static const char *findPointerOfArg(const char *message, S32 count)
{
   S32 spacecount = 0;
   S32 cur = 0;
   char prevchar = 0;

   // Message needs to include everything including multiple spaces.  Message starts after second space.
   while(message[cur] != '\0' && spacecount != count)
   {
      if(message[cur] == ' ' && prevchar != ' ')
         spacecount++;        // Double space does not count as a seperate parameter
      prevchar = message[cur];
      cur++;
   }
   return &message[cur];
}


// static method
void GameUserInterface::addTimeHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(words.size() < 2 || words[1] == "")
      game->displayErrorMessage("!!! Need to supply a time (in minutes)");
   else
   {
      U8 mins;    // Use U8 to limit number of mins that can be added, while nominally having no limit!
                  // Parse 2nd arg -- if first digit isn't a number, user probably screwed up.
                  // atoi will return 0, but this probably isn't what the user wanted.

      bool err = false;
      if(words[1][0] >= '0' && words[1][0] <= '9')
         mins = atoi(words[1].c_str());
      else
         err = true;

      if(err || mins == 0)
         game->displayErrorMessage("!!! Invalid value... game time not changed");
      else
      {
         if(game->getGameType())
         {
            game->displayMessage(gCmdChatColor, "Extended game by %d minute%s", mins, (mins == 1) ? "" : "s");
            game->getGameType()->addTime(mins * 60 * 1000);
         }
      }
   }
}


void GameUserInterface::sVolHandler(const Vector<string> &words)
{
   setVolume(SfxVolumeType, words);
}

void GameUserInterface::mVolHandler(const Vector<string> &words)
{
   setVolume(MusicVolumeType, words);
}

void GameUserInterface::vVolHandler(const Vector<string> &words)
{
   setVolume(VoiceVolumeType, words);
}

void GameUserInterface::servVolHandler(const Vector<string> &words)
{
   setVolume(ServerAlertVolumeType, words);
}


void GameUserInterface::mNextHandler(const Vector<string> &words)
{
   SoundSystem::playNextTrack();
}


void GameUserInterface::mPrevHandler(const Vector<string> &words)
{
   SoundSystem::playPrevTrack();
}


void GameUserInterface::getMapHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   GameConnection *gc = game->getConnectionToServer();

   if(gc->isLocalConnection())
      game->displayErrorMessage("!!! Can't download levels from a local server");
   else
   {
      string filename;

      if(words.size() > 1 && words[1] != "")
         filename = words[1];
      else
         filename = "downloaded_" + makeFilenameFromString(game->getGameType() ?
               game->getGameType()->getLevelName()->getString() : "Level");

      // Add an extension if needed
      if(filename.find(".") == string::npos)
         filename += ".level";

      game->setRemoteLevelDownloadFilename(filename);

      gc->c2sRequestCurrentLevel();
   }
}


void GameUserInterface::nextLevelHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasLevelChange("!!! You don't have permission to change levels"))
      game->getConnectionToServer()->c2sRequestLevelChange(ServerGame::NEXT_LEVEL, false);
}


void GameUserInterface::prevLevelHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasLevelChange("!!! You don't have permission to change levels"))
      game->getConnectionToServer()->c2sRequestLevelChange(ServerGame::PREVIOUS_LEVEL, false);
}


void GameUserInterface::restartLevelHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasLevelChange("!!! You don't have permission to change levels"))
      game->getConnectionToServer()->c2sRequestLevelChange(ServerGame::REPLAY_LEVEL, false);
}

void GameUserInterface::randomLevelHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasLevelChange("!!! You don't have permission to change levels"))
      game->getConnectionToServer()->c2sRequestLevelChange(ServerGame::RANDOM_LEVEL, false);
}


void GameUserInterface::shutdownServerHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! You don't have permission to shut the server down"))
   {
      U16 time = 0;
      bool timefound = true;
      string reason;

      if(words.size() > 1)
         time = (U16) atoi(words[1].c_str());
      if(time <= 0)
      {
         time = 10;
         timefound = false;
      }

      S32 first = timefound ? 2 : 1;
      for(S32 i = first; i < words.size(); i++)
      {
         if(i != first)
            reason = reason + " ";
         reason = reason + words[i];
      }

      game->getConnectionToServer()->c2sRequestShutdown(time, reason.c_str());
   }
}


void GameUserInterface::kickPlayerHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! You don't have permission to kick players"))
   {
      if(words.size() < 2 || words[1] == "")
         game->displayErrorMessage("!!! Need to specify who to kick");
      else
      {
         // Did user provide a valid, known name?
         string name = words[1];
         
         if(!game->checkName(name))
         {
            game->displayErrorMessage("!!! Could not find player: %s", words[1].c_str());
            return;
         }

         if(game->getGameType())
            game->getGameType()->c2sKickPlayer(words[1].c_str());
      }
   }
}


void GameUserInterface::submitPassHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(words.size() < 2)
      return;

   GameConnection *conn = game->getConnectionToServer();
   conn->submitPassword(words[1].c_str());
}


void GameUserInterface::showCoordsHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   game->toggleShowingShipCoords();
}


void GameUserInterface::showZonesHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(!(gServerGame))// && gServerGame->isTestServer()))  sam: problem with not being able to test from editor due to editor crashing and loading improperly...
      game->displayErrorMessage("!!! Zones can only be displayed on a local host");
   else
      game->toggleShowingMeshZones();
}


extern bool showDebugBots;  // in game.cpp

void GameUserInterface::showPathsHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(!(gServerGame && gServerGame->isTestServer())) 
      game->displayErrorMessage("!!! Robots can only be shown on a test server");
   else
      showDebugBots = !showDebugBots;
}


void GameUserInterface::pauseBotsHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(!(gServerGame && gServerGame->isTestServer())) 
      game->displayErrorMessage("!!! Robots can only be frozen on a test server");
   else
      EventManager::get()->togglePauseStatus();
}


void GameUserInterface::stepBotsHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(!(gServerGame && gServerGame->isTestServer())) 
      game->displayErrorMessage("!!! Robots can only be stepped on a test server");
   else
   {
      S32 steps = words.size() > 1 ? atoi(words[1].c_str()) : 1;
      EventManager::get()->addSteps(steps);
   }
}


void GameUserInterface::setAdminPassHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! You don't have permission to set the admin password"))
      game->changePassword(GameConnection::AdminPassword, words, true);
}


void GameUserInterface::setServerPassHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! You don't have permission to set the server password"))
      game->changePassword(GameConnection::ServerPassword, words, false);
}


void GameUserInterface::setLevPassHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! You don't have permission to set the level change password"))
      game->changePassword(GameConnection::LevelChangePassword, words, false);
}


void GameUserInterface::setServerNameHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! You don't have permission to set the server name"))
      game->changeServerParam(GameConnection::ServerName, words);
}


void GameUserInterface::setServerDescrHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! You don't have permission to set the server description"))
      game->changeServerParam(GameConnection::ServerDescr, words);
}


void GameUserInterface::setLevelDirHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! You don't have permission to set the leveldir param"))
      game->changeServerParam(GameConnection::LevelDir, words);
}


void GameUserInterface::deleteCurrentLevelHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();

   if(game->hasAdmin("!!! You don't have permission to delete the current level"))
      game->changeServerParam(GameConnection::DeleteLevel, words);    // handles deletes too
}


void GameUserInterface::idleHandler(const Vector<string> &words)
{
   getGame()->requestSpawnDelayed();
}


void GameUserInterface::suspendHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();

   if(game->getPlayerCount() > 1)
      game->displayErrorMessage("!!! Can't suspend when others are playing");
   else
      game->getUIManager()->getGameUserInterface()->suspendGame();
}


extern S32 LOADOUT_PRESETS;

void GameUserInterface::showPresetsHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   Vector<U8> preset(ShipModuleCount + ShipWeaponCount);

   for(S32 i = 0; i < LOADOUT_PRESETS; i++)
   {
      preset.clear();
      game->getSettings()->getLoadoutPreset(i, preset);

      string loadoutStr = Ship::loadoutToString(preset);
      
      string display;
      
      if(loadoutStr != "")
         display = "Preset " + itos(i + 1) + ": " + replaceString(loadoutStr, ",", "; ");
      else
         display = "Preset " + itos(i + 1) + " is undefined";

      game->displayMessage(Colors::cyan, display.c_str());
   }
}


void GameUserInterface::lineWidthHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   F32 linewidth;
   if(words.size() < 2 || words[1] == "")
      game->displayErrorMessage("!!! Need to supply line width");
   else
   {
      linewidth = (F32)atof(words[1].c_str());
      if(linewidth < 0.125f)
         linewidth = 0.125f;

      gDefaultLineWidth = linewidth;
      gLineWidth1 = linewidth * 0.5f;
      gLineWidth3 = linewidth * 1.5f;
      gLineWidth4 = linewidth * 2;

      glLineWidth(gDefaultLineWidth);    //make this change happen instantly
   }
}


void GameUserInterface::maxFpsHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   S32 number = words.size() > 1 ? atoi(words[1].c_str()) : 0;

   if(number < 1)                              // Don't allow zero or negative numbers
      game->displayErrorMessage("!!! Usage: /maxfps <frame rate>, default = 100");
   else
      game->getSettings()->getIniSettings()->maxFPS = number;
}


void GameUserInterface::lagHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   U32 sendLag  = words.size() > 1 ? atoi(words[1].c_str()) : 0;
   F32 sendLoss = words.size() > 2 ? atof(words[2].c_str()) : 0;

   U32 receiveLag;
   F32 receiveLoss = sendLoss;

   static const U32 MaxLag = 5000;

   if(sendLag > MaxLag)
   {
      game->displayErrorMessage("!!! Send lag too high or invalid");
      return;
   }
   if(sendLoss < 0 || sendLoss > 100)         // Percent range
   {
      game->displayErrorMessage("!!! Send packet loss must be between 0 and 100 percent");
      return;
   }

   if(words.size() > 3)
   {
      receiveLag = atoi(words[3].c_str());
      if(words.size() > 4)
         receiveLoss = atof(words[4].c_str());
      if(receiveLoss < 0 || receiveLoss > 100)         // Percent range
      {
         game->displayErrorMessage("!!! Receive packet loss must be between 0 and 100 percent");
         return;
      }
      if(receiveLag > MaxLag)
      {
         game->displayErrorMessage("!!! Receive lag too high or invalid");
         return;
      }
   }
   else
   {
      receiveLag = (sendLag + 1) / 2;
      sendLag /= 2;
   }

   getGame()->getConnectionToServer()->setSimulatedNetParams(sendLoss / 100, sendLag, receiveLoss / 100, receiveLag);
}


void GameUserInterface::clearCacheHandler(const Vector<string> &words)
{
   if(getGame()->hasAdmin("!!! Need admin permissions"))

   if(getGame()->getGameType())
         getGame()->getGameType()->c2sClearScriptCache();
}


void GameUserInterface::pmHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(words.size() < 3)
      game->displayErrorMessage("!!! Usage: /pm <player name> <message>");
   else
   {
      if(!game->checkName(words[1]))
         game->displayErrorMessage("!!! Unknown name: %s", words[1].c_str());
      else
      {
         S32 argCount = 2 + countCharInString(words[1], ' ');  // Set pointer after 2 args + number of spaces in player name
         const char *message = game->getUIManager()->getGameUserInterface()->mLineEditor.c_str();      // Get the original line
         message = findPointerOfArg(message, argCount);        // Get the rest of the message

         GameType *gt = game->getGameType();

         if(gt)
            gt->c2sSendChatPM(words[1], message);
      }
   }
}


void GameUserInterface::muteHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(words.size() < 2)
      game->displayErrorMessage("!!! Usage: /mute <player name>");
   else
   {
      if(!game->checkName(words[1]))
         game->displayErrorMessage("!!! Unknown name: %s", words[1].c_str());

      // Un-mute if already on the list
      else if(game->isOnMuteList(words[1]))
      {
         game->removeFromMuteList(words[1]);
         game->displaySuccessMessage("Player %s has been un-muted", words[1].c_str());
      }

      // Mute!
      else
      {
         game->addToMuteList(words[1]);
         game->displaySuccessMessage("Player %s has been muted", words[1].c_str());
      }
   }
}


void GameUserInterface::voiceMuteHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(words.size() < 2)
      game->displayErrorMessage("!!! Usage: /vmute <player name>");
   else
   {
      if(!game->checkName(words[1]))
         game->displayErrorMessage("!!! Unknown name: %s", words[1].c_str());

      // Un-mute if already on the list
      else if(game->isOnVoiceMuteList(words[1]))
      {
         game->removeFromVoiceMuteList(words[1]);
         game->displaySuccessMessage("Voice for %s has been un-muted", words[1].c_str());
      }

      // Mute!
      else
      {
         game->addToVoiceMuteList(words[1]);
         game->displaySuccessMessage("Voice for %s has been muted", words[1].c_str());
      }
   }
}


void GameUserInterface::setTimeHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasLevelChange("!!! Need level change permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Enter time in minutes");
         return;
      }

      S32 timeMillis = S32(60 * 1000 * atof(words[1].c_str()));      // Reminder: We'll have problems if game time rises above S32_MAX

      if(timeMillis < 0 || (timeMillis == 0 && stricmp(words[1].c_str(), "0") && stricmp(words[1].c_str(), "unlim")) )  // 0 --> unlimited
      {
         game->displayErrorMessage("!!! Invalid value... game time not changed");
         return;
      }

      if(game->getGameType())
         game->getGameType()->c2sSetTime(timeMillis);
   }
}


void GameUserInterface::setWinningScoreHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasLevelChange("!!! Need level change permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Enter winning score limit");
         return;
      }

      S32 score = atoi(words[1].c_str());

      if(score <= 0)    // i.e. score is invalid
      {
         game->displayErrorMessage("!!! Invalid score... winning score not changed");
         return;
      }

      if(game->getGameType())
      {
         if(game->getGameType()->getGameTypeId() == CoreGame)
            game->displayErrorMessage("!!! Cannot change score in Core game type");
         else
            game->getGameType()->c2sSetWinningScore(score);
      }
   }
}


void GameUserInterface::resetScoreHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasLevelChange("!!! Need level change permission"))
   {
      if(game->getGameType())
      {
         if(game->getGameType()->getGameTypeId() == CoreGame)
            game->displayErrorMessage("!!! Cannot change score in Core game type");
         else
            game->getGameType()->c2sResetScore();
      }
   }
}


static bool fixupArgs(ClientGame *game, Vector<StringTableEntry> &args)
{
   // c2sAddBot expects the args is a slightly different order than what we have; it wants team first, then bot name, then bot args
   // However, we want users to be able to enter the bot name first, followed by an optional team.
   // If the user specifies a team name as the 2nd arg, translate that into a team number.
   // If the first arg is a string and there is a second arg, switch them.  If first arg is a string, and there is no second arg,
   // insert the NO_TEAM arg.  If first arg is numeric, hope user entered a team index first, and do not switch the args.
   // Normal arg order is bot name, team, bot args

   // First thing is to try to translate the 2nd arg into a team number
   if(args.size() >= 2 && !isInteger(args[1].getString()))
   {
      S32 teamIndex = game->getTeamIndexFromTeamName(args[1].getString());
      if(teamIndex == NO_TEAM)
      {
         game->displayErrorMessage("!!! Invalid team specified");
         return false;
      }

      args[1] = itos(teamIndex);
   }

   bool firstArgIsInt  = args.size() >= 1 && isInteger(args[0].getString());
   
   if(firstArgIsInt)          // If first arg is numeric, hope user entered a team index first, and do not switch the args.
      return true;

   if(args.size() >= 2)       // If the first arg is a string and there is a second arg, switch them.
   {
      StringTableEntry temp = args[0];
      args[0] = args[1];
      args[1] = temp;
   }
   else if(args.size() == 1)  // If first arg is a string, and there is no second arg, insert the NO_TEAM arg.
   {
      args.push_back(args[0]);
      args[0] = itos(NO_TEAM).c_str();
   }

   return true;
}


void GameUserInterface::addBotHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();

   if(game->hasLevelChange("!!! Need level change permissions to add a bot"))
   {
      // Build args by skipping first word (the command)
      Vector<StringTableEntry> args;
      for(S32 i = 1; i < words.size(); i++)
         args.push_back(StringTableEntry(words[i]));

      if(!fixupArgs(game, args))    // Reorder args for c2sAddBot, translate team names to indices, and do a little checking
         return;     

      if(game->getGameType())
         game->getGameType()->c2sAddBot(args);
   }
}


void GameUserInterface::addBotsHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasLevelChange("!!! Need level change permissions to add a bots"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Specify number of bots to add");
         return;
      }

      S32 count = 0;
      count = atoi(words[1].c_str());

      if(count <= 0 || count > 1000)
      {
         game->displayErrorMessage("!!! Invalid number of bots to add");
         return;
      }

      // Build args by skipping first two words (the command, and count)
      Vector<StringTableEntry> args;
      for(S32 i = 2; i < words.size(); i++)
         args.push_back(StringTableEntry(words[i]));

      if(!fixupArgs(game, args))        // Reorder args for c2sAddBot translate team names to indices
         return;

      if(game->getGameType())
         game->getGameType()->c2sAddBots(count, args);
   }
}


void GameUserInterface::kickBotHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasLevelChange("!!! Need level change permissions to kick a bot"))
   {
      if(game->getGameType())
         game->getGameType()->c2sKickBot();
   }
}


void GameUserInterface::kickBotsHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasLevelChange("!!! Need level change permissions to kick all bots"))
   {
      if(game->getGameType())
         game->getGameType()->c2sKickBots();
   }
}


void GameUserInterface::showBotsHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->getGameType())
      game->getGameType()->c2sShowBots();
}


void GameUserInterface::setMaxBotsHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! Need admin permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Enter maximum number of bots");
         return;
      }

      S32 count = 0;
      count = atoi(words[1].c_str());

      if(count <= 0)
      {
         game->displayErrorMessage("!!! Invalid number of bots");
         return;
      }

      if(game->getGameType())
         game->getGameType()->c2sSetMaxBots(count);
   }
}


void GameUserInterface::shuffleTeams(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! You don't have permissions to shuffle the teams"))
   {
      if(game->getTeamCount() < 2)
      {
         game->displayErrorMessage("!!! Two or more teams required to shuffle");
         return;
      }

      game->getUIManager()->getGameUserInterface()->enterMode(TeamShuffleMode);
   }
}


void GameUserInterface::banPlayerHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! Need admin permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! /ban <player name> [duration in minutes]");
         return;
      }

      ClientInfo *bannedClientInfo = game->findClientInfo(words[1].c_str());

      if(!bannedClientInfo)
      {
         game->displayErrorMessage("!!! Player name not found");
         return;
      }

      if(bannedClientInfo->isRobot())
      {
         game->displayErrorMessage("!!! Cannot ban robots, you silly fool!");
         return;
      }

      if(bannedClientInfo->isAdmin())
      {
         game->displayErrorMessage("!!! Cannot ban an admin");
         return;
      }

      S32 banDuration = 0;
      if (words.size() > 2)
         banDuration = atoi(words[2].c_str());

      if(game->getGameType())
         game->getGameType()->c2sBanPlayer(words[1].c_str(), banDuration);
   }
}


void GameUserInterface::banIpHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! Need admin permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! /banip <player name> [duration in minutes]");
         return;
      }

      Address ipAddress(words[1].c_str());

      if(!ipAddress.isValid())
      {
         game->displayErrorMessage("!!! Invalid IP address to ban");
         return;
      }

      S32 banDuration = 0;
      if (words.size() > 2)
         banDuration = atoi(words[2].c_str());

      if(game->getGameType())
         game->getGameType()->c2sBanIp(words[1].c_str(), banDuration);
   }
}


void GameUserInterface::renamePlayerHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! Need admin permission"))
   {
      if(words.size() < 3)
      {
         game->displayErrorMessage("!!! /rename <from name> <to name>");
         return;
      }

      ClientInfo *clientInfo = game->findClientInfo(words[1].c_str());

      if(!clientInfo)
      {
         game->displayErrorMessage("!!! Player name not found");
         return;
      }

      if(clientInfo->isAuthenticated())
      {
         game->displayErrorMessage("!!! Cannot rename authenticated players");
         return;
      }

      if(game->getGameType())
         game->getGameType()->c2sRenamePlayer(words[1].c_str(), words[2].c_str());
   }
}


void GameUserInterface::globalMuteHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   if(game->hasAdmin("!!! Need admin permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Enter player name");
         return;
      }

      ClientInfo *clientInfo = game->findClientInfo(words[1].c_str());

      if(!clientInfo)
      {
         game->displayErrorMessage("!!! Player name not found");
         return;
      }

      if(clientInfo->isAdmin())
      {
         game->displayErrorMessage("!!! Cannot mute an admin");
         return;
      }

      if(game->getGameType())
         game->getGameType()->c2sGlobalMutePlayer(words[1].c_str());
   }
}


// Use this method when you need to keep client/server compatibility between bitfighter
// versions (e.g. 015 -> 015a)
// If you are working on a new version (e.g. 016), then create an appropriate c2s handler
// function
void GameUserInterface::serverCommandHandler(const Vector<string> &words)
{
   ClientGame *game = getGame();
   GameType *gameType = game->getGameType();

   if(gameType)
   {
      Vector<StringPtr> args;

      for(S32 i = 1; i < words.size(); i++)
         args.push_back(StringPtr(words[i]));

      gameType->c2sSendCommand(StringTableEntry(words[0], false), args);
   }
}


CommandInfo chatCmds[] = {   
   //  cmdName          cmdCallback                 cmdArgInfo cmdArgCount   helpCategory helpGroup lines,  helpArgString            helpTextString
   { "password",&GameUserInterface::submitPassHandler,{ STR },       1,      ADV_COMMANDS,     0,     1,    {"<password>"},         "Request admin or level change permissions"  },
   { "servvol", &GameUserInterface::servVolHandler,   { xINT },      1,      ADV_COMMANDS,     0,     1,    {"<0-10>"},             "Set volume of server"  },
   { "getmap",  &GameUserInterface::getMapHandler,    { STR },       1,      ADV_COMMANDS,     1,     1,    {"[file]"},             "Save currently playing level in [file], if allowed" },
   { "idle",    &GameUserInterface::idleHandler,      {  },          0,      ADV_COMMANDS,     1,     1,    {  },                   "Place client in idle mode (AFK)" },
   { "suspend", &GameUserInterface::suspendHandler,   {  },          0,      ADV_COMMANDS,     1,     1,    {  },                   "Place server on hold while waiting for players" },
   { "pm",      &GameUserInterface::pmHandler,        { NAME, STR }, 2,      ADV_COMMANDS,     1,     1,    {"<name>","<message>"}, "Send private message to player" },
   { "mute",    &GameUserInterface::muteHandler,      { NAME },      1,      ADV_COMMANDS,     1,     1,    {"<name>"},             "Toggle hiding chat messages from <name>" },
   { "vmute",   &GameUserInterface::voiceMuteHandler, { NAME },      1,      ADV_COMMANDS,     1,     1,    {"<name>"},             "Toggle muting voice chat from <name>" },

   { "mvol",    &GameUserInterface::mVolHandler,      { xINT },      1,      SOUND_COMMANDS,   2,     1,    {"<0-10>"},             "Set music volume"      },
   { "svol",    &GameUserInterface::sVolHandler,      { xINT },      1,      SOUND_COMMANDS,   2,     1,    {"<0-10>"},             "Set SFX volume"        },
   { "vvol",    &GameUserInterface::vVolHandler,      { xINT },      1,      SOUND_COMMANDS,   2,     1,    {"<0-10>"},             "Set voice chat volume" },
   { "mnext",   &GameUserInterface::mNextHandler,     {  },          0,      SOUND_COMMANDS,   2,     1,    {  },                   "Play next track in the music list" },
   { "mprev",   &GameUserInterface::mPrevHandler,     {  },          0,      SOUND_COMMANDS,   2,     1,    {  },                   "Play previous track in the music list" },

   { "add",         &GameUserInterface::addTimeHandler,         { xINT },                 1, LEVEL_COMMANDS,  0,  1,  {"<time in minutes>"},                      "Add time to the current game" },
   { "next",        &GameUserInterface::nextLevelHandler,       {  },                     0, LEVEL_COMMANDS,  0,  1,  {  },                                       "Start next level" },
   { "prev",        &GameUserInterface::prevLevelHandler,       {  },                     0, LEVEL_COMMANDS,  0,  1,  {  },                                       "Replay previous level" },
   { "restart",     &GameUserInterface::restartLevelHandler,    {  },                     0, LEVEL_COMMANDS,  0,  1,  {  },                                       "Restart current level" },
   { "random",      &GameUserInterface::randomLevelHandler,     {  },                     0, LEVEL_COMMANDS,  0,  1,  {  },                                       "Start random level" },
   { "settime",     &GameUserInterface::setTimeHandler,         { xINT },                 1, LEVEL_COMMANDS,  0,  1,  {"<time in minutes>"},                      "Set play time for the level" },
   { "setscore",    &GameUserInterface::setWinningScoreHandler, { xINT },                 1, LEVEL_COMMANDS,  0,  1,  {"<score>"},                                "Set score to win the level" },
   { "resetscore",  &GameUserInterface::resetScoreHandler,      {  },                     0, LEVEL_COMMANDS,  0,  1,  {  },                                       "Reset all scores to zero" },
   { "addbot",      &GameUserInterface::addBotHandler,          { STR, TEAM, STR },       3, LEVEL_COMMANDS,  1,  2,  {"[file]", "[team name or num]","[args]"},          "Add bot from [file] to [team num], pass [args] to bot" },
   { "addbots",     &GameUserInterface::addBotsHandler,         { xINT, STR, TEAM, STR }, 4, LEVEL_COMMANDS,  1,  2,  {"[count]","[file]","[team name or num]","[args]"}, "Add [count] bots from [file] to [team num], pass [args] to bot" },
   { "kickbot",     &GameUserInterface::kickBotHandler,         {  },                     0, LEVEL_COMMANDS,  1,  1,  {  },                                       "Kick most recently added bot" },
   { "kickbots",    &GameUserInterface::kickBotsHandler,        {  },                     0, LEVEL_COMMANDS,  1,  1,  {  },                                       "Kick all bots" },

   { "kick",               &GameUserInterface::kickPlayerHandler,         { NAME },       1, ADMIN_COMMANDS,  0,  1,  {"<name>"},              "Kick a player from the game" },
   { "ban",                &GameUserInterface::banPlayerHandler,          { NAME, xINT }, 2, ADMIN_COMMANDS,  0,  1,  {"<name>","[duration]"}, "Ban a player from the server (IP-based, def. = 60 mins)" },
   { "banip",              &GameUserInterface::banIpHandler,              { STR, xINT },  2, ADMIN_COMMANDS,  0,  1,  {"<ip>","[duration]"},   "Ban an IP address from the server (def. = 60 mins)" },
   { "shutdown",           &GameUserInterface::shutdownServerHandler,     { xINT, STR },  2, ADMIN_COMMANDS,  0,  1,  {"[time]","[message]"},  "Start orderly shutdown of server (def. = 10 secs)" },
   { "setlevpass",         &GameUserInterface::setLevPassHandler,         { STR },        1, ADMIN_COMMANDS,  0,  1,  {"[passwd]"},            "Set level change password (use blank to clear)" },
   { "setadminpass",       &GameUserInterface::setAdminPassHandler,       { STR },        1, ADMIN_COMMANDS,  0,  1,  {"[passwd]"},            "Set admin password" },
   { "setserverpass",      &GameUserInterface::setServerPassHandler,      { STR },        1, ADMIN_COMMANDS,  0,  1,  {"<passwd>"},            "Set server password (use blank to clear)" },
   { "leveldir",           &GameUserInterface::setLevelDirHandler,        { STR },        1, ADMIN_COMMANDS,  0,  1,  {"<new level folder>"},  "Set leveldir param on the server (changes levels available)" },
   { "setservername",      &GameUserInterface::setServerNameHandler,      { STR },        1, ADMIN_COMMANDS,  0,  1,  {"<name>"},              "Set server name" },
   { "setserverdescr",     &GameUserInterface::setServerDescrHandler,     { STR },        1, ADMIN_COMMANDS,  0,  1,  {"<descr>"},             "Set server description" },
   { "deletecurrentlevel", &GameUserInterface::deleteCurrentLevelHandler, { },            0, ADMIN_COMMANDS,  0,  1,  {""},                    "Remove current level from server" },
   { "gmute",              &GameUserInterface::globalMuteHandler,         { NAME },       1, ADMIN_COMMANDS,  0,  1,  {"<name>"},              "Globally mute/unmute a player" },
   { "rename",             &GameUserInterface::renamePlayerHandler,       { NAME, STR },  2, ADMIN_COMMANDS,  0,  1,  {"<from>","<to>"},       "Give a player a new name" },
   { "maxbots",            &GameUserInterface::setMaxBotsHandler,         { xINT },       1, ADMIN_COMMANDS,  0,  1,  {"<count>"},             "Set the maximum bots allowed for this server" },
   { "shuffle",            &GameUserInterface::shuffleTeams,              { },            0, ADMIN_COMMANDS,  0,  1,   { "" },                 "Randomly reshuffle teams" },

   { "showcoords", &GameUserInterface::showCoordsHandler,    {  },        0, DEBUG_COMMANDS, 0,  1, {  },         "Show ship coordinates" },
   { "showzones",  &GameUserInterface::showZonesHandler,     {  },        0, DEBUG_COMMANDS, 0,  1, {  },         "Show bot nav mesh zones" },
   { "showpaths",  &GameUserInterface::showPathsHandler,     {  },        0, DEBUG_COMMANDS, 0,  1, {  },         "Show robot paths" },
   { "showbots",   &GameUserInterface::showBotsHandler,      {  },        0, DEBUG_COMMANDS, 0,  1, {  },         "Show all robots" },
   { "pausebots",  &GameUserInterface::pauseBotsHandler,     {  },        0, DEBUG_COMMANDS, 0,  1, {  },         "Pause all bots. Reissue to start again" },
   { "stepbots",   &GameUserInterface::stepBotsHandler,      { xINT },    1, DEBUG_COMMANDS, 1,  1, {"[steps]"},  "Advance bots by number of steps (default = 1)"},
   { "linewidth",  &GameUserInterface::lineWidthHandler,     { xINT },    1, DEBUG_COMMANDS, 1,  1, {"[number]"}, "Change width of all lines (default = 2)" },
   { "maxfps",     &GameUserInterface::maxFpsHandler,        { xINT },    1, DEBUG_COMMANDS, 1,  1, {"<number>"}, "Set maximum speed of game in frames per second" },
   { "lag",        &GameUserInterface::lagHandler, {xINT,xINT,xINT,xINT}, 4, DEBUG_COMMANDS, 1,  2, {"<send lag>", "[% of send drop packets]", "[receive lag]", "[% of receive drop packets]"}, "Set additional lag and percent of dropped packets" },
   { "clearcache", &GameUserInterface::clearCacheHandler,    {  },        0, DEBUG_COMMANDS, 1,  1, { },          "Clear any cached scripts, forcing them to be reloaded" },
};


// Display proper chat queue based on mMessageDisplayMode.  These displayers are configured in the constructor. 
void GameUserInterface::renderChatMsgs()
{
   bool helperActive = (mHelper != NULL);

   if(mMessageDisplayMode == ShortTimeout)
      mChatMessageDisplayer1.render(CHAT_Y_POS, helperActive);

   else if(mMessageDisplayMode == ShortFixed)
      mChatMessageDisplayer2.render(CHAT_Y_POS, helperActive);

   else
      mChatMessageDisplayer3.render(CHAT_Y_POS, helperActive);


   mServerMessageDisplayer.render(getGame()->getSettings()->getIniSettings()->showWeaponIndicators ? messageMargin : vertMargin, helperActive);
}


S32 chatCmdSize = ARRAYSIZE(chatCmds);    // So instructions will now how big chatCmds is

// Render chat msg that user is composing
void GameUserInterface::renderCurrentChat()
{
   if(mCurrentChatType == NoChat)
      return;

   const char *promptStr;

   Color baseColor;

   if(isCmdChat())      // Whatever the underlying chat mode, seems we're entering a command here
   {
      baseColor = gCmdChatColor;
      promptStr = mCurrentChatType ? "(Command): /" : "(Command): ";
   }
   else if(mCurrentChatType == TeamChat)    // Team chat (goes to all players on team)
   {
      baseColor = gTeamChatColor;
      promptStr = "(Team): ";
   }
   else                                     // Global in-game chat (goes to all players in game)
   {
      baseColor = gGlobalChatColor;
      promptStr = "(Global): ";
   }

   // Protect against crashes while game is initializing... is this really needed??
   if(!getGame()->getConnectionToServer())
      return;

   S32 promptSize = getStringWidth(CHAT_FONT_SIZE, promptStr);
   S32 nameSize = getStringWidthf(CHAT_FONT_SIZE, "%s: ", getGame()->getClientInfo()->getName().getString());
   S32 nameWidth = max(nameSize, promptSize);
   // Above block repeated below...

   const S32 ypos = CHAT_Y_POS + CHAT_FONT_SIZE + (2 * CHAT_FONT_GAP) + 5;

   S32 boxWidth = gScreenInfo.getGameCanvasWidth() - 2 * horizMargin - (nameWidth - promptSize) - 230;

   // Render text entry box like thingy
   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   F32 vertices[] = {
         horizMargin, ypos - 3,
         horizMargin + boxWidth, ypos - 3,
         horizMargin + boxWidth, ypos + CHAT_FONT_SIZE + 7,
         horizMargin, ypos + CHAT_FONT_SIZE + 7
   };
   for(S32 i = 1; i >= 0; i--)
   {
      glColor(baseColor, i ? .25f : .4f);
      renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, i ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
   }

   glColor(baseColor);

   // Display prompt
   S32 promptWidth = getStringWidth(CHAT_FONT_SIZE, promptStr);
   S32 xStartPos = horizMargin + 3 + promptWidth;

   drawString(horizMargin + 3, ypos, CHAT_FONT_SIZE, promptStr);  // draw prompt

   // Display typed text
   string displayString = mLineEditor.getString();
   S32 displayWidth = getStringWidth(CHAT_FONT_SIZE, displayString.c_str());

   // If the string goes too far out of bounds, display it chopped off at the front to give more room to type
   while (displayWidth > boxWidth - promptWidth - 16)  // 16 -> Account for margin and cursor
   {
      displayString = displayString.substr(25, string::npos);  // 25 -> # chars to chop off displayed text if overflow
      displayWidth = getStringWidth(CHAT_FONT_SIZE, displayString.c_str());
   }

   drawString(xStartPos, ypos, CHAT_FONT_SIZE, displayString.c_str());

   // If we've just finished entering a chat cmd, show next parameter
   if(isCmdChat())
   {
      string line = mLineEditor.getString();
      Vector<string> words = parseStringx(line.c_str());

      if(words.size() > 0)
      {
         for(S32 i = 0; i < chatCmdSize; i++)
         {
            const char *cmd = words[0].c_str();

            if(!stricmp(cmd, chatCmds[i].cmdName.c_str()))
            {
               // My thinking here is that if the number of quotes is odd, the last argument is not complete, even if
               // it ends in a space.  There may be an edge case that voids this argument, but our use is simple enough 
               // that this should work well.  If a number is even, num % 2 will be 0.
               S32 numberOfQuotes = count(line.begin(), line.end(), '"');
               if(chatCmds[i].cmdArgCount >= words.size() && line[line.size() - 1] == ' ' && numberOfQuotes % 2 == 0)
               {
                  glColor(baseColor * .5);
                  drawString(xStartPos + displayWidth, ypos, CHAT_FONT_SIZE, chatCmds[i].helpArgString[words.size() - 1].c_str());
               }

               break;
            }
         }
      }
   }

   glColor(baseColor);
   mLineEditor.drawCursor(xStartPos, ypos, CHAT_FONT_SIZE, displayWidth);
}


extern S32 QSORT_CALLBACK alphaSort(string *a, string *b);     // Sort alphanumerically

static Vector<string> commandCandidateList;

static void makeCommandCandidateList()
{
   for(S32 i = 0; i < chatCmdSize; i++)
      commandCandidateList.push_back(chatCmds[i].cmdName);

   commandCandidateList.sort(alphaSort);
}


// Make a list of all players in the game
static void makePlayerNameList(Game *game, Vector<string> &nameCandidateList)
{
   nameCandidateList.clear();

   for(S32 i = 0; i < game->getClientCount(); i++)
      nameCandidateList.push_back(((Game *)game)->getClientInfo(i)->getName().getString());
}


static void makeTeamNameList(const Game *game, Vector<string> &nameCandidateList)
{
   nameCandidateList.clear();

   if(game->getGameType())
      for(S32 i = 0; i < game->getTeamCount(); i++)
         nameCandidateList.push_back(game->getTeamName(i).getString());
}


static Vector<string> *getCandidateList(Game *game, const char *first, S32 arg)
{
   if(arg == 0)         // ==> Command completion
      return &commandCandidateList;

   else if(arg > 0)     // ==> Arg completion
   {
      // Figure out which command we're entering, so we know what kind of args to expect
      S32 cmd = -1;

      for(S32 i = 0; i < chatCmdSize; i++)
         if(!stricmp(chatCmds[i].cmdName.c_str(), first))
         {
            cmd = i;
            break;
         }

      if(cmd != -1 && arg <= chatCmds[cmd].cmdArgCount)     // Found a command
      {
         ArgTypes argType = chatCmds[cmd].cmdArgInfo[arg - 1];  // What type of arg are we expecting?

         static Vector<string> nameCandidateList;     // Reusable container

         if(argType == NAME)           // ==> Player name completion
         {  
            makePlayerNameList(game, nameCandidateList);    // Creates a list of all player names
            return &nameCandidateList;
         }

         else if(argType == TEAM)      // ==> Team name completion
         {
            makeTeamNameList(game, nameCandidateList);
            return &nameCandidateList;
         }
         // else no arg completion for you!
      }
   }
   
   return NULL;                        // ==> No completion options
}


bool GameUserInterface::processChatModeKey(InputCode inputCode)
{
   TNLAssert(mCurrentChatType != NoChat, "Must be in a chat mode to get here!");

   if(inputCode == KEY_ENTER)
      issueChat();
   else if(inputCode == KEY_BACKSPACE)
      mLineEditor.backspacePressed();
   else if(inputCode == KEY_DELETE)
      mLineEditor.deletePressed();
   else if(inputCode == KEY_ESCAPE || inputCode == BUTTON_BACK)
      cancelChat();
   else if(inputCode == KEY_TAB)      // Auto complete any commands
   {
      if(isCmdChat())     // It's a command!  Complete!  Complete!
      {
         // First, parse line into words
         Vector<string> words = parseString(mLineEditor.getString());

         bool needLeadingSlash = false;

         // Handle leading slash when command is entered from ordinary chat prompt
         if(words.size() > 0 && words[0][0] == '/')
         {
            // Special case: User has entered process by starting with global chat, and has typed "/" then <tab>
            if(mLineEditor.getString() == "/")
               words.clear();          // Clear -- it's as if we're at a fresh "/" prompt where the user has typed nothing
            else
               words[0].erase(0, 1);   // Strip char -- remove leading "/" so it's as if were at a regular "/" prompt

            needLeadingSlash = true;   // We'll need to add the stripped "/" back in later
         }
               
         S32 arg;                 // Which word we're looking at
         string partial;          // The partially typed word we're trying to match against
         const char *first;       // First arg we entered (will match partial if we're still entering the first one)
         
         // Check for trailing space --> http://www.suodenjoki.dk/us/archive/2010/basic-string-back.htm
         if(words.size() > 0 && *mLineEditor.getString().rbegin() != ' ')   
         {
            arg = words.size() - 1;          // No trailing space --> current arg is the last word we've been typing
            partial = words[arg];            // We'll be matching against what we've typed so far
            first = words[0].c_str();      
         }
         else if(words.size() > 0)           // We've entered a word, pressed space indicating word is complete, 
         {                                   // but have not started typing the next word.  We'll let user cycle through every
            arg = words.size();              // possible value for next argument.
            partial = "";
            first = words[0].c_str(); 
         }
         else     // If the editor is empty, or if final character is a space, then we need to set these params differently
         {
            arg = words.size();              // Trailing space --> current arg is the next word we've not yet started typing
            partial = "";
            first = "";                      // We'll be matching against an empty list since we've typed nothing so far
         }
         
         const string *entry = mLineEditor.getStringPtr();  

         Vector<string> *candidates = getCandidateList(getGame(), first, arg);     // Could return NULL

         // If the command string has quotes in it, use the last space up to the first quote
         std::size_t lastChar = string::npos;
         if(entry->find_first_of("\"") != string::npos)
            lastChar = entry->find_first_of("\"");

         string appender = " ";

         std::size_t pos = entry->find_last_of(' ', lastChar);

         if(pos == string::npos)                         // String does not contain a space, requires special handling
         {
            pos = 0;
            if(words.size() <= 1 && needLeadingSlash)    // ugh!  More special cases!
               appender = "/";
            else
               appender = "";
         }

         mLineEditor.completePartial(candidates, partial, pos, appender); 
      }
   }
   else
      return false;

   return true;
}


void GameUserInterface::onKeyUp(InputCode inputCode)
{
   // These keys works in any mode!  And why not??

   GameSettings *settings = getGame()->getSettings();

   if(checkInputCode(settings, InputCodeManager::BINDING_MISSION, inputCode))
      mMissionOverlayActive = false;
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
   if((mCurrentChatType == NoChat) && !mDisableShipKeyboardInput && !gConsole.isVisible())
   {
      // Some helpers (like TeamShuffle) like to disable movement when they are active
      if(mHelper && mHelper->isMovementDisabled())
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


// User has finished entering a chat message and pressed <enter>
void GameUserInterface::issueChat()
{
   TNLAssert(mCurrentChatType != NoChat, "Not in chat mode!");

   if(!mLineEditor.isEmpty())
   {
      // Check if chat buffer holds a message or a command
      if(!isCmdChat())    // It's not a command
      {
         GameType *gameType = getGame()->getGameType();
         if(gameType)
            gameType->c2sSendChat(mCurrentChatType == GlobalChat, mLineEditor.c_str());   // Broadcast message
      }
      else    // It's a command
         runCommand(mLineEditor.c_str());
   }

   cancelChat();     // Hide chat display
}


Vector<string> GameUserInterface::parseStringx(const char *str)
{
   Vector<string> words = parseString(str);

   if(words.size() > 0)
      if(words[0][0] == '/')
         words[0].erase(0, 1);      // Remove leading /

   return words;
}


void GameUserInterface::resetLevelInfoDisplayTimer()
{
   mLevelInfoDisplayTimer.reset(6000);  // 6 seconds
}


void GameUserInterface::clearLevelInfoDisplayTimer()
{
   mLevelInfoDisplayTimer.clear();
}


// Process a command entered at the chat prompt
// Returns true if command was handled (even if it was bogus); returning false will cause command to be passed on to the server
// Runs on client
void GameUserInterface::runCommand(const char *input)
{
   Vector<string> words = parseStringx(input);  // yes

   if(words.size() == 0)            // Just in case, must have 1 or more words to check the first word as command.
      return;

   GameConnection *gc = getGame()->getConnectionToServer();
   if(!gc)
   {
      displayErrorMessage("!!! Not connected to server");
      return;
   }

   for(U32 i = 0; i < ARRAYSIZE(chatCmds); i++)
      if(lcase(words[0]) == chatCmds[i].cmdName)
      {
         (this->*(chatCmds[i].cmdCallback))(words);
         return;
      }

   serverCommandHandler(words);     // Command unknown to client, will pass it on to server
}


// Set specified volume to the specefied level
void GameUserInterface::setVolume(VolumeType volType, const Vector<string> &words)
{
   S32 vol;

   if(words.size() < 2)
   {
      displayErrorMessage("!!! Need to specify volume");
      return;
   }

   string volstr = words[1];

   // Parse volstr -- if first digit isn't a number, user probably screwed up.
   // atoi will return 0, but this probably isn't what the user wanted.
   if(volstr[0] >= '0' && volstr[0] <= '9')
      vol = max(min(atoi(volstr.c_str()), 10), 0);
   else
   {
      displayErrorMessage("!!! Invalid value... volume not changed");
      return;
   }

  switch(volType)
  {
   case SfxVolumeType:
      getGame()->getSettings()->getIniSettings()->sfxVolLevel = (F32) vol / 10.f;
      displayMessagef(gCmdChatColor, "SFX volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
      return;

   case MusicVolumeType:
      getGame()->getSettings()->getIniSettings()->setMusicVolLevel((F32) vol / 10.f);
      displayMessagef(gCmdChatColor, "Music volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
      return;

   case VoiceVolumeType:
   {
      F32 oldVol = getGame()->getSettings()->getIniSettings()->voiceChatVolLevel;
      getGame()->getSettings()->getIniSettings()->voiceChatVolLevel = (F32) vol / 10.f;
      displayMessagef(gCmdChatColor, "Voice chat volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
      if((oldVol == 0) != (vol == 0) && getGame()->getConnectionToServer())
         getGame()->getConnectionToServer()->s2rVoiceChatEnable(vol != 0);
      return;
   }

   case ServerAlertVolumeType:
      getGame()->getConnectionToServer()->c2sSetServerAlertVolume((S8) vol);
      displayMessagef(gCmdChatColor, "Server alerts chat volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
      return;
  }
}


void GameUserInterface::cancelChat()
{
   mLineEditor.clear();
   mCurrentChatType = NoChat;
   setBusyChatting(false);

   getGame()->undelaySpawn();
}


// Constructor
GameUserInterface::VoiceRecorder::VoiceRecorder(ClientGame *game)
{
   mRecordingAudio = false;
   mMaxAudioSample = 0;
   mMaxForGain = 0;
   mVoiceEncoder = new SpeexVoiceEncoder;

   mGame = game;
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


void GameUserInterface::suspendGame()
{
   getGame()->getConnectionToServer()->suspendGame();    // Tell server we're suspending
   getGame()->suspendGame(false);                        // Suspend locally
   getUIManager()->activate(SuspendedUI);                // And enter chat mode
}


void GameUserInterface::unsuspendGame()
{
   getGame()->unsuspendGame();                                 // Unsuspend locally
   getGame()->getConnectionToServer()->unsuspendGame();        // Tell the server we're unsuspending
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
   const S32 legendSize = fontSize - 3;     
   const S32 legendGap  = 3;                        // Space between scoreboard and legend
   const S32 humans     = getGame()->getPlayerCount();
   const S32 legendPos  = (canvasHeight - totalHeight) / 2 + totalHeight + legendGap;   

   string legendWhite = itos(humans) + " Human" + (humans != 1 ? "s" : "") + " | " + adminSymbol + "= Admin | " + 
                        levelChangerSymbol + "= Can Change Levels | " + botSymbol + "= Bot |";

   // Not quite the function's intended purpose, but it does the job
   drawCenteredStringPair(legendPos, legendSize, Colors::standardPlayerScoreboardColor, Colors::idlePlayerScoreboardColor, legendWhite.c_str(), "Idle Player");
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
   if(mLevelInfoDisplayTimer.getCurrent() || mMissionOverlayActive)
      renderMissionOverlay(gameType);

   if(mInputModeChangeAlertDisplayTimer.getCurrent() != 0)
      renderInputModeChangeAlert();

   Game *game = getGame();
   S32 teamCount = game->getTeamCount();

   U32 rightAlignCoord = gScreenInfo.getGameCanvasWidth() - horizMargin;

   if((gameType->isGameOver() || scoreboardVisible) && teamCount > 0)
      renderScoreboard();
   
   // Render team scores in lower-right corner when scoreboard is off
   else if(teamCount > 1 && gameType->isTeamGame())
   {
      // Render Core scores
      if(gameType->getGameTypeId() == CoreGame)
         renderCoreScores(gameType, rightAlignCoord);

      // Render scores for the rest of the team game types, which use flags
      else
         renderTeamFlagScores(gameType, rightAlignCoord);
   }
   else if(teamCount > 0 && !gameType->isTeamGame())     // For single team games like rabbit and bitmatch
      renderLeadingPlayerScores(gameType, rightAlignCoord);

   renderTimeLeft(rightAlignCoord);
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


void GameUserInterface::renderMissionOverlay(const GameType *gameType)
{
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();
   S32 yCenter = canvasHeight / 2;

   // Fade message out
   F32 alpha = 1;
   if(mLevelInfoDisplayTimer.getCurrent() < 1000 && !mMissionOverlayActive)
      alpha = mLevelInfoDisplayTimer.getCurrent() * 0.001f;

   glColor(Colors::white, alpha);
   drawCenteredStringf(yCenter - 180, 30, "Level: %s", gameType->getLevelName()->getString());

   // Prefix game type with "Team" if they are typically individual games, but are being played in team mode
   const char *gtPrefix = (gameType->canBeIndividualGame() && gameType->getGameTypeId() != SoccerGame && 
                           getGame()->getTeamCount() > 1) ? "Team " : "";

   drawCenteredStringf(yCenter - 140, 30, "Game Type: %s%s", gtPrefix, gameType->getGameTypeName());

   glColor(Colors::cyan, alpha);
   drawCenteredString(yCenter - 100, 20, gameType->getInstructionString());

   glColor(Colors::magenta, alpha);
   drawCenteredString(yCenter - 75, 20, gameType->getLevelDescription()->getString());

   glColor(Colors::menuHelpColor, alpha);
   drawCenteredStringf(canvasHeight - 100, 20, "Press [%s] to see this information again", 
                                               getInputCodeString(getGame()->getSettings(), InputCodeManager::BINDING_MISSION));

   if(gameType->getLevelCredits()->isNotNull())    // Only render credits string if it's is not empty
   {
      glColor(Colors::red, alpha);
      drawCenteredStringf(yCenter + 50, 20, "%s", gameType->getLevelCredits()->getString());
   }

   glColor(Colors::yellow, alpha);
   drawCenteredStringf(yCenter - 50, 20, "Score to Win: %d", gameType->getWinningScore());

   mInputModeChangeAlertDisplayTimer.reset(0);     // Supress mode change alert if this message is displayed...
}


void GameUserInterface::renderTeamFlagScores(const GameType *gameType, U32 rightAlignCoord)
{
   S32 lroff = gameType->getLowerRightCornerScoreboardOffsetFromBottom();

   Game *game = getGame();
   S32 teamCount = game->getTeamCount();

   //Vector<Team *> *teams = game->getSortedTeamList_score();

   const S32 textsize = 32;
   S32 xpos = rightAlignCoord - gameType->getDigitsNeededToDisplayScore() * getStringWidth(textsize, "0");

   for(S32 i = 0; i < teamCount; i++)
   {
      S32 ypos = gScreenInfo.getGameCanvasHeight() - vertMargin - lroff - (teamCount - i - 1) * 38;

      if(gameType->teamHasFlag(i))
      {
         glColor(Colors::magenta);
         drawString(xpos - 50, ypos + 3, 18, "*");
      }

      Team *team = (Team *)game->getTeam(i);
      renderFlag(F32(xpos - 20), F32(ypos + 18), team->getColor());

      glColor(Colors::white);
      drawStringf(xpos, ypos, textsize, "%d", team->getScore());
   }
}


void GameUserInterface::renderCoreScores(const GameType *gameType, U32 rightAlignCoord)
{
   CoreGameType *cgt = static_cast<CoreGameType*>(const_cast<GameType*>(gameType));

   S32 lroff = gameType->getLowerRightCornerScoreboardOffsetFromBottom();

   Game *game = getGame();
   S32 teamCount = game->getTeamCount();

   //Vector<Team *> *teams = game->getSortedTeamList_score();

   const S32 textSize = 32;
   S32 xpos = rightAlignCoord - gameType->getDigitsNeededToDisplayScore() * getStringWidth(textSize, "0");

   for(S32 i = 0; i < teamCount; i++)
   {
      S32 ypos = gScreenInfo.getGameCanvasHeight() - vertMargin - lroff - (teamCount - i - 1) * 38;

      Team *team = (Team *)game->getTeam(i);
      Point center(xpos - 20, ypos + 19);

      renderCoreSimple(center, team->getColor(), 20);

      // Render something if a Core is being attacked
      if(cgt->isTeamCoreBeingAttacked(i))
      {
         if(game->getCurrentTime() % 300 > 150)
            glColor(Colors::red80);
         else
            glColor(Colors::yellow, 0.6f);
         
         drawCircle(center, 15);
      }

      glColor(Colors::white);
      drawStringf(xpos, ypos, textSize, "%d", team->getScore());
   }
}


void GameUserInterface::renderLeadingPlayerScores(const GameType *gameType, U32 rightAlignCoord)
{
   // We can render before we get the first unpackUpdate packet that gets all the client infos.
   // In this case just exit
   if(getGame()->getLocalRemoteClientInfo() == NULL)
      return;

   if(gameType->getLeadingPlayer() < 0)
      return;

   Game *game = static_cast<Game *>(getGame());    // This is a sign of a problem

   S32 lroff = gameType->getLowerRightCornerScoreboardOffsetFromBottom() - 22;

   const S32 textsize = 12;

   /// Render player score
   bool hasSecondLeader = gameType->getSecondLeadingPlayer() >= 0;


   const StringTableEntry localClientName = getGame()->getClientInfo()->getName();

   // The player is the leader if a leader is detected and it matches his name
   bool localClientIsLeader = localClientName == game->getClientInfo(gameType->getLeadingPlayer())->getName();

   const char *name;
   S32 topScore, bottomScore;


   const Color *winnerColor = &Colors::red;
   const Color *loserColor = &Colors::red60;

   S32 vertOffset = (hasSecondLeader || !localClientIsLeader) ? textsize * 4 / 3 : 0;    // Make room for a second entry, as needed
   S32 ypos = gScreenInfo.getGameCanvasHeight() - vertMargin - lroff - vertOffset;

   glColor(winnerColor);

   name = game->getClientInfo(gameType->getLeadingPlayer())->getName().getString();
   topScore = gameType->getLeadingPlayerScore();

   drawStringfr(rightAlignCoord, ypos, textsize, "%s %d", name, topScore);


   // Render bottom score if we have one
   // This will either render the current client on the bottom; or, if he is the leader
   // it will render the second leader
   if(hasSecondLeader || !localClientIsLeader)
   {
      // Should test if leader first
      if(!localClientIsLeader)
      {
         bottomScore = getGame()->getLocalRemoteClientInfo()->getScore();
         name = getGame()->getLocalRemoteClientInfo()->getName().getString();
      }
      // hasSecondLeader
      else
      {
         bottomScore = gameType->getSecondLeadingPlayerScore();
         name = game->getClientInfo(gameType->getSecondLeadingPlayer())->getName().getString();
      }

      S32 ypos = gScreenInfo.getGameCanvasHeight() - vertMargin - lroff;

      // Special case: if players are tied, render both with winner's color
      if(topScore == bottomScore)
         glColor(winnerColor);
      else
         glColor(loserColor);

      drawStringfr(rightAlignCoord, ypos, textsize, "%s %d", name, bottomScore);
   }
}


void GameUserInterface::renderTimeLeft(U32 rightAlignCoord)
{
   const S32 size = 20;       // Size of time
   const S32 gtsize = 12;     // Size of game type/score indicator
   
   GameType *gameType = getGame()->getGameType();

   string txt = string("[") + gameType->getShortName() + "/" + itos(gameType->getWinningScore()) + "]";

   static const U32 w00 = getStringWidth(size, "00:00");
   static const U32 wUnlim = getStringWidth(size, "Unlim.");

   U32 w = gameType->isTimeUnlimited() ? wUnlim : w00;

   S32 x = rightAlignCoord - w;
   S32 y = gScreenInfo.getGameCanvasHeight() - vertMargin - 20;

   glColor(Colors::cyan);
   drawStringfr(x - 5, y + ((size - gtsize) / 2) + 2, gtsize, txt.c_str());
   
   glColor(Colors::white);

   if(gameType->isTimeUnlimited())  
      drawString(x, y, size, "Unlim.");
   else
      drawTime(x, y, size, gameType->getRemainingGameTimeInMs());
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


void GameUserInterface::onChatMessageRecieved(const Color &msgColor, const char *format, ...)
{
   // Ignore empty message
   if(!strcmp(format, ""))
      return;

   static char buffer[MAX_CHAT_MSG_LENGTH];

   va_list args;

   va_start(args, format);
   vsnprintf(buffer, sizeof(buffer), format, args);
   va_end(args);

   mChatMessageDisplayer1.onChatMessageRecieved(msgColor, buffer);      // Standard chat stream
   mChatMessageDisplayer2.onChatMessageRecieved(msgColor, buffer);      // Short, non-expiring chat stream
   mChatMessageDisplayer3.onChatMessageRecieved(msgColor, buffer);      // Long, non-expiring chat stream
}


// Set which chat message display mode we're in (Ctrl-M)
void GameUserInterface::toggleChatDisplayMode()
{
   S32 m = mMessageDisplayMode + 1;

   if(m >= MessageDisplayModes)
      m = 0;

   mMessageDisplayMode = MessageDisplayMode(m);
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
void ChatMessageDisplayer::onChatMessageRecieved(const Color &msgColor, const string &msg)
{
   Vector<string> lines = UserInterface::wrapString(substitueVars(msg), mWrapWidth, mFontSize, "      ");

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
void ChatMessageDisplayer::render(S32 anchorPos, bool helperVisible)
{
   // Are we in the act of transitioning between one message and another?
   bool isScrolling = (mChatScrollTimer.getCurrent() > 0);  

   // Check if there any messages to display... if not, bail
   if(mFirst == mLast && !(mTopDown && isScrolling))
      return;

   S32 lineHeight = mFontSize + mFontGap;

   GLboolean scissorsShouldBeEnabled;

   // Make these static to save a tiny bit of construction and tear-down costs.  We run this a lot, and they're small.
   static GLint scissorBox[4];
   static Point p1, p2;

   // Only need to set scissors if we're scrolling.  When not scrolling, we control the display by only showing
   // the specified number of lines; there are normally no partial lines that need vertical clipping as 
   // there are when we're scrolling.  Note also that we only clip vertically, and can ignore the horizontal.
   if(isScrolling)    
   {
      glGetBooleanv(GL_SCISSOR_TEST, &scissorsShouldBeEnabled);

      if(scissorsShouldBeEnabled)
         glGetIntegerv(GL_SCISSOR_BOX, &scissorBox[0]);

      // Remember that our message list contains an extra entry that exists only for scrolling purposes.
      // We want the height of the clip window to omit this line, so we subtract 1 below.  
      S32 displayAreaHeight = (mMessages.size() - 1) * lineHeight;     
      S32 displayAreaYPos = anchorPos + (mTopDown ? displayAreaHeight : lineHeight);

      DisplayMode mode = mGame->getSettings()->getIniSettings()->displayMode;    // Windowed, full_screen_stretched, full_screen_unstretched

      // p1 will be x and y, uses raw OpenGL coordinates, which are flipped from the system used in the game
      p1 = gScreenInfo.convertCanvasToWindowCoord(0, gScreenInfo.getGameCanvasHeight() - displayAreaYPos,  mode);
      // p2 will be w and h.  Also we don't care about width here... wrapping takes care of that.
      p2 = gScreenInfo.convertCanvasToWindowCoord(gScreenInfo.getGameCanvasWidth(), displayAreaHeight, mode);

      glScissor(p1.x, p1.y, p2.x, p2.y);

      glEnable(GL_SCISSOR_TEST);
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
                                  
   for(U32 i = mFirst; i != mLast - renderExtra; i--)
   {
      U32 index = i % (U32)mMessages.size();    // Handle wrapping in our message list

      if(helperVisible)   
         glColor(mMessages[index].color, 0.2f); // Dim
      else
         glColor(mMessages[index].color);       // Bright

      UserInterface::drawString(UserInterface::horizMargin, y, mFontSize, mMessages[index].str.c_str());

      y -= lineHeight;
   }


   // Restore scissors settings -- only used during scrolling
   if(isScrolling)
   {
      if(scissorsShouldBeEnabled)
         glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
      else
         glDisable(GL_SCISSOR_TEST);
   }
}


};

