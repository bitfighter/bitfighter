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
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef JOYSTICK_H_
#define JOYSTICK_H_

#ifndef ZAP_DEDICATED
#include "tnlTypes.h"
#include "tnlVector.h"
#include "keyCodeEnum.h"
#include "SDL/SDL_joystick.h"
#endif

using namespace TNL;

namespace Zap {


enum ControllerTypeType
{
   LogitechWingman,
   LogitechDualAction,
   SaitekDualAnalogP880,      // 6 buttons on top, 2 in front
   SaitekDualAnalogRumblePad, // 4 buttons on top, 4 in front
   PS2DualShock,
   PS2DualShockConversionCable,
   PS3DualShock,
   XBoxController,
   XBoxControllerOnXBox,
   ControllerTypeCount,       // Number of predefined controllers
   GenericController,         // Something confirmed as a controller, but of unknown type --> treat as controller
   UnknownController,         // Something not confirmed as controller, or of unknown type --> treat as no controller
   NoController               // Pretty sure there is no controller
};

#ifndef ZAP_DEDICATED
enum ControllerButton {
   ControllerButton1,
   ControllerButton2,
   ControllerButton3,
   ControllerButton4,
   ControllerButton5,
   ControllerButton6,
   ControllerButton7,
   ControllerButton8,
   ControllerButtonStart,
   ControllerButtonBack,
   ControllerButtonDPadUp,
   ControllerButtonDPadDown,
   ControllerButtonDPadLeft,
   ControllerButtonDPadRight,
   MaxControllerButtons
};

enum JoystickAxesMask {
   MoveAxesLeftMask = BIT(0),
   MoveAxesRightMask = BIT(1),
   MoveAxesUpMask = BIT(2),
   MoveAxesDownMask = BIT(3),
   ShootAxesLeftMask = BIT(4),
   ShootAxesRightMask = BIT(5),
   ShootAxesUpMask = BIT(6),
   ShootAxesDownMask = BIT(7),

   MoveAxisLeftRightMask = MoveAxesLeftMask | MoveAxesRightMask,
   MoveAxisUpDownMask = MoveAxesUpMask | MoveAxesDownMask,
   ShootAxisLeftRightMask = ShootAxesLeftMask | ShootAxesRightMask,
   ShootAxisUpDownMask = ShootAxesUpMask | ShootAxesDownMask,

   MoveAxesMask = MoveAxesLeftMask | MoveAxesRightMask | MoveAxesUpMask | MoveAxesDownMask,
   ShootAxesMask = ShootAxesLeftMask | ShootAxesRightMask | ShootAxesUpMask | ShootAxesDownMask,

   NegativeAxesMask = MoveAxesLeftMask | MoveAxesUpMask | ShootAxesLeftMask | ShootAxesUpMask,
   PositiveAxesMask = MoveAxesRightMask | MoveAxesDownMask | ShootAxesRightMask | ShootAxesDownMask,

   AllAxesMask = MoveAxesMask | ShootAxesMask,
};

enum JoystickAxesDirections {
   MoveAxesLeft,
   MoveAxesRight,
   MoveAxesUp,
   MoveAxesDown,
   ShootAxesLeft,
   ShootAxesRight,
   ShootAxesUp,
   ShootAxesDown,
   MaxAxesDirections
};

enum AlignType {
   ALIGN_LEFT,
   ALIGN_CENTER,
   ALIGN_RIGHT
};

struct JoystickInput {
   U32 axesDirection;
   U32 axesMask;
   KeyCode keyCode;
   F32 value;
};

// Struct to hold joystick information once it has been detected
struct JoystickInfo {
   const char *name;
   const char *nameForINI;
   U32 buttonCount;    // how many buttons
   U32 moveAxesSdlIndex[2];    // primary axes; 0 -> left/right, 1 -> up/down
   U32 shootAxesSdlIndex[2];   // secondary axes; could be anything; first -> left/right, second -> up/down
   U32 buttonMappings[MaxControllerButtons];
};

class Joystick {
private:
   static SDL_Joystick *sdlJoystick;       // The current Joystick in use

   static void populatePredefinedJoystickList();

public:
   Joystick();
   virtual ~Joystick();

   static U32 ButtonMask;
   static const S32 rawAxisCount = 32;
   static F32 rawAxis[rawAxisCount];
   static Vector<const char *> DetectedJoystickNameList;   // All detected joystick names

   // static data
   static S16 SensitivityThreshold;
   static S32 UseJoystickNumber;
   static JoystickInfo PredefinedJoystickList[ControllerTypeCount];
   static JoystickInput JoystickInputData[MaxAxesDirections];
   static U32 AxesKeyCodeMask;

   static bool initJoystick();
   static void shutdownJoystick();

   static void populateJoystickStaticData();

   static const char *getJoystickName();
   static ControllerTypeType autodetectJoystickType();

   static ControllerTypeType stringToJoystickType(const char * strJoystick);
   static const char *joystickTypeToString(S32 controllerType);
   static const char *joystickTypeToPrettyString(S32 controllerType);

   static U8 remapJoystickButton(U8 button);
};

#endif // ZAP_DEDICATED
} /* namespace Zap */
#endif /* JOYSTICK_H_ */
