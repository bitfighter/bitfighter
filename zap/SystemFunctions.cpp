//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "SystemFunctions.h"

#include "GameManager.h"
#include "GameSettings.h"
#include "ServerGame.h"
#include "LevelSource.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "UIManager.h"
#  include "UIMenus.h"
#  include "UIErrorMessage.h"
#endif

#include "stringUtils.h"

#ifdef WIN32
#  include <fcntl.h>
#  include <io.h>
#endif

#if defined(TNL_OS_MAC_OSX) || defined(TNL_OS_IOS)
#  include "Directory.h"
#endif

#include "tnlAssert.h"

using namespace TNL;
using namespace std;

namespace Zap
{


// Host a game (and maybe even play a bit, too!)
void initHosting(GameSettingsPtr settings, LevelSourcePtr levelSource, bool testMode, bool dedicatedServer)
{
   TNLAssert(!GameManager::getServerGame(), "Already have a ServerGame!");

   Address address(IPProtocol, Address::Any, GameSettings::DEFAULT_GAME_PORT);   // Equivalent to ("IP:Any:28000")
   address.set(settings->getHostAddress());                          // May overwrite parts of address, depending on what getHostAddress contains

   GameManager::setServerGame(new ServerGame(address, settings, levelSource, testMode, dedicatedServer));

   GameManager::getServerGame()->setReadyToConnectToMaster(true);
   Game::seedRandomNumberGenerator(settings->getHostName());

   // Don't need to build our level list when in test mode because we're only running that one level stored in editor.tmp
   if(!testMode)
   {
      logprintf(LogConsumer::ServerFilter, "----------\n"
                                           "Bitfighter server started [%s]", getTimeStamp().c_str());
      logprintf(LogConsumer::ServerFilter, "hostname=[%s], hostdescr=[%s]", settings->getHostName().c_str(), 
                                                                            settings->getHostDescr().c_str());

      logprintf(LogConsumer::ServerFilter, "Loaded %d levels:", levelSource->getLevelCount());
   }

   if(levelSource->getLevelCount() == 0)     // No levels!
   {
      abortHosting_noLevels(GameManager::getServerGame());
      GameManager::deleteServerGame();
      return;
   }

   GameManager::getServerGame()->resetLevelLoadIndex();

   GameManager::setHostingModePhase(GameManager::LoadingLevels);

#ifndef ZAP_DEDICATED
   const Vector<ClientGame *> *clientGames = GameManager::getClientGames();

   for(S32 i = 0; i < clientGames->size(); i++)
      clientGames->get(i)->getUIManager()->enableLevelLoadDisplay();
#endif
}


void shutdownBitfighter();    // Forward declaration

// If we can't load any levels, here's the plan...
void abortHosting_noLevels(ServerGame *serverGame)
{
   if(serverGame->isDedicated())  
   {
      FolderManager *folderManager = serverGame->getSettings()->getFolderManager();
      const char *levelDir = folderManager->getLevelDir().c_str();

      logprintf(LogConsumer::LogError,     "No levels found in folder %s.  Cannot host a game.", levelDir);
      logprintf(LogConsumer::ServerFilter, "No levels found in folder %s.  Cannot host a game.", levelDir);
   }


#ifndef ZAP_DEDICATED
   const Vector<ClientGame *> *clientGames = GameManager::getClientGames();

   for(S32 i = 0; i < clientGames->size(); i++)    // <<=== Should probably only display this message on the clientGame that initiated hosting
   {
      UIManager *uiManager = clientGames->get(i)->getUIManager();

      ErrorMessageUserInterface *errUI = uiManager->getUI<ErrorMessageUserInterface>();

      FolderManager *folderManager = serverGame->getSettings()->getFolderManager();
      string levelDir = folderManager->getLevelDir();

      errUI->reset();
      errUI->setTitle("HOUSTON, WE HAVE A PROBLEM");
      errUI->setMessage("No levels were loaded.  Cannot host a game.  "
                        "Check the LevelDir parameter in your INI file, "
                        "or your command-line parameters to make sure "
                        "you have correctly specified a folder containing "
                        "valid level files.\n\n"
                        "Trying to load levels from folder:\n" +
                     (levelDir == "" ? string("<<Unresolvable>>") : levelDir));

      errUI->setInstr("Press [[Esc]] to continue");

      uiManager->activate<ErrorMessageUserInterface>();
      uiManager->disableLevelLoadDisplay(false); 
   }

   if(clientGames->size() == 0)
#endif
      shutdownBitfighter();      // Quit in an orderly fashion
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


// This function returns the path where the game resources have been installed into the system
// before being copied to the user's data path
string getInstalledDataDir()
{
   string path;

#if defined(TNL_OS_LINUX)
   // In Linux, the data dir can be anywhere!  Usually in something like /usr/share/bitfighter
   // or /usr/local/share/bitfighter.  Ignore if compiling DEBUG
#if defined(LINUX_DATA_DIR) && !defined(TNL_DEBUG)
   path = string(LINUX_DATA_DIR) + "/bitfighter";
#else
   // We'll default to the directory the executable is in
   path = getExecutableDir();
#endif

#elif defined(TNL_OS_MAC_OSX) || defined(TNL_OS_IOS)
   getAppResourcePath(path);  // Directory.h

#elif defined(TNL_OS_WIN32)
   // On Windows, the installed data dir is always where the executable is
   path = getExecutableDir();

#else
#  error "Path needs to be defined for this platform"
#endif

   return path;
}


}

