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
/* Fixes after 015a
<h2>New features</h2>
<ul>
<li>Added /leveldir admin command to change folder where levels are read.  Change affects current session only, and will not be saved in the INI.
<li>Can now specify whether soccer game permits picking up the ball or not
<li>-help cmd line option now displays meaningful help
<li>Loadout presets: define them with Ctrl-1/2/3, retrieve them with Alt-1/2/3.  They persist in the INI between sessions.  You can see your current preset definitions with the /showpresets command.
<li>Mouse wheel zooms in and out in the editor
<li>Implemented 2-tier snapping in editor -- hold space to disable grid snap, but still snap to wall corners and other items; hold shift-space to completely disable snapping
</ul>

<h2>Bot scripting</h2>
<ul>
<li>Lua added copyMoveFromObject, Lua getCurrLoadout and getReqLoadout can now be used for ships
</ul>

<h2>Bug Fixes</h2>
<ul>
<li>Fix team bitmatch suicide score
<li>Teleporter, added Delay option in levels for teleporters      <<=== what is this??
<li>Teleporting onto a loadout zone triggers loadout change, just like flying onto one
<li>Console command history now working in a sane manner
</ul>

<h2>Other changes</h2>
<li>Deprecated SoccerPickup parameter -- now stored as an option on the Specials line.  Will be completely removed in 017.  Easiest fix is to load
    an affected level into the editor and save; parameter will be properly rewritten
<li>Reduced CPU usage for overlapping asteroids
<li>Removed -jsave and -jplay cmd line options.  It's been ages since they worked, and it's unlikely they ever will
<li>Removed optional hostAddr cmd line parameter for -dedicated <hostAddr>.  Specify host address (only rarely needed) with the existing -hostAddr option
<li>Removed -server cmd line parameter; not sure it really ever worked at all
<li>Smaller text should look crisper throughout the game
<li>Tab does filename completion in editor levelname entry screen
<li>Mouse wheel will scroll through existing level names on levelname entry screen
<li>Triples consume about 20% less energy than in the 015 series
<li>Removed dedicated wall-editing mode (Ctrl-A)
<li>Loadouts no longer carry over from level to level -- now you start a new level with the default loadout
<li>When requesting admin or level change permissions, incorrect passwords are now logged in server log
</ul>
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
#include "tnlJournal.h"

#include "oglconsole.h"

#include "zapjournal.h"

#include <stdarg.h>

using namespace TNL;

#ifndef ZAP_DEDICATED
#include "UINameEntry.h"
#include "UIEditor.h"
#include "UIErrorMessage.h"
#include "ClientGame.h"
#include "Cursor.h"          // For cursor defs
#include "Joystick.h"
#include "Event.h"
#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#include "SDL/SDL_syswm.h"
#endif

#include "version.h"       // For BUILD_VERSION def
#include "Colors.h"
#include "ScreenInfo.h"
#include "stringUtils.h"
#include "BanList.h"    

#include <math.h>

#ifdef WIN32
// For writeToConsole()
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <shellapi.h>

#define USE_BFUP
#endif

#ifdef TNL_OS_MAC_OSX
#include "Directory.h"
#include <unistd.h>
#endif

#ifdef TNL_OS_LINUX
#include <X11/Xlib.h>
#endif

#if defined(TNL_OS_LINUX) && defined(ZAP_DEDICATED)
#define USE_EXCEPTION_BACKTRACE
#endif

#ifdef USE_EXCEPTION_BACKTRACE
#include <execinfo.h>
#include <signal.h>
#endif


namespace Zap
{
extern ClientGame *gClientGame1;
extern ClientGame *gClientGame2;

extern void setDefaultBlendFunction();

// Handle any md5 requests
md5wrapper md5;


bool gShowAimVector = false;     // Do we render an aim vector?  This should probably not be a global, but until we find a better place for it...

CIniFile gINI("dummy");          // This is our INI file.  Filename set down in main(), but compiler seems to want an arg here.

OGLCONSOLE_Console gConsole;     // For the moment, we'll just have one console for levelgens and bots.  This may change later.


// Some colors -- other candidates include global and local chat colors, which are defined elsewhere.  Include here?
Color gNexusOpenColor(0, 0.7, 0);
Color gNexusClosedColor(0.85, 0.3, 0);
Color gErrorMessageTextColor(Colors::paleRed);
Color gNeutralTeamColor(Colors::gray80);        // Objects that are neutral (on team -1)
Color gHostileTeamColor(Colors::gray50);        // Objects that are "hostile-to-all" (on team -2)
Color gMasterServerBlue(0.8, 0.8, 1);           // Messages about successful master server statii
Color gHelpTextColor(Colors::green);
Color EDITOR_WALL_FILL_COLOR(.5, .5, 1); 

S32 gMaxPolygonPoints = 32;                     // Max number of points we can have in Walls, Nexuses, LoadoutZones, etc.

DataConnection *dataConn = NULL;

U16 DEFAULT_GAME_PORT = 28000;

ScreenInfo gScreenInfo;

ZapJournal gZapJournal;          // Our main journaling object

S32 LOADOUT_PRESETS = 3;


void exitToOs(S32 errcode)
{
#ifdef TNL_OS_XBOX
   extern void xboxexit();
   xboxexit();
#else
   exit(errcode);
#endif
}


// Exit the game, back to the OS
void exitToOs()
{
   exitToOs(0);
}


void shutdownBitfighter();    // Forward declaration

// If we can't load any levels, here's the plan...
void abortHosting_noLevels()
{
   TNLAssert(gServerGame, "gServerGame should always exist here!");

   if(gServerGame->isDedicated())
   {
      FolderManager *folderManager = gServerGame->getSettings()->getFolderManager();
      const char *levelDir = folderManager->levelDir.c_str();

      logprintf(LogConsumer::LogError,     "No levels found in folder %s.  Cannot host a game.", levelDir);
      logprintf(LogConsumer::ServerFilter, "No levels found in folder %s.  Cannot host a game.", levelDir);
   }

   delete gServerGame;
   gServerGame = NULL;

#ifndef ZAP_DEDICATED
   if(gClientGame)
   {
      ErrorMessageUserInterface *errUI = gClientGame->getUIManager()->getErrorMsgUserInterface();
      FolderManager *folderManager = gServerGame->getSettings()->getFolderManager();
      string levelDir = folderManager->levelDir;

      errUI->reset();
      errUI->setTitle("HOUSTON, WE HAVE A PROBLEM");
      errUI->setMessage(1, "No levels were loaded.  Cannot host a game.");
      errUI->setMessage(3, "Check the LevelDir parameter in your INI file,");
      errUI->setMessage(4, "or your command-line parameters to make sure");
      errUI->setMessage(5, "you have correctly specified a folder containing");
      errUI->setMessage(6, "valid level files.");
      errUI->setMessage(8, "Trying to load levels from folder:");
      errUI->setMessage(9, levelDir == "" ? "<<Unresolvable>>" : levelDir.c_str());
      errUI->activate();

      HostMenuUserInterface *menuUI = gClientGame->getUIManager()->getHostMenuUserInterface();
      menuUI->levelLoadDisplayDisplay = false;
      menuUI->levelLoadDisplayFadeTimer.clear();
   }
   else
#endif
      shutdownBitfighter();      // Quit in an orderly fashion
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

   S32 nameBytes = min((S32)name.length(), totalByteCount - timeByteCount);     // # of bytes we get from the provided name

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


////////////////////////////////////////
////////////////////////////////////////
// Call this function when running game in console mode; causes output to be dumped to console, if it was run from one
// Loosely based on http://www.codeproject.com/KB/dialog/ConsoleAdapter.aspx
bool writeToConsole()
{

#if defined(WIN32) && (_WIN32_WINNT >= 0x0500)
   // _WIN32_WINNT is needed in case of compiling for old windows 98 (this code won't work for windows 98)
   if(!AttachConsole(-1))
      return false;

   try
   {
      int m_nCRTOut = _open_osfhandle((intptr_t) GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
      if(m_nCRTOut == -1)
         return false;

      FILE *m_fpCRTOut = _fdopen( m_nCRTOut, "w" );

      if( !m_fpCRTOut )
         return false;

      *stdout = *m_fpCRTOut;

      //// If clear is not done, any cout statement before AllocConsole will 
      //// cause, the cout after AllocConsole to fail, so this is very important
      // But here, we're not using AllocConsole...
      //std::cout.clear();
   }
   catch ( ... )
   {
      return false;
   } 
#endif    
   return true;
}


// Host a game (and maybe even play a bit, too!)
void initHostGame(GameSettings *settings, const Vector<string> &levelList, bool testMode, bool dedicatedServer)
{
   TNLAssert(!gServerGame, "already exists!");
   if(gServerGame)
   {
      delete gServerGame;
      gServerGame = NULL;
   }

   Address address(IPProtocol, Address::Any, DEFAULT_GAME_PORT);     // Equivalent to ("IP:Any:28000")
   address.set(settings->getHostAddress());                          // May overwrite parts of address, depending on what getHostAddress contains

   gServerGame = new ServerGame(address, settings, testMode, dedicatedServer);

   gServerGame->setReadyToConnectToMaster(true);
   seedRandomNumberGenerator(settings->getHostName());

   // Don't need to build our level list when in test mode because we're only running that one level stored in editor.tmp
   if(!testMode)
   {
      logprintf(LogConsumer::ServerFilter, "----------\nBitfighter server started [%s]", getTimeStamp().c_str());
      logprintf(LogConsumer::ServerFilter, "hostname=[%s], hostdescr=[%s]", gServerGame->getSettings()->getHostName().c_str(), 
                                                                            gServerGame->getSettings()->getHostDescr().c_str());

      logprintf(LogConsumer::ServerFilter, "Loaded %d levels:", levelList.size());
   }

   if(levelList.size())
   {
      gServerGame->buildBasicLevelInfoList(levelList);     // Take levels in gLevelList and create a set of empty levelInfo records
      gServerGame->resetLevelLoadIndex();

#ifndef ZAP_DEDICATED
      if(gClientGame)
         gClientGame->getUIManager()->getHostMenuUserInterface()->levelLoadDisplayDisplay = true;
#endif
   }
   else  // No levels!
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
   if(!gServerGame->startHosting())
   {
      abortHosting_noLevels();
      return;
   }

#ifndef ZAP_DEDICATED
   if(gClientGame)      // Should be true if this isn't a dedicated server...
   {
      HostMenuUserInterface *ui = gClientGame->getUIManager()->getHostMenuUserInterface();

      ui->levelLoadDisplayDisplay = false;
      ui->levelLoadDisplayFadeTimer.reset();

      gClientGame->joinGame(Address(), false, true);   // ...then we'll play, too!
   }
#endif
}


#ifndef ZAP_DEDICATED
// Draw the screen
void display()
{
   glFlush();

   gClientGame->getUIManager()->renderCurrent();

   // Swap the buffers. This this tells the driver to render the next frame from the contents of the
   // back-buffer, and to set all rendering operations to occur on what was the front-buffer.
   // Double buffering prevents nasty visual tearing from the application drawing on areas of the
   // screen that are being updated at the same time.
   SDL_GL_SwapBuffers();  // Use this if we convert to SDL
}


static SDL_SysWMinfo windowManagerInfo;

void setWindowPosition(S32 left, S32 top)
{
   SDL_VERSION(&windowManagerInfo.version);

   // No window information..  Abort!!
   if(!SDL_GetWMInfo(&windowManagerInfo))
   {
      logprintf(LogConsumer::LogError, "Failed to set window position");
      return;
   }

#ifdef TNL_OS_LINUX
   if (windowManagerInfo.subsystem == SDL_SYSWM_X11) {
       windowManagerInfo.info.x11.lock_func();
       XMoveWindow(windowManagerInfo.info.x11.display, windowManagerInfo.info.x11.wmwindow, left, top);
       windowManagerInfo.info.x11.unlock_func();
   }
#endif

#ifdef TNL_OS_MAC_OSX
   // I don't know if anything can be done here..
#endif

#ifdef TNL_OS_WIN32
   SetWindowPos(windowManagerInfo.window, HWND_TOP, left, top, 0, 0, SWP_NOSIZE);
#endif
}


S32 getWindowPositionCoord(bool getX)
{
   SDL_VERSION(&windowManagerInfo.version);

   if(!SDL_GetWMInfo(&windowManagerInfo))
      logprintf(LogConsumer::LogError, "Failed to set window position");

#ifdef TNL_OS_LINUX
   if (windowManagerInfo.subsystem == SDL_SYSWM_X11) {
      XWindowAttributes xAttributes;
      Window parent, childIgnore, rootIgnore;
      Window *childrenIgnore;
      U32 childrenCountIgnore;
      S32 x, y;

      windowManagerInfo.info.x11.lock_func();

      // Find parent window of managed window to get proper coordinates
      XQueryTree(windowManagerInfo.info.x11.display, windowManagerInfo.info.x11.wmwindow,
              &rootIgnore, &parent, &childrenIgnore, &childrenCountIgnore);

      // Get Window attributes
      XGetWindowAttributes(windowManagerInfo.info.x11.display, parent, &xAttributes);

      // Now find absolute values
      XTranslateCoordinates(windowManagerInfo.info.x11.display, parent,
            xAttributes.root, 0, 0, &x, &y, &childIgnore);

      windowManagerInfo.info.x11.unlock_func();
      return getX ? x : y;
   }
#endif

#ifdef TNL_OS_MAC_OSX
   // I don't know if anything can be done here..
#endif

#ifdef TNL_OS_WIN32
   RECT rect;
   GetWindowRect(windowManagerInfo.window, &rect);
   return getX ? rect.left : rect.top;
#endif

   // Otherwise just return 0
   return 0;
}


S32 getWindowPositionX()
{
   return getWindowPositionCoord(true);
}


S32 getWindowPositionY()
{
   return getWindowPositionCoord(false);
}
#endif


void gameIdle(U32 integerTime)
{
#ifndef ZAP_DEDICATED
   if(UserInterface::current)
      UserInterface::current->idle(integerTime);
#endif

   if(!(gServerGame && gServerGame->hostingModePhase == ServerGame::LoadingLevels))    // Don't idle games during level load
   {
#ifndef ZAP_DEDICATED
      if(gClientGame2)
      {
         gClientGame2->getSettings()->getIniSettings()->inputMode = InputModeJoystick;

         gClientGame1->mUserInterfaceData->get();
         gClientGame2->mUserInterfaceData->set();

         gClientGame = gClientGame2;
         gClientGame->idle(integerTime);

         gClientGame->getSettings()->getIniSettings()->inputMode = InputModeKeyboard;

         gClientGame2->mUserInterfaceData->get();
         gClientGame1->mUserInterfaceData->set();
      }
      if(gClientGame1)
      {
         gClientGame = gClientGame1;
         gClientGame->idle(integerTime);
      }
#endif

      if(gServerGame)
      {
         // This block has to be outside gServerGame because deleting an object from within is in rather poor form
         if(gServerGame->isReadyToShutdown(integerTime))
         {
#ifndef ZAP_DEDICATED
            if(gClientGame)
            {
               gClientGame->closeConnectionToGameServer();

               delete gServerGame;
               gServerGame = NULL;
            }
            else
#endif
               shutdownBitfighter();
         }

         else
            gServerGame->idle(integerTime);
      }
   }
}


// This is the master idle loop that gets registered with GLUT and is called on every game tick.
// This in turn calls the idle functions for all other objects in the game.
void idle()
{
   GameSettings *settings;

   if(gServerGame)
   {
      settings = gServerGame->getSettings();

      if(gServerGame->hostingModePhase == ServerGame::LoadingLevels)
         gServerGame->loadNextLevelInfo();
      else if(gServerGame->hostingModePhase == ServerGame::DoneLoadingLevels)
         hostGame();
   }
#ifndef ZAP_DEDICATED
   else
      settings = gClientGame->getSettings();
#endif

/*
   static S64 lastTimer = Platform::getHighPrecisionTimerValue(); // accurate, but possible wrong speed when overclocking or underclocking CPU
   static U32 lastTimer2 = Platform::getRealMilliseconds();  // right speed
   static F64 unusedFraction = 0;
   static S32 timerElapsed2 = 0;

   S64 currentTimer = Platform::getHighPrecisionTimerValue();
   U32 currentTimer2 = Platform::getRealMilliseconds();

   if(lastTimer > currentTimer) lastTimer=currentTimer; //Prevent freezing when currentTimer overflow -- seems very unlikely
   if(lastTimer2 > currentTimer2) lastTimer2=currentTimer2;

   F64 timeElapsed = Platform::getHighPrecisionMilliseconds(currentTimer - lastTimer) + unusedFraction;
   S32 integerTime1 = S32(timeElapsed);

   unusedFraction = timeElapsed - integerTime1;
   lastTimer = currentTimer;
   timerElapsed2 = timerElapsed2 + S32(currentTimer2 - lastTimer2) - integerTime1;
   if(timerElapsed2 < 0)  // getHighPrecisionTimerValue going slower then getRealMilliseconds
   {
      integerTime1 += timerElapsed2;
      timerElapsed2 = 0;
   }
   if(timerElapsed2 > 200)  // getHighPrecisionTimerValue going faster then getRealMilliseconds
   {
      integerTime1 += timerElapsed2 - 200;
      timerElapsed2 = 200;
   }
   lastTimer2 = currentTimer2;
   integerTime += integerTime1;
   */

   static S32 integerTime = 0;   // static, as we need to keep holding the value that was set
   static U32 prevTimer = 0;

   U32 currentTimer = Platform::getRealMilliseconds();
   integerTime += currentTimer - prevTimer;
   prevTimer = currentTimer;

   if(integerTime < -500 || integerTime > 5000)
      integerTime = 10;

   U32 sleepTime = 1;

   bool dedicated = gServerGame && gServerGame->isDedicated();


   if( ( dedicated && integerTime >= S32(1000 / settings->getIniSettings()->maxDedicatedFPS)) || 
       (!dedicated && integerTime >= S32(1000 / settings->getIniSettings()->maxFPS)) )
   {
      gameIdle(U32(integerTime));

#ifndef ZAP_DEDICATED
      if(!dedicated)
         display();          // Draw the screen if not dedicated
#endif
      integerTime = 0;

      if(!dedicated)
         sleepTime = 0;      
   }


