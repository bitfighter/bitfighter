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

#include "gameConnection.h"

#include "game.h"
#include "UIGame.h"
#include "UIMenus.h"
#include "UIInstructions.h"
#include "UIChat.h"
#include "UIMessage.h"
#include "UIDiagnostics.h"
#include "UIErrorMessage.h"
#include "gameType.h"
#include "lpc10.h"
#include "IniFile.h"             // For access to gINI functions
#include "engineeredObjects.h"   // For EngineerModuleDeployer

#include "../tnl/tnlEndian.h"

#include "ship.h"
#include "shipItems.h"           // For EngineerBuildObjects
#include "gameObjectRender.h"
#include "input.h"
#include "config.h"
#include "loadoutHelper.h"
#include "gameNetInterface.h"

#include "md5wrapper.h"          // For submission of passwords

#include "../glut/glutInclude.h"
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "oglconsole.h"          // Our console object

#include "config.h"              // for Getmap level dir

namespace Zap
{
extern ConfigDirectories gConfigDirs;          //in main.cpp for Getmap

//GameUserInterface mGameUserInterface;

// TODO: Make these static like privateF5MessageDisplayedInGameColor!
Color gGlobalChatColor(0.9, 0.9, 0.9);
Color gTeamChatColor(0, 1, 0);
extern Color gCmdChatColor;


Color GameUserInterface::privateF5MessageDisplayedInGameColor(0, 0, 1);


static void makeCommandCandidateList();      // Forward delcaration

// Constructor
GameUserInterface::GameUserInterface()
{
   mOutputFile = NULL;
   bool mLeftDisabled = false; // Fix some uninitalized variables (randomly was true)
   bool mRightDisabled = false;
   bool mUpDisabled = false;
   bool mDownDisabled = false;
   mInScoreboardMode = false;
   mFPSVisible = false;
   mHelper = NULL;
   displayInputModeChangeAlert = false;
   mMissionOverlayActive = false;
   mDebugShowShipCoords = false;
   mDebugShowMeshZones = false;

   setMenuID(GameUI);
   enterMode(PlayMode);
   mInScoreboardMode = false;

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

   mLineEditor = LineEditor(200);

   // Initialize message buffers
   for(S32 i = 0; i < MessageDisplayCount; i++)
      mDisplayMessage[i][0] = 0;

   for(S32 i = 0; i < MessageStoreCount; i++)
      mStoreMessage[i][0] = 0;

   mGotControlUpdate = false;
   mRecalcFPSTimer = 0;

   mFiring = false;
   for (U32 i = 0; i < (U32)ShipModuleCount; i++)
      mModActivated[i] = false;

   mDisplayMessageTimer.setPeriod(DisplayMessageTimeout);    // Set the period of our message timeout timer

   //populateChatCmdList();

   remoteLevelDownloadFilename = "downloaded.level";

   makeCommandCandidateList();
}


// Destructor
GameUserInterface::~GameUserInterface()
{
   if(mOutputFile)
      fclose(mOutputFile);
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
        if (sscanf(cmd, "add %i %i", &a, &b) == 2)
        {
            OGLCONSOLE_Output(console, "%i + %i = %i\n", a, b, a+b);
            return;
        }

        OGLCONSOLE_Output(console, "usage: add INT INT\n");
    }

    else
      OGLCONSOLE_Output(console, "Unknown command: %s\n", cmd);
}


extern bool gDisableShipKeyboardInput;

void GameUserInterface::onActivate()
{
   gDisableShipKeyboardInput = false;  // Make sure our ship controls are active
   mMissionOverlayActive = false;      // Turn off the mission overlay (if it was on)
   glutSetCursor(GLUT_CURSOR_NONE);    // Turn off cursor
   onMouseMoved();                     // Make sure ship pointed is towards mouse

   // Clear out any lingering chat messages
   for(S32 i = 0; i < MessageStoreCount; i++)
      mDisplayMessage[i][0] = 0;

   for(S32 i = 0; i < MessageDisplayCount; i++)
      mStoreMessage[i][0] = 0;

   mMessageDisplayMode = ShortTimeout;          // Start with normal chat msg display
   enterMode(PlayMode);                         // Make sure we're not in chat or loadout-select mode

   for(S32 i = 0; i < ShipModuleCount; i++)
      mModActivated[i] = false;

   mShutdownMode = None;

   gClientGame->unsuspendGame();                          // Never suspended when we start

   OGLCONSOLE_EnterKey(processGameConsoleCommand);        // Setup callback for processing console commands
}


