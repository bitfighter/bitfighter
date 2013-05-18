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

#include "SystemFunctions.h"

#include "GameSettings.h"
#include "ServerGame.h"

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

#include "tnlAssert.h"

using namespace TNL;
using namespace std;

namespace Zap
{


extern ServerGame *gServerGame;
extern Vector<ClientGame *> gClientGames;

// Host a game (and maybe even play a bit, too!)
void initHosting(GameSettings *settings, const Vector<string> &levelList, bool testMode, bool dedicatedServer)
{
   TNLAssert(!gServerGame, "already exists!");
   if(gServerGame)
   {
      delete gServerGame;
      gServerGame = NULL;
   }

   Address address(IPProtocol, Address::Any, GameSettings::DEFAULT_GAME_PORT);   // Equivalent to ("IP:Any:28000")
   address.set(settings->getHostAddress());                          // May overwrite parts of address, depending on what getHostAddress contains

   gServerGame = new ServerGame(address, settings, testMode, dedicatedServer);

   gServerGame->setReadyToConnectToMaster(true);
   Game::seedRandomNumberGenerator(settings->getHostName());

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
         gClientGames[i]->getUIManager()->enableLevelLoadDisplay();
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

      ErrorMessageUserInterface *errUI = uiManager->getUI<ErrorMessageUserInterface>();

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

      uiManager->activate<ErrorMessageUserInterface>();
      uiManager->disableLevelLoadDisplay(false); 
   }
#endif

   delete gServerGame;  // need gServerGame for above message
   gServerGame = NULL;

#ifndef ZAP_DEDICATED
   if(gClientGames.size() == 0)
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