#ifndef ZAP_DEDICATED
   // SDL requires an active polling loop.  We could use something like the following:
   SDL_Event event;

   while(SDL_PollEvent(&event))
   {
      TNLAssert(gClientGame, "Why are we here if there is no client game??");

      if(event.type == SDL_QUIT) // Handle quit here
         exitToOs();

      Event::onEvent(gClientGame, &event);
   }
   // END SDL event polling
#endif


   // Sleep a bit so we don't saturate the system. For a non-dedicated server,
   // sleep(0) helps reduce the impact of OpenGL on windows.

   // If there are no players, set sleepTime to 40 to further reduce impact on the server.
   // We'll only go into this longer sleep on dedicated servers when there are no players.
   if(dedicated && gServerGame->isSuspended())
      sleepTime = 40;     // The higher this number, the less accurate the ping is on server lobby when empty, but the less power consumed.

   Platform::sleep(sleepTime);

}  // end idle()


void dedicatedServerLoop()
{
   for(;;)        // Loop forever!
      idle();     // Idly!
}

////////////////////////////////////////
////////////////////////////////////////

// Our logfiles
StdoutLogConsumer gStdoutLog;     // Logs to console, when there is one

FileLogConsumer gMainLog;
FileLogConsumer gServerLog;       // We'll apply a filter later on, in main()

