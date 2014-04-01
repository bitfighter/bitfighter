//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CONFIG_ENUM_H_
#define _CONFIG_ENUM_H_


//               Data type           Setting name              INI Section       INI Key                     Default value                    INI Comment                                               
#define SETTINGS_TABLE   \
   SETTINGS_ITEM(string,             LastName,                 "Settings",       "LastName",                 "ChumpChange",                   "Name user entered when game last run (may be overwritten if you enter a different name on startup screen)")  \
   SETTINGS_ITEM(DisplayMode,        WindowMode,               "Settings",       "WindowMode",               DISPLAY_MODE_WINDOWED,           "Fullscreen, Fullscreen-Stretch or Window")                                                                   \
   SETTINGS_ITEM(YesNo,              UseFakeFullscreen,        "Settings",       "UseFakeFullscreen",        Yes,                             "Faster fullscreen switching; however, may not cover the taskbar")                                            \
   SETTINGS_ITEM(RelAbs,             ControlMode,              "Settings",       "ControlMode",              Absolute,                        "Use Relative or Absolute controls (Relative means left is ship's left, Absolute means left is screen left)") \
   SETTINGS_ITEM(YesNo,              VoiceEcho,                "Settings",       "VoiceEcho",                No,                              "Play echo when recording a voice message? Yes/No")                                                           \
   SETTINGS_ITEM(YesNo,              ShowInGameHelp,           "Settings",       "ShowInGameHelp",           Yes,                             "Show tutorial style messages in-game?  Yes/No")                                                              \
   SETTINGS_ITEM(string,             JoystickType,             "Settings",       "JoystickType",             NoJoystick,                      "Type of joystick to use if auto-detect doesn't recognize your controller")                                   \
   SETTINGS_ITEM(string,             HelpItemsAlreadySeenList, "Settings",       "HelpItemsAlreadySeenList", "",                              "Tracks which in-game help items have already been seen; let the game manage this")                           \
   SETTINGS_ITEM(U32,                EditorGridSize,           "Settings",       "EditorGridSize",           255,                             "Grid size used in the editor, mostly for snapping purposes")                                                 \
   SETTINGS_ITEM(YesNo,              LineSmoothing,            "Settings",       "LineSmoothing",            Yes,                             "Activates anti-aliased rendering.  This may be a little slower on some machines.  Yes/No")                   \
                                                                                                                                                                                                                                                            \
   SETTINGS_ITEM(ColorEntryMode,     ColorEntryMode,           "EditorSettings", "ColorEntryMode",           ColorEntryMode100,               "Specifies which color entry mode to use: RGB100, RGB255, RGBHEX; best to let the game manage this")          \
                                                                                                                                                                                                                                                            \
   SETTINGS_ITEM(YesNo,              NeverConnectDirect,       "Testing",        "NeverConnectDirect",       No,                              "Never connect to pingable internet server directly; forces arranged connections via master")                 \
   SETTINGS_ITEM(Color,              WallFillColor,            "Testing",        "WallFillColor",            Colors::DefaultWallFillColor,    "Color used locally for rendering wall fill (r g b), (values between 0 and 1), or #hexcolor")                 \
   SETTINGS_ITEM(Color,              WallOutlineColor,         "Testing",        "WallOutlineColor",         Colors::DefaultWallOutlineColor, "Color used locally for rendering wall outlines (r g b), (values between 0 and 1), or #hexcolor")             \
   SETTINGS_ITEM(GoalZoneFlashStyle, GoalZoneFlashStyle,       "Testing",        "GoalZoneFlashStyle",       GoalZoneFlashOriginal,           "Different flash patterns when goal is captured in ZC game: Original, Experimental, None")                    \
   SETTINGS_ITEM(U16,                ClientPortNumber,         "Testing",        "ClientPortNumber",         0,                               "Only helps when punching through firewall when using router's port forwarded for client port number")        \
   SETTINGS_ITEM(YesNo,              DisableScreenSaver,       "Testing",        "DisableScreenSaver",       Yes,                             "Disable ScreenSaver from having no input from keyboard/mouse, useful when using joystick")                   \
                                                                                                                                                                                                                                                            \
   SETTINGS_ITEM(string,             ServerName,               "Host",           "ServerName",              "Bitfighter host",                "The name others will see when they are browsing for servers (max 20 chars)")                                                   \
   SETTINGS_ITEM(string,             ServerAddress,            "Host",           "ServerAddress",            MASTER_SERVER_LIST_ADDRESS,      "Socket address and port to bind to, e.g. IP:Any:9876 or IP:54.35.110.99:8000 or IP:bitfighter.org:8888\n"                      \
                                                                                                                                              "(leave blank to let the system decide; this is almost always what you want)")                                                  \
   SETTINGS_ITEM(string,             ServerDescription,        "Host",           "ServerDescription",        "",                              "A one line description of your server.  Please include nickname and physical location!")                                       \
   SETTINGS_ITEM(string,             ServerPassword,           "Host",           "ServerPassword",           "",                              "You can require players to use a password to play on your server.  Leave blank to grant access to all.")                       \
   SETTINGS_ITEM(string,             OwnerPassword,            "Host",           "OwnerPassword",            "",                              "Super admin password.  Gives admin rights + power over admins.  Do not give this out!")                                        \
   SETTINGS_ITEM(string,             AdminPassword,            "Host",           "AdminPassword",            "",                              "Use this password to manage players & change levels on your server.")                                                          \
   SETTINGS_ITEM(string,             LevelChangePassword,      "Host",           "LevelChangePassword",      "",                              "Use this password to change levels on your server.  Leave blank to grant access to all.")                                      \
   SETTINGS_ITEM(string,             LevelDir,                 "Host",           "LevelDir",                 "",                              "Specify where level files are stored; can be overridden on command line with -leveldir param.")                                \
   SETTINGS_ITEM(U32,                MaxPlayers,               "Host",           "MaxPlayers",               127,                             "The max number of players that can play on your server.")                                                                      \
   SETTINGS_ITEM(S32,                MaxBots,                  "Host",           "MaxBots",                  10,                              "The max number of bots allowed on this server.")                                                                               \
   SETTINGS_ITEM(YesNo,              AddRobots,                "Host",           "AddRobots",                No,                              "Add robot players to this server.")                                                                                            \
   SETTINGS_ITEM(S32,                MinBalancedPlayers,       "Host",           "MinBalancedPlayers",       6,                               "The minimum number of players ensured in each map.  Bots will be added up to this number.")                                    \
   SETTINGS_ITEM(YesNo,              EnableServerVoiceChat,    "Host",           "EnableServerVoiceChat",    Yes,                             "If false, prevents any voice chat in a server.")                                                                               \
   SETTINGS_ITEM(YesNo,              AllowGetMap,              "Host",           "AllowGetMap",              No,                              "When getmap is allowed, anyone can download the current level using the /getmap command.")                                     \
   SETTINGS_ITEM(YesNo,              AllowDataConnections,     "Host",           "AllowDataConnections",     No,                              "When data connections are allowed, anyone with the admin password can upload or download levels, bots, or levelGen scripts.\n" \
                                                                                                                                              "This feature is probably insecure, and should be DISABLED unless you require the functionality.")                              \
   SETTINGS_ITEM(YesNo,              LogStats,                 "Host",           "LogStats",                 No,                              "Save game stats locally to built-in sqlite database (saves the same stats as are sent to the master)")                         \
   SETTINGS_ITEM(YesNo,              RandomLevels,             "Host",           "RandomLevels",             No,                              "When current level ends, this can enable randomly switching to any available levels.")                                         \
   SETTINGS_ITEM(YesNo,              SkipUploads,              "Host",           "SkipUploads",              No,                              "When current level ends, enables skipping all uploaded levels.")                                                               \
   SETTINGS_ITEM(YesNo,              AllowMapUpload,           "Host",           "AllowMapUpload",           No,                              "Allow users to upload maps")                                                                                                   \
   SETTINGS_ITEM(YesNo,              AllowAdminMapUpload,      "Host",           "AllowAdminMapUpload",      Yes,                             "Allow admins to upload maps")                                                                                                  \
   SETTINGS_ITEM(YesNo,              AllowLevelgenUpload,      "Host",           "AllowLevelgenUpload",      Yes,                             "Allow users to upload levelgens.  Note that while levelgens run in a sandbox that is mostly secure,\n"                         \
                                                                                                                                              "there may be unknown security risks with this setting.  Use with care.")                                                       \
   SETTINGS_ITEM(YesNo,              AllowTeamChanging,        "Host",           "AllowTeamChanging",        Yes,                             "Allow players to change teams.  You should generally allow this unless there is a good reason to disable it.")                 \
   SETTINGS_ITEM(string,             DefaultRobotScript,       "Host",           "DefaultRobotScript",       "s_bot.bot",                     "If user adds a robot, this script will be used if one is not specified")                                                       \
   SETTINGS_ITEM(string,             GlobalLevelScript,        "Host",           "GlobalLevelScript",        "",                              "Specify a levelgen that will get run on every level")                                                                          \
   SETTINGS_ITEM(YesNo,              GameRecording,            "Host",           "GameRecording",            No,                              "If Yes, games will be recorded; if No, they will not.  This is typically set via the menu.")                                   \
                                                                                                                                                                                                                                                                              \
   SETTINGS_ITEM(YesNo,              VotingEnabled,            "Host-Voting",    "VoteEnable",               No,                              "Enable voting on this server")                                                                                                 \
   SETTINGS_ITEM(U32,                VoteLength,               "Host-Voting",    "VoteLength",               12,                              "Time voting will last for general items, in seconds (use 0 to disable)")                                                       \
   SETTINGS_ITEM(U32,                VoteLengthToChangeTeam,   "Host-Voting",    "VoteLengthToChangeTeam",   10,                              "Time voting will last for team changing, in seconds (use 0 to disable)")                                                       \
   SETTINGS_ITEM(U32,                VoteRetryLength,          "Host-Voting",    "VoteRetryLength",          30,                              "When a vote fails, the vote caller is unable to vote until this many seconds has elapsed.")                                    \
   SETTINGS_ITEM(S32,                VoteYesStrength,          "Host-Voting",    "VoteYesStrength",           3,                              "How much does a Yes vote count?")                                                                                              \
   SETTINGS_ITEM(S32,                VoteNoStrength,           "Host-Voting",    "VoteNoStrength",           -3,                              "How much does a No vote count?")                                                                                               \
   SETTINGS_ITEM(S32,                VoteNothingStrength,      "Host-Voting",    "VoteNothingStrength",      -1,                              "How much does an abstention count?")                                                                                           \