void GameUserInterface::onReactivate()
{
   if(gClientGame->isSuspended())
      unsuspendGame();

   gDisableShipKeyboardInput = false;
   glutSetCursor(GLUT_CURSOR_NONE);    // Turn off cursor
   enterMode(PlayMode);

   for(S32 i = 0; i < ShipModuleCount; i++)
      mModActivated[i] = false;

   onMouseMoved();   // Call onMouseMoved to get ship pointed at current cursor location
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


extern Color colors[];
void GameUserInterface::displayMessage(GameConnection::MessageColors msgColorIndex, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   vsnprintf(stringBuffer, sizeof(stringBuffer), format, args);
   va_end(args);
   displayMessage(colors[msgColorIndex], stringBuffer);
}


// A new chat message is here!  We don't actually display anything here, despite the name...
// just add it to the list, will be displayed in render()
void GameUserInterface::displayMessage(const Color &msgColor, const char *format, ...)
{
   // Ignore empty message
   if(!strcmp(format, ""))
      return;

   // Create a slot for our new message
   if(mDisplayMessage[0][0])
      for(S32 i = MessageDisplayCount - 1; i > 0; i--)
      {
         strcpy(mDisplayMessage[i], mDisplayMessage[i-1]);
         mDisplayMessageColor[i] = mDisplayMessageColor[i-1];
      }

   for(S32 i = MessageStoreCount - 1; i > 0; i--)
   {
      strcpy(mStoreMessage[i], mStoreMessage[i-1]);
      mStoreMessageColor[i] = mStoreMessageColor[i-1];
   }

   va_list args;

   va_start(args, format);
   vsnprintf(mDisplayMessage[0], sizeof(mDisplayMessage[0]), format, args);
   va_end(args);
   mDisplayMessageColor[0] = msgColor;

   va_start(args, format);
   vsnprintf(mStoreMessage[0], sizeof(mStoreMessage[0]), format, args);
   va_end(args);

   mStoreMessageColor[0] = msgColor;

   mDisplayMessageTimer.reset();
}


void GameUserInterface::idle(U32 timeDelta)
{
   mShutdownTimer.update(timeDelta);

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

   if(mCurrentMode == ChatMode)
      LineEditor::updateCursorBlink(timeDelta);    // Blink the cursor if in ChatMode

   else if(mHelper)
      mHelper->idle(timeDelta);

   mVoiceRecorder.idle(timeDelta);

   U32 indx = mFrameIndex % FPS_AVG_COUNT;
   mIdleTimeDelta[indx] = timeDelta;

   if(gClientGame->getConnectionToServer())
      mPing[indx] = (U32)gClientGame->getConnectionToServer()->getRoundTripTime();

   mFrameIndex++;

   mWrongModeMsgDisplay.update(timeDelta);

   mProgressBarFadeTimer.update(timeDelta);

   // Should we move this timer over to UIGame??
   if(gHostMenuUserInterface.levelLoadDisplayFadeTimer.update(timeDelta))
      gHostMenuUserInterface.clearLevelLoadDisplay();
}


#ifdef TNL_OS_WIN32
extern void checkMousePos(S32 maxdx, S32 maxdy);
#endif


// Draw main game screen (client only)
void GameUserInterface::render()
{
   glColor3f(0.0, 0.0, 0.0);
   if(!gClientGame->isConnectedToServer())
   {
      glColor3f(1, 1, 1);
      drawCenteredString(260, 30, "Connecting to server...");

      glColor3f(0, 1, 0);
      if(gClientGame->getConnectionToServer())
         drawCenteredString(310, 16, gConnectStatesTable[gClientGame->getConnectionToServer()->getConnectionState()]);

      glColor3f(1, 1, 1);
      drawCenteredString(346, 20, "Press <ESC> to abort");
   }

   gClientGame->render();

   // Provide fade in effect as level begins; doesn't work quite right because game is so busy at this point that 
   // it fades in jerkily.

   //glColor4f(0, 0, 0, mProgressBarFadeTimer.getFraction());  
   //glEnableBlend;
   //glBegin(GL_POLYGON);
   //   glVertex2f(0, 0);
   //   glVertex2f(canvasWidth, 0);
   //   glVertex2f(canvasWidth, canvasHeight);
   //   glVertex2f(0, canvasHeight);
   //glEnd();
   //glDisableBlend;

   // TODO: Can delete these two lines???
   //glMatrixMode(GL_MODELVIEW);
   //glLoadIdentity();          // OpenGL command to load an identity matrix (see OpenGL docs)

   if(!gClientGame->isSuspended())
   {
      renderReticle();           // Draw crosshairs if using mouse
      renderMessageDisplay();    // Render incoming chat msgs
      renderCurrentChat();       // Render any chat msg user is composing
      renderLoadoutIndicators(); // Draw indicators for the various loadout items

      gHostMenuUserInterface.renderProgressListItems();  // This is the list of levels loaded while hosting

      renderProgressBar();       // This is the status bar that shows progress of loading this level

      mVoiceRecorder.render();   // This is the indicator that someone is sending a voice msg

      // Display running average FPS
      if(mFPSVisible)
      {
         glColor3f(1, 1, 1);
         drawStringf(gScreenInfo.getGameCanvasWidth() - horizMargin - 220, vertMargin, 20, "%4.1f fps | %1.0f ms", mFPSAvg, mPingAvg);
      }

      // Render QuickChat / Loadout menus
      if(mHelper)
         mHelper->render();

      GameType *theGameType = gClientGame->getGameType();

      if(theGameType)
         theGameType->renderInterfaceOverlay(mInScoreboardMode);

      renderLostConnectionMessage();      // Renders message overlay if we're losing our connection to the server
   }

   renderShutdownMessage();

   renderConsole();  // Rendered last, so it's always on top

#if 0
// Some code for outputting the position of the ship for finding good spawns
GameConnection *con = gClientGame->getConnectionToServer();

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
   GameConnection *connection = gClientGame->getConnectionToServer();
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


void GameUserInterface::shutdownInitiated(U16 time, StringTableEntry who, StringPtr why, bool initiator)
{
   mShutdownMode = ShuttingDown;
   mShutdownName = who;
   mShutdownReason = why;
   mShutdownInitiator = initiator;
   mShutdownTimer.reset(time * 1000);
}


void GameUserInterface::shutdownCanceled()
{
   mShutdownMode = Canceled;
}


// Draws level-load progress bar across the bottom of the screen
void GameUserInterface::renderProgressBar()
{
   GameType *gt = gClientGame->getGameType();
   if((mShowProgressBar || mProgressBarFadeTimer.getCurrent() > 0) && gt && gt->mObjectsExpected > 0)
   {
      glEnableBlend;

      glColor4f(0, 1, 0, mShowProgressBar ? 1 : mProgressBarFadeTimer.getFraction());

      // Outline
      const S32 left = 200;
      const S32 width = gScreenInfo.getGameCanvasWidth() - 2 * left;
      const S32 height = 10;

      // For some reason, there are occasions where the status bar doesn't progress all the way over during the load process.
      // The problem is that, for some reason, some objects do not add themselves to the loaded object counter, and this creates
      // a disconcerting effect, as if the level did not fully load.  Rather than waste any more time on this problem, we'll just
      // fill in the status bar while it's fading, to make it look like the level fully loaded.  Since the only thing that this
      // whole mechanism is used for is to display something to the user, this should work fine.
      S32 barWidth = mShowProgressBar ? S32((F32) width * (F32) gClientGame->mObjectsLoaded / (F32) gt->mObjectsExpected) : width;

      for(S32 i = 1; i >= 0; i--)
      {
         S32 w = i ? width : barWidth;

         glBegin(i ? GL_LINE_LOOP : GL_POLYGON);
            glVertex2f(left,     gScreenInfo.getGameCanvasHeight() - vertMargin);
            glVertex2f(left + w, gScreenInfo.getGameCanvasHeight() - vertMargin);
            glVertex2f(left + w, gScreenInfo.getGameCanvasHeight() - vertMargin - height);
            glVertex2f(left,     gScreenInfo.getGameCanvasHeight() - vertMargin - height);
         glEnd();
      }

      glDisableBlend;
   }
}


extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;

// Draw the reticle (i.e. the mouse cursor) if we are using keyboard/mouse
void GameUserInterface::renderReticle()
{
   if(gIniSettings.inputMode == Keyboard)
   {
#if 0 // TNL_OS_WIN32
      Point realMousePoint = mMousePoint;
      if(!gIniSettings.controlsRelative)
      {
         F32 len = mMousePoint.len();
         checkMousePos(gScreenInfo.getWindowWidth()  * 100 / canvasWidth,
                       gScreenInfo.getWindowHeight() * 100 / canvasHeight);

         if(len > 100)
            realMousePoint *= 100 / len;
      }
#endif
      Point offsetMouse = mMousePoint + Point(gScreenInfo.getGameCanvasWidth() / 2, gScreenInfo.getGameCanvasHeight() / 2);

      glEnableBlend;
      glColor4f(0,1,0, 0.7);
      glBegin(GL_LINES);

      glVertex2f(offsetMouse.x - 15, offsetMouse.y);
      glVertex2f(offsetMouse.x + 15, offsetMouse.y);
      glVertex2f(offsetMouse.x, offsetMouse.y - 15);
      glVertex2f(offsetMouse.x, offsetMouse.y + 15);

      if(offsetMouse.x > 30)
      {
         glColor4f(0,1,0, 0);
         glVertex2f(0, offsetMouse.y);
         glColor4f(0,1,0, 0.7);
         glVertex2f(offsetMouse.x - 30, offsetMouse.y);
      }
      if(offsetMouse.x < gScreenInfo.getGameCanvasWidth() - 30)
      {
         glColor4f(0,1,0, 0.7);
         glVertex2f(offsetMouse.x + 30, offsetMouse.y);
         glColor4f(0,1,0, 0);
         glVertex2f(gScreenInfo.getGameCanvasWidth(), offsetMouse.y);
      }
      if(offsetMouse.y > 30)
      {
         glColor4f(0,1,0, 0);
         glVertex2f(offsetMouse.x, 0);
         glColor4f(0,1,0, 0.7);
         glVertex2f(offsetMouse.x, offsetMouse.y - 30);
      }
      if(offsetMouse.y < gScreenInfo.getGameCanvasHeight() - 30)
      {
         glColor4f(0,1,0, 0.7);
         glVertex2f(offsetMouse.x, offsetMouse.y + 30);
         glColor4f(0,1,0, 0);
         glVertex2f(offsetMouse.x, gScreenInfo.getGameCanvasHeight());
      }

      glEnd();
      glDisableBlend;
   }

   if(mWrongModeMsgDisplay.getCurrent())
   {
      glColor3f(1,.5,.5);
      drawCenteredString(225, 20, "You are in joystick mode.");
      drawCenteredString(250, 20, "You can change to Keyboard input with the Options menu.");
   }
}

static const S32 fontSize = 15;
static const S32 gapSize = 3;       // Gap between text and box

S32 gLoadoutIndicatorHeight = fontSize + gapSize * 2;


S32 renderIndicator(S32 xPos, const char *name)
{
   S32 width = UserInterface::getStringWidth(fontSize, name);

   glBegin(GL_LINE_LOOP);
      glVertex2f(xPos, UserInterface::vertMargin);
      glVertex2f(xPos + width + 2 * gapSize, UserInterface::vertMargin);
      glVertex2f(xPos + width + 2 * gapSize, UserInterface::vertMargin + fontSize + 2 * gapSize + 1);
      glVertex2f(xPos, UserInterface::vertMargin + fontSize + 2 * gapSize + 1);
   glEnd();

   // Add the weapon name
   UserInterface::drawString(xPos + gapSize, UserInterface::vertMargin + gapSize, fontSize, name);

   return width;
}


// Draw weapon indicators at top of the screen, runs on client
void GameUserInterface::renderLoadoutIndicators()
{
   if(!gIniSettings.showWeaponIndicators)      // If we're not drawing them, we've got nothing to do
      return;

   if(!gClientGame->getConnectionToServer())    // Can happen when first joining a game.  This was XelloBlue's crash...
      return;

   Ship *localShip = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
   if(!localShip)
      return;

   const Color INDICATOR_INACTIVE_COLOR(0,.8,0);    // green
   const Color INDICATOR_ACTIVE_COLOR(.8,0,0);      // red

   U32 xPos = UserInterface::horizMargin;

   // First, the weapons
   for(U32 i = 0; i < (U32)ShipWeaponCount; i++)
   {
      glColor(i == localShip->mActiveWeaponIndx ? INDICATOR_ACTIVE_COLOR : INDICATOR_INACTIVE_COLOR);

      S32 width = renderIndicator(xPos, gWeapons[localShip->getWeapon(i)].name.getString());

      xPos += UserInterface::vertMargin + width - 2 * gapSize;
   }

   xPos += 20;    // Small horizontal gap to seperate the weapon indicators from the module indicators

   // Next, loadout modules
   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
   {
      if(gClientGame->getModuleInfo(localShip->getModule(i))->getUseType() == ModuleUsePassive)
         glColor3f(1,1,0);      // yellow = passive indicator
      else if(localShip->isModuleActive(localShip->getModule(i)))
         glColor(INDICATOR_ACTIVE_COLOR);
      else 
         glColor(INDICATOR_INACTIVE_COLOR);

      S32 width = renderIndicator(xPos, gClientGame->getModuleInfo(localShip->getModule(i))->getName());

      xPos += UserInterface::vertMargin + width - 2 * gapSize;
   }
}


static const S32 FONTSIZE = 14;
static const S32 FONT_GAP = 4;

// Render any incoming chat msgs
void GameUserInterface::renderMessageDisplay()
{
   glColor3f(1,1,1);

   S32 y = gIniSettings.showWeaponIndicators ? UserInterface::chatMargin : UserInterface::vertMargin;
   S32 msgCount;

   if(mMessageDisplayMode == LongFixed)
      msgCount = MessageStoreCount;    // Long form
   else
      msgCount = MessageDisplayCount;  // Short form


   if(mMessageDisplayMode == ShortTimeout)
      for(S32 i = msgCount - 1; i >= 0; i--)
      {
         if(mDisplayMessage[i][0])
         {
            glColor(mDisplayMessageColor[i]);
            drawString(UserInterface::horizMargin, y, FONTSIZE, mDisplayMessage[i]);
            y += FONTSIZE + FONT_GAP;
         }
      }
   else
      for(S32 i = msgCount - 1; i >= 0; i--)
      {
         if(mStoreMessage[i][0])
         {
            glColor(mStoreMessageColor[i]);
            drawString(UserInterface::horizMargin, y, FONTSIZE, mStoreMessage[i]);
            y += FONTSIZE + FONT_GAP;
         }
      }
}


bool GameUserInterface::isCmdChat()
{
   return mLineEditor.at(0) == '/' || mCurrentChatType == CmdChat;
}


void GameUserInterface::onMouseDragged(S32 x, S32 y)
{
   onMouseMoved();
}


void GameUserInterface::onMouseMoved(S32 x, S32 y)
{
   onMouseMoved();
}


void GameUserInterface::onMouseMoved()
{
   mMousePoint.set(gScreenInfo.getMousePos()->x - gScreenInfo.getGameCanvasWidth()  / 2,
                   gScreenInfo.getMousePos()->y - gScreenInfo.getGameCanvasHeight() / 2);

   if(gClientGame->getInCommanderMap())     // Ship not in center of the screen in cmdrs map.  Where is it?
   {
      // If we join a server while in commander's map, we'll be here without a gameConnection and we'll get a crash without this check
      GameConnection *gameConnection = gClientGame->getConnectionToServer();
      if(!gameConnection)
         return;

      // Here's our ship...
      Ship *ship = dynamic_cast<Ship *>(gameConnection->getControlObject());
      if(!ship)      // Can sometimes happen when switching levels. This will stop the ensuing crashing.
         return;

      Point p = gClientGame->worldToScreenPoint( ship->getRenderPos() );

      mCurrentMove.angle = atan2(mMousePoint.y + gScreenInfo.getGameCanvasHeight() / 2 - p.y, 
                                 mMousePoint.x + gScreenInfo.getGameCanvasWidth() / 2 - p.x);
   }

   else     // Ship is at center of the screen
      mCurrentMove.angle = atan2(mMousePoint.y, mMousePoint.x);
}


// Enter QuickChat, Loadout, or Engineer mode
void GameUserInterface::enterMode(GameUserInterface::Mode mode)
{
   UserInterface::playBoop();
   mCurrentMode = mode;

   if(mode == QuickChatMode)
      mHelper = &mQuickChatHelper;
   else if(mode == LoadoutMode)
      mHelper = &mLoadoutHelper;
   else if(mode == EngineerMode)
      mHelper = &mEngineerHelper;
   else 
   {
      if(mode == PlayMode)
      {
         setBusyChatting(false);
         mUpDisabled = false;
         mDownDisabled = false;
         mLeftDisabled = false;
         mRightDisabled = false;
      }

      mHelper = NULL;
   }

   if(mHelper)
      mHelper->onMenuShow();
}


void GameUserInterface::renderEngineeredItemDeploymentMarker(Ship *ship)
{
   if(mCurrentMode == EngineerMode)
      mEngineerHelper.renderDeploymentMarker(ship);
}


// Runs on client
void GameUserInterface::dropItem()
{
   if(!gClientGame->getConnectionToServer())
      return;

   Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
   if(!ship)
      return;

   GameType *gt = gClientGame->getGameType();
   if(!gt)
      return;

   if(!gt->isCarryingItems(ship))
   {
      displayMessage(Color(1.0, 0.5, 0.5), "You don't have any items to drop!");
      return;
   }

   gt->c2sDropItem();
}


// Send a message to the server that we are (or are not) busy chatting
void GameUserInterface::setBusyChatting(bool busy)
{
   if( gClientGame && gClientGame->getConnectionToServer() )
      gClientGame->getConnectionToServer()->c2sSetIsBusy(busy);
}


// Select next weapon
void GameUserInterface::advanceWeapon()
{
   GameType *g = gClientGame->getGameType();
   if(g)
      g->c2sAdvanceWeapon();
}

// Select a weapon by its index
void GameUserInterface::selectWeapon(U32 indx)
{
   GameType *g = gClientGame->getGameType();
   if(g)
      g->c2sSelectWeapon(indx);
}

// Temporarily disable the effects of a movement key to avoid unpleasant interactions between ship movement and loadout/quick chat entry
void GameUserInterface::disableMovementKey(KeyCode keyCode)
{
   InputMode inputMode = gIniSettings.inputMode;

   if(keyCode == keyUP[inputMode])
      mUpDisabled = true;
   else if(keyCode == keyDOWN[inputMode])
      mDownDisabled = true;
   else if(keyCode == keyLEFT[inputMode])
      mLeftDisabled = true;
   else if(keyCode == keyRIGHT[inputMode])
      mRightDisabled = true;
}


// Key pressed --> take action!
// Handles all keypress events, including mouse clicks and controller button presses
void GameUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(OGLCONSOLE_ProcessBitfighterKeyEvent(keyCode, ascii))   // Pass the key on to the console for processing
   {
      // Do nothing... key processed
   }
   else if(keyCode == keyHELP)          // Turn on help screen
   {
      UserInterface::playBoop();

      if(mCurrentMode == ChatMode)
         gInstructionsUserInterface.activateInCommandMode();
      else
         gInstructionsUserInterface.activate();


   }
   // Shift-/ toggles console window for the moment  (Ctrl-/ fails in glut!)
   // Don't want to open console while chatting, do we?  Only open when not in any special mode.
   else if(mCurrentMode == PlayMode && keyCode == KEY_SLASH && getKeyState(KEY_SHIFT))   
   {
      OGLCONSOLE_ShowConsole();
   }
   else if(keyCode == keyOUTGAMECHAT)
   {
      setBusyChatting(true);
      gChatInterface.activate();
   }
   else if(keyCode == keyMISSION)
   {
      mMissionOverlayActive = true;

      GameType *gt = gClientGame->getGameType();
      if(gt)
         gt->mLevelInfoDisplayTimer.clear();    // Clear level-start display if user hits F2
   }
   else if(keyCode == KEY_M && getKeyState(KEY_CTRL))    // Ctrl-M, for now, to cycle through message dispaly modes
   {
      S32 m = mMessageDisplayMode + 1;
      if(m >= MessageDisplayModes)
         m = 0;
      mMessageDisplayMode = MessageDisplayMode(m);
   }
   else if(mHelper && mHelper->processKeyCode(keyCode))   // Will return true if key was processed
   {
      disableMovementKey(keyCode);
   }
   else 
   {
      // If we're in play mode, and we apply the engineer module, then we can handle that locally by throwing up a menu or message
      if(mCurrentMode == PlayMode)
      {
         Ship *ship = NULL;
         if(gClientGame->getConnectionToServer())   // Prevents errors, getConnectionToServer() might be NULL, and getControlObject may crash if NULL
            ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
         if(ship)
         {
            if(keyCode == keyMOD1[gIniSettings.inputMode] && ship->getModule(0) == ModuleEngineer || 
               keyCode == keyMOD2[gIniSettings.inputMode] && ship->getModule(1) == ModuleEngineer)
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

      if(mCurrentMode == ChatMode)
         processChatModeKey(keyCode, ascii);
      else   
         processPlayModeKey(keyCode, ascii);    // A non-chat key, really
   }
}


void GameUserInterface::processPlayModeKey(KeyCode keyCode, char ascii)
{
   InputMode inputMode = gIniSettings.inputMode;
   // The following keys are allowed in both play mode and in
   // loadout or engineering menu modes if not used in the loadout
   // menu above

   if(keyCode == KEY_CLOSEBRACKET && getKeyState(KEY_ALT))     // Alt-] advances bots by one step if frozen
   {
      if(Robot::isPaused())
         Robot::addSteps(1);
   }
   else if(keyCode == KEY_CLOSEBRACKET && getKeyState(KEY_CTRL))     // Ctrl-] advances bots by 10 steps if frozen
   {
      if(Robot::isPaused())
         Robot::addSteps(10);
   }
   else if(keyCode == keyMOD1[inputMode])
      mModActivated[0] = true;
   else if(keyCode == keyMOD2[inputMode])
      mModActivated[1] = true;
   else if(keyCode == keyFIRE[inputMode])
      mFiring = true;
   else if(keyCode == keySELWEAP1[inputMode])
      selectWeapon(0);
   else if(keyCode == keySELWEAP2[inputMode])
      selectWeapon(1);
   else if(keyCode == keySELWEAP3[inputMode])
      selectWeapon(2);
   else if(keyCode == keyFPS)
      mFPSVisible = !mFPSVisible;
   else if(keyCode == keyADVWEAP[inputMode])
      advanceWeapon();
   else if(keyCode == KEY_ESCAPE || keyCode == BUTTON_BACK)
   {
      if(mShutdownMode == ShuttingDown)
      {
         if(mShutdownInitiator)
         {
            gClientGame->getConnectionToServer()->c2sRequestCancelShutdown();
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

      UserInterface::playBoop();

      if(!gClientGame->isConnectedToServer())      // Perhaps we're still joining?
      {
         endGame();
         gMainMenuUserInterface.activate();
      }
      else
      {
         setBusyChatting(true);
         gGameMenuUserInterface.activate();
      }
   }     
   else if(keyCode == keyCMDRMAP[inputMode])
      gClientGame->zoomCommanderMap();

   else if(keyCode == keySCRBRD[inputMode])
   {     // (braces needed)
      if(!mInScoreboardMode)    // We're activating the scoreboard
      {
         mInScoreboardMode = true;
         GameType *gameType = gClientGame->getGameType();
         if(gameType)
            gameType->c2sRequestScoreboardUpdates(true);
      }
   }
   else if(keyCode == keyTOGVOICE[inputMode])
   {     // (braces needed)
      if(!mVoiceRecorder.mRecordingAudio)  // Turning recorder on
         mVoiceRecorder.start();
   }
   else if(mCurrentMode != LoadoutMode && mCurrentMode != QuickChatMode && mCurrentMode != EngineerMode)
   {
      // The following keys are only allowed in PlayMode, and never work in LoadoutMode
      if(keyCode == keyTEAMCHAT[inputMode])
      {
         mCurrentChatType = TeamChat;
         mCurrentMode = ChatMode;
         setBusyChatting(true);
      }
      else if(keyCode == keyGLOBCHAT[inputMode])
      {
         mCurrentChatType = GlobalChat;
         mCurrentMode = ChatMode;
         setBusyChatting(true);
      }
      else if(keyCode == keyCMDCHAT[inputMode])
      {
         mCurrentChatType = CmdChat;
         mCurrentMode = ChatMode;
         setBusyChatting(true);
      }
      else if(keyCode == keyQUICKCHAT[inputMode])
         enterMode(QuickChatMode);
      else if(keyCode == keyLOADOUT[inputMode])
         enterMode(LoadoutMode);
      else if(keyCode == keyDROPITEM[inputMode])
         dropItem();

      else if(inputMode == Joystick)      // Check if the user is trying to use keyboard to move when in joystick mode
         if(keyCode == keyUP[Keyboard] || keyCode == keyDOWN[Keyboard] || keyCode == keyLEFT[Keyboard] || keyCode == keyRIGHT[Keyboard])
            mWrongModeMsgDisplay.reset(WRONG_MODE_MSG_DISPLAY_TIME);
   }
}


static bool hasAdmin(GameConnection *gc, const char *failureMessage)
{
   if(!gc->isAdmin())
   {
      gClientGame->mGameUserInterface->displayErrorMessage(failureMessage);
      return false;
   }
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


static string makeFilenameFromString(const char *levelname)
{
   static char filename[MAX_FILE_NAME_LEN + 1];    // Leave room for terminating null

   U32 i = 0;

   while(i < MAX_FILE_NAME_LEN && levelname[i] != 0)
   {
      // Prevent invalid characters in file names
      char c = levelname[i];
      if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
         filename[i]=c;
      else
         filename[i]='_';
      i++;
   }

   filename[i] = 0;    // Null terminate
   return filename;
}


// TODO: Probably misnamed... handles deletes too
static void changeServerNameDescr(GameConnection *gc, GameConnection::ParamType type, const Vector<string> &words)
{
   // Concatenate all params into a single string
   string allWords = concatenate(words, 1);

   // Did the user provide a name/description?
   if(type != GameConnection::DeleteLevel && allWords == "")
   {
      gClientGame->mGameUserInterface->displayErrorMessage(type == GameConnection::ServerName ? "!!! Need to supply a name" : "!!! Need to supply a description");
      return;
   }

   gc->changeParam(allWords.c_str(), type);
}

extern CIniFile gINI;
extern md5wrapper md5;

static void changePassword(GameConnection *gc, GameConnection::ParamType type, const Vector<string> &words, bool required)
{
   if(required)
   {
      if(words.size() < 2 || words[1] == "")
      {
         gClientGame->mGameUserInterface->displayErrorMessage("!!! Need to supply a password");
         return;
      }

      gc->changeParam(words[1].c_str(), type);
   }
   else if(words.size() < 2)
   {
      gc->changeParam("", type);
   }

   if(words.size() < 2)    // Empty password
   {
      // Clear any saved password for this server
      if(type == GameConnection::LevelChangePassword)
         gINI.deleteKey("SavedLevelChangePasswords", gc->getServerName());
      else if(type == GameConnection::AdminPassword)
         gINI.deleteKey("SavedAdminPasswords", gc->getServerName());
   }
   else                    // Non-empty password
   {
      gc->changeParam(words[1].c_str(), type);

      // Save the password so the user need not enter it again the next time they're on this server
      if(type == GameConnection::LevelChangePassword)
         gINI.SetValue("SavedLevelChangePasswords", gc->getServerName(), words[1], true);
      else if(type == GameConnection::AdminPassword)
         gINI.SetValue("SavedAdminPasswords", gc->getServerName(), words[1], true);
   }
}


void GameUserInterface::addTimeHandler(GameUserInterface *gui, const Vector<string> &words)
{
   if(words.size() < 2 || words[1] == "")
      gui->displayErrorMessage("!!! Need to supply a time (in minutes)");
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
         gui->displayErrorMessage("!!! Invalid value... game time not changed");
      else
      {
         gui->displayMessage(gCmdChatColor, "Extended game by %d minute%s", mins, (mins == 1) ? "" : "s");

         if(gClientGame->getGameType())
            gClientGame->getGameType()->addTime(mins * 60 * 1000);
      }
   }
}


bool GameUserInterface::checkName(const string &name)
{
   S32 potentials = 0;
   string potential;

   for(S32 i = 0; i < gClientGame->getGameType()->mClientList.size(); i++)
   {
      if(!gClientGame->getGameType()->mClientList[i].isValid())
         continue;

      const char *n = gClientGame->getGameType()->mClientList[i]->name.getString();
      if(!strcmp(n, name.c_str()))           // Exact match
         return true;
      else if(!stricmp(n, name.c_str()))     // Case insensitive match
      {
         potentials++;
         potential = n;
      }
   }

   if(potentials == 1)
   {
      //*name = potential;
      return true;
   }

   return false;
}


void GameUserInterface::sVolHandler(GameUserInterface *gui, const Vector<string> &words)
{
   gui->setVolume(SfxVolumeType, words);
}

void GameUserInterface::mVolHandler(GameUserInterface *gui, const Vector<string> &words)
{
   gui->setVolume(MusicVolumeType, words);
}

void GameUserInterface::vVolHandler(GameUserInterface *gui, const Vector<string> &words)
{
   gui->setVolume(VoiceVolumeType, words);
}

void GameUserInterface::servVolHandler(GameUserInterface *gui, const Vector<string> &words)
{
   gui->setVolume(ServerAlertVolumeType, words);
}

void GameUserInterface::getMapHandler(GameUserInterface *gui, const Vector<string> &words)
{
   if(gClientGame->getConnectionToServer()->isLocalConnection())
      gui->displayErrorMessage("!!! Can't download levels from a local server");
   else
   {
      if(words.size() > 1 && words[1] != "")
         gui->remoteLevelDownloadFilename = words[1];
		else
         gui->remoteLevelDownloadFilename = "downloaded_" + makeFilenameFromString(gClientGame->getGameType() ?
               gClientGame->getGameType()->mLevelName.getString() : "Level");
      // Add an extension if needed
      if(gui->remoteLevelDownloadFilename.find(".") == string::npos)
         gui->remoteLevelDownloadFilename += ".level";

      // Make into a fully qualified file name
      string fullFile = strictjoindir(gConfigDirs.levelDir, gui->remoteLevelDownloadFilename);

      // Prepare for writing
      gui->mOutputFile = fopen(fullFile.c_str(), "w");    // TODO: Writes empty file when server does not allow getmap.  Shouldn't.

      if(!gui->mOutputFile)
      {
         logprintf("Problem opening file %s for writing", fullFile.c_str());
         gui->displayErrorMessage("!!! Problem opening file %s for writing", fullFile.c_str());
      }
      else
         gClientGame->getConnectionToServer()->c2sRequestCurrentLevel();
   }
}


void GameUserInterface::nextLevelHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(!gc->isLevelChanger())
      gui->displayErrorMessage("!!! You don't have permission to change levels");
   else
      gc->c2sRequestLevelChange(ServerGame::NEXT_LEVEL, false);
}


void GameUserInterface::prevLevelHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(!gc->isLevelChanger())
      gui->displayErrorMessage("!!! You don't have permission to change levels");
   else
      gc->c2sRequestLevelChange(ServerGame::PREVIOUS_LEVEL, false);
}

void GameUserInterface::restartLevelHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(!gc->isLevelChanger())
      gui->displayErrorMessage("!!! You don't have permission to change levels");
   else
      gc->c2sRequestLevelChange(ServerGame::REPLAY_LEVEL, false);
}


void GameUserInterface::shutdownServerHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(hasAdmin(gc, "!!! You don't have permission to shut the server down"))
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

      gc->c2sRequestShutdown(time, reason.c_str());
   }
}