////////////////////////////////////////
////////////////////////////////////////


// Run when we're quitting the game, returning to the OS.  Saves settings and does some final cleanup to keep things orderly.
// There are currently only 6 ways to get here (i.e. 6 legitimate ways to exit Bitfighter): 
// 1) Hit escape during initial name entry screen
// 2) Hit escape from the main menu
// 3) Choose Quit from main menu
// 4) Host a game with no levels as a dedicated server
// 5) Admin issues a shutdown command to a remote dedicated server
// 6) Click the X on the window to close the game window   <=== NOTE: This scenario fails for me when running a dedicated server on windows.
void shutdownBitfighter()
{
   GameSettings *settings;

   // Avoid this function being called twice when we exit via methods 1-4 above
#ifndef ZAP_DEDICATED
   if(!gClientGame)
#endif
      if(!gServerGame)
         exitToOs();

// Grab a pointer to settings wherever we can.  Note that gClientGame and gServerGame refer to the same object.
#ifndef ZAP_DEDICATED
   if(gClientGame)
   {
      settings = gClientGame->getSettings();
      delete gClientGame;     // Destructor terminates connection to master
      gClientGame = NULL;
   }
#endif

   if(gServerGame)
   {
      settings = gServerGame->getSettings();
      delete gServerGame;     // Destructor terminates connection to master
      gServerGame = NULL;
   }

   TNLAssert(settings, "Should always have a value here!");

   SoundSystem::shutdown();

#ifndef ZAP_DEDICATED
   OGLCONSOLE_Quit();
   Joystick::shutdownJoystick();

   // Save current window position if in windowed mode
   if(settings->getIniSettings()->displayMode == DISPLAY_MODE_WINDOWED)
   {
      settings->getIniSettings()->winXPos = getWindowPositionX();
      settings->getIniSettings()->winYPos = getWindowPositionY();
   }

   SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif

   settings->save();        // Writes all our settings to bitfighter.ini

   delete settings;

   NetClassRep::logBitUsage();
   logprintf("Bye!");

   exitToOs();    // Do not pass Go
}


