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

#include "UIGame.h"
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
#include "lpc10.h"
#include "IniFile.h"             // For access to gINI functions
#include "engineeredObjects.h"   // For EngineerModuleDeployer
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
#include "game.h"

#include "../tnl/tnlEndian.h"

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

namespace Zap
{
extern ConfigDirectories gConfigDirs;          //in main.cpp for Getmap

//GameUserInterface mGameUserInterface;

// TODO: Make these static like privateF5MessageDisplayedInGameColor!
Color gGlobalChatColor(0.9, 0.9, 0.9);
Color gTeamChatColor(Colors::green);
extern Color gCmdChatColor;


Color GameUserInterface::privateF5MessageDisplayedInGameColor(Colors::blue);


static void makeCommandCandidateList();      // Forward delcaration

// Constructor
GameUserInterface::GameUserInterface()
{
   //mOutputFile = NULL;
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

   for(S32 i = 0; i < ChatMessageStoreCount; i++)
      mStoreChatMessage[i][0] = 0;

   for(S32 i = 0; i < ChatMessageDisplayCount; i++)
      mDisplayChatMessage[i][0] = 0;

   mGotControlUpdate = false;
   mRecalcFPSTimer = 0;

   mFiring = false;
   for (U32 i = 0; i < (U32)ShipModuleCount; i++)
      mModActivated[i] = false;

   mDisplayMessageTimer.setPeriod(DisplayMessageTimeout);    // Set the period of our message timeout timer
   mDisplayChatMessageTimer.setPeriod(DisplayChatMessageTimeout);
   //populateChatCmdList();

   remoteLevelDownloadFilename = "downloaded.level";

   makeCommandCandidateList();
}


// Destructor
GameUserInterface::~GameUserInterface()
{
   //if(mOutputFile)
   //   fclose(mOutputFile);
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
   SDL_ShowCursor(SDL_DISABLE);    // Turn off cursor
   onMouseMoved();                     // Make sure ship pointed is towards mouse

   // Clear out any lingering chat messages
   for(S32 i = 0; i < ChatMessageStoreCount; i++)
      mDisplayMessage[i][0] = 0;

   for(S32 i = 0; i < MessageDisplayCount; i++)
      mStoreChatMessage[i][0] = 0;

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
   SDL_ShowCursor(SDL_DISABLE);    // Turn off cursor
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


// A new server message is here!  We don't actually display anything here, despite the name...
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

   va_list args;

   va_start(args, format);
   vsnprintf(mDisplayMessage[0], sizeof(mDisplayMessage[0]), format, args);
   va_end(args);
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
   // Update some timers
   mShutdownTimer.update(timeDelta);
   mInputModeChangeAlertDisplayTimer.update(timeDelta);
   mWrongModeMsgDisplay.update(timeDelta);
   mProgressBarFadeTimer.update(timeDelta);
   mLevelInfoDisplayTimer.update(timeDelta);


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
      renderMessageDisplay();    // Render incoming server msgs
      renderChatMessageDisplay();    // Render incoming chat msgs
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

// Render any incoming server msgs
void GameUserInterface::renderMessageDisplay()
{
   glColor3f(1,1,1);

   S32 y = gIniSettings.showWeaponIndicators ? UserInterface::messageMargin : UserInterface::vertMargin;
   S32 msgCount;

   msgCount = MessageDisplayCount;  // Short form

   S32 y_end = y + msgCount * (FONTSIZE + FONT_GAP);

   for(S32 i = msgCount - 1; i >= 0; i--)
   {
      if(mDisplayMessage[i][0])
      {
         glColor(mDisplayMessageColor[i]);
         //drawString(UserInterface::horizMargin, y, FONTSIZE, mDisplayMessage[i]);
         //y += FONTSIZE + FONT_GAP;
         y += (FONTSIZE + FONT_GAP)
            * UserInterface::drawWrapText(mDisplayMessage[i], UserInterface::horizMargin, y,
               750, // wrap width
               y_end, // ypos_end
               FONTSIZE + FONT_GAP, // line height
               FONTSIZE, // font size
               false); // align top
      }
   }
}


static const S32 CHAT_FONTSIZE = 12;
static const S32 CHAT_FONT_GAP = 3;

// Render any incoming player chat msgs
void GameUserInterface::renderChatMessageDisplay()
{
   glColor3f(1,1,1);

   S32 y = UserInterface::chatMessageMargin;
   S32 msgCount;

   if(mMessageDisplayMode == LongFixed)
      msgCount = ChatMessageStoreCount;    // Long form
   else
      msgCount = ChatMessageDisplayCount;  // Short form

   S32 y_end = y - msgCount * (CHAT_FONTSIZE + CHAT_FONT_GAP);

   if(mHelper)
      glEnableBlend;

   if(mMessageDisplayMode == ShortTimeout)
      for(S32 i = 0; i < msgCount; i++)
      {
         if(mDisplayChatMessage[i][0])
         {
            if (mHelper)
               glColor(mDisplayChatMessageColor[i], 0.2);
            else
               glColor(mDisplayChatMessageColor[i]);

            //drawString(UserInterface::horizMargin, y, CHAT_FONTSIZE, mDisplayChatMessage[i]);
            y -= (CHAT_FONTSIZE + CHAT_FONT_GAP)
               * UserInterface::drawWrapText(mDisplayChatMessage[i], UserInterface::horizMargin, y,
                  700, // wrap width
                  y_end, // ypos_end
                  CHAT_FONTSIZE + CHAT_FONT_GAP, // line height
                  CHAT_FONTSIZE, // font size
                  true); // align bottom
         }
      }
   else
      for(S32 i = 0; i < msgCount; i++)
      {
         if(mStoreChatMessage[i][0])
         {
            if (mHelper)
               glColor(mStoreChatMessageColor[i], 0.2);
            else
               glColor(mStoreChatMessageColor[i]);

            //drawString(UserInterface::horizMargin, y, CHAT_FONTSIZE, mStoreChatMessage[i]);
            y -= (CHAT_FONTSIZE + CHAT_FONT_GAP)
               * UserInterface::drawWrapText(mStoreChatMessage[i], UserInterface::horizMargin, y,
                  700, // wrap width
                  y_end, // ypos_end
                  CHAT_FONTSIZE + CHAT_FONT_GAP, // line height
                  CHAT_FONTSIZE, // font size
                  true); // align bottom
         }
      }

   if(mHelper)
      glDisableBlend;
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

      Point o = ship->getRenderPos();  // To avoid taking address of temporary
      Point p = gClientGame->worldToScreenPoint( &o );

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
   else if(keyCode == keyDIAG)            // Turn on diagnostic overlay
      gDiagnosticInterface.activate();
   else if(keyCode == keyMISSION)
   {
      mMissionOverlayActive = true;
      gClientGame->getUserInterface()->clearLevelInfoDisplayTimer();    // Clear level-start display if user hits F2
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


// Returns true if we have admin privs, displays error message and returns false if not
bool GameUserInterface::hasAdmin(GameConnection *gc, const char *failureMessage)
{
   if(gc->isAdmin())
      return true;
   
   displayErrorMessage(failureMessage);
   return false;
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



// TODO: Probably misnamed... handles deletes too
void GameUserInterface::changeServerNameDescr(GameConnection *gc, GameConnection::ParamType type, const Vector<string> &words)
{
   // Concatenate all params into a single string
   string allWords = concatenate(words, 1);

   // Did the user provide a name/description?
   if(type != GameConnection::DeleteLevel && allWords == "")
   {
      displayErrorMessage(type == GameConnection::ServerName ? "!!! Need to supply a name" : "!!! Need to supply a description");
      return;
   }

   gc->changeParam(allWords.c_str(), type);
}

extern CIniFile gINI;
extern md5wrapper md5;

void GameUserInterface::changePassword(GameConnection *gc, GameConnection::ParamType type, const Vector<string> &words, bool required)
{
   if(required)
   {
      if(words.size() < 2 || words[1] == "")
      {
         displayErrorMessage("!!! Need to supply a password");
         return;
      }

      gc->changeParam(words[1].c_str(), type);
   }
   else if(words.size() < 2)
      gc->changeParam("", type);

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

   for(S32 i = 0; i < gClientGame->getGameType()->getClientCount(); i++)
   {
      if(!gClientGame->getGameType()->getClient(i).isValid())
         continue;

      // TODO: make this work with StringTableEntry comparison rather than strcmp; might need to add new method
      const char *n = gClientGame->getGameType()->getClient(i)->name.getString();

      if(!strcmp(n, name.c_str()))           // Exact match
         return true;
      else if(!stricmp(n, name.c_str()))     // Case insensitive match
         potentials++;
   }

   return(potentials == 1);      // Return true if we only found exactly one potential match, false otherwise
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
               gClientGame->getGameType()->getLevelName()->getString() : "Level");

      // Add an extension if needed
      if(gui->remoteLevelDownloadFilename.find(".") == string::npos)
         gui->remoteLevelDownloadFilename += ".level";

      // Make into a fully qualified file name
      gui->mOutputFileName = strictjoindir(gConfigDirs.levelDir, gui->remoteLevelDownloadFilename);

      // Prepare for writing
      //gui->mOutputFile = fopen(fullFile.c_str(), "w");    // TODO: Writes empty file when server does not allow getmap.  Shouldn't.

      //if(!gui->mOutputFile)
      //{
      //   logprintf("Problem opening file %s for writing", fullFile.c_str());
      //   gui->displayErrorMessage("!!! Problem opening file %s for writing", fullFile.c_str());
      //}
      //else
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

   if(gui->hasAdmin(gc, "!!! You don't have permission to shut the server down"))
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

   if(gui->hasAdmin(gc, "!!! You don't have permission to kick players"))
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
   if(!(gServerGame))// && gServerGame->isTestServer()))  sam: problem with not being able to test from editor due to editor crashing and loading improperly...
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

   if(gui->hasAdmin(gc, "!!! You don't have permission to set the admin password"))
      gui->changePassword(gc, GameConnection::AdminPassword, words, true);
}

void GameUserInterface::setServerPassHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(gui->hasAdmin(gc, "!!! You don't have permission to set the server password"))
      gui->changePassword(gc, GameConnection::ServerPassword, words, false);
}

void GameUserInterface::setLevPassHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(gui->hasAdmin(gc, "!!! You don't have permission to set the level change password"))
      gui->changePassword(gc, GameConnection::LevelChangePassword, words, false);
}

void GameUserInterface::setServerNameHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(gui->hasAdmin(gc, "!!! You don't have permission to set the server name"))
      gui->changeServerNameDescr(gc, GameConnection::ServerName, words);
}

void GameUserInterface::setServerDescrHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(gui->hasAdmin(gc, "!!! You don't have permission to set the server description"))
      gui->changeServerNameDescr(gc, GameConnection::ServerDescr, words);
}


