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


// Constructor
GameUserInterface::GameUserInterface()
{
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

   for(U32 i = 0; i < FPS_AVG_COUNT; i++)
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

   populateChatCmdList();

   remoteLevelDownloadFilename = "downloaded.level";
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

         for(U32 i = 0; i < FPS_AVG_COUNT; i++)
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

   drawString(xpos, ypos, FONTSIZE, mLineEditor.c_str());

   mLineEditor.drawCursor(xpos, ypos, FONTSIZE);
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

   if(keyCode == keyMOD1[inputMode])
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
      if(isCmdChat())     // It's a command!
      {
         S32 found = -1;
         S32 start = mCurrentChatType == CmdChat ? 1 : 0;     // Do we need to lop the leading '/' off mChatCmds item?

         size_t len = mLineEditor.length();
         for(S32 i = 0; i < mChatCmds.size(); i++)
            if(mChatCmds[i].substr(start, len) == mLineEditor.getString())
            {
               if(found != -1)   // We found multiple matches, so that means it's not yet unique enough to autocomplete.
                  return;
               found = i;
            }

         if(found == -1)         // Found no match... no expansion possible
            return;

         mLineEditor.clear();
         mLineEditor.setString(mChatCmds[found].substr(start));    // Add the command
         mLineEditor.addChar(' ');                                 // Add a space
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
      else                             // It's a command
      {
         Vector<string> words = parseString(mLineEditor.c_str());

         if(!processCommand(words))    // Try the command locally; if can't be run, send it on to the server
         {
            const char * c1 = mLineEditor.c_str();
            GameType *gt = gClientGame->getGameType();
            if(gt)
            {
               Vector<StringPtr> args;
               for(S32 i = 1; i < words.size(); i++)
                  args.push_back(StringPtr(words[i]));
               gt->c2sSendCommand(StringTableEntry(words[0], false), args);
            }
         }
      }
   }
   cancelChat();
}