#ifndef ZAP_DEDICATED
void InitSdlVideo()
{
   // Information about the current video settings
   const SDL_VideoInfo* info = NULL;

   // Flags we will pass into SDL_SetVideoMode
   S32 flags = 0;

   // Init!
   SDL_Init(0);

   // First, initialize SDL's video subsystem
   if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
   {
      // Failed, exit
      logprintf(LogConsumer::LogFatalError, "SDL Video initialization failed: %s", SDL_GetError());
      exitToOs();
   }

   // Let's get some video information
   info = SDL_GetVideoInfo();

   if(!info)
   {
      // This should probably never happen
      logprintf(LogConsumer::LogFatalError, "SDL Video query failed: %s", SDL_GetError());
      exitToOs();
   }

   // Find the desktop width/height and initialize the ScreenInfo object with it
   gScreenInfo.init(info->current_w, info->current_h);

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

   static const char *WINDOW_TITLE = "Bitfighter";
   SDL_WM_SetCaption(WINDOW_TITLE, WINDOW_TITLE);  // Icon name is same as window title -- set here so window will be created with proper name

   // Set the window icon -- note that the icon must be a 32x32 bmp, and SDL will downscale it to 16x16 with no interpolation.  Therefore, 
   // it's best to start with a finely crafted 16x16 icon, then scale it up to 32x32 with no interpolation.  It will look crappy at 32x32, 
   // but good at 16x16, and that's all that really matters.
   SDL_Surface *image = SDL_LoadBMP("bficon.bmp");            // Save bmp as a 32 bit XRGB bmp file (Gimp can do it!)
   if(image != NULL)
      SDL_SetColorKey(image, SDL_SRCCOLORKEY, SDL_MapRGB(image->format, 0, 0, 0));

   SDL_WM_SetIcon(image,NULL);

   // We will run SDL_SetVideoMode in actualizeScreenMode()
}
#endif


