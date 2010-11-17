////////////////////////
//To do prior to 1.0 release
//
////////////////////////
// Nitnoid
// Make color defs below constant... need to change associated externs too!

// Some time
// Add mouse coords to diagnostics screen, raw key codes

// Long term
// Admin select level w/ preview(?)

//Test:
// Do ships remember their spawn points?  How about robots?
// Does chat now work properly when ship is dead?  no
// Do LuaShip proxies remain constant over time (i.e. does 013 fix for Buvet.bot now work?)
// Make sure things work when ship is deleted.  Do we get nils back (and not crashes)?


// Ideas for interactive help/tutorial:
//=======================================
// Mine: Explodes when ships fly too close
// Beware: Enemy mines are hard to see!        (plant your own w/ the mine layer weapon)
// Teleport: takes you to another location on the map
// Friendly FF: Lets friendly ships pass
// Enemy FF: Lets enemy ships pass - destroy by shooting the base
// Neutral FF: Claim it for your team by repairing with the repair module
// Friendly Turret: Targets enemies, but won't hurt you (on purpose)
// Enemy Turret: Defends enemy teritory.  Destroy with multiple shots
// Neutral turret: Claim it for your team by repairing with the repair module
// Timer shows time left in game
// Heatlh indicator shows health left
// basic controls:  x/x/x/x to move; use 1,2,3 to select weapons; <c> shows overview map
// Messages will appear here -->
// See current game info by pressing [F2]


     


// Random point in zone, random zone, isInCaptureZone should return actual capture zone
// backport player count stuff

/*
XXX need to document timers, new luavec stuff XXX

/shutdown enhancements: on screen timer after msg dismissed, instant dismissal of local notice, notice in join menu, shutdown after level, auto shutdown when quitting and players connected

*/

/* Fixes for 013f
<h4>Enhancements</h4>
<li>Better user message when server is full</li>

</ul>


014
Soccer revamp -- can pickup, carry, and pass ball

<h4>User Interface</h4>
<li>More menuing overhaul -- nicer colors, and much better design internally to support multiple menu item types on a single menu</li>

<h4>Verified accounts</h4>
<ul>User names now verified against forums user list -- if you want to play with a name from the forums, you need to supply the
forums password for that account</ul>

*/

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

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#include "IniFile.h"

#include "tnl.h"
#include "tnlRandom.h"
#include "tnlGhostConnection.h"
#include "tnlNetInterface.h"
#include "tnlJournal.h"

#include "oglconsole.h"

#ifdef TNL_OS_MAC_OSX
#include "Directory.h"
#endif

#include "zapjournal.h"

#include "../glut/glutInclude.h"
#include <stdarg.h>
#include <stdio.h>      // For logging to console

using namespace TNL;

#include "UI.h"
#include "UIGame.h"
#include "UINameEntry.h"
#include "UIMenus.h"
#include "UIEditor.h"
#include "UIErrorMessage.h"
#include "UIDiagnostics.h"
#include "UICredits.h"
#include "game.h"
#include "gameNetInterface.h"
#include "masterConnection.h"
#include "sfx.h"
#include "sparkManager.h"
#include "input.h"
#include "keyCode.h"
#include "config.h"
#include "md5wrapper.h"

#include "screenShooter.h"


#ifdef TNL_OS_MAC_OSX
#include <unistd.h>
#endif

