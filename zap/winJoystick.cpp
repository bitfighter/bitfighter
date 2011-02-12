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

#define STRICT
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include "dinput.h"	 // See readme in win_include_do_not_distribute folder

#include "gameConnection.h"
#include "UI.h"
#include "input.h"
#include "point.h"

namespace Zap
{

extern const char *gWindowTitle;

void getModifierState( bool &shiftDown, bool &controlDown, bool &altDown )
{
	shiftDown = ( GetKeyState( VK_SHIFT ) & 0xFF00 ) != 0;
	controlDown = ( GetKeyState( VK_CONTROL ) & 0xFF00 ) != 0;
	altDown = ( GetKeyState( VK_MENU ) & 0xFF00 ) != 0;
}

void checkMousePos(S32 maxdx, S32 maxdy)
{
   char windowName[256];

   HWND theWindow = GetForegroundWindow();
   GetWindowText(theWindow, windowName, sizeof(windowName));

   if(strcmp(windowName, gWindowTitle))
      return;

   RECT r;

   GetWindowRect(theWindow, &r);
   POINT cp;
   GetCursorPos(&cp);

   // Check our position in the window; if we're close to the top (within 32px) then ignore
   // same if we're within 5px of the edges. That way we can avoid going crazy when
   // people try to resize/drag the window.
   if(cp.x - r.top < 32)   return;
   if(cp.y - r.left < 5)   return;
   if(r.right - cp.y < 5)  return;
   if(r.bottom - cp.x < 5) return;

   S32 centerX = (r.right + r.left) >> 1;
   S32 centerY = (r.bottom + r.top) >> 1;

   cp.x -= centerX;
   cp.y -= centerY;

   if(cp.x > maxdx)
      cp.x = maxdx;
   if(cp.y > maxdy)
      cp.y = maxdy;
   if(cp.x < -maxdx)
      cp.x = -maxdx;
   if(cp.y < -maxdy)
      cp.y = -maxdy;

   SetCursorPos(int(centerX + cp.x), int(centerY + cp.y));
}

BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );

LPDIRECTINPUT8 gDirectInput = NULL;
LPDIRECTINPUTDEVICE8 gJoystick[32];
extern U32 gUseStickNumber;

extern Vector<string> gJoystickNames;

void InitJoystick()
{
   gJoystickNames.clear();
   if(FAILED(DirectInput8Create ( GetModuleHandle(NULL), DIRECTINPUT_VERSION,
         IID_IDirectInput8, (VOID**)&gDirectInput, NULL ) ) )
      return;

   if(FAILED(gDirectInput->EnumDevices( DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback,
         NULL, DIEDFL_ATTACHEDONLY ) ) )
      return;

   if(gUseStickNumber > (U32)gJoystickNames.size())
      gUseStickNumber = (U32)gJoystickNames.size();
}


BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance,
                                     VOID* pContext )
{
   // Obtain an interface to the enumerated joystick.
   if(FAILED(gDirectInput->CreateDevice( pdidInstance->guidInstance, &gJoystick[gJoystickNames.size()], NULL )))
      return DIENUM_CONTINUE;
   
   logprintf("Joystick %d found: %s", gJoystickNames.size(), pdidInstance->tszProductName);
   if( FAILED(gJoystick[gJoystickNames.size()]->SetDataFormat( &c_dfDIJoystick2 ) ) )
   {
      logprintf("Joystick fail to SetDataFormat");
      return DIENUM_CONTINUE;
   }

   gJoystickNames.push_back(pdidInstance->tszProductName);

   return DIENUM_CONTINUE;  // DIENUM_CONTINUE will find both multiple joysticks...  DIENUM_STOP will not.
}