void GameUserInterface::kickPlayerHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(hasAdmin(gc, "!!! You don't have permission to kick players"))
   {
      if(words.size() < 2 || words[1] == "")
         gui->displayErrorMessage("!!! Need to specify who to kick");
      else
      {
         // Did user provide a valid, known name?
         string name = words[1];
         
         if(!gui->checkName(name))
            gui->displayErrorMessage("!!! Could not find player: %s", words[1].c_str());
         else
            gc->c2sAdminPlayerAction(words[1].c_str(), PlayerMenuUserInterface::Kick, 0);     // Team doesn't matter with kick!
      }
   }
}

void GameUserInterface::adminPassHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(gc->isAdmin())
      gui->displayErrorMessage("!!! You are already an admin");
   else if(words.size() < 2 || words[1] == "")
      gui->displayErrorMessage("!!! Need to supply a password");
   else
      gc->submitAdminPassword(words[1].c_str());
}

void GameUserInterface::levelPassHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(gc->isLevelChanger())
      gui->displayErrorMessage("!!! You can already change levels");
   else if(words.size() < 2 || words[1] == "")
      gui->displayErrorMessage("!!! Need to supply a password");
   else
      gc->submitLevelChangePassword(words[1].c_str());
}

void GameUserInterface::showCoordsHandler(GameUserInterface *gui, const Vector<string> &words)
{
   gui->mDebugShowShipCoords = !gui->mDebugShowShipCoords;
}

