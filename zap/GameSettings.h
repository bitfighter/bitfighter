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

#ifndef _GAME_SETTINGS_H_
#define _GAME_SETTINGS_H_

#include "BanList.h"
#include"tnlVector.h"
#include <string>

using namespace std;
using namespace TNL;

namespace Zap
{

class GameSettings;

struct ConfigDirectories 
{
   string levelDir;
   string robotDir;
   string sfxDir;
   string musicDir;
   string cacheDir;
   string iniDir;
   string logDir;
   string screenshotDir;
   string luaDir;
   string rootDataDir;

   void resolveDirs(GameSettings *settings);                                   // calls resolveLevelDir()
   void resolveLevelDir();                                                     // calls resolveLevelDir(x,y,z)
   string resolveLevelDir(const string &levelDir, const string &iniLevelDir);  // calls resolveLevelDir(x,y)
   string resolveLevelDir(const string &levelDir);

   string findLevelFile(const string &filename) const;
   string findLevelFile(const string &levelDir, const string &filename) const;
   string findLevelGenScript(const string &fileName) const;
   string findBotFile(const string &filename) const;
};


////////////////////////////////////////
////////////////////////////////////////

class GameSettings
{
private:
   // Some items will be passthroughs to the underlying INI object; however, if a value can differ from the INI setting 
   // (such as when it can be overridden from the cmd line, or is set remotely), then we'll need to store the working value locally.

   string mHostName;                   // Server name used when hosting a game (default set in config.h, set in INI or on cmd line)
   string mHostDescr;                  // Brief description of host

   // Various passwords
   string mServerPassword;
   string mAdminPassword;
   string mLevelChangePassword;

   Vector<string> mLevelSkipList;      // Levels we'll never load, to create a pseudo delete function for remote server mgt
   ConfigDirectories mFolderManager;

   BanList *mBanList;                  // Our ban list

public:
   ~GameSettings();   // Destructor

   void setNewBanList(const string &iniDir) { mBanList = new BanList(iniDir); }

   string getHostName() { return mHostName; }
   void setHostName(const string &hostName, bool updateINI);
   void initHostName(const string &cmdLineVal, const string &iniVal);

   string getHostDescr() { return mHostDescr; }
   void setHostDescr(const string &hostDescr, bool updateINI);
   void initHostDescr(const string &cmdLineVal, const string &iniVal);

   string getServerPassword() { return mServerPassword; }
   void setServerPassword(const string &ServerPassword, bool updateINI);
   void initServerPassword(const string &cmdLineVal, const string &iniVal);

   string getAdminPassword() { return mAdminPassword; }
   void setAdminPassword(const string &AdminPassword, bool updateINI);
   void initAdminPassword(const string &cmdLineVal, const string &iniVal);

   string getLevelChangePassword() { return mLevelChangePassword; }
   void setLevelChangePassword(const string &LevelChangePassword, bool updateINI);
   void initLevelChangePassword(const string &cmdLineVal, const string &iniVal);

   Vector<string> *getLevelSkipList() { return &mLevelSkipList; }
   ConfigDirectories *getConfigDirs() { return &mFolderManager; }
   BanList *getBanList() { return mBanList; }

   string getHostAddress();
   U32 getServerMaxPlayers();
};


};


#endif
