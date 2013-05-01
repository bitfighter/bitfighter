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


#ifdef ZAP_DEDICATED
#  error "No joystick.h for dedicated build"
#endif

#ifndef JOYSTICK_H_
#define JOYSTICK_H_

#include "tnlTypes.h"
#include "tnlVector.h"
#include "Color.h"

#include "SDL_joystick.h"

#include <string>

using namespace TNL;
using namespace std;

namespace Zap {

#include "InputCodeEnum.h"    // Include inside Zap namespace

class GameSettings;

enum JoystickAxesMask {
   MoveAxesLeftMask = BIT(0),
   MoveAxesRightMask = BIT(1),
   MoveAxesUpMask = BIT(2),
   MoveAxesDownMask = BIT(3),
   ShootAxesLeftMask = BIT(4),
   ShootAxesRightMask = BIT(5),
   ShootAxesUpMask = BIT(6),
   ShootAxesDownMask = BIT(7),

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

enum JoystickHatDirections {
   HatUp,
   HatRight,
   HatDown,
   HatLeft,
   MaxHatDirections
};


struct JoystickInput {
   U32 axesDirection;
   U32 axesMask;
   InputCode inputCode;
   F32 value;
};


class Joystick {
private:
   static SDL_Joystick *sdlJoystick;       // The current Joystick in use

public:
   // This enum is for the in-game button type.  SDL raw button inputs will map to one of these
   enum Button {
      Button1,
      Button2,
      Button3,
      Button4,
      Button5,
      Button6,
      Button7,
      Button8,
      Button9,
      Button10,
      Button11,
      Button12,
      ButtonStart,
      ButtonBack,
      ButtonDPadUp,
      ButtonDPadDown,
      ButtonDPadLeft,
      ButtonDPadRight,
      MaxJoystickButtons,
      ButtonUnknown,
   };

   enum ButtonShape {
      ButtonShapeRound,
      ButtonShapeRect,
      ButtonShapeSmallRect,
      ButtonShapeRoundedRect,
      ButtonShapeSmallRoundedRect,
      ButtonShapeHorizEllipse,
      ButtonShapeRightTriangle,
   };


   enum ButtonSymbol {
      ButtonSymbolNone,
      ButtonSymbolPsCircle,
      ButtonSymbolPsCross,
      ButtonSymbolPsSquare,
      ButtonSymbolPsTriangle,
      ButtonSymbolSmallRightTriangle,
      ButtonSymbolSmallLeftTriangle,
   };


   struct ButtonInfo {
      Joystick::Button button;
      U8 sdlButton;
      string label;
      Color color;
      Joystick::ButtonShape buttonShape;
      Joystick::ButtonSymbol buttonSymbol;
   };


   // Struct to hold joystick information once it has been detected
   struct JoystickInfo {
      string identifier;             // Primary joystick identifier; used in bitfighter.ini; used as section name
      string name;                   // Pretty name to show in-game
      string searchString;           // Name that SDL detects when joystick is connected
      bool isSearchStringSubstring;  // If the search string is a substring pattern to look for
      U32 moveAxesSdlIndex[2];       // primary axes; 0 -> left/right, 1 -> up/down
      U32 shootAxesSdlIndex[2];      // secondary axes; could be anything; first -> left/right, second -> up/down

      ButtonInfo buttonMappings[MaxJoystickButtons];
   };

   Joystick();
   virtual ~Joystick();

   static const S32 rawAxisCount = 32;   // Maximum raw axis to detect
   static const U32 MaxSdlButtons = 32;  // Maximum raw buttons to detect
   static const U8 FakeRawButton = 254;  // A button that can't possible be real (must fit within U8)

   static U32 ButtonMask;    // Holds what buttons are current pressed down - can support up to 32
   static F32 rawAxis[rawAxisCount];
   static Vector<string> DetectedJoystickNameList;   // All detected joystick names

   // static data
   static S16 LowerSensitivityThreshold;
   static S16 UpperSensitivityThreshold;
   static S32 UseJoystickNumber;
   static Vector<JoystickInfo> JoystickPresetList;
   static U32 SelectedPresetIndex;
   static JoystickInput JoystickInputData[MaxAxesDirections];
   static U32 AxesInputCodeMask;
   static U32 HatInputCodeMask;

   static bool initJoystick(GameSettings *settings);
   static bool enableJoystick(GameSettings *settings, bool hasBeenOpenedBefore);
   static void shutdownJoystick();

   static void loadJoystickPresets(GameSettings *settings);
   static string autodetectJoystick(GameSettings *settings);
   static S32 checkJoystickString_exact_match(const string &controllerName);     // Searches for an exact match for controllerName
   static S32 checkJoystickString_partial_match(const string &controllerName);   // Searches for an exact match for controllerName
   static JoystickInfo getGenericJoystickInfo();
   static void setSelectedPresetIndex(U32 joystickIndex);

   static void getAllJoystickPrettyNames(Vector<string> &nameList);
   static Button stringToJoystickButton(const string &buttonString);
   static ButtonShape buttonLabelToButtonShape(const string &label);
   static ButtonSymbol stringToButtonSymbol(const string &label);
   static Color stringToColor(const string &colorString);
   static U32 getJoystickIndex(const string &joystickIndex);
   static JoystickInfo *getJoystickInfo(const string &joystickIndex);

   static Button remapSdlButtonToJoystickButton(U8 button);
};

} /* namespace Zap */
#endif /* JOYSTICK_H_ */