void setupLogging(IniSettings *iniSettings)
{
   gMainLog.setMsgType(LogConsumer::LogConnectionProtocol, iniSettings->logConnectionProtocol);
   gMainLog.setMsgType(LogConsumer::LogNetConnection, iniSettings->logNetConnection);
   gMainLog.setMsgType(LogConsumer::LogEventConnection, iniSettings->logEventConnection);
   gMainLog.setMsgType(LogConsumer::LogGhostConnection, iniSettings->logGhostConnection);

   gMainLog.setMsgType(LogConsumer::LogNetInterface, iniSettings->logNetInterface);
   gMainLog.setMsgType(LogConsumer::LogPlatform, iniSettings->logPlatform);
   gMainLog.setMsgType(LogConsumer::LogNetBase, iniSettings->logNetBase);
   gMainLog.setMsgType(LogConsumer::LogUDP, iniSettings->logUDP);

   gMainLog.setMsgType(LogConsumer::LogFatalError, iniSettings->logFatalError); 
   gMainLog.setMsgType(LogConsumer::LogError, iniSettings->logError); 
   gMainLog.setMsgType(LogConsumer::LogWarning, iniSettings->logWarning); 
   gMainLog.setMsgType(LogConsumer::ConfigurationError, iniSettings->logConfigurationError);
   gMainLog.setMsgType(LogConsumer::LogConnection, iniSettings->logConnection); 
   gMainLog.setMsgType(LogConsumer::LogLevelLoaded, iniSettings->logLevelLoaded); 
   gMainLog.setMsgType(LogConsumer::LogLuaObjectLifecycle, iniSettings->logLuaObjectLifecycle); 
   gMainLog.setMsgType(LogConsumer::LuaLevelGenerator, iniSettings->luaLevelGenerator); 
   gMainLog.setMsgType(LogConsumer::LuaBotMessage, iniSettings->luaBotMessage); 
   gMainLog.setMsgType(LogConsumer::ServerFilter, iniSettings->serverFilter); 
}


