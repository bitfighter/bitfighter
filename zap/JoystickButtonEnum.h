//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef JOYSTICK_BUTTON_ENUM_H_
#define JOYSTICK_BUTTON_ENUM_H_

#include "tnlTypes.h" 
#include "InputCodeEnum.h"

using namespace TNL;

namespace Zap 
{


// This enum is for the in-game button type.  SDL raw button inputs will map to one of these
enum JoystickButton {
   JoystickButton1,
   JoystickButton2,
   JoystickButton3,
   JoystickButton4,
   JoystickButton5,
   JoystickButton6,
   JoystickButton7,
   JoystickButton8,
   JoystickButton9,
   JoystickButton10,
   JoystickButton11,
   JoystickButton12,
   JoystickButtonStart,
   JoystickButtonBack,
   JoystickButtonDPadUp,
   JoystickButtonDPadDown,
   JoystickButtonDPadLeft,
   JoystickButtonDPadRight,
   JoystickButtonCount,
   JoystickButtonUnknown,
};


}

#endif
