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