void createClientGame(GameSettings *settings)
{
#ifndef ZAP_DEDICATED
   if(!settings->isDedicatedServer())                      // Create ClientGame object
   {
      gClientGame1 = new ClientGame(Address(IPProtocol, Address::Any, settings->getIniSettings()->clientPortNumber), settings);   //   Let the system figure out IP address and assign a port
      gClientGame = gClientGame1;

      gClientGame->setLoginPassword(settings->getPlayerPassword());

       // Put any saved filename into the editor file entry thingy
      gClientGame->getUIManager()->getLevelNameEntryUserInterface()->setString(settings->getIniSettings()->lastEditorName);

      seedRandomNumberGenerator(settings->getIniSettings()->lastName);
      gClientGame->getClientInfo()->getId()->getRandom();

      //gClientGame2 = new ClientGame(Address());   //  !!! 2-player split-screen game in same game.

      if(settings->shouldShowNameEntryScreenOnStartup())
      {
         if(gClientGame2)
         {
            gClientGame = gClientGame2;
            gClientGame1->mUserInterfaceData->get();
            gClientGame->getUIManager()->getNameEntryUserInterface()->activate();
            gClientGame2->mUserInterfaceData->get();
            gClientGame1->mUserInterfaceData->set();
            gClientGame = gClientGame1;
         }
         gClientGame->getUIManager()->getNameEntryUserInterface()->activate();
         seedRandomNumberGenerator(settings->getIniSettings()->lastName);
      }
      else
      {
         if(gClientGame2)
         {
            gClientGame = gClientGame2;
            gClientGame1->mUserInterfaceData->get();

            gClientGame2->mUserInterfaceData->get();
            gClientGame1->mUserInterfaceData->set();
            gClientGame = gClientGame1;
         }
         gClientGame->getUIManager()->getMainMenuUserInterface()->activate();

         gClientGame->setReadyToConnectToMaster(true);         // Set elsewhere if in dedicated server mode
         seedRandomNumberGenerator(settings->getPlayerName());
      }
   }
#endif
}


void setupLogging(const string &logDir)
{
   // Specify which events each logfile will listen for
   S32 events = LogConsumer::AllErrorTypes | LogConsumer::LogConnection | LogConsumer::LuaLevelGenerator | LogConsumer::LuaBotMessage;

   gMainLog.init(joindir(logDir, "bitfighter.log"), "w");
   //gMainLog.setMsgTypes(events);  ==> set from INI settings     
   gMainLog.logprintf("------ Bitfighter Log File ------");

   gStdoutLog.setMsgTypes(events);   // writes to stdout

   gServerLog.init(joindir(logDir, "bitfighter_server.log"), "a");
   gServerLog.setMsgTypes(LogConsumer::AllErrorTypes | LogConsumer::ServerFilter | LogConsumer::StatisticsFilter); 
   gStdoutLog.logprintf("Welcome to Bitfighter!");
}