namespace Zap
{

string gHostName;                // Server name used when hosting a game (default set in config.h, set in INI or on cmd line)
string gHostDescr;               // Brief description of host
const char *gWindowTitle = "Bitfighter";

// The following things can be set via command line parameters
S32 gMaxPlayers;                 // Max players allowed -- can change on cmd line, or INI.  Default value in config.h
U32 gSimulatedLag;               // Simulate a slow network -- can change on cmd line
F32 gSimulatedPacketLoss;        // Simulate a bad network -- can change on cmd line


#ifdef ZAP_DEDICATED
bool gDedicatedServer = true;    // This will allow us to omit the -dedicated parameter when compiled in dedicated mode
#else
bool gDedicatedServer = false;   // Usually, we just want to play.  If true, we'll be in server-only, no-player mode
#endif

bool gQuit = false;
bool gIsServer = false;

// Handle any md5 requests
md5wrapper md5;


bool gShowAimVector = false;     // Do we render an aim vector?  This should probably not be a global, but until we find a better place for it...
bool gDisableShipKeyboardInput;  // Disable ship movement while user is in menus

U32 gUseStickNumber = 1;         // Which joystick do you want to use (1 = first, which is typical)
U32 gSticksFound = 0;            // Which joystick we're actually using...

CIniFile gINI("dummy");          // This is our INI file.  Filename set down in main(), but compiler seems to want an arg here.

CmdLineSettings gCmdLineSettings;

ConfigDirectories gConfigDirs;

IniSettings gIniSettings;

ControllerTypeType gAutoDetectedJoystickType;   // Remember what sort of joystick was found for diagnostic purposes

OGLCONSOLE_Console gConsole;     // For the moment, we'll just have one console for levelgens and bots.  This may change later.


// Some colors -- other candidates include global and local chat colors, which are defined elsewhere.  Include here?
Color gNexusOpenColor(0, 0.7, 0);
Color gNexusClosedColor(0.85, 0.3, 0);
Color gErrorMessageTextColor(1, 0.5, 0.5);
Color gNeutralTeamColor(0.8, 0.8, 0.8);         // Objects that are neutral (on team -1)
Color gHostileTeamColor(0.5, 0.5, 0.5);         // Objects that are "hostile-to-all" (on team -2)
Color gMasterServerBlue(0.8, 0.8, 1);           // Messages about successful master server statii
Color gHelpTextColor(0, 1, 0);
Color WALL_OUTLINE_COLOR(0, 0, 1);
Color GAME_WALL_FILL_COLOR(0, 0, 0.15);         // Walls filled with this in game
Color EDITOR_WALL_FILL_COLOR(.5, .5, 1);        // Walls filled with this in editor

S32 gMaxPolygonPoints = 32;                     // Max number of points we can have in Nexuses, LoadoutZones, etc.

static const F32 MIN_SCALING_FACT = 0.15;       // Limits minimum window size
string gServerPassword = "";
string gAdminPassword = "";
string gLevelChangePassword = "";

Address gMasterAddress;
Address gBindAddress(IPProtocol, Address::Any, 28000);      // Good for now, may be overwritten by INI or cmd line setting
      // Above is equivalent to ("IP:Any:28000")

//Vector<StringTableEntry> gLevelList;      // Levels we'll play when we're hosting
Vector<StringTableEntry> gLevelSkipList;  // Levels we'll never load, to create a semi-delete function for remote server mgt

// Lower = more slippery!  Not used at the moment...
F32 gNormalFriction = 1000;   // Friction between vehicle and ground, ordinary
F32 gSlipFriction = 400;      // Friction, on a slip square

char gJoystickName[gJoystickNameLength] = "";

S32 gPhysicalScreenWidth;
S32 gPhysicalScreenHeight;

extern NameEntryUserInterface gNameEntryUserInterface;


// Since GLUT reports the current mouse pos via a series of events, and does not make
// its position available upon request, we'll store it when it changes so we'll have
// it when we need it.
//
// Note that when we are in the editor, we need to override the display mode because our
// canvas dimensions are irregular.
void setMousePos(S32 x, S32 y)
{
   DisplayMode reportedDisplayMode = gIniSettings.displayMode;

   if(UserInterface::current->getMenuID() == EditorUI && reportedDisplayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
      reportedDisplayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;

   gScreenInfo.setMousePos(x, y, reportedDisplayMode);
}


Screenshooter gScreenshooter;    // For taking screen shots

ZapJournal gZapJournal;          // Our main journaling object

/////
// TODO: Put these into some sort of struct?
string gPlayerName, gPlayerPassword;
Nonce gClientId;
bool gPlayerAuthenticated;       // False unless master server approves name
/////

// Handler called by GLUT when window is reshaped
void GLUT_CB_reshape(int nw, int nh)
{
   gZapJournal.reshape(nw, nh);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, reshape, (S32 newWidth, S32 newHeight), (newWidth, newHeight))
{
   // If we are entering fullscreen mode, then we don't want to mess around with proportions and all that.  Just save window size and get out.
   if(gIniSettings.displayMode == DISPLAY_MODE_FULL_SCREEN_STRETCHED || gIniSettings.displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
      gScreenInfo.setWindowSize(newWidth, newHeight);

   else
   {
     S32 canvasHeight = gScreenInfo.getGameCanvasHeight();
     S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
  
     // Constrain window to correct proportions...
     if((newWidth - canvasWidth) > (newHeight - canvasHeight))      // Wider than taller  (is this right? mixing virtual and physical pixels)
        gIniSettings.winSizeFact = max((F32) newHeight / (F32)canvasHeight, MIN_SCALING_FACT);
     else
        gIniSettings.winSizeFact = max((F32) newWidth / (F32)canvasWidth, MIN_SCALING_FACT);
  
     S32 width  = (S32)(canvasWidth  * gIniSettings.winSizeFact);   // virtual * (physical/virtual) = physical
     S32 height = (S32)(canvasHeight * gIniSettings.winSizeFact);    
  
     glutReshapeWindow(width, height);
     gScreenInfo.setWindowSize(width, height);        
   }

   glViewport(0, 0, gScreenInfo.getWindowWidth(), gScreenInfo.getWindowHeight());
   OGLCONSOLE_Reshape();

   gINI.SetValueF("Settings", "WindowScalingFactor", gIniSettings.winSizeFact, true);
}

// Handler called by GLUT when mouse motion is detected
void GLUT_CB_motion(int x, int y)
{
   gZapJournal.motion(x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, motion, (S32 x, S32 y), (x, y))
{
   setMousePos(x, y);

   if(UserInterface::current)
      UserInterface::current->onMouseDragged(x, y);
}

// Handler called by GLUT when "passive" mouse motion is detected
void GLUT_CB_passivemotion(int x, int y)
{
   gZapJournal.passivemotion(x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, passivemotion, (S32 x, S32 y), (x, y))
{
   const Point *winMousePos = gScreenInfo.getWindowMousePos();

   // Glut sometimes fires spurious events.  Let's ignore those.
   if(x == winMousePos->x && y == winMousePos->y)
      return;

   // This firstTime rigamarole prevents onMouseMoved() from firing when game first starts up; if we're in full screen mode,
   // GLUT calls this callback immediately.  Thanks, GLUT!
   bool firstTime = (winMousePos->x == -1); 

   setMousePos(x, y);

   if(firstTime)
      return;

   if(UserInterface::current)
      UserInterface::current->onMouseMoved(x, y);
}


void keyDown(KeyCode keyCode, char ascii)    // Launch the onKeyDown event
{
   if(UserInterface::current)
      UserInterface::current->onKeyDown(keyCode, ascii);
}

// Sometimes we need to pretend a key was pressed, such as for those that don't
// generate events (shift/ctrl/alt, controller buttons, etc.)
void simulateKeyDown(KeyCode keyCode)
{
   setKeyState(keyCode, true);
   keyDown(keyCode, 0);
}

void keyUp(KeyCode keyCode)              // Launch the onKeyUp event
{
   if(UserInterface::current)
      UserInterface::current->onKeyUp(keyCode);
}

void simulateKeyUp(KeyCode keyCode)
{
   setKeyState(keyCode, false);
   keyUp(keyCode);
}

// GLUT handler for key-down events
void GLUT_CB_keydown(unsigned char key, S32 x, S32 y)
{
   gZapJournal.keydown(key);
}


TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, keydown, (U8 key), (key))
{
   // First check for some "universal" keys.  If keydown isn't one of those, we'll pass the key onto the keyDown handler
   // Check for ALT-ENTER --> toggles window mode/full screen
   if(key == '\r' && (glutGetModifiers() & GLUT_ACTIVE_ALT))
      gOptionsMenuUserInterface.toggleDisplayMode();
   else if(key == 17)      // GLUT reports Ctrl-Q as 17
      gScreenshooter.phase = 1;
   else
   {
      KeyCode keyCode = standardGLUTKeyToKeyCode(key);
      setKeyState(keyCode, true);
      keyDown(keyCode, keyToAscii(key, keyCode));
   }
}

#ifndef ZAP_DEDICATED

// GLUT handler for key-up events
void GLUT_CB_keyup(unsigned char key, int x, int y)
{
   gZapJournal.keyup(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, keyup, (U8 key), (key))
{
   KeyCode keyCode = standardGLUTKeyToKeyCode(key);
   setKeyState(keyCode, false);
   keyUp(keyCode);
}

// GLUT handler for mouse clicks
void GLUT_CB_mouse(int button, int state, int x, int y)
{
   gZapJournal.mouse(button, state, x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, mouse, (S32 button, S32 state, S32 x, S32 y), (button, state, x, y))
{
   setMousePos(x, y);

   if(!UserInterface::current) return;    // Bail if no current UI

   if(button == GLUT_LEFT_BUTTON)
   {
      setKeyState(MOUSE_LEFT, (state == GLUT_DOWN));

      if(state == GLUT_DOWN)
         keyDown(MOUSE_LEFT, 0);
      else // state == GLUT_UP
         keyUp(MOUSE_LEFT);
   }
   else if(button == GLUT_RIGHT_BUTTON)
   {
      setKeyState(MOUSE_RIGHT, (state == GLUT_DOWN));

      if(state == GLUT_DOWN)
         keyDown(MOUSE_RIGHT, 0);
      else // state == GLUT_UP
         keyUp(MOUSE_RIGHT);
   }
   else if(button == GLUT_MIDDLE_BUTTON)
   {
      setKeyState(MOUSE_MIDDLE, (state == GLUT_DOWN));

      if(state == GLUT_DOWN)
         keyDown(MOUSE_MIDDLE, 0);
      else // state == GLUT_UP
         keyUp(MOUSE_MIDDLE);
   }
}

// GLUT callback for special key down (special keys are things like F1-F12)
void GLUT_CB_specialkeydown(int key, int x, int y)
{
   gZapJournal.specialkeydown(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, specialkeydown, (S32 key), (key))
{
   KeyCode keyCode = specialGLUTKeyToKeyCode(key);
   setKeyState(keyCode, true);

   if(keyCode == keyDIAG && !gDiagnosticInterface.isActive())   // Turn on diagnostic overlay if not already on
   {
      UserInterface::playBoop();
      gDiagnosticInterface.activate();
   }
   else
      keyDown(keyCode, keyToAscii(key, keyCode));           // Launch onKeyDown event
}


// GLUT callback for special key up
void GLUT_CB_specialkeyup(int key, int x, int y)
{
   gZapJournal.specialkeyup(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, specialkeyup, (S32 key), (key))
{
   KeyCode keyCode = specialGLUTKeyToKeyCode(key);
   setKeyState(keyCode, false);
   keyUp(keyCode);
}
#endif

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, modifierkeydown, (U32 key), (key))
{
   if(key == 0)         // shift
   {
      setKeyState(KEY_SHIFT, true);
      keyDown(KEY_SHIFT, 0);
   }
   else if(key == 1)    // ctrl
   {
      setKeyState(KEY_CTRL, true);
      keyDown(KEY_CTRL, 0);
   }
   else if(key == 2)    // alt
   {
      setKeyState(KEY_ALT, true);
      keyDown(KEY_ALT, 0);
   }
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, modifierkeyup, (U32 key), (key))
{
   if(key == 0)         // shift
   {
      setKeyState(KEY_SHIFT, false);
      keyUp(KEY_SHIFT);
   }
   else if(key == 1)    // ctrl
   {
      setKeyState(KEY_CTRL, false);
      keyUp(KEY_CTRL);
   }
   else if(key == 2)    // alt
   {
      setKeyState(KEY_ALT, false);
      keyUp(KEY_ALT);
   }
}


void exitGame(S32 errcode)
{
   #ifdef TNL_OS_XBOX
      extern void xboxexit();
      xboxexit();
   #else
      exit(errcode);
   #endif
}


// Exit the game, back to the OS
void exitGame()
{
   exitGame(0);
}


// If we can't load any levels, here's the plan...
void abortHosting_noLevels()
{
   if(gDedicatedServer)
   {
      logprintf(LogConsumer::LogError, "No levels found in folder %s.  Cannot host a game.", gConfigDirs.levelDir.c_str());
      logprintf(LogConsumer::ServerFilter, "No levels found in folder %s.  Cannot host a game.", gConfigDirs.levelDir.c_str());
      //printf("No levels were loaded from folder %s.  Cannot host a game.", gLevelDir.c_str());      ==> Does nothing
      exitGame(1);
   }
   else
   {
      gErrorMsgUserInterface.reset();
      gErrorMsgUserInterface.setTitle("HOUSTON, WE HAVE A PROBLEM");
      gErrorMsgUserInterface.setMessage(1, "No levels were loaded.  Cannot host a game.");
      gErrorMsgUserInterface.setMessage(3, "Check the LevelDir parameter in your INI file,");
      gErrorMsgUserInterface.setMessage(4, "or your command-line parameters to make sure");
      gErrorMsgUserInterface.setMessage(5, "you have correctly specified a folder containing");
      gErrorMsgUserInterface.setMessage(6, "valid level files.");
      gErrorMsgUserInterface.setMessage(8, "Trying to load levels from folder:");
      gErrorMsgUserInterface.setMessage(9, gConfigDirs.levelDir == "" ? "<<Unresolvable>>" : gConfigDirs.levelDir.c_str());
      gErrorMsgUserInterface.activate();
   }
   delete gServerGame;
   gServerGame = NULL;

   gHostMenuUserInterface.levelLoadDisplayDisplay = false;
   gHostMenuUserInterface.levelLoadDisplayFadeTimer.clear();

   return;
}


// GCC thinks min isn't defined, VC++ thinks it is
#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#endif

// This is not a very good way of seeding the prng, but it should generate unique, if not cryptographicly secure, streams.
// We'll get 4 bytes from the time, up to 12 bytes from the name, and any left over slots will be filled with unitialized junk.
void seedRandomNumberGenerator(string name)
{
   U32 seconds = Platform::getRealMilliseconds();
   const S32 timeByteCount = 4;
   const S32 totalByteCount = 16;

   S32 nameBytes = min(name.length(), totalByteCount - timeByteCount);     // # of bytes we get from the provided name

   unsigned char buf[totalByteCount];

   // Bytes from the time
   buf[0] = U8(seconds);
   buf[1] = U8(seconds >> 8);
   buf[2] = U8(seconds >> 16);
   buf[3] = U8(seconds >> 24);

   // Bytes from the name
   for(S32 i = 0; i < nameBytes; i++)
      buf[i + timeByteCount] = name.at(i);

   Random::addEntropy(buf, totalByteCount);     // May be some uninitialized bytes at the end of the buffer, but that's ok
}


// Host a game (and maybe even play a bit, too!)
void initHostGame(Address bindAddress, Vector<string> &levelList, bool testMode)
{
   gServerGame = new ServerGame(bindAddress, gMaxPlayers, gHostName.c_str(), testMode);

   gServerGame->setReadyToConnectToMaster(true);
   seedRandomNumberGenerator(gHostName);
   gClientId.getRandom();                    // Generate a player ID

   // Don't need to build our level list when in test mode because we're only running that one level stored in editor.tmp
   if(!testMode)
   {
      logprintf(LogConsumer::ServerFilter, "----------\nBitfighter server started [%s]", getTimeStamp().c_str());
      logprintf(LogConsumer::ServerFilter, "hostname=[%s], hostdescr=[%s]", gServerGame->getHostName(), gServerGame->getHostDescr());

      logprintf(LogConsumer::ServerFilter, "Loaded %d levels:", levelList.size());
   }

   // Parse all levels, make sure they are in some sense valid, and record some critical parameters
   if(levelList.size())
   {
      gServerGame->buildLevelList(levelList);     // Take levels in gLevelList and create a set of empty levelInfo records
      gServerGame->resetLevelLoadIndex();
      gHostMenuUserInterface.levelLoadDisplayDisplay = true;
   }
   else
   {
      abortHosting_noLevels();
      return;
   }

  // Do this even if there are no levels, so hostGame error handling will be triggered
  gServerGame->hostingModePhase = ServerGame::LoadingLevels;  
}


// All levels loaded, we're ready to go
void hostGame()
{
   if(gConfigDirs.levelDir == "")      // Never did resolve a leveldir... no hosting for you!
   {
      abortHosting_noLevels();         // Not sure this would ever get called...
      return;
   }

   gServerGame->hostingModePhase = ServerGame::Hosting;

   for(S32 i = 0; i < gServerGame->getLevelNameCount(); i++)
      logprintf(LogConsumer::ServerFilter, "\t%s [%s]", gServerGame->getLevelNameFromIndex(i).getString(), 
                                                        gServerGame->getLevelFileNameFromIndex(i).c_str());

   if(gServerGame->getLevelNameCount())                  // Levels loaded --> start game!
      gServerGame->cycleLevel(ServerGame::FIRST_LEVEL);  // Start with the first level

   else        // No levels loaded... we'll crash if we try to start a game
   {
      abortHosting_noLevels();
      return;
   }

   gHostMenuUserInterface.levelLoadDisplayDisplay = false;
   gHostMenuUserInterface.levelLoadDisplayFadeTimer.reset();

   if(!gDedicatedServer)                  // If this isn't a dedicated server...
      joinGame(Address(), false, true);   // ...then we'll play, too!
}


// This is the master idle loop that gets registered with GLUT and is called on every game tick.
// This in turn calls the idle functions for all other objects in the game.
void idle()
{
   if(gServerGame)
   {
      if(gServerGame->hostingModePhase == ServerGame::LoadingLevels)
         gServerGame->loadNextLevel();
      else if(gServerGame->hostingModePhase == ServerGame::DoneLoadingLevels)
         hostGame();
   }

   checkModifierKeyState();      // Most keys are handled as events by GLUT...  but not Ctrl, Alt, Shift!
   static S64 lastTimer = Platform::getHighPrecisionTimerValue();
   static F64 unusedFraction = 0;

   S64 currentTimer = Platform::getHighPrecisionTimerValue();

   F64 timeElapsed = Platform::getHighPrecisionMilliseconds(currentTimer - lastTimer) + unusedFraction;
   U32 integerTime = U32(timeElapsed);

   if(integerTime >= 10)         // Thus max frame rate = 100 fps
   {
      lastTimer = currentTimer;
      unusedFraction = timeElapsed - integerTime;

      gZapJournal.idle(integerTime);
   }


   // So, what's with all the SDL code in here?  I looked at converting from GLUT to SDL, in order to get
   // a richer set of keyboard events.  Looks possible, but SDL appears to be missing some very handy
   // windowing code (e.g. the ability to resize or move a window) that GLUT has.  So until we find a
   // platform independent window library, we'll stick with GLUT, or maybe go to FreeGlut.
   // Note that moving to SDL will require our journaling system to be re-engineered.
   // Note too that SDL will require linking in SDL.lib and SDLMain.lib, and including the SDL.dll in the EXE folder.

   /* SDL requires an active polling loop.  We could use something like the following:
   while(SDL_PollEvent(&e))
   {
      switch(e.type)
      {
         case SDL_KEYDOWN:
            gZapJournal.keydown((S32) e.key.keysym.sym);      // Cast to S32 to ensure journaling system can cope
            break;
         case SDL_KEYUP:
            gZapJournal.keyup((S32) e.key.keysym.sym);
            break;
         case SDL_MOUSEMOTION:
            break;
         case SDL_VIDEORESIZE:
            window_resized(e.resize.w, e.resize.h);
            break;
         case SDL_QUIT:       // User closed game window
            exitGame();
            break;
      }
   }

   gZapJournal.display();    // Draw the screen --> GLUT handles this via callback, with SDL we need to do it in our main loop
   END SDL event polling */


   // Sleep a bit so we don't saturate the system. For a non-dedicated server,
   // sleep(0) helps reduce the impact of OpenGL on windows.
   U32 sleepTime =  1; //integerTime< 8 ? 8-integerTime : 1;

   if(gClientGame && integerTime >= 10) 
      sleepTime = 0;      // Live player at the console, but if we're running > 100 fps, we can affort a nap
     
   // If there are no players, set sleepTime to 40 to further reduce impact on the server.
   // We'll only go into this longer sleep on dedicated servers when there are no players.
   if(gDedicatedServer && gServerGame->isSuspended())
      sleepTime = 40;

   Platform::sleep(sleepTime);

   gZapJournal.processNextJournalEntry();    // Does nothing unless we're playing back a journal...

}  // end idle()



TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, idle, (U32 integerTime), (integerTime))
{
   if(UserInterface::current)
      UserInterface::current->idle(integerTime);

   if(!(gServerGame && gServerGame->hostingModePhase == ServerGame::LoadingLevels))    // Don't idle games during level load
   {
      if(gClientGame)
         gClientGame->idle(integerTime);
      if(gServerGame)
         gServerGame->idle(integerTime);
   }

   if(gClientGame)
      glutPostRedisplay();
}

void dedicatedServerLoop()
{
   for(;;)        // Loop forever!
      idle();     // Idly!
}


#ifndef ZAP_DEDICATED
void GLUT_CB_display(void)
{
   gZapJournal.display();

   if(gScreenshooter.phase)      // We're in mid-shot, so be sure to visit the screenshooter!
      gScreenshooter.saveScreenshot();
}
#endif

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, display, (), ())
{
   glFlush();
   UserInterface::renderCurrent();

   // Render master connection state if we're not connected
   if(gClientGame && gClientGame->getConnectionToMaster() && 
      gClientGame->getConnectionToMaster()->getConnectionState() != NetConnection::Connected)
   {
      glColor3f(1, 1, 1);
      UserInterface::drawStringf(10, 550, 15, "Master Server - %s", gConnectStatesTable[gClientGame->getConnectionToMaster()->getConnectionState()]);
   }

   // Swap the buffers. This this tells the driver to render the next frame from the contents of the
   // back-buffer, and to set all rendering operations to occur on what was the front-buffer.
   // Double buffering prevents nasty visual tearing from the application drawing on areas of the
   // screen that are being updated at the same time.
   glutSwapBuffers();
   //SDL_GL_SwapBuffers();  // Use this if we convert to SDL
 }


string joindir(const string &path, const string &filename)
{
   // If there is no path, there's nothing to join -- just return filename
   if(path == "")
      return filename;

   // Does path already have a trailing delimieter?  If so, we'll use that.
   if(path[path.length() - 1] == '\\' || path[path.length() - 1] == '/')
      return path + filename;

   // Otherwise, join with a delimeter.  This works on Win, OS X, and Linux.
   return path + "/" + filename;
}

////////////////////////////////////////
////////////////////////////////////////

// Our logfiles
StdoutLogConsumer gStdoutLog;     // Logs to console, when there is one

FileLogConsumer gMainLog;
FileLogConsumer gServerLog;       // We'll apply a filter later on, in main()

////////////////////////////////////////
////////////////////////////////////////


// Player has selected a game from the QueryServersUserInterface, and is ready to join
void joinGame(Address remoteAddress, bool isFromMaster, bool local)
{
   if(isFromMaster && gClientGame->getConnectionToMaster())     // Request an arranged connection
   {
      gClientGame->getConnectionToMaster()->requestArrangedConnection(remoteAddress);
      gGameUserInterface.activate();
   }
   else                                                         // Try a direct connection
   {
      GameConnection *theConnection = new GameConnection();
      gClientGame->setConnectionToServer(theConnection);


      // Configure our new connection
      theConnection->setClientNameAndId(gPlayerName, gClientId);
      theConnection->setAuthenticated(gPlayerAuthenticated);
      theConnection->setSimulatedNetParams(gSimulatedPacketLoss, gSimulatedLag);

      gClientGame->setConnectionToServer(theConnection);

      if(local)   // We're a local client, running in the same process as the server... connect to that server
      {
         // Stuff on client side, so interface will offer the correct options
         theConnection->connectLocal(gClientGame->getNetInterface(), gServerGame->getNetInterface());
         theConnection->setIsAdmin(true);          // Local connection is always admin
         theConnection->setIsLevelChanger(true);   // Local connection can always change levels

         GameConnection *gc = dynamic_cast<GameConnection *>(theConnection->getRemoteConnectionObject());

         // Stuff on server side
         if(gc)                              
         {
            gc->setIsAdmin(true);            // Set isAdmin on server
            gc->setIsLevelChanger(true);     // Set isLevelChanger on server

            gc->s2cSetIsAdmin(true);                // Set isAdmin on the client
            gc->s2cSetIsLevelChanger(true, false);  // Set isLevelChanger on the client
            gc->setServerName(gServerGame->getHostName());     // Server name is whatever we've set locally

            gc->setAuthenticated(gPlayerAuthenticated);        // Tell the local host whether we're authenticated... no need to verify
         }
      }
      else        // Connect to a remote server, but not via the master server
         theConnection->connect(gClientGame->getNetInterface(), remoteAddress);  

      gGameUserInterface.activate();
   }
}

// Disconnect from servers and exit game in an orderly fashion.  But stay connected to the master until we exit the program altogether
void endGame()
{
   // Cancel any in-progress attempts to connect
   if(gClientGame && gClientGame->getConnectionToMaster())
      gClientGame->getConnectionToMaster()->cancelArrangedConnectionAttempt();

   // Disconnect from game server
   if(gClientGame && gClientGame->getConnectionToServer())
      gClientGame->getConnectionToServer()->disconnect(NetConnection::ReasonSelfDisconnect, "");

   delete gServerGame;
   gServerGame = NULL;

   gHostMenuUserInterface.levelLoadDisplayDisplay = false;

   if(gDedicatedServer)
      exitGame();
}


extern void saveWindowMode();
extern void saveWindowPosition(S32 x, S32 y);

// Run when we're quitting the game, returning to the OS
void onExit()
{
   endGame();

   delete gClientGame;     // Has effect of disconnecting from master

   OGLCONSOLE_Quit();
   SFXObject::shutdown();
   ShutdownJoystick();

   // Save settings to capture window position
   saveWindowMode();

   if(gIniSettings.displayMode == DISPLAY_MODE_WINDOWED)
      saveWindowPosition(glutGet(GLUT_WINDOW_X), glutGet(GLUT_WINDOW_Y));

   gINI.WriteFile();

   NetClassRep::logBitUsage();
   TNL::logprintf("Bye!");

   exitGame();
}


// If we're running in dedicated mode, these things need to be set as such.
void setParamsForDedicatedMode()
{
   gCmdLineSettings.clientMode = false;
   gCmdLineSettings.serverMode = true;
   gDedicatedServer = true;
}



// Read the command line params... if we're replaying a journal, we'll process those params as if they were actually there, while
// ignoring those params that were provided.
TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, readCmdLineParams, (Vector<StringPtr> argv), (argv))
{
   S32 argc = argv.size();

   // Process command line args  --> see http://bitfighter.org/wiki/index.php?title=Command_line_parameters
   for(S32 i = 0; i < argc; i+=2)
   {
      // Assume "args" starting with "-" are actually follow-on params
      bool hasAdditionalArg = (i != argc - 1 && argv[i + 1].getString()[0] != '-');     
      bool has2AdditionalArgs = hasAdditionalArg && (i != argc - 2);

      // Specify a master server
      if(!stricmp(argv[i], "-master"))        // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.masterAddress = argv[i+1];
         else
         {
            logprintf(LogConsumer::LogError, "You must specify a master server address with -master option");
            exitGame(1);
         }
      }
      // Address to use when we're hosting
      else if(!stricmp(argv[i], "-hostaddr"))       // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.hostaddr = argv[i+1];
         else
         {
            logprintf(LogConsumer::LogError, "You must specify a host address for the host to listen on (e.g. IP:Any:28000 or IP:192.169.1.100:5500)");
            exitGame(1);
         }
      }
      // Simulate packet loss 0 (none) - 1 (total)  [I think]
      else if(!stricmp(argv[i], "-loss"))          // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.loss = atof(argv[i+1]);
         else
         {
            logprintf(LogConsumer::LogError, "You must specify a loss rate between 0 and 1 with the -loss option");
            exitGame(1);
         }
      }
      // Simulate network lag
      else if(!stricmp(argv[i], "-lag"))           // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.lag = atoi(argv[i+1]);
         else
         {
            logprintf(LogConsumer::LogError, "You must specify a lag (in ms) with the -lag option");
            exitGame(1);
         }
      }

      else if(!stricmp(argv[i], "-forceUpdate"))
      {
         gCmdLineSettings.forceUpdate = true;
         i--;
      }

      // Run as a dedicated server
      else if(!stricmp(argv[i], "-dedicated"))     // additional arg optional
      {
         setParamsForDedicatedMode();

         if(hasAdditionalArg)
            gCmdLineSettings.dedicated = argv[i+1];
         else
            i--;     // Correct for the fact that we don't really have two args here...
      }
      // Specify user name
      else if(!stricmp(argv[i], "-name"))          // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.name = argv[i+1];
         else
         {
            logprintf(LogConsumer::LogError, "You must enter a nickname with the -name option");
            exitGame(1);
         }
      }
      // Specify user password
      else if(!stricmp(argv[i], "-password"))          // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.password = argv[i+1];
         else
         {
            logprintf(LogConsumer::LogError, "You must enter a password with the -password option");
            exitGame(1);
         }
      }




      // Specify password for accessing locked servers
      else if(!stricmp(argv[i], "-serverpassword"))      // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.serverPassword = argv[i+1];
         else
         {
            logprintf(LogConsumer::LogError, "You must enter a password with the -serverpassword option");
            exitGame(1);
         }
      }
      // Specify admin password for server
      else if(!stricmp(argv[i], "-adminpassword")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.adminPassword = argv[i+1];
         else
         {
            logprintf(LogConsumer::LogError, "You must specify an admin password with the -adminpassword option");
            exitGame(1);
         }
      }
      // Specify level change password for server
      else if(!stricmp(argv[i], "-levelchangepassword")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.levelChangePassword = argv[i+1];
         else
         {
            logprintf(LogConsumer::LogError, "You must specify an level-change password with the -levelchangepassword option");
            exitGame(1);
         }
      }

      else if(!stricmp(argv[i], "-rootdatadir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf(LogConsumer::LogError, "You must specify the root data folder with the -rootdatadir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.rootDataDir = argv[i+1].getString();
      }


      else if(!stricmp(argv[i], "-leveldir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf(LogConsumer::LogError, "You must specify a levels subfolder with the -leveldir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.levelDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-inidir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf(LogConsumer::LogError, "You must specify a the folder where your INI file is stored with the -inidir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.iniDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-logdir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf(LogConsumer::LogError, "You must specify your log folder with the -logdir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.logDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-scriptsdir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf(LogConsumer::LogError, "You must specify the folder where your Lua scripts are stored with the -scriptsdir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.luaDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-robotdir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf(LogConsumer::LogError, "You must specify the robots folder with the -robotdir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.robotDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-screenshotdir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf(LogConsumer::LogError, "You must specify your screenshots folder with the -screenshotdir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.screenshotDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-sfxdir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf(LogConsumer::LogError, "You must specify your sounds folder with the -sfxdir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.sfxDir = argv[i+1].getString();
      }

      // Specify list of levels...  all remaining params will be taken as level names
      else if(!stricmp(argv[i], "-levels"))     // additional arg(s) required
      {
         if(!hasAdditionalArg)
         {
            logprintf(LogConsumer::LogError, "You must specify one or more levels to load with the -levels option");
            exitGame(1);
         }

         // We'll overwrite our main level list directly, so if we're writing the INI for the first time,
         // we'll use the cmd line args to generate the INI Level keys, rather than the built-in defaults.
         for(S32 j = i+1; j < argc; j++)
            gCmdLineSettings.specifiedLevels.push_back(argv[j].getString());

         return;     // This param must be last, so no more args to process.  We can return.

      }
      // Specify name of the server as others will see it from the Join menu
      else if(!stricmp(argv[i], "-hostname"))   // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.hostname = argv[i+1];
         else
         {
            logprintf(LogConsumer::LogError, "You must specify a server name with the -hostname option");
            exitGame(1);
         }
      }
      else if(!stricmp(argv[i], "-hostdescr"))   // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.hostdescr = argv[i+1];
         else
         {
            logprintf(LogConsumer::LogError, "You must specify a description (use quotes) with the -hostdescr option");
            exitGame(1);
         }
      }
      // Change max players on server
      else if(!stricmp(argv[i], "-maxplayers")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.maxplayers = atoi(argv[i+1]);
         else
         {
            logprintf(LogConsumer::LogError, "You must specify the max number of players on your server with the -maxplayers option");
            exitGame(1);
         }
      }
      // Start in window mode
      else if(!stricmp(argv[i], "-window"))     // no additional args
      {
         i--;  // compentsate for +=2 in for loop with single param
         gCmdLineSettings.displayMode = DISPLAY_MODE_WINDOWED;
      }
      // Start in fullscreen mode
      else if(!stricmp(argv[i], "-fullscreen")) // no additional args
      {
         i--;
         gCmdLineSettings.displayMode = DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED;
      }
      // Start in fullscreen-stretch mode
      else if(!stricmp(argv[i], "-fullscreen-stretch")) // no additional args
      {
         i--;
         gCmdLineSettings.displayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;
      }

      // Specify position of window
      else if(!stricmp(argv[i], "-winpos"))     // 2 additional args required
      {
         if(has2AdditionalArgs)
         {
            gCmdLineSettings.xpos = atoi(argv[i+1]);
            gCmdLineSettings.ypos = atoi(argv[i+2]);
            i++;  // compentsate for +=2 in for loop with single param (because we're grabbing two)
         }
         else
         {
            logprintf(LogConsumer::LogError, "You must specify the x and y position of the window with the -winpos option");
            exitGame(1);
         }
      }
      // Specify width of the game window
      else if(!stricmp(argv[i], "-winwidth")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.winWidth = atoi(argv[i+1]);
         else
         {
            logprintf(LogConsumer::LogError, "You must specify the width of the game window with the -winwidth option");
            exitGame(1);
         }
      }
      else if(!stricmp(argv[i], "-help"))       // no additional args
      {
         i--;
         logprintf("See http://bitfighter.org/wiki/index.php?title=Command_line_parameters for information");
         exitGame(0);
      }
      else if(!stricmp(argv[i], "-usestick")) // additional arg required
      {
         if(hasAdditionalArg)
            gUseStickNumber = atoi(argv[i+1]);           /////////////////////////////////////////  TODO: should be part of gCmdLineSettings
         else
         {
            logprintf(LogConsumer::LogError, "You must specify the joystick you want to use with the -usestick option");
            exitGame(1);
         }
      }
   }

// Override some settings if we're compiling ZAP_DEDICATED
#ifdef ZAP_DEDICATED
   setParamsForDedicatedMode();
#endif
}

/*
void InitSdlVideo()
{
   // Information about the current video settings.
   const SDL_VideoInfo* info = NULL;

   // Flags we will pass into SDL_SetVideoMode.
   S32 flags = 0;

   // First, initialize SDL's video subsystem.
   if (SDL_Init(SDL_INIT_VIDEO) < 0)
   {
       // Failed, exit.
       logprintf(LogFatalError, "SDL Video initialization failed: %s", SDL_GetError( ));
       exitGame();
   }

   // Let's get some video information.
   info = SDL_GetVideoInfo( );

   if( !info ) {
       // This should probably never happen.
       logprintf(LogFatalError, "SDL Video query failed: %s", SDL_GetError());
       exitGame();
   }

   // We get the bpp we will request from
   // the display. On X11, VidMode can't change
   // resolution, so this is probably being overly
   // safe. Under Win32, ChangeDisplaySettings
   // can change the bpp.

   gBPP = info->vfmt->BitsPerPixel;

   // Now, we want to setup our requested
   // window attributes for our OpenGL window.
   // We want *at least* 5 bits of red, green
   // and blue. We also want at least a 16-bit
   // depth buffer.
   //
   // The last thing we do is request a double
   // buffered window. '1' turns on double
   // buffering, '0' turns it off.
   //
   // Note that we do not use SDL_DOUBLEBUF in
   // the flags to SDL_SetVideoMode. That does
   // not affect the GL attribute state, only
   // the standard 2D blitting setup.

   SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
   SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
   SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
   SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
   SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

   // We want to request that SDL provide us with an OpenGL window, possibly in a fullscreen video mode.
   // Note the SDL_DOUBLEBUF flag is not required to enable double buffering when setting an OpenGL
   // video mode. Double buffering is enabled or disabled using the SDL_GL_DOUBLEBUFFER attribute.
   flags = SDL_OPENGL | SDL_RESIZABLE ; // | SDL_FULLSCREEN;


   if(SDL_SetVideoMode(gScreenInfo.getGameCanvasWidth(), gScreenInfo.getGameCanvasHeight(), gBPP, flags ) == 0)
   {
      // This could happen for a variety of reasons,
      // including DISPLAY not being set, the specified
      // resolution not being available, etc.

      logprintf(LogFatalError, "SDL Video mode set failed: %s", SDL_GetError());
      exitGame();
   }

   SDL_WM_SetCaption(gWindowTitle, "Icon XXX");    // TODO: Fix icon here
}
*/

// Now integrate INI settings with those from the command line and process them
void processStartupParams()
{
   // These options can only be set on cmd line
   if(!gCmdLineSettings.server.empty())
      gBindAddress.set(gCmdLineSettings.server);

   if(!gCmdLineSettings.dedicated.empty())
      gBindAddress.set(gCmdLineSettings.dedicated);

   gSimulatedPacketLoss = gCmdLineSettings.loss;
   gSimulatedLag = gCmdLineSettings.lag;

   // Enable some logging...
   gMainLog.setMsgType(LogConsumer::LogConnectionProtocol, gIniSettings.logConnectionProtocol);
   gMainLog.setMsgType(LogConsumer::LogNetConnection, gIniSettings.logNetConnection);
   gMainLog.setMsgType(LogConsumer::LogEventConnection, gIniSettings.logEventConnection);
   gMainLog.setMsgType(LogConsumer::LogGhostConnection, gIniSettings.logGhostConnection);

   gMainLog.setMsgType(LogConsumer::LogNetInterface, gIniSettings.logNetInterface);
   gMainLog.setMsgType(LogConsumer::LogPlatform, gIniSettings.logPlatform);
   gMainLog.setMsgType(LogConsumer::LogNetBase, gIniSettings.logNetBase);
   gMainLog.setMsgType(LogConsumer::LogUDP, gIniSettings.logUDP);

   gMainLog.setMsgType(LogConsumer::LogFatalError, gIniSettings.logFatalError); 
   gMainLog.setMsgType(LogConsumer::LogError, gIniSettings.logError); 
   gMainLog.setMsgType(LogConsumer::LogWarning, gIniSettings.logWarning); 
   gMainLog.setMsgType(LogConsumer::LogConnection, gIniSettings.logConnection); 
   gMainLog.setMsgType(LogConsumer::LogLevelLoaded, gIniSettings.logLevelLoaded); 
   gMainLog.setMsgType(LogConsumer::LogLuaObjectLifecycle, gIniSettings.logLuaObjectLifecycle); 
   gMainLog.setMsgType(LogConsumer::LuaLevelGenerator, gIniSettings.luaLevelGenerator); 
   gMainLog.setMsgType(LogConsumer::LuaBotMessage, gIniSettings.luaBotMessage); 
   gMainLog.setMsgType(LogConsumer::ServerFilter, gIniSettings.serverFilter); 


   // These options can come either from cmd line or INI file
   //if(gCmdLineSettings.name != "")
   //   gNameEntryUserInterface.setString(gCmdLineSettings.name);
   //else if(gIniSettings.name != "")
   //   gNameEntryUserInterface.setString(gIniSettings.name);
   //else
   //   gNameEntryUserInterface.setString(gIniSettings.lastName);


   if(gCmdLineSettings.name != "")
      gPlayerName = gCmdLineSettings.name;
   else if(gIniSettings.name != "")
      gPlayerName = gIniSettings.name;
   else
      gPlayerName = gIniSettings.lastName;

   gPlayerAuthenticated = false;
   
   if(gCmdLineSettings.password != "")
      gPlayerPassword = gCmdLineSettings.password;
   else if(gIniSettings.password != "")
      gPlayerPassword = gIniSettings.password;
   else
      gPlayerPassword = gIniSettings.lastPassword;


   // Put any saved filename into the editor file entry thingy
   gLevelNameEntryUserInterface.setString(gIniSettings.lastEditorName);


   if(gCmdLineSettings.serverPassword != "")
      gServerPassword = gCmdLineSettings.serverPassword;
   else if(gIniSettings.serverPassword != "")
      gServerPassword = gIniSettings.serverPassword;
   // else rely on gServerPassword default of ""

   if(gCmdLineSettings.adminPassword != "")
      gAdminPassword = gCmdLineSettings.adminPassword;
   else if(gIniSettings.adminPassword != "")
      gAdminPassword = gIniSettings.adminPassword;
   // else rely on gAdminPassword default of ""   i.e. no one can do admin tasks on the server

   if(gCmdLineSettings.levelChangePassword != "")
      gLevelChangePassword = gCmdLineSettings.levelChangePassword;
   else if(gIniSettings.levelChangePassword != "")
      gLevelChangePassword = gIniSettings.levelChangePassword;
   // else rely on gLevelChangePassword default of ""   i.e. no one can change levels on the server

   gConfigDirs.resolveLevelDir(); 

 
   if(gIniSettings.levelDir == "")                      // If there is nothing in the INI,
      gIniSettings.levelDir = gConfigDirs.levelDir;     // write a good default to the INI


   if(gCmdLineSettings.hostname != "")
      gHostName = gCmdLineSettings.hostname;
   else
      gHostName = gIniSettings.hostname;

   if(gCmdLineSettings.hostdescr != "")
      gHostDescr = gCmdLineSettings.hostdescr;
   else
      gHostDescr = gIniSettings.hostdescr;

   if(gCmdLineSettings.hostaddr != "")
      gBindAddress.set(gCmdLineSettings.hostaddr);
   else if(gIniSettings.hostaddr != "")
      gBindAddress.set(gIniSettings.hostaddr);
   // else stick with default defined earlier

   U32 maxplay;
   if (gCmdLineSettings.maxplayers > 0)
      maxplay = gCmdLineSettings.maxplayers;
   else
      maxplay = gIniSettings.maxplayers;
   if (maxplay < 0 || maxplay > MAX_PLAYERS)
      maxplay = MAX_PLAYERS;
   gMaxPlayers = (U32) maxplay;


   if(gCmdLineSettings.displayMode != DISPLAY_MODE_UNKNOWN)
      gIniSettings.displayMode = gCmdLineSettings.displayMode;    // Simply clobber the gINISettings copy

   if(gCmdLineSettings.xpos != -9999)
      gIniSettings.winXPos = gCmdLineSettings.xpos;
   if(gCmdLineSettings.ypos != -9999)
      gIniSettings.winYPos = gCmdLineSettings.ypos;
   if(gCmdLineSettings.winWidth > 0)
      gIniSettings.winSizeFact = max((F32) gCmdLineSettings.winWidth / (F32) gScreenInfo.getGameCanvasWidth(), MIN_SCALING_FACT);

   if(gCmdLineSettings.masterAddress != "")
      gMasterAddress.set(gCmdLineSettings.masterAddress);
   else
      gMasterAddress.set(gIniSettings.masterAddress);    // This will always have a value

   if(gCmdLineSettings.name != "")                       // We'll clobber the INI file setting.  Since this
      gIniSettings.name = gCmdLineSettings.name;         // setting is never saved, we won't mess up our INI

   if(gCmdLineSettings.password != "")                   // We'll clobber the INI file setting.  Since this
      gIniSettings.password = gCmdLineSettings.password; // setting is never saved, we won't mess up our INI


   // Note that we can be in both clientMode and serverMode (such as when we're hosting a game interactively)

   if(gCmdLineSettings.clientMode)               // Create ClientGame object
      gClientGame = new ClientGame(Address());   //   let the system figure out IP address and assign a port


   // Not immediately starting a connection...  start out with name entry or main menu
   if(!gDedicatedServer)
   {
      if(gIniSettings.name == "")
         gNameEntryUserInterface.activate();
      else
      {
         gMainMenuUserInterface.activate();

         gClientGame->setReadyToConnectToMaster(true);         // Set elsewhere if in dedicated server mode
         seedRandomNumberGenerator(gIniSettings.name);
         gClientId.getRandom();                    // Generate a player ID
      }
   }
}


void processCmdLineParams(int argc, char **argv)
{
   Vector<TNL::StringPtr> theArgv;

   // Process some command line args that need to be handled early, like journaling options
   for(S32 i = 1; i < argc; i++)
   {
      if(!stricmp(argv[i], "-rules"))            // Print current rules and exit
      {
         GameType::printRules();
         exitGame(0);
      }
      else if(!stricmp(argv[i], "-jsave"))      // Write game to journal
      {
         if(i != argc - 1)
         {
            gZapJournal.record(argv[i+1]);
            i++;
         }
      }
      else if(!stricmp(argv[i], "-jplay"))      // Replay game from journal
      {
         if(i != argc - 1)
         {
            gZapJournal.load(argv[i+1]);
            i++;
         }
      }
      else
         theArgv.push_back(argv[i]);
   }  // End processing command line args

   gZapJournal.readCmdLineParams(theArgv);   // Process normal command line params, read INI, and start up
}


void setupLogging()
{
   // Specify which events each logfile will listen for
   S32 events = LogConsumer::AllErrorTypes | LogConsumer::LogConnection | LogConsumer::LuaLevelGenerator | LogConsumer::LuaBotMessage;

   gMainLog.init(joindir(gConfigDirs.logDir, "bitfighter.log"), "w");     
   //gMainLog.setMsgTypes(events);  ==> set from INI settings     
   gMainLog.logprintf("------ Bitfighter Log File ------");

   gStdoutLog.setMsgTypes(events);   // writes to stdout

   gServerLog.init(joindir(gConfigDirs.logDir, "bitfighter_server.log"), "a");
   gServerLog.setMsgTypes(LogConsumer::AllErrorTypes | LogConsumer::ServerFilter); 
   gStdoutLog.logprintf("Welcome to Bitfighter!");
}


void setOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top)
{
   glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

   // The best understanding I can get for glOrtho is that these are the coordinates you want to appear at the four corners of the
   // physical screen. If you want a "black border" down one side of the screen, you need to make left negative, so that 0 would 
   // appear some distance in from the left edge of the physical screen.  The same applies to the other coordinates as well.
   glOrtho(left, right, bottom, top, 0, 1);   

   glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


ScreenInfo gScreenInfo;


// Actually put us in windowed or full screen mode.  Pass true the first time this is used, false subsequently.
// This has the unfortunate side-effect of triggering a mouse move event.  
void actualizeScreenMode(bool changingInterfaces, bool first = false)
{      
   if(gIniSettings.oldDisplayMode == DISPLAY_MODE_WINDOWED && !first)
   {
      gIniSettings.winXPos = glutGet(GLUT_WINDOW_X);
      gIniSettings.winYPos = glutGet(GLUT_WINDOW_Y);

      gINI.SetValueI("Settings", "WindowXPos", gIniSettings.winXPos, true);
      gINI.SetValueI("Settings", "WindowYPos", gIniSettings.winYPos, true);
   }

   if(changingInterfaces)
      UserInterface::prevUIs.last()->onPreDisplayModeChange();
   else
      UserInterface::current->onPreDisplayModeChange();

   DisplayMode displayMode = gIniSettings.displayMode;

   gScreenInfo.resetGameCanvasSize();     // Set GameCanvasSize vars back to their default values

   // When we're in the editor, let's take advantage of the entire screen unstretched
   if(UserInterface::current->getMenuID() == EditorUI && 
            (gIniSettings.displayMode == DISPLAY_MODE_FULL_SCREEN_STRETCHED || gIniSettings.displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED))
   {
      // Smaller values give bigger magnification; makes small things easier to see on full screen
      F32 magFactor = 0.85;      

      // For screens smaller than normal, we need to readjust magFactor to make sure we get the full canvas height crammed onto
      // the screen; otherwise our dock will break.  Since this mode is only used in the editor, we don't really care about
      // screen width; tall skinny screens will work just fine.
      magFactor = max(magFactor, (F32)gScreenInfo.getGameCanvasHeight() / (F32)gScreenInfo.getPhysicalScreenHeight());

      gScreenInfo.setGameCanvasSize(gScreenInfo.getPhysicalScreenWidth() * magFactor, gScreenInfo.getPhysicalScreenHeight() * magFactor);

      displayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED; 
   }

   if(displayMode == DISPLAY_MODE_FULL_SCREEN_STRETCHED) 
   {
      glDisable(GL_SCISSOR_TEST);
      setOrtho(0, gScreenInfo.getGameCanvasWidth(), gScreenInfo.getGameCanvasHeight(), 0);   

      glutFullScreen();
   }

   else if(displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED) 
   {
      glClear(GL_COLOR_BUFFER_BIT);    // Clear the screen

      S32 horizDrawMargin = gScreenInfo.getHorizDrawMargin();     // In game pixels, not
      S32 vertDrawMargin = gScreenInfo.getVertDrawMargin();       // physical screen pixels

      setOrtho(-horizDrawMargin,                                        // left
                gScreenInfo.getGameCanvasWidth() + horizDrawMargin,     // right
                gScreenInfo.getGameCanvasHeight() + vertDrawMargin,     // bottom
               -vertDrawMargin);                                        // top

      glScissor(gScreenInfo.getHorizPhysicalMargin(),    // x
                gScreenInfo.getVertPhysicalMargin(),     // y
                gScreenInfo.getDrawAreaWidth(),          // width
                gScreenInfo.getDrawAreaHeight());        // height

      glEnable(GL_SCISSOR_TEST);    // Turn on clipping to keep the margins clear

      glutFullScreen();
   }

   else        // DISPLAY_MODE_WINDOWED
   {
      setOrtho(0, gScreenInfo.getGameCanvasWidth(), gScreenInfo.getGameCanvasHeight(), 0);   
      glDisable(GL_SCISSOR_TEST);

      glutReshapeWindow((S32) ((F32)gScreenInfo.getGameCanvasWidth()  * gIniSettings.winSizeFact), 
                        (S32) ((F32)gScreenInfo.getGameCanvasHeight() * gIniSettings.winSizeFact));

      glutPositionWindow(gIniSettings.winXPos, gIniSettings.winYPos);
   }

   UserInterface::current->onDisplayModeChange();     // Notify the UI that the screen has changed mode
}


void setJoystick(ControllerTypeType jsType)
{
   // Set joystick type if we found anything other than None or Unknown
   // Otherwise, it makes more sense to remember what the user had last specified

   if (jsType != NoController && jsType != UnknownController && jsType != GenericController)
      gIniSettings.joystickType = jsType;
   // else do nothing and leave the value we read from the INI file alone

   // Set primary input to joystick if any controllers were found, even a generic one
   if(jsType == NoController || jsType == UnknownController)
      gIniSettings.inputMode = Keyboard;
   else
      gIniSettings.inputMode = Joystick;
}


#ifdef WIN32
#include <direct.h>
#include <stdlib.h>
#include "stringUtils.h"      // For itos

// This block is Windows only, so it can do all sorts of icky stuff...
void launchUpdater(string bitfighterExecutablePathAndFilename)
{
   //string executable = ExtractFilename(bitfighterExecutablePathAndFilename);

   string updaterPath = ExtractDirectory(bitfighterExecutablePathAndFilename) + "\\updater";
   string updaterFileName = updaterPath + "\\bfup.exe";

   S32 buildVersion = gCmdLineSettings.forceUpdate ? 0 : BUILD_VERSION;

   S32 result = (S32) ShellExecuteA( NULL, NULL, updaterFileName.c_str(), itos(buildVersion).c_str(), updaterPath.c_str(), SW_SHOW );

   string msg = "";

   switch(result)
   {
      case 0:
	      msg = "The operating system is out of memory or resources.";
	      break;
      case ERROR_FILE_NOT_FOUND:
	      msg = "The specified file was not found (tried " + updaterFileName + ").";
	      break;
      case ERROR_PATH_NOT_FOUND:
	      msg = "The specified path was not found (tried " + updaterFileName + ").";
	      break;
      case ERROR_BAD_FORMAT:
	      msg = "The .exe file is invalid (non-Win32 .exe or error in .exe image --> tried " + updaterFileName + ").";
	      break;
      case SE_ERR_ACCESSDENIED:
	      msg = "The operating system denied access to the specified file (tried " + updaterFileName + ").";
	      break;
      case SE_ERR_ASSOCINCOMPLETE:
	      msg = "The file name association is incomplete or invalid (tried " + updaterFileName + ").";;
	      break;
      case SE_ERR_DDEBUSY:
	      msg = "The DDE transaction could not be completed because other DDE transactions were being processed.";
	      break;
      case SE_ERR_DDEFAIL:
	      msg = "The DDE transaction failed.";
	      break;
      case SE_ERR_DDETIMEOUT:
	      msg = "The DDE transaction could not be completed because the request timed out.";
	      break;
      case SE_ERR_DLLNOTFOUND:
	      msg = "The specified DLL was not found.";
	      break;
      case SE_ERR_NOASSOC:
	      msg = "There is no application associated with the given file name extension.";
	      break;
      case SE_ERR_OOM:
	      msg = "There was not enough memory to complete the operation.";
	      break;
      case SE_ERR_SHARE:
	      msg = "A sharing violation occurred.";
	      break;
   }

   if(msg != "")
      logprintf(LogConsumer::LogError, "Could not launch updater, returned error: %s", msg.c_str());
}
#endif

};  // namespace Zap


