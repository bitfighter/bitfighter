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
/* Fixes after 016
<h2>New features</h2>
<ul>
<li>
</ul>


// Scripting nots
bot:findItems --> findItems
bot:findGlobalItems --> findItems

printToOglConsole --> printToConsole
include (replaces require)

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
#  pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#include "IniFile.h"

#include "tnl.h"
#include "tnlRandom.h"
#include "tnlGhostConnection.h"
#include "tnlJournal.h"

#include "zapjournal.h"

#include "md5wrapper.h"


using namespace TNL;

#ifndef ZAP_DEDICATED
#  include "UIGame.h"
#  include "UINameEntry.h"
#  include "UIEditor.h"
#  include "UIErrorMessage.h"
#  include "Cursor.h"          // For cursor defs
#  include "Joystick.h"
#  include "Event.h"
#  include "SDL.h"

#  ifdef TNL_OS_MOBILE
#    include "SDL_opengles.h"
#  else
#    include "SDL_opengl.h"
#  endif

#  include "VideoSystem.h"
#  include "ClientGame.h"
#endif

#include "ServerGame.h"
#include "version.h"       // For BUILD_VERSION def
#include "Colors.h"
#include "ScreenInfo.h"
#include "stringUtils.h"
#include "BanList.h"
#include "dataConnection.h"
#include "game.h"
#include "SoundSystem.h"
#include "InputCode.h"     // initializeKeyNames()
#include "ClientInfo.h"
#include "Console.h"       // For access to console

#include <math.h>
#include <stdarg.h>

#ifdef WIN32
// For writeToConsole()
#  include <windows.h>
#  include <io.h>
#  include <fcntl.h>
#  include <shellapi.h>

#  define USE_BFUP
#endif

#if defined(TNL_OS_MAC_OSX) || defined(TNL_OS_IOS)
#  include "Directory.h"
#  include <unistd.h>
#endif

#ifdef __MINGW32__
#undef main
#endif


// Maybe don't enable by default?
//#if defined(TNL_OS_LINUX) && defined(ZAP_DEDICATED)
//#define USE_EXCEPTION_BACKTRACE
//#endif

#ifdef USE_EXCEPTION_BACKTRACE
#  include <execinfo.h>
#  include <signal.h>
#endif


namespace Zap
{
#ifndef ZAP_DEDICATED
   extern Vector<ClientGame *> gClientGames;
#endif

// Handle any md5 requests
md5wrapper md5;


bool gShowAimVector = false;     // Do we render an aim vector?  This should probably not be a global, but until we find a better place for it...

CIniFile gINI("dummy");          // This is our INI file.  Filename set down in main(), but compiler seems to want an arg here.
Console gConsole;                // For the moment, we'll just have one console for everything.  This may change later, but probably won't.


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


#ifndef ZAP_DEDICATED
   for(S32 i = 0; i < gClientGames.size(); i++)
   {
      UIManager *uiManager = gClientGames[i]->getUIManager();

      ErrorMessageUserInterface *errUI = static_cast<ErrorMessageUserInterface *>(uiManager->getUI(ErrorMessageUI));

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

      uiManager->activate(ErrorMessageUI);

      HostMenuUserInterface *menuUI = static_cast<HostMenuUserInterface *>(uiManager->getUI(HostingUI));
      menuUI->levelLoadDisplayDisplay = false;
      menuUI->levelLoadDisplayFadeTimer.clear();
   }
#endif

   delete gServerGame;  // need gServerGame for above message
   gServerGame = NULL;

#ifndef ZAP_DEDICATED
   if(gClientGames.size() == 0)
#endif
      shutdownBitfighter();      // Quit in an orderly fashion
}


// GCC thinks min isn't defined, VC++ thinks it is
#ifndef min
#  define min(a,b) ((a) <= (b) ? (a) : (b))
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
      for(S32 i = 0; i < gClientGames.size(); i++)
         gClientGames[i]->getUIManager()->getHostMenuUserInterface()->levelLoadDisplayDisplay = true;
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
   for(S32 i = 0; i < gClientGames.size(); i++)      // Should be true if this isn't a dedicated server...
   {
      HostMenuUserInterface *ui = gClientGames[i]->getUIManager()->getHostMenuUserInterface();

      ui->levelLoadDisplayDisplay = false;
      ui->levelLoadDisplayFadeTimer.reset();

      gClientGames[i]->joinGame(Address(), false, true);   // ...then we'll play, too!
   }
#endif
}


// Clear screen -- force clear of "black bars" area to avoid flickering on some video cards
static void clearScreen()
{
#ifndef ZAP_DEDICATED

   bool scissorMode = glIsEnabled(GL_SCISSOR_TEST);

   if(scissorMode)
      glDisable(GL_SCISSOR_TEST);

   glClear(GL_COLOR_BUFFER_BIT);

   if(scissorMode)
      glEnable(GL_SCISSOR_TEST);

#endif
}



#ifndef ZAP_DEDICATED

// Draw the screen
void display()
{
   clearScreen();

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   for(S32 i = 0; i < gClientGames.size(); i++)
   {
      // Do any la-ti-da that we might need to get the viewport setup for the game we're about to run.  For example, if
      // we have two games, we might want to divide the screen into two viewports, configuring each before running the 
      // associated render method which follows...
      // Each viewport should have an aspect ratio of 800x600.  The aspect ratio of the entire window will likely need to be different.
      TNLAssert(i == 0, "You need a little tra-la-la here before you can do that!");
      gClientGames[i]->getUIManager()->renderCurrent();
   }

   // Swap the buffers. This this tells the driver to render the next frame from the contents of the
   // back-buffer, and to set all rendering operations to occur on what was the front-buffer.
   // Double buffering prevents nasty visual tearing from the application drawing on areas of the
   // screen that are being updated at the same time.
#if SDL_VERSION_ATLEAST(2,0,0)
   SDL_GL_SwapWindow(gScreenInfo.sdlWindow);
#else
   SDL_GL_SwapBuffers();  // Use this if we convert to SDL
#endif
}


#endif // ZAP_DEDICATED



void gameIdle(U32 integerTime)
{
#ifndef ZAP_DEDICATED

   // If the main game interface is in the stack, idle that too to keep things current, update timers, etc.
   for(S32 i = 0; i < gClientGames.size(); i++)
   {
      UIManager *uiManager = gClientGames[i]->getUIManager();

      // Idle the currently active UI
      if(uiManager->getCurrentUI())
         uiManager->getCurrentUI()->idle(integerTime);

      // If we're in a UI and GameUI is still running, we need to idle that too
      if(uiManager->cameFrom(GameUI))
         uiManager->getGameUserInterface()->idle(integerTime);
   }
#endif

   // Don't idle games during level load
   if(!(gServerGame && gServerGame->hostingModePhase == ServerGame::LoadingLevels))    
   {
#ifndef ZAP_DEDICATED
      for(S32 i = 0; i < gClientGames.size(); i++)
      {
         //if(gClientGame2)
         //{
         //   gClientGame2->getSettings()->getInputCodeManager()->setInputMode(InputModeJoystick);

         //   gClientGame1->mUserInterfaceData->get();
         //   gClientGame2->mUserInterfaceData->set();

         //   gClientGame = gClientGame2;
         //   gClientGame->idle(integerTime);

         //   gClientGame->getSettings()->getInputCodeManager()->setInputMode(InputModeKeyboard);

         //   gClientGame2->mUserInterfaceData->get();
         //   gClientGame1->mUserInterfaceData->set();
         //}
         //if(gClientGame1)
         //{
         //   gClientGame = gClientGame1;
         //   gClientGame->idle(integerTime);
         //}

         gClientGames[i]->idle(integerTime);
      }
#endif

      if(gServerGame)
      {
         // This block has to be outside gServerGame because deleting an object from within is in rather poor form
         if(!gServerGame->isReadyToShutdown(integerTime))         
            gServerGame->idle(integerTime);

         else                                                     // gServerGame closing down so...
         {
#ifndef ZAP_DEDICATED
            for(S32 i = 0; i < gClientGames.size(); i++)
               gClientGames[i]->closeConnectionToGameServer();    // ...disconnect any local clients

           if(gClientGames.size() > 0)    // If there are any clients running...
            {
               delete gServerGame;        // ...purge gServerGame (leaving the clients running)...
               gServerGame = NULL;
            }
            else                          // ...otherwise...     
#endif
               shutdownBitfighter();      // ...shut down the whole shebang!
         }
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
   else     // If there is no server game, and this code is running, there *MUST* be a client game.
      settings = gClientGames[0]->getSettings();
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
      TNLAssert(gClientGames.size() > 0, "Why are we here if there is no client game??");

      if(event.type == SDL_QUIT) // Handle quit here
         shutdownBitfighter();

      // Pass the event to all clientGames..
      for(S32 i = 0; i < gClientGames.size(); i++)
         Event::onEvent(gClientGames[i], &event);
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

// Include class here to avoid contaminating tnlLog with the filth that is oglConsole
class OglConsoleLogConsumer : public LogConsumer    // Dumps to oglConsole
{
private:
   void writeString(const char *string) { gConsole.output(string); }
};


////////////////////////////////////////
////////////////////////////////////////
// Our logfiles
StdoutLogConsumer gStdoutLog;          // Logs to OS console, when there is one
OglConsoleLogConsumer gOglConsoleLog;  // Logs to our in-game console, when available

FileLogConsumer gMainLog;
FileLogConsumer gServerLog;            // We'll apply a filter later on, in main()

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
// and one illigitimate way
// 7) Lua panics!!
void shutdownBitfighter()
{
   GameSettings *settings = NULL;

   // Avoid this function being called twice when we exit via methods 1-4 above
#ifndef ZAP_DEDICATED
   if(gClientGames.size() == 0)
#endif
      if(!gServerGame)
         exitToOs();

// Grab a pointer to settings wherever we can.  Note that gClientGame and gServerGame refer to the same object.
#ifndef ZAP_DEDICATED
   if(gClientGames.size() > 0)
      settings = gClientGames[0]->getSettings();

   gClientGames.deleteAndClear();

#endif

   if(gServerGame)
   {
      settings = gServerGame->getSettings();
      delete gServerGame;     // Destructor terminates connection to master
      gServerGame = NULL;
   }


   TNLAssert(settings, "Should always have a value here!");

   EventManager::shutdown();
   LuaScriptRunner::shutdown();
   SoundSystem::shutdown();

   BotNavMeshZone::deleteBotZoneDatabase();

   if(!settings->isDedicatedServer())
   {
#ifndef ZAP_DEDICATED
      Joystick::shutdownJoystick();

      // Save current window position if in windowed mode
      if(settings->getIniSettings()->displayMode == DISPLAY_MODE_WINDOWED)
      {
         settings->getIniSettings()->winXPos = VideoSystem::getWindowPositionX();
         settings->getIniSettings()->winYPos = VideoSystem::getWindowPositionY();
      }

      SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif
   }

   // Avoids annoying shutdown crashes when logging is still trying to output to oglconsole
   gOglConsoleLog.setMsgTypes(LogConsumer::LogNone);

   settings->save();                                  // Write settings to bitfighter.ini

   delete settings;

   NetClassRep::logBitUsage();
   logprintf("Bye!");

   exitToOs();    // Do not pass Go
}


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
      // Create a new client, and let the system figure out IP address and assign a port
      ClientGame *clientGame = new ClientGame(Address(IPProtocol, Address::Any, settings->getIniSettings()->clientPortNumber), settings);  

      clientGame->setLoginPassword(settings->getPlayerPassword());

       // Put any saved filename into the editor file entry thingy
      clientGame->getUIManager()->getLevelNameEntryUserInterface()->setString(settings->getIniSettings()->lastEditorName);

      seedRandomNumberGenerator(settings->getIniSettings()->lastName);
      clientGame->getClientInfo()->getId()->getRandom();

      gClientGames.push_back(clientGame);

      //gClientGames.push_back(new ClientGame(Address(), settings));   //  !!! 2-player split-screen game in same game.

      // Set the intial UI
      if(settings->shouldShowNameEntryScreenOnStartup())
      {
         for(S32 i = 0; i < gClientGames.size(); i++)
            gClientGames[i]->getUIManager()->activate(NameEntryUI);

         //if(gClientGame)
         //{
         //   gClientGame = gClientGame2;
         //   gClientGame1->mUserInterfaceData->get();
         //   gClientGame->getUIManager()->getNameEntryUserInterface()->activate();  <-- won't work no more!
         //   gClientGame2->mUserInterfaceData->get();
         //   gClientGame1->mUserInterfaceData->set();
         //   gClientGame = gClientGame1;
         //}
         //gClientGame->getUIManager()->getNameEntryUserInterface()->activate();     <-- won't work no more!
         seedRandomNumberGenerator(settings->getIniSettings()->lastName);
      }
      else
      {
         for(S32 i = 0; i < gClientGames.size(); i++)
         {
            gClientGames[i]->getUIManager()->activate(MainUI);
            gClientGames[i]->setReadyToConnectToMaster(true);        
         }

         //if(gClientGame2)
         //{
         //   gClientGame = gClientGame2;
         //   gClientGame1->mUserInterfaceData->get();

         //   gClientGame2->mUserInterfaceData->get();
         //   gClientGame1->mUserInterfaceData->set();
         //   gClientGame = gClientGame1;
         //}
         //gClientGame->getUIManager()->getMainMenuUserInterface()->activate();<-- won't work no more!

         //gClientGame->setReadyToConnectToMaster(true);         // Set elsewhere if in dedicated server mode
         seedRandomNumberGenerator(settings->getPlayerName());
      }
   }
#endif
}


void setupLogging(const string &logDir)
{
   // Specify which events each logfile will listen for
   S32 events = LogConsumer::AllErrorTypes | LogConsumer::LogConnection | LogConsumer::LuaLevelGenerator | LogConsumer::LuaBotMessage;
   S32 consoleEvents = LogConsumer::AllErrorTypes |LogConsumer::LuaLevelGenerator | LogConsumer::LuaBotMessage | LogConsumer::ConsoleMsg;

   gMainLog.init(joindir(logDir, "bitfighter.log"), "w");
   //gMainLog.setMsgTypes(events);  ==> set from INI settings     
   gMainLog.logprintf("------ Bitfighter Log File ------");

   gStdoutLog.setMsgTypes(events);              // writes to stdout
   gOglConsoleLog.setMsgTypes(consoleEvents);   // writes to in-game console

   gServerLog.init(joindir(logDir, "bitfighter_server.log"), "a");
   gServerLog.setMsgTypes(LogConsumer::AllErrorTypes | LogConsumer::ServerFilter | LogConsumer::StatisticsFilter); 
   gStdoutLog.logprintf("Welcome to Bitfighter!");
}


#ifdef USE_BFUP
#include <direct.h>
#include <stdlib.h>

// This block is Windows only, so it can do all sorts of icky stuff...
void launchWindowsUpdater(string bitfighterExecutablePathAndFilename, bool forceUpdate)
{
   string updaterPath = extractDirectory(bitfighterExecutablePathAndFilename) + "\\updater";
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


void checkOnlineUpdate(string exePath, GameSettings *settings)
{
   // Windows only
#ifdef USE_BFUP
   // Spawn external updater tool to check for new version of Bitfighter
   if(settings->getIniSettings()->useUpdater)
      launchWindowsUpdater(exePath, settings->getForceUpdate());
#endif   // USE_BFUP

   // Mac OSX only
#ifdef TNL_OS_MAC_OSX
   checkForUpdates();  // From Directory.h
#endif
}


// Change this for packaging.  Can be done in package processing scripts using sed
// We probably need to migrate to a build system sometime...
#ifdef TNL_OS_LINUX
#include "../config.h"
#endif


// This function returns the path where the game resources have been installed into the system
// before being copied to the user's data path
string getInstalledDataDir()
{
   string path;

#if defined(TNL_OS_LINUX)
   // In Linux, the data dir can be anywhere!  Usually in something like /usr/share/bitfighter
   // or /usr/local/share/bitfighter
#ifdef LINUX_DATA_DIR
   path = string(LINUX_DATA_DIR) + "/bitfighter";
#else
   // We'll default to the directory the executable is in
   char buffer[1024];
   readlink("/proc/self/exe", buffer, sizeof(buffer));
   path = extractDirectory(string(buffer));
#endif

#elif defined(TNL_OS_MAC_OSX) || defined(TNL_OS_IOS)
   getAppResourcePath(path);  // Directory.h

#elif defined(TNL_OS_WIN32)
   // On Windows, the installed data dir is always where the executable is
   char buffer[MAX_PATH];
   GetModuleFileName(NULL, buffer, MAX_PATH);
   path = extractDirectory(string(buffer));

#else
#  error "Path needs to be defined for this platform"
#endif

   return path;
}


// Make sure we're in a sane working directory.  Mostly for preparation in copying
// resources, if needed, but also for properly running standalone builds
void normalizeWorkingDirectory()
{
#if defined(TNL_OS_MAC_OSX) || defined(TNL_OS_IOS)
   // Move to the application bundle's path (RDW)
   moveToAppPath();  // Directory.h
#else
   // Move to the installed data directory
   chdir(getInstalledDataDir().c_str());
#endif
}


// This function returns the path from where game resources are loaded
string getUserDataDir()
{
   string path;

#if defined(TNL_OS_LINUX)
   path = string(SDL_getenv("HOME")) + "/.bitfighter";  // TODO: migrate to XDG standards?  Too much work for now!

#elif defined(TNL_OS_MAC_OSX) || defined(TNL_OS_IOS)
   getUserDataPath(path);  // Directory.h

#elif defined(TNL_OS_WIN32)
   path = string(SDL_getenv("APPDATA")) + "\\Bitfighter";

#else
#  error "Path needs to be defined for this platform"
#endif

   return path;
}


void setDefaultPaths(Vector<string> &argv)
{
   // If we don't already have -rootdatadir specified on the command line
   if(!argv.contains("-rootdatadir"))
   {
      argv.push_back("-rootdatadir");
      argv.push_back(getUserDataDir());
   }

   // Same with -sfxdir
   if(!argv.contains("-sfxdir"))
   {
      argv.push_back("-sfxdir");
      argv.push_back(getInstalledDataDir() + getFileSeparator() + "sfx");
   }

   // iOS needs the INI in an editable location
#ifdef TNL_OS_IOS
   string fillPath;
   getDocumentsPath(fillPath);  // From Directory.h

   argv.push_back("-inidir");
   argv.push_back(fillPath);
#endif
}


void copyResources()
{
   // Just in case - no resource copying on mobile!
#if defined(TNL_OS_MOBILE)
   return;
#endif

   printf("Copying resources");

   // Everything but sfx
   Vector<string> dirArray;
   dirArray.push_back("levels");
   dirArray.push_back("robots");
   dirArray.push_back("scripts");
   dirArray.push_back("editor_plugins");
   dirArray.push_back("music");

   string userDataDir = getUserDataDir();
   string installDataDir = getInstalledDataDir();
   string fileSeparator = getFileSeparator();

   for(S32 i = 0; i < dirArray.size(); i++)
   {
      // Make sure each resource folder exists
      string userResourceDir = userDataDir + fileSeparator + dirArray[i];

      printf("Setting up folder: %s\n", userResourceDir.c_str());
      if(!makeSureFolderExists(userResourceDir))
      {
         printf("Resource directory creation failed: %s\n", userResourceDir.c_str());
         return;
      }

      // Now copy all files.  First find all files in the installed data directory for this
      // Resource dir
      string installedResourceDir = installDataDir + fileSeparator + dirArray[i];

      Vector<string> fillFiles;
      getFilesFromFolder(installedResourceDir, fillFiles);

      for(S32 i = 0; i < fillFiles.size(); i++)
      {
         string sourceFile = installedResourceDir + fileSeparator + fillFiles[i];
         printf("Attempting to copy file: %s\n", sourceFile.c_str());
         if(!copyFileToDir(sourceFile, userResourceDir))
         {
            printf("File copy failed.  File: %s to directory: %s\n", fillFiles[i].c_str(), userResourceDir.c_str());
            return;
         }
      }
   }
}


// Initial set-up actions taken if we discover this is the first time the game has been
// run by this user
void prepareFirstLaunch()
{
   string userDataDir = getUserDataDir();

   // Create our user data directory if it doesn't exist
   if(!makeSureFolderExists(userDataDir))
   {
      printf("User data directory creation failed: %s\n", userDataDir.c_str());
      return;
   }

   // Now copy resources from installed data directory to the newly created user data directory
   copyResources();

   // Do some other platform specific things
#ifdef TNL_OS_MAC_OSX
   prepareFirstLaunchMac();
#endif
}


// Function to handle one-time update tasks
// Use this when upgrading, and changing something like the name of an INI parameter.  The old version is stored in
// IniSettings.version, and the new version is in BUILD_VERSION.
void checkIfThisIsAnUpdate(GameSettings *settings)
{
   // Previous version is what the INI currently says
   U32 previousVersion = settings->getIniSettings()->version;

   // If we're at the same version as our INI, no need to update anything
   if(previousVersion == BUILD_VERSION)
      return;

   logprintf("Bitfighter was recently updated.  Migrating user preferences...");

   // Wipe out all comments; they will be automatically replaced with any updates
   gINI.deleteHeaderComments();
   gINI.deleteAllSectionComments();

   // Now for the version specific changes.  This can only grow larger!
   // See version.h for short history of roughly what version corresponds to a game release

   // 016:
   if(previousVersion < 1840 && settings->getIniSettings()->maxBots == 127)
      settings->getIniSettings()->maxBots = 10;

   if(previousVersion < 3737)
   {
      // Master server changed
      settings->getIniSettings()->masterAddress = MASTER_SERVER_LIST_ADDRESS;

      // We added editor plugins
      gINI.addSection("EditorPlugins");
      gINI.SetValue("EditorPlugins", "Plugin1", "Ctrl+;|draw_arcs.lua|Make curves!");
   }

   // 017:
   if(previousVersion < 4262)
   {
#ifdef TNL_OS_MAC_OSX
      settings->getIniSettings()->useFakeFullscreen = true;
#endif
   }

   // 018
   if(previousVersion < 5890)  // roughly.  TODO: change to correct version when about to release
   {
      FolderManager *folderManager = settings->getFolderManager();

      // Remove game.ogg from music folder, if it exists...
      if(remove(joindir(folderManager->musicDir, "game.ogg").c_str()) != 0)
         logprintf(LogConsumer::LogWarning, "Could not remove game.ogg from music folder during upgrade process." );
   }

   // Now copy over resources to user's preference directory.  This will overwrite the previous
   // resources with same names
   copyResources();
}


bool standaloneDetected()
{
   bool isStandalone = false;

   // If we did a debug compile, default standalone mode
#ifdef TNL_DEBUG
#  ifndef TNL_OS_MAC_OSX  // Mac is always non-standalone
   isStandalone = true;   // XXX Comment this out to test resource copying in debug build
#  endif
#endif

   return isStandalone;
}


};  // namespace Zap


#ifdef USE_EXCEPTION_BACKTRACE
void exceptionHandler(int sig) {
   void *stack[20];
   size_t size;
   char **functions;

   signal(SIGSEGV, NULL);   // turn off our handler


   // get void*'s for all entries on the stack
   size = backtrace(stack, 20);  // note, this uses malloc which may cause this to freeze if it segfault inside malloc

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

#if defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x0500  // windows 2000 or later
#  define USE_HIDING_CONSOLE
#endif

#ifdef USE_HIDING_CONSOLE
// with some help of searching and finding this:
// http://stackoverflow.com/questions/8610489/distinguish-if-program-runs-by-clicking-on-the-icon-typing-its-name-in-the-cons
static bool thisProgramHasCreatedConsoleWindow()
{
   HWND consoleWindow = GetConsoleWindow();
   if (consoleWindow != NULL)
   {
      DWORD windowCreatorProcessId;
      GetWindowThreadProcessId(consoleWindow, &windowCreatorProcessId);
      return (windowCreatorProcessId == GetCurrentProcessId()) ? true : false;
   }
   return false;
}
#endif

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

// Enable some heap checking stuff for Windows... slow... do not include in release version!!
//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF );


#ifdef USE_EXCEPTION_BACKTRACE
   signal(SIGSEGV, exceptionHandler);   // install our handler
#endif

   GameSettings *settings = new GameSettings(); // Will be deleted in shutdownBitfighter()

   // Put all cmd args into a Vector for easier processing
   Vector<string> argVector(argc - 1);

   for(S32 i = 1; i < argc; i++)
      argVector.push_back(argv[i]);

   printf("=> install data dir: %s\n", getInstalledDataDir().c_str());
   printf("=> user data dir: %s\n", getUserDataDir().c_str());

   bool isFirstLaunchEver = false;  // Is this the first time we've run for this user?

   // We change our current directory to be useful, usually to the location the executable resides
   normalizeWorkingDirectory();

   // Set default -rootdatadir, -sfxdir, and others if they are not set already, unless
   // we're in standalone mode.  This allows use to have default environment setups on
   // each platform
   if(!standaloneDetected())
   {
      // Copy resources to user data if it doesn't exist
      if(!fileExists(getUserDataDir()))
      {
         isFirstLaunchEver = true;

         prepareFirstLaunch();
      }

      // Set the default paths
      setDefaultPaths(argVector);
   }
   else
      printf("Standalone run detected\n");

   settings->readCmdLineParams(argVector);      // Read cmd line params, needed to resolve folder locations
   settings->resolveDirs();                     // Figures out where all our folders are (except leveldir)

   FolderManager *folderManager = settings->getFolderManager();

   // Before we go any further, we should get our log files in order.  We know where they'll be, as the 
   // only way to specify a non-standard location is via the command line, which we've now read.
   setupLogging(folderManager->logDir);

   InputCodeManager::initializeKeyNames();      // Used by loadSettingsFromINI()

   // Load our INI
   gINI.SetPath(joindir(folderManager->iniDir, "bitfighter.ini"));
   loadSettingsFromINI(&gINI, settings);

   // Time to check if there is an online update (for any relevant platforms)
   checkOnlineUpdate(argv[0], settings);

   // Make any adjustments needed when we run for the first time after an upgrade
   // Skip if this is the first run
   if(!isFirstLaunchEver)
      checkIfThisIsAnUpdate(settings);

   // Load Lua stuff
   LuaScriptRunner::setScriptingDir(folderManager->luaDir);    // Get this out of the way, shall we?
   LuaScriptRunner::startLua();                                // Create single "L" instance which all scripts will use


   setupLogging(settings->getIniSettings());    // Turns various logging options on and off

   Ship::computeMaxFireDelay();                 // Look over weapon info and get some ranges, which we'll need before we start sending data

   settings->runCmdLineDirectives();            // If we specified a directive on the cmd line, like -help, attend to that now

   SoundSystem::init(settings->getIniSettings()->sfxSet, folderManager->sfxDir, 
                     folderManager->musicDir, settings->getIniSettings()->getMusicVolLevel());  // Even dedicated server needs sound these days
   


   if(settings->isDedicatedServer())
      initHostGame(settings, settings->getLevelList(), false, true);     // Figure out what levels we'll be playing with, and start hosting  
   else
   {
#ifndef ZAP_DEDICATED
      createClientGame(settings);         // Instantiate ClietGame

      InputCodeManager::resetStates();    // Reset keyboard state mapping to show no keys depressed

      Joystick::loadJoystickPresets(settings);    // Load joystick presets from INI first
      SDL_Init(0);                                // Allows Joystick and VideoSystem to work.
      Joystick::initJoystick(settings);           // Initialize joystick system

#ifdef TNL_OS_MAC_OSX
      // On OS X, make sure we're in the right directory (again)
      moveToAppPath();
#endif

      VideoSystem::init();                // Initialize video and window system


#if SDL_VERSION_ATLEAST(2,0,0)
      SDL_StartTextInput();
#else
      SDL_EnableUNICODE(1);   // Activate unicode ==> http://sdl.beuc.net/sdl.wiki/SDL_EnableUNICODE
      SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);      // SDL_DEFAULT_REPEAT_DELAY defined as 500
#endif

      Zap::Cursor::init();

      BotNavMeshZone::createBotZoneDatabase();

      settings->getIniSettings()->oldDisplayMode = DISPLAY_MODE_UNKNOWN;   // We don't know what the old one was
      VideoSystem::actualizeScreenMode(settings, false, false);            // Create a display window

      gConsole.initialize();     // Initialize console *after* the screen mode has been actualized

      // Now show any error messages from start-up
      Vector<string> configurationErrors = settings->getConfigurationErrors();
      if(configurationErrors.size() > 0)
         for(S32 i = 0; i < gClientGames.size(); i++)
         {
            UIManager *uiManager = gClientGames[i]->getUIManager();
            ErrorMessageUserInterface *ui = uiManager->getErrorMsgUserInterface();

            ui->reset();
            ui->setTitle("CONFIGURATION ERROR");
            for(S32 i = 0; i < configurationErrors.size(); i++)
            {
               string message = itos(i + 1) + ". " + configurationErrors[i];
               ui->setMessage(i + 2, message);
            }

            uiManager->activate(ui);
         }

#endif   // !ZAP_DEDICATED

#if defined(USE_HIDING_CONSOLE) && !defined(TNL_DEBUG)
      // This basically hides the newly created console window only if double-clicked from icon
      // No freeConsole when started from command (cmd) to continues outputting text to console
      if(thisProgramHasCreatedConsoleWindow())
         FreeConsole();
#endif
   }

   dedicatedServerLoop();              // Loop forever, running the idle command endlessly

   return 0;
}

