//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "config.h"

#include "GameSettings.h"
#include "IniFile.h"
#include "InputCode.h"
#include "version.h"
#include "BanList.h"
#include "Colors.h"

#ifndef ZAP_DEDICATED
#  include "quickChatHelper.h"
#endif

#include "stringUtils.h"   // For itos
#include "MathUtils.h"     // For MIN

#include "tnlLog.h"

#ifdef _MSC_VER
#  pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#ifdef TNL_OS_WIN32
#  include <windows.h>   // For ARRAYSIZE when using ZAP_DEDICATED
#endif

namespace Zap
{


// Constructor
UserSettings::UserSettings()
{
   for(S32 i = 0; i < LevelCount; i++)
      levelupItemsAlreadySeen[i] = false;
}


// Destructor
UserSettings::~UserSettings() { /* Do nothing */ }


////////////////////////////////////////
////////////////////////////////////////


// bitfighter.org would soon be the same as 199.192.229.168
// Nov 1 2013: bitfighter.org ip address changed to 209.148.88.166
const char *MASTER_SERVER_LIST_ADDRESS = "bitfighter.org:25955,bitfighter.net:25955,IP:209.148.88.166:25955";
//const char *MASTER_SERVER_LIST_ADDRESS = "IP:199.192.229.168:25955, bitfighter.net:25955";


// Vol gets stored as a number from 0 to 10; normalize it to 0-1
static F32 checkVol(const F32 &vol) 
{ 
   F32 v = vol / 10.0f; 
   return CLAMP(v, 0, 1);
}  


static F32 writeVol(const F32 &vol) 
{ 
   return ceilf(vol * 10.0f);
}  


// Constructor: Set default values here
IniSettings::IniSettings()
{

#  define SETTINGS_ITEM(typeName, enumVal, section, key, defaultVal, readValidator, writeValidator, comment)    \
            mSettings.add(                                                                                      \
               new Setting<typeName, IniKey::SettingsItem>(IniKey::enumVal, defaultVal, key,                    \
                                                           section, readValidator, writeValidator, comment)     \
            );
      SETTINGS_TABLE
#  undef SETTINGS_ITEM

   oldDisplayMode = DISPLAY_MODE_UNKNOWN;
   joystickLinuxUseOldDeviceSystem = false;
   alwaysStartInKeyboardMode = false;

   sfxVolLevel       = 1.0;           // SFX volume (0 = silent, 1 = full bore)
   musicVolLevel     = 1.0;           // Music volume (range as above)
   voiceChatVolLevel = 1.0;           // INcoming voice chat volume (range as above)

   sfxSet = sfxModernSet;             // Start off with our modern sounds

   maxFPS = 100;                      // Max FPS on client/non-dedicated server

   masterAddress = MASTER_SERVER_LIST_ADDRESS;   // Default address of our master server
   name = "";                         // Player name (none by default)
   defaultName = "ChumpChange";       // Name used if user hits <enter> on name entry screen
   lastPassword = "";
   lastEditorName = "";               // No default editor level name

   connectionSpeed = 0;

   queryServerSortColumn = 0;
   queryServerSortAscending = true;

   // Game window location when in windowed mode
   winXPos = 0;  // if set to (0,0), it will not set the position meaning it uses operating system default position. (see bottom of "VideoSystem::actualizeScreenMode" in VideoSystem.cpp)
   winYPos = 0;
   winSizeFact = 1.0;

   musicMutedOnCmdLine = false;

   version = BUILD_VERSION;   // Default to current version to avoid triggering upgrade checks on fresh install
}


// Destructor
IniSettings::~IniSettings()
{
   // Do nothing
}


// This list is currently incomplete, will grow as we move our settings into the new structure
static const string sections[] = {"Settings", "Effects", "Host", "Host-Voting", "EditorSettings", "Updater", "Diagnostics"};
static const string headerComments[] = 
{
   "Settings entries contain a number of different options.",
   "Various visual effects.",
   "Items in this section control how Bitfighter works when you are hosting a game.  See also Host-Voting.",
   "Control how voting works on the server.  The default values work pretty well, but if you want to tweak them, go ahead!\n"
      "Yes and No votes, and abstentions, have different weights.  When a vote is conducted, the total value of all votes (or non-votes)\n"
      "is added up, and if the result is greater than 0, the vote passes.  Otherwise it fails.  You can adjust the weight of the votes below.",
   "EditorSettings entries relate to items in the editor",
   "The Updater section contains entries that control how game updates are handled.",
   "Diagnostic entries can be used to enable or disable particular actions for debugging purposes.\n"
      "You probably can't use any of these settings to enhance your gameplay experience!"
};


// Some static helper methods:

// Set all bits in items[] to false
void IniSettings::clearbits(bool *bitArray, S32 itemCount)
{
   for(S32 i = 0; i < itemCount; i++)
      bitArray[i] = false;
}


// Produce a string of Ys and Ns based on values in bool items[], suitable for storing in the INI in a semi-readable manner.
// And this doesn't really pack as much as serialize, but that doesn't sound as punchy.
string IniSettings::bitArrayToIniString(const bool *bitArray, S32 itemCount)
{
   string s = "";

   for(S32 i = 0; i < itemCount; i++)
      s += bitArray[i] ? "Y" : "N";

   return s;
}


// Takes a string; we'll set the corresponding bool in items[] to true whenever we encounter a 'Y'.  See pack() for comment about name.
void IniSettings::iniStringToBitArray(const string &vals, bool *bitArray, S32 itemCount)
{
   clearbits(bitArray, itemCount);

   S32 count = MIN((S32)vals.size(), itemCount);

   for(S32 i = 0; i < count; i++)
      if(vals.at(i) == 'Y')
         bitArray[i] = true;
}


Vector<PluginBinding> IniSettings::getDefaultPluginBindings() const
{
   Vector<PluginBinding> bindings;

   static Vector<string> plugins;
   plugins.push_back("Ctrl+;|draw_arcs.lua|Make curves!");
   plugins.push_back("Ctrl+'|draw_stars.lua|Create polygon/star");

   Vector<string> words;

   // Parse the retrieved strings.  They'll be in the form "Key Script Help"
   for(S32 i = 0; i < plugins.size(); i++)
   {
      parseString(trim(plugins[i]), words, '|');

      PluginBinding binding;
      binding.key = words[0];
      binding.script = words[1];
      binding.help = concatenate(words, 2);

      bindings.push_back(binding);
   }

   return bindings;
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

   for(S32 i = 0; i < GameSettings::LoadoutPresetCount; i++)
   {
      string presetStr = settings->getLoadoutPreset(i).toString(true);

      if(presetStr != "")
         ini->SetValue(section, "Preset" + itos(i + 1), presetStr);
   }
}


static void writePluginBindings(CIniFile *ini, IniSettings *iniSettings)
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
      addComment(" Ctrl+Alt+Shift+Meta+Super+key (omit unneeded modifiers, you can get correct Input Strings from the");
      addComment(" diagnostics screen).  Scripts should be stored in the plugins folder in the install directory. Please")
      addComment(" see the Bitfighter wiki for details.");
      addComment("----------------");
   }

   Vector<string> plugins;
   PluginBinding binding;
   for(S32 i = 0; i < iniSettings->pluginBindings.size(); i++)
   {
      binding = iniSettings->pluginBindings[i];
      plugins.push_back(string(binding.key + "|" + binding.script + "|" + binding.help));
   }

   ini->SetAllValues(section, "Plugin", plugins);
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
   ini->SetValue(section, "AlwaysPingList", listToString(iniSettings->alwaysPingList, ","));
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


