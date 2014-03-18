//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CONFIG_ENUM_H_
#define _CONFIG_ENUM_H_


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
   DISPLAY_MODE_UNKNOWN    // <== Note: code depends on this being the first value that's not a real mode
};


enum YesNo {
   No,      // ==> 0 ==> false
   Yes      // ==> 1 ==> true
};


enum ColorEntryMode {
   ColorEntryMode100,
   ColorEntryMode255,
   ColorEntryModeHex,
   ColorEntryModeCount
};


enum GoalZoneFlashStyle {
   GoalZoneFlashOriginal,
   GoalZoneFlashExperimental,
   GoalZoneFlashNone
};


enum RelAbs{
   Relative,
   Absolute
};



};

#endif /* _CONFIG_ENUM_H_ */
