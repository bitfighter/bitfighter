//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _INPUTCODE_H_
#define _INPUTCODE_H_

#include "InputModeEnum.h"
#include "InputCodeEnum.h"
#include "JoystickButtonEnum.h"

#include "tnlVector.h"
#include <string>

// Until we move completely to SDL2
typedef S32 SDL_Keycode;

using namespace std;

namespace Zap
{



////////////////////////////////////////
////////////////////////////////////////

struct BindingSet;

class InputCodeManager 
{
private:
   bool mBindingsHaveKeypadEntry;
   InputMode mInputMode;             // Joystick or Keyboard

   BindingSet *mCurrentBindingSet;     
   Vector<BindingSet> mBindingSets;

public:
   enum JoystickJoysticks {
      JOYSTICK_DPAD,
      JOYSTICK_STICK_1,
      JOYSTICK_STICK_2
   };

// Note that the BindingSet member name referenced below doesn't actually appear anywhere else... it could be any aribtrary and unique token

/*                                                            Saved    BindingSet        Def. kb           Def. js           */
/*            Enum                      Name in INI           in INI   member name       binding           binding           */
#define BINDING_TABLE \
   BINDING( BINDING_SELWEAP1,           "SelWeapon1",          true,  inputSELWEAP1,     KEY_1,            KEY_1            ) \
   BINDING( BINDING_SELWEAP2,           "SelWeapon2",          true,  inputSELWEAP2,     KEY_2,            KEY_2            ) \
   BINDING( BINDING_SELWEAP3,           "SelWeapon3",          true,  inputSELWEAP3,     KEY_3,            KEY_3            ) \
   BINDING( BINDING_ADVWEAP,            "SelNextWeapon",       true,  inputADVWEAP,      KEY_E,            BUTTON_1         ) \
   BINDING( BINDING_ADVWEAP2,           "SelNextWeapon2",      true,  inputADVWEAP2,     MOUSE_WHEEL_UP,   MOUSE_WHEEL_UP   ) \
   BINDING( BINDING_PREVWEAP,           "SelPrevWeapon",       true,  inputPREVWEAP,     MOUSE_WHEEL_DOWN, MOUSE_WHEEL_DOWN ) \
   BINDING( BINDING_CMDRMAP,            "ShowCmdrMap",         true,  inputCMDRMAP,      KEY_C,            BUTTON_2         ) \
   BINDING( BINDING_TEAMCHAT,           "TeamChat",            true,  inputTEAMCHAT,     KEY_T,            KEY_T            ) \
   BINDING( BINDING_GLOBCHAT,           "GlobalChat",          true,  inputGLOBCHAT,     KEY_G,            KEY_G            ) \
   BINDING( BINDING_QUICKCHAT,          "QuickChat",           true,  inputQUICKCHAT,    KEY_V,            BUTTON_3         ) \
   BINDING( BINDING_CMDCHAT,            "Command",             true,  inputCMDCHAT,      KEY_SLASH,        KEY_SLASH        ) \
   BINDING( BINDING_LOADOUT,            "ShowLoadoutMenu",     true,  inputLOADOUT,      KEY_Z,            BUTTON_4         ) \
   BINDING( BINDING_MOD1,               "ActivateModule1",     true,  inputMOD1,         KEY_SPACE,        BUTTON_7         ) \
   BINDING( BINDING_MOD2,               "ActivateModule2",     true,  inputMOD2,         MOUSE_RIGHT,      BUTTON_6         ) \
   BINDING( BINDING_FIRE,               "Fire",                true,  inputFIRE,         MOUSE_LEFT,       MOUSE_LEFT       ) \
   BINDING( BINDING_DROPITEM,           "DropItem",            true,  inputDROPITEM,     KEY_B,            KEY_B            ) \
   BINDING( BINDING_TOGVOICE,           "VoiceChat",           true,  inputTOGVOICE,     KEY_R,            KEY_R            ) \
   BINDING( BINDING_UP,                 "ShipUp",              true,  inputUP,           KEY_W,            KEY_UP           ) \
   BINDING( BINDING_DOWN,               "ShipDown",            true,  inputDOWN,         KEY_S,            KEY_DOWN         ) \
   BINDING( BINDING_LEFT,               "ShipLeft",            true,  inputLEFT,         KEY_A,            KEY_LEFT         ) \
   BINDING( BINDING_RIGHT,              "ShipRight",           true,  inputRIGHT,        KEY_D,            KEY_RIGHT        ) \
   BINDING( BINDING_SCRBRD,             "ShowScoreboard",      true,  inputSCRBRD,       KEY_TAB,          BUTTON_5         ) \
   BINDING( BINDING_MISSION,            "Mission",             true,  inputMISSION,      KEY_F2,           KEY_F2           ) \
   BINDING( BINDING_TOGGLE_RATING,      "ToggleRating",        true,  inputTOGGLERATING, KEY_EQUALS,       KEY_EQUALS       ) \
   BINDING( BINDING_HELP,               "Help",                false, keyHELP,           KEY_F1,           KEY_F1           ) \
   BINDING( BINDING_OUTGAMECHAT,        "OutOfGameChat",       false, keyOUTGAMECHAT,    KEY_F5,           KEY_F5           ) \
   BINDING( BINDING_FPS,                "FPS",                 false, keyFPS,            KEY_F6,           KEY_F6           ) \
   BINDING( BINDING_DIAG,               "Diagnostics",         false, keyDIAG,           KEY_F7,           KEY_F7           ) \
   BINDING( BINDING_LOAD_PRESET_1,      "LoadLoadoutPreset1",  false, keyLoadPreset1,    KEY_ALT_1,        KEY_ALT_1        ) \
   BINDING( BINDING_LOAD_PRESET_2,      "LoadLoadoutPreset2",  false, keyLoadPreset2,    KEY_ALT_2,        KEY_ALT_2        ) \
   BINDING( BINDING_LOAD_PRESET_3,      "LoadLoadoutPreset3",  false, keyLoadPreset3,    KEY_ALT_3,        KEY_ALT_3        ) \
   BINDING( BINDING_SAVE_PRESET_1,      "SaveLoadoutPreset1",  false, keySavePreset1,    KEY_CTRL_1,       KEY_CTRL_1       ) \
   BINDING( BINDING_SAVE_PRESET_2,      "SaveLoadoutPreset2",  false, keySavePreset2,    KEY_CTRL_2,       KEY_CTRL_2       ) \
   BINDING( BINDING_SAVE_PRESET_3,      "SaveLoadoutPreset3",  false, keySavePreset3,    KEY_CTRL_3,       KEY_CTRL_3       ) \
                                                                                                                              \
   /* Editor specific */                                                                                                      \
                                                                                                                              \
   BINDING( BINDING_TEAM_EDITOR,        "TeamEditor",          false, keyTeamEditor,      KEY_F2,           KEY_F2          ) \
   BINDING( BINDING_GAME_PARAMS_EDITOR, "GameParameterEditor", false, keyGameParamEditor, KEY_F3,           KEY_F3          ) \

enum BindingNameEnum {
#define BINDING(enumName, b, c, d, e, f) enumName,
    BINDING_TABLE
#undef BINDING
    BINDING_DEFINEABLE_KEY_COUNT
};

