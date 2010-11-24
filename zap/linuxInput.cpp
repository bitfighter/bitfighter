//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo relased for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#ifndef ZAP_DEDICATED

#include <X11/Xlib.h>
#include <X11/keysym.h>

#endif

#include "input.h"

namespace Zap
{

bool gJoystickInit = false;

#ifndef ZAP_DEDICATED
static Display *Xdisplay = XOpenDisplay(NULL);
#endif

void getModifierState(bool &shiftDown, bool &controlDown, bool &altDown)
{
#ifndef ZAP_DEDICATED
   char key_map_stat[32];
   XQueryKeymap(Xdisplay, key_map_stat);
   
   // This can most definitely be simplified
   altDown = (((key_map_stat[XKeysymToKeycode(Xdisplay,XK_Alt_L) >> 3] >> (XKeysymToKeycode(Xdisplay,XK_Alt_L) & 7)) & 1) ||
      ((key_map_stat[XKeysymToKeycode(Xdisplay,XK_Alt_R) >> 3] >> (XKeysymToKeycode(Xdisplay,XK_Alt_R) & 7)) & 1));

   shiftDown = (((key_map_stat[XKeysymToKeycode(Xdisplay,XK_Shift_L) >> 3] >> (XKeysymToKeycode(Xdisplay,XK_Shift_L) & 7)) & 1) ||
      ((key_map_stat[XKeysymToKeycode(Xdisplay,XK_Shift_R) >> 3] >> (XKeysymToKeycode(Xdisplay,XK_Shift_R) & 7)) & 1));

   controlDown = (((key_map_stat[XKeysymToKeycode(Xdisplay,XK_Control_L) >> 3] >> (XKeysymToKeycode(Xdisplay,XK_Control_L) & 7)) & 1) ||
      ((key_map_stat[XKeysymToKeycode(Xdisplay,XK_Control_R) >> 3] >> (XKeysymToKeycode(Xdisplay,XK_Control_R) & 7)) & 1));
#endif
}

void InitJoystick()
{
}

bool ReadJoystick(F32 axes[MaxJoystickAxes], U32 &buttonMask, U32 &hatMask)
{
   return false;
}

const char *GetJoystickName()
{
   return "";     // Apparently no joystick autodetect in Linux...
}

void ShutdownJoystick()
{
}

};

