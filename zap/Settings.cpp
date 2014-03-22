//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Settings.h"

using namespace TNL;

namespace Zap
{

// Convert a string value to our sfxSets enum
inline string displayModeToString(DisplayMode mode)
{
   if(mode == DISPLAY_MODE_FULL_SCREEN_STRETCHED)   return "Fullscreen-Stretch";
   if(mode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED) return "Fullscreen";
   return "Window";
}


inline string colorEntryModeToString(ColorEntryMode colorEntryMode)
{
   if(colorEntryMode == ColorEntryModeHex) return "RGBHEX";
   if(colorEntryMode == ColorEntryMode255) return "RGB255";
   return "RGB100";
}


inline string goalZoneFlashStyleToString(GoalZoneFlashStyle flashStyle)
{
   if(flashStyle == GoalZoneFlashExperimental) return "Experimental";
   if(flashStyle == GoalZoneFlashNone)         return "None";
   return "Original";
}


// Convert various things to strings
string Evaluator::toString(const string &val)             { return val;                                          }
string Evaluator::toString(S32 val)                       { return itos(val);                                    }
string Evaluator::toString(YesNo yesNo)                   { return yesNo  == Yes      ? "Yes" :      "No";       }
string Evaluator::toString(RelAbs relAbs)                 { return relAbs == Relative ? "Relative" : "Absolute"; }
string Evaluator::toString(DisplayMode displayMode)       { return displayModeToString(displayMode);             }
string Evaluator::toString(ColorEntryMode colorMode)      { return colorEntryModeToString(colorMode);            }
string Evaluator::toString(GoalZoneFlashStyle flashStyle) { return goalZoneFlashStyleToString(flashStyle);       }
string Evaluator::toString(const Color &color)            { return color.toHexStringForIni();                    }


// Convert a string value to a DisplayMode enum value
DisplayMode Evaluator::stringToDisplayMode(string mode) 
{
   if(lcase(mode) == "fullscreen-stretch")
      return DISPLAY_MODE_FULL_SCREEN_STRETCHED;
   else if(lcase(mode) == "fullscreen")
      return DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED;
   else
      return DISPLAY_MODE_WINDOWED;
}


// Convert a string value to a DisplayMode enum value
ColorEntryMode Evaluator::stringToColorEntryMode(string mode)
{
   if(lcase(mode) == "rgbhex")
      return ColorEntryModeHex;
   else if(lcase(mode) == "rgb255")
      return ColorEntryMode255;
   else
      return ColorEntryMode100;     // <== default
}


// Convert a string value to a DisplayMode enum value
GoalZoneFlashStyle Evaluator::stringToGoalZoneFlashStyle(string style)
{
   if(lcase(style) == "none")
      return GoalZoneFlashNone;
   else if(lcase(style) == "experimental")
      return GoalZoneFlashExperimental;
   else
      return GoalZoneFlashOriginal;     // <== default
}


YesNo Evaluator::stringToYesNo(string yesNo)
{
   return lcase(yesNo) == "yes" ? Yes : No;
}


RelAbs Evaluator::stringToRelAbs(string relAbs)
{
   return lcase(relAbs) == "relative" ? Relative : Absolute;
}


// Templated default - needs to be overriden
template<class DataType> DataType
Evaluator::fromString(const string &val) { TNLAssert(false, "Specialize me!"); return DataType(); }

// Specializations.
// NOTE: All template specializations must be declared in the namespace scope to be
// C++ compliant.  Shame on Visual Studio!
template<> string             Evaluator::fromString(const string &val) { return val;                             }
template<> S32                Evaluator::fromString(const string &val) { return atoi(val.c_str());               }
template<> U32                Evaluator::fromString(const string &val) { return atoi(val.c_str());               }
template<> U16                Evaluator::fromString(const string &val) { return atoi(val.c_str());               }
template<> DisplayMode        Evaluator::fromString(const string &val) { return stringToDisplayMode(val);        }
template<> YesNo              Evaluator::fromString(const string &val) { return stringToYesNo(val);              }
template<> RelAbs             Evaluator::fromString(const string &val) { return stringToRelAbs(val);             }
template<> ColorEntryMode     Evaluator::fromString(const string &val) { return stringToColorEntryMode(val);     }
template<> GoalZoneFlashStyle Evaluator::fromString(const string &val) { return stringToGoalZoneFlashStyle(val); }
template<> Color              Evaluator::fromString(const string &val) { return Color::iniValToColor(val);       }



}

