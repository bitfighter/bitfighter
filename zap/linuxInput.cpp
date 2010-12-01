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

#include "input.h"

#ifndef ZAP_DEDICATED

#include <X11/Xlib.h>
#include <X11/keysym.h>


//some joystick code was taken from http://coding.derkeiler.com/Archive/General/comp.programming/2007-05/msg00480.html
// need to include /usr/include/linux/joystick.h
#include <fcntl.h>
#include <linux/joystick.h>

#endif


namespace Zap
{

bool gJoystickInit = false;

#ifndef ZAP_DEDICATED
static Display *Xdisplay = XOpenDisplay(NULL);


#define MAX_AXIS 16
#define MAX_BUTTON 32	// Can fit 32 buttons in a 32 bit integer!


struct padData {
	unsigned char axisCount;
	unsigned char buttonCount;
	int fd;
	int version;
	char devName[80];
	int aPos[MAX_AXIS];
	int bPos[MAX_BUTTON];
	bool changed;
	js_event ev;
};

padData pad1;

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


// Joystick code loosely based on http://coding.derkeiler.com/Archive/General/comp.programming/2007-05/msg00480.html
void InitJoystick()
{
#ifndef ZAP_DEDICATED
   pad1.fd = open("/dev/input/js0", O_RDONLY);
   if (pad1.fd != 0){
      // Get pad info ...
      //logprintf("Joystick found");
      ioctl(pad1.fd, JSIOCGAXES,&pad1.axisCount);
      ioctl(pad1.fd, JSIOCGBUTTONS, &pad1.buttonCount);
      ioctl(pad1.fd, JSIOCGVERSION, &pad1.version);
      ioctl(pad1.fd, JSIOCGNAME(80), &pad1.devName);
      fcntl(pad1.fd, F_SETFL, O_NONBLOCK);

      //logprintf ("axis : %d\n", pad1.axisCount);
      //logprintf ("buttons : %d\n", pad1.buttonCount);
      //logprintf ("version : %d\n", pad1.version);
      //logprintf ("name : %s\n", pad1.devName);
      // set default values
      pad1.changed = false;
      for (int i=0;i<pad1.axisCount;i++) pad1.aPos[i]=0;
      for (int i=0;i<pad1.buttonCount;i++) pad1.bPos[i]=0;
   }
   gJoystickInit = true;
#endif
}

bool ReadJoystick(F32 axes[MaxJoystickAxes], U32 &buttonMask, U32 &hatMask)
{
#ifndef ZAP_DEDICATED
  if(! gJoystickInit)
	InitJoystick();

  if(pad1.fd == 0)
	return false;

  int result = read(pad1.fd, &pad1.ev, sizeof(pad1.ev));
  int loop1 = 0;

  while (result!=0 && loop1 < 100)
  {
    loop1++;
    switch (pad1.ev.type)
    {
		case JS_EVENT_INIT:
		case JS_EVENT_INIT | JS_EVENT_AXIS:
		case JS_EVENT_INIT | JS_EVENT_BUTTON:
		  break;
		case JS_EVENT_AXIS:
		  pad1.aPos[pad1.ev.number] = pad1.ev.value;
		  pad1.changed = true;
		  break;
		case JS_EVENT_BUTTON:
		  pad1.bPos[pad1.ev.number] = pad1.ev.value;
		  pad1.changed = true;
		break;
		default:
		  //logprintf ("Other event ? %d\nnumber=%d\nvalue=%d\n", pad1.ev.type,pad1.ev.number, pad1.ev.value);
		  break;
    }
    result = read(pad1.fd, &pad1.ev, sizeof(pad1.ev));
  }

  buttonMask = 0;
  for(S32 b=0; b<pad1.buttonCount; b++)
    buttonMask = buttonMask | ((pad1.bPos[b] != 0 ? 1 : 0) << b);

  for(S32 b=0; b<pad1.buttonCount && b<MaxJoystickAxes; b++)
    axes[b] = (F32)pad1.aPos[b]/32768;

  hatMask = 0; //POV (d-pad) either get maps to 2 more axes (logitech dual action), or 4 more buttons?

  return true;
#else
  return false;
#endif
}

const char *GetJoystickName()
{
#ifndef ZAP_DEDICATED
  if(! gJoystickInit)
	InitJoystick();
  return pad1.devName;
#else
  return "";
#endif
}

void ShutdownJoystick()
{
#ifndef ZAP_DEDICATED
  if(pad1.fd != 0) close(pad1.fd);
#endif
}


};

