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
#include "game.h"
#include "UIMenus.h"
#include "UIInstructions.h"
#include "UIChat.h"
#include "UIMessage.h"
#include "UIDiagnostics.h"
#include "UIErrorMessage.h"
#include "robot.h"               // For robot stuff
#include "gameType.h"
#include "IniFile.h"             // For access to gINI functions
#include "EngineeredItem.h"   // For EngineerModuleDeployer
#include "ship.h"
#include "shipItems.h"           // For EngineerBuildObjects
#include "gameObjectRender.h"
#include "input.h"
#include "config.h"
#include "loadoutHelper.h"
#include "gameNetInterface.h"
#include "SoundSystem.h"
#include "md5wrapper.h"          // For submission of passwords
#include "oglconsole.h"          // Our console object
#include "config.h"              // for Getmap level dir
#include "ScreenInfo.h"
#include "ClientGame.h"
#include "Colors.h"
#include "Cursor.h"
#include "CoreGame.h"

#include "../tnl/tnlEndian.h"

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"

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


Color GameUserInterface::privateF5MessageDisplayedInGameColor(Colors::blue);


static void makeCommandCandidateList();      // Forward delcaration

// Constructor
GameUserInterface::GameUserInterface(ClientGame *game) : Parent(game), 
                                                         mVoiceRecorder(game),
                                                         mLineEditor(200)
                                                         
{
   mInScoreboardMode = false;
   mFPSVisible = false;
   mHelper = NULL;
   displayInputModeChangeAlert = false;
   mMissionOverlayActive = false;

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


   // Initialize message buffers
   for(S32 i = 0; i < MessageDisplayCount; i++)
      mDisplayMessage[i][0] = 0;

   for(S32 i = 0; i < ChatMessageStoreCount; i++)
      mStoreChatMessage[i][0] = 0;

   for(S32 i = 0; i < ChatMessageDisplayCount; i++)
      mDisplayChatMessage[i][0] = 0;

   mGotControlUpdate = false;
   mRecalcFPSTimer = 0;

   mFiring = false;
   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
   {
      mModPrimaryActivated[i] = false;
      mModSecondaryActivated[i] = false;
   }

   mDisplayMessageTimer.setPeriod(DisplayMessageTimeout);    // Set the period of our message timeout timer
   mDisplayChatMessageTimer.setPeriod(DisplayChatMessageTimeout);
   mModuleOneDoubleClickTimer.setPeriod(DoubleClickTimeout);
   mModuleTwoDoubleClickTimer.setPeriod(DoubleClickTimeout);
   //populateChatCmdList();

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

void processGameConsoleCommand(OGLCONSOLE_Console console, char *cmd)
{
   if(!strncmp(cmd, "quit", 4) || !strncmp(cmd, "exit", 4)) 
      OGLCONSOLE_HideConsole();

   else if(!strncmp(cmd, "help", 4) || !strncmp(cmd, "?", 1)) 
      OGLCONSOLE_Output(console, "Commands: help; quit\n");

   else if(!strncmp(cmd, "add", 3))
   {
      int a, b;
      if(sscanf(cmd, "add %i %i", &a, &b) == 2)
      {
         OGLCONSOLE_Output(console, "%i + %i = %i\n", a, b, a+b);
         return;
      }

      OGLCONSOLE_Output(console, "usage: add INT INT\n");
    }

    else
      OGLCONSOLE_Output(console, "Unknown command: %s\n", cmd);
}


void GameUserInterface::onActivate()
{
   mDisableShipKeyboardInput = false;  // Make sure our ship controls are active
   mMissionOverlayActive = false;      // Turn off the mission overlay (if it was on)
   SDL_SetCursor(Cursor::getTransparent());        // Turn off cursor
   onMouseMoved();                     // Make sure ship pointed is towards mouse

   // Clear out any lingering chat messages
   for(S32 i = 0; i < ChatMessageStoreCount; i++)
      mDisplayMessage[i][0] = 0;

   for(S32 i = 0; i < MessageDisplayCount; i++)
      mStoreChatMessage[i][0] = 0;

   mMessageDisplayMode = ShortTimeout;          // Start with normal chat msg display
   enterMode(PlayMode);                         // Make sure we're not in chat or loadout-select mode

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      mModPrimaryActivated[i] = false;
      mModSecondaryActivated[i] = false;
   }

   mShutdownMode = None;

   getGame()->unsuspendGame();                            // Never suspended when we start

   OGLCONSOLE_EnterKey(processGameConsoleCommand);        // Setup callback for processing console commands
}


void GameUserInterface::onReactivate()
{
   if(getGame()->isSuspended())
      unsuspendGame();

   getGame()->undelaySpawn();

   mDisableShipKeyboardInput = false;
   SDL_SetCursor(Cursor::getTransparent());    // Turn off cursor
   enterMode(PlayMode);

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
   if(!strcmp(message, ""))
      return;

   // Create a slot for our new message
   if(mDisplayMessage[0][0])
      for(S32 i = MessageDisplayCount - 1; i > 0; i--)
      {
         strcpy(mDisplayMessage[i], mDisplayMessage[i-1]);
         mDisplayMessageColor[i] = mDisplayMessageColor[i-1];
      }

   strncpy(mDisplayMessage[0], message, sizeof(mDisplayMessage[0]));    // Use strncpy to avoid buffer overflows
   mDisplayMessageColor[0] = msgColor;
   mDisplayMessageTimer.reset();
}


