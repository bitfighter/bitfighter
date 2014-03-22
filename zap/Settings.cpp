//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Settings.h"

using namespace TNL;

namespace Zap
{

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



}