void GameUserInterface::showZonesHandler(GameUserInterface *gui, const Vector<string> &words)
{
   if(!(gServerGame && gServerGame->isTestServer())) 
      gui->displayErrorMessage("!!! Zones can only be displayed on a local host");
   else
      gui->mDebugShowMeshZones = !gui->mDebugShowMeshZones;
}


extern bool showDebugBots;  // in game.cpp

void GameUserInterface::showPathsHandler(GameUserInterface *gui, const Vector<string> &words)
{
   if(!(gServerGame && gServerGame->isTestServer())) 
      gui->displayErrorMessage("!!! Robots can only be shown on a test server");
   else
      showDebugBots = !showDebugBots;
}


void GameUserInterface::pauseBotsHandler(GameUserInterface *gui, const Vector<string> &words)
{
   if(!(gServerGame && gServerGame->isTestServer())) 
      gui->displayErrorMessage("!!! Robots can only be frozen on a test server");
   else
      Robot::togglePauseStatus();
}

void GameUserInterface::stepBotsHandler(GameUserInterface *gui, const Vector<string> &words)
{
   if(!(gServerGame && gServerGame->isTestServer())) 
      gui->displayErrorMessage("!!! Robots can only be stepped on a test server");
   else
   {
      S32 steps = words.size() > 1 ? atoi(words[1].c_str()) : 1;
      Robot::addSteps(steps);
   }
}