// A new chat message is here!  We don't actually display anything here, despite the name...
// just add it to the list, will be displayed in render()
void GameUserInterface::displayChatMessage(const Color &msgColor, const char *format, ...)
{
   // Ignore empty message
   if(!strcmp(format, ""))
      return;

   // Create a slot for our new message
   if(mDisplayChatMessage[0][0])
      for(S32 i = ChatMessageDisplayCount - 1; i > 0; i--)
      {
         strcpy(mDisplayChatMessage[i], mDisplayChatMessage[i-1]);
         mDisplayChatMessageColor[i] = mDisplayChatMessageColor[i-1];
      }

   for(S32 i = ChatMessageStoreCount - 1; i > 0; i--)
   {
      strcpy(mStoreChatMessage[i], mStoreChatMessage[i-1]);
      mStoreChatMessageColor[i] = mStoreChatMessageColor[i-1];
   }

   va_list args;

   va_start(args, format);
   vsnprintf(mDisplayChatMessage[0], sizeof(mDisplayChatMessage[0]), format, args);
   va_end(args);
   mDisplayChatMessageColor[0] = msgColor;

   va_start(args, format);
   vsnprintf(mStoreChatMessage[0], sizeof(mStoreChatMessage[0]), format, args);
   va_end(args);
   mStoreChatMessageColor[0] = msgColor;

   mDisplayChatMessageTimer.reset();
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
   mModuleOneDoubleClickTimer.update(timeDelta);
   mModuleTwoDoubleClickTimer.update(timeDelta);

   // Server messages
   if(mDisplayMessageTimer.update(timeDelta))
   {
      for(S32 i = MessageDisplayCount - 1; i > 0; i--)
      {
         strcpy(mDisplayMessage[i], mDisplayMessage[i-1]);
         mDisplayMessageColor[i] = mDisplayMessageColor[i-1];
      }

      mDisplayMessage[0][0] = 0;    // Null, that is
      mDisplayMessageTimer.reset();
   }

   // Chat messages
   if(mDisplayChatMessageTimer.update(timeDelta))
   {
      for(S32 i = ChatMessageDisplayCount - 1; i > 0; i--)
      {
         strcpy(mDisplayChatMessage[i], mDisplayChatMessage[i-1]);
         mDisplayChatMessageColor[i] = mDisplayChatMessageColor[i-1];
      }

      mDisplayChatMessage[0][0] = 0;    // Null, that is
      mDisplayChatMessageTimer.reset();
   }

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

   if(!getGame()->isSuspended())
   {
      renderReticle();           // Draw crosshairs if using mouse
      renderMessageDisplay();    // Render incoming server msgs
      renderChatMessageDisplay();    // Render incoming chat msgs
      renderCurrentChat();       // Render any chat msg user is composing
      renderLoadoutIndicators(); // Draw indicators for the various loadout items

      getUIManager()->getHostMenuUserInterface()->renderProgressListItems();  // This is the list of levels loaded while hosting

      renderProgressBar();       // This is the status bar that shows progress of loading this level

      mVoiceRecorder.render();   // This is the indicator that someone is sending a voice msg

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
   GameObject *co = con->getControlObject();
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


void GameUserInterface::renderLostConnectionMessage()
{
   GameConnection *connection = getGame()->getConnectionToServer();
   if(connection && connection->lostContact())
   {
      static const char *msg[] = { "", 
                                   "We may have lost contact with the server...", 
                                   "",
                                   " You can't play until the connection has been re-established ", 
                                   "" };
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
         static const char *msg[] = { "", timemsg, "", "Shutdown sequence intitated by you.", "", mShutdownReason.getString(), "" };
         renderMessageBox("SERVER SHUTDOWN INITIATED", "Press <ESC> to cancel shutdown", msg, 7);
      }
      else                       // Remote user intiated the shutdown
      {
         char whomsg[255];
         dSprintf(whomsg, sizeof(whomsg), "Shutdown sequence initiated by %s.", mShutdownName.getString());

         static const char *msg[] = { "", timemsg, "", whomsg, "", mShutdownReason.getString(), "" };
         renderMessageBox("SHUTDOWN INITIATED", "Press <ESC> to dismiss", msg, 7);
      }
   }
   else if(mShutdownMode == Canceled)
   {
      // Keep same number of messages as above, so if message changes, it will be a smooth transition
      static const char *msg[] = { "", "", "Server shutdown sequence canceled.", "", "Play on!", "", "" };     

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

         glBegin(i ? GL_LINE_LOOP : GL_POLYGON);
            glVertex2i(left,     gScreenInfo.getGameCanvasHeight() - vertMargin);
            glVertex2i(left + w, gScreenInfo.getGameCanvasHeight() - vertMargin);
            glVertex2i(left + w, gScreenInfo.getGameCanvasHeight() - vertMargin - height);
            glVertex2i(left,     gScreenInfo.getGameCanvasHeight() - vertMargin - height);
         glEnd();
      }
   }
}


