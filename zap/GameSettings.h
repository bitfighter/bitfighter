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

#include "config.h"
#include "tnlVector.h"
#include <string>

using namespace std;
using namespace TNL;

namespace CmdLineParams
{

enum ParamId {
   LOGIN_NAME,
   LOGIN_PASSWORD,
   WINDOW_MODE,
   FULLSCREEN_MODE,
   FULLSCREEN_STRETCH,
   WINDOW_POS,
   WINDOW_WIDTH,
   USE_STICK,
   MASTER_ADDRESS,

   DEDICATED,
   SERVER_PASSWORD,
   ADMIN_PASSWORD,
   LEVEL_CHANGE_PASSWORD,
   HOST_NAME,
   HOST_DESCRIPTION,
   MAX_PLAYERS_PARAM,
   HOST_ADDRESS,

   LEVEL_LIST,

   ROOT_DATA_DIR,
   LEVEL_DIR,
   INI_DIR,
   LOG_DIR,
   SCRIPTS_DIR,
   CACHE_DIR,
   ROBOT_DIR,
   SCREENSHOT_DIR,
   SFX_DIR,
   MUSIC_DIR,

   SIMULATED_LOSS,
   SIMULATED_LAG,
   SIMULATED_STUTTER,
   FORCE_UPDATE,

   SEND_RESOURCE,
   GET_RESOURCE,
   SHOW_RULES,
   HELP,

   PARAM_COUNT
};

};


using namespace CmdLineParams;

namespace Zap
{

class GameSettings;
class BanList;

////////////////////////////////////////
////////////////////////////////////////


enum SettingSource {
   INI,
   CMD_LINE,
   DEFAULT
};



class GameSettings
{
private:
   // Some items will be passthroughs to the underlying INI object; however, if a value can differ from the INI setting 
   // (such as when it can be overridden from the cmd line, or is set remotely), then we'll need to store the working value locally.

   string mHostName;                   // Server name used when hosting a game (default set in config.h, set in INI or on cmd line)
   string mHostDescr;                  // Brief description of host

   string mPlayerName, mPlayerPassword;
   bool mPlayerNameSpecifiedOnCmdLine;

   // Various passwords
   string mServerPassword;
   string mAdminPassword;
   string mLevelChangePassword;

   Vector<string> mLevelSkipList;      // Levels we'll never load, to create a pseudo delete function for remote server mgt
   FolderManager mFolderManager;

   BanList *mBanList;                  // Our ban list

   //CmdLineSettings mCmdLineSettings;
   IniSettings mIniSettings;

   // Store params read from the cmd line
   Vector<string> mCmdLineParams[CmdLineParams::PARAM_COUNT];

   Vector<string> mMasterServerList;
   bool mMasterServerSpecifiedOnCmdLine;

   // Helper functions:
   // This first lot return the first value following the cmd line parameter cast to various types
   string getString(ParamId paramId);
   U32 getU32(ParamId paramId);
   F32 getF32(ParamId paramId);

   bool getSpecified(ParamId paramId);                // Returns true if parameter was present, false if not

   DisplayMode resolveCmdLineSpecifiedDisplayMode();  // Tries to figure out what display mode was specified on the cmd line, if any

   Vector<Vector<U32> > mLoadoutPresets;

public:
   GameSettings();    // Constructor
   ~GameSettings();   // Destructor

   void readCmdLineParams(const Vector<string> &argv);
   void resolveDirs();

   string getHostName() { return mHostName; }
   void setHostName(const string &hostName, bool updateINI);

   string getHostDescr() { return mHostDescr; }
   void setHostDescr(const string &hostDescr, bool updateINI);

   string getServerPassword() { return mServerPassword; }
   void setServerPassword(const string &ServerPassword, bool updateINI);

   string getAdminPassword() { return mAdminPassword; }
   void setAdminPassword(const string &AdminPassword, bool updateINI);

   string getLevelChangePassword() { return mLevelChangePassword; }
   void setLevelChangePassword(const string &LevelChangePassword, bool updateINI);

   Vector<string> *getLevelSkipList() { return &mLevelSkipList; }
   Vector<string> *getSpecifiedLevels() { return &mCmdLineParams[CmdLineParams::LEVEL_LIST]; }

   // Variations on generating a list of levels
   Vector<string> getLevelList();                            // Generic, grab a list of levels based on current settings
   Vector<string> getLevelList(const string &levelFolder);   // Grab a list of levels from the specified level folder; ignore anything in the INI
private:
   Vector<string> getLevelList(const string &levelDir, bool ignoreCmdLine);    // Workhorse for above methods

public:

   Vector<string> *getMasterServerList() { return &mMasterServerList; }
   void saveMasterAddressListInIniUnlessItCameFromCmdLine();
   
   FolderManager *getFolderManager() { return &mFolderManager; }
   FolderManager getCmdLineFolderManager();    // Return a FolderManager struct populated with settings specified on cmd line

   BanList *getBanList() { return mBanList; }

   string getHostAddress();
   U32 getMaxPlayers();

   void save();

   IniSettings *getIniSettings() { return &mIniSettings; }

   void runCmdLineDirectives();

   bool shouldShowNameEntryScreenOnStartup();

   const Color *getWallFillColor() { return &mIniSettings.wallFillColor; }
   const Color *getWallOutlineColor() { return &mIniSettings.wallOutlineColor; }

   bool getStarsInDistance() { return mIniSettings.starsInDistance; }
   bool getEnableExperimentalAimMode() { return mIniSettings.enableExperimentalAimMode; }


   // Accessor methods
   U32 getSimulatedStutter() { return getU32(SIMULATED_STUTTER); }
   F32 getSimulatedLoss()    { return getF32(SIMULATED_LOSS); }
   U32 getSimulatedLag();

   string getDefaultName() { return mIniSettings.defaultName; }

   bool getForceUpdate()  { return getSpecified(FORCE_UPDATE); }

   string getPlayerName() { return mPlayerName; }

   void setPlayerName(const string &name, bool nameSuppliedByUser);
   void setPlayerNameAndPassword(const string &name, const string &password);

   string getPlayerPassword() { return mPlayerPassword; }

   bool isDedicatedServer() { return getSpecified(DEDICATED); }

   string getLevelDir(SettingSource source);


   bool getLoadoutPreset(S32 index, Vector<U32> &preset);
   void setLoadoutPreset(S32 index, const Vector<U32> &preset);

   // Other methods
   void saveLevelChangePassword(const string &serverName, const string &password);
   void saveAdminPassword(const string &serverName, const string &password);

   void forgetLevelChangePassword(const string &serverName);
   void forgetAdminPassword(const string &serverName);

   void onFinishedLoading();     // Should be run after INI and cmd line params have been read

   static void getRes(GameSettings *settings, const Vector<string> &words);
   static void sendRes(GameSettings *settings, const Vector<string> &words);
   static void showRules(GameSettings *settings, const Vector<string> &words);
   static void showHelp(GameSettings *settings, const Vector<string> &words);
};


};


#endif
