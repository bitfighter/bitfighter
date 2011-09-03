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

#include "keyCode.h"

#include "../tnl/tnlJournal.h"
#include "../tnl/tnlLog.h"         // For logprintf

#include "zapjournal.h"    // For journaling support

#include <ctype.h>

#ifndef ZAP_DEDICATED
#include "SDL/SDL.h"
#endif

using namespace TNL;

namespace Zap
{

static bool keyIsDown[MAX_KEYS];

// We have two sets of keys defined -- one for when we're playing
// with a keyboard, and one for joystick play
KeyCode keySELWEAP1[2];       // Select weapon 1
KeyCode keySELWEAP2[2];       // Select weapon 2
KeyCode keySELWEAP3[2];       // Select weapon 3
KeyCode keyADVWEAP[2];        // Pick next weapon
KeyCode keyCMDRMAP[2];        // Toggle commander's map
KeyCode keyTEAMCHAT[2];       // Send team chat message
KeyCode keyGLOBCHAT[2];       // Send global chat message
KeyCode keyQUICKCHAT[2];      // Enter QuickChat mode
KeyCode keyCMDCHAT[2];        // Go directly to command mode, bypassing chat
KeyCode keyLOADOUT[2];        // Enter Loadout mode
KeyCode keyMOD1[2];           // Activate module 1
KeyCode keyMOD2[2];           // Activate module 2
KeyCode keyFIRE[2];           // Fire
KeyCode keyDROPITEM[2];       // Drop flag or other item
KeyCode keyTOGVOICE[2];       // Toggle voice chat
KeyCode keyUP[2];             // Move ship
KeyCode keyDOWN[2];
KeyCode keyLEFT[2];
KeyCode keyRIGHT[2];
KeyCode keySCRBRD[2];         // Show scoreboard

// These keys are constant, regardless of mode, and can't be changed by the user
// Define these here to ensure they can be used for defining menus during initialization
// They really don't belong here, but what to do... what to do.
KeyCode keyHELP = KEY_F1;              // Display help
KeyCode keyOUTGAMECHAT = KEY_F5;       // Out of game chat
KeyCode keyFPS = KEY_F6;               // Show FPS display
KeyCode keyDIAG = KEY_F7;              // Show diagnostic overlay
KeyCode keyMISSION = KEY_F2;           // Show current mission info


// Initialize state of keys... assume none are depressed
void resetKeyStates()
{
   for (int i = 0; i < MAX_KEYS; i++)
      keyIsDown[i] = false;
}

// Prints list of any keys that are down, for debugging purposes
void dumpKeyStates()
{
  for (int i = 0; i < MAX_KEYS; i++)
     if(keyIsDown[i])
        logprintf("Key %s down", keyCodeToString((KeyCode) i));
}

// Set state of a key as Up (false) or Down (true)
void setKeyState(KeyCode keyCode, bool state)
{
   keyIsDown[(int) keyCode] = state;
}

// Returns true of key is down, false if it is up
bool getKeyState(KeyCode keyCode)
{
//   logprintf("State: key %d is %s", keyCode, keyIsDown[(int) keyCode] ? "down" : "up");
      return keyIsDown[(int) keyCode];
}


bool isPrintable(char c)
{
   return c >= 32 && c <= 126;
}


// If there is a printable ASCII code for the pressed key, return it
// Filter out some know spurious keystrokes
char keyToAscii(int unicode, KeyCode keyCode)
{
   //if(keyCode == KEY_UP || keyCode == KEY_DOWN || keyCode == KEY_LEFT || keyCode == KEY_RIGHT)
   //   return 0;

   if((unicode & 0xFF80) != 0) 
      return 0;

   char ch = unicode & 0x7F;

   return isPrintable(ch) ? ch : 0;
}


#ifndef ZAP_DEDICATED
// Translate SDL standard keys to our KeyCodes
KeyCode sdlKeyToKeyCode(int key)
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


S32 keyCodeToSDLKey(KeyCode keyCode)
{
   switch(keyCode)
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
         logprintf(LogConsumer::LogWarning, "Unknown keyCode detected: %d", keyCode);
         return SDLK_UNKNOWN;
   }
}
#endif