using namespace Zap;


////////////////////////////////////////
////////////////////////////////////////
// main()
////////////////////////////////////////
////////////////////////////////////////

#ifdef TNL_OS_XBOX
int zapmain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
#ifdef TNL_OS_MAC_OSX
   // Move to the application bundle's path (RDW)
   moveToAppPath();
#endif

   processCmdLineParams(argc, argv);

   gConfigDirs.resolveDirs();    // Resolve all folders except for levels folder, resolved later

   // Before we go any further, we should get our log files in order.  Now we know where they'll be, as the 
   // only way to specify a non-standard location is via the command line, which we've now read.
   setupLogging();

   // Load the INI file
   gINI.SetPath(joindir(gConfigDirs.iniDir, "bitfighter.ini"));   
   gIniSettings.init();                      // Init struct that holds INI settings


   gZapJournal.processNextJournalEntry();    // If we're replaying a journal, this will cause the cmd line params to be read from the saved journal

   loadSettingsFromINI();                    // Read INI

   processStartupParams();                   // And merge command line params and INI settings
   Ship::computeMaxFireDelay();              // Look over weapon info and get some ranges

   if(gCmdLineSettings.serverMode)           // Only gets set when compiled as a dedicated server, or when -dedicated param is specified
   {
      Vector<string> levels = LevelListLoader::buildLevelList();
      initHostGame(gBindAddress, levels, false);     // Start hosting
   }

   SFXObject::init();  // Even dedicated server needs sound


