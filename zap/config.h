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
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _CONFIG_H_
#define _CONFIG_H_

// This file contains definitions of two structs that are used to store our
// INI settings and command line param settings, which are read separately,
// but processed jointly.  Some default values are provided here as well,
// especially for the INI settings...  if the INI is deleted, these defaults
// will be used to rebuild it.

#include "tnlTypes.h"
#include "tnlNetStringTable.h"
#include "tnlVector.h"
#include "Color.h"      // For Color def

#include "input.h"
#include <string>

using namespace std;
using namespace TNL;


namespace Zap
{

enum sfxSets {
   sfxClassicSet,
   sfxModernSet
};

enum DisplayMode {
   DISPLAY_MODE_WINDOWED,
   DISPLAY_MODE_FULL_SCREEN_STRETCHED,
   DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED,
   DISPLAY_MODE_UNKNOWN
};


struct ConfigDirectories {
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

   static void resolveDirs();                                                                                    // calls resolveLevelDir()
   static void resolveLevelDir();                                                                                // calls resolveLevelDir(x,y,z)
   static string resolveLevelDir(const string &rootDataDir, const string &levelDir, const string &iniLevelDir);  // calls resolveLevelDir(x,y)
   static string resolveLevelDir(const string &rootDataDir, const string &levelDir);

   static string findLevelFile(const string &levelDir, const string &filename);
   static string findLevelGenScript(const string &fileName);
   static string findBotFile(const string &filename);
};


struct CmdLineSettings
{
   CmdLineSettings() { init(); }    // Quickie constructor
   void readParams(const Vector<string> &argv);

   bool clientMode;        // Start with client active
   bool serverMode;        // Start in server mode
   bool dedicatedMode;     // Will server be dedicated?

   string server;
   string masterAddress;   // Use this master server
   F32 loss;               // Simulate packet loss (0-1)
   U32 lag;                // Simulate server lag (in ms)
   bool forceUpdate;       // For testing updater
   string dedicated;       // Holds bind address specified on cmd line following -dedicated parameter
   string name;
   string password;

   string defaultName;     // Name used if user hits <enter> on name entry screen
   string lastName;        // Name the user previously entered
   string lastPassword;    // Last password user entered on startup screen
   string lastEditorName;  // Name of most recently edited level

   string hostname;
   string hostaddr;        // Address to listen on when we're host (e.g. IP:localhost:1234 or IP:Any:6666 or whatever)
   string hostdescr;       // One-line description of server
   string serverPassword;  // Password required to connect to server
   string adminPassword;   // Password required to perform certain admin functions
   string levelChangePassword;   // Password required to change levels and such

   ConfigDirectories dirs;

   S32 maxPlayers;

   DisplayMode displayMode;    // Fullscreen param supplied

   S32 winWidth;
   S32 xpos;
   S32 ypos;

   Vector<string> specifiedLevels;

   void init()
   {
      clientMode = true;
      serverMode = false;
      dedicatedMode = false;

      loss = 0;
      lag = 0;
      forceUpdate = false;
      maxPlayers = -1;
      displayMode = DISPLAY_MODE_UNKNOWN;
      winWidth = -1;
      xpos = -9999;
      ypos = -9999;
   };
};


struct IniSettings      // With defaults specified
{
   bool controlsRelative;
   DisplayMode displayMode;
   DisplayMode oldDisplayMode;
   U32 joystickType;
   bool echoVoice;

   F32 sfxVolLevel;                 // SFX volume (0 = silent, 1 = full bore)
   F32 musicVolLevel;               // As above
   F32 voiceChatVolLevel;           // Ditto
   F32 alertsVolLevel;              // And again

   sfxSets sfxSet;                  // Which set of SFX does the user want?

   bool starsInDistance;            // True if stars move in distance behind maze, false if they are in fixed location
   bool useLineSmoothing;           // Turn on anti-aliasing
   bool diagnosticKeyDumpMode;      // True if want to dump keystrokes to the screen

   bool showWeaponIndicators;       // True if we show the weapon indicators on the top of the screen
   bool verboseHelpMessages;        // If true, we'll show more handholding messages
   bool showKeyboardKeys;           // True if we show the keyboard shortcuts in joystick mode

   bool enableExperimentalAimMode;  // True if we want to show an aim vector in joystick mode

   bool allowGetMap;                // allow '/GetMap' command
   bool allowDataConnections;       // Specify whether data connections are allowed on this computer

   U32 maxDedicatedFPS;
   U32 maxFPS;

   InputMode inputMode;             // Joystick or Keyboard
   string masterAddress;            // Default address of our master server
   string name;                     // Player name (none by default)
   string password;                 // Player pasword (none by default)
   string defaultName;              // Name used if user hits <enter> on name entry screen
   string lastName;                 // Name user entered last time the game was run -- will be used as default on name entry screen
   string lastPassword;
   string lastEditorName;           // Name of file most recently edited by the user

   string hostname;                 // Server name when in host mode
   string hostaddr;                 // User-specified address/port of server
   string hostdescr;                // One-line description of server
   string serverPassword;
   string adminPassword;
   string levelChangePassword;      // Password to allow access to level changing functionality on non-local server
   string levelDir;                 // Folder where levels are stored, by default
   S32 maxPlayers;                  // Max number of players that can play on local server
   S32 maxBots;

   bool allowMapUpload;
   bool allowAdminMapUpload;

   bool voteEnable;
   U32 voteLength;
   U32 voteLengthToChangeTeam;
   U32 voteRetryLength;
   S32 voteYesStrength;
   S32 voteNoStrength;
   S32 voteNothingStrength;

   bool useUpdater;                 // Use updater system (windows only)

   // Game window location when in windowed mode
   S32 winXPos;
   S32 winYPos;
   F32 winSizeFact;

   // Testing values
   S32 burstGraphicsMode;           // Choose a burst graphic representation
   bool neverConnectDirect;
   Color wallFillColor;
   Color wallOutlineColor;

   // Logging options   --   true will enable logging these events, false will disable
   bool logConnectionProtocol;
   bool logNetConnection;
   bool logEventConnection;
   bool logGhostConnection;
   bool logNetInterface;
   bool logPlatform;
   bool logNetBase;
   bool logUDP;

   bool logFatalError;        
   bool logError;             
   bool logWarning;           
   bool logConnection;        
   bool logLevelLoaded;       
   bool logLuaObjectLifecycle;
   bool luaLevelGenerator;    
   bool luaBotMessage;        
   bool serverFilter;  
   bool logStats;

   string mySqlStatsDatabaseServer;
   string mySqlStatsDatabaseName;
   string mySqlStatsDatabaseUser;
   string mySqlStatsDatabasePassword;

   string defaultRobotScript;
   string globalLevelScript;
   string getInputMode();        // Returns display-friendly mode designator like "Keyboard" or "Joystick 1"

   Vector<StringTableEntry> levelList;

   Vector<string> reservedNames;
   Vector<string> reservedPWs;

   U32 version;

   bool oldGoalFlash;

   Vector<string> prevServerListFromMaster;
   Vector<string> alwaysPingList;

   // Set default values in config.cpp, IniSettings::init()
   void init();
};

extern IniSettings gIniSettings;
class CIniFile;

void saveSettingsToINI(CIniFile *ini);
void loadSettingsFromINI(CIniFile *ini);

void writeSkipList(CIniFile *ini);

};

#endif


