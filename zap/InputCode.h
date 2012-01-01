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

#ifndef _INPUTCODE_H_
#define _INPUTCODE_H_

#include <string>

namespace Zap
{

#include "InputCodeEnum.h"

enum JoystickJoysticks {
   JOYSTICK_DPAD,
   JOYSTICK_STICK_1,
   JOYSTICK_STICK_2
};


const char *inputCodeToString(InputCode inputCode);
InputCode stringToInputCode(const char *inputName);

void setInputCodeState(InputCode inputCode, bool state);    // Set key state (t=down, f=up)
bool getInputCodeState(InputCode inputCode);                // Return current key state (t=down, f=up)
void resetInputCodeStates();                                // Initialize key states
void dumpInputCodeStates();                                 // Log key states for testing
void initializeKeyNames();

// Ensure that specified modifer is the only one actually pressed... i.e. if Ctrl and Alt were down, checkModifier(KEY_CTRL) would be false
std::string makeInputString(InputCode inputCode);
bool checkModifier(InputCode mod1);            
bool checkModifier(InputCode mod1, InputCode mod2);            
bool checkModifier(InputCode mod1, InputCode mod2, InputCode mod3);            

#ifndef ZAP_DEDICATED
   InputCode sdlKeyToInputCode(int key);              // Convert SDL keys to InputCode
   int inputCodeToSDLKey(InputCode inputCode);        // Take a InputCode and return the SDL equivalent
#endif

char keyToAscii(int unicode, InputCode inputCode);    // Return a printable ascii char, if possible
bool isControllerButton(InputCode inputCode);         // Does inputCode represent a controller button?

extern InputCode inputSELWEAP1[2];
extern InputCode inputSELWEAP2[2];
extern InputCode inputSELWEAP3[2];
extern InputCode inputADVWEAP[2];
extern InputCode inputCMDRMAP[2];
extern InputCode inputTEAMCHAT[2];
extern InputCode inputGLOBCHAT[2];
extern InputCode inputQUICKCHAT[2];
extern InputCode inputCMDCHAT[2];
extern InputCode inputLOADOUT[2];
extern InputCode inputMOD1[2];
extern InputCode inputMOD2[2];
extern InputCode inputFIRE[2];
extern InputCode inputDROPITEM[2];
extern InputCode inputTOGVOICE[2];
extern InputCode inputUP[2];
extern InputCode inputDOWN[2];
extern InputCode inputLEFT[2];
extern InputCode inputRIGHT[2];
extern InputCode inputSCRBRD[2];
extern InputCode keyHELP;
extern InputCode keyOUTGAMECHAT;
extern InputCode keyMISSION;
extern InputCode keyFPS;
extern InputCode keyDIAG;


};     // namespace Zap

#endif

