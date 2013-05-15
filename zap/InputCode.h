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


   // TODO -- Hook into xmacro with BINDING_STRINGS
   enum BindingName {
      BINDING_SELWEAP1,    // Name def depends on these being in this order from here...
      BINDING_SELWEAP2,
      BINDING_SELWEAP3,
      BINDING_ADVWEAP,
      BINDING_ADVWEAP2,
      BINDING_PREVWEAP,
      BINDING_CMDRMAP,
      BINDING_TEAMCHAT,
      BINDING_GLOBCHAT,
      BINDING_QUICKCHAT,
      BINDING_CMDCHAT,
      BINDING_LOADOUT,
      BINDING_MOD1,
      BINDING_MOD2,
      BINDING_FIRE,
      BINDING_DROPITEM,
      BINDING_TOGVOICE,
      BINDING_UP,
      BINDING_DOWN,
      BINDING_LEFT,
      BINDING_RIGHT,
      BINDING_SCRBRD,
      BINDING_HELP,
      BINDING_OUTGAMECHAT,
      BINDING_MISSION,
      BINDING_FPS,
      BINDING_DIAG,        // ...to here
      BINDING_DEFINEABLE_KEY_COUNT,
      
      BINDING_NONE,
      BINDING_DUMMY_MOVE_SHIP_KEYS_UD,
      BINDING_DUMMY_MOVE_SHIP_KEYS_LR,
      BINDING_DUMMY_MOVE_SHIP_KEYS_MOUSE,
      BINDING_DUMMY_STICK_LEFT,
      BINDING_DUMMY_STICK_RIGHT,
      BINDING_DUMMY_MSG_MODE,
      BINDING_DUMMY_SS_MODE
   };

   InputCodeManager();     // Constructor
   virtual ~InputCodeManager();

   static const char *inputCodeToString(InputCode inputCode);

   static InputCode stringToInputCode(const char *inputName);

   static void setState(InputCode inputCode, bool state);   // Set key state (t=down, f=up)
   static bool getState(InputCode inputCode);               // Return current key state (t=down, f=up)
   static void resetStates();                               // Initialize key states
   static void dumpInputCodeStates();                       // Log key states for testing
   static void initializeKeyNames();

   //static S32 getBindingCount();
   static string getBindingName(BindingName binding);
   
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
   static bool isModifier(InputCode inputCode);                // Is inputCode a modifier key?

   static JoystickButton inputCodeToJoystickButton(InputCode inputCode);


   InputCode getBinding(BindingName binding) const; 
   InputCode getBinding(BindingName bindingName, InputMode inputMode) const;
   void setBinding(BindingName bindingName, InputCode key);
   void setBinding(BindingName bindingName, InputMode inputMode, InputCode key);
};

////////////////////////////////////////
////////////////////////////////////////

struct BindingSet
{
   BindingSet();     // Constructor
   virtual ~BindingSet();

   bool hasKeypad();

   InputCode getBinding(InputCodeManager::BindingName bindingName) const;
   void setBinding(InputCodeManager::BindingName bindingName, InputCode key);

   InputCode inputSELWEAP1;
   InputCode inputSELWEAP2;
   InputCode inputSELWEAP3;
   InputCode inputADVWEAP;
   InputCode inputADVWEAP2;
   InputCode inputPREVWEAP;
   InputCode inputCMDRMAP;
   InputCode inputTEAMCHAT;
   InputCode inputGLOBCHAT;
   InputCode inputQUICKCHAT;
   InputCode inputCMDCHAT;
   InputCode inputLOADOUT;
   InputCode inputMOD1;
   InputCode inputMOD2;
   InputCode inputFIRE;
   InputCode inputDROPITEM;
   InputCode inputTOGVOICE;
   InputCode inputUP;
   InputCode inputDOWN;
   InputCode inputLEFT;
   InputCode inputRIGHT;
   InputCode inputSCRBRD;
   InputCode keyHELP;
   InputCode keyOUTGAMECHAT;
   InputCode keyMISSION;
   InputCode keyFPS;
   InputCode keyDIAG;
};



};     // namespace Zap

#endif