   InputCodeManager();     // Constructor
   virtual ~InputCodeManager();

   static const char *inputCodeToString(InputCode inputCode);
   static const char *inputCodeToPrintableChar(InputCode inputCode);

   static InputCode stringToInputCode(const char *inputName);

   static void setState(InputCode inputCode, bool state);   // Set key state (t=down, f=up)
   static bool getState(InputCode inputCode);               // Return current key state (t=down, f=up)
   static void resetStates();                               // Initialize key states
   static void dumpInputCodeStates();                       // Log key states for testing
   static void initializeKeyNames();

   //static S32 getBindingCount();
   static string getBindingName(BindingNameEnum binding);
   
   InputCode getKeyBoundToBindingCodeName(const string &name) const;

   // Some converters
   InputCode filterInputCode(InputCode inputCode);    // Calls filters below

#ifndef ZAP_DEDICATED
   static InputCode joyHatToInputCode(int hatDirectionMask);
   static InputCode joystickButtonToInputCode(JoystickButton button);
#endif

   static InputCode convertJoystickToKeyboard(InputCode inputCode);
private:
   static InputCode convertNumPadToNum(InputCode inputCode);
   bool checkIfBindingsHaveKeypad();

public:
   static string getCurrentInputString(InputCode inputCode);

   static bool checkModifier(InputCode mod1);            
   static bool checkModifier(InputCode mod1, InputCode mod2);            
   static bool checkModifier(InputCode mod1, InputCode mod2, InputCode mod3);            

   void setInputMode(InputMode inputMode);
   InputMode getInputMode()    const;
   string getInputModeString() const;  // Returns display-friendly mode designator like "Keyboard" or "Joystick 1"

   #ifndef ZAP_DEDICATED
      static InputCode sdlKeyToInputCode(SDL_Keycode key);        // Convert SDL keys to InputCode
      static SDL_Keycode inputCodeToSDLKey(InputCode inputCode);  // Take a InputCode and return the SDL equivalent
   #endif

   static char keyToAscii(int unicode, InputCode inputCode);   // Return a printable ascii char, if possible
   static bool isControllerButton(InputCode inputCode);        // Does inputCode represent a controller button?
   static bool isKeypadKey(InputCode inputCode);               // Is inputCode on the numeric keypad?
   static bool isMouseAction(InputCode inputCode);             // Is inputCode related to the mouse?
   static bool isKeyboardKey(InputCode inputCode);             // Is inputCode a key on the keyboard?
   static bool isCtrlKey(InputCode inputCode);                 // Is inputCode modified with ctrl (e.g. KEY_CTRL_M)?
   static bool isAltKey(InputCode inputCode);                  // Is inputCode modified with alt (e.g. KEY_ALT_1)?
   static bool isModifier(InputCode inputCode);                // Is inputCode a modifier key?
   static bool isModified(InputCode inputCode);                // Does inputCode have a modifier attached to it?

   // For dealing with special cases, such as Ctrl-M
   static InputCode getModifier(InputCode inputCode);
   static string getModifierString(InputCode inputCode);

   static const Vector<string> *getModifierNames();

   static InputCode getBaseKey(InputCode inputCode);
   static string getBaseKeyString(InputCode inputCode);

   static JoystickButton inputCodeToJoystickButton(InputCode inputCode);


   InputCode getBinding(BindingNameEnum bindingName) const; 
   InputCode getBinding(BindingNameEnum bindingName, InputMode inputMode) const;
   void setBinding(BindingNameEnum bindingName, InputCode key);
   void setBinding(BindingNameEnum bindingName, InputMode inputMode, InputCode key);
};

////////////////////////////////////////
////////////////////////////////////////

struct BindingSet
{
   BindingSet();     // Constructor
   virtual ~BindingSet();

   bool hasKeypad();

   InputCode getBinding(InputCodeManager::BindingNameEnum bindingName) const;
   void setBinding(InputCodeManager::BindingNameEnum bindingName, InputCode key);

   // Create a sequence of member variables from memberName column of the BINDING_TABLE above
#define BINDING(a, b, c, memberName, e, f) InputCode memberName;
    BINDING_TABLE
#undef BINDING
};



};     // namespace Zap

#endif