#ifndef ZAP_DEDICATED
// Actually put us in windowed or full screen mode.  Pass true the first time this is used, false subsequently.
// This has the unfortunate side-effect of triggering a mouse move event.  
void actualizeScreenMode(bool changingInterfaces)
{
   if(changingInterfaces)
      gClientGame->getUIManager()->getPrevUI()->onPreDisplayModeChange();
   else
      UserInterface::current->onPreDisplayModeChange();

   GameSettings *settings = gClientGame->getSettings();

   DisplayMode displayMode = settings->getIniSettings()->displayMode;

   gScreenInfo.resetGameCanvasSize();     // Set GameCanvasSize vars back to their default values


   // If old display mode is windowed or current is windowed but we change interfaces,
   // save the window position
   if(settings->getIniSettings()->oldDisplayMode == DISPLAY_MODE_WINDOWED ||
         (changingInterfaces && settings->getIniSettings()->displayMode == DISPLAY_MODE_WINDOWED))
   {
      settings->getIniSettings()->winXPos = getWindowPositionX();
      settings->getIniSettings()->winYPos = getWindowPositionY();

      gINI.SetValueI("Settings", "WindowXPos", settings->getIniSettings()->winXPos, true);
      gINI.SetValueI("Settings", "WindowYPos", settings->getIniSettings()->winYPos, true);
   }


   // When we're in the editor, let's take advantage of the entire screen unstretched
   if(UserInterface::current->getMenuID() == EditorUI && 
         (settings->getIniSettings()->displayMode == DISPLAY_MODE_FULL_SCREEN_STRETCHED || 
          settings->getIniSettings()->displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED))
   {
      // Smaller values give bigger magnification; makes small things easier to see on full screen
      F32 magFactor = 0.85f;      

      // For screens smaller than normal, we need to readjust magFactor to make sure we get the full canvas height crammed onto
      // the screen; otherwise our dock will break.  Since this mode is only used in the editor, we don't really care about
      // screen width; tall skinny screens will work just fine.
      magFactor = max(magFactor, (F32)gScreenInfo.getGameCanvasHeight() / (F32)gScreenInfo.getPhysicalScreenHeight());

      gScreenInfo.setGameCanvasSize(S32(gScreenInfo.getPhysicalScreenWidth() * magFactor), S32(gScreenInfo.getPhysicalScreenHeight() * magFactor));

      displayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED; 
   }

   S32 sdlVideoFlags = 0;
   S32 sdlWindowWidth, sdlWindowHeight;
   F64 orthoLeft = 0, orthoRight = 0, orthoTop = 0, orthoBottom = 0;

   // Always use OpenGL
   sdlVideoFlags = SDL_OPENGL;

   // Set up variables according to display mode
   switch (displayMode)
   {
   case DISPLAY_MODE_FULL_SCREEN_STRETCHED:
      sdlWindowWidth = gScreenInfo.getPhysicalScreenWidth();
      sdlWindowHeight = gScreenInfo.getPhysicalScreenHeight();
      sdlVideoFlags |= settings->getIniSettings()->useFakeFullscreen ? SDL_NOFRAME : SDL_FULLSCREEN;

      orthoRight = gScreenInfo.getGameCanvasWidth();
      orthoBottom = gScreenInfo.getGameCanvasHeight();
      break;

   case DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED:
      sdlWindowWidth = gScreenInfo.getPhysicalScreenWidth();
      sdlWindowHeight = gScreenInfo.getPhysicalScreenHeight();
      sdlVideoFlags |= settings->getIniSettings()->useFakeFullscreen ? SDL_NOFRAME : SDL_FULLSCREEN;

      orthoLeft = -1 * (gScreenInfo.getHorizDrawMargin());
      orthoRight = gScreenInfo.getGameCanvasWidth() + gScreenInfo.getHorizDrawMargin();
      orthoBottom = gScreenInfo.getGameCanvasHeight() + gScreenInfo.getVertDrawMargin();
      orthoTop = -1 * (gScreenInfo.getVertDrawMargin());
      break;

   case DISPLAY_MODE_WINDOWED:
   default:  //  DISPLAY_MODE_WINDOWED
      sdlWindowWidth = (S32) floor((F32)gScreenInfo.getGameCanvasWidth()  * settings->getIniSettings()->winSizeFact + 0.5f);
      sdlWindowHeight = (S32) floor((F32)gScreenInfo.getGameCanvasHeight() * settings->getIniSettings()->winSizeFact + 0.5f);
      sdlVideoFlags |= SDL_RESIZABLE;

      orthoRight = gScreenInfo.getGameCanvasWidth();
      orthoBottom = gScreenInfo.getGameCanvasHeight();
      break;
   }

   // Set the SDL screen size and change to it
   if(SDL_SetVideoMode(sdlWindowWidth, sdlWindowHeight, 0, sdlVideoFlags) == NULL)
         logprintf(LogConsumer::LogFatalError, "Setting display mode failed: %s", SDL_GetError());

   // Now save the new window dimensions in ScreenInfo
   gScreenInfo.setWindowSize(sdlWindowWidth, sdlWindowHeight);

   glClearColor( 0, 0, 0, 0 );

   glViewport(0, 0, sdlWindowWidth, sdlWindowHeight);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   // The best understanding I can get for glOrtho is that these are the coordinates you want to appear at the four corners of the
   // physical screen. If you want a "black border" down one side of the screen, you need to make left negative, so that 0 would
   // appear some distance in from the left edge of the physical screen.  The same applies to the other coordinates as well.
   glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, 0, 1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // Do the scissoring
   if (displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
   {
      glScissor(gScreenInfo.getHorizPhysicalMargin(),    // x
                gScreenInfo.getVertPhysicalMargin(),     // y
                gScreenInfo.getDrawAreaWidth(),          // width
                gScreenInfo.getDrawAreaHeight());        // height

      glEnable(GL_SCISSOR_TEST);    // Turn on clipping to keep the margins clear
   }
   else
      glDisable(GL_SCISSOR_TEST);


   setDefaultBlendFunction();
   glLineWidth(gDefaultLineWidth);

   // Enable Line smoothing everywhere!  Make sure to disable temporarily for filled polygons and such
   glEnable(GL_LINE_SMOOTH);
   //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
   glEnable(GL_BLEND);

   // If OGLconsole has been created, recreate font texture and handle resize event
   if (gConsole)
   {
      OGLCONSOLE_CreateFont();
      OGLCONSOLE_Reshape();
   }

   // Now set the window position
   if (displayMode == DISPLAY_MODE_WINDOWED)
      setWindowPosition(settings->getIniSettings()->winXPos, settings->getIniSettings()->winYPos);
   else
      setWindowPosition(0, 0);

   UserInterface::current->onDisplayModeChange();     // Notify the UI that the screen has changed mode
}

#endif


// Function to handle one-time update tasks
// Use this when upgrading, and changing something like the name of an INI parameter.  The old version is stored in
// IniSettings.version, and the new version is in BUILD_VERSION.
void checkIfThisIsAnUpdate(GameSettings *settings)
{
   if(settings->getIniSettings()->version == BUILD_VERSION)
      return;

   // Wipe out all comments; they will be replaced with any updates
   gINI.deleteHeaderComments();
   gINI.deleteAllSectionComments();

   // version specific changes
   // 015a
//   if(settings->getIniSettings()->version < 1836)
//      settings->getIniSettings()->useLineSmoothing = true;

   // after 015a
   if(settings->getIniSettings()->version < 1840 && settings->getIniSettings()->maxBots == 127)
      settings->getIniSettings()->maxBots = 10;
   if(settings->getIniSettings()->version < 3006)
      settings->getIniSettings()->masterAddress = MASTER_SERVER_LIST_ADDRESS;
}


#ifdef USE_BFUP
#include <direct.h>
#include <stdlib.h>

// This block is Windows only, so it can do all sorts of icky stuff...
void launchUpdater(string bitfighterExecutablePathAndFilename, bool forceUpdate)
{
   string updaterPath = ExtractDirectory(bitfighterExecutablePathAndFilename) + "\\updater";
   string updaterFileName = updaterPath + "\\bfup.exe";

   S32 buildVersion = forceUpdate ? 0 : BUILD_VERSION;

   S64 result = (S64) ShellExecuteA( NULL, NULL, updaterFileName.c_str(), itos(buildVersion).c_str(), updaterPath.c_str(), SW_SHOW );

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


#ifdef USE_EXCEPTION_BACKTRACE
void exceptionHandler(int sig) {
   void *stack[20];
   size_t size;
   char **functions;

   signal(SIGSEGV, NULL);   // turn off our handler


   // get void*'s for all entries on the stack
   size = backtrace(stack, 20);

   // print and log all the frames
   logprintf(LogConsumer::LogError, "Error: signal %d:", sig);
   functions = backtrace_symbols(stack, size);

   for(size_t i=0; i < size; i++)
      logprintf(LogConsumer::LogError, "%d: %s", i, functions[i]);

   free(functions);
   //exit(1); // let it die (or use debugger) the normal way, after we turn off our handler
}
#endif


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

#ifdef USE_EXCEPTION_BACKTRACE
   signal(SIGSEGV, exceptionHandler);   // install our handler
#endif

   GameSettings *settings = new GameSettings(); // Will be deleted in shutdownBitfighter()

   // Put all cmd args into a Vector for easier processing
   Vector<string> argVector(argc - 1);

   for(S32 i = 1; i < argc; i++)
      argVector.push_back(argv[i]);

   settings->readCmdLineParams(argVector);      // Read cmd line params, needed to resolve folder locations
   settings->resolveDirs();                     // Figures out where all our folders are

   FolderManager *folderManager = settings->getFolderManager();

   // Before we go any further, we should get our log files in order.  We know where they'll be, as the 
   // only way to specify a non-standard location is via the command line, which we've now read.
   setupLogging(folderManager->logDir);

   // Load the INI file
   gINI.SetPath(joindir(folderManager->iniDir, "bitfighter.ini"));

   loadSettingsFromINI(&gINI, settings);        // Read INI
   setupLogging(settings->getIniSettings());    // Turns various logging options on and off

   Ship::computeMaxFireDelay();                 // Look over weapon info and get some ranges, which we'll need before we start sending data

   settings->runCmdLineDirectives();            // If we specified a directive on the cmd line, like -help, attend to that now

   SoundSystem::init(settings->getIniSettings()->sfxSet, folderManager->sfxDir, 
                     folderManager->musicDir, settings->getIniSettings()->musicVolLevel);  // Even dedicated server needs sound these days
   
   checkIfThisIsAnUpdate(settings);             // Make any adjustments needed when we run for the first time after an upgrade


   if(settings->isDedicatedServer())
      initHostGame(settings, settings->getLevelList(), false, true);     // Figure out what levels we'll be playing with, and start hosting  
   else
   {
#ifndef ZAP_DEDICATED
      createClientGame(settings);                  // Instantiate gClientGame

      FXManager::init();                           // Get ready for sparks!!  C'mon baby!!

      resetInputCodeStates();                      // Reset keyboard state mapping to show no keys depressed

      Joystick::loadJoystickPresets();             // Load joystick presets from INI first
      Joystick::initJoystick();                    // Initialize joystick system

#ifdef TNL_OS_MAC_OSX
      moveToAppPath();        // On OS X, make sure we're in the right directory
#endif

      InitSdlVideo();         // Get our main SDL rendering window all set up
      SDL_EnableUNICODE(1);   // Activate unicode ==> http://sdl.beuc.net/sdl.wiki/SDL_EnableUNICODE
      SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);      // SDL_DEFAULT_REPEAT_DELAY defined as 500

      Zap::Cursor::init();

      atexit(shutdownBitfighter);      // If user clicks the X on their game window, this runs shutdownBitfighter()

      settings->getIniSettings()->oldDisplayMode = DISPLAY_MODE_UNKNOWN;   // We don't know what the old one was
      actualizeScreenMode(false);      // Create a display window

      gConsole = OGLCONSOLE_Create();  // Create our console *after* the screen mode has been actualized

#ifdef USE_BFUP
      if(settings->getIniSettings()->useUpdater)
         launchUpdater(argv[0], settings->getForceUpdate());  // Spawn external updater tool to check for new version of Bitfighter -- Windows only
#endif   // USE_BFUP

#endif   // !ZAP_DEDICATED
   }

   dedicatedServerLoop();              // Loop forever, running the idle command endlessly

   return 0;
}

