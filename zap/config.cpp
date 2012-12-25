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
#include "InputCode.h"
#include "BanList.h"
#include "Colors.h"

#include "ship.h"          // For Ship::stringToLoadout (this could be moved?)

#include "stringUtils.h"  // For itos

#include "GameSettings.h"

#ifndef ZAP_DEDICATED
#  include "Joystick.h"
#  include "quickChatHelper.h"
#endif

#include "stringUtils.h"
#include "dataConnection.h"   // For defs of stuff used by transferResource() function below

#ifdef _MSC_VER
#  pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#ifdef TNL_OS_WIN32
#  include <windows.h>   // For ARRAYSIZE when using ZAP_DEDICATED
#endif

#ifndef min
#  define min(a,b) ((a) <= (b) ? (a) : (b))
#  define max(a,b) ((a) >= (b) ? (a) : (b))
#endif


namespace Zap
{

// bitfighter.org would soon be the same as 199.192.229.168
const char *MASTER_SERVER_LIST_ADDRESS = "IP:199.192.229.168:25955,bitfighter.org:25955";
//const char *MASTER_SERVER_LIST_ADDRESS = "IP:199.192.229.168:25955, bitfighter.net:25955";

// Constructor: Set default values here
IniSettings::IniSettings()
{
   controlsRelative = false;          // Relative controls is lame!
   displayMode = DISPLAY_MODE_WINDOWED;
   oldDisplayMode = DISPLAY_MODE_UNKNOWN;
   joystickType = "NoJoystick";
   joystickLinuxUseOldDeviceSystem = false;
   alwaysStartInKeyboardMode = false;
   echoVoice = false;

   sfxVolLevel       = 1.0;           // SFX volume (0 = silent, 1 = full bore)
   musicVolLevel     = 1.0;           // Music volume (range as above)
   voiceChatVolLevel = 1.0;           // INcoming voice chat volume (range as above)
   alertsVolLevel    = 1.0;           // Audio alerts volume (when in dedicated server mode only, range as above)

   sfxSet = sfxModernSet;             // Start off with our modern sounds

   starsInDistance = true;            // True if stars move in distance behind maze, false if they are in fixed location
   diagnosticKeyDumpMode = false;     // True if want to dump keystrokes to the screen
   enableExperimentalAimMode = false; // True if we want to show experimental aiming vector in joystick mode

   showWeaponIndicators = true;       // True if we show the weapon indicators on the top of the screen
   verboseHelpMessages = true;        // If true, we'll show more handholding messages
   showKeyboardKeys = true;           // True if we show the keyboard shortcuts in joystick mode
   allowDataConnections = false;      // Disabled unless explicitly enabled for security reasons -- most users won't need this
   allowGetMap = false;               // Disabled by default -- many admins won't want this

   maxDedicatedFPS = 100;             // Max FPS on dedicated server
   maxFPS = 100;                      // Max FPS on client/non-dedicated server

   masterAddress = MASTER_SERVER_LIST_ADDRESS;   // Default address of our master server
   name = "";                         // Player name (none by default)
   defaultName = "ChumpChange";       // Name used if user hits <enter> on name entry screen
   lastName = "ChumpChange";          // Name the user entered last time they ran the game
   lastPassword = "";
   lastEditorName = "";               // No default editor level name
   hostname = "Bitfighter host";      // Default host name
   hostdescr = "";
   maxPlayers = 127;
   maxBots = 10;
   botsBalanceTeams = false;
   minBalancedPlayers = 6;
   enableServerVoiceChat = true;
   serverPassword = "";               // Passwords empty by default
   adminPassword = "";
   levelChangePassword = "";
   levelDir = "";

   connectionSpeed = 0;

   defaultRobotScript = "s_bot.bot";            
   globalLevelScript = "";

   wallFillColor.set(0,0,.15);
   wallOutlineColor.set(Colors::blue);
   clientPortNumber = 0;

   randomLevels = false;
   skipUploads = false;

   allowMapUpload = false;
   allowAdminMapUpload = true;

   voteEnable = false;     // Voting disabled by default
   voteLength = 12;
   voteLengthToChangeTeam = 10;
   voteRetryLength = 30;
   voteYesStrength = 3;
   voteNoStrength = -3;
   voteNothingStrength = -1;

   useUpdater = true;

   // Game window location when in windowed mode
   winXPos = 0;  // if set to (0,0), it will not set the position meaning it uses operating system default position. (see bottom of "VideoSystem::actualizeScreenMode" in VideoSystem.cpp)
   winYPos = 0;
   winSizeFact = 1.0;

   useFakeFullscreen = true;

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
   logConfigurationError = true;
   logConnection = true;       
   logLevelLoaded = true;      
   logLuaObjectLifecycle = false;
   luaLevelGenerator = true;   
   luaBotMessage = true;       
   serverFilter = false; 

   logLevelError = true;

   logStats = false;          // Log statistics into local sqlite database

   version = BUILD_VERSION;   // Default to current version to avoid triggering upgrade checks on fresh install

   oldGoalFlash = true;
}



F32 IniSettings::getMusicVolLevel()
{
   if(musicMutedOnCmdLine)
      return 0;

   return musicVolLevel;
}


// As above, but ignores whether music was muted or not
F32 IniSettings::getRawMusicVolLevel()
{
   return musicVolLevel;
}


void  IniSettings::setMusicVolLevel(F32 vol)
{
   musicVolLevel = vol;
}



extern string lcase(string strToConvert);

// Sorts alphanumerically
extern S32 QSORT_CALLBACK alphaSort(string *a, string *b);

static void loadForeignServerInfo(CIniFile *ini, IniSettings *iniSettings)
{
   // AlwaysPingList will default to broadcast, can modify the list in the INI
   // http://learn-networking.com/network-design/how-a-broadcast-address-works
   iniSettings->alwaysPingList.clear();
   parseString(ini->GetValue("Connections", "AlwaysPingList", "IP:Broadcast:28000"), iniSettings->alwaysPingList, ',');

   // These are the servers we found last time we were able to contact the master.
   // In case the master server fails, we can use this list to try to find some game servers. 
   //parseString(ini->GetValue("ForeignServers", "ForeignServerList"), prevServerListFromMaster, ',');
   iniSettings->prevServerListFromMaster.clear();
   ini->GetAllValues("RecentForeignServers", iniSettings->prevServerListFromMaster);
}


// Use macro to make code more readable
#define addComment(comment) ini->sectionComment(section, comment);

extern S32 LOADOUT_PRESETS;

static void writeLoadoutPresets(CIniFile *ini, GameSettings *settings)
{
   const char *section = "LoadoutPresets";

   ini->addSection(section);      // Create the key, then provide some comments for documentation purposes


   if(ini->numSectionComments(section) == 0)
   {
      addComment("----------------");
      addComment(" Loadout presets are stored here.  You can manage these manually if you like, but it is usually easier");
      addComment(" to let the game do it for you.  Pressing Ctrl-1 will copy your current loadout into the first preset, etc.");
      addComment(" If you do choose to modify these, it is important to note that the modules come first, then the weapons.");
      addComment(" The order is the same as you would enter them when defining a loadout in-game.");
      addComment("----------------");
   }

   Vector<U8> preset;

   for(S32 i = 0; i < LOADOUT_PRESETS; i++)
   {
      if(!settings->getLoadoutPreset(i, preset))
         continue;

      string presetStr = Ship::loadoutToString(preset);

      if(presetStr != "")
         ini->SetValue(section, "Preset" + itos(i + 1), presetStr);
   }
}


static void writePluginBindings(CIniFile *ini)
{
   const char *section = "EditorPlugins";

   ini->addSection(section);             

   if(ini->numSectionComments(section) == 0)
   {
      addComment("----------------");
      addComment(" Editor plugins are lua scripts that can add extra functionality to the editor.  You can specify");
      addComment(" here using the following format:");
      addComment(" Plugin1=Key1|ScriptName.lua|Script help string");
      addComment(" ... etc ...");
      addComment(" The names of the presets are not important, and can be changed. Key combos follow the general form of");
      addComment(" Ctrl+Alt+Shift+Meta+Super+key (omit unneeded modifiers, you can get correct Input Strings from the ");
      addComment(" diagnostics screen).  Scripts should be stored in the plugins folder  in the install directory. Please")
      addComment(" see the Bitfighter wiki for details.");
      addComment("----------------");
   }
}


static void writeConnectionsInfo(CIniFile *ini, IniSettings *iniSettings)
{
   const char *section = "Connections";
   
   ini->addSection(section);

   if(ini->numSectionComments(section) == 0)
   {
      addComment("----------------");
      addComment(" AlwaysPingList - Always try to contact these servers (comma separated list); Format: IP:IPAddress:Port");
      addComment("                  Include 'IP:Broadcast:28000' to search LAN for local servers on default port");
      addComment("----------------");
   }

   // Creates comma delimited list
   ini->SetValue(section, "AlwaysPingList", listToString(iniSettings->alwaysPingList, ','));
}


static void writeForeignServerInfo(CIniFile *ini, IniSettings *iniSettings)
{
   const char *section = "RecentForeignServers";

   ini->addSection(section);

   if(ini->numSectionComments(section) == 0)
   {
      addComment("----------------");
      addComment(" This section contains a list of the most recent servers seen; used as a fallback if we can't reach the master");
      addComment(" Please be aware that this section will be automatically regenerated, and any changes you make will be overwritten");
      addComment("----------------");
   }

   ini->SetAllValues(section, "Server", iniSettings->prevServerListFromMaster);
}


// Read levels, if there are any...
 void loadLevels(CIniFile *ini, IniSettings *iniSettings)
{
   if(ini->findSection("Levels") != ini->noID)
   {
      S32 numLevels = ini->GetNumEntries("Levels");
      Vector<string> levelValNames;

      for(S32 i = 0; i < numLevels; i++)
         levelValNames.push_back(ini->ValueName("Levels", i));

      levelValNames.sort(alphaSort);

      string level;
      for(S32 i = 0; i < numLevels; i++)
      {
         level = ini->GetValue("Levels", levelValNames[i], "");
         if (level != "")
            iniSettings->levelList.push_back(StringTableEntry(level.c_str()));
      }
   }
}


// Read level deleteList, if there are any.  This could probably be made more efficient by not reading the
// valnames in first, but what the heck...
void loadLevelSkipList(CIniFile *ini, GameSettings *settings)
{
   settings->getLevelSkipList()->clear();
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


static void loadGeneralSettings(CIniFile *ini, IniSettings *iniSettings)
{
   string section = "Settings";

#ifdef TNL_OS_MOBILE
   // Mobile usually have a single, fullscreen mode
   iniSettings->displayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;
#else
   iniSettings->displayMode = stringToDisplayMode( ini->GetValue(section, "WindowMode", displayModeToString(iniSettings->displayMode)));
#endif

   iniSettings->oldDisplayMode = iniSettings->displayMode;

   iniSettings->controlsRelative = (lcase(ini->GetValue(section, "ControlMode", (iniSettings->controlsRelative ? "Relative" : "Absolute"))) == "relative");

   iniSettings->echoVoice            = ini->GetValueYN(section, "VoiceEcho", iniSettings->echoVoice);
   iniSettings->showWeaponIndicators = ini->GetValueYN(section, "LoadoutIndicators", iniSettings->showWeaponIndicators);
   iniSettings->verboseHelpMessages  = ini->GetValueYN(section, "VerboseHelpMessages", iniSettings->verboseHelpMessages);
   iniSettings->showKeyboardKeys     = ini->GetValueYN(section, "ShowKeyboardKeysInStickMode", iniSettings->showKeyboardKeys);

#ifndef ZAP_DEDICATED
   iniSettings->joystickType = ini->GetValue(section, "JoystickType", iniSettings->joystickType);
   iniSettings->joystickLinuxUseOldDeviceSystem = ini->GetValueYN(section, "JoystickLinuxUseOldDeviceSystem", iniSettings->joystickLinuxUseOldDeviceSystem);
   iniSettings->alwaysStartInKeyboardMode = ini->GetValueYN(section, "AlwaysStartInKeyboardMode", iniSettings->alwaysStartInKeyboardMode);
#endif
   iniSettings->useFakeFullscreen = ini->GetValueYN(section, "UseFakeFullscreen", iniSettings->useFakeFullscreen);

   iniSettings->winXPos = max(ini->GetValueI(section, "WindowXPos", iniSettings->winXPos), 0);    // Restore window location
   iniSettings->winYPos = max(ini->GetValueI(section, "WindowYPos", iniSettings->winYPos), 0);

   iniSettings->winSizeFact = (F32) ini->GetValueF(section, "WindowScalingFactor", iniSettings->winSizeFact);
   iniSettings->masterAddress = ini->GetValue(section, "MasterServerAddressList", iniSettings->masterAddress);
   
   iniSettings->name           = ini->GetValue(section, "Nickname", iniSettings->name);
   iniSettings->password       = ini->GetValue(section, "Password", iniSettings->password);

   iniSettings->defaultName    = ini->GetValue(section, "DefaultName", iniSettings->defaultName);
   iniSettings->lastName       = ini->GetValue(section, "LastName", iniSettings->lastName);
   iniSettings->lastPassword   = ini->GetValue(section, "LastPassword", iniSettings->lastPassword);
   iniSettings->lastEditorName = ini->GetValue(section, "LastEditorName", iniSettings->lastEditorName);

   iniSettings->version = ini->GetValueI(section, "Version", iniSettings->version);

   iniSettings->connectionSpeed = ini->GetValueI(section, "Version", iniSettings->connectionSpeed);

   iniSettings->enableExperimentalAimMode = ini->GetValueYN(section, "EnableExperimentalAimMode", iniSettings->enableExperimentalAimMode);
   S32 fps = ini->GetValueI(section, "MaxFPS", iniSettings->maxFPS);
   if(fps >= 1) 
      iniSettings->maxFPS = fps;   // Otherwise, leave it at the default value
   // else warn?

#ifndef ZAP_DEDICATED
   gDefaultLineWidth = (F32) ini->GetValueF(section, "LineWidth", 2);
   gLineWidth1 = gDefaultLineWidth * 0.5f;
   gLineWidth3 = gDefaultLineWidth * 1.5f;
   gLineWidth4 = gDefaultLineWidth * 2;
#endif
}


static void loadDiagnostics(CIniFile *ini, IniSettings *iniSettings)
{
   string section = "Diagnostics";

   iniSettings->diagnosticKeyDumpMode = ini->GetValueYN(section, "DumpKeys",              iniSettings->diagnosticKeyDumpMode);

   iniSettings->logConnectionProtocol = ini->GetValueYN(section, "LogConnectionProtocol", iniSettings->logConnectionProtocol);
   iniSettings->logNetConnection      = ini->GetValueYN(section, "LogNetConnection",      iniSettings->logNetConnection);
   iniSettings->logEventConnection    = ini->GetValueYN(section, "LogEventConnection",    iniSettings->logEventConnection);
   iniSettings->logGhostConnection    = ini->GetValueYN(section, "LogGhostConnection",    iniSettings->logGhostConnection);
   iniSettings->logNetInterface       = ini->GetValueYN(section, "LogNetInterface",       iniSettings->logNetInterface);
   iniSettings->logPlatform           = ini->GetValueYN(section, "LogPlatform",           iniSettings->logPlatform);
   iniSettings->logNetBase            = ini->GetValueYN(section, "LogNetBase",            iniSettings->logNetBase);
   iniSettings->logUDP                = ini->GetValueYN(section, "LogUDP",                iniSettings->logUDP);

   iniSettings->logFatalError         = ini->GetValueYN(section, "LogFatalError",         iniSettings->logFatalError);
   iniSettings->logError              = ini->GetValueYN(section, "LogError",              iniSettings->logError);
   iniSettings->logWarning            = ini->GetValueYN(section, "LogWarning",            iniSettings->logWarning);
   iniSettings->logConfigurationError = ini->GetValueYN(section, "LogConfigurationError", iniSettings->logConfigurationError);
   iniSettings->logConnection         = ini->GetValueYN(section, "LogConnection",         iniSettings->logConnection);
   iniSettings->logLevelError         = ini->GetValueYN(section, "LogLevelError",         iniSettings->logLevelError);

   iniSettings->logLevelLoaded        = ini->GetValueYN(section, "LogLevelLoaded",        iniSettings->logLevelLoaded);
   iniSettings->logLuaObjectLifecycle = ini->GetValueYN(section, "LogLuaObjectLifecycle", iniSettings->logLuaObjectLifecycle);
   iniSettings->luaLevelGenerator     = ini->GetValueYN(section, "LuaLevelGenerator",     iniSettings->luaLevelGenerator);
   iniSettings->luaBotMessage         = ini->GetValueYN(section, "LuaBotMessage",         iniSettings->luaBotMessage);
   iniSettings->serverFilter          = ini->GetValueYN(section, "ServerFilter",          iniSettings->serverFilter);
}


static void loadTestSettings(CIniFile *ini, IniSettings *iniSettings)
{
   iniSettings->neverConnectDirect = ini->GetValueYN("Testing", "NeverConnectDirect", iniSettings->neverConnectDirect);
   iniSettings->wallFillColor.set(ini->GetValue("Testing", "WallFillColor", iniSettings->wallFillColor.toRGBString()));
   iniSettings->wallOutlineColor.set(ini->GetValue("Testing", "WallOutlineColor", iniSettings->wallOutlineColor.toRGBString()));
   iniSettings->oldGoalFlash = ini->GetValueYN("Testing", "OldGoalFlash", iniSettings->oldGoalFlash);
   iniSettings->clientPortNumber = (U16) ini->GetValueI("Testing", "ClientPortNumber", iniSettings->clientPortNumber);
}


static void loadLoadoutPresets(CIniFile *ini, GameSettings *settings)
{
   Vector<string> rawPresets(LOADOUT_PRESETS);

   for(S32 i = 0; i < LOADOUT_PRESETS; i++)
      rawPresets.push_back(ini->GetValue("LoadoutPresets", "Preset" + itos(i + 1), ""));
   
   Vector<U8> loadout;

   for(S32 i = 0; i < LOADOUT_PRESETS; i++)
   {
      loadout.clear();

      if(Ship::stringToLoadout(rawPresets[i], loadout))
         settings->setLoadoutPreset(i, loadout);
   }
}


static void loadPluginBindings(CIniFile *ini, IniSettings *iniSettings)
{
   Vector<string> values;
   Vector<string> words;      // Reusable container

   ini->GetAllValues("EditorPlugins", values);

   // Parse the retrieved strings.  They'll be in the form "Key Script Help"
   for(S32 i = 0; i < values.size(); i++)
   {
      parseString(trim(values[i]), words, '|');

      if(words.size() < 3)
      {
         logprintf(LogConsumer::LogError, "Error parsing EditorPlugin defnition in INI: too few values (read: %s)", values[i].c_str());
         continue;
      }
      
      PluginBinding binding;
      binding.key = words[0];
      binding.script = words[1];
      binding.help = concatenate(words, 2);

      iniSettings->pluginBindings.push_back(binding);
   }
}


static void loadEffectsSettings(CIniFile *ini, IniSettings *iniSettings)
{
   iniSettings->starsInDistance  = (lcase(ini->GetValue("Effects", "StarsInDistance", (iniSettings->starsInDistance ? "Yes" : "No"))) == "yes");
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


static void loadSoundSettings(CIniFile *ini, GameSettings *settings, IniSettings *iniSettings)
{
   iniSettings->musicMutedOnCmdLine = settings->getSpecified(NO_MUSIC);

   iniSettings->sfxVolLevel       = (F32) ini->GetValueI("Sounds", "EffectsVolume",   (S32) (iniSettings->sfxVolLevel        * 10)) / 10.0f;
   iniSettings->setMusicVolLevel(   (F32) ini->GetValueI("Sounds", "MusicVolume",     (S32) (iniSettings->getMusicVolLevel() * 10)) / 10.0f);
   iniSettings->voiceChatVolLevel = (F32) ini->GetValueI("Sounds", "VoiceChatVolume", (S32) (iniSettings->voiceChatVolLevel  * 10)) / 10.0f;

   string sfxSet = ini->GetValue("Sounds", "SFXSet", "Modern");
   iniSettings->sfxSet = stringToSFXSet(sfxSet);

   // Bounds checking
   iniSettings->sfxVolLevel       = checkVol(iniSettings->sfxVolLevel);
   iniSettings->setMusicVolLevel(checkVol(iniSettings->getRawMusicVolLevel()));
   iniSettings->voiceChatVolLevel = checkVol(iniSettings->voiceChatVolLevel);
}


static void loadHostConfiguration(CIniFile *ini, IniSettings *iniSettings)
{
   const char *section = "Host";

   iniSettings->hostname  = ini->GetValue(section, "ServerName", iniSettings->hostname);
   iniSettings->hostaddr  = ini->GetValue(section, "ServerAddress", iniSettings->hostaddr);
   iniSettings->hostdescr = ini->GetValue(section, "ServerDescription", iniSettings->hostdescr);

   iniSettings->serverPassword      = ini->GetValue (section, "ServerPassword", iniSettings->serverPassword);
   iniSettings->adminPassword       = ini->GetValue (section, "AdminPassword", iniSettings->adminPassword);
   iniSettings->levelChangePassword = ini->GetValue (section, "LevelChangePassword", iniSettings->levelChangePassword);
   iniSettings->levelDir            = ini->GetValue (section, "LevelDir", iniSettings->levelDir);
   iniSettings->maxPlayers          = ini->GetValueI(section, "MaxPlayers", iniSettings->maxPlayers);
   iniSettings->maxBots             = ini->GetValueI(section, "MaxBots", iniSettings->maxBots);
   iniSettings->botsBalanceTeams    = ini->GetValueYN(section, "BotsBalanceTeams", iniSettings->botsBalanceTeams);
   iniSettings->minBalancedPlayers  = ini->GetValueI(section, "MinBalancedPlayers", iniSettings->minBalancedPlayers);
   iniSettings->enableServerVoiceChat = ini->GetValueYN (section, "EnableServerVoiceChat", iniSettings->enableServerVoiceChat);

   iniSettings->alertsVolLevel = (float) ini->GetValueI(section, "AlertsVolume", (S32) (iniSettings->alertsVolLevel * 10)) / 10.0f;
   iniSettings->allowGetMap          = ini->GetValueYN (section, "AllowGetMap", iniSettings->allowGetMap);
   iniSettings->allowDataConnections = ini->GetValueYN (section, "AllowDataConnections", iniSettings->allowDataConnections);

   S32 fps = ini->GetValueI(section, "MaxFPS", iniSettings->maxDedicatedFPS);
   if(fps >= 1) 
      iniSettings->maxDedicatedFPS = fps; 
   // TODO: else warn?

   iniSettings->logStats = ini->GetValueYN(section, "LogStats", iniSettings->logStats);

   //iniSettings->SendStatsToMaster = (lcase(ini->GetValue(section, "SendStatsToMaster", "yes")) != "no");

   iniSettings->alertsVolLevel = checkVol(iniSettings->alertsVolLevel);

   iniSettings->randomLevels           = (U32) ini->GetValueYN(section, "RandomLevels", S32(iniSettings->randomLevels) );
   iniSettings->skipUploads            = (U32) ini->GetValueYN(section, "SkipUploads", S32(iniSettings->skipUploads) );

   iniSettings->allowMapUpload         = (U32) ini->GetValueYN(section, "AllowMapUpload", S32(iniSettings->allowMapUpload) );
   iniSettings->allowAdminMapUpload    = (U32) ini->GetValueYN(section, "AllowAdminMapUpload", S32(iniSettings->allowAdminMapUpload) );

   iniSettings->voteEnable             = (U32) ini->GetValueYN(section, "VoteEnable", S32(iniSettings->voteEnable) );
   iniSettings->voteLength             = (U32) ini->GetValueI (section, "VoteLength", S32(iniSettings->voteLength) );
   iniSettings->voteLengthToChangeTeam = (U32) ini->GetValueI (section, "VoteLengthToChangeTeam", S32(iniSettings->voteLengthToChangeTeam) );
   iniSettings->voteRetryLength        = (U32) ini->GetValueI (section, "VoteRetryLength", S32(iniSettings->voteRetryLength) );

   iniSettings->voteYesStrength        = ini->GetValueI(section, "VoteYesStrength", iniSettings->voteYesStrength );
   iniSettings->voteNoStrength         = ini->GetValueI(section, "VoteNoStrength", iniSettings->voteNoStrength );
   iniSettings->voteNothingStrength    = ini->GetValueI(section, "VoteNothingStrength", iniSettings->voteNothingStrength );

#ifdef BF_WRITE_TO_MYSQL
   Vector<string> args;
   parseString(ini->GetValue(section, "MySqlStatsDatabaseCredentials"), args, ',');
   if(args.size() >= 1) iniSettings->mySqlStatsDatabaseServer = args[0];
   if(args.size() >= 2) iniSettings->mySqlStatsDatabaseName = args[1];
   if(args.size() >= 3) iniSettings->mySqlStatsDatabaseUser = args[2];
   if(args.size() >= 4) iniSettings->mySqlStatsDatabasePassword = args[3];
   if(iniSettings->mySqlStatsDatabaseServer == "server" && iniSettings->mySqlStatsDatabaseName == "dbname")
   {
      iniSettings->mySqlStatsDatabaseServer = "";  // blank this, so it won't try to connect to "server"
   }
#endif

   iniSettings->defaultRobotScript = ini->GetValue(section, "DefaultRobotScript", iniSettings->defaultRobotScript);
   iniSettings->globalLevelScript  = ini->GetValue(section, "GlobalLevelScript", iniSettings->globalLevelScript);
}


void loadUpdaterSettings(CIniFile *ini, IniSettings *iniSettings)
{
   iniSettings->useUpdater = lcase(ini->GetValue("Updater", "UseUpdater", "Yes")) != "no";
}


static InputCode getInputCode(CIniFile *ini, const string &section, const string &key, InputCode defaultValue)
{
   const char *code = InputCodeManager::inputCodeToString(defaultValue);
   return InputCodeManager::stringToInputCode(ini->GetValue(section, key, code).c_str());
}


#define LOAD_BINDING(binding, key) inputCodeManager->setBinding(binding, mode, getInputCode(ini, section, InputCodeManager::getBindingName(binding), key));

// Remember: If you change any of these defaults, you'll need to rebuild your INI file to see the results!
static void loadKeyBindings(CIniFile *ini, InputCodeManager *inputCodeManager)
{                                
   string section = "KeyboardKeyBindings";
   InputMode mode = InputModeKeyboard;

   // Top line expands to this:
   // inputCodeManager->setBinding(InputCodeManager::BINDING_SELWEAP1,  
   //         mode, getInputCode(ini, section, InputCodeManager::getBindingName(InputCodeManager::BINDING_SELWEAP1),  KEY_1));
   LOAD_BINDING(InputCodeManager::BINDING_SELWEAP1,  KEY_1);
   LOAD_BINDING(InputCodeManager::BINDING_SELWEAP2,  KEY_2);
   LOAD_BINDING(InputCodeManager::BINDING_SELWEAP3,  KEY_3);
   LOAD_BINDING(InputCodeManager::BINDING_ADVWEAP,   KEY_E);
   LOAD_BINDING(InputCodeManager::BINDING_ADVWEAP2,  MOUSE_WHEEL_UP);
   LOAD_BINDING(InputCodeManager::BINDING_PREVWEAP,  MOUSE_WHEEL_DOWN);
   LOAD_BINDING(InputCodeManager::BINDING_CMDRMAP,   KEY_C);
   LOAD_BINDING(InputCodeManager::BINDING_TEAMCHAT,  KEY_T);
   LOAD_BINDING(InputCodeManager::BINDING_GLOBCHAT,  KEY_G);
   LOAD_BINDING(InputCodeManager::BINDING_QUICKCHAT, KEY_V);
   LOAD_BINDING(InputCodeManager::BINDING_CMDCHAT,   KEY_SLASH);
   LOAD_BINDING(InputCodeManager::BINDING_LOADOUT,   KEY_Z);
   LOAD_BINDING(InputCodeManager::BINDING_MOD1,      KEY_SPACE);
   LOAD_BINDING(InputCodeManager::BINDING_MOD2,      MOUSE_RIGHT);
   LOAD_BINDING(InputCodeManager::BINDING_FIRE,      MOUSE_LEFT);
   LOAD_BINDING(InputCodeManager::BINDING_DROPITEM,  KEY_B);
   LOAD_BINDING(InputCodeManager::BINDING_TOGVOICE,  KEY_R);
   LOAD_BINDING(InputCodeManager::BINDING_UP,        KEY_W);
   LOAD_BINDING(InputCodeManager::BINDING_DOWN,      KEY_S);
   LOAD_BINDING(InputCodeManager::BINDING_LEFT,      KEY_A);
   LOAD_BINDING(InputCodeManager::BINDING_RIGHT,     KEY_D);
   LOAD_BINDING(InputCodeManager::BINDING_SCRBRD,    KEY_TAB);


   section = "JoystickKeyBindings";
   mode = InputModeJoystick;

   LOAD_BINDING(InputCodeManager::BINDING_SELWEAP1,  KEY_1);
   LOAD_BINDING(InputCodeManager::BINDING_SELWEAP2,  KEY_2);
   LOAD_BINDING(InputCodeManager::BINDING_SELWEAP3,  KEY_3);
   LOAD_BINDING(InputCodeManager::BINDING_ADVWEAP,   BUTTON_1);
   LOAD_BINDING(InputCodeManager::BINDING_ADVWEAP2,  MOUSE_WHEEL_UP);
   LOAD_BINDING(InputCodeManager::BINDING_PREVWEAP,  MOUSE_WHEEL_DOWN);
   LOAD_BINDING(InputCodeManager::BINDING_CMDRMAP,   BUTTON_2);
   LOAD_BINDING(InputCodeManager::BINDING_TEAMCHAT,  KEY_T);
   LOAD_BINDING(InputCodeManager::BINDING_GLOBCHAT,  KEY_G);
   LOAD_BINDING(InputCodeManager::BINDING_QUICKCHAT, BUTTON_3);
   LOAD_BINDING(InputCodeManager::BINDING_CMDCHAT,   KEY_SLASH);
   LOAD_BINDING(InputCodeManager::BINDING_LOADOUT,   BUTTON_4);
   LOAD_BINDING(InputCodeManager::BINDING_MOD1,      BUTTON_7);
   LOAD_BINDING(InputCodeManager::BINDING_MOD2,      BUTTON_6);
   LOAD_BINDING(InputCodeManager::BINDING_FIRE,      MOUSE_LEFT);
   LOAD_BINDING(InputCodeManager::BINDING_DROPITEM,  KEY_B);
   LOAD_BINDING(InputCodeManager::BINDING_TOGVOICE,  KEY_R);
   LOAD_BINDING(InputCodeManager::BINDING_UP,        KEY_UP);
   LOAD_BINDING(InputCodeManager::BINDING_DOWN,      KEY_DOWN);
   LOAD_BINDING(InputCodeManager::BINDING_LEFT,      KEY_LEFT);
   LOAD_BINDING(InputCodeManager::BINDING_RIGHT,     KEY_RIGHT);
   LOAD_BINDING(InputCodeManager::BINDING_SCRBRD,    BUTTON_5);

   // The following key bindings are not user-defineable at the moment, mostly because we want consistency
   // throughout the game, and that would require some real constraints on what keys users could choose.
   //keyHELP = KEY_F1;
   //keyMISSION = KEY_F2;
   //keyOUTGAMECHAT = KEY_F5;
   //keyFPS = KEY_F6;
   //keyDIAG = KEY_F7;
   // These were moved to main.cpp to get them defined before the menus
}


#define SAVE_BINDING(binding) ini->SetValue(section, InputCodeManager::getBindingName(binding), \
                                           InputCodeManager::inputCodeToString(inputCodeManager->getBinding(binding,  mode)));


static void writeKeyBindings(CIniFile *ini, InputCodeManager *inputCodeManager, const string &section, InputMode mode)
{
   // Top line evaluates to:
   // ini->SetValue(section, "SelWeapon1", InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_SELWEAP1,  mode)));
   SAVE_BINDING(InputCodeManager::BINDING_SELWEAP1);
   SAVE_BINDING(InputCodeManager::BINDING_SELWEAP2);
   SAVE_BINDING(InputCodeManager::BINDING_SELWEAP3);
   SAVE_BINDING(InputCodeManager::BINDING_ADVWEAP);
   SAVE_BINDING(InputCodeManager::BINDING_ADVWEAP2);
   SAVE_BINDING(InputCodeManager::BINDING_PREVWEAP);
   SAVE_BINDING(InputCodeManager::BINDING_CMDRMAP);
   SAVE_BINDING(InputCodeManager::BINDING_TEAMCHAT);
   SAVE_BINDING(InputCodeManager::BINDING_GLOBCHAT);
   SAVE_BINDING(InputCodeManager::BINDING_QUICKCHAT);
   SAVE_BINDING(InputCodeManager::BINDING_CMDCHAT);
   SAVE_BINDING(InputCodeManager::BINDING_LOADOUT);
   SAVE_BINDING(InputCodeManager::BINDING_MOD1);
   SAVE_BINDING(InputCodeManager::BINDING_MOD2);
   SAVE_BINDING(InputCodeManager::BINDING_FIRE);
   SAVE_BINDING(InputCodeManager::BINDING_DROPITEM);
   SAVE_BINDING(InputCodeManager::BINDING_TOGVOICE);
   SAVE_BINDING(InputCodeManager::BINDING_UP);
   SAVE_BINDING(InputCodeManager::BINDING_DOWN);
   SAVE_BINDING(InputCodeManager::BINDING_LEFT);
   SAVE_BINDING(InputCodeManager::BINDING_RIGHT);
   SAVE_BINDING(InputCodeManager::BINDING_SCRBRD);
}


static void writeKeyBindings(CIniFile *ini, InputCodeManager *inputCodeManager)
{
   writeKeyBindings(ini, inputCodeManager, "KeyboardKeyBindings", InputModeKeyboard);
   writeKeyBindings(ini, inputCodeManager, "JoystickKeyBindings", InputModeJoystick);
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
   emptynode.inputCode = KEY_UNKNOWN;
   emptynode.buttonCode = KEY_UNKNOWN;
   emptynode.teamOnly = false;
   emptynode.commandOnly = false;
   emptynode.caption = "";
   emptynode.msg = "";
   gQuickChatTree.push_back(emptynode);
   emptynode.isMsgItem = false;

   // Read QuickChat messages -- first search for keys matching "QuickChatMessagesGroup123"
   S32 keys = ini->GetNumSections();
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
      node.inputCode =  InputCodeManager::stringToInputCode(ini->GetValue(messages[i], "Key", "A").c_str());
      node.buttonCode = InputCodeManager::stringToInputCode(ini->GetValue(messages[i], "Button", "Button 1").c_str());
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
      node.inputCode =  InputCodeManager::stringToInputCode(ini->GetValue(groups[i], "Key", "A").c_str());
      node.buttonCode = InputCodeManager::stringToInputCode(ini->GetValue(groups[i], "Button", "Button 1").c_str());
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
         node.inputCode =  InputCodeManager::stringToInputCode(ini->GetValue(messages[j], "Key", "A").c_str());
         node.buttonCode = InputCodeManager::stringToInputCode(ini->GetValue(messages[j], "Button", "Button 1").c_str());
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


static void writeDefaultQuickChatMessages(CIniFile *ini, IniSettings *iniSettings)
{
   // Are there any QuickChatMessageGroups?  If not, we'll write the defaults.
   S32 keys = ini->GetNumSections();

   for(S32 i = 0; i < keys; i++)
   {
      string keyName = ini->getSectionName(i);
      if(keyName.substr(0, 22) == "QuickChatMessagesGroup" && keyName.find("_") == string::npos)
         return;
   }

   const char *section = "QuickChatMessages";

   ini->addSection(section);
   if(ini->numSectionComments(section) == 0)
   {
      addComment("----------------");
      addComment(" The structure of the QuickChatMessages sections is a bit complicated.  The structure reflects the");
      addComment(" way the messages are displayed in the QuickChat menu, so make sure you are familiar with that before");
      addComment(" you start modifying these items. Messages are grouped, and each group has a Caption (short name");
      addComment(" shown on screen), a Key (the shortcut key used to select the group), and a Button (a shortcut button");
      addComment(" used when in joystick mode).  If the Button is \"Undefined key\", then that item will not be shown");
      addComment(" in joystick mode, unless the ShowKeyboardKeysInStickMode setting is true.  Groups can be defined in");
      addComment(" any order, but will be displayed sorted by [section] name.  Groups are designated by the");
      addComment(" [QuickChatMessagesGroupXXX] sections, where XXX is a unique suffix, usually a number.");
      addComment(" ");
      addComment(" Each group can have one or more messages, as specified by the [QuickChatMessagesGroupXXX_MessageYYY]");
      addComment(" sections, where XXX is the unique group suffix, and YYY is a unique message suffix.  Again, messages");
      addComment(" can be defined in any order, and will appear sorted by their [section] name.  Key, Button, and");
      addComment(" Caption serve the same purposes as in the group definitions. Message is the actual message text that");
      addComment(" is sent, and MessageType should be either \"Team\" or \"Global\", depending on which users the");
      addComment(" message should be sent to.  You can mix Team and Global messages in the same section, but it may be");
      addComment(" less confusing not to do so.");
      addComment(" ");
      addComment(" Messages can also be added to the top-tier of items, by specifying a section like");
      addComment(" [QuickChat_MessageZZZ].");
      addComment(" ");
      addComment(" Note that no quotes are required around Messages or Captions, and if included, they will be sent as");
      addComment(" part of the message. Also, if you bullocks things up too badly, simply delete all QuickChatMessage");
      addComment(" sections, and they will be regenerated the next time you run the game (though your modifications");
      addComment(" will be lost).");
      addComment(" ");
      addComment(" Note that you can also use the QuickChat functionality to create shortcuts to commonly run /commands");
      addComment(" by setting the MessageType to \"Command\".  For example, if you define a QuickChat message to be");
      addComment(" \"addbots 2\" (without quotes, and without a slash), and the MessageType to \"Command\" (also");
      addComment(" without quotes), 2 robots will be added to the game when you press the appropriate keys.  You can");
      addComment(" use this functionality to assign commonly used commands to joystick buttons or short keyboard");
      addComment(" sequences.");
      addComment("----------------");
   }

   ini->SetValue("QuickChatMessagesGroup1", "Key", InputCodeManager::inputCodeToString(KEY_G));
   ini->SetValue("QuickChatMessagesGroup1", "Button", InputCodeManager::inputCodeToString(BUTTON_6));
   ini->SetValue("QuickChatMessagesGroup1", "Caption", "Global");
   ini->SetValue("QuickChatMessagesGroup1", "MessageType", "Global");

      ini->SetValue("QuickChatMessagesGroup1_Message1", "Key", InputCodeManager::inputCodeToString(KEY_A));
      ini->SetValue("QuickChatMessagesGroup1_Message1", "Button", InputCodeManager::inputCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup1_Message1", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message1", "Caption", "No Problem");
      ini->SetValue("QuickChatMessagesGroup1_Message1", "Message", "No Problemo.");

      ini->SetValue("QuickChatMessagesGroup1_Message2", "Key", InputCodeManager::inputCodeToString(KEY_T));
      ini->SetValue("QuickChatMessagesGroup1_Message2", "Button", InputCodeManager::inputCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup1_Message2", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message2", "Caption", "Thanks");
      ini->SetValue("QuickChatMessagesGroup1_Message2", "Message", "Thanks.");

      ini->SetValue("QuickChatMessagesGroup1_Message3", "Key", InputCodeManager::inputCodeToString(KEY_X));
      ini->SetValue("QuickChatMessagesGroup1_Message3", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup1_Message3", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message3", "Caption", "You idiot!");
      ini->SetValue("QuickChatMessagesGroup1_Message3", "Message", "You idiot!");

      ini->SetValue("QuickChatMessagesGroup1_Message4", "Key", InputCodeManager::inputCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup1_Message4", "Button", InputCodeManager::inputCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup1_Message4", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message4", "Caption", "Duh");
      ini->SetValue("QuickChatMessagesGroup1_Message4", "Message", "Duh.");

      ini->SetValue("QuickChatMessagesGroup1_Message5", "Key", InputCodeManager::inputCodeToString(KEY_C));
      ini->SetValue("QuickChatMessagesGroup1_Message5", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup1_Message5", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message5", "Caption", "Crap");
      ini->SetValue("QuickChatMessagesGroup1_Message5", "Message", "Ah Crap!");

      ini->SetValue("QuickChatMessagesGroup1_Message6", "Key", InputCodeManager::inputCodeToString(KEY_D));
      ini->SetValue("QuickChatMessagesGroup1_Message6", "Button", InputCodeManager::inputCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup1_Message6", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message6", "Caption", "Damnit");
      ini->SetValue("QuickChatMessagesGroup1_Message6", "Message", "Dammit!");

      ini->SetValue("QuickChatMessagesGroup1_Message7", "Key", InputCodeManager::inputCodeToString(KEY_S));
      ini->SetValue("QuickChatMessagesGroup1_Message7", "Button", InputCodeManager::inputCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup1_Message7", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message7", "Caption", "Shazbot");
      ini->SetValue("QuickChatMessagesGroup1_Message7", "Message", "Shazbot!");

      ini->SetValue("QuickChatMessagesGroup1_Message8", "Key", InputCodeManager::inputCodeToString(KEY_Z));
      ini->SetValue("QuickChatMessagesGroup1_Message8", "Button", InputCodeManager::inputCodeToString(BUTTON_6));
      ini->SetValue("QuickChatMessagesGroup1_Message8", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup1_Message8", "Caption", "Doh");
      ini->SetValue("QuickChatMessagesGroup1_Message8", "Message", "Doh!");

   ini->SetValue("QuickChatMessagesGroup2", "Key", InputCodeManager::inputCodeToString(KEY_D));
   ini->SetValue("QuickChatMessagesGroup2", "Button", InputCodeManager::inputCodeToString(BUTTON_5));
   ini->SetValue("QuickChatMessagesGroup2", "MessageType", "Team");
   ini->SetValue("QuickChatMessagesGroup2", "Caption", "Defense");

      ini->SetValue("QuickChatMessagesGroup2_Message1", "Key", InputCodeManager::inputCodeToString(KEY_G));
      ini->SetValue("QuickChatMessagesGroup2_Message1", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup2_Message1", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message1", "Caption", "Defend Our Base");
      ini->SetValue("QuickChatMessagesGroup2_Message1", "Message", "Defend our base.");

      ini->SetValue("QuickChatMessagesGroup2_Message2", "Key", InputCodeManager::inputCodeToString(KEY_D));
      ini->SetValue("QuickChatMessagesGroup2_Message2", "Button", InputCodeManager::inputCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup2_Message2", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message2", "Caption", "Defending Base");
      ini->SetValue("QuickChatMessagesGroup2_Message2", "Message", "Defending our base.");

      ini->SetValue("QuickChatMessagesGroup2_Message3", "Key", InputCodeManager::inputCodeToString(KEY_Q));
      ini->SetValue("QuickChatMessagesGroup2_Message3", "Button", InputCodeManager::inputCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup2_Message3", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message3", "Caption", "Is Base Clear?");
      ini->SetValue("QuickChatMessagesGroup2_Message3", "Message", "Is our base clear?");

      ini->SetValue("QuickChatMessagesGroup2_Message4", "Key", InputCodeManager::inputCodeToString(KEY_C));
      ini->SetValue("QuickChatMessagesGroup2_Message4", "Button", InputCodeManager::inputCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup2_Message4", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message4", "Caption", "Base Clear");
      ini->SetValue("QuickChatMessagesGroup2_Message4", "Message", "Base is secured.");

      ini->SetValue("QuickChatMessagesGroup2_Message5", "Key", InputCodeManager::inputCodeToString(KEY_T));
      ini->SetValue("QuickChatMessagesGroup2_Message5", "Button", InputCodeManager::inputCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup2_Message5", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message5", "Caption", "Base Taken");
      ini->SetValue("QuickChatMessagesGroup2_Message5", "Message", "Base is taken.");

      ini->SetValue("QuickChatMessagesGroup2_Message6", "Key", InputCodeManager::inputCodeToString(KEY_N));
      ini->SetValue("QuickChatMessagesGroup2_Message6", "Button", InputCodeManager::inputCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup2_Message6", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message6", "Caption", "Need More Defense");
      ini->SetValue("QuickChatMessagesGroup2_Message6", "Message", "We need more defense.");

      ini->SetValue("QuickChatMessagesGroup2_Message7", "Key", InputCodeManager::inputCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup2_Message7", "Button", InputCodeManager::inputCodeToString(BUTTON_6));
      ini->SetValue("QuickChatMessagesGroup2_Message7", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message7", "Caption", "Enemy Attacking Base");
      ini->SetValue("QuickChatMessagesGroup2_Message7", "Message", "The enemy is attacking our base.");

      ini->SetValue("QuickChatMessagesGroup2_Message8", "Key", InputCodeManager::inputCodeToString(KEY_A));
      ini->SetValue("QuickChatMessagesGroup2_Message8", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup2_Message8", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup2_Message8", "Caption", "Attacked");
      ini->SetValue("QuickChatMessagesGroup2_Message8", "Message", "We are being attacked.");

   ini->SetValue("QuickChatMessagesGroup3", "Key", InputCodeManager::inputCodeToString(KEY_F));
   ini->SetValue("QuickChatMessagesGroup3", "Button", InputCodeManager::inputCodeToString(BUTTON_4));
   ini->SetValue("QuickChatMessagesGroup3", "MessageType", "Team");
   ini->SetValue("QuickChatMessagesGroup3", "Caption", "Flag");

      ini->SetValue("QuickChatMessagesGroup3_Message1", "Key", InputCodeManager::inputCodeToString(KEY_F));
      ini->SetValue("QuickChatMessagesGroup3_Message1", "Button", InputCodeManager::inputCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup3_Message1", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message1", "Caption", "Get enemy flag");
      ini->SetValue("QuickChatMessagesGroup3_Message1", "Message", "Get the enemy flag.");

      ini->SetValue("QuickChatMessagesGroup3_Message2", "Key", InputCodeManager::inputCodeToString(KEY_R));
      ini->SetValue("QuickChatMessagesGroup3_Message2", "Button", InputCodeManager::inputCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup3_Message2", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message2", "Caption", "Return our flag");
      ini->SetValue("QuickChatMessagesGroup3_Message2", "Message", "Return our flag to base.");

      ini->SetValue("QuickChatMessagesGroup3_Message3", "Key", InputCodeManager::inputCodeToString(KEY_S));
      ini->SetValue("QuickChatMessagesGroup3_Message3", "Button", InputCodeManager::inputCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup3_Message3", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message3", "Caption", "Flag secure");
      ini->SetValue("QuickChatMessagesGroup3_Message3", "Message", "Our flag is secure.");

      ini->SetValue("QuickChatMessagesGroup3_Message4", "Key", InputCodeManager::inputCodeToString(KEY_H));
      ini->SetValue("QuickChatMessagesGroup3_Message4", "Button", InputCodeManager::inputCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup3_Message4", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message4", "Caption", "Have enemy flag");
      ini->SetValue("QuickChatMessagesGroup3_Message4", "Message", "I have the enemy flag.");

      ini->SetValue("QuickChatMessagesGroup3_Message5", "Key", InputCodeManager::inputCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup3_Message5", "Button", InputCodeManager::inputCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup3_Message5", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message5", "Caption", "Enemy has flag");
      ini->SetValue("QuickChatMessagesGroup3_Message5", "Message", "The enemy has our flag!");

      ini->SetValue("QuickChatMessagesGroup3_Message6", "Key", InputCodeManager::inputCodeToString(KEY_G));
      ini->SetValue("QuickChatMessagesGroup3_Message6", "Button", InputCodeManager::inputCodeToString(BUTTON_6));
      ini->SetValue("QuickChatMessagesGroup3_Message6", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup3_Message6", "Caption", "Flag gone");
      ini->SetValue("QuickChatMessagesGroup3_Message6", "Message", "Our flag is not in the base!");

   ini->SetValue("QuickChatMessagesGroup4", "Key", InputCodeManager::inputCodeToString(KEY_S));
   ini->SetValue("QuickChatMessagesGroup4", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
   ini->SetValue("QuickChatMessagesGroup4", "MessageType", "Team");
   ini->SetValue("QuickChatMessagesGroup4", "Caption", "Incoming Enemies - Direction");

      ini->SetValue("QuickChatMessagesGroup4_Message1", "Key", InputCodeManager::inputCodeToString(KEY_S));
      ini->SetValue("QuickChatMessagesGroup4_Message1", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup4_Message1", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup4_Message1", "Caption", "Incoming South");
      ini->SetValue("QuickChatMessagesGroup4_Message1", "Message", "*** INCOMING SOUTH ***");

      ini->SetValue("QuickChatMessagesGroup4_Message2", "Key", InputCodeManager::inputCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup4_Message2", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup4_Message2", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup4_Message2", "Caption", "Incoming East");
      ini->SetValue("QuickChatMessagesGroup4_Message2", "Message", "*** INCOMING EAST  ***");

      ini->SetValue("QuickChatMessagesGroup4_Message3", "Key", InputCodeManager::inputCodeToString(KEY_W));
      ini->SetValue("QuickChatMessagesGroup4_Message3", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup4_Message3", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup4_Message3", "Caption", "Incoming West");
      ini->SetValue("QuickChatMessagesGroup4_Message3", "Message", "*** INCOMING WEST  ***");

      ini->SetValue("QuickChatMessagesGroup4_Message4", "Key", InputCodeManager::inputCodeToString(KEY_N));
      ini->SetValue("QuickChatMessagesGroup4_Message4", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup4_Message4", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup4_Message4", "Caption", "Incoming North");
      ini->SetValue("QuickChatMessagesGroup4_Message4", "Message", "*** INCOMING NORTH ***");

      ini->SetValue("QuickChatMessagesGroup4_Message5", "Key", InputCodeManager::inputCodeToString(KEY_V));
      ini->SetValue("QuickChatMessagesGroup4_Message5", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup4_Message5", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup4_Message5", "Caption", "Incoming Enemies");
      ini->SetValue("QuickChatMessagesGroup4_Message5", "Message", "Incoming enemies!");

   ini->SetValue("QuickChatMessagesGroup5", "Key", InputCodeManager::inputCodeToString(KEY_V));
   ini->SetValue("QuickChatMessagesGroup5", "Button", InputCodeManager::inputCodeToString(BUTTON_3));
   ini->SetValue("QuickChatMessagesGroup5", "MessageType", "Team");
   ini->SetValue("QuickChatMessagesGroup5", "Caption", "Quick");

      ini->SetValue("QuickChatMessagesGroup5_Message1", "Key", InputCodeManager::inputCodeToString(KEY_J));
      ini->SetValue("QuickChatMessagesGroup5_Message1", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup5_Message1", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message1", "Caption", "Capture the objective");
      ini->SetValue("QuickChatMessagesGroup5_Message1", "Message", "Capture the objective.");

      ini->SetValue("QuickChatMessagesGroup5_Message2", "Key", InputCodeManager::inputCodeToString(KEY_O));
      ini->SetValue("QuickChatMessagesGroup5_Message2", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup5_Message2", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message2", "Caption", "Go on the offensive");
      ini->SetValue("QuickChatMessagesGroup5_Message2", "Message", "Go on the offensive.");

      ini->SetValue("QuickChatMessagesGroup5_Message3", "Key", InputCodeManager::inputCodeToString(KEY_A));
      ini->SetValue("QuickChatMessagesGroup5_Message3", "Button", InputCodeManager::inputCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup5_Message3", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message3", "Caption", "Attack!");
      ini->SetValue("QuickChatMessagesGroup5_Message3", "Message", "Attack!");

      ini->SetValue("QuickChatMessagesGroup5_Message4", "Key", InputCodeManager::inputCodeToString(KEY_W));
      ini->SetValue("QuickChatMessagesGroup5_Message4", "Button", InputCodeManager::inputCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup5_Message4", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message4", "Caption", "Wait for signal");
      ini->SetValue("QuickChatMessagesGroup5_Message4", "Message", "Wait for my signal to attack.");

      ini->SetValue("QuickChatMessagesGroup5_Message5", "Key", InputCodeManager::inputCodeToString(KEY_V));
      ini->SetValue("QuickChatMessagesGroup5_Message5", "Button", InputCodeManager::inputCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup5_Message5", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message5", "Caption", "Help!");
      ini->SetValue("QuickChatMessagesGroup5_Message5", "Message", "Help!");

      ini->SetValue("QuickChatMessagesGroup5_Message6", "Key", InputCodeManager::inputCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup5_Message6", "Button", InputCodeManager::inputCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup5_Message6", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message6", "Caption", "Regroup");
      ini->SetValue("QuickChatMessagesGroup5_Message6", "Message", "Regroup.");

      ini->SetValue("QuickChatMessagesGroup5_Message7", "Key", InputCodeManager::inputCodeToString(KEY_G));
      ini->SetValue("QuickChatMessagesGroup5_Message7", "Button", InputCodeManager::inputCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup5_Message7", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message7", "Caption", "Going offense");
      ini->SetValue("QuickChatMessagesGroup5_Message7", "Message", "Going offense.");

      ini->SetValue("QuickChatMessagesGroup5_Message8", "Key", InputCodeManager::inputCodeToString(KEY_Z));
      ini->SetValue("QuickChatMessagesGroup5_Message8", "Button", InputCodeManager::inputCodeToString(BUTTON_6));
      ini->SetValue("QuickChatMessagesGroup5_Message8", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup5_Message8", "Caption", "Move out");
      ini->SetValue("QuickChatMessagesGroup5_Message8", "Message", "Move out.");

   ini->SetValue("QuickChatMessagesGroup6", "Key", InputCodeManager::inputCodeToString(KEY_R));
   ini->SetValue("QuickChatMessagesGroup6", "Button", InputCodeManager::inputCodeToString(BUTTON_2));
   ini->SetValue("QuickChatMessagesGroup6", "MessageType", "Team");
   ini->SetValue("QuickChatMessagesGroup6", "Caption", "Reponses");

      ini->SetValue("QuickChatMessagesGroup6_Message1", "Key", InputCodeManager::inputCodeToString(KEY_A));
      ini->SetValue("QuickChatMessagesGroup6_Message1", "Button", InputCodeManager::inputCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup6_Message1", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message1", "Caption", "Acknowledge");
      ini->SetValue("QuickChatMessagesGroup6_Message1", "Message", "Acknowledged.");

      ini->SetValue("QuickChatMessagesGroup6_Message2", "Key", InputCodeManager::inputCodeToString(KEY_N));
      ini->SetValue("QuickChatMessagesGroup6_Message2", "Button", InputCodeManager::inputCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup6_Message2", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message2", "Caption", "No");
      ini->SetValue("QuickChatMessagesGroup6_Message2", "Message", "No.");

      ini->SetValue("QuickChatMessagesGroup6_Message3", "Key", InputCodeManager::inputCodeToString(KEY_Y));
      ini->SetValue("QuickChatMessagesGroup6_Message3", "Button", InputCodeManager::inputCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup6_Message3", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message3", "Caption", "Yes");
      ini->SetValue("QuickChatMessagesGroup6_Message3", "Message", "Yes.");

      ini->SetValue("QuickChatMessagesGroup6_Message4", "Key", InputCodeManager::inputCodeToString(KEY_S));
      ini->SetValue("QuickChatMessagesGroup6_Message4", "Button", InputCodeManager::inputCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup6_Message4", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message4", "Caption", "Sorry");
      ini->SetValue("QuickChatMessagesGroup6_Message4", "Message", "Sorry.");

      ini->SetValue("QuickChatMessagesGroup6_Message5", "Key", InputCodeManager::inputCodeToString(KEY_T));
      ini->SetValue("QuickChatMessagesGroup6_Message5", "Button", InputCodeManager::inputCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup6_Message5", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message5", "Caption", "Thanks");
      ini->SetValue("QuickChatMessagesGroup6_Message5", "Message", "Thanks.");

      ini->SetValue("QuickChatMessagesGroup6_Message6", "Key", InputCodeManager::inputCodeToString(KEY_D));
      ini->SetValue("QuickChatMessagesGroup6_Message6", "Button", InputCodeManager::inputCodeToString(BUTTON_6));
      ini->SetValue("QuickChatMessagesGroup6_Message6", "MessageType", "Team");
      ini->SetValue("QuickChatMessagesGroup6_Message6", "Caption", "Don't know");
      ini->SetValue("QuickChatMessagesGroup6_Message6", "Message", "I don't know.");

   ini->SetValue("QuickChatMessagesGroup7", "Key", InputCodeManager::inputCodeToString(KEY_T));
   ini->SetValue("QuickChatMessagesGroup7", "Button", InputCodeManager::inputCodeToString(BUTTON_1));
   ini->SetValue("QuickChatMessagesGroup7", "MessageType", "Global");
   ini->SetValue("QuickChatMessagesGroup7", "Caption", "Taunts");

      ini->SetValue("QuickChatMessagesGroup7_Message1", "Key", InputCodeManager::inputCodeToString(KEY_R));
      ini->SetValue("QuickChatMessagesGroup7_Message1", "Button", InputCodeManager::inputCodeToString(KEY_UNKNOWN));
      ini->SetValue("QuickChatMessagesGroup7_Message1", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message1", "Caption", "Rawr");
      ini->SetValue("QuickChatMessagesGroup7_Message1", "Message", "RAWR!");

      ini->SetValue("QuickChatMessagesGroup7_Message2", "Key", InputCodeManager::inputCodeToString(KEY_C));
      ini->SetValue("QuickChatMessagesGroup7_Message2", "Button", InputCodeManager::inputCodeToString(BUTTON_1));
      ini->SetValue("QuickChatMessagesGroup7_Message2", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message2", "Caption", "Come get some!");
      ini->SetValue("QuickChatMessagesGroup7_Message2", "Message", "Come get some!");

      ini->SetValue("QuickChatMessagesGroup7_Message3", "Key", InputCodeManager::inputCodeToString(KEY_D));
      ini->SetValue("QuickChatMessagesGroup7_Message3", "Button", InputCodeManager::inputCodeToString(BUTTON_2));
      ini->SetValue("QuickChatMessagesGroup7_Message3", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message3", "Caption", "Dance!");
      ini->SetValue("QuickChatMessagesGroup7_Message3", "Message", "Dance!");

      ini->SetValue("QuickChatMessagesGroup7_Message4", "Key", InputCodeManager::inputCodeToString(KEY_X));
      ini->SetValue("QuickChatMessagesGroup7_Message4", "Button", InputCodeManager::inputCodeToString(BUTTON_3));
      ini->SetValue("QuickChatMessagesGroup7_Message4", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message4", "Caption", "Missed me!");
      ini->SetValue("QuickChatMessagesGroup7_Message4", "Message", "Missed me!");

      ini->SetValue("QuickChatMessagesGroup7_Message5", "Key", InputCodeManager::inputCodeToString(KEY_W));
      ini->SetValue("QuickChatMessagesGroup7_Message5", "Button", InputCodeManager::inputCodeToString(BUTTON_4));
      ini->SetValue("QuickChatMessagesGroup7_Message5", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message5", "Caption", "I've had worse...");
      ini->SetValue("QuickChatMessagesGroup7_Message5", "Message", "I've had worse...");

      ini->SetValue("QuickChatMessagesGroup7_Message6", "Key", InputCodeManager::inputCodeToString(KEY_Q));
      ini->SetValue("QuickChatMessagesGroup7_Message6", "Button", InputCodeManager::inputCodeToString(BUTTON_5));
      ini->SetValue("QuickChatMessagesGroup7_Message6", "MessageType", "Global");
      ini->SetValue("QuickChatMessagesGroup7_Message6", "Caption", "How'd THAT feel?");
      ini->SetValue("QuickChatMessagesGroup7_Message6", "Message", "How'd THAT feel?");

      ini->SetValue("QuickChatMessagesGroup7_Message7", "Key", InputCodeManager::inputCodeToString(KEY_E));
      ini->SetValue("QuickChatMessagesGroup7_Message7", "Button", InputCodeManager::inputCodeToString(BUTTON_6));
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
//      parseString(ini->GetValue("Joystick", "Axes" + itos(i), i<8 ? itos(i+16) : ""), buttonList, ',');
//      gJoystickMapping.axes[i] = 0;
//      for(S32 j=0; j<buttonList.size(); j++)
//      {
//         gJoystickMapping.axes[i] |= 1 << atoi(buttonList[j].c_str());
//      }
//   }
//   for(U32 i=0; i<32; i++)
//   {
//      Vector<string> buttonList;
//      parseString(ini->GetValue("Joystick", "Button" + itos(i), i<10 ? itos(i) : ""), buttonList, ',');
//      gJoystickMapping.button[i] = 0;
//      for(S32 j=0; j<buttonList.size(); j++)
//      {
//         gJoystickMapping.button[i] |= 1 << atoi(buttonList[j].c_str());
//      }
//   }
//   for(U32 i=0; i<4; i++)
//   {
//      Vector<string> buttonList;
//      parseString(ini->GetValue("Joystick", "Pov" + itos(i), itos(i+10)), buttonList, ',');
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


static void loadServerBanList(CIniFile *ini, BanList *banList)
{
   Vector<string> banItemList;
   ini->GetAllValues("ServerBanList", banItemList);
   banList->loadBanList(banItemList);
}

// Can't be static -- called externally!
void writeServerBanList(CIniFile *ini, BanList *banList)
{
   // Refresh the server ban list
   ini->deleteSection("ServerBanList");
   ini->addSection("ServerBanList");

   string delim = banList->getDelimiter();
   string wildcard = banList->getWildcard();
   if(ini->numSectionComments("ServerBanList") == 0)
   {
      ini->sectionComment("ServerBanList", "----------------");
      ini->sectionComment("ServerBanList", " This section contains a list of bans that this dedicated server has enacted");
      ini->sectionComment("ServerBanList", " ");
      ini->sectionComment("ServerBanList", " Bans are in the following format:");
      ini->sectionComment("ServerBanList", "   IP Address " + delim + " nickname " + delim + " Start time (ISO time format) " + delim + " Duration in minutes ");
      ini->sectionComment("ServerBanList", " ");
      ini->sectionComment("ServerBanList", " Examples:");
      ini->sectionComment("ServerBanList", "   BanItem0=123.123.123.123" + delim + "watusimoto" + delim + "20110131T123000" + delim + "30");
      ini->sectionComment("ServerBanList", "   BanItem1=" + wildcard + delim + "watusimoto" + delim + "20110131T123000" + delim + "120");
      ini->sectionComment("ServerBanList", "   BanItem2=123.123.123.123" + delim + wildcard + delim + "20110131T123000" + delim + "30");
      ini->sectionComment("ServerBanList", " ");
      ini->sectionComment("ServerBanList", " Note: Wildcards (" + wildcard +") may be used for IP address and nickname" );
      ini->sectionComment("ServerBanList", " ");
      ini->sectionComment("ServerBanList", " Note: ISO time format is in the following format: YYYYMMDDTHH24MISS");
      ini->sectionComment("ServerBanList", "   YYYY = four digit year, (e.g. 2011)");
      ini->sectionComment("ServerBanList", "     MM = month (01 - 12), (e.g. 01)");
      ini->sectionComment("ServerBanList", "     DD = day of the month, (e.g. 31)");
      ini->sectionComment("ServerBanList", "      T = Just a one character divider between date and time, (will always be T)");
      ini->sectionComment("ServerBanList", "   HH24 = hour of the day (0-23), (e.g. 12)");
      ini->sectionComment("ServerBanList", "     MI = minute of the hour, (e.g. 30)");
      ini->sectionComment("ServerBanList", "     SS = seconds of the minute, (e.g. 00) (we don't really care about these... yet)");
      ini->sectionComment("ServerBanList", "----------------");
   }

   ini->SetAllValues("ServerBanList", "BanItem", banList->banListToString());
}


// Option default values are stored here, in the 3rd prarm of the GetValue call
// This is only called once, during initial initialization
void loadSettingsFromINI(CIniFile *ini, GameSettings *settings)
{
   InputCodeManager *inputCodeManager = settings->getInputCodeManager();
   IniSettings *iniSettings = settings->getIniSettings();

   ini->ReadFile();                             // Read the INI file

   loadSoundSettings(ini, settings, iniSettings);
   loadEffectsSettings(ini, iniSettings);
   loadGeneralSettings(ini, iniSettings);
   loadLoadoutPresets(ini, settings);
   loadPluginBindings(ini, iniSettings);

   loadHostConfiguration(ini, iniSettings);
   loadUpdaterSettings(ini, iniSettings);
   loadDiagnostics(ini, iniSettings);

   loadTestSettings(ini, iniSettings);

   loadKeyBindings(ini, inputCodeManager);
   loadForeignServerInfo(ini, iniSettings);     // Info about other servers
   loadLevels(ini, iniSettings);                // Read levels, if there are any
   loadLevelSkipList(ini, settings);            // Read level skipList, if there are any

   loadQuickChatMessages(ini);

   loadServerBanList(ini, settings->getBanList());

   saveSettingsToINI(ini, settings);            // Save to fill in any missing settings

   settings->onFinishedLoading();
}


static void writeDiagnostics(CIniFile *ini, IniSettings *iniSettings)
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
      ini->sectionComment(section, " LogConfigurationError - Log problems with configuration (Yes/No)");
      ini->sectionComment(section, " LogConnection - High level logging connections with remote machines (Yes/No)");
      ini->sectionComment(section, " LogLevelLoaded - Write a log entry when a level is loaded (Yes/No)");
      ini->sectionComment(section, " LogLevelError - Log errors and warnings about levels loaded (Yes/No)");
      ini->sectionComment(section, " LogLuaObjectLifecycle - Creation and destruciton of lua objects (Yes/No)");
      ini->sectionComment(section, " LuaLevelGenerator - Messages from the LuaLevelGenerator (Yes/No)");
      ini->sectionComment(section, " LuaBotMessage - Message from a bot (Yes/No)");
      ini->sectionComment(section, " ServerFilter - For logging messages specific to hosting games (Yes/No)");
      ini->sectionComment(section, "                (Note: these messages will go to bitfighter_server.log regardless of this setting) ");
      ini->sectionComment(section, "----------------");
   }

   ini->setValueYN(section, "DumpKeys", iniSettings->diagnosticKeyDumpMode);
   ini->setValueYN(section, "LogConnectionProtocol", iniSettings->logConnectionProtocol);
   ini->setValueYN(section, "LogNetConnection",      iniSettings->logNetConnection);
   ini->setValueYN(section, "LogEventConnection",    iniSettings->logEventConnection);
   ini->setValueYN(section, "LogGhostConnection",    iniSettings->logGhostConnection);
   ini->setValueYN(section, "LogNetInterface",       iniSettings->logNetInterface);
   ini->setValueYN(section, "LogPlatform",           iniSettings->logPlatform);
   ini->setValueYN(section, "LogNetBase",            iniSettings->logNetBase);
   ini->setValueYN(section, "LogUDP",                iniSettings->logUDP);

   ini->setValueYN(section, "LogFatalError",         iniSettings->logFatalError);
   ini->setValueYN(section, "LogError",              iniSettings->logError);
   ini->setValueYN(section, "LogWarning",            iniSettings->logWarning);
   ini->setValueYN(section, "LogConfigurationError", iniSettings->logConfigurationError);
   ini->setValueYN(section, "LogConnection",         iniSettings->logConnection);
   ini->setValueYN(section, "LogLevelLoaded",        iniSettings->logLevelLoaded);
   ini->setValueYN(section, "LogLevelError",         iniSettings->logLevelError);
   ini->setValueYN(section, "LogLuaObjectLifecycle", iniSettings->logLuaObjectLifecycle);
   ini->setValueYN(section, "LuaLevelGenerator",     iniSettings->luaLevelGenerator);
   ini->setValueYN(section, "LuaBotMessage",         iniSettings->luaBotMessage);
   ini->setValueYN(section, "ServerFilter",          iniSettings->serverFilter);
}


static void writeEffects(CIniFile *ini, IniSettings *iniSettings)
{
   const char *section = "Effects";
   ini->addSection(section);

   if (ini->numSectionComments(section) == 0)
   {
      ini->sectionComment(section, "----------------");
      ini->sectionComment(section, " Various visual effects");
      ini->sectionComment(section, " StarsInDistance - Yes gives the game a floating, 3-D effect.  No gives the flat 'classic zap' mode.");
      ini->sectionComment(section, "----------------");
   }

   ini->setValueYN(section, "StarsInDistance", iniSettings->starsInDistance);
}

static void writeSounds(CIniFile *ini, IniSettings *iniSettings)
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

   ini->SetValueI("Sounds", "EffectsVolume", (S32) (iniSettings->sfxVolLevel * 10));
   ini->SetValueI("Sounds", "MusicVolume",   (S32) (iniSettings->getRawMusicVolLevel() * 10));
   ini->SetValueI("Sounds", "VoiceChatVolume",   (S32) (iniSettings->voiceChatVolLevel * 10));

   ini->SetValue("Sounds", "SFXSet", iniSettings->sfxSet == sfxClassicSet ? "Classic" : "Modern");
}


void saveWindowMode(CIniFile *ini, IniSettings *iniSettings)
{
   ini->SetValue("Settings",  "WindowMode", displayModeToString(iniSettings->displayMode));
}


void saveWindowPosition(CIniFile *ini, S32 x, S32 y)
{
   ini->SetValueI("Settings", "WindowXPos", x);
   ini->SetValueI("Settings", "WindowYPos", y);
}


static void writeSettings(CIniFile *ini, IniSettings *iniSettings)
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
      ini->sectionComment(section, " UseFakeFullscreen - Faster fullscreen switching; however, may not cover the taskbar");
      ini->sectionComment(section, " VoiceEcho - Play echo when recording a voice message? Yes/No");
      ini->sectionComment(section, " ControlMode - Use Relative or Absolute controls (Relative means left is ship's left, Absolute means left is screen left)");
      ini->sectionComment(section, " LoadoutIndicators - Display indicators showing current weapon?  Yes/No");
      ini->sectionComment(section, " VerboseHelpMessages - Display additional on-screen messages while learning the game?  Yes/No");
      ini->sectionComment(section, " ShowKeyboardKeysInStickMode - If you are using a joystick, also show keyboard shortcuts in Loadout and QuickChat menus");
      ini->sectionComment(section, " JoystickType - Type of joystick to use if auto-detect doesn't recognize your controller");
      ini->sectionComment(section, " JoystickLinuxUseOldDeviceSystem - Force SDL to add the older /dev/input/js0 device to the enumerated joystick list.  No effect on Windows/Mac systems");
      ini->sectionComment(section, " AlwaysStartInKeyboardMode - Change to 'Yes' to always start the game in keyboard mode (don't auto-select the joystick)");
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
   saveWindowMode(ini, iniSettings);
   saveWindowPosition(ini, iniSettings->winXPos, iniSettings->winYPos);

   ini->setValueYN(section, "UseFakeFullscreen", iniSettings->useFakeFullscreen);
   ini->SetValueF (section, "WindowScalingFactor", iniSettings->winSizeFact);
   ini->setValueYN(section, "VoiceEcho", iniSettings->echoVoice );
   ini->SetValue  (section, "ControlMode", (iniSettings->controlsRelative ? "Relative" : "Absolute"));

   // inputMode is not saved, but rather determined at runtime by whether a joystick is attached

   ini->setValueYN(section, "LoadoutIndicators", iniSettings->showWeaponIndicators);
   ini->setValueYN(section, "VerboseHelpMessages", iniSettings->verboseHelpMessages);
   ini->setValueYN(section, "ShowKeyboardKeysInStickMode", iniSettings->showKeyboardKeys);

#ifndef ZAP_DEDICATED
   ini->SetValue  (section, "JoystickType", iniSettings->joystickType);
   ini->setValueYN(section, "JoystickLinuxUseOldDeviceSystem", iniSettings->joystickLinuxUseOldDeviceSystem);
   ini->setValueYN(section, "AlwaysStartInKeyboardMode", iniSettings->alwaysStartInKeyboardMode);
#endif
   ini->SetValue  (section, "MasterServerAddressList", iniSettings->masterAddress);
   ini->SetValue  (section, "DefaultName", iniSettings->defaultName);
   ini->SetValue  (section, "LastName", iniSettings->lastName);
   ini->SetValue  (section, "LastPassword", iniSettings->lastPassword);
   ini->SetValue  (section, "LastEditorName", iniSettings->lastEditorName);

   ini->setValueYN(section, "EnableExperimentalAimMode", iniSettings->enableExperimentalAimMode);
   ini->SetValueI (section, "MaxFPS", iniSettings->maxFPS);  

   ini->SetValueI (section, "ConnectionSpeed", iniSettings->connectionSpeed);  
   ini->SetValueI (section, "Version", BUILD_VERSION);

#ifndef ZAP_DEDICATED
   // Don't save new value if out of range, so it will go back to the old value. Just in case a user screw up with /linewidth command using value too big or too small
   if(gDefaultLineWidth >= 0.5 && gDefaultLineWidth <= 5)
      ini->SetValueF (section, "LineWidth", gDefaultLineWidth);
#endif
}


static void writeUpdater(CIniFile *ini, IniSettings *iniSettings)
{
   ini->addSection("Updater");

   if(ini->numSectionComments("Updater") == 0)
   {
      ini->sectionComment("Updater", "----------------");
      ini->sectionComment("Updater", " The Updater section contains entries that control how game updates are handled");
      ini->sectionComment("Updater", " UseUpdater - Enable or disable process that installs updates (WINDOWS ONLY)");
      ini->sectionComment("Updater", "----------------");

   }
   ini->setValueYN("Updater", "UseUpdater", iniSettings->useUpdater, true);
}


static void writeHost(CIniFile *ini, IniSettings *iniSettings)
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
      addComment(" BotsBalanceTeams - Enable bot auto-balancing in each level.");
      addComment(" MinBalancedPlayers - The minimum number of players ensured in each map.  Bots will be added up to this number.");
      addComment(" EnableServerVoiceChat - If false, prevents any voice chat in a server.");
      addComment(" AlertsVolume - Volume of audio alerts when players join or leave game from 0 (mute) to 10 (full bore).");
      addComment(" MaxFPS - Maximum FPS the dedicaetd server will run at.  Higher values use more CPU, lower may increase lag (default = 100).");
      addComment(" RandomLevels - When current level ends, this can enable randomly switching to any available levels.");
      addComment(" SkipUploads - When current level ends, enables skipping all uploaded levels.");
      addComment(" AllowGetMap - When getmap is allowed, anyone can download the current level using the /getmap command.");
      addComment(" AllowDataConnections - When data connections are allowed, anyone with the admin password can upload or download levels, bots, or");
      addComment("                        levelGen scripts.  This feature is probably insecure, and should be DISABLED unless you require the functionality.");
      addComment(" LogStats - Save game stats locally to built-in sqlite database (saves the same stats as are sent to the master)");
      addComment(" DefaultRobotScript - If user adds a robot, this script is used if none is specified");
      addComment(" GlobalLevelScript - Specify a levelgen that will get run on every level");
      addComment(" MySqlStatsDatabaseCredentials - If MySql integration has been compiled in (which it probably hasn't been), you can specify the");
      addComment("                                 database server, database name, login, and password as a comma delimeted list");
      addComment(" VoteLength - number of seconds the voting will last, zero will disable voting.");
      addComment(" VoteRetryLength - When vote fail, the vote caller is unable to vote until after this number of seconds.");
      addComment(" Vote Strengths - Vote will pass when sum of all vote strengths is bigger then zero.");
      addComment("----------------");
   }

   ini->SetValue  (section, "ServerName", iniSettings->hostname);
   ini->SetValue  (section, "ServerAddress", iniSettings->hostaddr);
   ini->SetValue  (section, "ServerDescription", iniSettings->hostdescr);
   ini->SetValue  (section, "ServerPassword", iniSettings->serverPassword);
   ini->SetValue  (section, "AdminPassword", iniSettings->adminPassword);
   ini->SetValue  (section, "LevelChangePassword", iniSettings->levelChangePassword);
   ini->SetValue  (section, "LevelDir", iniSettings->levelDir);
   ini->SetValueI (section, "MaxPlayers", iniSettings->maxPlayers);
   ini->SetValueI (section, "MaxBots", iniSettings->maxBots);
   ini->setValueYN(section, "BotsBalanceTeams", iniSettings->botsBalanceTeams);
   ini->SetValueI (section, "MinBalancedPlayers", iniSettings->minBalancedPlayers);
   ini->setValueYN(section, "EnableServerVoiceChat", iniSettings->enableServerVoiceChat);
   ini->SetValueI (section, "AlertsVolume", (S32) (iniSettings->alertsVolLevel * 10));
   ini->setValueYN(section, "AllowGetMap", iniSettings->allowGetMap);
   ini->setValueYN(section, "AllowDataConnections", iniSettings->allowDataConnections);
   ini->SetValueI (section, "MaxFPS", iniSettings->maxDedicatedFPS);
   ini->setValueYN(section, "LogStats", iniSettings->logStats);

   ini->setValueYN(section, "RandomLevels", S32(iniSettings->randomLevels) );
   ini->setValueYN(section, "SkipUploads", S32(iniSettings->skipUploads) );

   ini->setValueYN(section, "AllowMapUpload", S32(iniSettings->allowMapUpload) );
   ini->setValueYN(section, "AllowAdminMapUpload", S32(iniSettings->allowAdminMapUpload) );


   ini->setValueYN(section, "VoteEnable", S32(iniSettings->voteEnable) );
   ini->SetValueI(section, "VoteLength", S32(iniSettings->voteLength) );
   ini->SetValueI(section, "VoteLengthToChangeTeam", S32(iniSettings->voteLengthToChangeTeam) );
   ini->SetValueI(section, "VoteRetryLength", S32(iniSettings->voteRetryLength) );
   ini->SetValueI(section, "VoteYesStrength", iniSettings->voteYesStrength );
   ini->SetValueI(section, "VoteNoStrength", iniSettings->voteNoStrength );
   ini->SetValueI(section, "VoteNothingStrength", iniSettings->voteNothingStrength );

   ini->SetValue  (section, "DefaultRobotScript", iniSettings->defaultRobotScript);
   ini->SetValue  (section, "GlobalLevelScript", iniSettings->globalLevelScript );
#ifdef BF_WRITE_TO_MYSQL
   if(iniSettings->mySqlStatsDatabaseServer == "" && iniSettings->mySqlStatsDatabaseName == "" && iniSettings->mySqlStatsDatabaseUser == "" && iniSettings->mySqlStatsDatabasePassword == "")
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


static void writeTesting(CIniFile *ini, GameSettings *settings)
{
   IniSettings *iniSettings = settings->getIniSettings();

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
      ini->sectionComment("Testing", " ClientPortNumber - Only helps when punching through firewall when using router's port forwarded for client port number");
      ini->sectionComment("Testing", "----------------");
   }

   ini->setValueYN("Testing", "NeverConnectDirect", iniSettings->neverConnectDirect);

   // Maybe we should not write these if they are the default values?
   ini->SetValue  ("Testing", "WallFillColor",   settings->getWallFillColor()->toRGBString());
   ini->SetValue  ("Testing", "WallOutlineColor", iniSettings->wallOutlineColor.toRGBString());

   ini->setValueYN("Testing", "OldGoalFlash", iniSettings->oldGoalFlash);
   ini->SetValueI ("Testing", "ClientPortNumber", iniSettings->clientPortNumber);
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

   IniSettings *iniSettings = settings->getIniSettings();

   writeHost(ini, iniSettings);
   writeForeignServerInfo(ini, iniSettings);
   writeLoadoutPresets(ini, settings);
   writePluginBindings(ini);
   writeConnectionsInfo(ini, iniSettings);
   writeEffects(ini, iniSettings);
   writeSounds(ini, iniSettings);
   writeSettings(ini, iniSettings);
   writeDiagnostics(ini, iniSettings);
   writeLevels(ini);
   writeSkipList(ini, settings->getLevelSkipList());
   writeUpdater(ini, iniSettings);
   writeTesting(ini, settings);
   writePasswordSection(ini);
   writeKeyBindings(ini, settings->getInputCodeManager());
   
   writeDefaultQuickChatMessages(ini, iniSettings);  // Does nothing if there are already chat messages in the INI

   // only needed for users using custom joystick 
   // or joystick that maps differenly in LINUX
   // This adds 200+ lines.
   //writeJoystick();
   writeServerBanList(ini, settings->getBanList());

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

// Constructor
FolderManager::FolderManager()
{
   // Do nothing
}


// Constructor
FolderManager::FolderManager(const string &levelDir,    const string &robotDir,  const string &sfxDir,        const string &musicDir, 
                             const string &iniDir,      const string &logDir,    const string &screenshotDir, const string &luaDir,
                             const string &rootDataDir, const string &pluginDir) :
               levelDir      (levelDir),
               robotDir      (robotDir),
               sfxDir        (sfxDir),
               musicDir      (musicDir),
               iniDir        (iniDir),
               logDir        (logDir),
               screenshotDir (screenshotDir),
               luaDir        (luaDir),
               rootDataDir   (rootDataDir),
               pluginDir     (pluginDir)
{
   // Do nothing (more)
}                


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
struct CmdLineSettings;

// Doesn't handle leveldir -- that one is handled separately, later, because it requires input from the INI file
void FolderManager::resolveDirs(GameSettings *settings)
{
   FolderManager *folderManager = settings->getFolderManager();
   FolderManager cmdLineDirs = settings->getCmdLineFolderManager();     // Versions specified on the cmd line

   string rootDataDir = cmdLineDirs.rootDataDir;

   folderManager->rootDataDir = rootDataDir;

   // Note that we generally rely on Bitfighter being run from its install folder for these paths to be right... at least in Windows

   // rootDataDir used to specify the following folders
   folderManager->robotDir      = resolutionHelper(cmdLineDirs.robotDir,      rootDataDir, "robots");
   folderManager->pluginDir     = resolutionHelper(cmdLineDirs.pluginDir,     rootDataDir, "editor_plugins");
   folderManager->luaDir        = resolutionHelper(cmdLineDirs.luaDir,        rootDataDir, "scripts");
   folderManager->iniDir        = resolutionHelper(cmdLineDirs.iniDir,        rootDataDir, "");
   folderManager->logDir        = resolutionHelper(cmdLineDirs.logDir,        rootDataDir, "");
   folderManager->screenshotDir = resolutionHelper(cmdLineDirs.screenshotDir, rootDataDir, "screenshots");
   folderManager->musicDir      = resolutionHelper(cmdLineDirs.musicDir,      rootDataDir, "music");

   // rootDataDir not used for these folders
   folderManager->sfxDir        = resolutionHelper(cmdLineDirs.sfxDir,        "", "sfx");

   gSqlite = folderManager->logDir + "stats";
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
//     If levelDir is also specified try
//         iniLevelDir/levelDir
//     End   
//     iniLevelDir
// End    ==> Don't use iniLevelDir
//      
// levels
//
// If none of the above work, no hosting/editing for you!
//
// NOTE: See above for full explanation of what these functions are doing
string FolderManager::resolveLevelDir(const string &levelDir)    
{
   if(levelDir != "")
      if(fileExists(levelDir))     // Check for a valid absolute path in levelDir
         return levelDir;

   if(rootDataDir != "" && levelDir != "")
   {
      string candidate = strictjoindir(rootDataDir, "levels", levelDir);
      if(fileExists(candidate))
         return candidate;

      candidate = strictjoindir(rootDataDir, levelDir);
      if(fileExists(candidate))
         return candidate;
   }

   return "";
}


// Figuring out where the levels are stored is so complex, it needs its own function!
void FolderManager::resolveLevelDir(GameSettings *settings)  
{
   string cmdLineLevelDir = settings->getLevelDir(CMD_LINE);

   string resolved = resolveLevelDir(cmdLineLevelDir);

   if(resolved != "")
   {
      levelDir = resolved;
      return;
   }

   if(rootDataDir != "")
   {
      string candidate = strictjoindir(rootDataDir, "levels");    // Try rootDataDir/levels
      if(fileExists(candidate))   
      {
         levelDir = candidate;
         return;
      }
   }


   string iniLevelDir = settings->getLevelDir(INI);

   // rootDataDir is blank, or nothing using it worked
   if(iniLevelDir != "")
   {
      string candidate;

      if(cmdLineLevelDir != "")
      {
         candidate = strictjoindir(iniLevelDir, cmdLineLevelDir);    // Check if cmdLineLevelDir is a subfolder of iniLevelDir
         if(fileExists(candidate))
         {
            levelDir = candidate;
            return;
         }
      }
      

      // Ok, forget about cmdLineLevelDir.  Getting desperate here.  Try just the straight folder name specified in the INI file.
      if(fileExists(iniLevelDir))
      {
         levelDir = iniLevelDir;
         return;
      }
   }

   // Maybe there is just a local folder called levels?
   if(fileExists("levels"))
      levelDir = "levels";
   else
      levelDir = "";    // Surrender
}


extern string strictjoindir(const string &part1, const string &part2);
extern bool fileExists(const string &filename);

static string checkName(const string &filename, const Vector<string> &folders, const char *extensions[])
{
   string name;
   if(filename.find('.') != string::npos)       // filename has an extension
   {
      for(S32 i = 0; i < folders.size(); i++)
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
         for(S32 j = 0; j < folders.size(); j++)
         {
            name = strictjoindir(folders[j], filename + extensions[i]);
            if(fileExists(name))
               return name;
         }
         i++;
      }
   }

   return "";
}


string FolderManager::findLevelFile(const string &filename) const
{
   return findLevelFile(levelDir, filename);
}


string FolderManager::findLevelFile(const string &leveldir, const string &filename) const
{
#ifdef TNL_OS_XBOX         // This logic completely untested for OS_XBOX... basically disables -leveldir param
   const char *folders[] = { "d:\\media\\levels\\", "" };
#else
   Vector<string> folders;
   folders.push_back(leveldir);

#endif
   const char *extensions[] = { ".level", "" };

   return checkName(filename, folders, extensions);
}


Vector<string> FolderManager::getScriptFolderList() const
{
   Vector<string> folders;
   folders.push_back(levelDir);
   folders.push_back(luaDir);

   return folders;
}


Vector<string> FolderManager::getHelperScriptFolderList() const
{
   Vector<string> folders;
   folders.push_back(luaDir);
   folders.push_back(levelDir);
   folders.push_back(robotDir);

   return folders;
}


Vector<string> FolderManager::getPluginFolderList() const
{
   Vector<string> folders;
   folders.push_back(pluginDir);

   return folders;
}


string FolderManager::findLevelGenScript(const string &filename) const
{
   const char *extensions[] = { ".levelgen", ".lua", "" };

   return checkName(filename, getScriptFolderList(), extensions);
}


string FolderManager::findScriptFile(const string &filename) const
{
   const char *extensions[] = { ".lua", "" };

   return checkName(filename, getHelperScriptFolderList(), extensions);
}


string FolderManager::findPlugin(const string &filename) const
{
   const char *extensions[] = { ".lua", "" };

   return checkName(filename, getPluginFolderList(), extensions);
}


string FolderManager::findBotFile(const string &filename) const          
{
   Vector<string> folders;
   folders.push_back(robotDir);

   const char *extensions[] = { ".bot", ".lua", "" };

   return checkName(filename, folders, extensions);
}


////////////////////////////////////////
////////////////////////////////////////

CmdLineSettings::CmdLineSettings()
{
   init();
}


void CmdLineSettings::init()
{
   dedicatedMode = false;

   loss = 0;
   lag = 0;
   stutter = 0;
   forceUpdate = false;
   maxPlayers = -1;
   displayMode = DISPLAY_MODE_UNKNOWN;
   winWidth = -1;
   xpos = -9999;
   ypos = -9999;
};


};


