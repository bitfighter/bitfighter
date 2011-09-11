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

#include "input.h"
#include "IniFile.h"
#include "config.h"
#include "gameLoader.h"    // For LevelListLoader::levelList
#include "version.h"
#include "stringUtils.h"
#include "Joystick.h"
#include "keyCode.h"

#include "GameSettings.h"

#ifndef ZAP_DEDICATED
#include "quickChatHelper.h"
#endif

#include "dataConnection.h"   // For defs of stuff used by transferResource() function below

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#ifdef TNL_OS_WIN32
#include <windows.h>   // For ARRAYSIZE when using ZAP_DEDICATED
#endif

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif


namespace Zap
{

// Set default values here
void IniSettings::init()
{
   controlsRelative = false;          // Relative controls is lame!
   displayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;
   oldDisplayMode = DISPLAY_MODE_UNKNOWN;
   joystickType = NoController;
   echoVoice = false;

   sfxVolLevel = 1.0;                 // SFX volume (0 = silent, 1 = full bore)
   musicVolLevel = 1.0;               // Music volume (range as above)
   voiceChatVolLevel = 1.0;           // INcoming voice chat volume (range as above)
   alertsVolLevel = 1.0;              // Audio alerts volume (when in dedicated server mode only, range as above)

   sfxSet = sfxModernSet;             // Start off with our modern sounds

   starsInDistance = true;            // True if stars move in distance behind maze, false if they are in fixed location
   useLineSmoothing = true;          // Enable/disable anti-aliasing
   diagnosticKeyDumpMode = false;     // True if want to dump keystrokes to the screen
   enableExperimentalAimMode = false; // True if we want to show experimental aiming vector in joystick mode

   showWeaponIndicators = true;       // True if we show the weapon indicators on the top of the screen
   verboseHelpMessages = true;        // If true, we'll show more handholding messages
   showKeyboardKeys = true;           // True if we show the keyboard shortcuts in joystick mode
   allowDataConnections = false;      // Disabled unless explicitly enabled for security reasons -- most users won't need this
   allowGetMap = false;               // Disabled by default -- many admins won't want this

   maxDedicatedFPS = 100;             // Max FPS on dedicated server
   maxFPS = 100;                      // Max FPS on client/non-dedicated server

   inputMode = InputModeKeyboard;     // Joystick or Keyboard
   masterAddress = "IP:67.18.11.66:25955";   // Default address of our master server
   name = "";                         // Player name (none by default)
   defaultName = "ChumpChange";       // Name used if user hits <enter> on name entry screen
   lastName = "ChumpChange";          // Name the user entered last time they ran the game
   lastPassword = "";
   lastEditorName = "";               // No default editor level name
   hostname = "Bitfighter host";      // Default host name
   hostdescr = "";
   maxPlayers = 127;
   maxBots = 10;
   serverPassword = "";               // Passwords empty by default
   adminPassword = "";
   levelChangePassword = "";
   levelDir = "";

   defaultRobotScript = "s_bot.bot";            
   globalLevelScript = "";

   wallFillColor.set(0,0,.15);
   wallOutlineColor.set(0,0,1);

   allowMapUpload = false;
   allowAdminMapUpload = true;

   voteEnable = false; // disable by default.
   voteLength = 12;
   voteLengthToChangeTeam = 10;
   voteRetryLength = 30;
   voteYesStrength = 3;
   voteNoStrength = -3;
   voteNothingStrength = -1;

   useUpdater = true;

   // Game window location when in windowed mode
   winXPos = 100;
   winYPos = 100;
   winSizeFact = 1.0;

   burstGraphicsMode = 1;
   neverConnectDirect = false;

   // Specify which events to log
   logConnectionProtocol = false;
   logNetConnection = false;
   logEventConnection = false;
   logGhostConnection = false;
   logNetInterface = false;
   logPlatform = false;
   logNetBase = false;
   logUDP = false;

   logFatalError = true;       
   logError = true;            
   logWarning = true;          
   logConnection = true;       
   logLevelLoaded = true;      
   logLuaObjectLifecycle = false;
   luaLevelGenerator = true;   
   luaBotMessage = true;       
   serverFilter = false; 

   logStats = true;            // Log statistics into ServerFilter log files

   version = 0;

   oldGoalFlash = false;
}


extern string lcase(string strToConvert);

// Sorts alphanumerically
extern S32 QSORT_CALLBACK alphaSort(string *a, string *b);

static void loadForeignServerInfo(CIniFile *ini)
{
   // AlwaysPingList will default to broadcast, can modify the list in the INI
   // http://learn-networking.com/network-design/how-a-broadcast-address-works
   parseString(ini->GetValue("Connections", "AlwaysPingList", "IP:Broadcast:28000").c_str(), gIniSettings.alwaysPingList, ',');

   // These are the servers we found last time we were able to contact the master.
   // In case the master server fails, we can use this list to try to find some game servers. 
   //parseString(ini->GetValue("ForeignServers", "ForeignServerList").c_str(), prevServerListFromMaster, ',');
   ini->GetAllValues("RecentForeignServers", gIniSettings.prevServerListFromMaster);
}

static void writeConnectionsInfo(CIniFile *ini)
{
   if(ini->numSectionComments("Connections") == 0)
   {
      ini->sectionComment("Connections", "----------------");
      ini->sectionComment("Connections", " AlwaysPingList - Always try to contact these servers (comma separated list); Format: IP:IPAddress:Port");
      ini->sectionComment("Connections", "                  Include 'IP:Broadcast:28000' to search LAN for local servers on default port");
      ini->sectionComment("Connections", "----------------");
   }

   // Creates comma delimited list
   ini->SetValue("Connections", "AlwaysPingList", listToString(gIniSettings.alwaysPingList, ','));
}


static void writeForeignServerInfo(CIniFile *ini)
{
   if(ini->numSectionComments("RecentForeignServers") == 0)
   {
   
      ini->sectionComment("RecentForeignServers", "----------------");
      ini->sectionComment("RecentForeignServers", " This section contains a list of the most recent servers seen; used as a fallback if we can't reach the master");
      ini->sectionComment("RecentForeignServers", " Please be aware that this section will be automatically regenerated, and any changes you make will be overwritten");
      ini->sectionComment("RecentForeignServers", "----------------");
   }

   ini->SetAllValues("RecentForeignServers", "Server", gIniSettings.prevServerListFromMaster);
}


// Read levels, if there are any...
 void loadLevels(CIniFile *ini)
{
   if(ini->findSection("Levels") != ini->noID)
   {
      S32 numLevels = ini->NumValues("Levels");
      Vector<string> levelValNames;

      for(S32 i = 0; i < numLevels; i++)
         levelValNames.push_back(ini->ValueName("Levels", i));

      levelValNames.sort(alphaSort);

      string level;
      for(S32 i = 0; i < numLevels; i++)
      {
         level = ini->GetValue("Levels", levelValNames[i], "");
         if (level != "")
            gIniSettings.levelList.push_back(StringTableEntry(level.c_str()));
      }
   }
}


// Read level deleteList, if there are any.  This could probably be made more efficient by not reading the
// valnames in first, but what the heck...
void loadLevelSkipList(CIniFile *ini, GameSettings *settings)
{
   ini->GetAllValues("LevelSkipList", *settings->getLevelSkipList());
}


// Convert a string value to a DisplayMode enum value
static DisplayMode stringToDisplayMode(string mode)
{
   if(lcase(mode) == "fullscreen-stretch")
      return DISPLAY_MODE_FULL_SCREEN_STRETCHED;
   else if(lcase(mode) == "fullscreen")
      return DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED;
   else 
      return DISPLAY_MODE_WINDOWED;
}


// Convert a string value to our sfxSets enum
static string displayModeToString(DisplayMode mode)
{
   if(mode == DISPLAY_MODE_FULL_SCREEN_STRETCHED)
      return "Fullscreen-Stretch";
   else if(mode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
      return "Fullscreen";
   else
      return "Window";
}


static void loadGeneralSettings(CIniFile *ini)
{
   string section = "Settings";

   gIniSettings.displayMode = stringToDisplayMode( ini->GetValue(section, "WindowMode", displayModeToString(gIniSettings.displayMode)));
   gIniSettings.oldDisplayMode = gIniSettings.displayMode;

   gIniSettings.controlsRelative = (lcase(ini->GetValue(section, "ControlMode", (gIniSettings.controlsRelative ? "Relative" : "Absolute"))) == "relative");

   gIniSettings.echoVoice            = ini->GetValueYN(section, "VoiceEcho", gIniSettings.echoVoice);
   gIniSettings.showWeaponIndicators = ini->GetValueYN(section, "LoadoutIndicators", gIniSettings.showWeaponIndicators);
   gIniSettings.verboseHelpMessages  = ini->GetValueYN(section, "VerboseHelpMessages", gIniSettings.verboseHelpMessages);
   gIniSettings.showKeyboardKeys     = ini->GetValueYN(section, "ShowKeyboardKeysInStickMode", gIniSettings.showKeyboardKeys);

#ifndef ZAP_DEDICATED
   gIniSettings.joystickType = Joystick::stringToJoystickType(ini->GetValue(section, "JoystickType", Joystick::joystickTypeToString(gIniSettings.joystickType)).c_str());
#endif

   gIniSettings.winXPos = max(ini->GetValueI(section, "WindowXPos", gIniSettings.winXPos), 0);    // Restore window location
   gIniSettings.winYPos = max(ini->GetValueI(section, "WindowYPos", gIniSettings.winYPos), 0);

   gIniSettings.winSizeFact = (F32) ini->GetValueF(section, "WindowScalingFactor", gIniSettings.winSizeFact);
   gIniSettings.masterAddress = ini->GetValue(section, "MasterServerAddressList", gIniSettings.masterAddress);
   
   gIniSettings.name           = ini->GetValue(section, "Nickname", gIniSettings.name);
   gIniSettings.password       = ini->GetValue(section, "Password", gIniSettings.password);

   gIniSettings.defaultName    = ini->GetValue(section, "DefaultName", gIniSettings.defaultName);
   gIniSettings.lastName       = ini->GetValue(section, "LastName", gIniSettings.lastName);
   gIniSettings.lastPassword   = ini->GetValue(section, "LastPassword", gIniSettings.lastPassword);
   gIniSettings.lastEditorName = ini->GetValue(section, "LastEditorName", gIniSettings.lastEditorName);

   gIniSettings.version = ini->GetValueI(section, "Version", gIniSettings.version);

   gIniSettings.enableExperimentalAimMode = ini->GetValueYN(section, "EnableExperimentalAimMode", gIniSettings.enableExperimentalAimMode);
   S32 fps = ini->GetValueI(section, "MaxFPS", gIniSettings.maxFPS);
   if(fps >= 1) 
      gIniSettings.maxFPS = fps;   // Otherwise, leave it at the default value
   // else warn?

#ifndef ZAP_DEDICATED
   gDefaultLineWidth = (F32) ini->GetValueF(section, "LineWidth", 2);
   gLineWidth1 = gDefaultLineWidth * 0.5f;
   gLineWidth3 = gDefaultLineWidth * 1.5f;
   gLineWidth4 = gDefaultLineWidth * 2;
#endif
}


static void loadDiagnostics(CIniFile *ini)
{
   string section = "Diagnostics";

   gIniSettings.diagnosticKeyDumpMode = ini->GetValueYN(section, "DumpKeys",              gIniSettings.diagnosticKeyDumpMode);

   gIniSettings.logConnectionProtocol = ini->GetValueYN(section, "LogConnectionProtocol", gIniSettings.logConnectionProtocol);
   gIniSettings.logNetConnection      = ini->GetValueYN(section, "LogNetConnection",      gIniSettings.logNetConnection);
   gIniSettings.logEventConnection    = ini->GetValueYN(section, "LogEventConnection",    gIniSettings.logEventConnection);
   gIniSettings.logGhostConnection    = ini->GetValueYN(section, "LogGhostConnection",    gIniSettings.logGhostConnection);
   gIniSettings.logNetInterface       = ini->GetValueYN(section, "LogNetInterface",       gIniSettings.logNetInterface);
   gIniSettings.logPlatform           = ini->GetValueYN(section, "LogPlatform",           gIniSettings.logPlatform);
   gIniSettings.logNetBase            = ini->GetValueYN(section, "LogNetBase",            gIniSettings.logNetBase);
   gIniSettings.logUDP                = ini->GetValueYN(section, "LogUDP",                gIniSettings.logUDP);

   gIniSettings.logFatalError         = ini->GetValueYN(section, "LogFatalError",         gIniSettings.logFatalError);
   gIniSettings.logError              = ini->GetValueYN(section, "LogError",              gIniSettings.logError);
   gIniSettings.logWarning            = ini->GetValueYN(section, "LogWarning",            gIniSettings.logWarning);
   gIniSettings.logConnection         = ini->GetValueYN(section, "LogConnection",         gIniSettings.logConnection);

   gIniSettings.logLevelLoaded        = ini->GetValueYN(section, "LogLevelLoaded",        gIniSettings.logLevelLoaded);
   gIniSettings.logLuaObjectLifecycle = ini->GetValueYN(section, "LogLuaObjectLifecycle", gIniSettings.logLuaObjectLifecycle);
   gIniSettings.luaLevelGenerator     = ini->GetValueYN(section, "LuaLevelGenerator",     gIniSettings.luaLevelGenerator);
   gIniSettings.luaBotMessage         = ini->GetValueYN(section, "LuaBotMessage",         gIniSettings.luaBotMessage);
   gIniSettings.serverFilter          = ini->GetValueYN(section, "ServerFilter",          gIniSettings.serverFilter);
}


static void loadTestSettings(CIniFile *ini)
{
   gIniSettings.burstGraphicsMode = max(ini->GetValueI("Testing", "BurstGraphics", gIniSettings.burstGraphicsMode), 0);
   gIniSettings.neverConnectDirect = ini->GetValueYN("Testing", "NeverConnectDirect", gIniSettings.neverConnectDirect);
   gIniSettings.wallFillColor.set(ini->GetValue("Testing", "WallFillColor", gIniSettings.wallFillColor.toRGBString()));
   gIniSettings.wallOutlineColor.set(ini->GetValue("Testing", "WallOutlineColor", gIniSettings.wallOutlineColor.toRGBString()));
   gIniSettings.oldGoalFlash = ini->GetValueYN("Testing", "OldGoalFlash", gIniSettings.oldGoalFlash);
}


static void loadEffectsSettings(CIniFile *ini)
{
   gIniSettings.starsInDistance  = (lcase(ini->GetValue("Effects", "StarsInDistance", (gIniSettings.starsInDistance ? "Yes" : "No"))) == "yes");
   gIniSettings.useLineSmoothing = (lcase(ini->GetValue("Effects", "LineSmoothing", "No")) == "yes");
}


// Convert a string value to our sfxSets enum
static sfxSets stringToSFXSet(string sfxSet)
{
   return (lcase(sfxSet) == "classic") ? sfxClassicSet : sfxModernSet;
}


static F32 checkVol(F32 vol)
{
   return max(min(vol, 1), 0);    // Restrict volume to be between 0 and 1
}


static void loadSoundSettings(CIniFile *ini)
{
   gIniSettings.sfxVolLevel       = (F32) ini->GetValueI("Sounds", "EffectsVolume",   (S32) (gIniSettings.sfxVolLevel       * 10)) / 10.0f;
   gIniSettings.musicVolLevel     = (F32) ini->GetValueI("Sounds", "MusicVolume",     (S32) (gIniSettings.musicVolLevel     * 10)) / 10.0f;
   gIniSettings.voiceChatVolLevel = (F32) ini->GetValueI("Sounds", "VoiceChatVolume", (S32) (gIniSettings.voiceChatVolLevel * 10)) / 10.0f;

   string sfxSet = ini->GetValue("Sounds", "SFXSet", "Modern");
   gIniSettings.sfxSet = stringToSFXSet(sfxSet);

   // Bounds checking
   gIniSettings.sfxVolLevel       = checkVol(gIniSettings.sfxVolLevel);
   gIniSettings.musicVolLevel     = checkVol(gIniSettings.musicVolLevel);
   gIniSettings.voiceChatVolLevel = checkVol(gIniSettings.voiceChatVolLevel);
}

static void loadHostConfiguration(CIniFile *ini)
{
   gIniSettings.hostname  = ini->GetValue("Host", "ServerName", gIniSettings.hostname);
   gIniSettings.hostaddr  = ini->GetValue("Host", "ServerAddress", gIniSettings.hostaddr);
   gIniSettings.hostdescr = ini->GetValue("Host", "ServerDescription", gIniSettings.hostdescr);

   gIniSettings.serverPassword      = ini->GetValue("Host", "ServerPassword", gIniSettings.serverPassword);
   gIniSettings.adminPassword       = ini->GetValue("Host", "AdminPassword", gIniSettings.adminPassword);
   gIniSettings.levelChangePassword = ini->GetValue("Host", "LevelChangePassword", gIniSettings.levelChangePassword);
   gIniSettings.levelDir            = ini->GetValue("Host", "LevelDir", gIniSettings.levelDir);
   gIniSettings.maxPlayers          = ini->GetValueI("Host", "MaxPlayers", gIniSettings.maxPlayers);
   gIniSettings.maxBots             = ini->GetValueI("Host", "MaxBots", gIniSettings.maxBots);

   gIniSettings.alertsVolLevel = (float) ini->GetValueI("Host", "AlertsVolume", (S32) (gIniSettings.alertsVolLevel * 10)) / 10.0f;
   gIniSettings.allowGetMap          = ini->GetValueYN("Host", "AllowGetMap", gIniSettings.allowGetMap);
   gIniSettings.allowDataConnections = ini->GetValueYN("Host", "AllowDataConnections", gIniSettings.allowDataConnections);

   S32 fps = ini->GetValueI("Host", "MaxFPS", gIniSettings.maxDedicatedFPS);
   if(fps >= 1) 
      gIniSettings.maxDedicatedFPS = fps; 
   // TODO: else warn?

   gIniSettings.logStats = ini->GetValueYN("Host", "LogStats", gIniSettings.logStats);

   //gIniSettings.SendStatsToMaster = (lcase(ini->GetValue("Host", "SendStatsToMaster", "yes")) != "no");

   gIniSettings.alertsVolLevel = checkVol(gIniSettings.alertsVolLevel);

   gIniSettings.allowMapUpload         = (U32) ini->GetValueYN("Host", "AllowMapUpload", S32(gIniSettings.allowMapUpload) );
   gIniSettings.allowAdminMapUpload    = (U32) ini->GetValueYN("Host", "AllowAdminMapUpload", S32(gIniSettings.allowAdminMapUpload) );

   gIniSettings.voteEnable             = (U32) ini->GetValueYN("Host", "VoteEnable", S32(gIniSettings.voteEnable) );
   gIniSettings.voteLength             = (U32) ini->GetValueI("Host", "VoteLength", S32(gIniSettings.voteLength) );
   gIniSettings.voteLengthToChangeTeam = (U32) ini->GetValueI("Host", "VoteLengthToChangeTeam", S32(gIniSettings.voteLengthToChangeTeam) );
   gIniSettings.voteRetryLength        = (U32) ini->GetValueI("Host", "VoteRetryLength", S32(gIniSettings.voteRetryLength) );
   gIniSettings.voteYesStrength        = ini->GetValueI("Host", "VoteYesStrength", gIniSettings.voteYesStrength );
   gIniSettings.voteNoStrength         = ini->GetValueI("Host", "VoteNoStrength", gIniSettings.voteNoStrength );
   gIniSettings.voteNothingStrength    = ini->GetValueI("Host", "VoteNothingStrength", gIniSettings.voteNothingStrength );

#ifdef BF_WRITE_TO_MYSQL
   Vector<string> args;
   parseString(ini->GetValue("Host", "MySqlStatsDatabaseCredentials").c_str(), args, ',');
   if(args.size() >= 1) gIniSettings.mySqlStatsDatabaseServer = args[0];
   if(args.size() >= 2) gIniSettings.mySqlStatsDatabaseName = args[1];
   if(args.size() >= 3) gIniSettings.mySqlStatsDatabaseUser = args[2];
   if(args.size() >= 4) gIniSettings.mySqlStatsDatabasePassword = args[3];
   if(gIniSettings.mySqlStatsDatabaseServer == "server" && gIniSettings.mySqlStatsDatabaseName == "dbname")
   {
      gIniSettings.mySqlStatsDatabaseServer = "";  // blank this, so it won't try to connect to "server"
   }
#endif

   gIniSettings.defaultRobotScript = ini->GetValue("Host", "DefaultRobotScript", gIniSettings.defaultRobotScript);
   gIniSettings.globalLevelScript  = ini->GetValue("Host", "GlobalLevelScript", gIniSettings.globalLevelScript);
}


void loadUpdaterSettings(CIniFile *ini)
{
   gIniSettings.useUpdater = lcase(ini->GetValue("Updater", "UseUpdater", "Yes")) != "no";
   //if(! gIniSettings.useUpdater) logprintf("useUpdater is OFF");
   //if(gIniSettings.useUpdater) logprintf("useUpdater is ON");
}


static KeyCode getKeyCode(CIniFile *ini, const string &section, const string &key, KeyCode defaultValue)
{
   return stringToKeyCode(ini->GetValue(section, key, keyCodeToString(defaultValue)).c_str());
}


// Remember: If you change any of these defaults, you'll need to rebuild your INI file to see the results!
static void loadKeyBindings(CIniFile *ini)
{                                
   string section = "KeyboardKeyBindings";

   keySELWEAP1[InputModeKeyboard]  = getKeyCode(ini, section, "SelWeapon1",      KEY_1);
   keySELWEAP2[InputModeKeyboard]  = getKeyCode(ini, section, "SelWeapon2",      KEY_2);
   keySELWEAP3[InputModeKeyboard]  = getKeyCode(ini, section, "SelWeapon3",      KEY_3);
   keyADVWEAP[InputModeKeyboard]   = getKeyCode(ini, section, "SelNextWeapon",   KEY_E);
   keyCMDRMAP[InputModeKeyboard]   = getKeyCode(ini, section, "ShowCmdrMap",     KEY_C);
   keyTEAMCHAT[InputModeKeyboard]  = getKeyCode(ini, section, "TeamChat",        KEY_T);
   keyGLOBCHAT[InputModeKeyboard]  = getKeyCode(ini, section, "GlobalChat",      KEY_G);
   keyQUICKCHAT[InputModeKeyboard] = getKeyCode(ini, section, "QuickChat",       KEY_V);
   keyCMDCHAT[InputModeKeyboard]   = getKeyCode(ini, section, "Command",         KEY_SLASH);
   keyLOADOUT[InputModeKeyboard]   = getKeyCode(ini, section, "ShowLoadoutMenu", KEY_Z);
   keyMOD1[InputModeKeyboard]      = getKeyCode(ini, section, "ActivateModule1", KEY_SPACE);
   keyMOD2[InputModeKeyboard]      = getKeyCode(ini, section, "ActivateModule2", MOUSE_RIGHT);
   keyFIRE[InputModeKeyboard]      = getKeyCode(ini, section, "Fire",            MOUSE_LEFT);
   keyDROPITEM[InputModeKeyboard]  = getKeyCode(ini, section, "DropItem",        KEY_B);

   keyTOGVOICE[InputModeKeyboard]  = getKeyCode(ini, section, "VoiceChat",       KEY_R);
   keyUP[InputModeKeyboard]        = getKeyCode(ini, section, "ShipUp",          KEY_W);
   keyDOWN[InputModeKeyboard]      = getKeyCode(ini, section, "ShipDown",        KEY_S);
   keyLEFT[InputModeKeyboard]      = getKeyCode(ini, section, "ShipLeft",        KEY_A);
   keyRIGHT[InputModeKeyboard]     = getKeyCode(ini, section, "ShipRight",       KEY_D);
   keySCRBRD[InputModeKeyboard]    = getKeyCode(ini, section, "ShowScoreboard",  KEY_TAB);

   section = "JoystickKeyBindings";

   keySELWEAP1[InputModeJoystick]  = getKeyCode(ini, section, "SelWeapon1",      KEY_1);
   keySELWEAP2[InputModeJoystick]  = getKeyCode(ini, section, "SelWeapon2",      KEY_2);
   keySELWEAP3[InputModeJoystick]  = getKeyCode(ini, section, "SelWeapon3",      KEY_3);
   keyADVWEAP[InputModeJoystick]   = getKeyCode(ini, section, "SelNextWeapon",   BUTTON_1);
   keyCMDRMAP[InputModeJoystick]   = getKeyCode(ini, section, "ShowCmdrMap",     BUTTON_2);
   keyTEAMCHAT[InputModeJoystick]  = getKeyCode(ini, section, "TeamChat",        KEY_T);
   keyGLOBCHAT[InputModeJoystick]  = getKeyCode(ini, section, "GlobalChat",      KEY_G);
   keyQUICKCHAT[InputModeJoystick] = getKeyCode(ini, section, "QuickChat",       BUTTON_3);
   keyCMDCHAT[InputModeJoystick]   = getKeyCode(ini, section, "Command",         KEY_SLASH);
   keyLOADOUT[InputModeJoystick]   = getKeyCode(ini, section, "ShowLoadoutMenu", BUTTON_4);
   keyMOD1[InputModeJoystick]      = getKeyCode(ini, section, "ActivateModule1", BUTTON_7);
   keyMOD2[InputModeJoystick]      = getKeyCode(ini, section, "ActivateModule2", BUTTON_6);
   keyFIRE[InputModeJoystick]      = getKeyCode(ini, section, "Fire",            MOUSE_LEFT);
   keyDROPITEM[InputModeJoystick]  = getKeyCode(ini, section, "DropItem",        BUTTON_8);
   keyTOGVOICE[InputModeJoystick]  = getKeyCode(ini, section, "VoiceChat",       KEY_R);
   keyUP[InputModeJoystick]        = getKeyCode(ini, section, "ShipUp",          KEY_UP);
   keyDOWN[InputModeJoystick]      = getKeyCode(ini, section, "ShipDown",        KEY_DOWN);
   keyLEFT[InputModeJoystick]      = getKeyCode(ini, section, "ShipLeft",        KEY_LEFT);
   keyRIGHT[InputModeJoystick]     = getKeyCode(ini, section, "ShipRight",       KEY_RIGHT);
   keySCRBRD[InputModeJoystick]    = getKeyCode(ini, section, "ShowScoreboard",  BUTTON_5);

   // The following key bindings are not user-defineable at the moment, mostly because we want consistency
   // throughout the game, and that would require some real constraints on what keys users could choose.
   //keyHELP = KEY_F1;
   //keyMISSION = KEY_F2;
   //keyOUTGAMECHAT = KEY_F5;
   //keyFPS = KEY_F6;
   //keyDIAG = KEY_F7;
   // These were moved to main.cpp to get them defined before the menus
}

static void writeKeyBindings(CIniFile *ini)
{
   const char *section;

   section = "KeyboardKeyBindings";

   ini->SetValue(section, "SelWeapon1",      keyCodeToString(keySELWEAP1[InputModeKeyboard]));
   ini->SetValue(section, "SelWeapon2",      keyCodeToString(keySELWEAP2[InputModeKeyboard]));
   ini->SetValue(section, "SelWeapon3",      keyCodeToString(keySELWEAP3[InputModeKeyboard]));
   ini->SetValue(section, "SelNextWeapon",   keyCodeToString(keyADVWEAP[InputModeKeyboard]));
   ini->SetValue(section, "ShowCmdrMap",     keyCodeToString(keyCMDRMAP[InputModeKeyboard]));
   ini->SetValue(section, "TeamChat",        keyCodeToString(keyTEAMCHAT[InputModeKeyboard]));
   ini->SetValue(section, "GlobalChat",      keyCodeToString(keyGLOBCHAT[InputModeKeyboard]));
   ini->SetValue(section, "QuickChat",       keyCodeToString(keyQUICKCHAT[InputModeKeyboard]));
   ini->SetValue(section, "Command",         keyCodeToString(keyCMDCHAT[InputModeKeyboard]));
   ini->SetValue(section, "ShowLoadoutMenu", keyCodeToString(keyLOADOUT[InputModeKeyboard]));
   ini->SetValue(section, "ActivateModule1", keyCodeToString(keyMOD1[InputModeKeyboard]));
   ini->SetValue(section, "ActivateModule2", keyCodeToString(keyMOD2[InputModeKeyboard]));
   ini->SetValue(section, "Fire",            keyCodeToString(keyFIRE[InputModeKeyboard]));
   ini->SetValue(section, "DropItem",        keyCodeToString(keyDROPITEM[InputModeKeyboard]));
   ini->SetValue(section, "VoiceChat",       keyCodeToString(keyTOGVOICE[InputModeKeyboard]));
   ini->SetValue(section, "ShipUp",          keyCodeToString(keyUP[InputModeKeyboard]));
   ini->SetValue(section, "ShipDown",        keyCodeToString(keyDOWN[InputModeKeyboard]));
   ini->SetValue(section, "ShipLeft",        keyCodeToString(keyLEFT[InputModeKeyboard]));
   ini->SetValue(section, "ShipRight",       keyCodeToString(keyRIGHT[InputModeKeyboard]));
   ini->SetValue(section, "ShowScoreboard",  keyCodeToString(keySCRBRD[InputModeKeyboard]));

   section = "JoystickKeyBindings";

   ini->SetValue(section, "SelWeapon1",      keyCodeToString(keySELWEAP1[InputModeJoystick]));
   ini->SetValue(section, "SelWeapon2",      keyCodeToString(keySELWEAP2[InputModeJoystick]));
   ini->SetValue(section, "SelWeapon3",      keyCodeToString(keySELWEAP3[InputModeJoystick]));
   ini->SetValue(section, "SelNextWeapon",   keyCodeToString(keyADVWEAP[InputModeJoystick]));
   ini->SetValue(section, "ShowCmdrMap",     keyCodeToString(keyCMDRMAP[InputModeJoystick]));
   ini->SetValue(section, "TeamChat",        keyCodeToString(keyTEAMCHAT[InputModeJoystick]));
   ini->SetValue(section, "GlobalChat",      keyCodeToString(keyGLOBCHAT[InputModeJoystick]));
   ini->SetValue(section, "QuickChat",       keyCodeToString(keyQUICKCHAT[InputModeJoystick]));
   ini->SetValue(section, "Command",         keyCodeToString(keyCMDCHAT[InputModeJoystick]));
   ini->SetValue(section, "ShowLoadoutMenu", keyCodeToString(keyLOADOUT[InputModeJoystick]));
   ini->SetValue(section, "ActivateModule1", keyCodeToString(keyMOD1[InputModeJoystick]));
   ini->SetValue(section, "ActivateModule2", keyCodeToString(keyMOD2[InputModeJoystick]));
   ini->SetValue(section, "Fire",            keyCodeToString(keyFIRE[InputModeJoystick]));
   ini->SetValue(section, "DropItem",        keyCodeToString(keyDROPITEM[InputModeJoystick]));
   ini->SetValue(section, "VoiceChat",       keyCodeToString(keyTOGVOICE[InputModeJoystick]));
   ini->SetValue(section, "ShipUp",          keyCodeToString(keyUP[InputModeJoystick]));
   ini->SetValue(section, "ShipDown",        keyCodeToString(keyDOWN[InputModeJoystick]));
   ini->SetValue(section, "ShipLeft",        keyCodeToString(keyLEFT[InputModeJoystick]));
   ini->SetValue(section, "ShipRight",       keyCodeToString(keyRIGHT[InputModeJoystick]));
   ini->SetValue(section, "ShowScoreboard",  keyCodeToString(keySCRBRD[InputModeJoystick]));
}


/*  INI file looks a little like this:
   [QuickChatMessagesGroup1]
   Key=F
   Button=1
   Caption=Flag

   [QuickChatMessagesGroup1_Message1]
   Key=G
   Button=Button 1
   Caption=Flag Gone!
   Message=Our flag is not in the base!
   MessageType=Team     -or-     MessageType=Global

   == or, a top tiered message might look like this ==

   [QuickChat_Message1]
   Key=A
   Button=Button 1
   Caption=Hello
   MessageType=Hello there!
*/
static void loadQuickChatMessages(CIniFile *ini)
{
#ifndef ZAP_DEDICATED
   // Add initial node
   QuickChatNode emptynode;
   emptynode.depth = 0;    // This is a beginning or ending node
   emptynode.keyCode = KEY_UNKNOWN;
   emptynode.buttonCode = KEY_UNKNOWN;
   emptynode.teamOnly = false;
   emptynode.commandOnly = false;
   emptynode.caption = "";
   emptynode.msg = "";
   gQuickChatTree.push_back(emptynode);
   emptynode.isMsgItem = false;

   // Read QuickChat messages -- first search for keys matching "QuickChatMessagesGroup123"
   S32 keys = ini->GetNumKeys();
   Vector<string> groups;

   // Next, read any top-level messages
   Vector<string> messages;
   for(S32 i = 0; i < keys; i++)
   {
      string keyName = ini->getSectionName(i);
      if(keyName.substr(0, 17) == "QuickChat_Message")   // Found message group
         messages.push_back(keyName);
   }

   messages.sort(alphaSort);

   for(S32 i = messages.size()-1; i >= 0; i--)
   {
      QuickChatNode node;
      node.depth = 1;   // This is a top-level message node
      node.keyCode = stringToKeyCode(ini->GetValue(messages[i], "Key", "A").c_str());
      node.buttonCode = stringToKeyCode(ini->GetValue(messages[i], "Button", "Button 1").c_str());
      string str1 = lcase(ini->GetValue(messages[i], "MessageType", "Team"));      // lcase for case insensitivity
      node.teamOnly = str1 == "team";
      node.commandOnly = str1 == "command";
      node.caption = ini->GetValue(messages[i], "Caption", "Caption");
      node.msg = ini->GetValue(messages[i], "Message", "Message");
      node.isMsgItem = true;
      gQuickChatTree.push_back(node);
   }

   for(S32 i = 0; i < keys; i++)
   {
      string keyName = ini->getSectionName(i);
      if(keyName.substr(0, 22) == "QuickChatMessagesGroup" && keyName.find("_") == string::npos)   // Found message group
         groups.push_back(keyName);
   }

   groups.sort(alphaSort);

   // Now find all the individual message definitions for each key -- match "QuickChatMessagesGroup123_Message456"
   // quickChat render functions were designed to work with the messages sorted in reverse.  Rather than
   // reenigneer those, let's just iterate backwards and leave the render functions alone.

   for(S32 i = groups.size()-1; i >= 0; i--)
   {
      Vector<string> messages;
      for(S32 j = 0; j < keys; j++)
      {
         string keyName = ini->getSectionName(j);
         if(keyName.substr(0, groups[i].length() + 1) == groups[i] + "_")
            messages.push_back(keyName);
      }

      messages.sort(alphaSort);

      QuickChatNode node;
      node.depth = 1;      // This is a group node
      node.keyCode = stringToKeyCode(ini->GetValue(groups[i], "Key", "A").c_str());
      node.buttonCode = stringToKeyCode(ini->GetValue(groups[i], "Button", "Button 1").c_str());
      string str1 = lcase(ini->GetValue(groups[i], "MessageType", "Team"));      // lcase for case insensitivity
      node.teamOnly = str1 == "team";
      node.commandOnly = str1 == "command";
      node.caption = ini->GetValue(groups[i], "Caption", "Caption");
      node.msg = "";
      node.isMsgItem = false;
      gQuickChatTree.push_back(node);

      for(S32 j = messages.size()-1; j >= 0; j--)
      {
         node.depth = 2;   // This is a message node
         node.keyCode = stringToKeyCode(ini->GetValue(messages[j], "Key", "A").c_str());
         node.buttonCode = stringToKeyCode(ini->GetValue(messages[j], "Button", "Button 1").c_str());
         str1 = lcase(ini->GetValue(messages[j], "MessageType", "Team"));      // lcase for case insensitivity
         node.teamOnly = str1 == "team";
         node.commandOnly = str1 == "command";
         node.caption = ini->GetValue(messages[j], "Caption", "Caption");
         node.msg = ini->GetValue(messages[j], "Message", "Message");
         node.isMsgItem = true;
         gQuickChatTree.push_back(node);
      }
   }

   // Add final node.  Last verse, same as the first.
   gQuickChatTree.push_back(emptynode);
#endif
}


static void writeDefaultQuickChatMessages(CIniFile *ini)
{
   // Are there any QuickChatMessageGroups?  If not, we'll write the defaults.
   S32 keys = ini->GetNumKeys();

   for(S32 i = 0; i < keys; i++)
   {
      string keyName = ini->getSectionName(i);
      if(keyName.substr(0, 22) == "QuickChatMessagesGroup" && keyName.find("_") == string::npos)
         return;
   }

   ini->addSection("QuickChatMessages");
   if(ini->numSectionComments("QuickChatMessages") == 0)
   {
      ini->sectionComment("QuickChatMessages", "----------------");
      ini->sectionComment("QuickChatMessages", " The structure of the QuickChatMessages sections is a bit complicated.  The structure reflects the way the messages are");
      ini->sectionComment("QuickChatMessages", " displayed in the QuickChat menu, so make sure you are familiar with that before you start modifying these items.");
      ini->sectionComment("QuickChatMessages", " Messages are grouped, and each group has a Caption (short name shown on screen), a Key (the shortcut key used to select");
      ini->sectionComment("QuickChatMessages", " the group), and a Button (a shortcut button used when in joystick mode).  If the Button is \"Undefined key\", then that");
      ini->sectionComment("QuickChatMessages", " item will not be shown in joystick mode, unless the ShowKeyboardKeysInStickMode setting is true.  Groups can be defined ");
      ini->sectionComment("QuickChatMessages", " in any order, but will be displayed sorted by [section] name.  Groups are designated by the [QuickChatMessagesGroupXXX]");
      ini->sectionComment("QuickChatMessages", " sections, where XXX is a unique suffix, usually a number.");
      ini->sectionComment("QuickChatMessages", " ");
      ini->sectionComment("QuickChatMessages", " Each group can have one or more messages, as specified by the [QuickChatMessagesGroupXXX_MessageYYY] sections, where XXX");
      ini->sectionComment("QuickChatMessages", " is the unique group suffix, and YYY is a unique message suffix.  Again, messages can be defined in any order, and will");
      ini->sectionComment("QuickChatMessages", " appear sorted by their [section] name.  Key, Button, and Caption serve the same purposes as in the group definitions.");
      ini->sectionComment("QuickChatMessages", " Message is the actual message text that is sent, and MessageType should be either \"Team\" or \"Global\", depending on which");
      ini->sectionComment("QuickChatMessages", " users the message should be sent to.  You can mix Team and Global messages in the same section, but it may be less");
      ini->sectionComment("QuickChatMessages", " confusing not to do so.");
      ini->sectionComment("QuickChatMessages", " ");
      ini->sectionComment("QuickChatMessages", " Messages can also be added to the top-tier of items, by specifying a section like [QuickChat_MessageZZZ].");
      ini->sectionComment("QuickChatMessages", " ");
      ini->sectionComment("QuickChatMessages", " Note that no quotes are required around Messages or Captions, and if included, they will be sent as part");
      ini->sectionComment("QuickChatMessages", " of the message.  Also, if you bullocks things up too badly, simply delete all QuickChatMessage sections,");
      ini->sectionComment("QuickChatMessages", " and they will be regenerated the next time you run the game (though your modifications will be lost).");
      ini->sectionComment("QuickChatMessages", "----------------");
   }

   ini->SetValue("QuickChatMessagesGroup1", "Key", keyCodeToString(KEY_G));
   ini->SetValue("QuickChatMessagesGroup1", "Button", keyCodeToString(BUTTON_6));
   ini->SetValue("QuickChatMessagesGroup1", "Caption", "Global");
   ini->SetValue("QuickChatMessagesGroup1", "MessageType", "Global");

      ini->SetValue("QuickChatMessagesGroup1_Message1", "Key", keyCodeToString(KEY_A));
      ini->SetValue("QuickChatMessagesGroup1_Message1", "Button", keyCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup1_Message1", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message1", "Caption", "No Problem");
      ini->SetValue("QuickChatMessagesGroup1_Message1", "Message", "No Problemo.");

      ini->SetValue("QuickChatMessagesGroup1_Message2", "Key", keyCodeToString(KEY_T));
      ini->SetValue("QuickChatMessagesGroup1_Message2", "Button", keyCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup1_Message2", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message2", "Caption", "Thanks");
      ini->SetValue("QuickChatMessagesGroup1_Message2", "Message", "Thanks.");

      ini->SetValue("QuickChatMessagesGroup1_Message3", "Key", keyCodeToString(KEY_X));
      ini->SetValue("QuickChatMessagesGroup1_Message3", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup1_Message3", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message3", "Caption", "You idiot!");
      ini->SetValue("QuickChatMessagesGroup1_Message3", "Message", "You idiot!");

      ini->SetValue("QuickChatMessagesGroup1_Message4", "Key", keyCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup1_Message4", "Button", keyCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup1_Message4", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message4", "Caption", "Duh");
      ini->SetValue("QuickChatMessagesGroup1_Message4", "Message", "Duh.");

      ini->SetValue("QuickChatMessagesGroup1_Message5", "Key", keyCodeToString(KEY_C));
      ini->SetValue("QuickChatMessagesGroup1_Message5", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup1_Message5", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message5", "Caption", "Crap");
      ini->SetValue("QuickChatMessagesGroup1_Message5", "Message", "Ah Crap!");

      ini->SetValue("QuickChatMessagesGroup1_Message6", "Key", keyCodeToString(KEY_D));
      ini->SetValue("QuickChatMessagesGroup1_Message6", "Button", keyCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup1_Message6", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message6", "Caption", "Damnit");
      ini->SetValue("QuickChatMessagesGroup1_Message6", "Message", "Dammit!");

      ini->SetValue("QuickChatMessagesGroup1_Message7", "Key", keyCodeToString(KEY_S));
      ini->SetValue("QuickChatMessagesGroup1_Message7", "Button", keyCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup1_Message7", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message7", "Caption", "Shazbot");
      ini->SetValue("QuickChatMessagesGroup1_Message7", "Message", "Shazbot!");

      ini->SetValue("QuickChatMessagesGroup1_Message8", "Key", keyCodeToString(KEY_Z));
      ini->SetValue("QuickChatMessagesGroup1_Message8", "Button", keyCodeToString(BUTTON_6));
      ini->SetValue("QuickChatMessagesGroup1_Message8", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message8", "Caption", "Doh");
      ini->SetValue("QuickChatMessagesGroup1_Message8", "Message", "Doh!");

   ini->SetValue("QuickChatMessagesGroup2", "Key", keyCodeToString(KEY_D));
   ini->SetValue("QuickChatMessagesGroup2", "Button", keyCodeToString(BUTTON_5));
   ini->SetValue("QuickChatMessagesGroup2", "MessageType", "Team");
   ini->SetValue("QuickChatMessagesGroup2", "Caption", "Defense");

      ini->SetValue("QuickChatMessagesGroup2_Message1", "Key", keyCodeToString(KEY_G));
      ini->SetValue("QuickChatMessagesGroup2_Message1", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup2_Message1", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message1", "Caption", "Defend Our Base");
      ini->SetValue("QuickChatMessagesGroup2_Message1", "Message", "Defend our base.");

      ini->SetValue("QuickChatMessagesGroup2_Message2", "Key", keyCodeToString(KEY_D));
      ini->SetValue("QuickChatMessagesGroup2_Message2", "Button", keyCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup2_Message2", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message2", "Caption", "Defending Base");
      ini->SetValue("QuickChatMessagesGroup2_Message2", "Message", "Defending our base.");

      ini->SetValue("QuickChatMessagesGroup2_Message3", "Key", keyCodeToString(KEY_Q));
      ini->SetValue("QuickChatMessagesGroup2_Message3", "Button", keyCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup2_Message3", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message3", "Caption", "Is Base Clear?");
      ini->SetValue("QuickChatMessagesGroup2_Message3", "Message", "Is our base clear?");

      ini->SetValue("QuickChatMessagesGroup2_Message4", "Key", keyCodeToString(KEY_C));
      ini->SetValue("QuickChatMessagesGroup2_Message4", "Button", keyCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup2_Message4", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message4", "Caption", "Base Clear");
      ini->SetValue("QuickChatMessagesGroup2_Message4", "Message", "Base is secured.");

      ini->SetValue("QuickChatMessagesGroup2_Message5", "Key", keyCodeToString(KEY_T));
      ini->SetValue("QuickChatMessagesGroup2_Message5", "Button", keyCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup2_Message5", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message5", "Caption", "Base Taken");
      ini->SetValue("QuickChatMessagesGroup2_Message5", "Message", "Base is taken.");

      ini->SetValue("QuickChatMessagesGroup2_Message6", "Key", keyCodeToString(KEY_N));
      ini->SetValue("QuickChatMessagesGroup2_Message6", "Button", keyCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup2_Message6", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message6", "Caption", "Need More Defense");
      ini->SetValue("QuickChatMessagesGroup2_Message6", "Message", "We need more defense.");

      ini->SetValue("QuickChatMessagesGroup2_Message7", "Key", keyCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup2_Message7", "Button", keyCodeToString(BUTTON_6));
      ini->SetValue("QuickChatMessagesGroup2_Message7", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message7", "Caption", "Enemy Attacking Base");
      ini->SetValue("QuickChatMessagesGroup2_Message7", "Message", "The enemy is attacking our base.");

      ini->SetValue("QuickChatMessagesGroup2_Message8", "Key", keyCodeToString(KEY_A));
      ini->SetValue("QuickChatMessagesGroup2_Message8", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup2_Message8", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message8", "Caption", "Attacked");
      ini->SetValue("QuickChatMessagesGroup2_Message8", "Message", "We are being attacked.");

   ini->SetValue("QuickChatMessagesGroup3", "Key", keyCodeToString(KEY_F));
   ini->SetValue("QuickChatMessagesGroup3", "Button", keyCodeToString(BUTTON_4));
   ini->SetValue("QuickChatMessagesGroup3", "MessageType", "Team");
   ini->SetValue("QuickChatMessagesGroup3", "Caption", "Flag");

      ini->SetValue("QuickChatMessagesGroup3_Message1", "Key", keyCodeToString(KEY_F));
      ini->SetValue("QuickChatMessagesGroup3_Message1", "Button", keyCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup3_Message1", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message1", "Caption", "Get enemy flag");
      ini->SetValue("QuickChatMessagesGroup3_Message1", "Message", "Get the enemy flag.");

      ini->SetValue("QuickChatMessagesGroup3_Message2", "Key", keyCodeToString(KEY_R));
      ini->SetValue("QuickChatMessagesGroup3_Message2", "Button", keyCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup3_Message2", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message2", "Caption", "Return our flag");
      ini->SetValue("QuickChatMessagesGroup3_Message2", "Message", "Return our flag to base.");

      ini->SetValue("QuickChatMessagesGroup3_Message3", "Key", keyCodeToString(KEY_S));
      ini->SetValue("QuickChatMessagesGroup3_Message3", "Button", keyCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup3_Message3", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message3", "Caption", "Flag secure");
      ini->SetValue("QuickChatMessagesGroup3_Message3", "Message", "Our flag is secure.");

      ini->SetValue("QuickChatMessagesGroup3_Message4", "Key", keyCodeToString(KEY_H));
      ini->SetValue("QuickChatMessagesGroup3_Message4", "Button", keyCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup3_Message4", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message4", "Caption", "Have enemy flag");
      ini->SetValue("QuickChatMessagesGroup3_Message4", "Message", "I have the enemy flag.");

      ini->SetValue("QuickChatMessagesGroup3_Message5", "Key", keyCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup3_Message5", "Button", keyCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup3_Message5", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message5", "Caption", "Enemy has flag");
      ini->SetValue("QuickChatMessagesGroup3_Message5", "Message", "The enemy has our flag!");

      ini->SetValue("QuickChatMessagesGroup3_Message6", "Key", keyCodeToString(KEY_G));
      ini->SetValue("QuickChatMessagesGroup3_Message6", "Button", keyCodeToString(BUTTON_6));
      ini->SetValue("QuickChatMessagesGroup3_Message6", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message6", "Caption", "Flag gone");
      ini->SetValue("QuickChatMessagesGroup3_Message6", "Message", "Our flag is not in the base!");

   ini->SetValue("QuickChatMessagesGroup4", "Key", keyCodeToString(KEY_S));
   ini->SetValue("QuickChatMessagesGroup4", "Button", keyCodeToString(KEY_UNKNOWN));
   ini->SetValue("QuickChatMessagesGroup4", "MessageType", "Team");
   ini->SetValue("QuickChatMessagesGroup4", "Caption", "Incoming Enemies - Direction");

      ini->SetValue("QuickChatMessagesGroup4_Message1", "Key", keyCodeToString(KEY_S));
      ini->SetValue("QuickChatMessagesGroup4_Message1", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup4_Message1", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup4_Message1", "Caption", "Incoming South");
      ini->SetValue("QuickChatMessagesGroup4_Message1", "Message", "*** INCOMING SOUTH ***");

      ini->SetValue("QuickChatMessagesGroup4_Message2", "Key", keyCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup4_Message2", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup4_Message2", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup4_Message2", "Caption", "Incoming East");
      ini->SetValue("QuickChatMessagesGroup4_Message2", "Message", "*** INCOMING EAST  ***");

      ini->SetValue("QuickChatMessagesGroup4_Message3", "Key", keyCodeToString(KEY_W));
      ini->SetValue("QuickChatMessagesGroup4_Message3", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup4_Message3", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup4_Message3", "Caption", "Incoming West");
      ini->SetValue("QuickChatMessagesGroup4_Message3", "Message", "*** INCOMING WEST  ***");

      ini->SetValue("QuickChatMessagesGroup4_Message4", "Key", keyCodeToString(KEY_N));
      ini->SetValue("QuickChatMessagesGroup4_Message4", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup4_Message4", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup4_Message4", "Caption", "Incoming North");
      ini->SetValue("QuickChatMessagesGroup4_Message4", "Message", "*** INCOMING NORTH ***");

      ini->SetValue("QuickChatMessagesGroup4_Message5", "Key", keyCodeToString(KEY_V));
      ini->SetValue("QuickChatMessagesGroup4_Message5", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup4_Message5", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup4_Message5", "Caption", "Incoming Enemies");
      ini->SetValue("QuickChatMessagesGroup4_Message5", "Message", "Incoming enemies!");

   ini->SetValue("QuickChatMessagesGroup5", "Key", keyCodeToString(KEY_V));
   ini->SetValue("QuickChatMessagesGroup5", "Button", keyCodeToString(BUTTON_3));
   ini->SetValue("QuickChatMessagesGroup5", "MessageType", "Team");
   ini->SetValue("QuickChatMessagesGroup5", "Caption", "Quick");

      ini->SetValue("QuickChatMessagesGroup5_Message1", "Key", keyCodeToString(KEY_J));
      ini->SetValue("QuickChatMessagesGroup5_Message1", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup5_Message1", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message1", "Caption", "Capture the objective");
      ini->SetValue("QuickChatMessagesGroup5_Message1", "Message", "Capture the objective.");

      ini->SetValue("QuickChatMessagesGroup5_Message2", "Key", keyCodeToString(KEY_O));
      ini->SetValue("QuickChatMessagesGroup5_Message2", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup5_Message2", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message2", "Caption", "Go on the offensive");
      ini->SetValue("QuickChatMessagesGroup5_Message2", "Message", "Go on the offensive.");

      ini->SetValue("QuickChatMessagesGroup5_Message3", "Key", keyCodeToString(KEY_A));
      ini->SetValue("QuickChatMessagesGroup5_Message3", "Button", keyCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup5_Message3", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message3", "Caption", "Attack!");
      ini->SetValue("QuickChatMessagesGroup5_Message3", "Message", "Attack!");

      ini->SetValue("QuickChatMessagesGroup5_Message4", "Key", keyCodeToString(KEY_W));
      ini->SetValue("QuickChatMessagesGroup5_Message4", "Button", keyCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup5_Message4", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message4", "Caption", "Wait for signal");
      ini->SetValue("QuickChatMessagesGroup5_Message4", "Message", "Wait for my signal to attack.");

      ini->SetValue("QuickChatMessagesGroup5_Message5", "Key", keyCodeToString(KEY_V));
      ini->SetValue("QuickChatMessagesGroup5_Message5", "Button", keyCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup5_Message5", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message5", "Caption", "Help!");
      ini->SetValue("QuickChatMessagesGroup5_Message5", "Message", "Help!");

      ini->SetValue("QuickChatMessagesGroup5_Message6", "Key", keyCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup5_Message6", "Button", keyCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup5_Message6", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message6", "Caption", "Regroup");
      ini->SetValue("QuickChatMessagesGroup5_Message6", "Message", "Regroup.");

      ini->SetValue("QuickChatMessagesGroup5_Message7", "Key", keyCodeToString(KEY_G));
      ini->SetValue("QuickChatMessagesGroup5_Message7", "Button", keyCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup5_Message7", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message7", "Caption", "Going offense");
      ini->SetValue("QuickChatMessagesGroup5_Message7", "Message", "Going offense.");

      ini->SetValue("QuickChatMessagesGroup5_Message8", "Key", keyCodeToString(KEY_Z));
      ini->SetValue("QuickChatMessagesGroup5_Message8", "Button", keyCodeToString(BUTTON_6));
      ini->SetValue("QuickChatMessagesGroup5_Message8", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message8", "Caption", "Move out");
      ini->SetValue("QuickChatMessagesGroup5_Message8", "Message", "Move out.");

   ini->SetValue("QuickChatMessagesGroup6", "Key", keyCodeToString(KEY_R));
   ini->SetValue("QuickChatMessagesGroup6", "Button", keyCodeToString(BUTTON_2));
   ini->SetValue("QuickChatMessagesGroup6", "MessageType", "Team");
   ini->SetValue("QuickChatMessagesGroup6", "Caption", "Reponses");

      ini->SetValue("QuickChatMessagesGroup6_Message1", "Key", keyCodeToString(KEY_A));
      ini->SetValue("QuickChatMessagesGroup6_Message1", "Button", keyCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup6_Message1", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message1", "Caption", "Acknowledge");
      ini->SetValue("QuickChatMessagesGroup6_Message1", "Message", "Acknowledged.");

      ini->SetValue("QuickChatMessagesGroup6_Message2", "Key", keyCodeToString(KEY_N));
      ini->SetValue("QuickChatMessagesGroup6_Message2", "Button", keyCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup6_Message2", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message2", "Caption", "No");
      ini->SetValue("QuickChatMessagesGroup6_Message2", "Message", "No.");

      ini->SetValue("QuickChatMessagesGroup6_Message3", "Key", keyCodeToString(KEY_Y));
      ini->SetValue("QuickChatMessagesGroup6_Message3", "Button", keyCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup6_Message3", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message3", "Caption", "Yes");
      ini->SetValue("QuickChatMessagesGroup6_Message3", "Message", "Yes.");

      ini->SetValue("QuickChatMessagesGroup6_Message4", "Key", keyCodeToString(KEY_S));
      ini->SetValue("QuickChatMessagesGroup6_Message4", "Button", keyCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup6_Message4", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message4", "Caption", "Sorry");
      ini->SetValue("QuickChatMessagesGroup6_Message4", "Message", "Sorry.");

      ini->SetValue("QuickChatMessagesGroup6_Message5", "Key", keyCodeToString(KEY_T));
      ini->SetValue("QuickChatMessagesGroup6_Message5", "Button", keyCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup6_Message5", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message5", "Caption", "Thanks");
      ini->SetValue("QuickChatMessagesGroup6_Message5", "Message", "Thanks.");

      ini->SetValue("QuickChatMessagesGroup6_Message6", "Key", keyCodeToString(KEY_D));
      ini->SetValue("QuickChatMessagesGroup6_Message6", "Button", keyCodeToString(BUTTON_6));
      ini->SetValue("QuickChatMessagesGroup6_Message6", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message6", "Caption", "Don't know");
      ini->SetValue("QuickChatMessagesGroup6_Message6", "Message", "I don't know.");

   ini->SetValue("QuickChatMessagesGroup7", "Key", keyCodeToString(KEY_T));
   ini->SetValue("QuickChatMessagesGroup7", "Button", keyCodeToString(BUTTON_1));
   ini->SetValue("QuickChatMessagesGroup7", "MessageType", "Global");
   ini->SetValue("QuickChatMessagesGroup7", "Caption", "Taunts");

      ini->SetValue("QuickChatMessagesGroup7_Message1", "Key", keyCodeToString(KEY_R));
      ini->SetValue("QuickChatMessagesGroup7_Message1", "Button", keyCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup7_Message1", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message1", "Caption", "Rawr");
      ini->SetValue("QuickChatMessagesGroup7_Message1", "Message", "RAWR!");

      ini->SetValue("QuickChatMessagesGroup7_Message2", "Key", keyCodeToString(KEY_C));
      ini->SetValue("QuickChatMessagesGroup7_Message2", "Button", keyCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup7_Message2", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message2", "Caption", "Come get some!");
      ini->SetValue("QuickChatMessagesGroup7_Message2", "Message", "Come get some!");

      ini->SetValue("QuickChatMessagesGroup7_Message3", "Key", keyCodeToString(KEY_D));
      ini->SetValue("QuickChatMessagesGroup7_Message3", "Button", keyCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup7_Message3", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message3", "Caption", "Dance!");
      ini->SetValue("QuickChatMessagesGroup7_Message3", "Message", "Dance!");

      ini->SetValue("QuickChatMessagesGroup7_Message4", "Key", keyCodeToString(KEY_X));
      ini->SetValue("QuickChatMessagesGroup7_Message4", "Button", keyCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup7_Message4", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message4", "Caption", "Missed me!");
      ini->SetValue("QuickChatMessagesGroup7_Message4", "Message", "Missed me!");

      ini->SetValue("QuickChatMessagesGroup7_Message5", "Key", keyCodeToString(KEY_W));
      ini->SetValue("QuickChatMessagesGroup7_Message5", "Button", keyCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup7_Message5", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message5", "Caption", "I've had worse...");
      ini->SetValue("QuickChatMessagesGroup7_Message5", "Message", "I've had worse...");

      ini->SetValue("QuickChatMessagesGroup7_Message6", "Key", keyCodeToString(KEY_Q));
      ini->SetValue("QuickChatMessagesGroup7_Message6", "Button", keyCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup7_Message6", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message6", "Caption", "How'd THAT feel?");
      ini->SetValue("QuickChatMessagesGroup7_Message6", "Message", "How'd THAT feel?");

      ini->SetValue("QuickChatMessagesGroup7_Message7", "Key", keyCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup7_Message7", "Button", keyCodeToString(BUTTON_6));
      ini->SetValue("QuickChatMessagesGroup7_Message7", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message7", "Caption", "Yoohoo!");
      ini->SetValue("QuickChatMessagesGroup7_Message7", "Message", "Yoohoo!");
}

// TODO:  reimplement joystick mapping methods for future custom joystick
// mappings with axes as well as buttons
//void readJoystick()
//{
//   gJoystickMapping.enable = ini->GetValueYN("Joystick", "Enable", false);
//   for(U32 i=0; i<MaxJoystickAxes*2; i++)
//   {
//      Vector<string> buttonList;
//      parseString(ini->GetValue("Joystick", "Axes" + itos(i), i<8 ? itos(i+16) : "").c_str(), buttonList, ',');
//      gJoystickMapping.axes[i] = 0;
//      for(S32 j=0; j<buttonList.size(); j++)
//      {
//         gJoystickMapping.axes[i] |= 1 << atoi(buttonList[j].c_str());
//      }
//   }
//   for(U32 i=0; i<32; i++)
//   {
//      Vector<string> buttonList;
//      parseString(ini->GetValue("Joystick", "Button" + itos(i), i<10 ? itos(i) : "").c_str(), buttonList, ',');
//      gJoystickMapping.button[i] = 0;
//      for(S32 j=0; j<buttonList.size(); j++)
//      {
//         gJoystickMapping.button[i] |= 1 << atoi(buttonList[j].c_str());
//      }
//   }
//   for(U32 i=0; i<4; i++)
//   {
//      Vector<string> buttonList;
//      parseString(ini->GetValue("Joystick", "Pov" + itos(i), itos(i+10)).c_str(), buttonList, ',');
//      gJoystickMapping.pov[i] = 0;
//      for(S32 j=0; j<buttonList.size(); j++)
//      {
//         gJoystickMapping.pov[i] |= 1 << atoi(buttonList[j].c_str());
//      }
//   }
//}
//
//void writeJoystick()
//{
//   ini->setValueYN("Joystick", "Enable", gJoystickMapping.enable);
//   ///for(listToString(alwaysPingList, ',')
//   for(U32 i=0; i<MaxJoystickAxes*2; i++)
//   {
//      Vector<string> buttonList;
//      for(U32 j=0; j<32; j++)
//      {
//         if(gJoystickMapping.axes[i] & (1 << j))
//            buttonList.push_back(itos(j));
//      }
//      ini->SetValue("Joystick", "Axes" + itos(i), listToString(buttonList, ','));
//   }
//   for(U32 i=0; i<32; i++)
//   {
//      Vector<string> buttonList;
//      for(U32 j=0; j<32; j++)
//      {
//         if(gJoystickMapping.button[i] & (1 << j))
//            buttonList.push_back(itos(j));
//      }
//      ini->SetValue("Joystick", "Button" + itos(i), listToString(buttonList, ','));
//   }
//   for(U32 i=0; i<4; i++)
//   {
//      Vector<string> buttonList;
//      for(U32 j=0; j<32; j++)
//      {
//         if(gJoystickMapping.pov[i] & (1 << j))
//            buttonList.push_back(itos(j));
//      }
//      ini->SetValue("Joystick", "Pov" + itos(i), listToString(buttonList, ','));
//   }
//}


// Option default values are stored here, in the 3rd prarm of the GetValue call
// This is only called once, during initial initialization
void loadSettingsFromINI(CIniFile *ini, GameSettings *settings)
{
   ini->ReadFile();        // Read the INI file  (journaling of read lines happens within)

   loadSoundSettings(ini);
   loadEffectsSettings(ini);
   loadGeneralSettings(ini);
   loadHostConfiguration(ini);
   loadUpdaterSettings(ini);
   loadDiagnostics(ini);

   loadTestSettings(ini);

   loadKeyBindings(ini);
   loadForeignServerInfo(ini);         // Info about other servers
   loadLevels(ini);                    // Read levels, if there are any
   loadLevelSkipList(ini, settings);   // Read level skipList, if there are any

   loadQuickChatMessages(ini);

//   readJoystick();

   saveSettingsToINI(ini, settings);    // Save to fill in any missing settings
}


static void writeDiagnostics(CIniFile *ini)
{
   const char *section = "Diagnostics";
   ini->addSection(section);

   if (ini->numSectionComments(section) == 0)
   {
      ini->sectionComment(section, "----------------");
      ini->sectionComment(section, " Diagnostic entries can be used to enable or disable particular actions for debugging purposes.");
      ini->sectionComment(section, " You probably can't use any of these settings to enhance your gameplay experience!");
      ini->sectionComment(section, " DumpKeys - Enable this to dump raw input to the screen (Yes/No)");
      ini->sectionComment(section, " LogConnectionProtocol - Log ConnectionProtocol events (Yes/No)");
      ini->sectionComment(section, " LogNetConnection - Log NetConnectionEvents (Yes/No)");
      ini->sectionComment(section, " LogEventConnection - Log EventConnection events (Yes/No)");
      ini->sectionComment(section, " LogGhostConnection - Log GhostConnection events (Yes/No)");
      ini->sectionComment(section, " LogNetInterface - Log NetInterface events (Yes/No)");
      ini->sectionComment(section, " LogPlatform - Log Platform events (Yes/No)");
      ini->sectionComment(section, " LogNetBase - Log NetBase events (Yes/No)");
      ini->sectionComment(section, " LogUDP - Log UDP events (Yes/No)");

      ini->sectionComment(section, " LogFatalError - Log fatal errors; should be left on (Yes/No)");
      ini->sectionComment(section, " LogError - Log serious errors; should be left on (Yes/No)");
      ini->sectionComment(section, " LogWarning - Log less serious errors (Yes/No)");
      ini->sectionComment(section, " LogConnection - High level logging connections with remote machines (Yes/No)");
      ini->sectionComment(section, " LogLevelLoaded - Write a log entry when a level is loaded (Yes/No)");
      ini->sectionComment(section, " LogLuaObjectLifecycle - Creation and destruciton of lua objects (Yes/No)");
      ini->sectionComment(section, " LuaLevelGenerator - Messages from the LuaLevelGenerator (Yes/No)");
      ini->sectionComment(section, " LuaBotMessage - Message from a bot (Yes/No)");
      ini->sectionComment(section, " ServerFilter - For logging messages specific to hosting games (Yes/No)");
      ini->sectionComment(section, "                (Note: these messages will go to bitfighter_server.log regardless of this setting) ");
      ini->sectionComment(section, "----------------");
   }

   ini->setValueYN(section, "DumpKeys", gIniSettings.diagnosticKeyDumpMode);
   ini->setValueYN(section, "LogConnectionProtocol", gIniSettings.logConnectionProtocol);
   ini->setValueYN(section, "LogNetConnection",      gIniSettings.logNetConnection);
   ini->setValueYN(section, "LogEventConnection",    gIniSettings.logEventConnection);
   ini->setValueYN(section, "LogGhostConnection",    gIniSettings.logGhostConnection);
   ini->setValueYN(section, "LogNetInterface",       gIniSettings.logNetInterface);
   ini->setValueYN(section, "LogPlatform",           gIniSettings.logPlatform);
   ini->setValueYN(section, "LogNetBase",            gIniSettings.logNetBase);
   ini->setValueYN(section, "LogUDP",                gIniSettings.logUDP);

   ini->setValueYN(section, "LogFatalError",         gIniSettings.logFatalError);
   ini->setValueYN(section, "LogError",              gIniSettings.logError);
   ini->setValueYN(section, "LogWarning",            gIniSettings.logWarning);
   ini->setValueYN(section, "LogConnection",         gIniSettings.logConnection);
   ini->setValueYN(section, "LogLevelLoaded",        gIniSettings.logLevelLoaded);
   ini->setValueYN(section, "LogLuaObjectLifecycle", gIniSettings.logLuaObjectLifecycle);
   ini->setValueYN(section, "LuaLevelGenerator",     gIniSettings.luaLevelGenerator);
   ini->setValueYN(section, "LuaBotMessage",         gIniSettings.luaBotMessage);
   ini->setValueYN(section, "ServerFilter",          gIniSettings.serverFilter);
}


static void writeEffects(CIniFile *ini)
{
   const char *section = "Effects";
   ini->addSection(section);

   if (ini->numSectionComments(section) == 0)
   {
      ini->sectionComment(section, "----------------");
      ini->sectionComment(section, " Various visual effects");
      ini->sectionComment(section, " StarsInDistance - Yes gives the game a floating, 3-D effect.  No gives the flat 'classic zap' mode.");
      ini->sectionComment(section, " LineSmoothing - Yes activates anti-aliased rendering.  This may be a little slower on some machines.");
      ini->sectionComment(section, "----------------");
   }

   ini->setValueYN(section, "StarsInDistance", gIniSettings.starsInDistance);
   ini->setValueYN(section, "LineSmoothing",   gIniSettings.useLineSmoothing);
}

static void writeSounds(CIniFile *ini)
{
   ini->addSection("Sounds");

   if (ini->numSectionComments("Sounds") == 0)
   {
      ini->sectionComment("Sounds", "----------------");
      ini->sectionComment("Sounds", " Sound settings");
      ini->sectionComment("Sounds", " EffectsVolume - Volume of sound effects from 0 (mute) to 10 (full bore)");
      ini->sectionComment("Sounds", " MusicVolume - Volume of sound effects from 0 (mute) to 10 (full bore)");
      ini->sectionComment("Sounds", " VoiceChatVolume - Volume of incoming voice chat messages from 0 (mute) to 10 (full bore)");
      ini->sectionComment("Sounds", " SFXSet - Select which set of sounds you want: Classic or Modern");
      ini->sectionComment("Sounds", "----------------");
   }

   ini->SetValueI("Sounds", "EffectsVolume", (S32) (gIniSettings.sfxVolLevel * 10));
   ini->SetValueI("Sounds", "MusicVolume",   (S32) (gIniSettings.musicVolLevel * 10));
   ini->SetValueI("Sounds", "VoiceChatVolume",   (S32) (gIniSettings.voiceChatVolLevel * 10));

   ini->SetValue("Sounds", "SFXSet", gIniSettings.sfxSet == sfxClassicSet ? "Classic" : "Modern");
}


void saveWindowMode(CIniFile *ini)
{
   ini->SetValue("Settings",  "WindowMode", displayModeToString(gIniSettings.displayMode));
}


void saveWindowPosition(CIniFile *ini, S32 x, S32 y)
{
   ini->SetValueI("Settings", "WindowXPos", x);
   ini->SetValueI("Settings", "WindowYPos", y);
}


static void writeSettings(CIniFile *ini)
{
   const char *section = "Settings";
   ini->addSection(section);

   if (ini->numSectionComments(section) == 0)
   {
      ini->sectionComment(section, "----------------");
      ini->sectionComment(section, " Settings entries contain a number of different options");
      ini->sectionComment(section, " WindowMode - Fullscreen, Fullscreen-Stretch or Window");
      ini->sectionComment(section, " WindowXPos, WindowYPos - Position of window in window mode (will overwritten if you move your window)");
      ini->sectionComment(section, " WindowScalingFactor - Used to set size of window.  1.0 = 800x600. Best to let the program manage this setting.");
      ini->sectionComment(section, " VoiceEcho - Play echo when recording a voice message? Yes/No");
      ini->sectionComment(section, " ControlMode - Use Relative or Absolute controls (Relative means left is ship's left, Absolute means left is screen left)");
      ini->sectionComment(section, " LoadoutIndicators - Display indicators showing current weapon?  Yes/No");
      ini->sectionComment(section, " VerboseHelpMessages - Display additional on-screen messages while learning the game?  Yes/No");
      ini->sectionComment(section, " ShowKeyboardKeysInStickMode - If you are using a joystick, also show keyboard shortcuts in Loadout and QuickChat menus");
      ini->sectionComment(section, " JoystickType - Type of joystick to use if auto-detect doesn't recognize your controller");
      ini->sectionComment(section, " MasterServerAddressList - Comma seperated list of Address of master server, in form: IP:67.18.11.66:25955,IP:myMaster.org:25955 (tries all listed, only connects to one at a time)");
      ini->sectionComment(section, " DefaultName - Name that will be used if user hits <enter> on name entry screen without entering one");
      ini->sectionComment(section, " Nickname - Specify your nickname to bypass the name entry screen altogether");
      ini->sectionComment(section, " Password - Password to use if your nickname has been reserved in the forums");
      ini->sectionComment(section, " EnableExperimentalAimMode - Use experimental aiming system (works only with controller) Yes/No");
      ini->sectionComment(section, " LastName - Name user entered when game last run (may be overwritten if you enter a different name on startup screen)");
      ini->sectionComment(section, " LastPassword - Password user entered when game last run (may be overwritten if you enter a different pw on startup screen)");
      ini->sectionComment(section, " LastEditorName - Last edited file name");
      ini->sectionComment(section, " MaxFPS - Maximum FPS the client will run at.  Higher values use more CPU, lower may increase lag (default = 100)");
      ini->sectionComment(section, " LineWidth - Width of a \"standard line\" in pixels (default 2); can set with /linewidth in game ");
      ini->sectionComment(section, " Version - Version of game last time it was run.  Don't monkey with this value; nothing good can come of it!");
      ini->sectionComment(section, "----------------");
   }
   saveWindowMode(ini);
   saveWindowPosition(ini, gIniSettings.winXPos, gIniSettings.winYPos);

   ini->SetValueF (section, "WindowScalingFactor", gIniSettings.winSizeFact);
   ini->setValueYN(section, "VoiceEcho", gIniSettings.echoVoice );
   ini->SetValue  (section, "ControlMode", (gIniSettings.controlsRelative ? "Relative" : "Absolute"));

   // inputMode is not saved, but rather determined at runtime by whether a joystick is attached

   ini->setValueYN(section, "LoadoutIndicators", gIniSettings.showWeaponIndicators);
   ini->setValueYN(section, "VerboseHelpMessages", gIniSettings.verboseHelpMessages);
   ini->setValueYN(section, "ShowKeyboardKeysInStickMode", gIniSettings.showKeyboardKeys);

#ifndef ZAP_DEDICATED
   ini->SetValue  (section, "JoystickType", Joystick::joystickTypeToString(gIniSettings.joystickType));
#endif
   ini->SetValue  (section, "MasterServerAddressList", gIniSettings.masterAddress);
   ini->SetValue  (section, "DefaultName", gIniSettings.defaultName);
   ini->SetValue  (section, "LastName", gIniSettings.lastName);
   ini->SetValue  (section, "LastPassword", gIniSettings.lastPassword);
   ini->SetValue  (section, "LastEditorName", gIniSettings.lastEditorName);

   ini->setValueYN(section, "EnableExperimentalAimMode", gIniSettings.enableExperimentalAimMode);
   ini->SetValueI (section, "MaxFPS", gIniSettings.maxFPS);  

   ini->SetValueI (section, "Version", BUILD_VERSION);

#ifndef ZAP_DEDICATED
   // Don't save new value if out of range, so it will go back to the old value. Just in case a user screw up with /linewidth command using value too big or too small
   if(gDefaultLineWidth >= 0.5 && gDefaultLineWidth <= 8)
      ini->SetValueF (section, "LineWidth", gDefaultLineWidth);
#endif
}


static void writeUpdater(CIniFile *ini)
{
   ini->addSection("Updater");

   if(ini->numSectionComments("Updater") == 0)
   {
      ini->sectionComment("Updater", "----------------");
      ini->sectionComment("Updater", " The Updater section contains entries that control how game updates are handled");
      ini->sectionComment("Updater", " UseUpdater - Enable or disable process that installs updates (WINDOWS ONLY)");
      ini->sectionComment("Updater", "----------------");

   }
   ini->setValueYN("Updater", "UseUpdater", gIniSettings.useUpdater, true);
}

// TEST!!
// Does this macro def make it easier to read the code?
#define addComment(comment) ini->sectionComment(section, comment);

static void writeHost(CIniFile *ini)
{
   const char *section = "Host";
   ini->addSection(section);

   if(ini->numSectionComments(section) == 0)
   {
      addComment("----------------");
      addComment(" The Host section contains entries that configure the game when you are hosting");
      addComment(" ServerName - The name others will see when they are browsing for servers (max 20 chars)");
      addComment(" ServerAddress - The address of your server, e.g. IP:localhost:1234 or IP:54.35.110.99:8000 or IP:bitfighter.org:8888 (leave blank to let the system decide)");
      addComment(" ServerDescription - A one line description of your server.  Please include nickname and physical location!");
      addComment(" ServerPassword - You can require players to use a password to play on your server.  Leave blank to grant access to all.");
      addComment(" AdminPassword - Use this password to manage players & change levels on your server.");
      addComment(" LevelChangePassword - Use this password to change levels on your server.  Leave blank to grant access to all.");
      addComment(" LevelDir - Specify where level files are stored; can be overridden on command line with -leveldir param.");
      addComment(" MaxPlayers - The max number of players that can play on your server.");
      addComment(" MaxBots - The max number of bots allowed on this server.");
      addComment(" AlertsVolume - Volume of audio alerts when players join or leave game from 0 (mute) to 10 (full bore).");
      addComment(" MaxFPS - Maximum FPS the dedicaetd server will run at.  Higher values use more CPU, lower may increase lag (default = 100).");
      addComment(" AllowGetMap - When getmap is allowed, anyone can download the current level using the /getmap command.");
      addComment(" AllowDataConnections - When data connections are allowed, anyone with the admin password can upload or download levels, bots, or");
      addComment("                        levelGen scripts.  This feature is probably insecure, and should be DISABLED unless you require the functionality.");
      addComment(" LogStats - Save game stats locally to built-in sqlite database (saves the same stats as are sent to the master)");
      addComment(" DefaultRobotScript - If user adds a robot, this script is used if none is specified");
      addComment(" MySqlStatsDatabaseCredentials - If MySql integration has been compiled in (which it probably hasn't been), you can specify the");
      addComment("                                 database server, database name, login, and password as a comma delimeted list");
      addComment(" VoteLength - number of seconds the voting will last, zero will disable voting.");
      addComment(" VoteRetryLength - When vote fail, the vote caller is unable to vote until after this number of seconds.");
      addComment(" Vote Strengths - Vote will pass when sum of all vote strengths is bigger then zero.");
      addComment("----------------");
   }

   ini->SetValue  (section, "ServerName", gIniSettings.hostname);
   ini->SetValue  (section, "ServerAddress", gIniSettings.hostaddr);
   ini->SetValue  (section, "ServerDescription", gIniSettings.hostdescr);
   ini->SetValue  (section, "ServerPassword", gIniSettings.serverPassword);
   ini->SetValue  (section, "AdminPassword", gIniSettings.adminPassword);
   ini->SetValue  (section, "LevelChangePassword", gIniSettings.levelChangePassword);
   ini->SetValue  (section, "LevelDir", gIniSettings.levelDir);
   ini->SetValueI (section, "MaxPlayers", gIniSettings.maxPlayers);
   ini->SetValueI (section, "MaxBots", gIniSettings.maxBots);
   ini->SetValueI (section, "AlertsVolume", (S32) (gIniSettings.alertsVolLevel * 10));
   ini->setValueYN(section, "AllowGetMap", gIniSettings.allowGetMap);
   ini->setValueYN(section, "AllowDataConnections", gIniSettings.allowDataConnections);
   ini->SetValueI (section, "MaxFPS", gIniSettings.maxDedicatedFPS);
   ini->setValueYN(section, "LogStats", gIniSettings.logStats);

   ini->setValueYN(section, "AllowMapUpload", S32(gIniSettings.allowMapUpload) );
   ini->setValueYN(section, "AllowAdminMapUpload", S32(gIniSettings.allowAdminMapUpload) );


   ini->setValueYN(section, "VoteEnable", S32(gIniSettings.voteEnable) );
   ini->SetValueI(section, "VoteLength", S32(gIniSettings.voteLength) );
   ini->SetValueI(section, "VoteLengthToChangeTeam", S32(gIniSettings.voteLengthToChangeTeam) );
   ini->SetValueI(section, "VoteRetryLength", S32(gIniSettings.voteRetryLength) );
   ini->SetValueI(section, "VoteYesStrength", gIniSettings.voteYesStrength );
   ini->SetValueI(section, "VoteNoStrength", gIniSettings.voteNoStrength );
   ini->SetValueI(section, "VoteNothingStrength", gIniSettings.voteNothingStrength );

   ini->SetValue  (section, "DefaultRobotScript", gIniSettings.defaultRobotScript);
   ini->SetValue  (section, "GlobalLevelScript", gIniSettings.globalLevelScript );
#ifdef BF_WRITE_TO_MYSQL
   if(gIniSettings.mySqlStatsDatabaseServer == "" && gIniSettings.mySqlStatsDatabaseName == "" && gIniSettings.mySqlStatsDatabaseUser == "" && gIniSettings.mySqlStatsDatabasePassword == "")
      ini->SetValue  (section, "MySqlStatsDatabaseCredentials", "server, dbname, login, password");
#endif
}


static void writeLevels(CIniFile *ini)
{
   // If there is no Levels key, we'll add it here.  Otherwise, we'll do nothing so as not to clobber an existing value
   // We'll write the default level list (which may have been overridden by the cmd line) because there are no levels in the INI
   if(ini->findSection("Levels") == ini->noID)    // Section doesn't exist... let's write one
      ini->addSection("Levels");              

   if(ini->numSectionComments("Levels") == 0)
   {
      ini->sectionComment("Levels", "----------------");
      ini->sectionComment("Levels", " All levels in this section will be loaded when you host a game in Server mode.");
      ini->sectionComment("Levels", " You can call the level keys anything you want (within reason), and the levels will be sorted");
      ini->sectionComment("Levels", " by key name and will appear in that order, regardless of the order the items are listed in.");
      ini->sectionComment("Levels", " Example:");
      ini->sectionComment("Levels", " Level1=ctf.level");
      ini->sectionComment("Levels", " Level2=zonecontrol.level");
      ini->sectionComment("Levels", " ... etc ...");
      ini->sectionComment("Levels", "This list can be overidden on the command line with the -leveldir, -rootdatadir, or -levels parameters.");
      ini->sectionComment("Levels", "----------------");
   }
}


static void writeTesting(CIniFile *ini)
{
   ini->addSection("Testing");
   if (ini->numSectionComments("Testing") == 0)
   {
      ini->sectionComment("Testing", "----------------");
      ini->sectionComment("Testing", " These settings are here to enable/disable certain items for testing.  They are by their nature");
      ini->sectionComment("Testing", " short lived, and may well be removed in the next version of Bitfighter.");
      ini->sectionComment("Testing", " BurstGraphics - Select which graphic to use for bursts (1-5)");
      ini->sectionComment("Testing", " NeverConnectDirect - Never connect to pingable internet server directly; forces arranged connections via master");
      ini->sectionComment("Testing", " WallOutlineColor - Color used locally for rendering wall outlines (r,g,b), (values between 0 and 1)");
      ini->sectionComment("Testing", " WallFillColor - Color used locally for rendering wall fill (r,g,b), (values between 0 and 1)");
      ini->sectionComment("Testing", "----------------");
   }

   ini->SetValueI ("Testing", "BurstGraphics",  (S32) (gIniSettings.burstGraphicsMode), true);
   ini->setValueYN("Testing", "NeverConnectDirect", gIniSettings.neverConnectDirect);
   ini->SetValue  ("Testing", "WallFillColor",   gIniSettings.wallFillColor.toRGBString());
   ini->SetValue  ("Testing", "WallOutlineColor", gIniSettings.wallOutlineColor.toRGBString());
   ini->setValueYN("Testing", "OldGoalFlash", gIniSettings.oldGoalFlash);
}


static void writePasswordSection_helper(CIniFile *ini, string section)
{
   ini->addSection(section);
   if (ini->numSectionComments(section) == 0)
   {
      ini->sectionComment(section, "----------------");
      ini->sectionComment(section, " This section holds passwords you've entered to gain access to various servers.");
      ini->sectionComment(section, "----------------");
   }
}

static void writePasswordSection(CIniFile *ini)
{
   writePasswordSection_helper(ini, "SavedLevelChangePasswords");
   writePasswordSection_helper(ini, "SavedAdminPasswords");
   writePasswordSection_helper(ini, "SavedServerPasswords");
}


static void writeINIHeader(CIniFile *ini)
{
   if(!ini->NumHeaderComments())
   {
      ini->headerComment("Bitfighter configuration file");
      ini->headerComment("=============================");
      ini->headerComment(" This file is intended to be user-editable, but some settings here may be overwritten by the game.");
      ini->headerComment(" If you specify any cmd line parameters that conflict with these settings, the cmd line options will be used.");
      ini->headerComment(" First, some basic terminology:");
      ini->headerComment(" [section]");
      ini->headerComment(" key=value");
      ini->headerComment("");
   }
}


// Save more commonly altered settings first to make them easier to find
void saveSettingsToINI(CIniFile *ini, GameSettings *settings)
{
   writeINIHeader(ini);

   writeHost(ini);
   writeForeignServerInfo(ini);
   writeConnectionsInfo(ini);
   writeEffects(ini);
   writeSounds(ini);
   writeSettings(ini);
   writeDiagnostics(ini);
   writeLevels(ini);
   writeSkipList(ini, settings->getLevelSkipList());
   writeUpdater(ini);
   writeTesting(ini);
   writePasswordSection(ini);
   writeKeyBindings(ini);
   
   writeDefaultQuickChatMessages(ini);    // Does nothing if there are already chat messages in the INI

      // only needed for users using custom joystick 
      // or joystick that maps differenly in LINUX
      // This adds 200+ lines.
   //writeJoystick();

   ini->WriteFile();
}


// Can't be static -- called externally!
void writeSkipList(CIniFile *ini, const Vector<string> *levelSkipList)
{
   // If there is no LevelSkipList key, we'll add it here.  Otherwise, we'll do nothing so as not to clobber an existing value
   // We'll write our current skip list (which may have been specified via remote server management tools)

   ini->deleteSection("LevelSkipList");   // Delete all current entries to prevent user renumberings to be corrected from tripping us up
                                          // This may the unfortunate side-effect of pushing this section to the bottom of the INI file

   ini->addSection("LevelSkipList");      // Create the key, then provide some comments for documentation purposes

   ini->sectionComment("LevelSkipList", "----------------");
   ini->sectionComment("LevelSkipList", " Levels listed here will be skipped and will NOT be loaded, even when they are specified in");
   ini->sectionComment("LevelSkipList", " on the command line.  You can edit this section, but it is really intended for remote");
   ini->sectionComment("LevelSkipList", " server management.  You will experience slightly better load times if you clean this section");
   ini->sectionComment("LevelSkipList", " out from time to time.  The names of the keys are not important, and may be changed.");
   ini->sectionComment("LevelSkipList", " Example:");
   ini->sectionComment("LevelSkipList", " SkipLevel1=skip_me.level");
   ini->sectionComment("LevelSkipList", " SkipLevel2=dont_load_me_either.level");
   ini->sectionComment("LevelSkipList", " ... etc ...");
   ini->sectionComment("LevelSkipList", "----------------");

   Vector<string> normalizedSkipList;

   for(S32 i = 0; i < levelSkipList->size(); i++)
   {
      // "Normalize" the name a little before writing it
      string filename = lcase(levelSkipList->get(i));
      if(filename.find(".level") == string::npos)
         filename += ".level";

      normalizedSkipList.push_back(filename);
   }

   ini->SetAllValues("LevelSkipList", "SkipLevel", normalizedSkipList);
}


//////////////////////////////////
//////////////////////////////////

extern CmdLineSettings gCmdLineSettings;

static string resolutionHelper(const string &cmdLineDir, const string &rootDataDir, const string &subdir)
{
   if(cmdLineDir != "")             // Direct specification of ini path takes precedence...
      return cmdLineDir;
   else if(rootDataDir != "")       // ...over specification via rootdatadir param
      return joindir(rootDataDir, subdir);
   else 
      return subdir;
}


extern string gSqlite;

// Doesn't handle leveldir -- that one is handled separately, later, because it requires input from the INI file
void ConfigDirectories::resolveDirs(GameSettings *settings)
{
   ConfigDirectories *configDirs = settings->getConfigDirs();
   string rootDataDir = gCmdLineSettings.dirs.rootDataDir;

   // rootDataDir used to specify the following folders
   configDirs->robotDir      = resolutionHelper(gCmdLineSettings.dirs.robotDir,      rootDataDir, "robots");
   configDirs->iniDir        = resolutionHelper(gCmdLineSettings.dirs.iniDir,        rootDataDir, "");
   configDirs->logDir        = resolutionHelper(gCmdLineSettings.dirs.logDir,        rootDataDir, "");
   configDirs->screenshotDir = resolutionHelper(gCmdLineSettings.dirs.screenshotDir, rootDataDir, "screenshots");

   // rootDataDir not used for these folders
   configDirs->cacheDir      = resolutionHelper(gCmdLineSettings.dirs.cacheDir,      "", "cache");
   configDirs->luaDir        = resolutionHelper(gCmdLineSettings.dirs.luaDir,        "", "scripts");
   configDirs->sfxDir        = resolutionHelper(gCmdLineSettings.dirs.sfxDir,        "", "sfx");
   configDirs->musicDir      = resolutionHelper(gCmdLineSettings.dirs.musicDir,      "", "music");

   gSqlite = configDirs->logDir + "stats";
}


// Figure out where the levels are.  This is exceedingly complex.
//
// Here are the rules:
//
// rootDataDir is specified on the command line via the -rootdatadir parameter
// levelDir is specified on the command line via the -leveldir parameter
// iniLevelDir is specified in the INI file
//
// Prioritize command line over INI setting, and -leveldir over -rootdatadir
//
// If levelDir exists, just use it (ignoring rootDataDir)
// ...Otherwise...
//
// If rootDataDir is specified then try
//       If levelDir is also specified try
//            rootDataDir/levels/levelDir
//            rootDataDir/levelDir
//       End
//   
//       rootDataDir/levels
// End      ==> Don't use rootDataDir
//      
// If iniLevelDir is specified
//       If levelDir is also specified try
//            iniLevelDir/levelDir
//     End   
//     iniLevelDir
// End    ==> Don't use iniLevelDir
//      
// levels
//
// If none of the above work, no hosting/editing for you!
//
// NOTE: See above for full explanation of what these functions are doing
string ConfigDirectories::resolveLevelDir(const string &levelDir)    
{
   if(levelDir != "")
      if(fileExists(levelDir))     // Check for a valid absolute path in levelDir
         return levelDir;

   if(rootDataDir != "")
   {
      if(levelDir != "")
      {
         string candidate = strictjoindir(rootDataDir, "levels", levelDir);
         if(fileExists(candidate))
            return candidate;

         candidate = strictjoindir(rootDataDir, levelDir);
         if(fileExists(candidate))
            return candidate;
      }
   }

   return "";
}


string ConfigDirectories::resolveLevelDir(const string &levelDir, const string &iniLevelDir)     // static
{
   string resolved = resolveLevelDir(levelDir);
   if(resolved != "")
      return resolved;

   if(rootDataDir != "")
   {
      string candidate = strictjoindir(rootDataDir, "levels");
      if(fileExists(candidate))   // Try rootDataDir/levels
         return candidate;
   }

   // rootDataDir is blank, or nothing using it worked
   if(iniLevelDir != "")
   {
      string candidate;

      if(levelDir != "")
      {
         candidate = strictjoindir(iniLevelDir, levelDir);
         if(fileExists(candidate))
            return candidate;
      }
      
      if(fileExists(iniLevelDir))
         return iniLevelDir;
   }

   if(fileExists("levels"))
      return "levels";

   return "";     // Surrender
}


#ifdef false
// WARNING: May not still work...
static void testLevelDirResolution(ConfigDirectories *configDirs)
{
   // These need to exist!
   // c:\temp\leveldir
   // c:\temp\leveldir2\levels\leveldir
   // For last test to work, need to run from folder that has no levels subdir

   /* 
   cd \temp
   mkdir leveldir
   mkdir leveldir2
   cd leveldir2
   mkdir levels
   cd levels
   mkdir leveldir
   */
   
   // rootDataDir, levelDir, iniLevelDir
   resolveLevelDir("c:/temp", "leveldir", "c:/ini/level/dir/");       // rootDataDir/levelDir
   TNLAssertV(configDirs->levelDir == "c:/temp/leveldir", ("Bad leveldir: %s", configDirs->levelDir.c_str()));

   resolveLevelDir("c:/temp/leveldir2", "leveldir", "c:/ini/level/dir/");       // rootDataDir/levels/levelDir
   TNLAssertV(configDirs->levelDir == "c:/temp/leveldir2/levels/leveldir", ("Bad leveldir: %s", configDirs->levelDir.c_str()));

   resolveLevelDir("c:/temp/leveldir2", "c:/temp/leveldir", "c:/ini/level/dir/");       // levelDir
   TNLAssertV(configDirs->levelDir == "c:/temp/leveldir", ("Bad leveldir: %s", configDirs->levelDir.c_str()));

   resolveLevelDir("c:/temp/leveldir2", "nosuchfolder", "c:/ini/level/dir/");       // rootDataDir/levels
   TNLAssertV(configDirs->levelDir == "c:/temp/leveldir2/levels", ("Bad leveldir: %s", configDirs->levelDir.c_str()));

   resolveLevelDir("c:/temp/nosuchfolder", "leveldir", "c:/temp/");       // iniLevelDir/levelDir
   TNLAssertV(configDirs->levelDir == "c:/temp/leveldir", ("Bad leveldir: %s", configDirs->levelDir.c_str()));

   resolveLevelDir("c:/temp/nosuchfolder", "nosuchfolder", "c:/temp");       // iniLevelDir
   TNLAssertV(configDirs->levelDir == "c:/temp", ("Bad leveldir: %s", configDirs->levelDir.c_str()));

   resolveLevelDir("c:/temp/nosuchfolder", "nosuchfolder", "c:/does/not/exist");       // total failure
   TNLAssertV(configDirs->levelDir == 0, "", ("Bad leveldir: %s", configDirs->levelDir.c_str()));

   printf("passed leveldir resolution tests!\n");
}
#endif


void ConfigDirectories::resolveLevelDir()  
{
   //testLevelDirResolution();
   levelDir = resolveLevelDir(gCmdLineSettings.dirs.levelDir, gIniSettings.levelDir);
}


static string checkName(const string &filename, const char *folders[], const char *extensions[])
{
   string name;
   if(filename.find('.') != string::npos)       // filename has an extension
   {
      S32 i = 0;
      while(strcmp(folders[i], ""))
      {
         name = strictjoindir(folders[i], filename);
         if(fileExists(name))
            return name;
         i++;
      }
   }
   else
   {
      S32 i = 0; 
      while(strcmp(extensions[i], ""))
      {
         S32 j = 0;
         while(strcmp(folders[j], ""))
         {
            name = strictjoindir(folders[j], filename + extensions[i]);
            if(fileExists(name))
               return name;
            j++;
         }
         i++;
      }
   }

   return "";
}


string ConfigDirectories::findLevelFile(const string &filename) const
{
   return findLevelFile(levelDir, filename);
}


string ConfigDirectories::findLevelFile(const string &leveldir, const string &filename) const
{
#ifdef TNL_OS_XBOX         // This logic completely untested for OS_XBOX... basically disables -leveldir param
   const char *folders[] = { "d:\\media\\levels\\", "" };
#else
   const char *folders[] = { leveldir.c_str(), "" };
#endif
   const char *extensions[] = { ".level", "" };

   return checkName(filename, folders, extensions);
}


string ConfigDirectories::findLevelGenScript(const string &filename) const
{
   const char *folders[] = { levelDir.c_str(), luaDir.c_str(), "" };
   const char *extensions[] = { ".levelgen", ".lua", "" };

   return checkName(filename, folders, extensions);
}


string ConfigDirectories::findBotFile(const string &filename) const          
{
   const char *folders[] = { robotDir.c_str(), "" };
   const char *extensions[] = { ".bot", ".lua", "" };

   return checkName(filename, folders, extensions);
}


////////////////////////////////////////
////////////////////////////////////////

// Returns display-friendly mode designator like "Keyboard" or "Joystick 1"
string IniSettings::getInputMode()
{
#ifndef ZAP_DEDICATED
   if(gIniSettings.inputMode == InputModeJoystick)
      return "Joystick " + itos(Joystick::UseJoystickNumber);
   else
#endif
      return "Keyboard";
}


////////////////////////////////////////
////////////////////////////////////////

// Cmd Line processing callbacks


static void paramRootDataDir(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.dirs.rootDataDir = words[0];

   ConfigDirectories *folderManager = settings->getConfigDirs();
   folderManager->rootDataDir = words[0];             // Also sock it away here
}

static void paramLevelDir(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.dirs.levelDir = words[0];
}

static void paramIniDir(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.dirs.iniDir= words[0];
}

static void paramLogDir(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.dirs.logDir = words[0];
}

static void paramScriptsDir(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.dirs.luaDir = words[0];
}

static void paramCacheDir(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.dirs.cacheDir = words[0];
}

static void paramRobotDir(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.dirs.robotDir = words[0];
}

static void paramScreenshotDir(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.dirs.screenshotDir = words[0];
}

static void paramSfxDir(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.dirs.sfxDir = words[0];
}

static void paramMusicDir(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.dirs.musicDir = words[0];
}

static void paramMaster(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.masterAddress = words[0];
}

static void paramHostAddr(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.hostaddr = words[0];
}

static void paramLoss(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.loss = stof(words[0]);
}

static void paramLag(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.lag = stoi(words[0]);
}

static void paramForceUpdate(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.forceUpdate = true;
}

static void paramDedicated(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.dedicatedMode = true;

   if(words.size() == 1)
      gCmdLineSettings.dedicated = words[0];
}

static void paramName(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.name = words[0];
}

static void paramPassword(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.password = words[0];
}

static void paramServerPassword(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.serverPassword = words[0];
}

static void paramAdminPassword(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.adminPassword = words[0];
}

static void paramLevelChangePassword(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.levelChangePassword = words[0];
}

static void paramLevels(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.specifiedLevels = words;
}

static void paramHostName(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.hostname = words[0];
}

static void paramHostDescr(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.hostdescr = words[0];
}

static void paramMaxPlayers(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.maxPlayers = stoi(words[0]);
}

static void paramWindow(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.displayMode = DISPLAY_MODE_WINDOWED;
}

static void paramFullscreen(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.displayMode = DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED;
}

static void paramFullscreenStretch(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.displayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;
}

static void paramWinPos(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.xpos = stoi(words[0]);
   gCmdLineSettings.ypos = stoi(words[1]);
}

static void paramWinWidth(GameSettings *settings, const Vector<string> &words)
{
   gCmdLineSettings.winWidth = stoi(words[0]);
}


extern void exitToOs(S32 errcode);
extern void transferResource(GameSettings *settings, const string &addr, const string &pw, const string &fileName, const string &resourceType, bool sending);

static void paramGetRes(GameSettings *settings, const Vector<string> &words)
{
   transferResource(settings, words[0], words[1], words[2], words[3], false);
}

static void paramSendRes(GameSettings *settings, const Vector<string> &words)
{
   transferResource(settings, words[0], words[1], words[2], words[3], true);
}


extern bool writeToConsole();
extern void printRules();

static void paramRules(GameSettings *settings, const Vector<string> &words)
{
   writeToConsole();
   printRules();
   exitToOs(0);
}

static void paramHelp(GameSettings *settings, const Vector<string> &words);    // Forward declare this one here; it is defined down below


static void paramUseStick(GameSettings *settings, const Vector<string> &words)
{
#ifndef ZAP_DEDICATED
   Joystick::UseJoystickNumber = stoi(words[0]) - 1;  // zero-indexed     //  TODO: should be part of gCmdLineSettings
#endif
}


enum ParamRequirements {
   NO_PARAMETERS,      
   ONE_OPTIONAL,
   ONE_REQUIRED,
   TWO_REQUIRED,
   FOUR_REQUIRED,
   ALL_REMAINING
};


struct ParamInfo {
   string paramName;
   S32 pass;
   ParamRequirements argsRequired;
   void (*paramCallback)(GameSettings *settings, const Vector<string> &args);
   S32 docLevel;     
   string paramString;
   string helpString;
   const char *errorMsg;
};

const char *helpTitles[] = {
   "Player-oriented options",
   "Options for hosting",
   "Specifying levels",
   "Specifying folders\nAll of the following options can be specified with either a relative or absolute path. They are primarily intended to make installation on certain Linux platforms more flexible; they are not meant for daily use by average users.\nIn most cases, -rootdatadir is the only parameter in this section you will need.",
   "Developer-oriented options",
   "Advanced server management options",
   "Other commands",
};


// The order of these is unimportant -- it only determines the order in which the items are printed out with the -help option.  
// Items are sorted by tier, but order is otherwise preserved.
// Only items related to determining the locations of resources should be included in pass 0; most items should be set to pass 1.

ParamInfo paramDefs[] = {   
// Parameter           Pass, Args required  Callback function        Tier  Args                                                Help string            Error message (not needed for NO_PARAMETERS)

// Player-oriented options
{ "name",                1,  ONE_REQUIRED,  paramName,                0, "<string>", "Specify your username",                                                                   "You must enter a nickname with the -name option" },
{ "password",            1,  ONE_REQUIRED,  paramPassword,            0, "<string>", "Specify your password",                                                                   "You must enter a password with the -password option" },
{ "window",              1,  NO_PARAMETERS, paramWindow,              0, "", "Start in windowed mode",                                       "" },
{ "fullscreen",          1,  NO_PARAMETERS, paramFullscreen,          0, "", "Start in fullscreen mode (no stretching)",                     "" },
{ "fullscreen-stretch",  1,  NO_PARAMETERS, paramFullscreenStretch,   0, "", "Start in fullscreen mode (gaphics stretched to fill monitor)", "" },
{ "winpos",              1,  TWO_REQUIRED,  paramWinPos,              0, "<int> <int>", "Specify x,y location of game window (note that this is the position of the UL corner of the game canvas, and does not account for the window frame)", "You must specify the x and y position of the window with the -winpos option" },
{ "winwidth",            1,  ONE_REQUIRED,  paramWinWidth,            0, "<int>",       "Specify width of game window. Height will be set automatically. Note that the specified width is the width of the game canvas itself, and does not take account of window borders. Therefore, the entire window width will exceed the size specified slightly.", "You must specify the width of the game window with the -winwidth option" },
{ "usestick",            1,  ONE_REQUIRED,  paramUseStick,            0, "<int>",       "Specify which joystick or other input device to use. Default is 1.", "You must specify the joystick you want to use with the -usestick option" },
{ "master",              1,  ONE_REQUIRED,  paramMaster,              0, "<address>", "Use master server (game finder) at specified address",                                   "You must specify a master server address with -master option" },

// Options for hosting
{ "dedicated",           1,  ONE_OPTIONAL,  paramDedicated,           1, "[address]", "Run as a dedicated game server (i.e. no game window, console mode)",                     "" },
{ "serverpassword",      1,  ONE_REQUIRED,  paramServerPassword,      1, "<string>", "Specify a server password (players will need to know this to connect to your server)",    "You must enter a password with the -serverpassword option" },
{ "adminpassword",       1,  ONE_REQUIRED,  paramAdminPassword,       1, "<string>", "Specify an admin password (allowing those with the password to kick players and change their teams) when you host a game or run a dedicated server", "You must specify an admin password with the -adminpassword option" },
{ "levelchangepassword", 1,  ONE_REQUIRED,  paramLevelChangePassword, 1, "<string>", "Specify the password required for players to be able to change levels on your server when you host a game or run a dedicated server", "You must specify an level-change password with the -levelchangepassword option" },
{ "hostname",            1,  ONE_REQUIRED,  paramHostName,            1, "<string>", "Set the name that will appear in the server browser when searching for servers", "You must specify a server name with the -hostname option" },
{ "hostdescr",           1,  ONE_REQUIRED,  paramHostDescr,           1, "<string>", "Set a brief description of the server, which will be visible when players browse for game servers. Use double quotes (\") for descriptions containing spaces.", "You must specify a description (use quotes) with the -hostdescr option" },
{ "maxplayers",          1,  ONE_REQUIRED,  paramMaxPlayers,          1, "<int>",    "Max players allowed in a game (default is 128)", "You must specify the max number of players on your server with the -maxplayers option" }, 
{ "hostaddr",            1,  ONE_REQUIRED,  paramHostAddr,            1, "<address>", "Specify host address for the server to listen to when hosting",                        "You must specify a host address for the host to listen on (e.g. IP:Any:28000 or IP:192.169.1.100:5500)" },

// Specifying levels
{ "levels",              1,  ALL_REMAINING, paramLevels,              2, "<level 1> [level 2]...", "Specify the levels to play. Note that all remaining items on the command line will be interpreted as levels, so this must be the last parameter.", "You must specify one or more levels to load with the -levels option" },

// Specifying folders
{ "rootdatadir",         0,  ONE_REQUIRED,  paramRootDataDir,         3, "<path>",                "Equivalent to setting the -inidir, -logdir, -robotdir, -screenshotdir, and -leveldir parameters. The application will automatially append \"/robots\", \"/screenshots\", and \"/levels\" to path as appropriate.", "You must specify the root data folder with the -rootdatadir option" },
{ "leveldir",            0,  ONE_REQUIRED,  paramLevelDir,            2, "<folder or subfolder>", "Load all levels in specified system folder, or a subfolder under the levels folder. Levels will be loaded in alphabetical order by level-file name. Admins can create custom level lists by copying selected levels into folders or subfolders, and rename the files to get them to load in the proper order.", "You must specify a levels subfolder with the -leveldir option" },
{ "inidir",              0,  ONE_REQUIRED,  paramIniDir,              3, "<path>",                "Folder where INI file is stored",            "You must specify a the folder where your INI file is stored with the -inidir option" },
{ "logdir",              0,  ONE_REQUIRED,  paramLogDir,              3, "<path>",                "Folder where logfiles will be written",      "You must specify your log folder with the -logdir option" },
{ "scriptsdir",          0,  ONE_REQUIRED,  paramScriptsDir,          3, "<path>",                "Folder where Lua helper scripts are stored", "You must specify the folder where your Lua scripts are stored with the -scriptsdir option" },
{ "cachedir",            0,  ONE_REQUIRED,  paramCacheDir,            3, "<path>",                "Folder where cache files are stored",        "You must specify the folder where cache files are to be stored with the -cachedir option" },
{ "robotdir",            0,  ONE_REQUIRED,  paramRobotDir,            3, "<path>",                "Folder where robot scripts are stored",      "You must specify the robots folder with the -robotdir option" },
{ "screenshotdir",       0,  ONE_REQUIRED,  paramScreenshotDir,       3, "<path>",                "Folder where screenshots are stored",        "You must specify your screenshots folder with the -screenshotdir option" },
{ "sfxdir",              0,  ONE_REQUIRED,  paramSfxDir,              3, "<path>",                "Folder where sounds are stored",             "You must specify your sounds folder with the -sfxdir option" },
{ "musicdir",            0,  ONE_REQUIRED,  paramMusicDir,            3, "<path>",                "Folder where game music stored",             "You must specify your sounds folder with the -musicdir option" }, 

// Developer-oriented options
{ "loss",                1,  ONE_REQUIRED,  paramLoss,                4, "<float>",   "Simulate the specified amount of packet loss, from 0 (no loss) to 1 (all packets lost)", "You must specify a loss rate between 0 and 1 with the -loss option" },
{ "lag",                 1,  ONE_REQUIRED,  paramLag,                 4, "<int>",     "Simulate the specified amount of server lag (in milliseconds)",                          "You must specify a lag (in ms) with the -lag option" },
{ "forceupdate",         1,  NO_PARAMETERS, paramForceUpdate,         4, "",          "Trick game into thinking it needs to update",                                           "" },

// Advanced server management options
{ "getres",              1,  FOUR_REQUIRED, paramGetRes,              5, "<server address> <admin password> <resource name> <LEVEL|LEVELGEN|BOT>", "Send a resource to a remote server. Address must be specified in the form IP:nnn.nnn.nnn.nnn:port. The server must be running, have an admin password set, and have resource management enabled ([Host] section in the bitfighter.ini file).", "Usage: bitfighter getres <server address> <password> <file> <resource type>" },
{ "sendres",             1,  FOUR_REQUIRED, paramSendRes,             5, "<server address> <admin password> <resource name> <LEVEL|LEVELGEN|BOT>", "Retrieve a resource from a remote server, with same requirements as -sendres.",                                                                                                                                                                "Usage: bitfighter sendres <server address> <password> <file> <resource type>" },

// Other commands
{ "rules",               1,  NO_PARAMETERS, paramRules,               6, "",  "Print a list of \"rules of the game\" and other possibly useful data", "" },
{ "help",                1,  NO_PARAMETERS, paramHelp,                6, "",  "Display this message", "" },  

};


void parameterError(S32 i)
{
   printf("%s\n", paramDefs[i].errorMsg);
   exitToOs(1);
}


static string makeParamStr(const ParamInfo paramInfo)
{
   return paramInfo.paramName + (paramInfo.paramString == "" ? "" : " ") + paramInfo.paramString;
}


static string makePad(U32 len)
{
   string padding = "";

   for(U32 i = 0; i < len; i++)
      padding += " ";

   return padding;
}


static const S32 MAX_HELP_LINE_LEN = 110;

static U32 chunkStart;
static string chunkText;

// Return a chunk of text starting at start, with a max of len chars
static string getChunk(U32 len)
{
   if(chunkStart >= chunkText.length())
      return "";

   // Advance chunkStart to position of first non-space; avoids leading spaces
   chunkStart += chunkText.substr(chunkStart, len + 1).find_first_not_of(' ');
               
   // Create a chunk of text, with the max length we have room for
   string chunk = chunkText.substr(chunkStart, len + 1);
               
   if(chunk.length() >= len)                                // If chunk would fill a full line...
      chunk = chunk.substr(0, chunk.find_last_of(' '));     // ...lop chunk off at last space

   chunkStart += chunk.length();

   return chunk;
}


static void resetChunker(const string &text)
{
   chunkText = trim(text);
   chunkStart = 0;
}


static void paramHelp(GameSettings *settings, const Vector<string> &words)
{
   for(S32 i = 0; i < S32(ARRAYSIZE(helpTitles)); i++)
   {
      // Make an initial sweep through to check on the sizes of things, to ensure we get the indention right
      U32 maxSize = 0;

      for(S32 j = 0; j < S32(ARRAYSIZE(paramDefs)); j++)
         if(paramDefs[j].docLevel == i)
         {
            U32 len = makeParamStr(paramDefs[j]).length();

            if(len > maxSize)
               maxSize = len;
         }

      bool firstSection = true;

      for(S32 j = 0; j < S32(ARRAYSIZE(paramDefs)); j++)
      {
         if(paramDefs[j].docLevel != i)
            continue;

         if(firstSection)     // First item in this docLevel... print the section header
         {
            printf("\n");
            string title = helpTitles[i];

            while(title.length())
            {
               printf("\n");

               // Have to use string::size_type here because U32 and string::npos don't
               // compare well on x86_64 machines
               string::size_type firstCR = title.find_first_of('\n');     // Grab a line of the title
               resetChunker(title.substr(0, firstCR));

               if(firstCR != string::npos)
                  title = title.substr(firstCR + 1);
               else
                  title = "";

               while(true)
               {
                  string chunk = getChunk(MAX_HELP_LINE_LEN);

                  if(!chunk.length())
                     break;

                  printf("%s\n", chunk.c_str());
               }

               firstSection = false;
            }
         }

         string paramStr = makeParamStr(paramDefs[j]);
         U32 paddingLen = maxSize - paramStr.length();

         U32 wrapWidth = MAX_HELP_LINE_LEN - maxSize;

         U32 currPos = 0;

         string helpString = trim(paramDefs[j].helpString);
         U32 length = helpString.length();                     // Make sure errant trailing spaces don't screw us up

         resetChunker(helpString);

         bool first = true;

         while(true)
         {
            string chunk = getChunk(wrapWidth);

            if(!chunk.length())
               break;

            if(first)
            {
               printf("\t-%s%s -- %s\n", paramStr.c_str(), makePad(paddingLen).c_str(),  chunk.c_str());
               first = false;
            }
            else
               printf("\t%s %s\n",                         makePad(maxSize + 4).c_str(), chunk.c_str());
         }
      }
   }

   // Add some final notes...
   printf("\n\nNotes:\n\
   \t<param> denotes a required parameter\n\
   \t[param] denotes an optional parameter\n\
   \taddress is an address in the form ip address:port. (e.g. 192.168.1.55:25955)\n\
   \tstring means a parameter consisting of some combination of letters and numbers (e.g. Grambol_22).\n\
   \t   In many cases, spaces can be included by enclosing entire string in double quotes (\"Solid Gold Levels\").\n\
   \tinteger means an integer number must be specified (e.g. 4)\n\
   \tfloat means a floating point number must be specified (e.g. 3.5)\n");

   exitToOs(0);
}


////////////////////////////////////////
////////////////////////////////////////

void CmdLineSettings::readParams(GameSettings *settings, const Vector<string> &argv, S32 pass)
{
   S32 argc = argv.size();
   S32 argPtr = 0;

   Vector<string> params;

   while(argPtr < argc)
   {
      // Assume "args" starting with "-" are actually subsequent params
      bool hasAdditionalArg =                         (argPtr != argc - 1 && argv[argPtr + 1][0] != '-');     
      bool has2AdditionalArgs = hasAdditionalArg   && (argPtr != argc - 2 && argv[argPtr + 2][0] != '-');
      bool has3AdditionalArgs = has2AdditionalArgs && (argPtr != argc - 3 && argv[argPtr + 3][0] != '-');
      bool has4AdditionalArgs = has3AdditionalArgs && (argPtr != argc - 4 && argv[argPtr + 4][0] != '-');

      bool found = false;

      string arg = argv[argPtr];
      argPtr++;      // Advance argPtr to location of first parameter argument

      for(U32 i = 0; i < ARRAYSIZE(paramDefs); i++)
      {
         if(arg == "-" + paramDefs[i].paramName)
         {
            found = true;

            params.clear();

            if(paramDefs[i].argsRequired == NO_PARAMETERS)
            {
               // Do nothing
            }
            else if(paramDefs[i].argsRequired == ONE_OPTIONAL)
            {
               if(hasAdditionalArg)
               {
                  params.push_back(argv[argPtr]);
                  argPtr += 1;
               }
            }
            else if(paramDefs[i].argsRequired == ONE_REQUIRED)
            {
               if(!hasAdditionalArg)
                  parameterError(i);

               params.push_back(argv[argPtr]);
               argPtr += 1;
            }
            else if(paramDefs[i].argsRequired == TWO_REQUIRED)
            {
               if(!has2AdditionalArgs)
                  parameterError(i);

               params.push_back(argv[argPtr]);
               params.push_back(argv[argPtr + 1]);
               argPtr += 2;
            }
            else if(paramDefs[i].argsRequired == FOUR_REQUIRED)
            {
               if(!has4AdditionalArgs)
                  parameterError(i);

               params.push_back(argv[argPtr]);
               params.push_back(argv[argPtr + 1]);
               params.push_back(argv[argPtr + 2]);
               params.push_back(argv[argPtr + 3]);
               argPtr += 4;
            }
            else if(paramDefs[i].argsRequired == ALL_REMAINING)
            {
               if(!hasAdditionalArg)
                  parameterError(i);

               for(S32 j = argPtr; j < argc; j++)
                  params.push_back(argv[j]);

               argPtr = argc;
            }

            if(paramDefs[i].pass == pass)
               paramDefs[i].paramCallback(settings, params);    // Call the parameter processing function, if we're in the right pass

            break;
         }
      }

      if(!found)
      {
         printf("Invalid cmd line parameter found: %s\n", arg.c_str());
         exit(1);
      }
   }

#ifdef ZAP_DEDICATED
   // Override some settings if we're compiling ZAP_DEDICATED
   gCmdLineSettings.dedicatedMode = true;
#endif

}

};


