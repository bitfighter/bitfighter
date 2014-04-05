//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CONFIG_ENUM_H_
#define _CONFIG_ENUM_H_

#ifdef BF_WRITE_TO_MYSQL
#  define MYSQL_SETTINGS_TABLE_ENTRY \
      SETTINGS_ITEM(string, MySqlStatsDatabaseCredentials, "Host", "MySqlStatsDatabaseCredentials", "server, dbname, login, password", NULL, NULL,  \
      "To activate MySql integration, specify the database server, database name, login, and password as a comma delimeted list")
#else
#  define MYSQL_SETTINGS_TABLE_ENTRY
#endif

//                                                                                                                                          Reading    Writing
//               Data type           Setting name              INI Section       INI Key                     Default value                 Validator  Validator   INI Comment                                               
#define SETTINGS_TABLE   \
   SETTINGS_ITEM(string,             LastName,                 "Settings",       "LastName",                 "ChumpChange",                   NULL,     NULL,     "Name user entered when game last run (may be overwritten if you enter a different name on startup screen)")                    \
   SETTINGS_ITEM(DisplayMode,        WindowMode,               "Settings",       "WindowMode",               DISPLAY_MODE_WINDOWED,           NULL,     NULL,     "Fullscreen, Fullscreen-Stretch or Window")                                                                                     \
   SETTINGS_ITEM(YesNo,              UseFakeFullscreen,        "Settings",       "UseFakeFullscreen",        Yes,                             NULL,     NULL,     "Faster fullscreen switching; however, may not cover the taskbar")                                                              \
   SETTINGS_ITEM(RelAbs,             ControlMode,              "Settings",       "ControlMode",              Absolute,                        NULL,     NULL,     "Use Relative or Absolute controls (Relative means left is ship's left, Absolute means left is screen left)")                   \
   SETTINGS_ITEM(YesNo,              VoiceEcho,                "Settings",       "VoiceEcho",                No,                              NULL,     NULL,     "Play echo when recording a voice message? Yes/No")                                                                             \
   SETTINGS_ITEM(YesNo,              ShowInGameHelp,           "Settings",       "ShowInGameHelp",           Yes,                             NULL,     NULL,     "Show tutorial style messages in-game?  Yes/No")                                                                                \
   SETTINGS_ITEM(string,             JoystickType,             "Settings",       "JoystickType",             NoJoystick,                      NULL,     NULL,     "Type of joystick to use if auto-detect doesn't recognize your controller")                                                     \
   SETTINGS_ITEM(string,             HelpItemsAlreadySeenList, "Settings",       "HelpItemsAlreadySeenList", "",                              NULL,     NULL,     "Tracks which in-game help items have already been seen; let the game manage this")                                             \
   SETTINGS_ITEM(U32,                EditorGridSize,           "Settings",       "EditorGridSize",           255,                             NULL,     NULL,     "Grid size used in the editor, mostly for snapping purposes")                                                                   \
   SETTINGS_ITEM(YesNo,              LineSmoothing,            "Settings",       "LineSmoothing",            Yes,                             NULL,     NULL,     "Activates anti-aliased rendering.  This may be a little slower on some machines.  Yes/No")                                     \
                                                                                                                                                                                                                                                                                                  \
   SETTINGS_ITEM(ColorEntryMode,     ColorEntryMode,           "EditorSettings", "ColorEntryMode",           ColorEntryMode100,               NULL,     NULL,     "Specifies which color entry mode to use: RGB100, RGB255, RGBHEX; best to let the game manage this")                            \
                                                                                                                                                                                                                                                                                                  \
   SETTINGS_ITEM(YesNo,              NeverConnectDirect,       "Testing",        "NeverConnectDirect",       No,                              NULL,     NULL,     "Never connect to pingable internet server directly; forces arranged connections via master")                                   \
   SETTINGS_ITEM(Color,              WallFillColor,            "Testing",        "WallFillColor",            Colors::DefaultWallFillColor,    NULL,     NULL,     "Color used locally for rendering wall fill (r g b), (values between 0 and 1), or #hexcolor")                                   \
   SETTINGS_ITEM(Color,              WallOutlineColor,         "Testing",        "WallOutlineColor",         Colors::DefaultWallOutlineColor, NULL,     NULL,     "Color used locally for rendering wall outlines (r g b), (values between 0 and 1), or #hexcolor")                               \
   SETTINGS_ITEM(GoalZoneFlashStyle, GoalZoneFlashStyle,       "Testing",        "GoalZoneFlashStyle",       GoalZoneFlashOriginal,           NULL,     NULL,     "Different flash patterns when goal is captured in ZC game: Original, Experimental, None")                                      \
   SETTINGS_ITEM(U16,                ClientPortNumber,         "Testing",        "ClientPortNumber",         0,                               NULL,     NULL,     "Only helps when punching through firewall when using router's port forwarded for client port number")                          \
   SETTINGS_ITEM(YesNo,              DisableScreenSaver,       "Testing",        "DisableScreenSaver",       Yes,                             NULL,     NULL,     "Disable ScreenSaver from having no input from keyboard/mouse, useful when using joystick")                                     \
                                                                                                                                                                                                                                                                                                  \
   SETTINGS_ITEM(string,             ServerName,               "Host",           "ServerName",              "Bitfighter host",                NULL,     NULL,     "The name others will see when they are browsing for servers (max 20 chars)")                                                   \
   SETTINGS_ITEM(string,             ServerAddress,            "Host",           "ServerAddress",            MASTER_SERVER_LIST_ADDRESS,      NULL,     NULL,     "Socket address and port to bind to, e.g. IP:Any:9876 or IP:54.35.110.99:8000 or IP:bitfighter.org:8888\n"                      \
                                                                                                                                                                  "(leave blank to let the system decide; this is almost always what you want)")                                                  \
   SETTINGS_ITEM(string,             ServerDescription,        "Host",           "ServerDescription",        "",                              NULL,     NULL,     "A one line description of your server.  Please include nickname and physical location!")                                       \
   SETTINGS_ITEM(string,             ServerPassword,           "Host",           "ServerPassword",           "",                              NULL,     NULL,     "You can require players to use a password to play on your server.  Leave blank to grant access to all.")                       \
   SETTINGS_ITEM(string,             OwnerPassword,            "Host",           "OwnerPassword",            "",                              NULL,     NULL,     "Super admin password.  Gives admin rights + power over admins.  Do not give this out!")                                        \
   SETTINGS_ITEM(string,             AdminPassword,            "Host",           "AdminPassword",            "",                              NULL,     NULL,     "Use this password to manage players & change levels on your server.")                                                          \
   SETTINGS_ITEM(string,             LevelChangePassword,      "Host",           "LevelChangePassword",      "",                              NULL,     NULL,     "Use this password to change levels on your server.  Leave blank to grant access to all.")                                      \
   SETTINGS_ITEM(string,             LevelDir,                 "Host",           "LevelDir",                 "",                              NULL,     NULL,     "Specify where level files are stored; can be overridden on command line with -leveldir param.")                                \
   SETTINGS_ITEM(U32,                MaxPlayers,               "Host",           "MaxPlayers",               127,                             NULL,     NULL,     "The max number of players that can play on your server.")                                                                      \
   SETTINGS_ITEM(S32,                MaxBots,                  "Host",           "MaxBots",                  10,                              NULL,     NULL,     "The max number of bots allowed on this server.")                                                                               \
   SETTINGS_ITEM(YesNo,              AddRobots,                "Host",           "AddRobots",                No,                              NULL,     NULL,     "Add robot players to this server.")                                                                                            \
   SETTINGS_ITEM(S32,                MinBalancedPlayers,       "Host",           "MinBalancedPlayers",       6,                               NULL,     NULL,     "The minimum number of players ensured in each map.  Bots will be added up to this number.")                                    \
   SETTINGS_ITEM(YesNo,              EnableServerVoiceChat,    "Host",           "EnableServerVoiceChat",    Yes,                             NULL,     NULL,     "If false, prevents any voice chat in a server.")                                                                               \
   SETTINGS_ITEM(YesNo,              AllowGetMap,              "Host",           "AllowGetMap",              No,                              NULL,     NULL,     "When getmap is allowed, anyone can download the current level using the /getmap command.")                                     \
   SETTINGS_ITEM(YesNo,              AllowDataConnections,     "Host",           "AllowDataConnections",     No,                              NULL,     NULL,     "When data connections are allowed, anyone with the admin password can upload or download levels, bots, or levelGen scripts.\n" \
                                                                                                                                                                  "This feature is probably insecure, and should be DISABLED unless you require the functionality.")                              \
   SETTINGS_ITEM(YesNo,              LogStats,                 "Host",           "LogStats",                 No,                              NULL,     NULL,     "Save game stats locally to built-in sqlite database (saves the same stats as are sent to the master)")                         \
   SETTINGS_ITEM(F32,                AlertsVolume,             "Host",           "AlertsVolume",             10,                              checkVol, writeVol, "Volume of audio alerts on dedicated server when players join or leave game from 0 (mute) to 10 (full bore).")                  \
   SETTINGS_ITEM(YesNo,              RandomLevels,             "Host",           "RandomLevels",             No,                              NULL,     NULL,     "When current level ends, this can enable randomly switching to any available levels.")                                         \
   SETTINGS_ITEM(YesNo,              SkipUploads,              "Host",           "SkipUploads",              No,                              NULL,     NULL,     "When current level ends, enables skipping all uploaded levels.")                                                               \
   SETTINGS_ITEM(YesNo,              AllowMapUpload,           "Host",           "AllowMapUpload",           No,                              NULL,     NULL,     "Allow users to upload maps")                                                                                                   \
   SETTINGS_ITEM(YesNo,              AllowAdminMapUpload,      "Host",           "AllowAdminMapUpload",      Yes,                             NULL,     NULL,     "Allow admins to upload maps")                                                                                                  \
   SETTINGS_ITEM(YesNo,              AllowLevelgenUpload,      "Host",           "AllowLevelgenUpload",      Yes,                             NULL,     NULL,     "Allow users to upload levelgens.  Note that while levelgens run in a sandbox that is mostly secure,\n"                         \
                                                                                                                                                                  "there may be unknown security risks with this setting.  Use with care.")                                                       \
   SETTINGS_ITEM(YesNo,              AllowTeamChanging,        "Host",           "AllowTeamChanging",        Yes,                             NULL,     NULL,     "Allow players to change teams.  You should generally allow this unless there is a good reason to disable it.")                 \
   SETTINGS_ITEM(string,             DefaultRobotScript,       "Host",           "DefaultRobotScript",       "s_bot.bot",                     NULL,     NULL,     "If user adds a robot, this script will be used if one is not specified")                                                       \
   SETTINGS_ITEM(string,             GlobalLevelScript,        "Host",           "GlobalLevelScript",        "",                              NULL,     NULL,     "Specify a levelgen that will get run on every level")                                                                          \
   SETTINGS_ITEM(YesNo,              GameRecording,            "Host",           "GameRecording",            No,                              NULL,     NULL,     "If Yes, games will be recorded; if No, they will not.  This is typically set via the menu.")                                   \
   SETTINGS_ITEM(U32,                MaxFpsServer,             "Host",           "MaxFPS",                   100,                             NULL,     NULL,     "Maximum FPS the dedicated server will run at.  Higher values use more CPU (and power), lower may increase lag.\n"              \
                                                                                                                                                                  "Specify 0 for no limit. Negative values will not make Bitfighter run backwards.  Sorry.  (default = 100)")                     \
   MYSQL_SETTINGS_TABLE_ENTRY                                                                                                                                                                                                                                                                     \
                                                                                                                                                                                                                                                                                                  \
   SETTINGS_ITEM(YesNo,              VotingEnabled,            "Host-Voting",    "VoteEnable",               No,                              NULL,     NULL,     "Enable voting on this server")                                                                                                 \
   SETTINGS_ITEM(U32,                VoteLength,               "Host-Voting",    "VoteLength",               12,                              NULL,     NULL,     "Time voting will last for general items, in seconds (use 0 to disable)")                                                       \
   SETTINGS_ITEM(U32,                VoteLengthToChangeTeam,   "Host-Voting",    "VoteLengthToChangeTeam",   10,                              NULL,     NULL,     "Time voting will last for team changing, in seconds (use 0 to disable)")                                                       \
   SETTINGS_ITEM(U32,                VoteRetryLength,          "Host-Voting",    "VoteRetryLength",          30,                              NULL,     NULL,     "When a vote fails, the vote caller is unable to vote until this many seconds has elapsed.")                                    \
   SETTINGS_ITEM(S32,                VoteYesStrength,          "Host-Voting",    "VoteYesStrength",           3,                              NULL,     NULL,     "How much does a Yes vote count?")                                                                                              \
   SETTINGS_ITEM(S32,                VoteNoStrength,           "Host-Voting",    "VoteNoStrength",           -3,                              NULL,     NULL,     "How much does a No vote count?")                                                                                               \
   SETTINGS_ITEM(S32,                VoteNothingStrength,      "Host-Voting",    "VoteNothingStrength",      -1,                              NULL,     NULL,     "How much does an abstention count?")                                                                                           \
                                                                                                                                                                                                                                                                                                  \
   SETTINGS_ITEM(YesNo,              UseUpdater,               "Updater",        "UseUpdater",                Yes,                            NULL,     NULL,     "Automatically upgrade Bitfighter when a new version is available (Yes/No, Windows only)")                                      \
                                                                                                                                                                                                                                                                                                  \
   SETTINGS_ITEM(YesNo,              DumpKeys,                 "Diagnostics",    "DumpKeys",                  No,                             NULL,     NULL,     "Enable this to dump raw input to the screen (Yes/No)")                                                                         \
   SETTINGS_ITEM(YesNo,              LogConnectionProtocol,    "Diagnostics",    "LogConnectionProtocol",     No,                             NULL,     NULL,     "Log ConnectionProtocol events (Yes/No)")                                                                                       \
   SETTINGS_ITEM(YesNo,              LogNetConnection,         "Diagnostics",    "LogNetConnection",          No,                             NULL,     NULL,     "Log NetConnectionEvents (Yes/No)")                                                                                             \
   SETTINGS_ITEM(YesNo,              LogEventConnection,       "Diagnostics",    "LogEventConnection",        No,                             NULL,     NULL,     "Log EventConnection events (Yes/No)")                                                                                          \
   SETTINGS_ITEM(YesNo,              LogGhostConnection,       "Diagnostics",    "LogGhostConnection",        No,                             NULL,     NULL,     "Log GhostConnection events (Yes/No)")                                                                                          \
   SETTINGS_ITEM(YesNo,              LogNetInterface,          "Diagnostics",    "LogNetInterface",           No,                             NULL,     NULL,     "Log NetInterface events (Yes/No)")                                                                                             \
   SETTINGS_ITEM(YesNo,              LogPlatform,              "Diagnostics",    "LogPlatform",               No,                             NULL,     NULL,     "Log Platform events (Yes/No)")                                                                                                 \
   SETTINGS_ITEM(YesNo,              LogNetBase,               "Diagnostics",    "LogNetBase",                No,                             NULL,     NULL,     "Log NetBase events (Yes/No)")                                                                                                  \
   SETTINGS_ITEM(YesNo,              LogUDP,                   "Diagnostics",    "LogUDP",                    No,                             NULL,     NULL,     "Log UDP events (Yes/No)")                                                                                                      \
   SETTINGS_ITEM(YesNo,              LogFatalError,            "Diagnostics",    "LogFatalError",             Yes,                            NULL,     NULL,     "Log fatal errors; should be left on (Yes/No)")                                                                                 \
   SETTINGS_ITEM(YesNo,              LogError,                 "Diagnostics",    "LogError",                  Yes,                            NULL,     NULL,     "Log serious errors; should be left on (Yes/No)")                                                                               \
   SETTINGS_ITEM(YesNo,              LogWarning,               "Diagnostics",    "LogWarning",                Yes,                            NULL,     NULL,     "Log less serious errors (Yes/No)")                                                                                             \
   SETTINGS_ITEM(YesNo,              LogConfigurationError,    "Diagnostics",    "LogConfigurationError",     Yes,                            NULL,     NULL,     "Log problems with configuration (Yes/No)")                                                                                     \
   SETTINGS_ITEM(YesNo,              LogConnection,            "Diagnostics",    "LogConnection",             Yes,                            NULL,     NULL,     "High level logging connections with remote machines (Yes/No)")                                                                 \
   SETTINGS_ITEM(YesNo,              LogLevelLoaded,           "Diagnostics",    "LogLevelLoaded",            Yes,                            NULL,     NULL,     "Write a log entry when a level is loaded (Yes/No)")                                                                            \
   SETTINGS_ITEM(YesNo,              LogLevelError,            "Diagnostics",    "LogLevelError",             Yes,                            NULL,     NULL,     "Log errors and warnings about levels loaded (Yes/No)")                                                                         \
   SETTINGS_ITEM(YesNo,              LogLuaObjectLifecycle,    "Diagnostics",    "LogLuaObjectLifecycle",     No,                             NULL,     NULL,     "Creation and destruction of Lua objects (Yes/No)")                                                                             \
   SETTINGS_ITEM(YesNo,              LuaLevelGenerator,        "Diagnostics",    "LuaLevelGenerator",         Yes,                            NULL,     NULL,     "Messages from the LuaLevelGenerator (Yes/No)")                                                                                 \
   SETTINGS_ITEM(YesNo,              LuaBotMessage,            "Diagnostics",    "LuaBotMessage",             Yes,                            NULL,     NULL,     "Message from a bot (Yes/No)")                                                                                                  \
   SETTINGS_ITEM(YesNo,              ServerFilter,             "Diagnostics",    "ServerFilter",              No,                             NULL,     NULL,     "For logging messages specific to hosting games (Yes/No)\n"                                                                     \
                                                                                                                                                                  "(Note: These messages will go to bitfighter_server.log regardless of this setting)")                                           \
                                                                                                                                                                                                                                                                                             

namespace Zap
{

namespace IniKey
{
enum SettingsItem {
#define SETTINGS_ITEM(a, enumVal, c, d, e, f, g, h) enumVal,
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