// Draw the reticle (i.e. the mouse cursor) if we are using keyboard/mouse
void GameUserInterface::renderReticle()
{
   bool shouldRender = getGame()->getSettings()->getIniSettings()->inputMode == InputModeKeyboard &&     // Reticle in keyboard mode only
                       current->getMenuID() == GameUI;                                                   // And not when menu is active
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

      glColor(Colors::green, 0.7f);
      glBegin(GL_LINES);
         glVertex2f(offsetMouse.x - 15, offsetMouse.y);
         glVertex2f(offsetMouse.x + 15, offsetMouse.y);
         glVertex2f(offsetMouse.x, offsetMouse.y - 15);
         glVertex2f(offsetMouse.x, offsetMouse.y + 15);

         if(offsetMouse.x > 30)
         {
            glColor(Colors::green, 0);
            glVertex2f(0, offsetMouse.y);
            glColor(Colors::green, 0.7f);
            glVertex2f(offsetMouse.x - 30, offsetMouse.y);
         }
         if(offsetMouse.x < gScreenInfo.getGameCanvasWidth() - 30)
         {
            glColor(Colors::green, 0.7f);
            glVertex2f(offsetMouse.x + 30, offsetMouse.y);
            glColor(Colors::green, 0);
            glVertex2f((F32)gScreenInfo.getGameCanvasWidth(), offsetMouse.y);
         }
         if(offsetMouse.y > 30)
         {
            glColor(Colors::green, 0);
            glVertex2f(offsetMouse.x, 0);
            glColor(Colors::green, 0.7f);
            glVertex2f(offsetMouse.x, offsetMouse.y - 30);
         }
         if(offsetMouse.y < gScreenInfo.getGameCanvasHeight() - 30)
         {
            glColor(Colors::green, 0.7f);
            glVertex2f(offsetMouse.x, offsetMouse.y + 30);
            glColor(Colors::green, 0);
            glVertex2f(offsetMouse.x, (F32)gScreenInfo.getGameCanvasHeight());
         }

      glEnd();
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


// Render any incoming server msgs
void GameUserInterface::renderMessageDisplay()
{
   glColor(Colors::white);

   S32 y = getGame()->getSettings()->getIniSettings()->showWeaponIndicators ? messageMargin : vertMargin;
   S32 msgCount;

   msgCount = MessageDisplayCount;  // Short form

   S32 y_end = y + msgCount * (SERVER_MSG_FONT_SIZE + SERVER_MSG_FONT_GAP);

   for(S32 i = msgCount - 1; i >= 0; i--)
   {
      if(mDisplayMessage[i][0])
      {
         glColor(mDisplayMessageColor[i]);
         //drawString(UserInterface::horizMargin, y, FONTSIZE, mDisplayMessage[i]);
         //y += FONTSIZE + FONT_GAP;
         y += (SERVER_MSG_FONT_SIZE + SERVER_MSG_FONT_GAP)
            * drawWrapText(mDisplayMessage[i], horizMargin, y,
               750, // wrap width
               y_end, // ypos_end
               SERVER_MSG_FONT_SIZE + SERVER_MSG_FONT_GAP, // line height
               SERVER_MSG_FONT_SIZE, // font size
               false); // align top
      }
   }
}


// Render any incoming player chat msgs
void GameUserInterface::renderChatMessageDisplay()
{
   glColor(Colors::white);

   S32 y = chatMessageMargin;
   S32 msgCount;

   if(mMessageDisplayMode == LongFixed)
      msgCount = ChatMessageStoreCount;    // Long form
   else
      msgCount = ChatMessageDisplayCount;  // Short form

   S32 y_end = y - msgCount * (CHAT_FONT_SIZE + CHAT_FONT_GAP);


   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   if(mMessageDisplayMode == ShortTimeout)
      for(S32 i = 0; i < msgCount; i++)
      {
         if(mDisplayChatMessage[i][0])
         {
            if (mHelper)   // fade out text if a helper menu is active
               glColor(mDisplayChatMessageColor[i], 0.2f);
            else
               glColor(mDisplayChatMessageColor[i]);

            //drawString(horizMargin, y, CHAT_FONTSIZE, mDisplayChatMessage[i]);
            y -= (CHAT_FONT_SIZE + CHAT_FONT_GAP) * drawWrapText(mDisplayChatMessage[i], horizMargin, y,
                  700, // wrap width
                  y_end, // ypos_end
                  CHAT_FONT_SIZE + CHAT_FONT_GAP, // line height
                  CHAT_FONT_SIZE, // font size
                  CHAT_MULTILINE_INDENT, // how much extra to indent if chat has muliple lines
                  true); // align bottom
         }
      }
   else
      for(S32 i = 0; i < msgCount; i++)
      {
         if(mStoreChatMessage[i][0])
         {
            if (mHelper)   // fade out text if a helper menu is active
               glColor(mStoreChatMessageColor[i], 0.2f);
            else
               glColor(mStoreChatMessageColor[i]);

            //drawString(horizMargin, y, CHAT_FONTSIZE, mStoreChatMessage[i]);
            y -= (CHAT_FONT_SIZE + CHAT_FONT_GAP) * drawWrapText(mStoreChatMessage[i], horizMargin, y,
                  700, // wrap width
                  y_end, // ypos_end
                  CHAT_FONT_SIZE + CHAT_FONT_GAP, // line height
                  CHAT_FONT_SIZE, // font size
                  CHAT_MULTILINE_INDENT, // how much extra to indent if chat has muliple lines
                  true); // align bottom
         }
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
                                 mMousePoint.x + gScreenInfo.getGameCanvasWidth() / 2 - p.x);
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
   TNLAssert(mode != ChatMode, "Should not called to enter chat mode!");

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
void GameUserInterface::setBusyChatting(bool busy)
{
   if(getGame()->getConnectionToServer())
      getGame()->getConnectionToServer()->c2sSetIsBusy(busy);
}


// Select next weapon
void GameUserInterface::advanceWeapon()
{
   GameType *gameType = getGame()->getGameType();
   if(gameType)
      gameType->c2sAdvanceWeapon();
}


// Select a weapon by its index
void GameUserInterface::selectWeapon(U32 indx)
{
   GameType *gameType = getGame()->getGameType();
   if(gameType)
      gameType->c2sSelectWeapon(indx);
}


// Key pressed --> take action!
// Handles all keypress events, including mouse clicks and controller button presses
void GameUserInterface::onKeyDown(InputCode inputCode, char ascii)
{
   Parent::onKeyDown(inputCode, ascii);

   if(OGLCONSOLE_ProcessBitfighterKeyEvent(inputCode, ascii))   // Pass the key on to the console for processing
   {
      // Do nothing... key processed
   }
   else if(inputCode == keyHELP)          // Turn on help screen
   {
      playBoop();

      // If we have a helper, let that determine what happens when the help key is pressed.  Otherwise, show help normally.
      if(mHelper)
         mHelper->activateHelp(getUIManager());
      else
         getUIManager()->getInstructionsUserInterface()->activate();
   }

   // Ctrl-/ toggles console window for the moment
   // Only open when not in any special mode.
   else if(!mHelper && inputCode == KEY_SLASH && checkModifier(KEY_CTRL))
   {
      if(OGLCONSOLE_GetVisibility())      // Hide console if it's visible...
         OGLCONSOLE_HideConsole();
      else                                // ...and show it if it's not
         OGLCONSOLE_ShowConsole();     
   }
   else if(inputCode == keyOUTGAMECHAT)
   {
      setBusyChatting(true);
      getUIManager()->getChatUserInterface()->activate();
   }
   else if(inputCode == keyDIAG)            // Turn on diagnostic overlay
      getUIManager()->getDiagnosticUserInterface()->activate();
   else if(inputCode == keyMISSION)
   {
      mMissionOverlayActive = true;
      getUIManager()->getGameUserInterface()->clearLevelInfoDisplayTimer();    // Clear level-start display if user hits F2
   }
   else if(inputCode == KEY_M && checkModifier(KEY_CTRL))    // Ctrl-M, for now, to cycle through message dispaly modes
   {
      S32 m = mMessageDisplayMode + 1;
      if(m >= MessageDisplayModes)
         m = 0;
      mMessageDisplayMode = MessageDisplayMode(m);
   }
   else if(mHelper && mHelper->processInputCode(inputCode))   // Will return true if key was processed
   {
      // Experimental, to keep ship from moving after entering a quick chat that has the same sortcut as a movement key
      setInputCodeState(inputCode, false);      
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
            InputMode inputMode = getGame()->getSettings()->getIniSettings()->inputMode;

            if( (inputCode == inputMOD1[inputMode] && ship->getModule(0) == ModuleEngineer) ||
                (inputCode == inputMOD2[inputMode] && ship->getModule(1) == ModuleEngineer) )
            {
               string msg = EngineerModuleDeployer::checkResourcesAndEnergy(ship);      // Returns "" if ok, error message otherwise

               if(msg != "")
                  displayErrorMessage(msg.c_str());
               else
                  enterMode(EngineerMode);

               return;
            }
         }
      }

      if(mCurrentChatType != NoChat)
         processChatModeKey(inputCode, ascii);
      else   
         processPlayModeKey(inputCode, ascii);    // A non-chat key, really
   }
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

   Vector<U32> loadout(ShipModuleCount + ShipWeaponCount);
   ship->getLoadout(loadout);

   game->getSettings()->setLoadoutPreset(slot, loadout);
   game->displaySuccessMessage(("Current loadout saved as preset " + itos(slot + 1)).c_str());
}


