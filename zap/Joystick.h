//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifdef ZAP_DEDICATED
#  error "No joystick.h for dedicated build"
#endif

#ifndef JOYSTICK_H_
#define JOYSTICK_H_

#include "JoystickButtonEnum.h"
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
      ButtonInfo();

      JoystickButton button;
      U8 sdlButton;
      U8 rawAxis;
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

      ButtonInfo buttonMappings[JoystickButtonCount];
   };

   Joystick();
   virtual ~Joystick();

   static const U8 FakeRawButton = 254;  // A button that can't possibly be real (must fit within U8)

   static U32 ButtonMask;    // Holds what buttons are current pressed down - can support up to 32
   static S16 axesValues[SDL_CONTROLLER_AXIS_MAX];  // Current state of all controller axes

   // static data
   static S16 LowerSensitivityThreshold;
   static S16 UpperSensitivityThreshold;
   static S32 UseJoystickNumber;
   static Vector<JoystickInfo> JoystickPresetList;
   static U32 SelectedPresetIndex;
   static U32 AxesInputCodeMask;

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
   static JoystickButton stringToJoystickButton(const string &buttonString);
   static ButtonShape buttonLabelToButtonShape(const string &label);
   static ButtonSymbol stringToButtonSymbol(const string &label);
   static Color stringToColor(const string &colorString);
   static U32 getJoystickIndex(const string &joystickIndex);
   static JoystickInfo *getJoystickInfo(const string &joystickIndex);

   static bool isButtonDefined(S32 presetIndex, S32 buttonIndex);
};

} /* namespace Zap */
#endif /* JOYSTICK_H_ */