namespace Zap
{

namespace IniKey
{
enum SettingsItem {
#define SETTINGS_ITEM(a, enumVal, c, d, e, f) enumVal,
    SETTINGS_TABLE
#undef SETTINGS_ITEM
};

}

#define DISPLAY_MODES_TABLE \
DISPLAY_MODE_ITEM(DISPLAY_MODE_WINDOWED,                "Window"             )  \
DISPLAY_MODE_ITEM(DISPLAY_MODE_FULL_SCREEN_STRETCHED,   "Fullscreen-Stretch" )  \
DISPLAY_MODE_ITEM(DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED, "Fullscreen"         )  \

// Gernerate an enum
enum DisplayMode {
#define DISPLAY_MODE_ITEM(enumVal, b) enumVal,
    DISPLAY_MODES_TABLE
#undef DISPLAY_MODE_ITEM
    DISPLAY_MODE_UNKNOWN    // <== Note: code depends on this being the first value that's not a real mode
};  

enum sfxSets {
   sfxClassicSet,
   sfxModernSet
};


#define YES_NO_TABLE \
YES_NO_ITEM(No,  "No"  ) /* ==> 0 ==> false */ \
YES_NO_ITEM(Yes, "Yes" ) /* ==> 1 ==> true  */ \

// Gernerate an enum
enum YesNo {
#define YES_NO_ITEM(enumVal, b) enumVal,
    YES_NO_TABLE
#undef YES_NO_ITEM
};


#define COLOR_ENTRY_MODES_TABLE \
COLOR_ENTRY_MODE_ITEM(ColorEntryMode100, "RGB100" )  \
COLOR_ENTRY_MODE_ITEM(ColorEntryMode255, "RGB255" )  \
COLOR_ENTRY_MODE_ITEM(ColorEntryModeHex, "RGBHEX" )  \

// Gernerate an enum
enum ColorEntryMode {
#define COLOR_ENTRY_MODE_ITEM(enumVal, b) enumVal,
    COLOR_ENTRY_MODES_TABLE
#undef COLOR_ENTRY_MODE_ITEM
    ColorEntryModeCount  
}; 


#define GOAL_ZONE_FLASH_TABLE \
GOAL_ZONE_FLASH_ITEM(GoalZoneFlashOriginal,     "Original"     )  \
GOAL_ZONE_FLASH_ITEM(GoalZoneFlashExperimental, "Experimental" )  \
GOAL_ZONE_FLASH_ITEM(GoalZoneFlashNone,         "None"         )  \

// Gernerate an enum
enum GoalZoneFlashStyle {
#define GOAL_ZONE_FLASH_ITEM(enumVal, b) enumVal,
    GOAL_ZONE_FLASH_TABLE
#undef GOAL_ZONE_FLASH_ITEM
};


#define RELATIVE_ABSOLUTE_TABLE \
RELATIVE_ABSOLUTE_ITEM(Relative, "Relative" )  \
RELATIVE_ABSOLUTE_ITEM(Absolute, "Absolute" )  \

// Gernerate an enum
enum RelAbs {
#define RELATIVE_ABSOLUTE_ITEM(enumVal, b) enumVal,
    RELATIVE_ABSOLUTE_TABLE
#undef RELATIVE_ABSOLUTE_ITEM
};


};

#endif /* _CONFIG_ENUM_H_ */
