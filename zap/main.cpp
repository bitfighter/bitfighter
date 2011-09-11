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
</ul>

<h2>Bot scripting</h2>
<ul>
<li>Lua added copyMoveFromObject, Lua getCurrLoadout and getReqLoadout can now be used for ships
</ul>

<h2>Bug Fixes</h2>
<ul>
<li>Fix team bitmatch suicide score
<li>Teleporter, added Delay option in levels for teleporters      <<=== what is this??
</ul>

<h2>Other changes</h2>
<li>Deprecated SoccerPickup parameter -- now stored as an option on the Specials line.  Will be completely removed in 017.  Easiest fix is to load
    an affected level into the editor and save; parameter will be properly rewritten
<li>Reduced CPU usage for overlapping asteroids
<li>Removed -jsave and -jplay cmd line options.  It's been ages since they worked, and it's unlikely they ever will
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
#include "Joystick.h"
#include "Event.h"
#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#endif

#include "game.h"
#include "gameNetInterface.h"
#include "masterConnection.h"
#include "config.h"
#include "md5wrapper.h"
#include "version.h"       // For BUILD_VERSION def
#include "Colors.h"
#include "ScreenInfo.h"
#include "stringUtils.h"
//#include "BanList.h"

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

namespace Zap
{
extern ClientGame *gClientGame1;
extern ClientGame *gClientGame2;

// Handle any md5 requests
md5wrapper md5;


bool gShowAimVector = false;     // Do we render an aim vector?  This should probably not be a global, but until we find a better place for it...

CIniFile gINI("dummy");          // This is our INI file.  Filename set down in main(), but compiler seems to want an arg here.

// These will be moved to the GameSettings object
CmdLineSettings gCmdLineSettings;
IniSettings gIniSettings;

OGLCONSOLE_Console gConsole;     // For the moment, we'll just have one console for levelgens and bots.  This may change later.


// Some colors -- other candidates include global and local chat colors, which are defined elsewhere.  Include here?
Color gNexusOpenColor(0, 0.7, 0);
Color gNexusClosedColor(0.85, 0.3, 0);
Color gErrorMessageTextColor(Colors::paleRed);
Color gNeutralTeamColor(Colors::gray80);        // Objects that are neutral (on team -1)
Color gHostileTeamColor(Colors::gray50);        // Objects that are "hostile-to-all" (on team -2)
Color gMasterServerBlue(0.8, 0.8, 1);           // Messages about successful master server statii
Color gHelpTextColor(Colors::green);


S32 gMaxPolygonPoints = 32;                     // Max number of points we can have in Walls, Nexuses, LoadoutZones, etc.

DataConnection *dataConn = NULL;

U16 DEFAULT_GAME_PORT = 28000;

Address gBindAddress(IPProtocol, Address::Any, DEFAULT_GAME_PORT);      // Good for now, may be overwritten by INI or cmd line setting
// Above is equivalent to ("IP:Any:28000")

ScreenInfo gScreenInfo;

ZapJournal gZapJournal;          // Our main journaling object

//BanList gBanList;         // Our ban list

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
      ConfigDirectories *folderManager = gServerGame->getSettings()->getConfigDirs();
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
      ConfigDirectories *folderManager = gServerGame->getSettings()->getConfigDirs();
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
   {
      shutdownBitfighter();      // Quit in an orderly fashion
   }
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


U32 getServerMaxPlayers()
{
   U32 maxplay;
   if (gCmdLineSettings.maxPlayers > 0)
      maxplay = gCmdLineSettings.maxPlayers;
   else
      maxplay = gIniSettings.maxPlayers;

   if(maxplay > MAX_PLAYERS)
      maxplay = MAX_PLAYERS;

   return maxplay;
}

// Host a game (and maybe even play a bit, too!)
void initHostGame(Address bindAddress, GameSettings *settings, Vector<string> &levelList, bool testMode, bool dedicatedServer)
{
   TNLAssert(!gServerGame, "already exists!");

   gServerGame = new ServerGame(bindAddress, settings, getServerMaxPlayers(), testMode, dedicatedServer);

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
   ConfigDirectories *folderManager = gServerGame->getSettings()->getConfigDirs();

   if(folderManager->levelDir == "")     // Never did resolve a leveldir... no hosting for you!
   {
      abortHosting_noLevels();           // Not sure this would ever get called...
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

   UserInterface::renderCurrent();

   // Render master connection state if we're not connected
   // TODO: should this go elsewhere?
   if(gClientGame && gClientGame->getConnectionToMaster() &&
         gClientGame->getConnectionToMaster()->getConnectionState() != NetConnection::Connected)
   {
      glColor(Colors::white);
      UserInterface::drawStringf(10, 550, 15, "Master Server - %s", gConnectStatesTable[gClientGame->getConnectionToMaster()->getConnectionState()]);
   }

   // Swap the buffers. This this tells the driver to render the next frame from the contents of the
   // back-buffer, and to set all rendering operations to occur on what was the front-buffer.
   // Double buffering prevents nasty visual tearing from the application drawing on areas of the
   // screen that are being updated at the same time.
   SDL_GL_SwapBuffers();  // Use this if we convert to SDL
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
         gIniSettings.inputMode = InputModeJoystick;
         gClientGame1->mUserInterfaceData->get();
         gClientGame2->mUserInterfaceData->set();

         gClientGame = gClientGame2;
         gClientGame->idle(integerTime);

         gIniSettings.inputMode = InputModeKeyboard;
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
         gServerGame->idle(integerTime);
   }
}

// This is the master idle loop that gets registered with GLUT and is called on every game tick.
// This in turn calls the idle functions for all other objects in the game.
void idle()
{
   if(gServerGame)
   {
      if(gServerGame->hostingModePhase == ServerGame::LoadingLevels)
         gServerGame->loadNextLevelInfo();
      else if(gServerGame->hostingModePhase == ServerGame::DoneLoadingLevels)
         hostGame();
   }

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

   if( ( dedicated && integerTime >= S32(1000 / gIniSettings.maxDedicatedFPS)) || 
       (!dedicated && integerTime >= S32(1000 / gIniSettings.maxFPS)) )
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


   // So, what's with all the SDL code in here?  I looked at converting from GLUT to SDL, in order to get
   // a richer set of keyboard events.  Looks possible, but SDL appears to be missing some very handy
   // windowing code (e.g. the ability to resize or move a window) that GLUT has.  So until we find a
   // platform independent window library, we'll stick with GLUT, or maybe go to FreeGlut.
   // Note that moving to SDL will require our journaling system to be re-engineered.
   // Note too that SDL will require linking in SDL.lib and SDLMain.lib, and including the SDL.dll in the EXE folder.

#ifndef ZAP_DEDICATED
   // SDL requires an active polling loop.  We could use something like the following:
   SDL_Event event;

   while(SDL_PollEvent(&event))
   {
      if(event.type == SDL_QUIT) // Handle quit here
         exitToOs();

      Event::onEvent(&event);
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


extern void saveWindowMode(CIniFile *ini);

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
   if(!gClientGame && !gServerGame)
      exitToOs();

// Grab a pointer to settings wherever we can.  Note that gClientGame and gServerGame refer to the same copy.
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

   // Save settings to capture window position
   saveWindowMode(&gINI);
   // TODO: reimplement window position saving with SDL
   //   if(gIniSettings.displayMode == DISPLAY_MODE_WINDOWED)
   //      saveWindowPosition(glutGet(GLUT_WINDOW_X), glutGet(GLUT_WINDOW_Y));

   SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif

   saveSettingsToINI(&gINI, settings);    // Writes settings to the INI, then saves it to disk
//   gBanList.writeToFile();      // Writes ban list back to file XXX enable this when admin functionality is built in

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

   SDL_Surface* icon = SDL_LoadBMP("zap_win_icon.bmp");     // <=== TODO: put a real bmp here...
   SDL_WM_SetIcon(icon, NULL);

   // We want to request that SDL provide us with an OpenGL window, possibly in a fullscreen video mode.
   // Note the SDL_DOUBLEBUF flag is not required to enable double buffering when setting an OpenGL
   // video mode. Double buffering is enabled or disabled using the SDL_GL_DOUBLEBUFFER attribute.
   flags = SDL_OPENGL | SDL_HWSURFACE;

   // We don't need a size to initialize
   if(SDL_SetVideoMode(0, 0, 0, flags) == NULL)
   {
      logprintf(LogConsumer::LogWarning, "Unable to create hardware OpenGL window, falling back to software");

      flags = SDL_OPENGL;
      gScreenInfo.setHardwareSurface(false);

      if (SDL_SetVideoMode(0, 0, 0, flags) == NULL)
      {
         logprintf(LogConsumer::LogFatalError, "Unable to create OpenGL window: %s", SDL_GetError());
         exitToOs();
      }
   }
   else
   {
      logprintf("Using hardware OpenGL window");
      gScreenInfo.setHardwareSurface(true);
   }
}
#endif


// Now integrate INI settings with those from the command line and process them
void processStartupParams(GameSettings *settings)
{
   // These options can only be set on cmd line
   if(!gCmdLineSettings.server.empty())
      gBindAddress.set(gCmdLineSettings.server);

   if(!gCmdLineSettings.dedicated.empty())
      gBindAddress.set(gCmdLineSettings.dedicated);

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


   settings->initServerPassword(gCmdLineSettings.serverPassword, gIniSettings.serverPassword);
   settings->initAdminPassword(gCmdLineSettings.adminPassword, gIniSettings.adminPassword);
   settings->initLevelChangePassword(gCmdLineSettings.levelChangePassword, gIniSettings.levelChangePassword);

   ConfigDirectories *folderManager = settings->getConfigDirs();

   folderManager->resolveLevelDir(); 

   if(gIniSettings.levelDir == "")                       // If there is nothing in the INI,
      gIniSettings.levelDir = folderManager->levelDir;   // write a good default to the INI

   settings->initHostName(gCmdLineSettings.hostname, gIniSettings.hostname);
   settings->initHostDescr(gCmdLineSettings.hostdescr, gIniSettings.hostdescr);

   if(gCmdLineSettings.hostaddr != "")
      gBindAddress.set(gCmdLineSettings.hostaddr);
   else if(gIniSettings.hostaddr != "")
      gBindAddress.set(gIniSettings.hostaddr);
   // else stick with default defined earlier


   if(gCmdLineSettings.displayMode != DISPLAY_MODE_UNKNOWN)
      gIniSettings.displayMode = gCmdLineSettings.displayMode;    // Simply clobber the gINISettings copy

   if(gCmdLineSettings.xpos != -9999)
      gIniSettings.winXPos = gCmdLineSettings.xpos;
   if(gCmdLineSettings.ypos != -9999)
      gIniSettings.winYPos = gCmdLineSettings.ypos;
   if(gCmdLineSettings.winWidth > 0)
      gIniSettings.winSizeFact = max((F32) gCmdLineSettings.winWidth / (F32) gScreenInfo.getGameCanvasWidth(), gScreenInfo.getMinScalingFactor());

   Game::setMasterAddress(gCmdLineSettings.masterAddress, gIniSettings.masterAddress);    // The INI one will always have a value
   

   if(gCmdLineSettings.name != "")                          // We'll clobber the INI file setting.  Since this
      gIniSettings.name = gCmdLineSettings.name;            // setting is never saved, we won't mess up our INI

   if(gCmdLineSettings.password != "")                      // We'll clobber the INI file setting.  Since this
      gIniSettings.password = gCmdLineSettings.password;    // setting is never saved, we won't mess up our INI


#ifndef ZAP_DEDICATED
   if(!gCmdLineSettings.dedicatedMode)                      // Create ClientGame object
   {
      gClientGame1 = new ClientGame(Address(), settings);   //   Let the system figure out IP address and assign a port
      gClientGame = gClientGame1;

      gClientGame->setLoginPassword(gCmdLineSettings.password, gIniSettings.password, gIniSettings.lastPassword);

       // Put any saved filename into the editor file entry thingy
      gClientGame->getUIManager()->getLevelNameEntryUserInterface()->setString(gIniSettings.lastEditorName);

      //gClientGame2 = new ClientGame(Address());   //  !!! 2-player split-screen game in same game.

      if(gIniSettings.name == "")
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
         seedRandomNumberGenerator(gIniSettings.lastName);
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
         seedRandomNumberGenerator(gIniSettings.name);
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

   // TODO: reimplement window positioning - difficult with SDL since it doesn't have much access to the
   // window manager; however, it may be possible to do position upon start-up, but not save when exiting

   //   if(gIniSettings.oldDisplayMode == DISPLAY_MODE_WINDOWED && !first)
   //   {
   //      gIniSettings.winXPos = glutGet(GLUT_WINDOW_X);
   //      gIniSettings.winYPos = glutGet(GLUT_WINDOW_Y);
   //
   //      gINI.SetValueI("Settings", "WindowXPos", gIniSettings.winXPos, true);
   //      gINI.SetValueI("Settings", "WindowYPos", gIniSettings.winYPos, true);
   //   }

   if(changingInterfaces)
      gClientGame->getUIManager()->getPrevUI()->onPreDisplayModeChange();
   else
      UserInterface::current->onPreDisplayModeChange();

   DisplayMode displayMode = gIniSettings.displayMode;

   gScreenInfo.resetGameCanvasSize();     // Set GameCanvasSize vars back to their default values

   // When we're in the editor, let's take advantage of the entire screen unstretched
   if(UserInterface::current->getMenuID() == EditorUI && 
         (gIniSettings.displayMode == DISPLAY_MODE_FULL_SCREEN_STRETCHED || gIniSettings.displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED))
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
   sdlVideoFlags = gScreenInfo.isHardwareSurface() ? SDL_OPENGL | SDL_HWSURFACE : SDL_OPENGL;

   // Set up variables according to display mode
   switch (displayMode)
   {
   case DISPLAY_MODE_FULL_SCREEN_STRETCHED:
      sdlWindowWidth = gScreenInfo.getPhysicalScreenWidth();
      sdlWindowHeight = gScreenInfo.getPhysicalScreenHeight();
      sdlVideoFlags |= SDL_FULLSCREEN;

      orthoRight = gScreenInfo.getGameCanvasWidth();
      orthoBottom = gScreenInfo.getGameCanvasHeight();
      break;

   case DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED:
      sdlWindowWidth = gScreenInfo.getPhysicalScreenWidth();
      sdlWindowHeight = gScreenInfo.getPhysicalScreenHeight();
      sdlVideoFlags |= SDL_FULLSCREEN;

      orthoLeft = -1 * (gScreenInfo.getHorizDrawMargin());
      orthoRight = gScreenInfo.getGameCanvasWidth() + gScreenInfo.getHorizDrawMargin();
      orthoBottom = gScreenInfo.getGameCanvasHeight() + gScreenInfo.getVertDrawMargin();
      orthoTop = -1 * (gScreenInfo.getVertDrawMargin());
      break;

   default:  //  DISPLAY_MODE_WINDOWED
      sdlWindowWidth = (S32) floor((F32)gScreenInfo.getGameCanvasWidth()  * gIniSettings.winSizeFact + 0.5f);
      sdlWindowHeight = (S32) floor((F32)gScreenInfo.getGameCanvasHeight() * gIniSettings.winSizeFact + 0.5f);
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


   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glLineWidth(gDefaultLineWidth);

   if(gIniSettings.useLineSmoothing)
   {
      glEnable(GL_LINE_SMOOTH);
      //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
      glEnable(GL_BLEND);
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
      gIniSettings.inputMode = InputModeKeyboard;
   else
      gIniSettings.inputMode = InputModeJoystick;
}
#endif


// Function to handle one-time update tasks
// Use this when upgrading, and changing something like the name of an INI parameter.  The old version is stored in
// gIniSettings.version, and the new version is in BUILD_VERSION.
void checkIfThisIsAnUpdate()
{
   if(gIniSettings.version == BUILD_VERSION)
      return;

   // Wipe out all comments; they will be replaced with any updates
   gINI.deleteHeaderComments();
   gINI.deleteAllSectionComments();

   // version specific changes
   // 015a
   if(gIniSettings.version < 1836)
      gIniSettings.useLineSmoothing = true;

   // after 015a
   if(gIniSettings.version < 1840 && gIniSettings.maxBots == 127)
      gIniSettings.maxBots = 10;
}


#ifdef USE_BFUP
#include <direct.h>
#include <stdlib.h>
#include "stringUtils.h"      // For itos

// This block is Windows only, so it can do all sorts of icky stuff...
void launchUpdater(string bitfighterExecutablePathAndFilename)
{
   string updaterPath = ExtractDirectory(bitfighterExecutablePathAndFilename) + "\\updater";
   string updaterFileName = updaterPath + "\\bfup.exe";

   S32 buildVersion = gCmdLineSettings.forceUpdate ? 0 : BUILD_VERSION;

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

   GameSettings *settings = new GameSettings(); // Will be deleted in shutdownBitfighter()

   // Put all cmd args into a Vector for easier processing
   Vector<string> argVector(argc - 1);

   for(S32 i = 1; i < argc; i++)
      argVector.push_back(argv[i]);

   ConfigDirectories *folderManager = settings->getConfigDirs();

   gCmdLineSettings.readParams(settings, argVector, 0);   // Read first tranche of cmd line params, needed to resolve folder locations
   folderManager->resolveDirs(settings);                  // Resolve all folders except for levels folder, which is resolved later

   // Before we go any further, we should get our log files in order.  Now we know where they'll be, as the 
   // only way to specify a non-standard location is via the command line, which we've now read.
   setupLogging(folderManager->logDir);

   gCmdLineSettings.readParams(settings, argVector, 1);   // Read remaining cmd line params 

   // Load the INI file
   gINI.SetPath(joindir(folderManager->iniDir, "bitfighter.ini"));
   gIniSettings.init();                   // Init struct that holds INI settings

   loadSettingsFromINI(&gINI, settings);  // Read INI

   processStartupParams(settings);        // And merge command line params and INI settings
   Ship::computeMaxFireDelay();           // Look over weapon info and get some ranges, which we'll need before we start sending data

   if(gCmdLineSettings.dedicatedMode)
   {
      Vector<string> levels = LevelListLoader::buildLevelList(folderManager->levelDir, settings->getLevelSkipList());
      initHostGame(gBindAddress, settings, levels, false, true);       // Start hosting
   }

   SoundSystem::init(folderManager->sfxDir, folderManager->musicDir);  // Even dedicated server needs sound these days
   
   checkIfThisIsAnUpdate();


#ifndef ZAP_DEDICATED
   if(gClientGame)     // Only exists when we're starting up in interactive mode, as opposed to running a dedicated server
   {
      FXManager::init();                           // Get ready for sparks!!  C'mon baby!!
      Joystick::populateJoystickStaticData();      // Build static data needed for joysticks
      Joystick::initJoystick();                    // Initialize joystick system
      resetKeyStates();                            // Reset keyboard state mapping to show no keys depressed
      ControllerTypeType controllerType = Joystick::autodetectJoystickType();
      setJoystick(controllerType);                 // Will override INI settings, so process INI first

      
#ifdef TNL_OS_MAC_OSX
      moveToAppPath();        // On OS X, make sure we're in the right directory
#endif

      InitSdlVideo();         // Get our main SDL rendering window all set up
      SDL_EnableUNICODE(1);   // Activate unicode ==> http://sdl.beuc.net/sdl.wiki/SDL_EnableUNICODE
      SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);      // SDL_DEFAULT_REPEAT_DELAY defined as 500

      atexit(shutdownBitfighter);      // If user clicks the X on their game window, this runs shutdownBitfighter()
      actualizeScreenMode(false);      // Create a display window

      gConsole = OGLCONSOLE_Create();  // Create our console *after* the screen mode has been actualized

#ifdef USE_BFUP
      if(gIniSettings.useUpdater)
         launchUpdater(argv[0]);       // Spawn external updater tool to check for new version of Bitfighter -- Windows only
#endif

   }
#endif
   dedicatedServerLoop();              // Loop forever, running the idle command endlessly

   return 0;
}