static void loadLoadoutPreset(ClientGame *game, S32 slot)
{
   Vector<U32> loadout(ShipModuleCount + ShipWeaponCount);     // Define it
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


// Can only get here if we're not in chat mode
void GameUserInterface::processPlayModeKey(InputCode inputCode, char ascii)
{
   InputMode inputMode = getGame()->getSettings()->getIniSettings()->inputMode;
   // The following keys are allowed in both play mode and in
   // loadout or engineering menu modes if not used in the loadout
   // menu above

   if(inputCode == KEY_CLOSEBRACKET && checkModifier(KEY_ALT))           // Alt-] advances bots by one step if frozen
      EventManager::get()->addSteps(1);
   else if(inputCode == KEY_CLOSEBRACKET && checkModifier(KEY_CTRL))     // Ctrl-] advances bots by 10 steps if frozen
      EventManager::get()->addSteps(10);
   else if(inputCode == KEY_1 && checkModifier(KEY_CTRL))              // Ctrl-1 saves loadout preset in slot 1 (with index 0, of course!)
      saveLoadoutPreset(getGame(), 0);
   else if(inputCode == KEY_1 && checkModifier(KEY_ALT))               // Alt-1 loads preset from slot 1 (with index 0, of course!)
      loadLoadoutPreset(getGame(), 0);
   else if(inputCode == KEY_2 && checkModifier(KEY_CTRL))              
      saveLoadoutPreset(getGame(), 1);
   else if(inputCode == KEY_2 && checkModifier(KEY_ALT))             
      loadLoadoutPreset(getGame(), 1);
   else if(inputCode == KEY_3 && checkModifier(KEY_CTRL))              
      saveLoadoutPreset(getGame(), 2);
   else if(inputCode == KEY_3 && checkModifier(KEY_ALT))             
      loadLoadoutPreset(getGame(), 2);

   else if(inputCode == inputMOD1[inputMode])
   {
      mModPrimaryActivated[0] = true;
      // If double-click timer hasn't run out, activate the secondary active component
      if (mModuleOneDoubleClickTimer.getCurrent() != 0)
      {
         mModSecondaryActivated[0] = true;
         mModuleOneDoubleClickTimer.clear();
      }
   }
   else if(inputCode == inputMOD2[inputMode])
   {
      mModPrimaryActivated[1] = true;
      // If double-click timer hasn't run out, activate the secondary active component
      if (mModuleTwoDoubleClickTimer.getCurrent() != 0)
      {
         mModSecondaryActivated[1] = true;
         mModuleTwoDoubleClickTimer.clear();
      }
   }
   else if(inputCode == inputFIRE[inputMode])
      mFiring = true;
   else if(inputCode == inputSELWEAP1[inputMode])
      selectWeapon(0);
   else if(inputCode == inputSELWEAP2[inputMode])
      selectWeapon(1);
   else if(inputCode == inputSELWEAP3[inputMode])
      selectWeapon(2);
   else if(inputCode == keyFPS)
      mFPSVisible = !mFPSVisible;
   else if(inputCode == inputADVWEAP[inputMode])
      advanceWeapon();
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

         return;
      }
      else if(mShutdownMode == Canceled)
      {
         mShutdownMode = None;
         return;
      }

      playBoop();

      if(!getGame()->isConnectedToServer())      // Perhaps we're still joining?
      {
         getGame()->closeConnectionToGameServer();
         getUIManager()->getMainMenuUserInterface()->activate();     // Back to main menu
      }
      else
      {
         setBusyChatting(true);
         getUIManager()->getGameMenuUserInterface()->activate();
      }
   }     
   else if(inputCode == inputCMDRMAP[inputMode])
      getGame()->zoomCommanderMap();

   else if(inputCode == inputSCRBRD[inputMode])
   {     // (braces needed)
      if(!mInScoreboardMode)    // We're activating the scoreboard
      {
         mInScoreboardMode = true;
         GameType *gameType = getGame()->getGameType();
         if(gameType)
            gameType->c2sRequestScoreboardUpdates(true);
      }
   }
   else if(inputCode == inputTOGVOICE[inputMode])
   {     // (braces needed)
      if(!mVoiceRecorder.mRecordingAudio)  // Turning recorder on
         mVoiceRecorder.start();
   }
   else if(!mHelper)    // The following keys are only allowed in PlayMode
   {
      if(inputCode == inputTEAMCHAT[inputMode])          // Start entering a team chat msg
      {
         mCurrentChatType = TeamChat;
         setBusyChatting(true);
      }
      else if(inputCode == inputGLOBCHAT[inputMode])     // Start entering a global chat msg
      {
         mCurrentChatType = GlobalChat;
         setBusyChatting(true);
      }
      else if(inputCode == inputCMDCHAT[inputMode])      // Start entering a command
      {
         mCurrentChatType = CmdChat;
         setBusyChatting(true);
      }
      else if(inputCode == inputQUICKCHAT[inputMode])
         enterMode(QuickChatMode);
      else if(inputCode == inputLOADOUT[inputMode])
         enterMode(LoadoutMode);
      else if(inputCode == inputDROPITEM[inputMode])
         dropItem();
      else if(inputMode == InputModeJoystick)      // Check if the user is trying to use keyboard to move when in joystick mode
         if(inputCode == inputUP[InputModeKeyboard]   || inputCode == inputDOWN[InputModeKeyboard] || 
            inputCode == inputLEFT[InputModeKeyboard] || inputCode == inputRIGHT[InputModeKeyboard])
            mWrongModeMsgDisplay.reset(WRONG_MODE_MSG_DISPLAY_TIME);
   }
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
void GameUserInterface::addTimeHandler(ClientGame *game, const Vector<string> &words)
{
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


void GameUserInterface::sVolHandler(ClientGame *game, const Vector<string> &words)
{
   game->setVolume(SfxVolumeType, words);
}

void GameUserInterface::mVolHandler(ClientGame *game, const Vector<string> &words)
{
   game->setVolume(MusicVolumeType, words);
}

void GameUserInterface::vVolHandler(ClientGame *game, const Vector<string> &words)
{
   game->setVolume(VoiceVolumeType, words);
}

void GameUserInterface::servVolHandler(ClientGame *game, const Vector<string> &words)
{
   game->setVolume(ServerAlertVolumeType, words);
}


void GameUserInterface::getMapHandler(ClientGame *game, const Vector<string> &words)
{
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


void GameUserInterface::nextLevelHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! You don't have permission to change levels"))
      game->getConnectionToServer()->c2sRequestLevelChange(ServerGame::NEXT_LEVEL, false);
}


void GameUserInterface::prevLevelHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! You don't have permission to change levels"))
      game->getConnectionToServer()->c2sRequestLevelChange(ServerGame::PREVIOUS_LEVEL, false);
}


void GameUserInterface::restartLevelHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! You don't have permission to change levels"))
      game->getConnectionToServer()->c2sRequestLevelChange(ServerGame::REPLAY_LEVEL, false);
}


void GameUserInterface::shutdownServerHandler(ClientGame *game, const Vector<string> &words)
{
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


void GameUserInterface::kickPlayerHandler(ClientGame *game, const Vector<string> &words)
{
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


void GameUserInterface::submitPassHandler(ClientGame *game, const Vector<string> &words)
{
   GameConnection *conn = game->getConnectionToServer();

   conn->submitPassword(words[1].c_str());
}

void GameUserInterface::showCoordsHandler(ClientGame *game, const Vector<string> &words)
{
   game->toggleShowingShipCoords();
}


void GameUserInterface::showZonesHandler(ClientGame *game, const Vector<string> &words)
{
   if(!(gServerGame))// && gServerGame->isTestServer()))  sam: problem with not being able to test from editor due to editor crashing and loading improperly...
      game->displayErrorMessage("!!! Zones can only be displayed on a local host");
   else
      game->toggleShowingMeshZones();
}


extern bool showDebugBots;  // in game.cpp

void GameUserInterface::showPathsHandler(ClientGame *game, const Vector<string> &words)
{
   if(!(gServerGame && gServerGame->isTestServer())) 
      game->displayErrorMessage("!!! Robots can only be shown on a test server");
   else
      showDebugBots = !showDebugBots;
}


void GameUserInterface::pauseBotsHandler(ClientGame *game, const Vector<string> &words)
{
   if(!(gServerGame && gServerGame->isTestServer())) 
      game->displayErrorMessage("!!! Robots can only be frozen on a test server");
   else
      EventManager::get()->togglePauseStatus();
}


void GameUserInterface::stepBotsHandler(ClientGame *game, const Vector<string> &words)
{
   if(!(gServerGame && gServerGame->isTestServer())) 
      game->displayErrorMessage("!!! Robots can only be stepped on a test server");
   else
   {
      S32 steps = words.size() > 1 ? atoi(words[1].c_str()) : 1;
      EventManager::get()->addSteps(steps);
   }
}


void GameUserInterface::setAdminPassHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to set the admin password"))
      game->changePassword(GameConnection::AdminPassword, words, true);
}


void GameUserInterface::setServerPassHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to set the server password"))
      game->changePassword(GameConnection::ServerPassword, words, false);
}


void GameUserInterface::setLevPassHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to set the level change password"))
      game->changePassword(GameConnection::LevelChangePassword, words, false);
}


void GameUserInterface::setServerNameHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to set the server name"))
      game->changeServerParam(GameConnection::ServerName, words);
}


void GameUserInterface::setServerDescrHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to set the server description"))
      game->changeServerParam(GameConnection::ServerDescr, words);
}


void GameUserInterface::setLevelDirHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to set the leveldir param"))
      game->changeServerParam(GameConnection::LevelDir, words);
}


void GameUserInterface::deleteCurrentLevelHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to delete the current level"))
      game->changeServerParam(GameConnection::DeleteLevel, words);    // handles deletes too
}


void GameUserInterface::suspendHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->getPlayerCount() > 1)
      game->displayErrorMessage("!!! Can't suspend when others are playing");
   else
      game->suspendGame();
}


extern S32 LOADOUT_PRESETS;

