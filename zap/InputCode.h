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

#include "input.h"            // For InputMode enum
#include "tnlVector.h"
#include "Joystick.h"
#include <string>

using namespace std;

namespace Zap
{

#include "InputCodeEnum.h"    // Include inside Zap namespace


class InputCodeManager 
{
private:
   bool mBindingsHaveKeypadEntry;
   InputMode mInputMode;             // Joystick or Keyboard

public:
   enum JoystickJoysticks {
      JOYSTICK_DPAD,
      JOYSTICK_STICK_1,
      JOYSTICK_STICK_2
   };

   enum BindingName {
      BINDING_SELWEAP1,
      BINDING_SELWEAP2,
      BINDING_SELWEAP3,
      BINDING_ADVWEAP,
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
      BINDING_DIAG,
      
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

   static const char *inputCodeToString(InputCode inputCode);
   static InputCode stringToInputCode(const char *inputName);

   static void setState(InputCode inputCode, bool state);   // Set key state (t=down, f=up)
   static bool getState(InputCode inputCode);               // Return current key state (t=down, f=up)
   static void resetStates();                               // Initialize key states
   static void dumpInputCodeStates();                       // Log key states for testing
   static void initializeKeyNames();

   // Some converters
   InputCode filterInputCode(InputCode inputCode);    // Calls filters below

   static InputCode joyHatToInputCode(int hatDirectionMask);
   static InputCode joystickButtonToInputCode(Joystick::Button button);

   static InputCode convertJoystickToKeyboard(InputCode inputCode);
private:
   static InputCode convertNumPadToNum(InputCode inputCode);
   bool checkIfBindingsHaveKeypad();

public:
   // Ensure that specified modifer is the only one actually pressed... i.e. if Ctrl and Alt were down, checkModifier(KEY_CTRL) would be false
   static string makeInputString(InputCode inputCode);

   static bool checkModifier(InputCode mod1);            
   static bool checkModifier(InputCode mod1, InputCode mod2);            
   static bool checkModifier(InputCode mod1, InputCode mod2, InputCode mod3);            

   void setInputMode(InputMode inputMode);
   InputMode getInputMode();
   string getInputModeString();     // Returns display-friendly mode designator like "Keyboard" or "Joystick 1"

   #ifndef ZAP_DEDICATED
      static InputCode sdlKeyToInputCode(int key);              // Convert SDL keys to InputCode
      static int inputCodeToSDLKey(InputCode inputCode);        // Take a InputCode and return the SDL equivalent
   #endif

   static char keyToAscii(int unicode, InputCode inputCode);    // Return a printable ascii char, if possible
   static bool isControllerButton(InputCode inputCode);         // Does inputCode represent a controller button?
   static bool isKeypadKey(InputCode inputCode);                // Is inputCode on the numeric keypad?

   InputCode getBinding(BindingName binding);
   InputCode getBinding(BindingName bindingName, InputMode inputMode);
   void setBinding(BindingName bindingName, InputCode key);
   void setBinding(BindingName bindingName, InputMode inputMode, InputCode key);

private:
   InputCode inputSELWEAP1[2];
   InputCode inputSELWEAP2[2];
   InputCode inputSELWEAP3[2];
   InputCode inputADVWEAP[2];
   InputCode inputCMDRMAP[2];
   InputCode inputTEAMCHAT[2];
   InputCode inputGLOBCHAT[2];
   InputCode inputQUICKCHAT[2];
   InputCode inputCMDCHAT[2];
   InputCode inputLOADOUT[2];
   InputCode inputMOD1[2];
   InputCode inputMOD2[2];
   InputCode inputFIRE[2];
   InputCode inputDROPITEM[2];
   InputCode inputTOGVOICE[2];
   InputCode inputUP[2];
   InputCode inputDOWN[2];
   InputCode inputLEFT[2];
   InputCode inputRIGHT[2];
   InputCode inputSCRBRD[2];
   InputCode keyHELP;
   InputCode keyOUTGAMECHAT;
   InputCode keyMISSION;
   InputCode keyFPS;
   InputCode keyDIAG;
};

};     // namespace Zap

#endif