void GameUserInterface::deleteCurrentLevelHandler(GameUserInterface *gui, const Vector<string> &words)
{
   GameConnection *gc = gClientGame->getConnectionToServer();

   if(gui->hasAdmin(gc, "!!! You don't have permission to delete the current level"))
      gui->changeServerNameDescr(gc, GameConnection::DeleteLevel, words);    // handles deletes too
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

   S32 promptSize = getStringWidth(CHAT_FONTSIZE, promptStr);
   S32 nameSize = getStringWidthf(CHAT_FONTSIZE, "%s: ", gClientGame->getConnectionToServer()->getClientName().getString());
   S32 nameWidth = max(nameSize, promptSize);
   // Above block repeated below...

   const S32 ypos = chatMessageMargin + CHAT_FONTSIZE + (2 * CHAT_FONT_GAP) + 5;

   S32 boxWidth = gScreenInfo.getGameCanvasWidth() - 2 * horizMargin - (nameWidth - promptSize) - 230;

   // Render text entry box like thingy
   glEnableBlend;

   for(S32 i = 1; i >= 0; i--)
   {
      glColor(baseColor, i ? .25 : .4);

      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2f(horizMargin, ypos - 3);
         glVertex2f(horizMargin + boxWidth, ypos - 3);
         glVertex2f(horizMargin + boxWidth, ypos + CHAT_FONTSIZE + 7);
         glVertex2f(horizMargin, ypos + CHAT_FONTSIZE + 7);
      glEnd();
   }
   glDisableBlend;

   glColor(baseColor);

   // Display prompt
   S32 promptWidth = getStringWidth(CHAT_FONTSIZE, promptStr);
   S32 xStartPos = horizMargin + 3 + promptWidth;

   drawString(horizMargin + 3, ypos, CHAT_FONTSIZE, promptStr);  // draw prompt

   // Display typed text
   string displayString = mLineEditor.getString();
   S32 displayWidth = getStringWidth(CHAT_FONTSIZE, displayString.c_str());

   // If the string goes too far out of bounds, display it chopped off at the front to give more room to type
   while (displayWidth > boxWidth - promptWidth - 16)  // 16 -> Account for margin and cursor
   {
      displayString = displayString.substr(25, string::npos);  // 25 -> # chars to chop off displayed text if overflow
      displayWidth = getStringWidth(CHAT_FONTSIZE, displayString.c_str());
   }

   drawString(xStartPos, ypos, CHAT_FONTSIZE, displayString.c_str());

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
                  drawString(xStartPos + displayWidth, ypos, CHAT_FONTSIZE, chatCmds[i].helpArgString[words.size() - 1].c_str());
               }

               break;
            }
         }
      }
   }

   glColor(baseColor);
   mLineEditor.drawCursor(xStartPos, ypos, CHAT_FONTSIZE, displayWidth);
}


