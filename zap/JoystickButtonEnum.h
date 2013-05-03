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
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef JOYSTICK_BUTTON_ENUM_H_
#define JOYSTICK_BUTTON_ENUM_H_

#include "tnlTypes.h" 

using namespace TNL;

namespace Zap 
{

#include "InputCodeEnum.h"    // Inside Zap namespace

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