void GameUserInterface::setAdminPassHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(hasAdmin(gc, "!!! You don't have permission to set the admin password"))
      changePassword(gc, GameConnection::AdminPassword, words, true);
}

void GameUserInterface::setServerPassHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(hasAdmin(gc, "!!! You don't have permission to set the server password"))
      changePassword(gc, GameConnection::ServerPassword, words, false);
}

void GameUserInterface::setLevPassHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(hasAdmin(gc, "!!! You don't have permission to set the level change password"))
      changePassword(gc, GameConnection::LevelChangePassword, words, false);
}

void GameUserInterface::setServerNameHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(hasAdmin(gc, "!!! You don't have permission to set the server name"))
      changeServerNameDescr(gc, GameConnection::ServerName, words);
}

void GameUserInterface::setServerDescrHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(hasAdmin(gc, "!!! You don't have permission to set the server description"))
      changeServerNameDescr(gc, GameConnection::ServerDescr, words);
}


void GameUserInterface::deleteCurrentLevelHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(hasAdmin(gc, "!!! You don't have permission to delete the current level"))
      changeServerNameDescr(gc, GameConnection::DeleteLevel, words);    // handles deletes too
}


void GameUserInterface::suspendHandler(GameUserInterface *gui, const Vector<string> &words)
{
   U32 players = gClientGame->getPlayerCount();
   if(players == (U32)Game::PLAYER_COUNT_UNAVAILABLE || players > 1)
      gui->displayErrorMessage("!!! Can't suspend when others are playing");
   else
      gui->suspendGame();
}


void GameUserInterface::lineWidthHandler(GameUserInterface *gui, const Vector<string> &words)
{
   F32 linewidth;
   if(words.size() < 2 || words[1] == "")
      gui->displayErrorMessage("!!! Need to supply line width");
   else
   {
      linewidth = atof(words[1].c_str());
      if(linewidth < 0.125f)
         linewidth = 0.125f;

      gDefaultLineWidth = linewidth;
      gLineWidth1 = linewidth * 0.5f;
      gLineWidth3 = linewidth * 1.5f;
      gLineWidth4 = linewidth * 2;

      glLineWidth(gDefaultLineWidth);    //make this change happen instantly
   }
}


void GameUserInterface::lineSmoothHandler(GameUserInterface *gui, const Vector<string> &words)
{
   gIniSettings.useLineSmoothing = !gIniSettings.useLineSmoothing;
   if(gIniSettings.useLineSmoothing)
   {
      glEnable(GL_LINE_SMOOTH);
      glEnable(GL_BLEND);
   }else
   {
      glDisable(GL_LINE_SMOOTH);
      glDisable(GL_BLEND);
   }
}


void GameUserInterface::maxFpsHandler(GameUserInterface *gui, const Vector<string> &words)
{
   S32 number = words.size() > 1 ? atoi(words[1].c_str()) : 0;
   if(number < 1)                              // Don't allow zero or negative numbers
      gui->displayErrorMessage("!!! Usage: /maxfps <frame rate>, default = 100");
   else
      gIniSettings.maxFPS = number;
}