KeyCode joyButtonToKeyCode(int buttonIndex)
{
   switch(buttonIndex)
   {
      case 0:
         return BUTTON_1;
      case 1:
         return BUTTON_2;
      case 2:
         return BUTTON_3;
      case 3:
         return BUTTON_4;
      case 4:
         return BUTTON_5;
      case 5:
         return BUTTON_6;
      case 6:
         return BUTTON_7;
      case 7:
         return BUTTON_8;
      case 8:
         return BUTTON_START;
      case 9:
         return BUTTON_BACK;
      default:
         return BUTTON_UNKNOWN;
   }
}


KeyCode joyHatToKeyCode(int hatDirectionMask)
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


S32 keyCodeToButtonIndex(KeyCode keyCode)
{
   switch(keyCode)
   {
      case BUTTON_1:
         return 0;
      case BUTTON_2:
         return 1;
      case BUTTON_3:
         return 2;
      case BUTTON_4:
         return 3;
      case BUTTON_5:
         return 4;
      case BUTTON_6:
         return 5;
      case BUTTON_7:
         return 6;
      case BUTTON_8:
         return 7;
      case BUTTON_START:
         return 8;
      case BUTTON_BACK:
         return 9;
      case BUTTON_DPAD_UP:
         return 10;
      case BUTTON_DPAD_DOWN:
         return 11;
      case BUTTON_DPAD_LEFT:
         return 12;
      case BUTTON_DPAD_RIGHT:
         return 13;
      default:
         return 9999;
   }
}


// We'll also treat controller buttons like simulated keystrokes
bool isControllerButton(KeyCode keyCode)
{
   if (keyCode == BUTTON_1 || keyCode == BUTTON_2 || keyCode == BUTTON_3 ||
       keyCode == BUTTON_4 || keyCode == BUTTON_5 || keyCode == BUTTON_6 ||
       keyCode == BUTTON_7 || keyCode == BUTTON_8 ||
       keyCode == BUTTON_BACK || keyCode == BUTTON_START)
      return true;
   else
      return false;
}

// Translate a KeyCode into a string name, primarily used
// for displaying keys in help and during rebind mode, and
// also when storing key bindings in INI files
const char *keyCodeToString(KeyCode keyCode)
{
   switch (keyCode)
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
         return";";
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
   default:
//      logprintf("undefined keycode number: %d", keyCode);
      return "Undefined key";
   }
}