extern F32 gLineWidth1;
extern F32 gDefaultLineWidth;
extern F32 gLineWidth3;
extern F32 gLineWidth4;

typedef Vector<AbstractSetting<IniKey::SettingsItem> *> SettingsType;

static void loadSettings(CIniFile *ini, IniSettings *iniSettings, const string &section)
{
   SettingsType settings = iniSettings->mSettings.getSettingsInSection(section);

   for(S32 i = 0; i < settings.size(); i++)
      settings[i]->setValFromString(ini->GetValue(section, settings[i]->getKey(), settings[i]->getDefaultValueString()));
}


static void loadGeneralSettings(CIniFile *ini, IniSettings *iniSettings)
{
   // New school
   for(U32 i = 0; i < ARRAYSIZE(sections); i++)
      loadSettings(ini, iniSettings, sections[i]);

   string section = "Settings";

   // Now read the settings still defined all old school

#ifdef TNL_OS_MOBILE
   // Mobile usually have a single, fullscreen mode
   iniSettings->mSettings.setVal("WindowMode", DISPLAY_MODE_FULL_SCREEN_STRETCHED);
#endif

   iniSettings->oldDisplayMode = iniSettings->mSettings.getVal<DisplayMode>(IniKey::WindowMode);

#ifndef ZAP_DEDICATED
   iniSettings->joystickLinuxUseOldDeviceSystem = ini->GetValueYN(section, "JoystickLinuxUseOldDeviceSystem", iniSettings->joystickLinuxUseOldDeviceSystem);
   iniSettings->alwaysStartInKeyboardMode = ini->GetValueYN(section, "AlwaysStartInKeyboardMode", iniSettings->alwaysStartInKeyboardMode);
#endif

   iniSettings->winXPos = max(ini->GetValueI(section, "WindowXPos", iniSettings->winXPos), 0);    // Restore window location
   iniSettings->winYPos = max(ini->GetValueI(section, "WindowYPos", iniSettings->winYPos), 0);

   iniSettings->winSizeFact   = ini->GetValueF(section, "WindowScalingFactor", iniSettings->winSizeFact);
   iniSettings->masterAddress = ini->GetValue (section, "MasterServerAddressList", iniSettings->masterAddress);
   
   iniSettings->name           = ini->GetValue(section, "Nickname", iniSettings->name);
   iniSettings->password       = ini->GetValue(section, "Password", iniSettings->password);

   iniSettings->defaultName    = ini->GetValue(section, "DefaultName", iniSettings->defaultName);

   iniSettings->lastPassword   = ini->GetValue(section, "LastPassword", iniSettings->lastPassword);
   iniSettings->lastEditorName = ini->GetValue(section, "LastEditorName", iniSettings->lastEditorName);

   iniSettings->version = ini->GetValueI(section, "Version", iniSettings->version);

   iniSettings->connectionSpeed = ini->GetValueI(section, "ConnectionSpeed", iniSettings->connectionSpeed);

   S32 fps = ini->GetValueI(section, "MaxFPS", iniSettings->maxFPS);
   if(fps >= 1) 
      iniSettings->maxFPS = fps;   // Otherwise, leave it at the default value
   // else warn?

   iniSettings->queryServerSortColumn    = ini->GetValueI(section, "QueryServerSortColumn",    iniSettings->queryServerSortColumn);
   iniSettings->queryServerSortAscending = ini->GetValueB(section, "QueryServerSortAscending", iniSettings->queryServerSortAscending);


#ifndef ZAP_DEDICATED
   gDefaultLineWidth = ini->GetValueF(section, "LineWidth", 2);
   gLineWidth1 = gDefaultLineWidth * 0.5f;
   gLineWidth3 = gDefaultLineWidth * 1.5f;
   gLineWidth4 = gDefaultLineWidth * 2;
#endif
}


