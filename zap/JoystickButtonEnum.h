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


enum JoystickAxesDirections {
   JoystickMoveAxesLeft,
   JoystickMoveAxesRight,
   JoystickMoveAxesUp,
   JoystickMoveAxesDown,
   JoystickShootAxesLeft,
   JoystickShootAxesRight,
   JoystickShootAxesUp,
   JoystickShootAxesDown,
   JoystickAxesDirectionCount
};


enum JoystickAxesMask {
   MoveAxesLeftMask   = BIT(0),
   MoveAxesRightMask  = BIT(1),
   MoveAxesUpMask     = BIT(2),
   MoveAxesDownMask   = BIT(3),
   ShootAxesLeftMask  = BIT(4),
   ShootAxesRightMask = BIT(5),
   ShootAxesUpMask    = BIT(6),
   ShootAxesDownMask  = BIT(7),

   MoveAxisLeftRightMask  = MoveAxesLeftMask  | MoveAxesRightMask,
   MoveAxisUpDownMask     = MoveAxesUpMask    | MoveAxesDownMask,
   ShootAxisLeftRightMask = ShootAxesLeftMask | ShootAxesRightMask,
   ShootAxisUpDownMask    = ShootAxesUpMask   | ShootAxesDownMask,

   MoveAxesMask  = MoveAxesLeftMask  | MoveAxesRightMask  | MoveAxesUpMask  | MoveAxesDownMask,
   ShootAxesMask = ShootAxesLeftMask | ShootAxesRightMask | ShootAxesUpMask | ShootAxesDownMask,

   NegativeAxesMask = MoveAxesLeftMask  | MoveAxesUpMask   | ShootAxesLeftMask  | ShootAxesUpMask,
   PositiveAxesMask = MoveAxesRightMask | MoveAxesDownMask | ShootAxesRightMask | ShootAxesDownMask,

   AllAxesMask = MoveAxesMask | ShootAxesMask,
};


struct JoystickStaticDataStruct {
   U32 axesDirection;
   U32 axesMask;
   InputCode inputCode;
};


}

#endif