// Translate from a string key name into a KeyCode
// (primarily for loading key bindings from INI files)
KeyCode stringToKeyCode(const char *keyname)
{
   if (stricmp(keyname, "Backspace") == 0)
      return KEY_BACKSPACE;
   else if (stricmp(keyname, "Del") == 0)
      return KEY_DELETE;
   else if (stricmp(keyname, "Tab") == 0)
      return KEY_TAB;
   else if (stricmp(keyname, "Enter") == 0)
      return KEY_ENTER;
   else if (stricmp(keyname, "Esc") == 0)
      return KEY_ESCAPE;
   else if (stricmp(keyname, "Space") == 0)
      return KEY_SPACE;
   else if (stricmp(keyname, "0") == 0)
      return KEY_0;
   else if (stricmp(keyname, "1") == 0)
      return KEY_1;
   else if (stricmp(keyname, "2") == 0)
      return KEY_2;
   else if (stricmp(keyname, "3") == 0)
      return KEY_3;
   else if (stricmp(keyname, "4") == 0)
      return KEY_4;
   else if (stricmp(keyname, "5") == 0)
      return KEY_5;
   else if (stricmp(keyname, "6") == 0)
      return KEY_6;
   else if (stricmp(keyname, "7") == 0)
      return KEY_7;
   else if (stricmp(keyname, "8") == 0)
      return KEY_8;
   else if (stricmp(keyname, "9") == 0)
      return KEY_9;
   else if (stricmp(keyname, "A") == 0)
      return KEY_A;
   else if (stricmp(keyname, "B") == 0)
      return KEY_B;
   else if (stricmp(keyname, "C") == 0)
      return KEY_C;
   else if (stricmp(keyname, "D") == 0)
      return KEY_D;
   else if (stricmp(keyname, "E") == 0)
      return KEY_E;
   else if (stricmp(keyname, "F") == 0)
      return KEY_F;
   else if (stricmp(keyname, "G") == 0)
      return KEY_G;
   else if (stricmp(keyname, "H") == 0)
      return KEY_H;
   else if (stricmp(keyname, "I") == 0)
      return KEY_I;
   else if (stricmp(keyname, "J") == 0)
      return KEY_J;
   else if (stricmp(keyname, "K") == 0)
      return KEY_K;
   else if (stricmp(keyname, "L") == 0)
      return KEY_L;
   else if (stricmp(keyname, "M") == 0)
      return KEY_M;
   else if (stricmp(keyname, "N") == 0)
      return KEY_N;
   else if (stricmp(keyname, "O") == 0)
      return KEY_O;
   else if (stricmp(keyname, "P") == 0)
      return KEY_P;
   else if (stricmp(keyname, "Q") == 0)
      return KEY_Q;
   else if (stricmp(keyname, "R") == 0)
      return KEY_R;
   else if (stricmp(keyname, "S") == 0)
      return KEY_S;
   else if (stricmp(keyname, "T") == 0)
      return KEY_T;
   else if (stricmp(keyname, "U") == 0)
      return KEY_U;
   else if (stricmp(keyname, "V") == 0)
      return KEY_V;
   else if (stricmp(keyname, "W") == 0)
      return KEY_W;
   else if (stricmp(keyname, "X") == 0)
      return KEY_X;
   else if (stricmp(keyname, "Y") == 0)
      return KEY_Y;
   else if (stricmp(keyname, "Z") == 0)
      return KEY_Z;
   else if (stricmp(keyname, "~") == 0)
      return KEY_TILDE;
   else if (stricmp(keyname, "-") == 0)
      return KEY_MINUS;
   else if (stricmp(keyname, "=") == 0)
      return KEY_EQUALS;
   else if (stricmp(keyname, "[") == 0)
      return KEY_OPENBRACKET;
   else if (stricmp(keyname, "]") == 0)
      return KEY_CLOSEBRACKET;
   else if (stricmp(keyname, "\\") == 0)
      return KEY_BACKSLASH;
   else if (stricmp(keyname, ";") == 0)
      return KEY_SEMICOLON;
   else if (stricmp(keyname, "'") == 0)
      return KEY_QUOTE;
   else if (stricmp(keyname, ",") == 0)
      return KEY_COMMA;
   else if (stricmp(keyname, ".") == 0)
      return KEY_PERIOD;
   else if (stricmp(keyname, "/") == 0)
      return KEY_SLASH;
   else if (stricmp(keyname, "Page Up") == 0)
      return KEY_PAGEUP;
   else if (stricmp(keyname, "Page Down") == 0)
      return KEY_PAGEDOWN;
   else if (stricmp(keyname, "End") == 0)
      return KEY_END;
   else if (stricmp(keyname, "Home") == 0)
      return KEY_HOME;
   else if (stricmp(keyname, "Left Arrow") == 0)
      return KEY_LEFT;
   else if (stricmp(keyname, "Up Arrow") == 0)
      return KEY_UP;
   else if (stricmp(keyname, "Right Arrow") == 0)
      return KEY_RIGHT;
   else if (stricmp(keyname, "Down Arrow") == 0)
      return KEY_DOWN;
   else if (stricmp(keyname, "Insert") == 0)
      return KEY_INSERT;
   else if (stricmp(keyname, "F1") == 0)
      return KEY_F1;
   else if (stricmp(keyname, "F2") == 0)
      return KEY_F2;
   else if (stricmp(keyname, "F3") == 0)
      return KEY_F3;
   else if (stricmp(keyname, "F4") == 0)
      return KEY_F4;
   else if (stricmp(keyname, "F5") == 0)
      return KEY_F5;
   else if (stricmp(keyname, "F6") == 0)
      return KEY_F6;
   else if (stricmp(keyname, "F7") == 0)
      return KEY_F7;
   else if (stricmp(keyname, "F8") == 0)
      return KEY_F8;
   else if (stricmp(keyname, "F9") == 0)
      return KEY_F9;
   else if (stricmp(keyname, "F10") == 0)
      return KEY_F10;
   else if (stricmp(keyname, "F11") == 0)
      return KEY_F11;
   else if (stricmp(keyname, "F12") == 0)
      return KEY_F12;
   else if (stricmp(keyname, "Shift") == 0)
      return KEY_SHIFT;
   else if (stricmp(keyname, "Alt") == 0)
      return KEY_ALT;
   else if (stricmp(keyname, "Ctrl") == 0)
      return KEY_CTRL;
   else if (stricmp(keyname, "Left-mouse") == 0)
      return MOUSE_LEFT;
   else if (stricmp(keyname, "Middle-mouse") == 0)
      return MOUSE_MIDDLE;
   else if (stricmp(keyname, "Right-mouse") == 0)
      return MOUSE_RIGHT;
   else if (stricmp(keyname, "Button 1") == 0)
      return BUTTON_1;
   else if (stricmp(keyname, "Button 2") == 0)
      return BUTTON_2;
   else if (stricmp(keyname, "Button 3") == 0)
      return BUTTON_3;
   else if (stricmp(keyname, "Button 4") == 0)
      return BUTTON_4;
   else if (stricmp(keyname, "Button 5") == 0)
      return BUTTON_5;
   else if (stricmp(keyname, "Button 6") == 0)
      return BUTTON_6;
   else if (stricmp(keyname, "Button 7") == 0)
      return BUTTON_7;
   else if (stricmp(keyname, "Button 8") == 0)
      return BUTTON_8;
   else if (stricmp(keyname, "Button 9") == 0)     // An alias for "Back"
      return BUTTON_BACK;
   else if (stricmp(keyname, "Back") == 0)
      return BUTTON_BACK;
   else if (stricmp(keyname, "Button 10") == 0)    // An alias for "Start"
      return BUTTON_START;
   else if (stricmp(keyname, "Start") == 0)
      return BUTTON_START;
   else if (stricmp(keyname, "DPad Up") == 0)
      return BUTTON_DPAD_UP;
   else if (stricmp(keyname, "DPad Down") == 0)
      return BUTTON_DPAD_DOWN;
   else if (stricmp(keyname, "DPad Left") == 0)
      return BUTTON_DPAD_LEFT;
   else if (stricmp(keyname, "DPad Right") == 0)
      return BUTTON_DPAD_RIGHT;
   else if (stricmp(keyname, "Stick 1 Left") == 0)
      return STICK_1_LEFT;
   else if (stricmp(keyname, "Stick 1 Right") == 0)
      return STICK_1_RIGHT;
   else if (stricmp(keyname, "Stick 1 Up") == 0)
      return STICK_1_UP;
   else if (stricmp(keyname, "Stick 1 Down") == 0)
      return STICK_1_DOWN;
   else if (stricmp(keyname, "Stick 2 Left") == 0)
      return STICK_2_LEFT;
   else if (stricmp(keyname, "Stick 2 Right") == 0)
      return STICK_2_RIGHT;
   else if (stricmp(keyname, "Stick 2 Up") == 0)
      return STICK_2_UP;
   else if (stricmp(keyname, "Stick 2 Down") == 0)
      return STICK_2_DOWN;
   else if (stricmp(keyname, "Mouse") == 0)
      return MOUSE;
   else
      return KEY_UNKNOWN;
}

}

#ifdef __cplusplus
extern "C" {
#endif
// Provide access to getKeyState from C code.
// Has to be outside the namespace declaration because C doesn't use namespaces.
int getKeyState_c(int keyCode) { return Zap::getKeyState((Zap::KeyCode)keyCode); }
#ifdef __cplusplus
}
#endif