static void loadLoadoutPresets(CIniFile *ini, GameSettings *settings)
{
   Vector<string> rawPresets(GameSettings::LoadoutPresetCount);

   for(S32 i = 0; i < GameSettings::LoadoutPresetCount; i++)
      rawPresets.push_back(ini->GetValue("LoadoutPresets", "Preset" + itos(i + 1), ""));
   
   for(S32 i = 0; i < GameSettings::LoadoutPresetCount; i++)
   {
      LoadoutTracker loadout(rawPresets[i]);
      if(loadout.isValid())
         settings->setLoadoutPreset(&loadout, i);
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

   // If no plugins we're loaded, add our defaults  (maybe we don't want to do this?)
   if(iniSettings->pluginBindings.size() == 0)
      iniSettings->pluginBindings = iniSettings->getDefaultPluginBindings();
}


// Convert a string value to our sfxSets enum
static sfxSets stringToSFXSet(string sfxSet)
{
   return (lcase(sfxSet) == "classic") ? sfxClassicSet : sfxModernSet;
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


static InputCode getInputCode(CIniFile *ini, const string &section, const string &key, InputCode defaultValue)
{
   const char *code = InputCodeManager::inputCodeToString(defaultValue);
   return InputCodeManager::stringToInputCode(ini->GetValue(section, key, code).c_str());
}


// Returns a string like "Ctrl+L"
static string getInputString(CIniFile *ini, const string &section, const string &key, const string &defaultValue)
{
   string inputStringFromIni = ini->GetValue(section, key, defaultValue);
   string normalizedInputString = InputCodeManager::normalizeInputString(inputStringFromIni);

   // Check if inputString is valid -- we could get passed any ol' garbage that got put in the INI file
   if(InputCodeManager::isValidInputString(normalizedInputString))
   {
      // If normalized binding is different than what is in the INI file, replace the INI version with the good version
      if(normalizedInputString != inputStringFromIni)
         ini->SetValue(section, key, normalizedInputString);

      return normalizedInputString;
   }

   // We don't understand what is in the INI file... print a warning, and fall back to the default
   logprintf(LogConsumer::ConfigurationError, "Invalid key binding in INI section [%s]: %s=%s", 
             section.c_str(), key.c_str(), inputStringFromIni.c_str());
   return defaultValue;
}


// Remember: If you change any of these defaults, you'll need to rebuild your INI file to see the results!
static void setDefaultKeyBindings(CIniFile *ini, InputCodeManager *inputCodeManager)
{                                
   ///// BINDING_TABLE
   ///// KEYBOARD

   // Generates a block of code that looks like this:
   // if(true)
   //    inputCodeManager->setBinding(InputCodeManager::BINDING_SELWEAP1, InputModeKeyboard, 
   //          getInputCode(ini, "KeyboardKeyBindings", InputCodeManager::getBindingName(InputCodeManager::BINDING_SELWEAP1), 
   //                       KEY_1));

#define BINDING(enumVal, b, savedInIni, d, defaultKeyboardBinding, f)                           \
      if(savedInIni)                                                                            \
         inputCodeManager->setBinding(enumVal, InputModeKeyboard,                            	\
            getInputCode(ini, "KeyboardKeyBindings", InputCodeManager::getBindingName(enumVal), \
                         defaultKeyboardBinding));
    BINDING_TABLE
#undef BINDING


   ///// JOYSTICK

   // Basically the same, except that we use the default joystick binding column... generated code will look pretty much the same
#define BINDING(enumVal, b, savedInIni, d, e, defaultJoystickBinding)                           \
      if(savedInIni)                                                                            \
         inputCodeManager->setBinding(enumVal, InputModeJoystick,                               \
            getInputCode(ini, "JoystickKeyBindings", InputCodeManager::getBindingName(enumVal), \
                         defaultJoystickBinding));
    BINDING_TABLE
#undef BINDING

   // Keys where savedInIni is false are not user-defineable at the moment, mostly because we want consistency
   // throughout the game, and that would require some real constraints on what keys users could choose.
   // keyHELP = KEY_F1;
   // keyOUTGAMECHAT = KEY_F5;
   // keyFPS = KEY_F6;
   // keyDIAG = KEY_F7;
}


// Only called while loading keys from the INI; Note that this function might not be able to be modernized!
void setDefaultEditorKeyBindings(CIniFile *ini, InputCodeManager *inputCodeManager)
{
#define EDITOR_BINDING(editorEnumVal, b, c, defaultEditorKeyboardBinding)                                       \
      inputCodeManager->setEditorBinding(editorEnumVal,                                                         \
                                          getInputString(ini, "EditorKeyBindings",                              \
                                                         InputCodeManager::getEditorBindingName(editorEnumVal), \
                                                         defaultEditorKeyboardBinding)); 
    EDITOR_BINDING_TABLE
#undef EDITOR_BINDING
}


// Only called while loading keys from the INI
void setDefaultSpecialKeyBindings(CIniFile *ini, InputCodeManager *inputCodeManager)
{
#define SPECIAL_BINDING(specialEnumVal, b, c, defaultSpecialKeyboardBinding)                                      \
      inputCodeManager->setSpecialBinding(specialEnumVal,                                                         \
                                          getInputString(ini, "SpecialKeyBindings",                               \
                                                         InputCodeManager::getSpecialBindingName(specialEnumVal), \
                                                         defaultSpecialKeyboardBinding)); 
    SPECIAL_BINDING_TABLE
#undef SPECIAL_BINDING
}


static void writeKeyBindings(CIniFile *ini, InputCodeManager *inputCodeManager, const string &section, InputMode mode)
{
   // Top line evaluates to:
   // if(true)
   //    ini->SetValue(section, InputCodeManager::getBindingName(InputCodeManager::BINDING_SELWEAP1),
   //                           InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_SELWEAP1, mode)));

#define BINDING(enumVal, b, savedInIni, d, e, f)                                                                   \
      if(savedInIni)                                                                                               \
         ini->SetValue(section, InputCodeManager::getBindingName(enumVal),                                         \
                                InputCodeManager::inputCodeToString(inputCodeManager->getBinding(enumVal, mode))); 
    BINDING_TABLE
#undef BINDING
}


// Note that this function might not be able to be modernized!
static void writeEditorKeyBindings(CIniFile *ini, InputCodeManager *inputCodeManager, const string &section)
{
   string key;

   // Expands to:
   // key = InputCodeManager::getEditorBindingName(FlipItemHorizontal); 
   // if(!ini->hasKey(section, key))
   //   ini->SetValue(section, key, inputCodeManager->getEditorBinding(editorEnumVal));

   // Don't overwrite existing bindings for now... there is no way to modify them in-game, and if the user has
   // specified an invalid binding, leaving it wrong will make it easier for them to find and fix the error
#define EDITOR_BINDING(editorEnumVal, b, c, d)                                               \
      key = InputCodeManager::getEditorBindingName(editorEnumVal);                           \
      if(!ini->hasKey(section, key))                                                         \
         ini->SetValue(section, key, inputCodeManager->getEditorBinding(editorEnumVal));
    EDITOR_BINDING_TABLE
#undef EDITOR_BINDING
}


static void writeSpecialKeyBindings(CIniFile *ini, InputCodeManager *inputCodeManager, const string &section)
{
   string key;

   // Expands to:
   // key = InputCodeManager::getSpecialBindingName(FlipItemHorizontal); 
   // if(!ini->hasKey(section, key))
   //   ini->SetValue(section, key, inputCodeManager->getSpecialBinding(specialEnumVal));

   // Don't overwrite existing bindings for now... there is no way to modify them in-game, and if the user has
   // specified an invalid binding, leaving it wrong will make it easier for them to find and fix the error
#define SPECIAL_BINDING(specialEnumVal, b, c, d)                                            \
      key = InputCodeManager::getSpecialBindingName(specialEnumVal);                        \
      if(!ini->hasKey(section, key))                                                        \
         ini->SetValue(section, key, inputCodeManager->getSpecialBinding(specialEnumVal));
    SPECIAL_BINDING_TABLE
#undef SPECIAL_BINDING
}


static void writeKeyBindings(CIniFile *ini, InputCodeManager *inputCodeManager)
{
   writeKeyBindings(ini, inputCodeManager, "KeyboardKeyBindings", InputModeKeyboard);
   writeKeyBindings(ini, inputCodeManager, "JoystickKeyBindings", InputModeJoystick);
   writeEditorKeyBindings (ini, inputCodeManager, "EditorKeyboardKeyBindings");
   writeSpecialKeyBindings(ini, inputCodeManager, "SpecialKeyBindings");
}


static void insertQuickChatMessageCommonBits(CIniFile *ini, const string &key, 
                                   MessageType messageType, 
                                   InputCode keyCode, InputCode buttonCode, 
                                   const string &caption)
{
   ini->SetValue(key, "Key", InputCodeManager::inputCodeToString(keyCode));
   ini->SetValue(key, "Button", InputCodeManager::inputCodeToString(buttonCode));
   ini->SetValue(key, "MessageType", Evaluator::toString(messageType));
   ini->SetValue(key, "Caption", caption);
}


static void insertQuickChatMessageSection(CIniFile *ini, S32 group, MessageType messageType, 
                                   InputCode keyCode, InputCode buttonCode, 
                                   const string &caption)
{
   const string key = "QuickChatMessagesGroup" + itos(group);

   insertQuickChatMessageCommonBits(ini, key, messageType, keyCode, buttonCode, caption);
}


static void insertQuickChatMessage(CIniFile *ini, S32 group, S32 messageId, MessageType messageType, 
                                   InputCode keyCode, InputCode buttonCode, 
                                   const string &caption, const string &message)
{
   const string key = "QuickChatMessagesGroup" + itos(group) + "_Message" + itos(messageId);

   insertQuickChatMessageCommonBits(ini, key, messageType, keyCode, buttonCode, caption);
   ini->SetValue(key, "Message", message);
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

   == or, a top-tiered message might look like this ==

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
   QuickChatNode emptyNode;

   QuickChatHelper::nodeTree.push_back(emptyNode);

   S32 keys = ini->GetNumSections();
   Vector<string> groups;

   // Read any top-level messages (those starting with "QuickChat_Message")
   Vector<string> messages;
   for(S32 i = 0; i < keys; i++)
   {
      string keyName = ini->getSectionName(i);
      if(keyName.substr(0, 17) == "QuickChat_Message")   // Found message group
         messages.push_back(keyName);
   }

   messages.sort(alphaSort);

   for(S32 i = messages.size() - 1; i >= 0; i--)
      QuickChatHelper::nodeTree.push_back(QuickChatNode(1, ini, messages[i], true));

   // Now search for groups, which have keys matching "QuickChatMessagesGroup123"
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

   for(S32 i = groups.size() - 1; i >= 0; i--)
   {
      Vector<string> messages;
      for(S32 j = 0; j < keys; j++)
      {
         string keyName = ini->getSectionName(j);
         if(keyName.substr(0, groups[i].length() + 1) == groups[i] + "_")
            messages.push_back(keyName);
      }

      messages.sort(alphaSort);

      QuickChatHelper::nodeTree.push_back(QuickChatNode(1, ini, groups[i], true));

      for(S32 j = messages.size() - 1; j >= 0; j--)
         QuickChatHelper::nodeTree.push_back(QuickChatNode(2, ini, messages[j], false));
   }

   // Add final node.  Last verse, same as the first.
   QuickChatHelper::nodeTree.push_back(emptyNode);
#endif
}


static void writeDefaultQuickChatMessages(CIniFile *ini, IniSettings *iniSettings)
{
   const char *section = "QuickChatMessages";

   ini->addSection(section);
   if(ini->numSectionComments(section) == 0)
   {
      addComment("----------------");
      addComment(" WARNING!  Do not edit this section while Bitfighter is running... your changes will be clobbered!");
      addComment("----------------");
      addComment(" The structure of the QuickChatMessages sections is a bit complicated.  The structure reflects the");
      addComment(" way the messages are displayed in the QuickChat menu, so make sure you are familiar with that before");
      addComment(" you start modifying these items. ");
      addComment(" ");
      addComment(" Messages are grouped, and each group has a Caption (short name");
      addComment(" shown on screen), a Key (the shortcut key used to select the group), and a Button (a shortcut button");
      addComment(" used when in joystick mode).  If the Button is \"Undefined key\", then that item will not be shown");
      addComment(" in joystick mode, unless the setting is true.  Groups can be defined in");
      addComment(" any order, but will be displayed sorted by [section] name.  Groups are designated by the");
      addComment(" [QuickChatMessagesGroupXXX] sections, where XXX is a unique suffix, usually a number.");
      addComment(" ");
      addComment(" Each group can have one or more messages, as specified by the [QuickChatMessagesGroupXXX_MessageYYY]");
      addComment(" sections, where XXX is the unique group suffix, and YYY is a unique message suffix.  Again, messages");
      addComment(" can be defined in any order, and will appear sorted by their [section] name.  Key, Button, and");
      addComment(" Caption serve the same purposes as in the group definitions. Message is the actual message text that");
      addComment(" is sent, and MessageType should be either \"Team\" or \"Global\", depending on which users the");
      addComment(" message should be sent to.  You can mix Team and Global messages in the same section, but it may be");
      addComment(" less confusing not to do so.  MessageType can also be \"Command\", in which case the message will be");
      addComment(" sent to the server, as if it were a /command; see below for more details.");
      addComment(" ");
      addComment(" Messages can also be added to the top-tier of items, by specifying a section like [QuickChat_MessageZZZ].");
      addComment(" ");
      addComment(" Note that quotes are not required around Messages or Captions, and if included, they will be sent as");
      addComment(" part of the message. Also, if you bullocks things up too badly, simply delete all QuickChatMessage");
      addComment(" sections, along with this section and all comments, and a clean set of commands will be regenerated"); 
      addComment(" the next time you run the game (though your modifications will be lost, obviously).");
      addComment(" ");
      addComment(" Note that you can also use the QuickChat functionality to create shortcuts to commonly run /commands");
      addComment(" by setting the MessageType to \"Command\".  For example, if you define a QuickChat message to be");
      addComment(" \"addbots 2\" (without quotes, and without a leading \"/\"), and the MessageType to \"Command\" (also");
      addComment(" without quotes), 2 robots will be added to the game when you select the appropriate message.  You can");
      addComment(" use this functionality to assign commonly used commands to joystick buttons or short key sequences.");
      addComment("----------------");
   }


   // Are there any QuickChatMessageGroups?  If not, we'll write the defaults.
   S32 keys = ini->GetNumSections();

   for(S32 i = 0; i < keys; i++)
   {
      string keyName = ini->getSectionName(i);
      if(keyName.substr(0, 22) == "QuickChatMessagesGroup" && keyName.find("_") == string::npos)
         return;
   }

   insertQuickChatMessageSection(ini, 1, GlobalMessageType, KEY_G, BUTTON_6, "Global");
      insertQuickChatMessage(ini, 1, 1, GlobalMessageType, KEY_A, BUTTON_1,    "No Problem",            "No Problemo.");
      insertQuickChatMessage(ini, 1, 2, GlobalMessageType, KEY_T, BUTTON_2,    "Thanks",                "Thanks.");
      insertQuickChatMessage(ini, 1, 3, GlobalMessageType, KEY_X, KEY_UNKNOWN, "You idiot!",            "You idiot!");
      insertQuickChatMessage(ini, 1, 4, GlobalMessageType, KEY_E, BUTTON_3,    "Duh",                   "Duh.");
      insertQuickChatMessage(ini, 1, 5, GlobalMessageType, KEY_C, KEY_UNKNOWN, "Crap",                  "Ah Crap!");
      insertQuickChatMessage(ini, 1, 6, GlobalMessageType, KEY_D, BUTTON_4,    "Damnit",                "Dammit!");
      insertQuickChatMessage(ini, 1, 7, GlobalMessageType, KEY_S, BUTTON_5,    "Shazbot",               "Shazbot!");
      insertQuickChatMessage(ini, 1, 8, GlobalMessageType, KEY_Z, BUTTON_6,    "Doh",                   "Doh!");

   insertQuickChatMessageSection(ini, 2, TeamMessageType, KEY_D, BUTTON_5, "Defense");
      insertQuickChatMessage(ini, 2, 1, TeamMessageType, KEY_G, KEY_UNKNOWN,   "Defend Our Base",       "Defend our base.");
      insertQuickChatMessage(ini, 2, 2, TeamMessageType, KEY_D, BUTTON_1,      "Defending Base",        "Defending our base.");
      insertQuickChatMessage(ini, 2, 3, TeamMessageType, KEY_Q, BUTTON_2,      "Is Base Clear?",        "Is our base clear?");
      insertQuickChatMessage(ini, 2, 4, TeamMessageType, KEY_C, BUTTON_3,      "Base Clear",            "Base is secured.");
      insertQuickChatMessage(ini, 2, 5, TeamMessageType, KEY_T, BUTTON_4,      "Base Taken",            "Base is taken.");
      insertQuickChatMessage(ini, 2, 6, TeamMessageType, KEY_N, BUTTON_5,      "Need More Defense",     "We need more defense.");
      insertQuickChatMessage(ini, 2, 7, TeamMessageType, KEY_E, BUTTON_6,      "Enemy Attacking Base",  "The enemy is attacking our base.");
      insertQuickChatMessage(ini, 2, 8, TeamMessageType, KEY_A, KEY_UNKNOWN,   "Attacked",              "We are being attacked.");

   insertQuickChatMessageSection(ini, 3, TeamMessageType, KEY_F, BUTTON_4, "Flag");
      insertQuickChatMessage(ini, 3, 1, TeamMessageType, KEY_F, BUTTON_1,      "Get enemy flag",        "Get the enemy flag.");
      insertQuickChatMessage(ini, 3, 2, TeamMessageType, KEY_R, BUTTON_2,      "Return our flag",       "Return our flag to base.");
      insertQuickChatMessage(ini, 3, 3, TeamMessageType, KEY_S, BUTTON_3,      "Flag secure",           "Our flag is secure.");
      insertQuickChatMessage(ini, 3, 4, TeamMessageType, KEY_H, BUTTON_4,      "Have enemy flag",       "I have the enemy flag.");
      insertQuickChatMessage(ini, 3, 5, TeamMessageType, KEY_E, BUTTON_5,      "Enemy has flag",        "The enemy has our flag!");
      insertQuickChatMessage(ini, 3, 6, TeamMessageType, KEY_G, BUTTON_6,      "Flag gone",             "Our flag is not in the base!");

   insertQuickChatMessageSection(ini, 4, TeamMessageType, KEY_S, KEY_UNKNOWN, "Incoming Enemies - Direction");
      insertQuickChatMessage(ini, 4, 1, TeamMessageType, KEY_S, KEY_UNKNOWN,   "Incoming South",        "*** INCOMING SOUTH ***");
      insertQuickChatMessage(ini, 4, 2, TeamMessageType, KEY_E, KEY_UNKNOWN,   "Incoming East",         "*** INCOMING EAST  ***");
      insertQuickChatMessage(ini, 4, 3, TeamMessageType, KEY_W, KEY_UNKNOWN,   "Incoming West",         "*** INCOMING WEST  ***");
      insertQuickChatMessage(ini, 4, 4, TeamMessageType, KEY_N, KEY_UNKNOWN,   "Incoming North",        "*** INCOMING NORTH ***");
      insertQuickChatMessage(ini, 4, 5, TeamMessageType, KEY_V, KEY_UNKNOWN,   "Incoming Enemies",      "Incoming enemies!");

   insertQuickChatMessageSection(ini, 5, TeamMessageType, KEY_V, BUTTON_3, "Quick");
      insertQuickChatMessage(ini, 5, 1, TeamMessageType, KEY_J, KEY_UNKNOWN,   "Capture the objective", "Capture the objective.");
      insertQuickChatMessage(ini, 5, 2, TeamMessageType, KEY_O, KEY_UNKNOWN,   "Go on the offensive",   "Go on the offensive.");
      insertQuickChatMessage(ini, 5, 3, TeamMessageType, KEY_A, BUTTON_1,      "Attack!",               "Attack!");
      insertQuickChatMessage(ini, 5, 4, TeamMessageType, KEY_W, BUTTON_2,      "Wait for signal",       "Wait for my signal to attack.");
      insertQuickChatMessage(ini, 5, 5, TeamMessageType, KEY_V, BUTTON_3,      "Help!",                 "Help!");
      insertQuickChatMessage(ini, 5, 6, TeamMessageType, KEY_E, BUTTON_4,      "Regroup",               "Regroup.");
      insertQuickChatMessage(ini, 5, 7, TeamMessageType, KEY_G, BUTTON_5,      "Going offense",         "Going offense.");
      insertQuickChatMessage(ini, 5, 8, TeamMessageType, KEY_Z, BUTTON_6,      "Move out",              "Move out.");

   insertQuickChatMessageSection(ini, 6, TeamMessageType, KEY_R, BUTTON_2, "Reponses");
      insertQuickChatMessage(ini, 6, 1, TeamMessageType, KEY_A, BUTTON_1,      "Acknowledge",           "Acknowledged.");
      insertQuickChatMessage(ini, 6, 2, TeamMessageType, KEY_N, BUTTON_2,      "No",                    "No.");
      insertQuickChatMessage(ini, 6, 3, TeamMessageType, KEY_Y, BUTTON_3,      "Yes",                   "Yes.");
      insertQuickChatMessage(ini, 6, 4, TeamMessageType, KEY_S, BUTTON_4,      "Sorry",                 "Sorry.");
      insertQuickChatMessage(ini, 6, 5, TeamMessageType, KEY_T, BUTTON_5,      "Thanks",                "Thanks.");
      insertQuickChatMessage(ini, 6, 6, TeamMessageType, KEY_D, BUTTON_6,      "Don't know",            "I don't know.");

   insertQuickChatMessageSection(ini, 7, GlobalMessageType, KEY_T, BUTTON_1, "Taunts");
      insertQuickChatMessage(ini, 7, 1, GlobalMessageType, KEY_R, KEY_UNKNOWN, "Rawr",                  "RAWR!");
      insertQuickChatMessage(ini, 7, 2, GlobalMessageType, KEY_C, BUTTON_1,    "Come get some!",        "Come get some!");
      insertQuickChatMessage(ini, 7, 3, GlobalMessageType, KEY_D, BUTTON_2,    "Dance!",                "Dance!"); 
      insertQuickChatMessage(ini, 7, 4, GlobalMessageType, KEY_X, BUTTON_3,    "Missed me!",            "Missed me!");
      insertQuickChatMessage(ini, 7, 5, GlobalMessageType, KEY_W, BUTTON_4,    "I've had worse...",     "I've had worse...");
      insertQuickChatMessage(ini, 7, 6, GlobalMessageType, KEY_Q, BUTTON_5,    "How'd THAT feel?",      "How'd THAT feel?");
      insertQuickChatMessage(ini, 7, 7, GlobalMessageType, KEY_E, BUTTON_6,    "Yoohoo!",               "Yoohoo!");
}


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


// This is only called once, during initial initialization
// Is also called from gameType::processServerCommand (why?)
void loadSettingsFromINI(CIniFile *ini, GameSettings *settings)
{
   InputCodeManager *inputCodeManager = settings->getInputCodeManager();
   IniSettings *iniSettings = settings->getIniSettings();

   ini->ReadFile();                             // Read the INI file

   for(U32 i = 0; i < ARRAYSIZE(sections); i++)
      loadSettings(ini, iniSettings, sections[i]);

   // These two sections can be modernized, the remainder maybe not
   loadSoundSettings(ini, settings, iniSettings);
   loadGeneralSettings(ini, iniSettings);

   loadLoadoutPresets(ini, settings);
   loadPluginBindings(ini, iniSettings);

   setDefaultKeyBindings(ini, inputCodeManager);
   setDefaultEditorKeyBindings(ini, inputCodeManager);
   setDefaultSpecialKeyBindings(ini, inputCodeManager);

   loadForeignServerInfo(ini, iniSettings);     // Info about other servers
   loadLevels(ini, iniSettings);                // Read levels, if there are any
   loadLevelSkipList(ini, settings);            // Read level skipList, if there are any

   loadQuickChatMessages(ini);
   loadServerBanList(ini, settings->getBanList());

   saveSettingsToINI(ini, settings);            // Save to fill in any missing settings

   settings->onFinishedLoading();               // Merge INI settings with cmd line settings
}


void IniSettings::loadUserSettingsFromINI(CIniFile *ini, GameSettings *settings)
{
   UserSettings userSettings;

   // Get a list of sections... we should have one per user
   S32 sections = ini->GetNumSections();

   for(S32 i = 0; i < sections; i++)
   {
      userSettings.name = ini->getSectionName(i);

      string seenList = ini->GetValue(userSettings.name, "LevelupItemsAlreadySeenList", "");
      IniSettings::iniStringToBitArray(seenList, userSettings.levelupItemsAlreadySeen, userSettings.LevelCount);

      settings->addUserSettings(userSettings);
   }
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


void saveWindowPosition(CIniFile *ini, S32 x, S32 y)
{
   ini->SetValueI("Settings", "WindowXPos", x);
   ini->SetValueI("Settings", "WindowYPos", y);
}


static void writeSettings(CIniFile *ini, IniSettings *iniSettings)
{
   TNLAssert(ARRAYSIZE(sections) == ARRAYSIZE(headerComments), "Mismatch!");
   static const string HorizontalLine = "----------------";

   for(U32 i = 0; i < ARRAYSIZE(sections); i++)
   {
      ini->addSection(sections[i]);

      const string section = sections[i];

      SettingsType settings = iniSettings->mSettings.getSettingsInSection(section);
   
      if(true || ini->numSectionComments(section) == 0)  // <<<==== remove true when done testing!
      {
         const Vector<string> comments = wrapString(headerComments[i], NO_AUTO_WRAP);

         ini->deleteSectionComments(section);      // Delete when done testing (harmless but useless)

         ini->sectionComment(section, HorizontalLine);      // ----------------
         for(S32 i = 0; i < comments.size(); i++)
            ini->sectionComment(section, " " + comments[i]);
         ini->sectionComment(section, HorizontalLine);      // ----------------

         // Write all our section comments for items defined in the new manner
         for(S32 i = 0; i < settings.size(); i++)
         {
            // Pass NO_AUTO_WRAP as width to disable automatic wrapping... we'll rely on \ns to do our wrapping here
            const string prefix = settings[i]->getKey() + " - ";
            const Vector<string> comments = wrapString(prefix + settings[i]->getComment(), NO_AUTO_WRAP, string(prefix.size(), ' '));
            for(S32 j = 0; j < comments.size(); j++)
               ini->sectionComment(section, " " + comments[j]);
         }

         ini->sectionComment(section, HorizontalLine);      // ----------------
      }

      // Write the settings themselves
      for(S32 i = 0; i < settings.size(); i++)
         ini->SetValue(section, settings[i]->getKey(), settings[i]->getIniString());
   }

   const char *section = "Settings";

   ini->sectionComment(section, " WindowXPos, WindowYPos - Position of window in window mode (will overwritten if you move your window)");
   ini->sectionComment(section, " WindowScalingFactor - Used to set size of window.  1.0 = 800x600. Best to let the program manage this setting.");
   ini->sectionComment(section, " LoadoutIndicators - Display indicators showing current weapon?  Yes/No");
   ini->sectionComment(section, " JoystickLinuxUseOldDeviceSystem - Force SDL to add the older /dev/input/js0 device to the enumerated joystick list.  No effect on Windows/Mac systems");
   ini->sectionComment(section, " AlwaysStartInKeyboardMode - Change to 'Yes' to always start the game in keyboard mode (don't auto-select the joystick)");
   ini->sectionComment(section, " MasterServerAddressList - Comma seperated list of Address of master server, in form: IP:67.18.11.66:25955,IP:myMaster.org:25955 (tries all listed, only connects to one at a time)");
   ini->sectionComment(section, " DefaultName - Name that will be used if user hits <enter> on name entry screen without entering one");
   ini->sectionComment(section, " Nickname - Specify the nickname to use for autologin, or clear to disable autologin");
   ini->sectionComment(section, " Password - Password to use for autologin, if your nickname has been reserved in the forums");
   ini->sectionComment(section, " LastPassword - Password user entered when game last run (may be overwritten if you enter a different pw on startup screen)");
   ini->sectionComment(section, " LastEditorName - Last edited file name");
   ini->sectionComment(section, " MaxFPS - Maximum FPS the client will run at.  Higher values use more CPU, lower may increase lag (default = 100)");
   ini->sectionComment(section, " LineWidth - Width of a \"standard line\" in pixels (default 2); can set with /linewidth in game");
   ini->sectionComment(section, " Version - Version of game last time it was run.  Don't monkey with this value; nothing good can come of it!");
   ini->sectionComment(section, " QueryServerSortColumn - Index of column to sort by when in the Join Servers menu. (0 is first col.)  This value managed by game.");
   ini->sectionComment(section, " QueryServerSortAscending - 1 for ascending sort, 0 for descending.  This value managed by game.");

   ini->sectionComment(section, "----------------");


   // And the ones still to be ported to the new system


   saveWindowPosition(ini, iniSettings->winXPos, iniSettings->winYPos);

   ini->SetValueF (section, "WindowScalingFactor", iniSettings->winSizeFact);

#ifndef ZAP_DEDICATED
   ini->setValueYN(section, "JoystickLinuxUseOldDeviceSystem", iniSettings->joystickLinuxUseOldDeviceSystem);
   ini->setValueYN(section, "AlwaysStartInKeyboardMode", iniSettings->alwaysStartInKeyboardMode);
#endif
   ini->SetValue  (section, "MasterServerAddressList", iniSettings->masterAddress);
   ini->SetValue  (section, "DefaultName", iniSettings->defaultName);
   ini->SetValue  (section, "Nickname", iniSettings->name);
   ini->SetValue  (section, "Password", iniSettings->password);
   ini->SetValue  (section, "LastPassword", iniSettings->lastPassword);
   ini->SetValue  (section, "LastEditorName", iniSettings->lastEditorName);

   ini->SetValueI (section, "MaxFPS", iniSettings->maxFPS);  

   ini->SetValueI (section, "ConnectionSpeed", iniSettings->connectionSpeed);  
   ini->SetValueI (section, "Version", BUILD_VERSION);

   ini->SetValueI (section, "QueryServerSortColumn",    iniSettings->queryServerSortColumn);
   ini->SetValueB (section, "QueryServerSortAscending", iniSettings->queryServerSortAscending);

#ifndef ZAP_DEDICATED
   // Don't save new value if out of range, so it will go back to the old value. 
   // Just in case a user screw up with /linewidth command using value too big or too small.
   if(gDefaultLineWidth >= 0.5 && gDefaultLineWidth <= 5)
      ini->SetValueF (section, "LineWidth", gDefaultLineWidth);
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
   writePasswordSection_helper(ini, "SavedOwnerPasswords");
   writePasswordSection_helper(ini, "SavedServerPasswords");
}


static void writeINIHeader(CIniFile *ini)
{
   if(!ini->NumHeaderComments())
   {
      //ini->headerComment("Bitfighter configuration file");
      //ini->headerComment("=============================");
      //ini->headerComment(" This file is intended to be user-editable, but some settings here may be overwritten by the game.");
      //ini->headerComment(" If you specify any cmd line parameters that conflict with these settings, the cmd line options will be used.");
      //ini->headerComment(" First, some basic terminology:");
      //ini->headerComment(" [section]");
      //ini->headerComment(" key=value");

      string headerComments =
         "Bitfighter configuration file\n"
         "=============================\n"
         "This file is intended to be user-editable, but some settings here may be overwritten by the game. "
         "If you specify any cmd line parameters that conflict with these settings, the cmd line options will be used.\n"
         "\n"
         "First, some basic terminology:\n"
         "\t[section]\n"
         "\tkey=value\n";

      Vector<string> lines = wrapString(headerComments, 100);

      for(S32 i = 0; i < lines.size(); i++)
         ini->headerComment(" " + lines[i]);

      ini->headerComment("");
   }
}


// Save more commonly altered settings first to make them easier to find
void saveSettingsToINI(CIniFile *ini, GameSettings *settings)
{
   writeINIHeader(ini);

   IniSettings *iniSettings = settings->getIniSettings();

   writeForeignServerInfo(ini, iniSettings);
   writeLoadoutPresets(ini, settings);
   writePluginBindings(ini, iniSettings);
   writeConnectionsInfo(ini, iniSettings);
   writeSounds(ini, iniSettings);
   writeSettings(ini, iniSettings);
   writeLevels(ini);
   writeSkipList(ini, settings->getLevelSkipList());
   writePasswordSection(ini);
   writeKeyBindings(ini, settings->getInputCodeManager());
   
   writeDefaultQuickChatMessages(ini, iniSettings);  // Does nothing if there are already chat messages in the INI

   // only needed for users using custom joystick 
   // or joystick that maps differenly in LINUX
   // This adds 200+ lines.
   //writeJoystick();
   writeServerBanList(ini, settings->getBanList());

   ini->WriteFile();    // Commit the file to disk
}


void IniSettings::saveUserSettingsToINI(const string &name, CIniFile *ini, GameSettings *settings)
{
   const UserSettings *userSettings = settings->getUserSettings(name);

   string val = IniSettings::bitArrayToIniString(userSettings->levelupItemsAlreadySeen, userSettings->LevelCount);

   ini->SetValue(name, "LevelupItemsAlreadySeenList", val, true);
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
   resolveDirs(getExecutableDir());
}


// Constructor
FolderManager::FolderManager(const string &levelDir,    const string &robotDir,  const string &sfxDir,        const string &musicDir, 
                             const string &iniDir,      const string &logDir,    const string &screenshotDir, const string &luaDir,
                             const string &rootDataDir, const string &pluginDir, const string &fontsDir,      const string &recordDir) :
               levelDir      (levelDir),
               robotDir      (robotDir),
               sfxDir        (sfxDir),
               musicDir      (musicDir),
               iniDir        (iniDir),
               logDir        (logDir),
               screenshotDir (screenshotDir),
               luaDir        (luaDir),
               rootDataDir   (rootDataDir),
               pluginDir     (pluginDir),
               fontsDir      (fontsDir),
               recordDir     (recordDir)
{
   // Do nothing (more)
}


// Destructor
FolderManager::~FolderManager()
{
   // Do nothing
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

// Getters
string FolderManager::getLevelDir()      const { return levelDir;      }
string FolderManager::getIniDir()        const { return iniDir;        }
string FolderManager::getRecordDir()     const { return recordDir;     }
string FolderManager::getRobotDir()      const { return robotDir;      }
string FolderManager::getFontsDir()      const { return fontsDir;      }
string FolderManager::getScreenshotDir() const { return screenshotDir; }
string FolderManager::getSfxDir()        const { return sfxDir;        }
string FolderManager::getMusicDir()      const { return musicDir;      }
string FolderManager::getRootDataDir()   const { return rootDataDir;   }
string FolderManager::getLogDir()        const { return logDir;        }
string FolderManager::getPluginDir()     const { return pluginDir;     }
string FolderManager::getLuaDir()        const { return luaDir;        }


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
   folderManager->recordDir     = resolutionHelper(cmdLineDirs.musicDir,      rootDataDir, "record");

   // rootDataDir not used for these folders
   folderManager->sfxDir        = resolutionHelper(cmdLineDirs.sfxDir,        "", "sfx");
   folderManager->fontsDir      = resolutionHelper(cmdLineDirs.fontsDir,      "", "fonts");

   gSqlite = folderManager->logDir + "stats";
}


void FolderManager::resolveDirs(const string &root)
{
   rootDataDir = root;

   // root used to specify the following folders
   robotDir      = joindir(root, "robots");
   pluginDir     = joindir(root, "editor_plugins");
   luaDir        = joindir(root, "scripts");
   iniDir        = joindir(root, "");
   logDir        = joindir(root, "");
   screenshotDir = joindir(root, "screenshots");
   musicDir      = joindir(root, "music");
   recordDir     = joindir(root, "record");

   // root not used for these folders
   sfxDir        = joindir("", "sfx");
   fontsDir      = joindir("", "fonts");

   gSqlite = logDir + "stats";
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


string FolderManager::findLevelFile(const string &leveldir, const string &filename)
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


// Destructor
CmdLineSettings::~CmdLineSettings()
{
   // Do nothing
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