// hatMask tracks DPad entry, axes and buttonMask track joysticks and buttons
// hatMask is usually 0 when passed in
bool ReadJoystick(F32 axes[MaxJoystickAxes], U32 &buttonMask, U32 &hatMask)
{
   // mark: it's ok
   // mark: it's called "winJoystick"
   // mark: it's supposed to be gross.

   U32 useStickNumber = gUseStickNumber - 1;  // first joystick is zero.

   if(useStickNumber >= (U32)gJoystickNames.size())  // out of range
      return false;

   if(FAILED(gJoystick[useStickNumber]->Poll() ) )
   {
      HRESULT hr;
      hr = gJoystick[useStickNumber]->Acquire();

      while( hr == DIERR_INPUTLOST )
         hr = gJoystick[useStickNumber]->Acquire();
      return false;
   }

   DIJOYSTATE2 js;       // DInput joystick state

   // Get the input's device state
   if(FAILED(gJoystick[useStickNumber]->GetDeviceState( sizeof(DIJOYSTATE2), &js ) ) )
      return false; // The device should have been acquired during the Poll()

   //logprintf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",js.lX,js.lY,js.lZ,js.lRx,js.lRy,js.lRz,js.rglSlider[0],js.rglSlider[1],js.rgdwPOV[0],js.rgdwPOV[1],js.rgdwPOV[2],js.rgdwPOV[3],js./*rgbButtonsButtons[128];//128buttons*/lVX,js.lVY,js.lVZ,js.lVRx,js.lVRy,js.lVRz,js.rglVSlider[0],js.rglVSlider[1],js.lAX,js.lAY,js.lAZ,js.lARx,js.lARy,js.lARz,js.rglASlider[0],js.rglASlider[1],js.lFX,js.lFY,js.lFZ,js.lFRx,js.lFRy,js.lFRz,js.rglFSlider[0],js.rglFSlider[1]);

   F32 scale = 1 / 32768.0f;
   axes[0] = (F32(js.lX) - 32768.0f) * scale;
   axes[1] = (F32(js.lY) - 32768.0f) * scale;
   axes[2] = (F32(js.lZ) - 32768.0f) * scale;
   axes[3] = (F32(js.lRx) - 32768.0f) * scale;
   axes[4] = (F32(js.lRy) - 32768.0f) * scale;
   axes[5] = (F32(js.lRz) - 32768.0f) * scale;
   axes[6] = (F32(js.rglSlider[0]) - 32768.0f) * scale;
   axes[7] = (F32(js.rglSlider[1]) - 32768.0f) * scale;
   axes[8] = 0;
   axes[9] = 0;
   axes[10] = 0;
   axes[11] = 0;

   //logprintf("%2.2f,%2.2f,%2.2f,%2.2f,%2.2f,%2.2f,%2.2f,%2.2f,%2.2f,%2.2f,%2.2f,%2.2f", axes[0],axes[1], axes[2], axes[3], axes[4], axes[5], axes[6], axes[7], axes[8], axes[9], axes[10], axes[11]);

   // check the state of the buttons:
   buttonMask = 0;
   U32 pov = js.rgdwPOV[0];
   
   for( U32 i = 0; i < 32; i++ )       // Most sticks have 12 or fewer buttons, but we can detect up to 32
      if((js.rgbButtons[i] & 0x80) != 0)
         buttonMask |= BIT(i);

   // Check for DPad input
   switch(pov)
   {
   case 0:
      hatMask |= ControllerButtonDPadUp;
      break;
   case 4500:
      hatMask |= ControllerButtonDPadUp | ControllerButtonDPadRight;
      break;
   case 9000:
      hatMask |= ControllerButtonDPadRight;
      break;
   case 13500:
      hatMask |= ControllerButtonDPadRight | ControllerButtonDPadDown;
      break;
   case 18000:
      hatMask |= ControllerButtonDPadDown;
      break;
   case 22500:
      hatMask |= ControllerButtonDPadDown | ControllerButtonDPadLeft;
      break;
   case 27000:
      hatMask |= ControllerButtonDPadLeft;
      break;
   case 31500:
      hatMask |= ControllerButtonDPadLeft | ControllerButtonDPadUp;
      break;
   }
   return true;
}

void ShutdownJoystick()
{
   for(S32 i = 0; i < gJoystickNames.size(); i++)
   {
      // Unacquire the device one last time just in case
      // the app tried to exit while the device is still acquired.
      if(gJoystick[i])
         gJoystick[i]->Unacquire();

      // Release any DirectInput objects.
      if(gJoystick[i])
         gJoystick[i]->Release();
   }
   gJoystickNames.clear();

   if(gDirectInput)
      gDirectInput->Release();
}

};