void GameUserInterface::showPresetsHandler(ClientGame *game, const Vector<string> &words)
{
   Vector<U32> preset(ShipModuleCount + ShipWeaponCount);

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


void GameUserInterface::lineWidthHandler(ClientGame *game, const Vector<string> &words)
{
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


void GameUserInterface::maxFpsHandler(ClientGame *game, const Vector<string> &words)
{
   S32 number = words.size() > 1 ? atoi(words[1].c_str()) : 0;

   if(number < 1)                              // Don't allow zero or negative numbers
      game->displayErrorMessage("!!! Usage: /maxfps <frame rate>, default = 100");
   else
      game->getSettings()->getIniSettings()->maxFPS = number;
}


void GameUserInterface::pmHandler(ClientGame *game, const Vector<string> &words)
{
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


void GameUserInterface::muteHandler(ClientGame *game, const Vector<string> &words)
{
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


void GameUserInterface::setTimeHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Enter time in minutes");
         return;
      }

      S32 timeMillis = S32(60 * 1000 * atof(words[1].c_str()));

      if(timeMillis < 0 || (timeMillis == 0 && stricmp(words[1].c_str(), "0") && stricmp(words[1].c_str(), "unlim")) )  // 0 --> unlimited
      {
         game->displayErrorMessage("!!! Invalid time... game time not changed");
         return;
      }

      if(game->getGameType())
         game->getGameType()->c2sSetTime(timeMillis);
   }
}


void GameUserInterface::setWinningScoreHandler(ClientGame *game, const Vector<string> &words)
{
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
         if(game->getGameType()->getGameType() == CoreGame)
            game->displayErrorMessage("!!! Cannot change score in Core game type");
         else
            game->getGameType()->c2sSetWinningScore(score);
      }
   }
}


void GameUserInterface::resetScoreHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permission"))
   {
      if(game->getGameType())
      {
         if(game->getGameType()->getGameType() == CoreGame)
            game->displayErrorMessage("!!! Cannot change score in Core game type");
         else
            game->getGameType()->c2sResetScore();
      }
   }
}


void GameUserInterface::addBotHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permissions to add a bot"))
   {
      // Build args by skipping first word (the command)
      Vector<StringTableEntry> args;
      for(S32 i = 1; i < words.size(); i++)
         args.push_back(StringTableEntry(words[i]));

      if(game->getGameType())
         game->getGameType()->c2sAddBot(args);
   }

}


void GameUserInterface::addBotsHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permissions to add a bots"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Enter number of bots to add");
         return;
      }

      S32 count = 0;
      count = atoi(words[1].c_str());

      if(count <= 0 || count >= 0x00010000)
      {
         game->displayErrorMessage("!!! Invalid number of bots to add");
         return;
      }

      // Build args by skipping first two words (the command, and count)
      Vector<StringTableEntry> args;
      for(S32 i = 2; i < words.size(); i++)
         args.push_back(StringTableEntry(words[i]));

      if(game->getGameType())
         game->getGameType()->c2sAddBots(count, args);
   }

}


void GameUserInterface::kickBotHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permissions to kick a bot"))
   {
      if(game->getGameType())
         game->getGameType()->c2sKickBot();
   }
}


void GameUserInterface::kickBotsHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permissions to kick all bots"))
   {
      if(game->getGameType())
         game->getGameType()->c2sKickBots();
   }
}


void GameUserInterface::showBotsHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->getGameType())
      game->getGameType()->c2sShowBots();
}


void GameUserInterface::setMaxBotsHandler(ClientGame *game, const Vector<string> &words)
{
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


void GameUserInterface::shuffleTeams(ClientGame *game, const Vector<string> &words)
{
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


void GameUserInterface::banPlayerHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! Need admin permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! /ban <player name> [duration in minutes]");
         return;
      }

      ClientInfo *banedClientInfo = game->findClientInfo(words[1].c_str());

      if(!banedClientInfo)
      {
         game->displayErrorMessage("!!! Player name not found");
         return;
      }

      if(banedClientInfo->isAdmin())
      {
         game->displayErrorMessage("!!! Cannot ban an admin");
         return;
      }

      if(!banedClientInfo->getConnection()->isEstablished())
      {
         game->displayErrorMessage("!!! Cannot ban robots, you silly fool!");
         return;
      }

      S32 banDuration = 0;
      if (words.size() > 2)
         banDuration = atoi(words[2].c_str());

      if(game->getGameType())
         game->getGameType()->c2sBanPlayer(words[1].c_str(), banDuration);
   }
}