Vector<string> GameUserInterface::parseString(const char *str)
{
   // Parse the string
   string word = "";
   Vector<string> words;

   S32 startIndex = (mCurrentChatType == CmdChat) ? 0 : 1;        // Start at 1 to omit the leading '/'

   for(size_t i = startIndex; i < strlen(str); i++)
   {
      if(str[i] != ' ')
         word += words.size() == 0 ? tolower(str[i]) : str[i];    // Make first word all lower case for case insensitivity
      else if(word != "")
      {
         words.push_back(word);
         word = "";
      }
   }
   if(word != "")
      words.push_back(word);

   return words;
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


extern CIniFile gINI;
extern md5wrapper md5;

static void changePassword(GameConnection *gc, GameConnection::ParamType type, Vector<string> &words, bool required)
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


static void changeServerNameDescr(GameConnection *gc, GameConnection::ParamType type, Vector<string> &words)
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



// returns a pointer of string of chars, after "count" number of args
const char * findPointerOfArg(const char *message, S32 count)
{
   S32 spacecount = 0;
   S32 cur = 0;
   char prevchar = 0;

   // Message needs to include everything including multiple spaces.  Message starts after second space.
   while(message[cur] != '\0' && spacecount != count)
   {
      if(message[cur] == ' ' && prevchar!=' ') 
         spacecount++;  // Double space does not count as a seperate parameter
      prevchar = message[cur];
      cur++;
   }
   return &message[cur];
}




extern ClientInfo gClientInfo;
extern bool showDebugBots;  // in game.cpp

// Process a command entered at the chat prompt
// Make sure any commands listed here are also included in mChatCmds for auto-completion purposes...
// Returns true if command was handled (even if it was bogus); returning false will cause command to be passed on to the server
// Runs on client
bool GameUserInterface::processCommand(Vector<string> &words)
{
   if(words.size() == 0)            // Just in case, must have 1 or more words to check the first word as command.
      return true;

   GameConnection *gc = gClientGame->getConnectionToServer();
   if(!gc)
   {
      displayErrorMessage("!!! Not connected to server");
   }
   else if(words[0] == "add")            // Add time to the game
   {
      if(words.size() < 2 || words[1] == "")
         displayErrorMessage("!!! Need to supply a time (in minutes)");
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
            displayErrorMessage("!!! Invalid value... game time not changed");
         else
         {
            displayMessage(gCmdChatColor, "Extended game by %d minute%s", mins, (mins == 1) ? "" : "s");

            if(gClientGame->getGameType())
               gClientGame->getGameType()->addTime(mins * 60 * 1000);
         }
      }
   }

   else if(words[0] == "next")      // Go to next level
   {
      if(!gc->isLevelChanger())
         displayErrorMessage("!!! You don't have permission to change levels");
      else
         gc->c2sRequestLevelChange(ServerGame::NEXT_LEVEL, false);
   }
   else if(words[0] == "prev")      // Go to previous level
   {
      if(!gc->isLevelChanger())
         displayErrorMessage("!!! You don't have permission to change levels");
      else
         gc->c2sRequestLevelChange(ServerGame::PREVIOUS_LEVEL, false);
   }
   else if(words[0] == "restart")      // Restart current level
   {
      if(!gc->isLevelChanger())
         displayErrorMessage("!!! You don't have permission to change levels");
      else
         gc->c2sRequestLevelChange(ServerGame::REPLAY_LEVEL, false);
   }
   else if(words[0] == "shutdown")
   {
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
   else if(words[0] == "kick")      // Kick a player
   {
      if(hasAdmin(gc, "!!! You don't have permission to kick players"))
      {
         if(words.size() < 2 || words[1] == "")
            displayErrorMessage("!!! Need to specify who to kick");
         else
         {
            // Did user provide a valid, known name?
            if(gClientGame->getGameType())
            {
               ClientRef *clientRef = gClientGame->getGameType()->findClientRef(words[1].c_str());
               if(!clientRef)
                  displayErrorMessage("!!! Could not find player %s", words[1].c_str());
               else
                  gc->c2sAdminPlayerAction(words[1].c_str(), PlayerMenuUserInterface::Kick, 0);     // Team doesn't matter with kick!
            }
         }
      }
   }

   else if(words[0] == "admin")     // Request admin permissions
   {
      if(words.size() < 2 || words[1] == "")
         displayErrorMessage("!!! Need to supply a password");
      else if(gc->isAdmin())
         displayErrorMessage("!!! You are already an admin");
      else
         gc->submitAdminPassword(words[1].c_str());
   }

   else if(words[0] == "levpass" || words[0] == "levelpass" || words[0] == "lvlpass")
   {
      if(words.size() < 2 || words[1] == "")
         displayErrorMessage("!!! Need to supply a password");
      else if(gc->isLevelChanger())
         displayErrorMessage("!!! You can already change levels");
      else
         gc->submitLevelChangePassword(words[1].c_str());
   }
   else if(words[0] == "dcoords")
      mDebugShowShipCoords = !mDebugShowShipCoords;
   else if(words[0] == "dzones")
   {
       mDebugShowMeshZones = !mDebugShowMeshZones;
       if(!gServerGame) displayErrorMessage("!!! Zones can only be displayed on a local host");
   }
   else if(words[0] == "drobot")
   {
       showDebugBots = !showDebugBots;
       if(!gServerGame) displayErrorMessage("!!! d-robot can only be displayed on a local host");
   }
   else if(words[0] == "svol")      // SFX volume
      setVolume(SfxVolumeType, words);
   else if(words[0] == "mvol")      // Music volume
      setVolume(MusicVolumeType, words);
   else if(words[0] == "vvol")      // Voice chat volume
      setVolume(VoiceVolumeType, words);
   else if(words[0] == "servvol")   // Server alerts volume
      setVolume(ServerAlertVolumeType, words);

   else if(words[0] == "setadminpass")
   {
      if(hasAdmin(gc, "!!! You don't have permission to set the admin password"))
         changePassword(gc, GameConnection::AdminPassword, words, true);
   }

   else if(words[0] == "setserverpass")
   {
      if(hasAdmin(gc, "!!! You don't have permission to set the server password"))
         changePassword(gc, GameConnection::ServerPassword, words, false);
   }

   else if(words[0] == "setlevpass")
   {
      if(hasAdmin(gc, "!!! You don't have permission to set the level change password"))
         changePassword(gc, GameConnection::LevelChangePassword, words, false);
   }

   else if(words[0] == "setservername")
   {
      if(hasAdmin(gc, "!!! You don't have permission to set the server name"))
         changeServerNameDescr(gc, GameConnection::ServerName, words);
   }

   else if(words[0] == "setserverdescr")
   {
      if(hasAdmin(gc, "!!! You don't have permission to set the server description"))
         changeServerNameDescr(gc, GameConnection::ServerDescr, words);
   }

   else if(words[0] == "deletecurrentlevel")
   {
      if(hasAdmin(gc, "!!! You don't have permission to delete the current level"))
         changeServerNameDescr(gc, GameConnection::DeleteLevel, words);
   }

   else if(words[0] == "suspend")
   {
      U32 players = gClientGame->getPlayerCount();
      if(players == (U32)Game::PLAYER_COUNT_UNAVAILABLE || players > 1)
         displayErrorMessage("!!! Can't suspend when others are playing");
      else
         suspendGame();    // Do the deed
   }
   else if(words[0] == "linewidth")
   {
      F32 linewidth;
      if(words.size() < 2 || words[1] == "")
         displayErrorMessage("!!! Need to supply line width");
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
   else if(words[0] == "linesmooth")
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
   else if(words[0] == "maxfps")
   {
      S32 number = words.size() > 1 ? atoi(words[1].c_str()) : 0;
      if(number < 1)                              // Don't allow zero or negative numbers
         displayErrorMessage("!!! Usage: /maxfps <frame rate>, default = 100");
      else
         gIniSettings.maxFPS = number;
   }
   else if(words[0] == "pm")
   {
      if(words.size() < 2)
         displayErrorMessage("!!! Usage: /pm <player name> <message>");
      else
      {
         const char *message = mLineEditor.c_str();  // Get the original line.
         message = findPointerOfArg(message,2); // Set pointer after 2 args

         // The above keeps multiple space, but not the one below.
         //string message = concatenate(words, 2);  // in this line, multi space converts to single space; "1   2      3" becomes "1 2 3"

         GameType *gt = gClientGame->getGameType();
         if(gt)
            gt->c2sSendChatPM(words[1], message); //.c_str()
      }
   }
   else if(words[0] == "getmap" || words[0] == "getlevel")    // Getmap or getlevel?  Allow either...
   {
      if(gClientGame->getConnectionToServer()->isLocalConnection())
         displayErrorMessage("!!! Can't download levels from a local server");
      else
      {
         if(words.size() > 1 && words[1] != "")
            remoteLevelDownloadFilename = words[1];
			else
            remoteLevelDownloadFilename = "downloaded_" + makeFilenameFromString(gClientGame->getGameType() ? 
                                                                        gClientGame->getGameType()->mLevelName.getString() : "Level");
         // Add an extension if needed
         if(remoteLevelDownloadFilename.find(".") == string::npos)
            remoteLevelDownloadFilename += ".level";

         // Make into a fully qualified file name
         string fullFile = strictjoindir(gConfigDirs.levelDir, remoteLevelDownloadFilename);

         // Prepare for writing
         mOutputFile = fopen(fullFile.c_str(), "w");    // TODO: Writes empty file when server does not allow getmap.  Shouldn't.

         if(!mOutputFile)
         {
            logprintf("Problem opening file %s for writing", fullFile.c_str());
            displayErrorMessage("!!! Problem opening file %s for writing", fullFile.c_str());
         }
         else
            gClientGame->getConnectionToServer()->c2sRequestCurrentLevel();
      }
   }
   else if(words[0] == "engf" || words[0] == "engt")
   {
      Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
      GameType *gt = gClientGame->getGameType();
      EngineerModuleDeployer deployer;

      if(!(gt && gt->engineerIsEnabled()))
         displayErrorMessage("!!! %s only works on levels where Engineer module is allowed", words[0].c_str());
      else
      {
         EngineerBuildObjects objType = (words[0] == "engf") ? EngineeredForceField : EngineeredTurret;

         // Check deployment status on client; will be checked again on server, but server will only handle likely valid placements
         if(!deployer.canCreateObjectAtLocation(ship, objType))     
            displayErrorMessage(deployer.getErrorMessage().c_str());
         else
            gc->c2sEngineerDeployObject(objType);
      }
   }
   else
      return false;     // Command unknown to client, will pass it on to server

   return true;         // Handled
}


// For auto-completion purposes
void GameUserInterface::populateChatCmdList()
{
   // Our list of commands that can be entered at the chat prompt
   mChatCmds.push_back("/add");
   mChatCmds.push_back("/admin");
   mChatCmds.push_back("/dcoords");
   mChatCmds.push_back("/dzones");
   mChatCmds.push_back("/drobot");
   mChatCmds.push_back("/levpass");
   mChatCmds.push_back("/mvol");
   mChatCmds.push_back("/next");
   mChatCmds.push_back("/prev");
   mChatCmds.push_back("/restart");
   mChatCmds.push_back("/svol");
   mChatCmds.push_back("/vvol");
   mChatCmds.push_back("/suspend");
   mChatCmds.push_back("/linewidth");
   mChatCmds.push_back("/linesmooth");
   mChatCmds.push_back("/maxfps");
   mChatCmds.push_back("/pm");
   mChatCmds.push_back("/getmap");

   // commands that runs in game server, in processServerCommand
   mChatCmds.push_back("/settime");
   mChatCmds.push_back("/setscore");
   mChatCmds.push_back("/showBots");

   // Administrative commands
   mChatCmds.push_back("/kick");
   mChatCmds.push_back("/shutdown");
   mChatCmds.push_back("/servvol");
   mChatCmds.push_back("/setlevpass");
   mChatCmds.push_back("/setadminpass");
   mChatCmds.push_back("/setserverpass");
   mChatCmds.push_back("/setservername");
   mChatCmds.push_back("/setserverdescr");
   mChatCmds.push_back("/deletecurrentlevel");

   // Engineer commands
   mChatCmds.push_back("/engf");
   mChatCmds.push_back("/engt");
}


// Set specified volume to the specefied level
void GameUserInterface::setVolume(VolumeType volType, Vector<string> &words)
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

