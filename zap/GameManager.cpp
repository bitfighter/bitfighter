//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "GameManager.h"

#include "DisplayManager.h"
#include "FontManager.h"
#include "RenderManager.h"
#include "ServerGame.h"
#include "SoundSystem.h"
#include "VideoSystem.h"
#include "Console.h"

#ifndef ZAP_DEDICATED
#  include "UIErrorMessage.h"
#  include "UIManager.h"
#  include "ClientGame.h"
#  include "ClientInfo.h"
#endif

#ifndef BF_NO_CONSOLE
#  include "ConsoleLogConsumer.h"
#endif

namespace Zap
{

// Declare statics
ServerGame *GameManager::mServerGame = NULL;
#ifndef ZAP_DEDICATED
   Vector<ClientGame *> GameManager::mClientGames;
#endif
GameManager::HostingModePhase GameManager::mHostingModePhase = GameManager::NotHosting;
Console *GameManager::gameConsole = NULL;    // For the moment, we'll just have one console for everything.  This may change later, but probably won't.

static ConsoleLogConsumer *ConsoleLog;

// Constructor
GameManager::GameManager()
{
   gameConsole = new Console();
}


// Destructor
GameManager::~GameManager()
{
   delete gameConsole;
   gameConsole = NULL;

   delete ConsoleLog;
}


void GameManager::initialize()
{
#ifndef BF_NO_CONSOLE
   gameConsole->initialize();
   ConsoleLog = new ConsoleLogConsumer(gameConsole);  // Logs to our in-game console, when available
#endif

}


ServerGame *GameManager::getServerGame()
{
   return mServerGame;
}


void GameManager::setServerGame(ServerGame *serverGame)
{
   TNLAssert(serverGame, "Expect a valid serverGame here!");
   TNLAssert(!mServerGame, "Already have a ServerGame!");

   mServerGame = serverGame;
}


void GameManager::deleteServerGame()
{
   // mServerGame might be NULL here; for example when quitting after losing a connection to the game server
   delete mServerGame;     // Kill the serverGame (leaving the clients running)
   mServerGame = NULL;
}


void GameManager::idleServerGame(U32 timeDelta)
{
   if(mServerGame)
      mServerGame->idle(timeDelta);
}


/////

#ifndef ZAP_DEDICATED
const Vector<ClientGame *> *GameManager::getClientGames()
{
   return &mClientGames;
}


void GameManager::deleteClientGames()
{
   mClientGames.deleteAndClear();
}


void GameManager::deleteClientGame(S32 index)
{
   mClientGames.deleteAndErase(index);
}


void GameManager::addClientGame(ClientGame *clientGame)
{
   mClientGames.push_back(clientGame);
}
#endif


void GameManager::idleClientGames(U32 timeDelta)
{
#ifndef ZAP_DEDICATED
   for(S32 i = 0; i < mClientGames.size(); i++)
      mClientGames[i]->idle(timeDelta);
#endif
}

void GameManager::idle(U32 timeDelta)
{
   idleServerGame(timeDelta);
   idleClientGames(timeDelta);
}


void GameManager::setHostingModePhase(HostingModePhase phase)
{
   mHostingModePhase = phase;
}


GameManager::HostingModePhase GameManager::getHostingModePhase()
{
   return mHostingModePhase;
}


extern void exitToOs();

// Run when we're quitting the game, returning to the OS.  Saves settings and does some final cleanup to keep things orderly.
// There are currently only 6 ways to get here (i.e. 6 legitimate ways to exit Bitfighter): 
// 1) Hit escape during initial name entry screen
// 2) Hit escape from the main menu
// 3) Choose Quit from main menu
// 4) Host a game with no levels as a dedicated server
// 5) Admin issues a shutdown command to a remote dedicated server
// 6) Click the X on the window to close the game window   <=== NOTE: This scenario fails for me when running a dedicated server on windows.
// and two illigitimate ways
// 7) Lua panics!!
// 8) Video system fails to initialize
void GameManager::shutdownBitfighter()
{
   GameSettings *settings = NULL;

   // Avoid this function being called twice when we exit via methods 1-4 above
#ifndef ZAP_DEDICATED
   if(GameManager::getClientGames()->size() == 0)
#endif
      if(GameManager::getServerGame())
         exitToOs();

   // Grab a pointer to settings wherever we can.  Note that all Games (client or server) currently refer to the same settings object.
#ifndef ZAP_DEDICATED
   if(GameManager::getClientGames()->size() > 0)
      settings = GameManager::getClientGames()->get(0)->getSettings();

   GameManager::deleteClientGames();

#endif

   if(GameManager::getServerGame())
   {
      settings = GameManager::getServerGame()->getSettings();
      GameManager::deleteServerGame();
   }


   TNLAssert(settings, "Should always have a value here!");

   EventManager::shutdown();
   LuaScriptRunner::shutdown();
   SoundSystem::shutdown();

   if(!settings->isDedicatedServer())
   {
#ifndef ZAP_DEDICATED
      Joystick::shutdownJoystick();

      // Save current window position if in windowed mode
      if(settings->getSetting<DisplayMode>(IniKey::WindowMode) == DISPLAY_MODE_WINDOWED)
         settings->setWindowPosition(VideoSystem::getWindowPositionX(), VideoSystem::getWindowPositionY());

      SDL_QuitSubSystem(SDL_INIT_VIDEO);

      FontManager::cleanup();
      RenderManager::shutdown();
#endif
   }

#ifndef BF_NO_CONSOLE
   // Avoids annoying shutdown crashes when logging is still trying to output to oglconsole
   ConsoleLog->setMsgTypes(LogConsumer::LogNone);
#endif

   settings->save();                                  // Write settings to bitfighter.ini

   delete settings;

   DisplayManager::cleanup();

   NetClassRep::logBitUsage();
   logprintf("Bye!");

   exitToOs();    // Do not pass Go
}



} 
