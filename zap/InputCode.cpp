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

#include "InputCode.h"
#include "Joystick.h"

#include "../tnl/tnlJournal.h"
#include "../tnl/tnlLog.h"         // For logprintf

#include "zapjournal.h"            // For journaling support

#include <ctype.h>

#ifndef ZAP_DEDICATED
#include "SDL/SDL.h"
#endif

#ifdef TNL_OS_WIN32
#include <windows.h>   // For ARRAYSIZE 
#endif


using namespace TNL;

namespace Zap
{

static bool inputCodeIsDown[MAX_INPUT_CODES];

// We have two sets of input codes defined -- one for when we're playing
// with a keyboard, and one for joystick play
InputCode inputSELWEAP1[2];       // Select weapon 1
InputCode inputSELWEAP2[2];       // Select weapon 2
InputCode inputSELWEAP3[2];       // Select weapon 3
InputCode inputADVWEAP[2];        // Pick next weapon
InputCode inputCMDRMAP[2];        // Toggle commander's map
InputCode inputTEAMCHAT[2];       // Send team chat message
InputCode inputGLOBCHAT[2];       // Send global chat message
InputCode inputQUICKCHAT[2];      // Enter QuickChat mode
InputCode inputCMDCHAT[2];        // Go directly to command mode, bypassing chat
InputCode inputLOADOUT[2];        // Enter Loadout mode
InputCode inputMOD1[2];           // Activate module 1
InputCode inputMOD2[2];           // Activate module 2
InputCode inputFIRE[2];           // Fire
InputCode inputDROPITEM[2];       // Drop flag or other item
InputCode inputTOGVOICE[2];       // Toggle voice chat
InputCode inputUP[2];             // Move ship
InputCode inputDOWN[2];
InputCode inputLEFT[2];
InputCode inputRIGHT[2];
InputCode inputSCRBRD[2];         // Show scoreboard

// These input codes are constant, regardless of mode, and can't be changed by the user
// Define these here to ensure they can be used for defining menus during initialization
// They really don't belong here, but what to do... what to do.
InputCode keyHELP = KEY_F1;              // Display help
InputCode keyOUTGAMECHAT = KEY_F5;       // Out of game chat
InputCode keyFPS = KEY_F6;               // Show FPS display
InputCode keyDIAG = KEY_F7;              // Show diagnostic overlay
InputCode keyMISSION = KEY_F2;           // Show current mission info


// Initialize state of keys... assume none are depressed
void resetInputCodeStates()
{
   for (int i = 0; i < MAX_INPUT_CODES; i++)
      inputCodeIsDown[i] = false;
}

// Prints list of any input codes that are down, for debugging purposes
void dumpInputCodeStates()
{
  for (int i = 0; i < MAX_INPUT_CODES; i++)
     if(inputCodeIsDown[i])
        logprintf("Key %s down", inputCodeToString((InputCode) i));
}


// Set state of a input code as Up (false) or Down (true)
void setInputCodeState(InputCode inputCode, bool state)
{
   inputCodeIsDown[(int) inputCode] = state;
}


// Returns true of input code is down, false if it is up
bool getInputCodeState(InputCode inputCode)
{
//   logprintf("State: key %d is %s", inputCode, keyIsDown[(int) inputCode] ? "down" : "up");
      return inputCodeIsDown[(int) inputCode];
}


InputCode modifiers[] = { KEY_CTRL, KEY_ALT, KEY_SHIFT, KEY_META, KEY_SUPER };


std::string makeInputString(InputCode inputCode)
{
   for(S32 i = 0; i < S32(ARRAYSIZE(modifiers)); i++)
      if(inputCode == modifiers[i])
         return inputCodeToString(inputCode);

   std::string inputString = "";
   std::string joiner = "+";

   for(S32 i = 0; i < S32(ARRAYSIZE(modifiers)); i++)
      if(getInputCodeState(modifiers[i]))
         inputString += inputCodeToString(modifiers[i]) + joiner;
   
   inputString += inputCodeToString(inputCode);
   return inputString;
}


// Can pass in one of the above, or KEY_NONE to check if no modifiers are pressed
bool checkModifier(InputCode mod1)    
{
   S32 foundCount = 0;

   for(S32 i = 0; i < S32(ARRAYSIZE(modifiers)); i++)
      if(getInputCodeState(modifiers[i]))                      // Modifier is down
      {
         if(modifiers[i] == mod1)      
            foundCount++;
         else                                                  // Wrong modifier!               
            return false;        
      }

   return mod1 == KEY_NONE || foundCount == 1;
}


// Check if two modifiers are both pressed (i.e. Ctrl+Alt)
bool checkModifier(InputCode mod1, InputCode mod2)
{
   S32 foundCount = 0;

   for(S32 i = 0; i < S32(ARRAYSIZE(modifiers)); i++)
      if(getInputCodeState(modifiers[i]))                      // Modifier is down
      {
         if(modifiers[i] == mod1 || modifiers[i] == mod2)      
            foundCount++;
         else                                                  // Wrong modifier!               
            return false;        
      }

   return foundCount == 2;
}


// Check to see if three modifers are all pressed (i.e. Ctrl+Alt+Shift)
bool checkModifier(InputCode mod1, InputCode mod2, InputCode mod3)
{
   S32 foundCount = 0;

   for(S32 i = 0; i < S32(ARRAYSIZE(modifiers)); i++)
      if(getInputCodeState(modifiers[i]))                      // Modifier is down
      {
         if(modifiers[i] == mod1 || modifiers[i] == mod2 || modifiers[i] == mod3)      
            foundCount++;
         else                                                  // Wrong modifier!               
            return false;        
      }

   return foundCount == 3;
}


bool isPrintable(char c)
{
   return c >= 32 && c <= 126;
}


// If there is a printable ASCII code for the pressed key, return it
// Filter out some know spurious keystrokes
char keyToAscii(int unicode, InputCode inputCode)
{
   //if(inputCode == KEY_UP || inputCode == KEY_DOWN || inputCode == KEY_LEFT || inputCode == KEY_RIGHT)
   //   return 0;

   if((unicode & 0xFF80) != 0) 
      return 0;

   char ch = unicode & 0x7F;

   return isPrintable(ch) ? ch : 0;
}


#ifndef ZAP_DEDICATED
// Translate SDL standard keys to our InputCodes
InputCode sdlKeyToInputCode(int key)
{
   switch(key)
   {
	   case SDLK_BACKSPACE:
		   return KEY_BACKSPACE;
	   case SDLK_TAB:
		   return KEY_TAB;
	   case SDLK_CLEAR:
		   return KEY_CLEAR;
	   case SDLK_RETURN:
		   return KEY_ENTER;
	   case SDLK_PAUSE:
		   return KEY_PAUSE;
	   case SDLK_ESCAPE:
		   return KEY_ESCAPE;
	   case SDLK_SPACE:
		   return KEY_SPACE;
	   case SDLK_EXCLAIM:
		   return KEY_EXCLAIM;
	   case SDLK_QUOTEDBL:
		   return KEY_DOUBLEQUOTE;
	   case SDLK_HASH:
		   return KEY_HASH;
	   case SDLK_DOLLAR:
		   return KEY_DOLLAR;
	   case SDLK_AMPERSAND:
		   return KEY_AMPERSAND;
	   case SDLK_QUOTE:
		   return KEY_QUOTE;
	   case SDLK_LEFTPAREN:
		   return KEY_OPENPAREN;
	   case SDLK_RIGHTPAREN:
		   return KEY_CLOSEPAREN;
	   case SDLK_ASTERISK:
		   return KEY_ASTERISK;
	   case SDLK_PLUS:
		   return KEY_PLUS;
	   case SDLK_COMMA:
		   return KEY_COMMA;
	   case SDLK_MINUS:
		   return KEY_MINUS;
	   case SDLK_PERIOD:
		   return KEY_PERIOD;
	   case SDLK_SLASH:
		   return KEY_SLASH;
	   case SDLK_0:
		   return KEY_0;
	   case SDLK_1:
		   return KEY_1;
	   case SDLK_2:
		   return KEY_2;
	   case SDLK_3:
		   return KEY_3;
	   case SDLK_4:
		   return KEY_4;
	   case SDLK_5:
		   return KEY_5;
	   case SDLK_6:
		   return KEY_6;
	   case SDLK_7:
		   return KEY_7;
	   case SDLK_8:
		   return KEY_8;
	   case SDLK_9:
		   return KEY_9;
	   case SDLK_COLON:
		   return KEY_COLON;
	   case SDLK_SEMICOLON:
		   return KEY_SEMICOLON;
	   case SDLK_LESS:
		   return KEY_LESS;
	   case SDLK_EQUALS:
		   return KEY_EQUALS;
	   case SDLK_GREATER:
		   return KEY_GREATER;
	   case SDLK_QUESTION:
		   return KEY_QUESTION;
	   case SDLK_AT:
		   return KEY_AT;
   	
	   case SDLK_LEFTBRACKET:
		   return KEY_OPENBRACKET;
	   case SDLK_BACKSLASH:
		   return KEY_BACKSLASH;
	   case SDLK_RIGHTBRACKET:
		   return KEY_CLOSEBRACKET;
	   case SDLK_CARET:
		   return KEY_CARET;
	   case SDLK_UNDERSCORE:
		   return KEY_UNDERSCORE;
	   case SDLK_BACKQUOTE:
		   return KEY_BACKQUOTE;
	   case SDLK_a:
		   return KEY_A;
	   case SDLK_b:
		   return KEY_B;
	   case SDLK_c:
		   return KEY_C;
	   case SDLK_d:
		   return KEY_D;
	   case SDLK_e:
		   return KEY_E;
	   case SDLK_f:
		   return KEY_F;
	   case SDLK_g:
		   return KEY_G;
	   case SDLK_h:
		   return KEY_H;
	   case SDLK_i:
		   return KEY_I;
	   case SDLK_j:
		   return KEY_J;
	   case SDLK_k:
		   return KEY_K;
	   case SDLK_l:
		   return KEY_L;
	   case SDLK_m:
		   return KEY_M;
	   case SDLK_n:
		   return KEY_N;
	   case SDLK_o:
		   return KEY_O;
	   case SDLK_p:
		   return KEY_P;
	   case SDLK_q:
		   return KEY_Q;
	   case SDLK_r:
		   return KEY_R;
	   case SDLK_s:
		   return KEY_S;
	   case SDLK_t:
		   return KEY_T;
	   case SDLK_u:
		   return KEY_U;
	   case SDLK_v:
		   return KEY_V;
	   case SDLK_w:
		   return KEY_W;
	   case SDLK_x:
		   return KEY_X;
	   case SDLK_y:
		   return KEY_Y;
	   case SDLK_z:
		   return KEY_Z;
	   case SDLK_DELETE:
		   return KEY_DELETE;

	   // International keyboard syms
	   case SDLK_WORLD_0:
		   return KEY_WORLD_0;
	   case SDLK_WORLD_1:
		   return KEY_WORLD_1;
	   case SDLK_WORLD_2:
		   return KEY_WORLD_2;
	   case SDLK_WORLD_3:
		   return KEY_WORLD_3;
	   case SDLK_WORLD_4:
		   return KEY_WORLD_4;
	   case SDLK_WORLD_5:
		   return KEY_WORLD_5;
	   case SDLK_WORLD_6:
		   return KEY_WORLD_6;
	   case SDLK_WORLD_7:
		   return KEY_WORLD_7;
	   case SDLK_WORLD_8:
		   return KEY_WORLD_8;
	   case SDLK_WORLD_9:
		   return KEY_WORLD_9;
	   case SDLK_WORLD_10:
		   return KEY_WORLD_10;
	   case SDLK_WORLD_11:
		   return KEY_WORLD_11;
	   case SDLK_WORLD_12:
		   return KEY_WORLD_12;
	   case SDLK_WORLD_13:
		   return KEY_WORLD_13;
	   case SDLK_WORLD_14:
		   return KEY_WORLD_14;
	   case SDLK_WORLD_15:
		   return KEY_WORLD_15;
	   case SDLK_WORLD_16:
		   return KEY_WORLD_16;
	   case SDLK_WORLD_17:
		   return KEY_WORLD_17;
	   case SDLK_WORLD_18:
		   return KEY_WORLD_18;
	   case SDLK_WORLD_19:
		   return KEY_WORLD_19;
	   case SDLK_WORLD_20:
		   return KEY_WORLD_20;
	   case SDLK_WORLD_21:
		   return KEY_WORLD_21;
	   case SDLK_WORLD_22:
		   return KEY_WORLD_22;
	   case SDLK_WORLD_23:
		   return KEY_WORLD_23;
	   case SDLK_WORLD_24:
		   return KEY_WORLD_24;
	   case SDLK_WORLD_25:
		   return KEY_WORLD_25;
	   case SDLK_WORLD_26:
		   return KEY_WORLD_26;
	   case SDLK_WORLD_27:
		   return KEY_WORLD_27;
	   case SDLK_WORLD_28:
		   return KEY_WORLD_28;
	   case SDLK_WORLD_29:
		   return KEY_WORLD_29;
	   case SDLK_WORLD_30:
		   return KEY_WORLD_30;
	   case SDLK_WORLD_31:
		   return KEY_WORLD_31;
	   case SDLK_WORLD_32:
		   return KEY_WORLD_32;
	   case SDLK_WORLD_33:
		   return KEY_WORLD_33;
	   case SDLK_WORLD_34:
		   return KEY_WORLD_34;
	   case SDLK_WORLD_35:
		   return KEY_WORLD_35;
	   case SDLK_WORLD_36:
		   return KEY_WORLD_36;
	   case SDLK_WORLD_37:
		   return KEY_WORLD_37;
	   case SDLK_WORLD_38:
		   return KEY_WORLD_38;
	   case SDLK_WORLD_39:
		   return KEY_WORLD_39;
	   case SDLK_WORLD_40:
		   return KEY_WORLD_40;
	   case SDLK_WORLD_41:
		   return KEY_WORLD_41;
	   case SDLK_WORLD_42:
		   return KEY_WORLD_42;
	   case SDLK_WORLD_43:
		   return KEY_WORLD_43;
	   case SDLK_WORLD_44:
		   return KEY_WORLD_44;
	   case SDLK_WORLD_45:
		   return KEY_WORLD_45;
	   case SDLK_WORLD_46:
		   return KEY_WORLD_46;
	   case SDLK_WORLD_47:
		   return KEY_WORLD_47;
	   case SDLK_WORLD_48:
		   return KEY_WORLD_48;
	   case SDLK_WORLD_49:
		   return KEY_WORLD_49;
	   case SDLK_WORLD_50:
		   return KEY_WORLD_50;
	   case SDLK_WORLD_51:
		   return KEY_WORLD_51;
	   case SDLK_WORLD_52:
		   return KEY_WORLD_52;
	   case SDLK_WORLD_53:
		   return KEY_WORLD_53;
	   case SDLK_WORLD_54:
		   return KEY_WORLD_54;
	   case SDLK_WORLD_55:
		   return KEY_WORLD_55;
	   case SDLK_WORLD_56:
		   return KEY_WORLD_56;
	   case SDLK_WORLD_57:
		   return KEY_WORLD_57;
	   case SDLK_WORLD_58:
		   return KEY_WORLD_58;
	   case SDLK_WORLD_59:
		   return KEY_WORLD_59;
	   case SDLK_WORLD_60:
		   return KEY_WORLD_60;
	   case SDLK_WORLD_61:
		   return KEY_WORLD_61;
	   case SDLK_WORLD_62:
		   return KEY_WORLD_62;
	   case SDLK_WORLD_63:
		   return KEY_WORLD_63;
	   case SDLK_WORLD_64:
		   return KEY_WORLD_64;
	   case SDLK_WORLD_65:
		   return KEY_WORLD_65;
	   case SDLK_WORLD_66:
		   return KEY_WORLD_66;
	   case SDLK_WORLD_67:
		   return KEY_WORLD_67;
	   case SDLK_WORLD_68:
		   return KEY_WORLD_68;
	   case SDLK_WORLD_69:
		   return KEY_WORLD_69;
	   case SDLK_WORLD_70:
		   return KEY_WORLD_70;
	   case SDLK_WORLD_71:
		   return KEY_WORLD_71;
	   case SDLK_WORLD_72:
		   return KEY_WORLD_72;
	   case SDLK_WORLD_73:
		   return KEY_WORLD_73;
	   case SDLK_WORLD_74:
		   return KEY_WORLD_74;
	   case SDLK_WORLD_75:
		   return KEY_WORLD_75;
	   case SDLK_WORLD_76:
		   return KEY_WORLD_76;
	   case SDLK_WORLD_77:
		   return KEY_WORLD_77;
	   case SDLK_WORLD_78:
		   return KEY_WORLD_78;
	   case SDLK_WORLD_79:
		   return KEY_WORLD_79;
	   case SDLK_WORLD_80:
		   return KEY_WORLD_80;
	   case SDLK_WORLD_81:
		   return KEY_WORLD_81;
	   case SDLK_WORLD_82:
		   return KEY_WORLD_82;
	   case SDLK_WORLD_83:
		   return KEY_WORLD_83;
	   case SDLK_WORLD_84:
		   return KEY_WORLD_84;
	   case SDLK_WORLD_85:
		   return KEY_WORLD_85;
	   case SDLK_WORLD_86:
		   return KEY_WORLD_86;
	   case SDLK_WORLD_87:
		   return KEY_WORLD_87;
	   case SDLK_WORLD_88:
		   return KEY_WORLD_88;
	   case SDLK_WORLD_89:
		   return KEY_WORLD_89;
	   case SDLK_WORLD_90:
		   return KEY_WORLD_90;
	   case SDLK_WORLD_91:
		   return KEY_WORLD_91;
	   case SDLK_WORLD_92:
		   return KEY_WORLD_92;
	   case SDLK_WORLD_93:
		   return KEY_WORLD_93;
	   case SDLK_WORLD_94:
		   return KEY_WORLD_94;
	   case SDLK_WORLD_95:
		   return KEY_WORLD_95;

	   // Numeric keypad
	   case SDLK_KP0:
		   return KEY_KEYPAD0;
	   case SDLK_KP1:
		   return KEY_KEYPAD1;
	   case SDLK_KP2:
		   return KEY_KEYPAD2;
	   case SDLK_KP3:
		   return KEY_KEYPAD3;
	   case SDLK_KP4:
		   return KEY_KEYPAD4;
	   case SDLK_KP5:
		   return KEY_KEYPAD5;
	   case SDLK_KP6:
		   return KEY_KEYPAD6;
	   case SDLK_KP7:
		   return KEY_KEYPAD7;
	   case SDLK_KP8:
		   return KEY_KEYPAD8;
	   case SDLK_KP9:
		   return KEY_KEYPAD9;
	   case SDLK_KP_PERIOD:
		   return KEY_KEYPAD_PERIOD;
	   case SDLK_KP_DIVIDE:
		   return KEY_KEYPAD_DIVIDE;
	   case SDLK_KP_MULTIPLY:
		   return KEY_KEYPAD_MULTIPLY;
	   case SDLK_KP_MINUS:
		   return KEY_KEYPAD_MINUS;
	   case SDLK_KP_PLUS:
		   return KEY_KEYPAD_PLUS;
	   case SDLK_KP_ENTER:
		   return KEY_KEYPAD_ENTER;
	   case SDLK_KP_EQUALS:
		   return KEY_KEYPAD_EQUALS;

	   // Arrows + Home/End pad
	   case SDLK_UP:
		   return KEY_UP;
	   case SDLK_DOWN:
		   return KEY_DOWN;
	   case SDLK_RIGHT:
		   return KEY_RIGHT;
	   case SDLK_LEFT:
		   return KEY_LEFT;
	   case SDLK_INSERT:
		   return KEY_INSERT;
	   case SDLK_HOME:
		   return KEY_HOME;
	   case SDLK_END:
		   return KEY_END;
	   case SDLK_PAGEUP:
		   return KEY_PAGEUP;
	   case SDLK_PAGEDOWN:
		   return KEY_PAGEDOWN;

	   // Function keys
	   case SDLK_F1:
		   return KEY_F1;
	   case SDLK_F2:
		   return KEY_F2;
	   case SDLK_F3:
		   return KEY_F3;
	   case SDLK_F4:
		   return KEY_F4;
	   case SDLK_F5:
		   return KEY_F5;
	   case SDLK_F6:
		   return KEY_F6;
	   case SDLK_F7:
		   return KEY_F7;
	   case SDLK_F8:
		   return KEY_F8;
	   case SDLK_F9:
		   return KEY_F9;
	   case SDLK_F10:
		   return KEY_F10;
	   case SDLK_F11:
		   return KEY_F11;
	   case SDLK_F12:
		   return KEY_F12;
	   case SDLK_F13:
		   return KEY_F13;
	   case SDLK_F14:
		   return KEY_F14;
	   case SDLK_F15:
		   return KEY_F15;

	   // Key state modifier keys
	   case SDLK_NUMLOCK:
		   return KEY_NUMLOCK;
	   case SDLK_CAPSLOCK:
		   return KEY_CAPSLOCK;
	   case SDLK_SCROLLOCK:
		   return KEY_SCROLLOCK;
	   case SDLK_RSHIFT:
	   case SDLK_LSHIFT:
		   return KEY_SHIFT;
	   case SDLK_RCTRL:
	   case SDLK_LCTRL:
		   return KEY_CTRL;
	   case SDLK_RALT:
	   case SDLK_LALT:
		   return KEY_ALT;
	   case SDLK_RMETA:
	   case SDLK_LMETA:
		   return KEY_META;
	   case SDLK_LSUPER:
	   case SDLK_RSUPER:
		   return KEY_SUPER;
	   case SDLK_MODE:
		   return KEY_MODE;
	   case SDLK_COMPOSE:
		   return KEY_COMPOSE;

	   // Miscellaneous function keys
	   case SDLK_HELP:
		   return KEY_HELP;
	   case SDLK_PRINT:
		   return KEY_PRINT;
	   case SDLK_SYSREQ:
		   return KEY_SYSREQ;
	   case SDLK_BREAK:
		   return KEY_BREAK;
	   case SDLK_MENU:
		   return KEY_MENU;
	   case SDLK_POWER:
		   return KEY_POWER;
	   case SDLK_EURO:
		   return KEY_EURO;
	   case SDLK_UNDO:
		   return KEY_UNDO;
      default:
         logprintf(LogConsumer::LogWarning, "Unknown key detected: %d", key);
         return KEY_UNKNOWN;
   }
}


S32 inputCodeToSDLKey(InputCode inputCode)
{
   switch(inputCode)
   {
	   case KEY_BACKSPACE:
		   return SDLK_BACKSPACE;
	   case KEY_TAB:
		   return SDLK_TAB;
	   case KEY_CLEAR:
		   return SDLK_CLEAR;
	   case KEY_ENTER:
		   return SDLK_RETURN;
	   case KEY_PAUSE:
		   return SDLK_PAUSE;
	   case KEY_ESCAPE:
		   return SDLK_ESCAPE;
	   case KEY_SPACE:
		   return SDLK_SPACE;
	   case KEY_EXCLAIM:
		   return SDLK_EXCLAIM;
	   case KEY_DOUBLEQUOTE:
		   return SDLK_QUOTEDBL;
	   case KEY_HASH:
		   return SDLK_HASH;
	   case KEY_DOLLAR:
		   return SDLK_DOLLAR;
	   case KEY_AMPERSAND:
		   return SDLK_AMPERSAND;
	   case KEY_QUOTE:
		   return SDLK_QUOTE;
	   case KEY_OPENPAREN:
		   return SDLK_LEFTPAREN;
	   case KEY_CLOSEPAREN:
		   return SDLK_RIGHTPAREN;
	   case KEY_ASTERISK:
		   return SDLK_ASTERISK;
	   case KEY_PLUS:
		   return SDLK_PLUS;
	   case KEY_COMMA:
		   return SDLK_COMMA;
	   case KEY_MINUS:
		   return SDLK_MINUS;
	   case KEY_PERIOD:
		   return SDLK_PERIOD;
	   case KEY_SLASH:
		   return SDLK_SLASH;
	   case KEY_0:
		   return SDLK_0;
	   case KEY_1:
		   return SDLK_1;
	   case KEY_2:
		   return SDLK_2;
	   case KEY_3:
		   return SDLK_3;
	   case KEY_4:
		   return SDLK_4;
	   case KEY_5:
		   return SDLK_5;
	   case KEY_6:
		   return SDLK_6;
	   case KEY_7:
		   return SDLK_7;
	   case KEY_8:
		   return SDLK_8;
	   case KEY_9:
		   return SDLK_9;
	   case KEY_COLON:
		   return SDLK_COLON;
	   case KEY_SEMICOLON:
		   return SDLK_SEMICOLON;
	   case KEY_LESS:
		   return SDLK_LESS;
	   case KEY_EQUALS:
		   return SDLK_EQUALS;
	   case KEY_GREATER:
		   return SDLK_GREATER;
	   case KEY_QUESTION:
		   return SDLK_QUESTION;
	   case KEY_AT:
		   return SDLK_AT;
		   
	   case KEY_OPENBRACKET:
		   return SDLK_LEFTBRACKET;
	   case KEY_BACKSLASH:
		   return SDLK_BACKSLASH;
	   case KEY_CLOSEBRACKET:
		   return SDLK_RIGHTBRACKET;
	   case KEY_CARET:
		   return SDLK_CARET;
	   case KEY_UNDERSCORE:
		   return SDLK_UNDERSCORE;
	   case KEY_BACKQUOTE:
		   return SDLK_BACKQUOTE;
	   case KEY_A:
		   return SDLK_a;
	   case KEY_B:
		   return SDLK_b;
	   case KEY_C:
		   return SDLK_c;
	   case KEY_D:
		   return SDLK_d;
	   case KEY_E:
		   return SDLK_e;
	   case KEY_F:
		   return SDLK_f;
	   case KEY_G:
		   return SDLK_g;
	   case KEY_H:
		   return SDLK_h;
	   case KEY_I:
		   return SDLK_i;
	   case KEY_J:
		   return SDLK_j;
	   case KEY_K:
		   return SDLK_k;
	   case KEY_L:
		   return SDLK_l;
	   case KEY_M:
		   return SDLK_m;
	   case KEY_N:
		   return SDLK_n;
	   case KEY_O:
		   return SDLK_o;
	   case KEY_P:
		   return SDLK_p;
	   case KEY_Q:
		   return SDLK_q;
	   case KEY_R:
		   return SDLK_r;
	   case KEY_S:
		   return SDLK_s;
	   case KEY_T:
		   return SDLK_t;
	   case KEY_U:
		   return SDLK_u;
	   case KEY_V:
		   return SDLK_v;
	   case KEY_W:
		   return SDLK_w;
	   case KEY_X:
		   return SDLK_x;
	   case KEY_Y:
		   return SDLK_y;
	   case KEY_Z:
		   return SDLK_z;
	   case KEY_DELETE:
		   return SDLK_DELETE;

	   // International keyboard syms
	   case KEY_WORLD_0:
		   return SDLK_WORLD_0;
	   case KEY_WORLD_1:
		   return SDLK_WORLD_1;
	   case KEY_WORLD_2:
		   return SDLK_WORLD_2;
	   case KEY_WORLD_3:
		   return SDLK_WORLD_3;
	   case KEY_WORLD_4:
		   return SDLK_WORLD_4;
	   case KEY_WORLD_5:
		   return SDLK_WORLD_5;
	   case KEY_WORLD_6:
		   return SDLK_WORLD_6;
	   case KEY_WORLD_7:
		   return SDLK_WORLD_7;
	   case KEY_WORLD_8:
		   return SDLK_WORLD_8;
	   case KEY_WORLD_9:
		   return SDLK_WORLD_9;
	   case KEY_WORLD_10:
		   return SDLK_WORLD_10;
	   case KEY_WORLD_11:
		   return SDLK_WORLD_11;
	   case KEY_WORLD_12:
		   return SDLK_WORLD_12;
	   case KEY_WORLD_13:
		   return SDLK_WORLD_13;
	   case KEY_WORLD_14:
		   return SDLK_WORLD_14;
	   case KEY_WORLD_15:
		   return SDLK_WORLD_15;
	   case KEY_WORLD_16:
		   return SDLK_WORLD_16;
	   case KEY_WORLD_17:
		   return SDLK_WORLD_17;
	   case KEY_WORLD_18:
		   return SDLK_WORLD_18;
	   case KEY_WORLD_19:
		   return SDLK_WORLD_19;
	   case KEY_WORLD_20:
		   return SDLK_WORLD_20;
	   case KEY_WORLD_21:
		   return SDLK_WORLD_21;
	   case KEY_WORLD_22:
		   return SDLK_WORLD_22;
	   case KEY_WORLD_23:
		   return SDLK_WORLD_23;
	   case KEY_WORLD_24:
		   return SDLK_WORLD_24;
	   case KEY_WORLD_25:
		   return SDLK_WORLD_25;
	   case KEY_WORLD_26:
		   return SDLK_WORLD_26;
	   case KEY_WORLD_27:
		   return SDLK_WORLD_27;
	   case KEY_WORLD_28:
		   return SDLK_WORLD_28;
	   case KEY_WORLD_29:
		   return SDLK_WORLD_29;
	   case KEY_WORLD_30:
		   return SDLK_WORLD_30;
	   case KEY_WORLD_31:
		   return SDLK_WORLD_31;
	   case KEY_WORLD_32:
		   return SDLK_WORLD_32;
	   case KEY_WORLD_33:
		   return SDLK_WORLD_33;
	   case KEY_WORLD_34:
		   return SDLK_WORLD_34;
	   case KEY_WORLD_35:
		   return SDLK_WORLD_35;
	   case KEY_WORLD_36:
		   return SDLK_WORLD_36;
	   case KEY_WORLD_37:
		   return SDLK_WORLD_37;
	   case KEY_WORLD_38:
		   return SDLK_WORLD_38;
	   case KEY_WORLD_39:
		   return SDLK_WORLD_39;
	   case KEY_WORLD_40:
		   return SDLK_WORLD_40;
	   case KEY_WORLD_41:
		   return SDLK_WORLD_41;
	   case KEY_WORLD_42:
		   return SDLK_WORLD_42;
	   case KEY_WORLD_43:
		   return SDLK_WORLD_43;
	   case KEY_WORLD_44:
		   return SDLK_WORLD_44;
	   case KEY_WORLD_45:
		   return SDLK_WORLD_45;
	   case KEY_WORLD_46:
		   return SDLK_WORLD_46;
	   case KEY_WORLD_47:
		   return SDLK_WORLD_47;
	   case KEY_WORLD_48:
		   return SDLK_WORLD_48;
	   case KEY_WORLD_49:
		   return SDLK_WORLD_49;
	   case KEY_WORLD_50:
		   return SDLK_WORLD_50;
	   case KEY_WORLD_51:
		   return SDLK_WORLD_51;
	   case KEY_WORLD_52:
		   return SDLK_WORLD_52;
	   case KEY_WORLD_53:
		   return SDLK_WORLD_53;
	   case KEY_WORLD_54:
		   return SDLK_WORLD_54;
	   case KEY_WORLD_55:
		   return SDLK_WORLD_55;
	   case KEY_WORLD_56:
		   return SDLK_WORLD_56;
	   case KEY_WORLD_57:
		   return SDLK_WORLD_57;
	   case KEY_WORLD_58:
		   return SDLK_WORLD_58;
	   case KEY_WORLD_59:
		   return SDLK_WORLD_59;
	   case KEY_WORLD_60:
		   return SDLK_WORLD_60;
	   case KEY_WORLD_61:
		   return SDLK_WORLD_61;
	   case KEY_WORLD_62:
		   return SDLK_WORLD_62;
	   case KEY_WORLD_63:
		   return SDLK_WORLD_63;
	   case KEY_WORLD_64:
		   return SDLK_WORLD_64;
	   case KEY_WORLD_65:
		   return SDLK_WORLD_65;
	   case KEY_WORLD_66:
		   return SDLK_WORLD_66;
	   case KEY_WORLD_67:
		   return SDLK_WORLD_67;
	   case KEY_WORLD_68:
		   return SDLK_WORLD_68;
	   case KEY_WORLD_69:
		   return SDLK_WORLD_69;
	   case KEY_WORLD_70:
		   return SDLK_WORLD_70;
	   case KEY_WORLD_71:
		   return SDLK_WORLD_71;
	   case KEY_WORLD_72:
		   return SDLK_WORLD_72;
	   case KEY_WORLD_73:
		   return SDLK_WORLD_73;
	   case KEY_WORLD_74:
		   return SDLK_WORLD_74;
	   case KEY_WORLD_75:
		   return SDLK_WORLD_75;
	   case KEY_WORLD_76:
		   return SDLK_WORLD_76;
	   case KEY_WORLD_77:
		   return SDLK_WORLD_77;
	   case KEY_WORLD_78:
		   return SDLK_WORLD_78;
	   case KEY_WORLD_79:
		   return SDLK_WORLD_79;
	   case KEY_WORLD_80:
		   return SDLK_WORLD_80;
	   case KEY_WORLD_81:
		   return SDLK_WORLD_81;
	   case KEY_WORLD_82:
		   return SDLK_WORLD_82;
	   case KEY_WORLD_83:
		   return SDLK_WORLD_83;
	   case KEY_WORLD_84:
		   return SDLK_WORLD_84;
	   case KEY_WORLD_85:
		   return SDLK_WORLD_85;
	   case KEY_WORLD_86:
		   return SDLK_WORLD_86;
	   case KEY_WORLD_87:
		   return SDLK_WORLD_87;
	   case KEY_WORLD_88:
		   return SDLK_WORLD_88;
	   case KEY_WORLD_89:
		   return SDLK_WORLD_89;
	   case KEY_WORLD_90:
		   return SDLK_WORLD_90;
	   case KEY_WORLD_91:
		   return SDLK_WORLD_91;
	   case KEY_WORLD_92:
		   return SDLK_WORLD_92;
	   case KEY_WORLD_93:
		   return SDLK_WORLD_93;
	   case KEY_WORLD_94:
		   return SDLK_WORLD_94;
	   case KEY_WORLD_95:
		   return SDLK_WORLD_95;

	   // Numeric keypad
	   case KEY_KEYPAD0:
		   return SDLK_KP0;
	   case KEY_KEYPAD1:
		   return SDLK_KP1;
	   case KEY_KEYPAD2:
		   return SDLK_KP2;
	   case KEY_KEYPAD3:
		   return SDLK_KP3;
	   case KEY_KEYPAD4:
		   return SDLK_KP4;
	   case KEY_KEYPAD5:
		   return SDLK_KP5;
	   case KEY_KEYPAD6:
		   return SDLK_KP6;
	   case KEY_KEYPAD7:
		   return SDLK_KP7;
	   case KEY_KEYPAD8:
		   return SDLK_KP8;
	   case KEY_KEYPAD9:
		   return SDLK_KP9;
	   case KEY_KEYPAD_PERIOD:
		   return SDLK_KP_PERIOD;
	   case KEY_KEYPAD_DIVIDE:
		   return SDLK_KP_DIVIDE;
	   case KEY_KEYPAD_MULTIPLY:
		   return SDLK_KP_MULTIPLY;
	   case KEY_KEYPAD_MINUS:
		   return SDLK_KP_MINUS;
	   case KEY_KEYPAD_PLUS:
		   return SDLK_KP_PLUS;
	   case KEY_KEYPAD_ENTER:
		   return SDLK_KP_ENTER;
	   case KEY_KEYPAD_EQUALS:
		   return SDLK_KP_EQUALS;

	   // Arrows + Home/End pad
	   case KEY_UP:
		   return SDLK_UP;
	   case KEY_DOWN:
		   return SDLK_DOWN;
	   case KEY_RIGHT:
		   return SDLK_RIGHT;
	   case KEY_LEFT:
		   return SDLK_LEFT;
	   case KEY_INSERT:
		   return SDLK_INSERT;
	   case KEY_HOME:
		   return SDLK_HOME;
	   case KEY_END:
		   return SDLK_END;
	   case KEY_PAGEUP:
		   return SDLK_PAGEUP;
	   case KEY_PAGEDOWN:
		   return SDLK_PAGEDOWN;

	   // Function keys
	   case KEY_F1:
		   return SDLK_F1;
	   case KEY_F2:
		   return SDLK_F2;
	   case KEY_F3:
		   return SDLK_F3;
	   case KEY_F4:
		   return SDLK_F4;
	   case KEY_F5:
		   return SDLK_F5;
	   case KEY_F6:
		   return SDLK_F6;
	   case KEY_F7:
		   return SDLK_F7;
	   case KEY_F8:
		   return SDLK_F8;
	   case KEY_F9:
		   return SDLK_F9;
	   case KEY_F10:
		   return SDLK_F10;
	   case KEY_F11:
		   return SDLK_F11;
	   case KEY_F12:
		   return SDLK_F12;
	   case KEY_F13:
		   return SDLK_F13;
	   case KEY_F14:
		   return SDLK_F14;
	   case KEY_F15:
		   return SDLK_F15;

	   // Key state modifier keys
	   case KEY_NUMLOCK:
		   return SDLK_NUMLOCK;
	   case KEY_CAPSLOCK:
		   return SDLK_CAPSLOCK;
	   case KEY_SCROLLOCK:
		   return SDLK_SCROLLOCK;
	   case KEY_MODE:
		   return SDLK_MODE;
	   case KEY_COMPOSE:
		   return SDLK_COMPOSE;

	   // Miscellaneous function keys
	   case KEY_HELP:
		   return SDLK_HELP;
	   case KEY_PRINT:
		   return SDLK_PRINT;
	   case KEY_SYSREQ:
		   return SDLK_SYSREQ;
	   case KEY_BREAK:
		   return SDLK_BREAK;
	   case KEY_MENU:
		   return SDLK_MENU;
	   case KEY_POWER:
		   return SDLK_POWER;
	   case KEY_EURO:
		   return SDLK_EURO;
	   case KEY_UNDO:
		   return SDLK_UNDO;
      default:
         logprintf(LogConsumer::LogWarning, "Unknown inputCode detected: %d", inputCode);
         return SDLK_UNKNOWN;
   }
}
#endif


InputCode joystickButtonToInputCode(Joystick::Button button)
{
   switch(button)
   {
      case Joystick::Button1:
         return BUTTON_1;
      case Joystick::Button2:
         return BUTTON_2;
      case Joystick::Button3:
         return BUTTON_3;
      case Joystick::Button4:
         return BUTTON_4;
      case Joystick::Button5:
         return BUTTON_5;
      case Joystick::Button6:
         return BUTTON_6;
      case Joystick::Button7:
         return BUTTON_7;
      case Joystick::Button8:
         return BUTTON_8;
      case Joystick::ButtonStart:
         return BUTTON_START;
      case Joystick::ButtonBack:
         return BUTTON_BACK;
      case Joystick::ButtonDPadUp:
         return BUTTON_DPAD_UP;
      case Joystick::ButtonDPadDown:
         return BUTTON_DPAD_DOWN;
      case Joystick::ButtonDPadLeft:
         return BUTTON_DPAD_LEFT;
      case Joystick::ButtonDPadRight:
         return BUTTON_DPAD_RIGHT;
      case Joystick::Button9:
         return BUTTON_9;
      case Joystick::Button10:
         return BUTTON_10;
      case Joystick::Button11:
         return BUTTON_11;
      case Joystick::Button12:
         return BUTTON_12;
      default:
         return BUTTON_UNKNOWN;
   }
}


InputCode joyHatToInputCode(int hatDirectionMask)
{
   switch(hatDirectionMask)
   {
      case 1:
         return BUTTON_DPAD_UP;
      case 2:
         return BUTTON_DPAD_RIGHT;
      case 4:
         return BUTTON_DPAD_DOWN;
      case 8:
         return BUTTON_DPAD_LEFT;
      default:
         return BUTTON_UNKNOWN;
   }
}


Joystick::Button inputCodeToJoystickButton(InputCode inputCode)
{
   switch(inputCode)
   {
      case BUTTON_1:
         return Joystick::Button1;
      case BUTTON_2:
         return Joystick::Button2;
      case BUTTON_3:
         return Joystick::Button3;
      case BUTTON_4:
         return Joystick::Button4;
      case BUTTON_5:
         return Joystick::Button5;
      case BUTTON_6:
         return Joystick::Button6;
      case BUTTON_7:
         return Joystick::Button7;
      case BUTTON_8:
         return Joystick::Button8;
      case BUTTON_9:
         return Joystick::Button9;
      case BUTTON_10:
         return Joystick::Button10;
      case BUTTON_11:
         return Joystick::Button11;
      case BUTTON_12:
         return Joystick::Button12;
      case BUTTON_START:
         return Joystick::ButtonStart;
      case BUTTON_BACK:
         return Joystick::ButtonBack;
      case BUTTON_DPAD_UP:
         return Joystick::ButtonDPadUp;
      case BUTTON_DPAD_DOWN:
         return Joystick::ButtonDPadDown;
      case BUTTON_DPAD_LEFT:
         return Joystick::ButtonDPadLeft;
      case BUTTON_DPAD_RIGHT:
         return Joystick::ButtonDPadRight;
      default:
         return Joystick::ButtonUnknown;
   }
}


// We'll also treat controller buttons like simulated keystrokes
bool isControllerButton(InputCode inputCode)
{
   if (inputCode == BUTTON_1 || inputCode == BUTTON_2 || inputCode == BUTTON_3 ||
       inputCode == BUTTON_4 || inputCode == BUTTON_5 || inputCode == BUTTON_6 ||
       inputCode == BUTTON_7 || inputCode == BUTTON_8 || inputCode == BUTTON_9 ||
       inputCode == BUTTON_10 || inputCode == BUTTON_11 || inputCode == BUTTON_12 ||
       inputCode == BUTTON_BACK || inputCode == BUTTON_START)
      return true;
   else
      return false;
}

// Translate an InputCode into a string name, primarily used
// for displaying keys in help and during rebind mode, and
// also when storing key bindings in INI files
const char *inputCodeToString(InputCode inputCode)
{
   switch (inputCode)
   {
   case KEY_BACKSPACE:
         return "Backspace";
   case KEY_DELETE:
         return "Del";
   case KEY_TAB:
         return "Tab";
   case KEY_ENTER:
         return "Enter";
   case KEY_ESCAPE:
         return "Esc";
   case KEY_SPACE:
         return "Space";
   case KEY_0:
         return "0";
   case KEY_1:
         return "1";
   case KEY_2:
         return "2";
   case KEY_3:
         return "3";
   case KEY_4:
         return "4";
   case KEY_5:
         return "5";
   case KEY_6:
         return "6";
   case KEY_7:
         return "7";
   case KEY_8:
         return "8";
   case KEY_9:
         return "9";
   case KEY_A:
         return "A";
   case KEY_B:
         return "B";
   case KEY_C:
         return "C";
   case KEY_D:
         return "D";
   case KEY_E:
         return "E";
   case KEY_F:
         return "F";
   case KEY_G:
         return "G";
   case KEY_H:
         return "H";
   case KEY_I:
         return "I";
   case KEY_J:
         return "J";
   case KEY_K:
         return "K";
   case KEY_L:
         return "L";
   case KEY_M:
         return "M";
   case KEY_N:
         return "N";
   case KEY_O:
         return "O";
   case KEY_P:
         return "P";
   case KEY_Q:
         return "Q";
   case KEY_R:
         return "R";
   case KEY_S:
         return "S";
   case KEY_T:
         return "T";
   case KEY_U:
         return "U";
   case KEY_V:
         return "V";
   case KEY_W:
         return "W";
   case KEY_X:
         return "X";
   case KEY_Y:
         return "Y";
   case KEY_Z:
         return "Z";
   case KEY_TILDE:
         return "~";
   case KEY_MINUS:
         return "-";
   case KEY_EQUALS:
         return "=";
   case KEY_OPENBRACKET:
         return "[";
   case KEY_CLOSEBRACKET:
         return "]";
   case KEY_BACKSLASH:
         return "\\";
   case KEY_SEMICOLON:
         return ";";
   case KEY_QUOTE:
         return "'";
   case KEY_COMMA:
         return ",";
   case KEY_PERIOD:
         return ".";
   case KEY_SLASH:
         return "/";
   case KEY_PAGEUP:
         return "Page Up";
   case KEY_PAGEDOWN:
         return "Page Down";
   case KEY_END:
         return "End";
   case KEY_HOME:
         return "Home";
   case KEY_LEFT:
         return "Left Arrow";
   case KEY_UP:
         return "Up Arrow";
   case KEY_RIGHT:
         return "Right Arrow";
   case KEY_DOWN:
         return "Down Arrow";
   case KEY_INSERT:
         return "Insert";
   case KEY_F1:
         return "F1";
   case KEY_F2:
         return "F2";
   case KEY_F3:
         return "F3";
   case KEY_F4:
         return "F4";
   case KEY_F5:
         return "F5";
   case KEY_F6:
         return "F6";
   case KEY_F7:
         return "F7";
   case KEY_F8:
         return "F8";
   case KEY_F9:
         return "F9";
   case KEY_F10:
         return "F10";
   case KEY_F11:
         return "F11";
   case KEY_F12:
         return "F12";
   case KEY_SHIFT:
         return "Shift";
   case KEY_ALT:
         return "Alt";
   case KEY_CTRL:
         return "Ctrl";
   case KEY_META:
         return "Meta";
   case KEY_SUPER:
         return "Super";
   case MOUSE_LEFT:
      return "Left-mouse";
   case MOUSE_MIDDLE:
      return "Middle-mouse";
   case MOUSE_RIGHT:
      return "Right-mouse";
   case MOUSE_WHEEL_UP:
      return "Mouse wheel up";
   case MOUSE_WHEEL_DOWN:
      return "Mouse wheel down";
   case BUTTON_1:
      return "Button 1";
   case BUTTON_2:
      return "Button 2";
   case BUTTON_3:
      return "Button 3";
   case BUTTON_4:
      return "Button 4";
   case BUTTON_5:
      return "Button 5";
   case BUTTON_6:
      return "Button 6";
   case BUTTON_7:
      return "Button 7";
   case BUTTON_8:
      return "Button 8";
   case BUTTON_9:
      return "Button 9";
   case BUTTON_10:
      return "Button 10";
   case BUTTON_11:
      return "Button 11";
   case BUTTON_12:
      return "Button 12";
   case BUTTON_BACK:
      return "Back";
   case BUTTON_START:
      return "Start";
   case BUTTON_DPAD_UP:
      return "DPad Up";
   case BUTTON_DPAD_DOWN:
      return "DPad Down";
   case BUTTON_DPAD_LEFT:
      return "DPad Left";
   case BUTTON_DPAD_RIGHT:
      return "DPad Right";
   case STICK_1_LEFT:
      return "Stick 1 Left";
   case STICK_1_RIGHT:
      return "Stick 1 Right";
   case STICK_1_UP:
      return "Stick 1 Up";
   case STICK_1_DOWN:
      return "Stick 1 Down";
   case STICK_2_LEFT:
      return "Stick 2 Left";
   case STICK_2_RIGHT:
      return "Stick 2 Right";
   case STICK_2_UP:
      return "Stick 2 Up";
   case STICK_2_DOWN:
      return "Stick 2 Down";
   case MOUSE:
      return "Mouse";
   case LEFT_JOYSTICK:
      return "Left joystick";
   case RIGHT_JOYSTICK:
      return "Right joystick";
   case KEY_CTRL_M:
      return "Ctrl-M";
   case KEY_CTRL_Q:
      return "Ctrl-Q";
   case KEY_CTRL_S:
      return "Ctrl-S";
   case KEY_BACKQUOTE:
      return "`";
   case KEY_MENU:
      return "Menu";
   case KEY_KEYPAD_DIVIDE:
      return "Keypad /";
   case KEY_KEYPAD_MULTIPLY:
      return "Keypad *";
   case KEY_KEYPAD_MINUS:
      return "Keypad -";
   case KEY_KEYPAD_PLUS:
      return "Keypad +";
   case KEY_PRINT:
      return "PrntScrn";
   case KEY_PAUSE:
      return "Pause";
   case KEY_SCROLLOCK:
      return "ScrollLock";
   case KEY_KEYPAD1:
      return "Keypad 1";
   case KEY_KEYPAD2:
      return "Keypad 2";
   case KEY_KEYPAD3:
      return "Keypad 3";
   case KEY_KEYPAD4:
      return "Keypad 4";
   case KEY_KEYPAD5:
      return "Keypad 5";
   case KEY_KEYPAD6:
      return "Keypad 6";
   case KEY_KEYPAD7:
      return "Keypad 7";
   case KEY_KEYPAD8:
      return "Keypad 8";
   case KEY_KEYPAD9:
      return "Keypad 9";
   case KEY_KEYPAD0:
      return "Keypad 0";
   case KEY_KEYPAD_PERIOD:
      return "Keypad .";
   case KEY_KEYPAD_ENTER:
      return "Keypad Enter";

   default:
//      logprintf("undefined inputCode number: %d", inputCode);
      return "Undefined input";
   }
}

// Translate from a string key name into a InputCode
// (primarily for loading key bindings from INI files)
InputCode stringToInputCode(const char *inputName)
{
   if (stricmp(inputName, "Backspace") == 0)
      return KEY_BACKSPACE;
   else if (stricmp(inputName, "Del") == 0)
      return KEY_DELETE;
   else if (stricmp(inputName, "Tab") == 0)
      return KEY_TAB;
   else if (stricmp(inputName, "Enter") == 0)
      return KEY_ENTER;
   else if (stricmp(inputName, "Esc") == 0)
      return KEY_ESCAPE;
   else if (stricmp(inputName, "Space") == 0)
      return KEY_SPACE;
   else if (stricmp(inputName, "0") == 0)
      return KEY_0;
   else if (stricmp(inputName, "1") == 0)
      return KEY_1;
   else if (stricmp(inputName, "2") == 0)
      return KEY_2;
   else if (stricmp(inputName, "3") == 0)
      return KEY_3;
   else if (stricmp(inputName, "4") == 0)
      return KEY_4;
   else if (stricmp(inputName, "5") == 0)
      return KEY_5;
   else if (stricmp(inputName, "6") == 0)
      return KEY_6;
   else if (stricmp(inputName, "7") == 0)
      return KEY_7;
   else if (stricmp(inputName, "8") == 0)
      return KEY_8;
   else if (stricmp(inputName, "9") == 0)
      return KEY_9;
   else if (stricmp(inputName, "A") == 0)
      return KEY_A;
   else if (stricmp(inputName, "B") == 0)
      return KEY_B;
   else if (stricmp(inputName, "C") == 0)
      return KEY_C;
   else if (stricmp(inputName, "D") == 0)
      return KEY_D;
   else if (stricmp(inputName, "E") == 0)
      return KEY_E;
   else if (stricmp(inputName, "F") == 0)
      return KEY_F;
   else if (stricmp(inputName, "G") == 0)
      return KEY_G;
   else if (stricmp(inputName, "H") == 0)
      return KEY_H;
   else if (stricmp(inputName, "I") == 0)
      return KEY_I;
   else if (stricmp(inputName, "J") == 0)
      return KEY_J;
   else if (stricmp(inputName, "K") == 0)
      return KEY_K;
   else if (stricmp(inputName, "L") == 0)
      return KEY_L;
   else if (stricmp(inputName, "M") == 0)
      return KEY_M;
   else if (stricmp(inputName, "N") == 0)
      return KEY_N;
   else if (stricmp(inputName, "O") == 0)
      return KEY_O;
   else if (stricmp(inputName, "P") == 0)
      return KEY_P;
   else if (stricmp(inputName, "Q") == 0)
      return KEY_Q;
   else if (stricmp(inputName, "R") == 0)
      return KEY_R;
   else if (stricmp(inputName, "S") == 0)
      return KEY_S;
   else if (stricmp(inputName, "T") == 0)
      return KEY_T;
   else if (stricmp(inputName, "U") == 0)
      return KEY_U;
   else if (stricmp(inputName, "V") == 0)
      return KEY_V;
   else if (stricmp(inputName, "W") == 0)
      return KEY_W;
   else if (stricmp(inputName, "X") == 0)
      return KEY_X;
   else if (stricmp(inputName, "Y") == 0)
      return KEY_Y;
   else if (stricmp(inputName, "Z") == 0)
      return KEY_Z;
   else if (stricmp(inputName, "~") == 0)
      return KEY_TILDE;
   else if (stricmp(inputName, "-") == 0)
      return KEY_MINUS;
   else if (stricmp(inputName, "=") == 0)
      return KEY_EQUALS;
   else if (stricmp(inputName, "[") == 0)
      return KEY_OPENBRACKET;
   else if (stricmp(inputName, "]") == 0)
      return KEY_CLOSEBRACKET;
   else if (stricmp(inputName, "\\") == 0)
      return KEY_BACKSLASH;
   else if (stricmp(inputName, ";") == 0)
      return KEY_SEMICOLON;
   else if (stricmp(inputName, "'") == 0)
      return KEY_QUOTE;
   else if (stricmp(inputName, ",") == 0)
      return KEY_COMMA;
   else if (stricmp(inputName, ".") == 0)
      return KEY_PERIOD;
   else if (stricmp(inputName, "/") == 0)
      return KEY_SLASH;
   else if (stricmp(inputName, "Page Up") == 0)
      return KEY_PAGEUP;
   else if (stricmp(inputName, "Page Down") == 0)
      return KEY_PAGEDOWN;
   else if (stricmp(inputName, "End") == 0)
      return KEY_END;
   else if (stricmp(inputName, "Home") == 0)
      return KEY_HOME;
   else if (stricmp(inputName, "Left Arrow") == 0)
      return KEY_LEFT;
   else if (stricmp(inputName, "Up Arrow") == 0)
      return KEY_UP;
   else if (stricmp(inputName, "Right Arrow") == 0)
      return KEY_RIGHT;
   else if (stricmp(inputName, "Down Arrow") == 0)
      return KEY_DOWN;
   else if (stricmp(inputName, "Insert") == 0)
      return KEY_INSERT;
   else if (stricmp(inputName, "F1") == 0)
      return KEY_F1;
   else if (stricmp(inputName, "F2") == 0)
      return KEY_F2;
   else if (stricmp(inputName, "F3") == 0)
      return KEY_F3;
   else if (stricmp(inputName, "F4") == 0)
      return KEY_F4;
   else if (stricmp(inputName, "F5") == 0)
      return KEY_F5;
   else if (stricmp(inputName, "F6") == 0)
      return KEY_F6;
   else if (stricmp(inputName, "F7") == 0)
      return KEY_F7;
   else if (stricmp(inputName, "F8") == 0)
      return KEY_F8;
   else if (stricmp(inputName, "F9") == 0)
      return KEY_F9;
   else if (stricmp(inputName, "F10") == 0)
      return KEY_F10;
   else if (stricmp(inputName, "F11") == 0)
      return KEY_F11;
   else if (stricmp(inputName, "F12") == 0)
      return KEY_F12;
   else if (stricmp(inputName, "Shift") == 0)
      return KEY_SHIFT;
   else if (stricmp(inputName, "Alt") == 0)
      return KEY_ALT;
   else if (stricmp(inputName, "Ctrl") == 0)
      return KEY_CTRL;
   else if (stricmp(inputName, "Left-mouse") == 0)
      return MOUSE_LEFT;
   else if (stricmp(inputName, "Middle-mouse") == 0)
      return MOUSE_MIDDLE;
   else if (stricmp(inputName, "Right-mouse") == 0)
      return MOUSE_RIGHT;
   else if (stricmp(inputName, "Button 1") == 0)
      return BUTTON_1;
   else if (stricmp(inputName, "Button 2") == 0)
      return BUTTON_2;
   else if (stricmp(inputName, "Button 3") == 0)
      return BUTTON_3;
   else if (stricmp(inputName, "Button 4") == 0)
      return BUTTON_4;
   else if (stricmp(inputName, "Button 5") == 0)
      return BUTTON_5;
   else if (stricmp(inputName, "Button 6") == 0)
      return BUTTON_6;
   else if (stricmp(inputName, "Button 7") == 0)
      return BUTTON_7;
   else if (stricmp(inputName, "Button 8") == 0)
      return BUTTON_8;
   else if (stricmp(inputName, "Button 9") == 0)
      return BUTTON_9;
   else if (stricmp(inputName, "Button 10") == 0)
      return BUTTON_10;
   else if (stricmp(inputName, "Button 11") == 0)
      return BUTTON_11;
   else if (stricmp(inputName, "Button 12") == 0)
      return BUTTON_12;
   else if (stricmp(inputName, "Back") == 0)
      return BUTTON_BACK;
   else if (stricmp(inputName, "Start") == 0)
      return BUTTON_START;
   else if (stricmp(inputName, "DPad Up") == 0)
      return BUTTON_DPAD_UP;
   else if (stricmp(inputName, "DPad Down") == 0)
      return BUTTON_DPAD_DOWN;
   else if (stricmp(inputName, "DPad Left") == 0)
      return BUTTON_DPAD_LEFT;
   else if (stricmp(inputName, "DPad Right") == 0)
      return BUTTON_DPAD_RIGHT;
   else if (stricmp(inputName, "Stick 1 Left") == 0)
      return STICK_1_LEFT;
   else if (stricmp(inputName, "Stick 1 Right") == 0)
      return STICK_1_RIGHT;
   else if (stricmp(inputName, "Stick 1 Up") == 0)
      return STICK_1_UP;
   else if (stricmp(inputName, "Stick 1 Down") == 0)
      return STICK_1_DOWN;
   else if (stricmp(inputName, "Stick 2 Left") == 0)
      return STICK_2_LEFT;
   else if (stricmp(inputName, "Stick 2 Right") == 0)
      return STICK_2_RIGHT;
   else if (stricmp(inputName, "Stick 2 Up") == 0)
      return STICK_2_UP;
   else if (stricmp(inputName, "Stick 2 Down") == 0)
      return STICK_2_DOWN;
   else if (stricmp(inputName, "Mouse") == 0)
      return MOUSE;
   else
      return KEY_UNKNOWN;
}

}

#ifdef __cplusplus
extern "C" {
#endif
// Provide access to getInputCodeState from C code.
// Has to be outside the namespace declaration because C doesn't use namespaces.
int getInputCodeState_c(int inputCode) { return Zap::getInputCodeState((Zap::InputCode)inputCode); }
#ifdef __cplusplus
}
#endif
