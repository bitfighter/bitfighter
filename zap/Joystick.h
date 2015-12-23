//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifdef ZAP_DEDICATED
#  error "No joystick.h for dedicated build"
#endif

#ifndef JOYSTICK_H_
#define JOYSTICK_H_

#include "InputCodeEnum.h"

#include "Color.h"
#include "tnlTypes.h"
#include "tnlVector.h"

#include "SDL_gamecontroller.h"

#include <string>

using namespace TNL;
using namespace std;

// Forward declarations
struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;


namespace Zap {

// Forward declarations
class GameSettings;


class Joystick {
private:
   static SDL_GameController *sdlController;       // The current Joystick in use

public:
   enum ButtonShape {
      ButtonShapeRound,
      ButtonShapeRect,
      ButtonShapeSmallRect,
      ButtonShapeRoundedRect,
      ButtonShapeSmallRoundedRect,
      ButtonShapeHorizEllipse,
      ButtonShapeRightTriangle,
      ButtonShapeDPadUp,
      ButtonShapeDPadDown,
      ButtonShapeDPadLeft,
      ButtonShapeDPadRight,
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


   // Added buttons we want to use in addition to SDL's
   enum ControllerButton {
      ControllerButtonTriggerLeft = SDL_CONTROLLER_BUTTON_MAX,
      ControllerButtonTriggerRight,
      ControllerButtonMax
   };


   struct ButtonInfo {
      string label;
      Color color;
      Joystick::ButtonShape buttonShape;
      Joystick::ButtonSymbol buttonSymbol;
   };

private:
   static ButtonInfo controllerButtonInfos[ControllerButtonMax];

public:
   Joystick();
   virtual ~Joystick();

   static U32 ButtonMask;    // Holds what buttons are current pressed down - can support up to 32

   // Current state of all controller axes; raw values
   static S16 rawAxesValues[SDL_CONTROLLER_AXIS_MAX];

   // static data
   static S16 LowerSensitivityThreshold;
   static S16 UpperSensitivityThreshold;

   static bool initJoystick(GameSettings *settings);
   static bool enableJoystick(GameSettings *settings, bool hasBeenOpenedBefore);
   static void shutdownJoystick();

   static ButtonInfo getButtonInfo(S16 button);
   static ButtonSymbol stringToButtonSymbol(const string &label);
};

} /* namespace Zap */
#endif /* JOYSTICK_H_ */
