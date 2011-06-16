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

#ifndef _KEYCODE_H_
#define _KEYCODE_H_

namespace Zap
{

#include "keyCodeEnum.h"

enum JoystickJoysticks {
   JOYSTICK_DPAD,
   JOYSTICK_STICK_1,
   JOYSTICK_STICK_2
};


const char *keyCodeToString(KeyCode keyCode);
KeyCode stringToKeyCode(const char *keyname);

void setKeyState(KeyCode keyCode, bool state);  // Set key state (t=down, f=up)
bool getKeyState(KeyCode keyCode);              // Return current key state (t=down, f=up)
void resetKeyStates();                          // Initialize key states
void dumpKeyStates();                           // Log key states for testing
void checkModifierKeyState();                   // Handle Ctrl, Shift, Alt
//KeyCode standardSDLKeyToKeyCode(int key);       // Convert SDL keycodes to our KeyCode system

//KeyCode standardGLUTKeyToKeyCode(int key);      // Convert standard keys to KeyCode
//KeyCode specialGLUTKeyToKeyCode(int key);       // Convert special keys to KeyCode
KeyCode standardSDLKeyToKeyCode(int key);       // Convert SDL keys to KeyCode
char keyToAscii(int key, KeyCode keyCode);      // Return a printable ascii char, if possible
bool isControllerButton(KeyCode keyCode);       // Does keyCode represent a controller button?

extern KeyCode keySELWEAP1[2];
extern KeyCode keySELWEAP2[2];
extern KeyCode keySELWEAP3[2];
extern KeyCode keyADVWEAP[2];
extern KeyCode keyCMDRMAP[2];
extern KeyCode keyTEAMCHAT[2];
extern KeyCode keyGLOBCHAT[2];
extern KeyCode keyQUICKCHAT[2];
extern KeyCode keyCMDCHAT[2];
extern KeyCode keyLOADOUT[2];
extern KeyCode keyMOD1[2];
extern KeyCode keyMOD2[2];
extern KeyCode keyFIRE[2];
extern KeyCode keyDROPITEM[2];
extern KeyCode keyTOGVOICE[2];
extern KeyCode keyUP[2];
extern KeyCode keyDOWN[2];
extern KeyCode keyLEFT[2];
extern KeyCode keyRIGHT[2];
extern KeyCode keySCRBRD[2];
extern KeyCode keyHELP;
extern KeyCode keyOUTGAMECHAT;
extern KeyCode keyMISSION;
extern KeyCode keyFPS;
extern KeyCode keyDIAG;


};     // namespace Zap

#endif