void GameUserInterface::banIpHandler(ClientGame *game, const Vector<string> &words)
{
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


void GameUserInterface::renamePlayerHandler(ClientGame *game, const Vector<string> &words)
{
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


void GameUserInterface::globalMuteHandler(ClientGame *game, const Vector<string> &words)
{
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

      if(game->getGameType())
         game->getGameType()->c2sGlobalMutePlayer(words[1].c_str());
   }
}


// Use this method when you need to keep client/server compatibility between bitfighter
// versions (e.g. 015 -> 015a)
// If you are working on a new version (e.g. 016), then create an appropriate c2s handler
// function
void GameUserInterface::serverCommandHandler(ClientGame *game, const Vector<string> &words)
{
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
   //  cmdName          cmdCallback               cmdArgInfo cmdArgCount   helpCategory  helpGroup lines,  helpArgString            helpTextSstring
   { "password",GameUserInterface::submitPassHandler,{ STR },      1,       ADV_COMMANDS,    0,     1,     {"<password>"},         "Request admin or level change permissions"  },
   { "servvol", GameUserInterface::servVolHandler,   { INT },      1,       ADV_COMMANDS,    0,     1,     {"<0-10>"},             "Set volume of server"  },
   { "getmap",  GameUserInterface::getMapHandler,    { STR },      1,       ADV_COMMANDS,    1,     1,     {"[file]"},             "Save currently playing level in [file], if allowed" },
   { "suspend", GameUserInterface::suspendHandler,   {  },         0,       ADV_COMMANDS,    1,     1,     {  },                   "Place game on hold while waiting for players" },
   { "pm",      GameUserInterface::pmHandler,        { NAME, STR },2,       ADV_COMMANDS,    1,     1,     {"<name>","<message>"}, "Send private message to player" },
   { "mvol",    GameUserInterface::mVolHandler,      { INT },      1,       ADV_COMMANDS,    2,     1,     {"<0-10>"},             "Set music volume"      },
   { "svol",    GameUserInterface::sVolHandler,      { INT },      1,       ADV_COMMANDS,    2,     1,     {"<0-10>"},             "Set SFX volume"        },
   { "vvol",    GameUserInterface::vVolHandler,      { INT },      1,       ADV_COMMANDS,    2,     1,     {"<0-10>"},             "Set voice chat volume" },
   { "mute",    GameUserInterface::muteHandler,      { NAME },     1,       ADV_COMMANDS,    3,     1,     {"<name>"},             "Hide chat messages from <name>. Run again to un-hide" },

   { "add",         GameUserInterface::addTimeHandler,         { INT },           0,      LEVEL_COMMANDS,  0,  1, {"<time in minutes>"},                  "Add time to the current game" },
   { "next",        GameUserInterface::nextLevelHandler,       {  },              0,      LEVEL_COMMANDS,  0,  1, {  },                                   "Start next level" },
   { "prev",        GameUserInterface::prevLevelHandler,       {  },              0,      LEVEL_COMMANDS,  0,  1, {  },                                   "Replay previous level" },
   { "restart",     GameUserInterface::restartLevelHandler,    {  },              0,      LEVEL_COMMANDS,  0,  1, {  },                                   "Restart current level" },
   { "settime",     GameUserInterface::setTimeHandler,         { INT },           1,      LEVEL_COMMANDS,  0,  1, {"<time in minutes>"},                  "Set play time for the level" },
   { "setwinscore", GameUserInterface::setWinningScoreHandler, { INT },           1,      LEVEL_COMMANDS,  0,  1, {"<score>"},                            "Set score to win the level" },
   { "resetscore",  GameUserInterface::resetScoreHandler,      {  },              0,      LEVEL_COMMANDS,  0,  1, {  },                                   "Reset all scores to zero" },
   { "addbot",      GameUserInterface::addBotHandler,          { TEAM, STR, STR },3,      LEVEL_COMMANDS,  1,  1, {"[team]","[file]","[args]"},           "Add a bot from [file] to [team], pass [args] to bot" },
   { "addbots",     GameUserInterface::addBotsHandler,         { INT, TEAM, STR, STR },4, LEVEL_COMMANDS,  1, 2,  {"[count]","[team]","[file]","[args]"}, "Add a [count] of bots from [file] to [team], pass [args] to bot" },
   { "kickbot",     GameUserInterface::kickBotHandler,         {  },              1,      LEVEL_COMMANDS,  1, 1,  {  },                                   "Kick most recently added bot" },
   { "kickbots",    GameUserInterface::kickBotsHandler,        {  },              1,      LEVEL_COMMANDS,  1, 1,  {  },                                   "Kick all bots" },

   { "kick",               GameUserInterface::kickPlayerHandler,         { NAME },      1,ADMIN_COMMANDS, 0,  1,  {"<name>"},              "Kick a player from the game" },
   { "ban",                GameUserInterface::banPlayerHandler,          { NAME, INT }, 2,ADMIN_COMMANDS, 0,  1,  {"<name>","[duration]"}, "Ban a player from the server (IP-based, def. = 60 mins)" },
   { "banip",              GameUserInterface::banIpHandler,              { STR, INT },  2,ADMIN_COMMANDS, 0,  1,  {"<ip>","[duration]"},   "Ban an IP address from the server (def. = 60 mins)" },
   { "shutdown",           GameUserInterface::shutdownServerHandler,     { INT, STR },  2,ADMIN_COMMANDS, 0,  1,  {"[time]","[message]"},  "Start orderly shutdown of server (def. = 10 secs)" },
   { "setlevpass",         GameUserInterface::setLevPassHandler,         { STR },       1,ADMIN_COMMANDS, 0,  1,  {"[passwd]"},            "Set server password (use blank to clear)" },
   { "setadminpass",       GameUserInterface::setAdminPassHandler,       { STR },       1,ADMIN_COMMANDS, 0,  1,  {"[passwd]"},            "Set level change password (use blank to clear)" },
   { "setserverpass",      GameUserInterface::setServerPassHandler,      { STR },       1,ADMIN_COMMANDS, 0,  1,  {"<passwd>"},            "Set admin password" },
   { "leveldir",           GameUserInterface::setLevelDirHandler,        { STR },       1,ADMIN_COMMANDS, 0,  1,  {"<new level folder>"},  "Set leveldir param on the server (changes levels available)" },
   { "setservername",      GameUserInterface::setServerNameHandler,      { STR },       1,ADMIN_COMMANDS, 0,  1,  {"<name>"},              "Set server name" },
   { "setserverdescr",     GameUserInterface::setServerDescrHandler,     { STR },       1,ADMIN_COMMANDS, 0,  1,  {"<descr>"},             "Set server description" },
   { "deletecurrentlevel", GameUserInterface::deleteCurrentLevelHandler, { },           0,ADMIN_COMMANDS, 0,  1,  {""},                    "Remove current level from server" },
   { "gmute",              GameUserInterface::globalMuteHandler,         { NAME },      1,ADMIN_COMMANDS, 0,  1,  {"<name>"},              "Globally mute/unmute a player" },
   { "rename",             GameUserInterface::renamePlayerHandler,       { NAME, STR }, 2,ADMIN_COMMANDS, 0,  1,  {"<from>","<to>"},   "Give a player a new name" },
   { "maxbots",            GameUserInterface::setMaxBotsHandler,         { INT },       1,ADMIN_COMMANDS, 0,  1,  {"<count>"},             "Set the maximum bots allowed for this server" },
   { "shuffle",            GameUserInterface::shuffleTeams,              { },           0,ADMIN_COMMANDS, 0,  1,   { "" },                 "Randomly reshuffle teams" },

   { "showcoords", GameUserInterface::showCoordsHandler,    {  },    0,DEBUG_COMMANDS, 0,  1, {  },         "Show ship coordinates" },
   { "showzones",  GameUserInterface::showZonesHandler,     {  },    0,DEBUG_COMMANDS, 0,  1, {  },         "Show bot nav mesh zones" },
   { "showpaths",  GameUserInterface::showPathsHandler,     {  },    0,DEBUG_COMMANDS, 0,  1, {  },         "Show robot paths" },
   { "showbots",   GameUserInterface::showBotsHandler,      {  },    0,DEBUG_COMMANDS, 0,  1, {  },         "Show all robots" },
   { "pausebots",  GameUserInterface::pauseBotsHandler,     {  },    0,DEBUG_COMMANDS, 0,  1, {  },         "Pause all bots. Reissue to start again" },
   { "stepbots",   GameUserInterface::stepBotsHandler,      { INT }, 1,DEBUG_COMMANDS, 1,  1, {"[steps]"},  "Advance bots by number of steps (def. = 1)"},
   { "linewidth",  GameUserInterface::lineWidthHandler,     { INT }, 1,DEBUG_COMMANDS, 1,  1, {"[number]"}, "Change width of all lines (def. = 2)" },
   { "maxfps",     GameUserInterface::maxFpsHandler,        { INT }, 1,DEBUG_COMMANDS, 1,  1, {"<number>"}, "Set maximum speed of game in frames per second" },
};



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

   const S32 ypos = chatMessageMargin + CHAT_FONT_SIZE + (2 * CHAT_FONT_GAP) + 5;

   S32 boxWidth = gScreenInfo.getGameCanvasWidth() - 2 * horizMargin - (nameWidth - promptSize) - 230;

   // Render text entry box like thingy
   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   for(S32 i = 1; i >= 0; i--)
   {
      glColor(baseColor, i ? .25f : .4f);

      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2i(horizMargin, ypos - 3);
         glVertex2i(horizMargin + boxWidth, ypos - 3);
         glVertex2i(horizMargin + boxWidth, ypos + CHAT_FONT_SIZE + 7);
         glVertex2i(horizMargin, ypos + CHAT_FONT_SIZE + 7);
      glEnd();
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
               if(chatCmds[i].cmdArgCount >= words.size() && line[line.size() - 1] == ' ')
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
         else
            TNLAssert(false, "Invalid argType!");
      }
   }
   
   return NULL;                        // ==> No completion options
}


void GameUserInterface::processChatModeKey(InputCode inputCode, char ascii)
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
         size_t lastChar = string::npos;
         if(entry->find_first_of("\"") != string::npos)
            lastChar = entry->find_first_of("\"");

         string appender = " ";

         size_t pos = entry->find_last_of(' ', lastChar);

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
   else if(ascii)     // Append any other keys to the chat message
   {
      // Protect against crashes while game is initializing (because we look at the ship for the player's name)
      if(getGame()->getConnectionToServer())     // getGame() cannot return NULL here
      {
//         S32 promptSize = getStringWidth(CHAT_FONT_SIZE, mCurrentChatType == TeamChat ? "(Team): " : "(Global): ");
//
//         S32 nameSize = getStringWidthf(CHAT_FONT_SIZE, "%s: ", getGame()->getClientInfo()->getName().getString());
//         S32 nameWidth = max(nameSize, promptSize);
         // Above block repeated above

         mLineEditor.addChar(ascii);
      }
   }
}