void GameUserInterface::pmHandler(GameUserInterface *gui, const Vector<string> &words)
{
   if(words.size() < 3)
      gui->displayErrorMessage("!!! Usage: /pm <player name> <message>");
   else
   {
      if(!gui->checkName(words[1]))
         gui->displayErrorMessage("!!! Unknown name: %s", words[1].c_str());
      else
      {
         S32 argCount = 2 + countCharInString(words[1], ' ');  // Set pointer after 2 args + number of spaces in player name
         const char *message = gui->mLineEditor.c_str();  // Get the original line
         message = findPointerOfArg(message, argCount);     // Get the rest of the message

         GameType *gt = gClientGame->getGameType();
         if(gt)
            gt->c2sSendChatPM(words[1], message);
      }
   }
}


void GameUserInterface::muteHandler(GameUserInterface *gui, const Vector<string> &words)
{
   if(words.size() < 2)
      gui->displayErrorMessage("!!! Usage: /mute <player name>");
   else
   {
      if(!gui->checkName(words[1]))
         gui->displayErrorMessage("!!! Unknown name: %s", words[1].c_str());
      else
      {
         gui->mMuteList.push_back(words[1]);
         gui->displaySuccessMessage("Player %s has been muted", words[1].c_str());
      }
   }
}


void GameUserInterface::serverCommandHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameType *gt = gClientGame->getGameType();
   if(gt)
   {
      Vector<StringPtr> args;

      for(S32 i = 1; i < words.size(); i++)
         args.push_back(StringPtr(words[i]));

      gt->c2sSendCommand(StringTableEntry(words[0], false), args);
   }
}


CommandInfo chatCmds[] = {   
   //  cmdName          cmdCallback               cmdArgInfo cmdArgCount   helpCategory helpGroup   helpArgString            helpTextSstring
   { "admin",   GameUserInterface::adminPassHandler, { STR },      1,       ADV_COMMANDS,    0,      {"<password>"},         "Request admin permissions"  },
   { "levpass", GameUserInterface::levelPassHandler, { STR },      1,       ADV_COMMANDS,    0,      {"<password>"},         "Request level change permissions"  },
   { "servvol", GameUserInterface::servVolHandler,   { INT },      1,       ADV_COMMANDS,    0,      {"<0-10>"},             "Set volume of server"  },
   { "getmap",  GameUserInterface::getMapHandler,    { STR },      1,       ADV_COMMANDS,    1,      {"[file]"},             "Save currently playing level in [file], if allowed" },
   { "suspend", GameUserInterface::suspendHandler,   {  },         0,       ADV_COMMANDS,    1,      {  },                   "Place game on hold while waiting for players" },
   { "pm",      GameUserInterface::pmHandler,        { NAME, STR },2,       ADV_COMMANDS,    1,      {"<name>","<message>"}, "Send private message to player" },
   { "mvol",    GameUserInterface::mVolHandler,      { INT },      1,       ADV_COMMANDS,    2,      {"<0-10>"},             "Set music volume"      },
   { "svol",    GameUserInterface::sVolHandler,      { INT },      1,       ADV_COMMANDS,    2,      {"<0-10>"},             "Set SFX volume"        },
   { "vvol",    GameUserInterface::vVolHandler,      { INT },      1,       ADV_COMMANDS,    2,      {"<0-10>"},             "Set voice chat volume" },
   { "mute",    GameUserInterface::muteHandler,      { NAME },     1,       ADV_COMMANDS,    3,      {"<name>"},             "Hide chat messages from <name> until you quit" },

   { "add",      GameUserInterface::addTimeHandler,       { INT },           0,       LEVEL_COMMANDS,  0,      {"<time in minutes>"},        "Add time to the current game" },
   { "next",     GameUserInterface::nextLevelHandler,     {  },              0,       LEVEL_COMMANDS,  0,      {  },                         "Start next level" },
   { "prev",     GameUserInterface::prevLevelHandler,     {  },              0,       LEVEL_COMMANDS,  0,      {  },                         "Replay previous level" },
   { "restart",  GameUserInterface::restartLevelHandler,  {  },              0,       LEVEL_COMMANDS,  0,      {  },                         "Restart current level" },
   { "settime",  GameUserInterface::serverCommandHandler, { INT },           1,       LEVEL_COMMANDS,  0,      {"<time in minutes>"},        "Set play time for the level" },
   { "setscore", GameUserInterface::serverCommandHandler, { INT },           1,       LEVEL_COMMANDS,  0,      {"<score>"},                  "Set score to win the level" },
   { "addbot",   GameUserInterface::serverCommandHandler, { TEAM, STR, STR },3,       LEVEL_COMMANDS,  1,      {"[team]","[file]","[args]"}, "Add a bot from [file] to [team], pass [args] to bot" },
   { "kickbot",  GameUserInterface::serverCommandHandler, {  },              1,       LEVEL_COMMANDS,  1,      {  },                         "Kick most recently added bot" },
   { "kickbots", GameUserInterface::serverCommandHandler, {  },              1,       LEVEL_COMMANDS,  1,      {  },                         "Kick all bots" },

   { "kick",               GameUserInterface::kickPlayerHandler,         { NAME },    1,  ADMIN_COMMANDS, 0, {"<player name>"},      "Kick a player from the game" },
   { "shutdown",           GameUserInterface::shutdownServerHandler,     {INT, STR }, 2,  ADMIN_COMMANDS, 0, {"[time]","[message]"}, "Start orderly shutdown of server (def. = 10 secs)" },
   { "setlevpass",         GameUserInterface::setLevPassHandler,         { STR },     1,  ADMIN_COMMANDS, 0, {"[passwd]"},           "Set server password  (use blank to clear)" },
   { "setadminpass",       GameUserInterface::setAdminPassHandler,       { STR },     1,  ADMIN_COMMANDS, 0, {"[passwd]"},           "Set level change password (use blank to clear)" },
   { "setserverpass",      GameUserInterface::setServerPassHandler,      { STR },     1,  ADMIN_COMMANDS, 0, {"<passwd>"},           "Set admin password" },
   { "setservername",      GameUserInterface::setServerNameHandler,      { STR },     1,  ADMIN_COMMANDS, 0, {"<name>"},             "Set server name" },
   { "setserverdescr",     GameUserInterface::setServerDescrHandler,     { STR },     1,  ADMIN_COMMANDS, 0, {"<descr>"},            "Set server description" },
   { "deletecurrentlevel", GameUserInterface::deleteCurrentLevelHandler, { },         0,  ADMIN_COMMANDS, 0, {""},                   "Remove current level from server" },

   { "showcoords", GameUserInterface::showCoordsHandler,    {  },    0, DEBUG_COMMANDS, 0, {  },         "Show ship coordinates" },
   { "showzones",  GameUserInterface::showZonesHandler,     {  },    0, DEBUG_COMMANDS, 0, {  },         "Show bot nav mesh zones" },
   { "showpaths",  GameUserInterface::showPathsHandler,     {  },    0, DEBUG_COMMANDS, 0, {  },         "Show robot paths" },
   { "showbots",   GameUserInterface::serverCommandHandler, {  },    0, DEBUG_COMMANDS, 0, {  },         "Show all robots" },
   { "pausebots",  GameUserInterface::pauseBotsHandler,     {  },    0, DEBUG_COMMANDS, 0, {  },         "Pause all bots.  Reissue to start again" },
   { "stepbots",   GameUserInterface::stepBotsHandler,      { INT }, 1, DEBUG_COMMANDS, 1, {"[steps]"},  "Advance bots by number of steps (def. = 1)"},
   { "linewidth",  GameUserInterface::lineWidthHandler,     { INT }, 1, DEBUG_COMMANDS, 1, {"[number]"}, "Change width of all lines (def. = 2)" },
   { "linesmooth", GameUserInterface::lineSmoothHandler,    {  },    0, DEBUG_COMMANDS, 1, {  },         "Enable line smoothing, might look better" },
   { "maxfps",     GameUserInterface::maxFpsHandler,        { INT }, 1, DEBUG_COMMANDS, 1, {"<number>"}, "Set maximum speed of game in frames per second" },
};

S32 chatCmdSize = ARRAYSIZE(chatCmds);    // So instructions will now how big chatCmds is

