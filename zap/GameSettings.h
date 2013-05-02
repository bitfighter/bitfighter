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
#include "InputCode.h"        // For InputCodeManager def
#include "LoadoutTracker.h"

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
   NO_MUSIC,
   MASTER_ADDRESS,

   DEDICATED,
   SERVER_PASSWORD,
   OWNER_PASSWORD,
   ADMIN_PASSWORD,
   NO_ADMIN_PASSWORD,
   LEVEL_CHANGE_PASSWORD,
   NO_LEVEL_CHANGE_PASSWORD,
   HOST_NAME,
   HOST_DESCRIPTION,
   MAX_PLAYERS_PARAM,
   HOST_ADDRESS,

   LEVEL_LIST,

   ROOT_DATA_DIR,
   PLUGIN_DIR,
   LEVEL_DIR,
   INI_DIR,
   LOG_DIR,
   SCRIPTS_DIR,
   ROBOT_DIR,
   SCREENSHOT_DIR,
   SFX_DIR,
   MUSIC_DIR,
   FONTS_DIR,

   SIMULATED_LOSS,
   SIMULATED_LAG,
   SIMULATED_STUTTER,
   FORCE_UPDATE,

   SEND_RESOURCE,
   GET_RESOURCE,
   SHOW_RULES,
   SHOW_LUA_CLASSES,
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


struct PluginBinding;

class GameSettings
{
private:
   // Some items will be passthroughs to the underlying INI object; however, if a value can differ from the INI setting 
   // (such as when it can be overridden from the cmd line, or is set remotely), then we'll need to store the working value locally.

   string mHostName;                   // Server name used when hosting a game (default set in config.h, set in INI or on cmd line)
   string mHostDescr;                  // Brief description of host

   string mPlayerName, mPlayerPassword;   // Resolved name/password, either from INI for cmdLine or login screen
   bool mPlayerNameSpecifiedOnCmdLine;

   // Various passwords
   string mServerPassword;
   string mOwnerPassword;
   string mAdminPassword;
   string mLevelChangePassword;

   Vector<string> mLevelSkipList;      // Levels we'll never load, to create a pseudo delete function for remote server mgt  <=== does this ever get loaded???
   static FolderManager *mFolderManager;
   InputCodeManager mInputCodeManager;

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

   DisplayMode resolveCmdLineSpecifiedDisplayMode();  // Tries to figure out what display mode was specified on the cmd line, if any
      
   Vector<LoadoutTracker> mLoadoutPresets;

   Vector<string> mConfigurationErrors;

public:
   GameSettings();            // Constructor
   virtual ~GameSettings();   // Destructor

   void readCmdLineParams(const Vector<string> &argv);
   void resolveDirs();

   string getHostName();
   void setHostName(const string &hostName, bool updateINI);

   string getHostDescr();
   void setHostDescr(const string &hostDescr, bool updateINI);

   string getServerPassword();
   void setServerPassword(const string &ServerPassword, bool updateINI);

   string getOwnerPassword();
   void setOwnerPassword(const string &OwnerPassword, bool updateINI);

   string getAdminPassword();
   void setAdminPassword(const string &AdminPassword, bool updateINI);

   string getLevelChangePassword();
   void setLevelChangePassword(const string &LevelChangePassword, bool updateINI);

   const Vector<PluginBinding> *getPluginBindings();

   InputCodeManager *getInputCodeManager(); 

   Vector<string> *getLevelSkipList();
   Vector<string> *getSpecifiedLevels();

   bool getSpecified(ParamId paramId);                      // Returns true if parameter was present, false if not

   // Variations on generating a list of levels
   Vector<string> getLevelList();                            // Generic, grab a list of levels based on current settings
   Vector<string> getLevelList(const string &levelFolder);   // Grab a list of levels from the specified level folder; ignore anything in the INI
private:
   Vector<string> getLevelList(const string &levelDir, bool ignoreCmdLine);    // Workhorse for above methods

public:
   static S32 UseJoystickNumber;

   Vector<string> *getMasterServerList();
   void saveMasterAddressListInIniUnlessItCameFromCmdLine();
   
   static FolderManager *getFolderManager();
   FolderManager getCmdLineFolderManager();    // Return a FolderManager struct populated with settings specified on cmd line

   BanList *getBanList();

   string getHostAddress();
   U32 getMaxPlayers();

   void save();

   IniSettings *getIniSettings();

   void runCmdLineDirectives();

   bool shouldShowNameEntryScreenOnStartup();

   const Color *getWallFillColor() const;
   const Color *getWallOutlineColor() const;

   // Accessor methods
   U32 getSimulatedStutter();
   F32 getSimulatedLoss();
   U32 getSimulatedLag();

   string getDefaultName();

   bool getForceUpdate();

   string getPlayerName();

   void updatePlayerName(const string &name);
   void setLoginCredentials(const string &name, const string &password, bool save);

   void setAutologin(bool autologin);

   string getPlayerPassword();

   bool isDedicatedServer();

   string getLevelDir(SettingSource source);

   LoadoutTracker getLoadoutPreset(S32 index);
   void setLoadoutPreset(const LoadoutTracker *preset, S32 index);

   void addConfigurationError(const string &errorMessage);
   Vector<string> getConfigurationErrors();

   // Other methods
   void saveLevelChangePassword(const string &serverName, const string &password);
   void saveAdminPassword(const string &serverName, const string &password);
   void saveOwnerPassword(const string &serverName, const string &password);

   void forgetLevelChangePassword(const string &serverName);
   void forgetAdminPassword(const string &serverName);
   void forgetOwnerPassword(const string &serverName);

   void onFinishedLoading();     // Should be run after INI and cmd line params have been read

   static void getRes(GameSettings *settings, const Vector<string> &words);
   static void sendRes(GameSettings *settings, const Vector<string> &words);
   static void showRules(GameSettings *settings, const Vector<string> &words);
   static void showHelp(GameSettings *settings, const Vector<string> &words);

   static Vector<string> DetectedJoystickNameList;   // List of joysticks we found attached to this machine

};


};


#endif