#ifndef ZAP_DEDICATED
   if(gClientGame)     // That is, we're starting up in interactive mode, as opposed to running a dedicated server
   {
      FXManager::init();                                    // Get ready for sparks!!  C'mon baby!!
      InitJoystick();
      resetKeyStates();                                     // Reset keyboard state mapping to show no keys depressed
      gAutoDetectedJoystickType = autodetectJoystickType();
      setJoystick(gAutoDetectedJoystickType);               // Will override INI settings, so process INI first

      // This is required on Linux, has no effect on Windows
      glutInitWindowSize((S32) ((F32)gScreenInfo.getGameCanvasWidth()  * gIniSettings.winSizeFact),
                         (S32) ((F32)gScreenInfo.getGameCanvasHeight() * gIniSettings.winSizeFact));   

      glutInit(&argc, argv);

      // On OS X, glutInit changes the working directory to the app
      // bundle's resource directory.  We don't want that. (RDW)
#ifdef TNL_OS_MAC_OSX
      moveToAppPath();
#endif
      // InitSdlVideo();      // Get our main SDL rendering window all set up
      // SDL_ShowCursor(0);   // Hide cursor

      gScreenInfo.init(glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));     

      glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
      glutCreateWindow(gWindowTitle);

      // Register keyboard/mouse event handlers -- see GLUT docs for details
      glutDisplayFunc(GLUT_CB_display);        // Called when GLUT thinks display needs to be redrawn
      glutReshapeFunc(GLUT_CB_reshape);        // Handle window reshape events
      glutPassiveMotionFunc(GLUT_CB_passivemotion);  // Handle mouse motion when button is not pressed
      glutMotionFunc(GLUT_CB_motion);          // Handle mouse motion when button is pressed
      glutKeyboardFunc(GLUT_CB_keydown);       // Handle key-down events for regular keys
      glutKeyboardUpFunc(GLUT_CB_keyup);       // Handle key-up events for regular keys
      glutSpecialFunc(GLUT_CB_specialkeydown); // Handle key-down events for special keys
      glutSpecialUpFunc(GLUT_CB_specialkeyup); // Handle key-up events for special keys
      glutMouseFunc(GLUT_CB_mouse);            // Handle mouse-clicks

      glutIdleFunc(idle);                      // Register our idle function.  This will get run whenever GLUT is idling.
      glutSetCursor(GLUT_CURSOR_NONE);         // Turn off the cursor for now... we'll turn it back on later

      // Put 0,0 at the center of the screen
      glTranslatef(gScreenInfo.getGameCanvasWidth() / 2, gScreenInfo.getGameCanvasHeight() / 2, 0);     

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glLineWidth(gDefaultLineWidth);

      atexit(onExit);
      actualizeScreenMode(false, true);               // Create a display window

      gConsole = OGLCONSOLE_Create();                 // Create our console *after* the screen mode has been actualized

#ifdef WIN32
      if(gIniSettings.useUpdater)
            launchUpdater(argv[0]);                   // Spawn external updater tool to check for new version of Bitfighter -- Windows only
#endif

      glutMainLoop();         // Launch GLUT on it's merry way.  It'll call back with events and when idling.
      // dedicatedServerLoop();  //    Instead, with SDL, loop forever, running the idle command endlessly

   }
   else                       // We're running a dedicated server so...
#endif
      dedicatedServerLoop();  //    loop forever, running the idle command endlessly

   return 0;
}