static Vector<string> commandCandidateList;
static Vector<string> nameCandidateList;         // Reuse this for player and team names

static void makeCommandCandidateList()
{
   for(S32 i = 0; i < chatCmdSize; i++)
      commandCandidateList.push_back(chatCmds[i].cmdName);
}

static void makePlayerNameCandidateList(const Game *game)
{
   nameCandidateList.clear();

   if(gClientGame->getGameType())
      for(S32 i = 0; i < game->getGameType()->getClientCount(); i++)
         if(game->getGameType()->getClient(i).isValid())
            nameCandidateList.push_back(game->getGameType()->getClient(i)->name.getString());
}


static void makeTeamNameCandidateList(const Game *game)
{
   nameCandidateList.clear();

   if(gClientGame->getGameType())
      for(S32 i = 0; i < game->getTeamCount(); i++)
         nameCandidateList.push_back(game->getTeamName(i).getString());
}


static Vector<string> *getCandidateList(Game *game, const string &cmdName, S32 arg)
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
            makePlayerNameCandidateList(game);
            return &nameCandidateList;
         }

         else if(argType == TEAM)      // Team names
         {
            makeTeamNameCandidateList(game);
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

         Vector<string> *candidates = getCandidateList(gClientGame, words[0], arg);     // Could return NULL

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
         S32 promptSize = getStringWidth(CHAT_FONTSIZE, mCurrentChatType == TeamChat ? "(Team): " : "(Global): ");

         //Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
         //if(!ship)
         //   return;  // problem with unable to type something while trying to respawn.

         S32 nameSize = getStringWidthf(CHAT_FONTSIZE, "%s: ", gClientGame->getConnectionToServer()->getClientName().getString());
         S32 nameWidth = max(nameSize, promptSize);
         // Above block repeated above

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
   mVoiceEncoder = new SpeexVoiceEncoder;
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
      if(gt && sendBuffer->getBufferSize() < 1024)      // don't try to send too big.
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


void GameUserInterface::renderScoreboard(const GameType *gameType)
{
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   if(mLevelInfoDisplayTimer.getCurrent() || mMissionOverlayActive)
   {
      F32 alpha = 1;
      if(mLevelInfoDisplayTimer.getCurrent() < 1000 && !mMissionOverlayActive)
         alpha = mLevelInfoDisplayTimer.getCurrent() * 0.001f;

      glEnableBlend;
         glColor(Colors::white, alpha);
         UserInterface::drawCenteredStringf(canvasHeight / 2 - 180, 30, "Level: %s", gameType->getLevelName()->getString());
         UserInterface::drawCenteredStringf(canvasHeight / 2 - 140, 30, "Game Type: %s", gameType->getGameTypeString());
         glColor(Colors::cyan, alpha);
         UserInterface::drawCenteredString(canvasHeight / 2 - 100, 20, gameType->getInstructionString());
         glColor(Colors::magenta, alpha);
         UserInterface::drawCenteredString(canvasHeight / 2 - 75, 20, gameType->getLevelDescription()->getString());

         glColor(Colors::green, alpha);
         UserInterface::drawCenteredStringf(canvasHeight - 100, 20, "Press [%s] to see this information again", keyCodeToString(keyMISSION));

         if(gameType->getLevelCredits()->isNull())    // Display credits if it's not empty
         {
            glColor(Colors::red, alpha);
            UserInterface::drawCenteredStringf(canvasHeight / 2 + 50, 20, "%s", gameType->getLevelCredits()->getString());
         }

         glColor4f(1, 1, 0, alpha);
         UserInterface::drawCenteredStringf(canvasHeight / 2 - 50, 20, "Score to Win: %d", gameType->getWinningScore());

      glDisableBlend;

      mInputModeChangeAlertDisplayTimer.reset(0);     // Supress mode change alert if this message is displayed...
   }

   if(mInputModeChangeAlertDisplayTimer.getCurrent() != 0)
   {
      // Display alert about input mode changing
      F32 alpha = 1;
      if(mInputModeChangeAlertDisplayTimer.getCurrent() < 1000)
         alpha = mInputModeChangeAlertDisplayTimer.getCurrent() * 0.001f;

      glEnableBlend;
      glColor4f(1, 0.5 , 0.5, alpha);
      UserInterface::drawCenteredStringf(UserInterface::vertMargin + 130, 20, "Input mode changed to %s", 
                                         gIniSettings.inputMode == Joystick ? "Joystick" : "Keyboard");
      glDisableBlend;
   }

   U32 totalWidth = gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin * 2;

   Game *game = gameType->getGame();
   S32 teams = gameType->isTeamGame() ? game->getTeamCount() : 1;

   U32 columnCount = min(teams, 2);

   U32 teamWidth = totalWidth / columnCount;
   S32 maxTeamPlayers = 0;
   gameType->countTeamPlayers();

   // Check to make sure at least one team has at least one player...
   for(S32 i = 0; i < game->getTeamCount(); i++)
   {
      Team *team = (Team *)game->getTeam(i);
      if(gameType->isTeamGame())
      {     // (braces required)
         if(team->getPlayerBotCount() > maxTeamPlayers)
            maxTeamPlayers = team->getPlayerBotCount();
      }
      else
         maxTeamPlayers += team->getPlayerBotCount();
   }
   // ...if not, then go home!
   if(!maxTeamPlayers)
      return;

   U32 teamAreaHeight = gameType->isTeamGame() ? 40 : 0;
   U32 numTeamRows = (game->getTeamCount() + 1) >> 1;

   U32 totalHeight = (gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin * 2) / numTeamRows - (numTeamRows - 1) * 2;
   U32 maxHeight = MIN(30, (totalHeight - teamAreaHeight) / maxTeamPlayers);

   U32 sectionHeight = (teamAreaHeight + maxHeight * maxTeamPlayers);
   totalHeight = sectionHeight * numTeamRows + (numTeamRows - 1) * 2;

   for(S32 i = 0; i < teams; i++)
   {
      S32 yt = (gScreenInfo.getGameCanvasHeight() - totalHeight) / 2 + (i >> 1) * (sectionHeight + 2);  // y-top
      S32 yb = yt + sectionHeight;     // y-bottom
      S32 xl = 10 + (i & 1) * teamWidth;
      S32 xr = xl + teamWidth - 2;

      const Color *teamColor = gameType->getGame()->getTeamColor(i);
      glEnableBlend;

      glColor(teamColor, 0.6);
      glBegin(GL_POLYGON);
         glVertex2i(xl, yt);
         glVertex2i(xr, yt);
         glVertex2i(xr, yb);
         glVertex2i(xl, yb);
      glEnd();

      glDisableBlend;

      glColor(Colors::white);
      if(gameType->isTeamGame())     // Render team scores
      {
         renderFlag(F32(xl + 20), F32(yt + 18), teamColor);
         renderFlag(F32(xr - 20), F32(yt + 18), teamColor);

         glColor(Colors::white);
         glBegin(GL_LINES);
            glVertex2i(xl, yt + S32(teamAreaHeight));
            glVertex2i(xr, yt + S32(teamAreaHeight));
         glEnd();

         UserInterface::drawString(xl + 40, yt + 2, 30, gameType->getGame()->getTeamName(i).getString());
         UserInterface::drawStringf(xr - 140, yt + 2, 30, "%d", ((Team *)(game->getTeam(i)))->getScore());
      }

      // Now for player scores.  First build a list, then sort it, then display it.
      Vector<RefPtr<ClientRef> > playerScores;
      gameType->getSortedPlayerScores(i, playerScores);     // Fills playerScores

      S32 curRowY = yt + teamAreaHeight + 1;
      S32 fontSize = U32(maxHeight * 0.8f);

      for(S32 j = 0; j < playerScores.size(); j++)
      {
         static const char *bot = "B ";
         S32 botsize = UserInterface::getStringWidth(F32(fontSize) * 0.5f, bot);
         S32 x = xl + 40;

         // Add the mark of the bot
         if(playerScores[j]->isRobot)
            UserInterface::drawString(x - botsize, curRowY + fontSize / 4 + 2, fontSize / 2, bot); 

         UserInterface::drawString(x, curRowY, fontSize, playerScores[j]->name.getString());

         static char buff[255] = "";

         if(gameType->isTeamGame())
            dSprintf(buff, sizeof(buff), "%2.2f", (F32)playerScores[j]->getRating());
         else
            dSprintf(buff, sizeof(buff), "%d", playerScores[j]->getScore());

         UserInterface::drawString(xr - (120 + S32(UserInterface::getStringWidth(F32(fontSize), buff))), curRowY, fontSize, buff);
         UserInterface::drawStringf(xr - 70, curRowY, fontSize, "%d", playerScores[j]->ping);
         curRowY += maxHeight;
      }
   }
}


// Sorts teams by score, high to low
S32 QSORT_CALLBACK teamScoreSort(Team **a, Team **b)
{
   return (*b)->getScore() - (*a)->getScore();  
}


void GameUserInterface::renderBasicInterfaceOverlay(const GameType *gameType, bool scoreboardVisible)
{
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   if(mLevelInfoDisplayTimer.getCurrent() || mMissionOverlayActive)
   {
      // Fade message out
      F32 alpha = 1;
      if(mLevelInfoDisplayTimer.getCurrent() < 1000 && !mMissionOverlayActive)
         alpha = mLevelInfoDisplayTimer.getCurrent() * 0.001f;

      glEnableBlend;
         glColor4f(1, 1, 1, alpha);
         UserInterface::drawCenteredStringf(canvasHeight / 2 - 180, 30, "Level: %s", gameType->getLevelName()->getString());
         UserInterface::drawCenteredStringf(canvasHeight / 2 - 140, 30, "Game Type: %s", gameType->getGameTypeString());
         glColor4f(0, 1, 1, alpha);
         UserInterface::drawCenteredString(canvasHeight / 2 - 100, 20, gameType->getInstructionString());
         glColor4f(1, 0, 1, alpha);
         UserInterface::drawCenteredString(canvasHeight / 2 - 75, 20, gameType->getLevelDescription()->getString());

         glColor4f(0, 1, 0, alpha);
         UserInterface::drawCenteredStringf(canvasHeight - 100, 20, "Press [%s] to see this information again", keyCodeToString(keyMISSION));

         if(gameType->getLevelCredits()->isNotNull())    // Only render credits string if it's is not empty
         {
            glColor4f(1, 0, 0, alpha);
            UserInterface::drawCenteredStringf(canvasHeight / 2 + 50, 20, "%s", gameType->getLevelCredits()->getString());
         }

         glColor4f(1, 1, 0, alpha);
         UserInterface::drawCenteredStringf(canvasHeight / 2 - 50, 20, "Score to Win: %d", gameType->getWinningScore());

      glDisableBlend;

      mInputModeChangeAlertDisplayTimer.reset(0);     // Supress mode change alert if this message is displayed...
   }

   if(mInputModeChangeAlertDisplayTimer.getCurrent() != 0)
   {
      // Display alert about input mode changing
      F32 alpha = 1;
      if(mInputModeChangeAlertDisplayTimer.getCurrent() < 1000)
         alpha = mInputModeChangeAlertDisplayTimer.getCurrent() * 0.001f;

      glEnableBlend;
      glColor4f(1, 0.5 , 0.5, alpha);
      UserInterface::drawCenteredStringf(UserInterface::vertMargin + 130, 20, "Input mode changed to %s", 
                                         gIniSettings.inputMode == Joystick ? "Joystick" : "Keyboard");
      glDisableBlend;
   }

   Game *game = gameType->getGame();
   S32 teamCount = game->getTeamCount();

   if((gameType->isGameOver() || scoreboardVisible) && teamCount > 0)      // Render scoreboard
      renderScoreboard(gameType);

   else if(teamCount > 1 && gameType->isTeamGame())  // Render team scores in lower-right corner when scoreboard is off
   {
      S32 lroff = gameType->getLowerRightCornerScoreboardOffsetFromBottom();

      // Build a list of teams, so we can sort by score
      Vector<Team *> teams;
      teams.reserve(teamCount);

      for(S32 i = 0; i < teamCount; i++)
      {
         teams.push_back((Team *)gameType->getGame()->getTeam(i));
         teams[i]->mId = i;
      }

      teams.sort(teamScoreSort);    

      S32 maxScore = gameType->getLeadingScore();

      const S32 textsize = 32;
      S32 xpos = gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin - gameType->getDigitsNeededToDisplayScore() * 
                                                                                 UserInterface::getStringWidth(F32(textsize), "0");

      for(S32 i = 0; i < teams.size(); i++)
      {
         S32 ypos = gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin - lroff - (teams.size() - i - 1) * 38;

         Team *team = (Team *)game->getTeam(i);
         glColor3f(1,0,1);
         if( gameType->teamHasFlag(team->getId()) )
            UserInterface::drawString(xpos - 50, ypos + 3, 18, "*");

         renderFlag(F32(xpos - 20), F32(ypos + 18), team->getColor());
         glColor3f(1,1,1);
         UserInterface::drawStringf(xpos, ypos, textsize, "%d", team->getScore());
      }
   }

   //else if(mGame->getTeamCount() > 0 && !isTeamGame())   // Render leaderboard for non-team games
   //{
   //   S32 lroff = getLowerRightCornerScoreboardOffsetFromBottom();

   //   // Build a list of teams, so we can sort by score
   //   Vector<RefPtr<ClientRef> > leaderboardList;

   //   // Add you to the leaderboard
   //   if(mLocalClient)
   //   {
   //      leaderboardList.push_back(mLocalClient);
   //      logprintf("Score = %d", mLocalClient->getScore());
   //   }

   //   // Get leading player
   //   ClientRef *winningClient = mClientList[0];
   //   for(S32 i = 1; i < mClientList.size(); i++)
   //   {
   //      if(mClientList[i]->getScore() > winningClient->getScore())
   //      {
   //         winningClient = mClientList[i];
   //      }
   //   }

   //   // Add leader to the leaderboard
   //   leaderboardList.push_back(winningClient);

   //   const S32 textsize = 20;

   //   for(S32 i = 0; i < leaderboardList.size(); i++)
   //   {
   //      const char* prefix = "";
   //      if(i == leaderboardList.size() - 1)
   //      {
   //         prefix = "Leader:";
   //      }
   //      const char* name = leaderboardList[i]->name.getString();
   //      S32 score = leaderboardList[i]->getScore();

   //      S32 xpos = gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin - 
   //                 UserInterface::getStringWidthf(textsize, "%s %s %d", prefix, name, score);
   //      S32 ypos = gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin - lroff - i * 24;

   //      glColor3f(1, 1, 1);
   //      UserInterface::drawStringf(xpos, ypos, textsize, "%s %s %d", prefix, name, score);
   //   }
   //}

   renderTimeLeft(gameType);
   renderTalkingClients(gameType);
   renderDebugStatus(gameType);
}



void GameUserInterface::renderTimeLeft(const GameType *gameType)
{
   const S32 size = 20;       // Size of time
   const S32 gtsize = 12;     // Size of game type/score indicator
   
   S32 len = UserInterface::getStringWidthf(gtsize, "[%s/%d]", gameType->getShortName(), gameType->getWinningScore());

   glColor3f(0,1,1);
   UserInterface::drawStringf(gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin - 65 - len - 5,
                              gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin - 20 + ((size - gtsize) / 2) + 2, 
                              gtsize, "[%s/%d]", gameType->getShortName(), gameType->getWinningScore());

   S32 x = gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin - 65;
   S32 y = gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin - 20;
   glColor3f(1,1,1);

   if(gameType->getTotalGameTime() == 0)
      UserInterface::drawString(x, y, size, "Unlim.");
   else
   {
      U32 timeLeft = gameType->getRemainingGameTime();      // Time remaining in game
      U32 minsRemaining = timeLeft / 60;
      U32 secsRemaining = timeLeft - (minsRemaining * 60);

      UserInterface::drawStringf(x, y, size, "%02d:%02d", minsRemaining, secsRemaining);
   }
}


void GameUserInterface::renderTalkingClients(const GameType *gameType)
{
   S32 y = 150;
   for(S32 i = 0; i < gameType->getClientCount(); i++)
   {
      if(gameType->getClient(i)->voiceSFX->isPlaying())
      {
         glColor( gameType->getGame()->getTeamColor(gameType->getClient(i)->getTeam()) );
         UserInterface::drawString(10, y, 20, gameType->getClient(i)->name.getString());
         y += 25;
      }
   }
}


void GameUserInterface::renderDebugStatus(const GameType *gameType)
{
   // When bots are frozen, render large pause icon in lower left
   if(Robot::isPaused())
   {
      glColor3f(1,1,1);

      const S32 PAUSE_HEIGHT = 40;
      const S32 PAUSE_WIDTH = 15;
      const S32 PAUSE_GAP = 8;
      const S32 BOX_INSET = 5;
      const S32 BOX_THICKNESS = 4;
      const S32 BOX_HEIGHT = PAUSE_HEIGHT + 2 * PAUSE_GAP + BOX_THICKNESS;
      const S32 BOX_WIDTH = 280;
      const S32 TEXT_SIZE = 20;

      S32 x, y;

      // Draw box
      x = UserInterface::vertMargin + BOX_THICKNESS / 2 - 3;
      y = gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin;

      for(S32 i = 1; i >= 0; i--)
      {
         glColor(i ? Colors::black : Colors::white);
         glBegin(i ? GL_POLYGON: GL_LINE_LOOP); 
            glVertex2i(x,             y);
            glVertex2i(x + BOX_WIDTH, y);
            glVertex2i(x + BOX_WIDTH, y - BOX_HEIGHT);
            glVertex2i(x,             y - BOX_HEIGHT);
         glEnd();
      }


      // Draw Pause symbol
      x = UserInterface::vertMargin + BOX_THICKNESS + BOX_INSET;
      y = gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin - BOX_THICKNESS - BOX_INSET;

      for(S32 i = 0; i < 2; i++)
      {
         glBegin(GL_POLYGON);    // Filled rectangle
            glVertex2i(x,               y);
            glVertex2i(x + PAUSE_WIDTH, y);
            glVertex2i(x + PAUSE_WIDTH, y - PAUSE_HEIGHT);
            glVertex2i(x,               y - PAUSE_HEIGHT);
         glEnd();

         x += PAUSE_WIDTH + PAUSE_GAP;
      }

      x += BOX_INSET;
      y -= (TEXT_SIZE + BOX_INSET + BOX_THICKNESS + 3);
      UserInterface::drawString(x, y, TEXT_SIZE, "STEP: Alt-], Ctrl-]");
   }
}



};

