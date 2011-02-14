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
#include "tnlLog.h"

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


const U32 MAX_AXIS = 32;
const U32 MAX_BUTTON = 32;	// Can fit 32 buttons in a 32 bit integer!


struct padData {
   unsigned char axisCount;
   unsigned char buttonCount;
   int fd;
   int version;
   char devName[80];
   js_event ev;
   bool changed;
   int aPos[MAX_AXIS];
   int bPos[MAX_BUTTON];
};

const int MAX_JOYPADS = 8;
padData joyPads[MAX_JOYPADS];

extern Vector<string> gJoystickNames;
extern U32 gUseStickNumber;

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
   //logprintf("finding joystick");
   if(gJoystickInit)
      ShutdownJoystick();

   char chr1[15] = "/dev/input/js0";
   U32 gSticksFound = 0;

   gJoystickNames.clear();
   for(int i = 0; i < MAX_JOYPADS; i++)
   {
      chr1[13] = i + '0';
      joyPads[gSticksFound].fd = open(chr1, O_RDONLY);
      if (joyPads[gSticksFound].fd != 0){
         // Get pad info ...
         //logprintf("Joystick found");
         ioctl(joyPads[gSticksFound].fd, JSIOCGAXES,&joyPads[gSticksFound].axisCount);
         ioctl(joyPads[gSticksFound].fd, JSIOCGBUTTONS, &joyPads[gSticksFound].buttonCount);
         ioctl(joyPads[gSticksFound].fd, JSIOCGVERSION, &joyPads[gSticksFound].version);
         ioctl(joyPads[gSticksFound].fd, JSIOCGNAME(80), &joyPads[gSticksFound].devName);
         fcntl(joyPads[gSticksFound].fd, F_SETFL, O_NONBLOCK);

         //logprintf ("axis : %d\n", joyPads[gSticksFound].axisCount);
         //logprintf ("buttons : %d\n", joyPads[gSticksFound].buttonCount);
         //logprintf ("version : %d\n", joyPads[gSticksFound].version);
         //logprintf ("name : %s\n", joyPads[gSticksFound].devName);
         // set default values
         joyPads[gSticksFound].changed = false;

         for (U32 i = 0; i<MAX_AXIS; i++)
            joyPads[gSticksFound].aPos[i]=0;

         for (U32 i = 0; i<MAX_BUTTON; i++)
            joyPads[gSticksFound].bPos[i]=0;

         if(joyPads[gSticksFound].axisCount != 0 || joyPads[gSticksFound].buttonCount != 0)
         {
            gJoystickNames.push_back(joyPads[gSticksFound].devName);
            gSticksFound++;
         }
         else
            close(joyPads[gSticksFound].fd);
      }
   }
   logprintf("Found %i joysticks", gSticksFound);
   gJoystickInit = true;

   if(gUseStickNumber > (U32)gJoystickNames.size())
      gUseStickNumber = (U32)gJoystickNames.size();
#endif
}

bool ReadJoystick(F32 axes[MaxJoystickAxes], U32 &buttonMask, U32 &hatMask)
{
#ifndef ZAP_DEDICATED
   if(! gJoystickInit)
      InitJoystick();

   U32 useStickNumber = gUseStickNumber - 1;
   if(useStickNumber >= (U32)gJoystickNames.size())
      return false;

   if(joyPads[useStickNumber].fd == 0)
      return false;

   int result = read(joyPads[useStickNumber].fd, &joyPads[useStickNumber].ev, sizeof(joyPads[useStickNumber].ev));
   int loop1 = 0;

   while (result!=0 && loop1 < 100)
   {
      U32 num;
      loop1++;
      switch (joyPads[useStickNumber].ev.type)
      {
      case JS_EVENT_INIT:
      case JS_EVENT_INIT | JS_EVENT_AXIS:
      case JS_EVENT_INIT | JS_EVENT_BUTTON:
      break;
      case JS_EVENT_AXIS:
         num = joyPads[useStickNumber].ev.number;
         if(num < MAX_AXIS)
            joyPads[useStickNumber].aPos[num] = joyPads[useStickNumber].ev.value;
         joyPads[useStickNumber].changed = true;
         break;
      case JS_EVENT_BUTTON:
         joyPads[useStickNumber].ev.number;
         if(num < MAX_BUTTON)
            joyPads[useStickNumber].bPos[num] = joyPads[useStickNumber].ev.value;
         joyPads[useStickNumber].changed = true;
         break;
      default:
         //logprintf ("Other event ? %d\nnumber=%d\nvalue=%d\n", pad1.ev.type,pad1.ev.number, pad1.ev.value);
         break;
      }
      result = read(joyPads[useStickNumber].fd, &joyPads[useStickNumber].ev, sizeof(joyPads[useStickNumber].ev));
   }

   buttonMask = 0;
   for(U32 b=0; b<joyPads[useStickNumber].buttonCount && b < MAX_BUTTON; b++)
      buttonMask = buttonMask | ((joyPads[useStickNumber].bPos[b] != 0 ? 1 : 0) << b);

   for(U32 b=0; b<MaxJoystickAxes && b<MAX_AXIS; b++)
      axes[b] = F32(joyPads[useStickNumber].aPos[b])/32768; //convert to Float between -1.0 and 1.0

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

   U32 useStickNumber = gUseStickNumber - 1;
   if(useStickNumber >= (U32)gJoystickNames.size())
      return "";

   return joyPads[useStickNumber].devName;
#else
   return "";
#endif
}

void ShutdownJoystick()
{
#ifndef ZAP_DEDICATED
   for(int i = 0; i < MAX_JOYPADS; i++)
   {
      if(joyPads[i].fd != 0) close(joyPads[i].fd);
   }

   gJoystickNames.clear();
   gJoystickInit = false;
#endif
}


};

