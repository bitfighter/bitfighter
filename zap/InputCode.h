//---------------------------------------------------------------------------------
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

#ifndef _INPUTCODE_H_
#define _INPUTCODE_H_

#include "InputModeEnum.h"
#include "InputCodeEnum.h"
#include "JoystickButtonEnum.h"

#include "tnlVector.h"
#include <string>


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
   TNL::Vector<BindingSet> mBindingSets;

public:
   enum JoystickJoysticks {
      JOYSTICK_DPAD,
      JOYSTICK_STICK_1,
      JOYSTICK_STICK_2
   };

    
/*            Enum                      Name in INI      Save in INI  BindingSet member name   */#define BINDING_TABLE \   BINDING( BINDING_SELWEAP1,           "SelWeapon1",          true,  inputSELWEAP1     ) \
   BINDING( BINDING_SELWEAP2,           "SelWeapon2",          true,  inputSELWEAP2     ) \
   BINDING( BINDING_SELWEAP3,           "SelWeapon3",          true,  inputSELWEAP3     ) \
   BINDING( BINDING_ADVWEAP,            "SelNextWeapon",       true,  inputADVWEAP      ) \
   BINDING( BINDING_ADVWEAP2,           "SelNextWeapon2",      true,  inputADVWEAP2     ) \
   BINDING( BINDING_PREVWEAP,           "SelPrevWeapon",       true,  inputPREVWEAP     ) \
   BINDING( BINDING_CMDRMAP,            "ShowCmdrMap",         true,  inputCMDRMAP      ) \
   BINDING( BINDING_TEAMCHAT,           "TeamChat",            true,  inputTEAMCHAT     ) \
   BINDING( BINDING_GLOBCHAT,           "GlobalChat",          true,  inputGLOBCHAT     ) \
   BINDING( BINDING_QUICKCHAT,          "QuickChat",           true,  inputQUICKCHAT    ) \
   BINDING( BINDING_CMDCHAT,            "Command",             true,  inputCMDCHAT      ) \
   BINDING( BINDING_LOADOUT,            "ShowLoadoutMenu",     true,  inputLOADOUT      ) \
   BINDING( BINDING_MOD1,               "ActivateModule1",     true,  inputMOD1         ) \
   BINDING( BINDING_MOD2,               "ActivateModule2",     true,  inputMOD2         ) \
   BINDING( BINDING_FIRE,               "Fire",                true,  inputFIRE         ) \
   BINDING( BINDING_DROPITEM,           "DropItem",            true,  inputDROPITEM     ) \
   BINDING( BINDING_TOGVOICE,           "VoiceChat",           true,  inputTOGVOICE     ) \
   BINDING( BINDING_UP,                 "ShipUp",              true,  inputUP           ) \
   BINDING( BINDING_DOWN,               "ShipDown",            true,  inputDOWN         ) \
   BINDING( BINDING_LEFT,               "ShipLeft",            true,  inputLEFT         ) \
   BINDING( BINDING_RIGHT,              "ShipRight",           true,  inputRIGHT        ) \
   BINDING( BINDING_SCRBRD,             "ShowScoreboard",      true,  inputSCRBRD       ) \
   BINDING( BINDING_HELP,               "Help",                false, keyHELP           ) \
   BINDING( BINDING_OUTGAMECHAT,        "OutOfGameChat",       false, keyOUTGAMECHAT    ) \
   BINDING( BINDING_MISSION,            "Mission",             true,  inputMISSION      ) \
   BINDING( BINDING_FPS,                "FPS",                 false, keyFPS            ) \
   BINDING( BINDING_DIAG,               "Diagnostics",         false, keyDIAG           ) \
   BINDING( BINDING_LOAD_PRESET_1,      "LoadLoadoutPreset1",  false, keyLoadPreset1    ) \
   BINDING( BINDING_LOAD_PRESET_2,      "LoadLoadoutPreset2",  false, keyLoadPreset2    ) \
   BINDING( BINDING_LOAD_PRESET_3,      "LoadLoadoutPreset3",  false, keyLoadPreset3    ) \
   BINDING( BINDING_SAVE_PRESET_1,      "SaveLoadoutPreset1",  false, keySavePreset1    ) \
   BINDING( BINDING_SAVE_PRESET_2,      "SaveLoadoutPreset2",  false, keySavePreset2    ) \
   BINDING( BINDING_SAVE_PRESET_3,      "SaveLoadoutPreset3",  false, keySavePreset3    ) \
   BINDING( BINDING_TOGGLE_RATING,      "ToggleRating",        true,  inputTOGGLERATING ) \
                                                                                          \
   /* Editor specific */                                                                  \
                                                                                          \
   BINDING( BINDING_TEAM_EDITOR,        "TeamEditor",          false, KEY_F2            ) \
   BINDING( BINDING_GAME_PARAMS_EDITOR, "GameParameterEditor", false, KEY_F3            ) \


enum BindingNameEnum {
#define BINDING(enumName, b, c, d) enumName,
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
      static InputCode sdlKeyToInputCode(int key);             // Convert SDL keys to InputCode
      static int inputCodeToSDLKey(InputCode inputCode);       // Take a InputCode and return the SDL equivalent
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
#define BINDING(a, b, c, memberName) InputCode memberName;
    BINDING_TABLE
#undef BINDING
};



};     // namespace Zap

#endif