// Render chat msg that user is composing
void GameUserInterface::renderCurrentChat()
{
   if(mCurrentMode != ChatMode)
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
   if(! (gClientGame && gClientGame->getConnectionToServer()))
      return;

   S32 promptSize = getStringWidth(FONTSIZE, promptStr);
   S32 nameSize = getStringWidthf(FONTSIZE, "%s: ", gClientGame->getConnectionToServer()->getClientName().getString());
   S32 nameWidth = max(nameSize, promptSize);
   // Above block repeated below...

   S32 xpos = UserInterface::horizMargin;

   const S32 ypos = UserInterface::vertMargin +
                    (gIniSettings.showWeaponIndicators ? UserInterface::chatMargin : UserInterface::vertMargin) +
                    (mMessageDisplayMode == LongFixed ? MessageStoreCount : MessageDisplayCount) * (FONTSIZE + FONT_GAP);

   S32 width = gScreenInfo.getGameCanvasWidth() - 2 * horizMargin - (nameWidth - promptSize) + 6;

   // Render text entry box like thingy
   glEnableBlend;

   for(S32 i = 1; i >= 0; i--)
   {
      glColor(baseColor, i ? .25 : .4);

      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2f(xpos, ypos - 3);
         glVertex2f(xpos + width, ypos - 3);
         glVertex2f(xpos + width, ypos + FONTSIZE + 7);
         glVertex2f(xpos, ypos + FONTSIZE + 7);
      glEnd();
   }
   glDisableBlend;

   glColor(baseColor);

   xpos += 3;     // Left margin
   xpos += drawStringAndGetWidth(xpos, ypos, FONTSIZE, promptStr);

   S32 x = drawStringAndGetWidth(xpos, ypos, FONTSIZE, mLineEditor.c_str());

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
                  drawString(xpos + x, ypos, FONTSIZE, chatCmds[i].helpArgString[words.size() - 1].c_str());
               }

               break;
            }
         }
      }
   }

   glColor(baseColor);
   mLineEditor.drawCursor(xpos, ypos, FONTSIZE);
}


static Vector<string> commandCandidateList;
static Vector<string> nameCandidateList;         // Reuse this for player and team names

static void makeCommandCandidateList()
{
   for(S32 i = 0; i < chatCmdSize; i++)
      commandCandidateList.push_back(chatCmds[i].cmdName);
}

static void makePlayerNameCandidateList()
{
   nameCandidateList.clear();

   if(gClientGame->getGameType())
      for(S32 i = 0; i < gClientGame->getGameType()->mClientList.size(); i++)
         if(gClientGame->getGameType()->mClientList[i].isValid())
            nameCandidateList.push_back(gClientGame->getGameType()->mClientList[i]->name.getString());
}


static void makeTeamNameCandidateList()
{
   nameCandidateList.clear();

   if(gClientGame->getGameType())
      for(S32 i = 0; i < gClientGame->getTeamCount(); i++)
         nameCandidateList.push_back(gClientGame->getGameType()->getTeamName(i).getString());
}


static Vector<string> *getCandidateList(const string &cmdName, S32 arg)
{
   if(arg == 0)         // Command completion
      return &commandCandidateList;

   else if(arg > 0)     // Arg completion
   {
      // Figure out which command we're entering, so we know what kind of args to expect
      S32 cmd = -1;

      for(S32 i = 0; i < chatCmdSize; i++)
         if(!stricmp(chatCmds[i].cmdName.c_str(), cmdName.c_str()))
         {
            cmd = i;
            break;
         }

      if(cmd != -1 && arg <= chatCmds[cmd].cmdArgCount)     // Found a command
      {
         ArgTypes argType = chatCmds[cmd].cmdArgInfo[arg - 1];  // What type of arg are we expecting?

         if(argType == NAME)           // Player names
         {  
            makePlayerNameCandidateList();
            return &nameCandidateList;
         }

         else if(argType == TEAM)      // Team names
         {
            makeTeamNameCandidateList();
            return &nameCandidateList;
         }
      }
   }
   
   return NULL;                        // No completion options
}


void GameUserInterface::processChatModeKey(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_ENTER)
      issueChat();
   else if(keyCode == KEY_BACKSPACE)
      mLineEditor.backspacePressed();
   else if(keyCode == KEY_DELETE)
      mLineEditor.deletePressed();
   else if(keyCode == KEY_ESCAPE || keyCode == BUTTON_BACK)
      cancelChat();
   else if(keyCode == KEY_TAB)      // Auto complete any commands
   {
      if(isCmdChat())     // It's a command!  Complete!  Complete!
      {
         // First, parse line into words
         Vector<string> words = parseString(mLineEditor.c_str());

         bool needLeadingSlash = false;

         // Handle leading slash when command is entered from ordinary chat prompt
         if(words.size() > 0 && words[0][0] == '/')
         {
            // Special case: User has entered process by starting with global chat, and has typed "/" then <tab>
            if(mLineEditor.getString() == "/")
               words.clear();          // Clear -- it's as if we're at a fresh "/" prompt where the user has typed nothing
            else
               words[0].erase(0, 1);   // Strip -- remove leading "/" so it's as if were at a regular "/" prompt

            needLeadingSlash = true;   // We'll need to add the stripped "/" back in later
         }
               
         S32 arg;                      // Which word we're looking at
         const char *partial;          // The partially typed word we're trying to match against
         
         // Check for trailing space --> http://www.suodenjoki.dk/us/archive/2010/basic-string-back.htm
         if(words.size() > 0 && *mLineEditor.getString().rbegin() != ' ')   
         {
            arg = words.size() - 1;          // No trailing space --> current arg is the last word we've been typing
            partial = words[arg].c_str();    // We'll be matching against what we've typed so far
         }
         else     // If the editor is empty, or if final character is a space, then we need to set these params differently
         {
            arg = words.size();              // Trailing space --> current arg is the next word we've not yet started typing
            partial = "";                    // We'll be matching against an empty list since we've typed nothing so far
         }

         Vector<string> *candidates = getCandidateList(words[0], arg);     // Could return NULL

         // Now we have our candidates list... let's compare to what the player has already typed to generate completion string
         if(candidates && candidates->size() > 0)
         {
            mLineEditor.buildMatchList(candidates, partial);    // Filter candidates by what we've typed so far

            if(mLineEditor.matchList.size() == 0)               // Found no matches... no expansion possible
               return;

            const string *str = mLineEditor.getStringPtr();     // Convenient shortcut

            // if the command string has quotes in it use the last space up to the first quote
            size_t lastChar = string::npos;
            if (str->find_first_of("\"") != string::npos)
               lastChar = str->find_first_of("\"");

            size_t pos = str->find_last_of(' ', lastChar);
            string space = " ";

            if(pos == string::npos)    // i.e. string does not contain a space, requires special handling
            {
               pos = 0;
               if(words.size() <= 1 && needLeadingSlash)    // ugh!  More special cases!
                  space = "/";
               else
                  space = "";
            }

            mLineEditor.matchIndex++;     // Advance to next potential match

            if(mLineEditor.matchIndex >= mLineEditor.matchList.size())     // Handle wrap-around
               mLineEditor.matchIndex = 0;

            // if match contains a space, wrap in quotes
            string matchedString = mLineEditor.matchList[mLineEditor.matchIndex];
            if (matchedString.find_first_of(" ") != string::npos)
               matchedString = "\"" + matchedString +"\"";

            mLineEditor.setString(str->substr(0, pos).append(space + matchedString));    // Add match to the command
         }
      }
   }
   else if(ascii)     // Append any other keys to the chat message
   {
      // Protect against crashes while game is initializing (because we look at the ship for the player's name)
      if(gClientGame->getConnectionToServer())     // gClientGame cannot be NULL here
      {
         S32 promptSize = getStringWidth(FONTSIZE, mCurrentChatType == TeamChat ? "(Team): " : "(Global): ");

         Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
         if(!ship)
            return;

         S32 nameSize = getStringWidthf(FONTSIZE, "%s: ", ship->getName().getString());
         S32 nameWidth = max(nameSize, promptSize);
         // Above block repeated above

         if(nameWidth + (S32) getStringWidthf(FONTSIZE, "%s%c", mLineEditor.c_str(), ascii) < 
                                             gScreenInfo.getGameCanvasWidth() - 2 * horizMargin - 3)
            mLineEditor.addChar(ascii);
      }
   }
}