void GameUserInterface::onKeyUp(InputCode inputCode)
{
   S32 inputMode = getGame()->getSettings()->getIniSettings()->inputMode;

   // These keys works in any mode!  And why not??

   if(inputCode == keyMISSION)
      mMissionOverlayActive = false;
   else if (inputCode == inputMOD1[inputMode])
   {
      mModPrimaryActivated[0] = false;
      mModSecondaryActivated[0] = false;
      mModuleOneDoubleClickTimer.reset();
   }
   else if (inputCode == inputMOD2[inputMode])
   {
      mModPrimaryActivated[1] = false;
      mModSecondaryActivated[1] = false;
      mModuleTwoDoubleClickTimer.reset();
   }
   else if (inputCode == inputFIRE[inputMode])
      mFiring = false;
   else if(inputCode == inputSCRBRD[inputMode])
   {     // (braces required)
      if(mInScoreboardMode)     // We're turning scoreboard off
      {
         mInScoreboardMode = false;
         GameType *gameType = getGame()->getGameType();
         if(gameType)
            gameType->c2sRequestScoreboardUpdates(false);
      }
   }
   else if(inputCode == inputTOGVOICE[inputMode])
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
   if((mCurrentChatType == NoChat) && !mDisableShipKeyboardInput && !OGLCONSOLE_GetVisibility())
   {
      InputMode inputMode = getGame()->getSettings()->getIniSettings()->inputMode;

      // Some helpers (like QuickChat or TeamShuffle) like to disable movement when they are active
      if(mHelper && mHelper->isMovementDisabled())
      {
         mCurrentMove.x = 0;
         mCurrentMove.y = 0;
      }
      else
      {
         mCurrentMove.x = F32((getInputCodeState(inputRIGHT[inputMode]) ? 1 : 0) - (getInputCodeState(inputLEFT[inputMode]) ? 1 : 0));
         mCurrentMove.y = F32((getInputCodeState(inputDOWN[inputMode])  ? 1 : 0) - (getInputCodeState(inputUP[inputMode]) ? 1 : 0));
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
      if(words[0] == chatCmds[i].cmdName)
      {
         chatCmds[i].cmdCallback(getGame(), words);
         return;
      }

   serverCommandHandler(getGame(), words);     // Command unknown to client, will pass it on to server
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
      getGame()->getSettings()->getIniSettings()->musicVolLevel = (F32) vol / 10.f;
      displayMessagef(gCmdChatColor, "Music volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
      return;

   case VoiceVolumeType:
      getGame()->getSettings()->getIniSettings()->voiceChatVolLevel = (F32) vol / 10.f;
      displayMessagef(gCmdChatColor, "Voice chat volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
      return;

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

      glColor3f(1, 1 ,1);
      glBegin(GL_LINES);
      glVertex2i(10, 130);
      glVertex2i(10, 145);
      glVertex2i(10 + totalLineCount * 2, 130);
      glVertex2i(10 + totalLineCount * 2, 145);

      F32 halfway = totalLineCount * 0.5f;
      F32 full = amt * totalLineCount;
      for(U32 i = 1; i < full; i++)
      {
         if(i < halfway)
            glColor3f(i / halfway, 1, 0);
         else
            glColor3f(1, 1 - (i - halfway) / halfway, 0);

         glVertex2i(10 + i * 2, 130);
         glVertex2i(10 + i * 2, 145);
      }
      glEnd();
   }
}


void GameUserInterface::VoiceRecorder::start()
{
   mWantToStopRecordingAudio = 0; // linux repeadedly sends key-up / key-down when only holding key down
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
   mWantToStopRecordingAudio = 2;
}


void GameUserInterface::VoiceRecorder::process()
{
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
   getGame()->getConnectionToServer()->suspendGame();     // Tell server we're suspending
   getGame()->suspendGame();                              // Suspend locally
   getUIManager()->getSuspendedUserInterface()->activate();          // And enter chat mode
}


void GameUserInterface::unsuspendGame()
{
   getGame()->unsuspendGame();                            // Unsuspend locally
   getGame()->getConnectionToServer()->unsuspendGame();   // Tell the server we're unsuspending
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

   bool isTeamGame = gameType->isTeamGame();

#ifdef USE_DUMMY_PLAYER_SCORES
   S32 maxTeamPlayers = getDummyMaxPlayers();
   S32 teams = isTeamGame ? getDummyTeamCount() : 1;
#else
   getGame()->countTeamPlayers();

   S32 teams = isTeamGame ? getGame()->getTeamCount() : 1;
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

   U32 numTeamRows = (teams + 1) >> 1;

   U32 totalHeight = (gScreenInfo.getGameCanvasHeight() - vertMargin * 2) / numTeamRows - (numTeamRows - 1) * 2;
   U32 maxHeight = MIN(30, (totalHeight - teamAreaHeight) / maxTeamPlayers);

   U32 sectionHeight = (teamAreaHeight + maxHeight * maxTeamPlayers);
   totalHeight = sectionHeight * numTeamRows + (numTeamRows - 1) * 2;

   for(S32 i = 0; i < teams; i++)
   {
      S32 yt = (gScreenInfo.getGameCanvasHeight() - totalHeight) / 2 + (i >> 1) * (sectionHeight + 2);  // y-top
      S32 yb = yt + sectionHeight;     // y-bottom
      S32 xl = 10 + (i & 1) * teamWidth;
      S32 xr = xl + teamWidth - 2;

      const Color *teamColor = getGame()->getTeamColor(i);

      TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

      glColor(teamColor, 0.6f);
      drawRect(xl, yt, xr, yb, GL_POLYGON);

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

      S32 curRowY = yt + teamAreaHeight + 1;
      S32 fontSize = U32(maxHeight * 0.85f);
      const char *botSymbol = "B ";
      const char *levelChangerSymbol = "+ ";
      const char *adminSymbol = "@ ";

      // Use any symbol for an offset
      S32 symbolFontSize = S32(fontSize * 0.8f);
      S32 symbolSize = getStringWidth(symbolFontSize, botSymbol);

      for(S32 j = 0; j < playerScores.size(); j++)
      {
         S32 x = xl + 40;
         S32 vertAdjustFact = (fontSize - symbolFontSize) / 2 - 1;

         if(playerScores[j]->getBadges() & DEVELOPER_BADGE)
            glColor(Colors::yellow);
         else
            glColor(Colors::white);


         // Add the mark of the bot
         if(playerScores[j]->isRobot())
            drawString(x - symbolSize, curRowY + vertAdjustFact + 2, symbolFontSize, botSymbol);

         // Add level changer mark
         if(playerScores[j]->isLevelChanger() && !playerScores[j]->isAdmin())
            drawString(x - symbolSize, curRowY + vertAdjustFact + 2, symbolFontSize, levelChangerSymbol);

         // Add admin mark
         if(playerScores[j]->isAdmin())
            drawString(x - symbolSize, curRowY + vertAdjustFact + 2, symbolFontSize, adminSymbol);

         drawString(x - 8, curRowY, fontSize, playerScores[j]->getName().getString());

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
}


// Sorts teams by score, high to low
S32 QSORT_CALLBACK teamScoreSort(Team **a, Team **b)
{
   return (*b)->getScore() - (*a)->getScore();  
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
      if(gameType->getGameType() == CoreGame)
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
                                      getGame()->getSettings()->getIniSettings()->inputMode == InputModeJoystick ? "Joystick" : "Keyboard");
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
   const char *gtPrefix = (gameType->canBeIndividualGame() && gameType->getGameType() != SoccerGame && 
                           getGame()->getTeamCount() > 1) ? "Team " : "";

   drawCenteredStringf(yCenter - 140, 30, "Game Type: %s%s", gtPrefix, gameType->getGameTypeString());

   glColor(Colors::cyan, alpha);
   drawCenteredString(yCenter - 100, 20, gameType->getInstructionString());

   glColor(Colors::magenta, alpha);
   drawCenteredString(yCenter - 75, 20, gameType->getLevelDescription()->getString());

   glColor(Colors::menuHelpColor, alpha);
   drawCenteredStringf(canvasHeight - 100, 20, "Press [%s] to see this information again", inputCodeToString(keyMISSION));

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

   // Build a list of teams, so we can sort by score
   Game *game = getGame();
   S32 teamCount = game->getTeamCount();

   static Vector<Team *> teams;
   teams.resize(teamCount);

   for(S32 i = 0; i < teamCount; i++)
   {
      teams[i] = (Team *)game->getTeam(i);
      teams[i]->mId = i;
   }

   teams.sort(teamScoreSort);    

   const S32 textsize = 32;
   S32 xpos = rightAlignCoord - gameType->getDigitsNeededToDisplayScore() * getStringWidth(textsize, "0");

   for(S32 i = 0; i < teams.size(); i++)
   {
      S32 ypos = gScreenInfo.getGameCanvasHeight() - vertMargin - lroff - (teams.size() - i - 1) * 38;

      Team *team = (Team *)game->getTeam(i);

      glColor(Colors::magenta);
      if( gameType->teamHasFlag(team->getId()) )
         drawString(xpos - 50, ypos + 3, 18, "*");

      renderFlag(F32(xpos - 20), F32(ypos + 18), team->getColor());

      glColor(Colors::white);
      drawStringf(xpos, ypos, textsize, "%d", team->getScore());
   }
}


void GameUserInterface::renderCoreScores(const GameType *gameType, U32 rightAlignCoord)
{
   CoreGameType *cgt = static_cast<CoreGameType*>(const_cast<GameType*>(gameType));

   S32 lroff = gameType->getLowerRightCornerScoreboardOffsetFromBottom();

   // Build a list of teams, so we can sort by score
   Game *game = getGame();
   S32 teamCount = game->getTeamCount();

   static Vector<Team *> teams;
   teams.resize(teamCount);

   for(S32 i = 0; i < teamCount; i++)
   {
      teams[i] = (Team *)game->getTeam(i);
      teams[i]->mId = i;
   }

   teams.sort(teamScoreSort);

   const S32 textSize = 32;
   S32 xpos = rightAlignCoord - gameType->getDigitsNeededToDisplayScore() * getStringWidth(textSize, "0");

   // Here we show the number of Cores remaining INSTEAD OF the score, which is negative
   for(S32 i = 0; i < teams.size(); i++)
   {
      S32 ypos = gScreenInfo.getGameCanvasHeight() - vertMargin - lroff - (teams.size() - i - 1) * 38;

      Team *team = (Team *)game->getTeam(i);
      Point center(xpos - 20, ypos + 19);

      renderCore(center, 10, team->getColor(), getGame()->getCurrentTime());

      // Render something if a Core is being attacked
      if(cgt->isTeamCoreBeingAttacked(i))
      {
         if(getGame()->getCurrentTime() % 300 > 150)
         {
            glColor(Colors::red80);
            drawCircle(center, 15);
         }
         else
         {
            glColor(Colors::yellow, 0.6);
            drawCircle(center, 15);
         }
      }

      glColor(Colors::white);
      drawStringf(xpos, ypos, textSize, "%d", cgt->getTeamCoreCount(i));
   }
}


void GameUserInterface::renderLeadingPlayerScores(const GameType *gameType, U32 rightAlignCoord)
{
   Game *game = gameType->getGame();

   S32 lroff = gameType->getLowerRightCornerScoreboardOffsetFromBottom() - 22;

   const S32 textsize = 12;

   /// Render player score
   bool hasLeader = gameType->getLeadingPlayer() >= 0;
   bool hasSecondLeader = gameType->getSecondLeadingPlayer() >= 0;

   const char *clientName = getGame()->getClientInfo()->getName().getString();

   // The player is the leader if a leader is detected and it matches his name
   bool clientIsLeader = hasLeader &&
         !strcmp(clientName, game->getClientInfo(gameType->getLeadingPlayer())->getName().getString());

   // The top rendered name is the leading player or none if no leading player
   // (no scoring even has occured yet)
   const char *nameTop = hasLeader ? game->getClientInfo(gameType->getLeadingPlayer())->getName().getString() : "";
   S32 scoreTop = hasLeader ? gameType->getLeadingPlayerScore() : S32_MIN;

   // The bottom rendered name is either second leader or the current player
   const char *nameBottom = clientIsLeader && hasSecondLeader ?
                                 game->getClientInfo(gameType->getSecondLeadingPlayer())->getName().getString() :
                                 clientName;

   TNLAssert(getGame()->getLocalRemoteClientInfo(), "How did this get to be NULL?");

   S32 scoreBottom;
   if(getGame()->getLocalRemoteClientInfo())
      scoreBottom = clientIsLeader && hasSecondLeader ?
                           gameType->getSecondLeadingPlayerScore() :
                           getGame()->getLocalRemoteClientInfo()->getScore();
   else
      scoreBottom = 0;
                     

   S32 ypos;

   // Render bottom score if player isn't the leader or we have a second leader
   if(!clientIsLeader || hasSecondLeader)
   {
      ypos = gScreenInfo.getGameCanvasHeight() - vertMargin - lroff - 0 * 16;

      glColor(Colors::red, 0.6f);
      drawStringfr(rightAlignCoord, ypos + textsize, textsize, "%s %d", nameBottom, scoreBottom);
   }

   // Render top score only if we have a leader
   if(hasLeader)
   {
      ypos = gScreenInfo.getGameCanvasHeight() - vertMargin - lroff - 1 * 16;

      // Draw leader name + score
      glColor(Colors::red);
      drawStringfr(rightAlignCoord, ypos + textsize, textsize, "%s %d", nameTop, scoreTop);
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

   bool useUnlim = gameType->getTotalGameTime() == 0 && !gameType->isGameOver();    // Time remaining in game
   U32 w = useUnlim ? wUnlim : w00;

   S32 x = rightAlignCoord - w;
   S32 y = gScreenInfo.getGameCanvasHeight() - vertMargin - 20;

   glColor(Colors::cyan);
   drawStringfr(x - 5, y + ((size - gtsize) / 2) + gtsize + 2, gtsize, txt.c_str());
   
   glColor(Colors::white);

   if(useUnlim)  // Don't render "Unlim" when time is 0 because it has expired
      drawString(x, y, size, "Unlim.");
   else
   {
      U32 timeLeft = gameType->getRemainingGameTime();               
      U32 minsRemaining = timeLeft / 60;
      U32 secsRemaining = timeLeft - (minsRemaining * 60);

      drawStringf(x, y, size, "%02d:%02d", minsRemaining, secsRemaining);
   }
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



};