void GameUserInterface::onKeyUp(KeyCode keyCode)
{
   S32 inputMode = gIniSettings.inputMode;

   // These keys works in any mode!  And why not??

   if(keyCode == keyMISSION)
      mMissionOverlayActive = false;
   else if (keyCode == keyMOD1[inputMode])
      mModActivated[0] = false;
   else if (keyCode == keyMOD2[inputMode])
      mModActivated[1] = false;
   else if (keyCode == keyFIRE[inputMode])
      mFiring = false;
   else if(keyCode == keySCRBRD[inputMode])
   {     // (braces required)
      if(mInScoreboardMode)     // We're turning scoreboard off
      {
         mInScoreboardMode = false;
         GameType *g = gClientGame->getGameType();
         if(g)
            g->c2sRequestScoreboardUpdates(false);
      }
   }
   else if(keyCode == keyTOGVOICE[inputMode])
   {     // (braces required)
      if(mVoiceRecorder.mRecordingAudio)  // Turning recorder off
         mVoiceRecorder.stop();
   }
   else if(keyCode == keyUP[inputMode])
      mUpDisabled = false;
   else if(keyCode == keyDOWN[inputMode])
      mDownDisabled = false;
   else if(keyCode == keyLEFT[inputMode])
      mLeftDisabled = false;
   else if(keyCode == keyRIGHT[inputMode])
      mRightDisabled = false;
}

// Return current move (actual move processing in ship.cpp)
// Will also transform move into "relative" mode if needed
// Note that all input supplied here will be overwritten if
// we are using a game controller. 
// Runs only on client
Move *GameUserInterface::getCurrentMove()
{
   if((mCurrentMode != ChatMode) && !gDisableShipKeyboardInput && !OGLCONSOLE_GetVisibility())
   {
      InputMode inputMode = gIniSettings.inputMode;
      mCurrentMove.up = !mUpDisabled && getKeyState(keyUP[inputMode]) ? 1 : 0;
      mCurrentMove.down = !mDownDisabled && getKeyState(keyDOWN[inputMode]) ? 1 : 0;
      mCurrentMove.left = !mLeftDisabled && getKeyState(keyLEFT[inputMode]) ? 1 : 0;
      mCurrentMove.right = !mRightDisabled && getKeyState(keyRIGHT[inputMode]) ? 1 : 0;

      mCurrentMove.fire = mFiring;

      for(U32 i = 0; i < (U32)ShipModuleCount; i++)
         mCurrentMove.module[i] = mModActivated[i];
   }
   else
   {
      mCurrentMove.up = 0;
      mCurrentMove.down = 0;
      mCurrentMove.left = 0;
      mCurrentMove.right = 0;

      mCurrentMove.fire = mFiring;     // should be false?

      for(U32 i = 0; i < (U32)ShipModuleCount; i++)
         mCurrentMove.module[i] = false;
   }

   if(!gIniSettings.controlsRelative)
      return &mCurrentMove;

   else     // Using relative controls -- all turning is done relative to the direction of the ship.
   {
      mTransformedMove = mCurrentMove;

      Point moveDir(mCurrentMove.right - mCurrentMove.left,
                    mCurrentMove.up - mCurrentMove.down);

      Point angleDir(cos(mCurrentMove.angle), sin(mCurrentMove.angle));

      Point rightAngleDir(-angleDir.y, angleDir.x);
      Point newMoveDir = angleDir * moveDir.y + rightAngleDir * moveDir.x;

      if(newMoveDir.x > 0)
      {
         mTransformedMove.right = newMoveDir.x;
         mTransformedMove.left = 0;
      }
      else
      {
         mTransformedMove.right = 0;
         mTransformedMove.left = -newMoveDir.x;
      }
      if(newMoveDir.y > 0)
      {
         mTransformedMove.down = newMoveDir.y;
         mTransformedMove.up = 0;
      }
      else
      {
         mTransformedMove.down = 0;
         mTransformedMove.up = -newMoveDir.y;
      }

      // Sanity checks
      mTransformedMove.right = min(1.0f, mTransformedMove.right);
      mTransformedMove.left  = min(1.0f, mTransformedMove.left);
      mTransformedMove.up    = min(1.0f, mTransformedMove.up);
      mTransformedMove.down  = min(1.0f, mTransformedMove.down);

      return &mTransformedMove;
   }
}


// User has finished entering a chat message and pressed <enter>
void GameUserInterface::issueChat()
{
   if(!mLineEditor.isEmpty())
   {
      // Check if chat buffer holds a message or a command
      if(mLineEditor.at(0) != '/' && mCurrentChatType != CmdChat)                   // It's a normal chat message
      {
         GameType *gt = gClientGame->getGameType();
         if(gt)
            gt->c2sSendChat(mCurrentChatType == GlobalChat, mLineEditor.c_str());   // Broadcast message
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


// Process a command entered at the chat prompt
// Returns true if command was handled (even if it was bogus); returning false will cause command to be passed on to the server
// Runs on client
void GameUserInterface::runCommand(const char *input)
{
   Vector<string> words = parseStringx(input);  // yes

   if(words.size() == 0)            // Just in case, must have 1 or more words to check the first word as command.
      return;

   GameConnection *gc = gClientGame->getConnectionToServer();
   if(!gc)
   {
      displayErrorMessage("!!! Not connected to server");
      return;
   }

   for(U32 i = 0; i < ARRAYSIZE(chatCmds); i++)
      if(words[0] == chatCmds[i].cmdName)
      {
         chatCmds[i].cmdCallback(this, words);
         return;
      }

   serverCommandHandler(this, words);     // Command unknown to client, will pass it on to server
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
      gIniSettings.sfxVolLevel = (F32) vol / 10.0;
      displayMessage(gCmdChatColor, "SFX volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
      return;
   case MusicVolumeType:
      gIniSettings.musicVolLevel = (F32) vol / 10.0;
      displayMessage(gCmdChatColor, "Music volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
      return;
   case VoiceVolumeType:
      gIniSettings.voiceChatVolLevel = (F32) vol / 10.0;
      displayMessage(gCmdChatColor, "Voice chat volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
      return;
   case ServerAlertVolumeType:
      gClientGame->getConnectionToServer()->c2sSetServerAlertVolume((S8) vol);
      displayMessage(gCmdChatColor, "Server alerts chat volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
      return;
  }
}


void GameUserInterface::cancelChat()
{
   mLineEditor.clear();
   enterMode(PlayMode);
}


bool GameUserInterface::isOnMuteList(const string &name)
{
   for(S32 i = 0; i < mMuteList.size(); i++)
   {
      if(mMuteList[i] == name)
         return true;
   }
   
   return false;
}


// Constructor
GameUserInterface::VoiceRecorder::VoiceRecorder()
{
   mRecordingAudio = false;
   mMaxAudioSample = 0;
   mMaxForGain = 0;
   mVoiceEncoder = new LPC10VoiceEncoder;
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
      glVertex2f(10, 130);
      glVertex2f(10, 145);
      glVertex2f(10 + totalLineCount * 2, 130);
      glVertex2f(10 + totalLineCount * 2, 145);

      F32 halfway = totalLineCount * 0.5;
      F32 full = amt * totalLineCount;
      for(U32 i = 1; i < full; i++)
      {
         if(i < halfway)
            glColor3f(i / halfway, 1, 0);
         else
            glColor3f(1, 1 - (i - halfway) / halfway, 0);

         glVertex2f(10 + i * 2, 130);
         glVertex2f(10 + i * 2, 145);
      }
      glEnd();
   }
}


void GameUserInterface::VoiceRecorder::start()
{
   if(!mRecordingAudio)
   {
      mRecordingAudio = SFXObject::startRecording();
      if(!mRecordingAudio)
         return;

      mUnusedAudio = new ByteBuffer(0);
      mRecordingAudio = true;
      mMaxAudioSample = 0;
      mVoiceAudioTimer.reset(FirstVoiceAudioSampleTime);

      // trim the start of the capture buffer:
      SFXObject::captureSamples(mUnusedAudio);
      mUnusedAudio->resize(0);
   }
}

void GameUserInterface::VoiceRecorder::stop()
{
   if(mRecordingAudio)
   {
      process();

      mRecordingAudio = false;
      SFXObject::stopRecording();
      mVoiceSfx = NULL;
      mUnusedAudio = NULL;
   }
}


void GameUserInterface::VoiceRecorder::process()
{
   U32 preSampleCount = mUnusedAudio->getBufferSize() / 2;
   SFXObject::captureSamples(mUnusedAudio);

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
      // apply some gain to the buffer:
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
      GameType *gt = gClientGame->getGameType();
      if(gt)
         gt->c2sVoiceChat(gIniSettings.echoVoice, sendBuffer);
   }
}


void GameUserInterface::suspendGame()
{
   gClientGame->getConnectionToServer()->suspendGame();     // Tell server we're suspending
   gClientGame->suspendGame();                              // Suspend locally
   gSuspendedInterface.activate();                          // And enter chat mode
}


void GameUserInterface::unsuspendGame()
{
   gClientGame->unsuspendGame();                            // Unsuspend locally
   gClientGame->getConnectionToServer()->unsuspendGame();   // Tell the server we're unsuspending
}


};

