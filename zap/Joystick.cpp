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

#include "Joystick.h"
#include "stringUtils.h"
#include "tnlLog.h"
#include "config.h"

#include "SDL/SDL.h"

namespace Zap {

// Linker needs these declared like this, why?
// private
SDL_Joystick *Joystick::sdlJoystick = NULL;

// public
Vector<const char *> Joystick::DetectedJoystickNameList;

U32 Joystick::ButtonMask = 0;
F32 Joystick::rawAxis[Joystick::rawAxisCount];
S16 Joystick::SensitivityThreshold = 3200;  // out of 32767
S32 Joystick::UseJoystickNumber = 0;
U32 Joystick::AxesKeyCodeMask = 0;


// Needs to be Aligned with JoystickAxesDirections
JoystickInput Joystick::JoystickInputData[MaxAxesDirections] = {
      {MoveAxesLeft,   MoveAxesLeftMask,   STICK_1_LEFT,  0.0f},
      {MoveAxesRight,  MoveAxesRightMask,  STICK_1_RIGHT, 0.0f},
      {MoveAxesUp,     MoveAxesUpMask,     STICK_1_UP,    0.0f},
      {MoveAxesDown,   MoveAxesDownMask,   STICK_1_DOWN,  0.0f},
      {ShootAxesLeft,  ShootAxesLeftMask,  STICK_2_LEFT,  0.0f},
      {ShootAxesRight, ShootAxesRightMask, STICK_2_RIGHT, 0.0f},
      {ShootAxesUp,    ShootAxesUpMask,    STICK_2_UP,    0.0f},
      {ShootAxesDown,  ShootAxesDownMask,  STICK_2_DOWN,  0.0f},
};


Joystick::Joystick() {
}

Joystick::~Joystick() {
}

bool Joystick::initJoystick()
{
   DetectedJoystickNameList.clear();

   // Close if already open.
   if (sdlJoystick != NULL)
   {
      SDL_JoystickClose(sdlJoystick);
      sdlJoystick = NULL;
   }

   // Will need to shutdown and init, to allow SDL_NumJoysticks to count new joysticks
   if(SDL_WasInit(SDL_INIT_JOYSTICK))
      SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

   // Initialize the SDL subsystem
   if (SDL_InitSubSystem(SDL_INIT_JOYSTICK))
   {
      logprintf("Unable to initialize the joystick subsystem");
      return false;
   }

   // How many joysticks are there
   S32 joystickCount = SDL_NumJoysticks();

   // No joysticks found
   if (joystickCount <= 0)
      return false;

   logprintf("%d joystick(s) detected:", joystickCount);
   for (S32 i = 0; i < joystickCount; i++)
   {
      const char * joystickName = SDL_JoystickName(i);
      logprintf("%d. %s", i, joystickName);
      DetectedJoystickNameList.push_back(joystickName);
   }

   // Enable joystick events
   SDL_JoystickEventState(SDL_ENABLE);


   // Start using joystick.
   sdlJoystick = SDL_JoystickOpen(UseJoystickNumber);
   if (sdlJoystick == NULL)
   {
      logprintf("Error opening joystick %d [%s]", UseJoystickNumber, SDL_JoystickName(UseJoystickNumber));
      return false;
   }
   logprintf("Using joystick %d - %s", UseJoystickNumber, SDL_JoystickName(UseJoystickNumber));

   // Determine the button mask for the UI stuff
   ButtonMask = 0;
   for(U32 b = 0; b < (U32)SDL_JoystickNumButtons(sdlJoystick) && b < 32; b++)  // We can detect up to 32 buttons with our mask
      ButtonMask |= 1 << b;

   return true;
}


void Joystick::populateJoystickStaticData()
{
   populatePredefinedJoystickList();
}


const char *Joystick::getJoystickName()
{
   return SDL_JoystickName(UseJoystickNumber);
}


void Joystick::shutdownJoystick()
{
   if (sdlJoystick != NULL) {
      SDL_JoystickClose(sdlJoystick);
      sdlJoystick = NULL;
   }

   if(SDL_WasInit(SDL_INIT_JOYSTICK))
      SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

JoystickInfo Joystick::PredefinedJoystickList[ControllerTypeCount] = 
{
   {
      "Logitech Wingman Dual-Analog",
      "LogitechWingman",
      9,
      {0, 1},
      {5, 6},
      { // LogitechWingman   9
         ControllerButton1,
         ControllerButton2,
         ControllerButton3,
         ControllerButton4,
         ControllerButton5,
         ControllerButton6,
         ControllerButton7,         // L-Trigger
         ControllerButton8,         // R-Trigger
         ControllerButtonBack,
         0,
         0,
         0,
         0,
         0,
      }
   },
   {
      "Logitech Dual Action",
      "LogitechDualAction",
      10,
      {0, 1},
      {2, 3},
      { // LogitechDualAction   10
         ControllerButton1,
         ControllerButton2,
         ControllerButton3,
         ControllerButton4,
         ControllerButton7,
         ControllerButton8,
         ControllerButton5,
         ControllerButton6,
         ControllerButtonBack,
         ControllerButtonStart,
         0,
         0,
         0,
         0,
      }
   },
   {
      "Saitek P-880 Dual-Analog",
      "SaitekDualAnalogP880",
      9,
      {0, 1},
      {3, 2},
      { // SaitekDualAnalogP880  9
         ControllerButton1,
         ControllerButton2,
         ControllerButton3,
         ControllerButton4,
         ControllerButton5,
         ControllerButton6,
         ControllerButton7,
         ControllerButton8,
         0,
         0,
         ControllerButtonBack,         // Red button??...  no start button??
         0,
         0,
         0,
      }
   },
   {
      "Saitek P-480 Dual-Analog",
      "SaitekDualAnalogRumblePad",
      10,
      {0, 1},
      {5, 2},
      { // SaitekDualAnalogRumblePad   10       // SAITEK P-480 DUAL-ANALOG
         ControllerButton1,
         ControllerButton2,
         ControllerButton3,
         ControllerButton4,
         ControllerButton5,
         ControllerButton6,
         ControllerButton7,
         ControllerButton8,
         ControllerButtonBack,      // Button 9
         ControllerButtonStart,     // Button 10
         0,
         0,
         0,
         0,
      }
   },
   {
      "PS2 Dualshock USB",
      "PS2DualShock",
      10,
      {0, 1},
      {2, 5},
      { // PS2DualShock    10
         ControllerButton4,
         ControllerButton2,
         ControllerButton1,
         ControllerButton3,
         ControllerButton5,
         ControllerButton6,
         ControllerButton7,
         ControllerButton8,
         ControllerButtonBack,
         0,
         0,
         ControllerButtonStart,
         0,
         0,
      }
   },
   {
      "PS2 Dualshock USB with Conversion Cable",
      "PS2DualShockConversionCable",
      10,
      {0, 1},
      {5, 2},
      { // PS2DualShockConversionCable    10
         ControllerButton4,
         ControllerButton2,
         ControllerButton1,
         ControllerButton3,
         ControllerButton5,
         ControllerButton6,
         ControllerButton7,
         ControllerButton8,
         ControllerButtonBack,
         ControllerButtonStart,
         0,
         0,
         0,
         0,
      }
   },
   {
      "PS3 Sixaxis",
      "PS3DualShock",
      13,
      {0, 1},
      {2, 3},
      { // PS3DualShock    13
         ControllerButtonStart, // Start
         0, // L3 - Unused
         0, // R3 - Unused
         ControllerButtonBack, // Select
         ControllerButtonDPadUp, // DPAD Up
         ControllerButtonDPadRight, // DPAD Right
         ControllerButtonDPadDown, // DPAD Down
         ControllerButtonDPadLeft, // DPAD Left
         ControllerButton5, // L2
         ControllerButton6, // R2
         ControllerButton7, // L1
         ControllerButton8, // R1
         ControllerButton4, // Triangle
         ControllerButton2, // Circle
         // FIXME: If you can get X and Square Working. Add them as these buttons:
         // ControllerButton1 // X
         // ControllerButton3 // Square
         // Above order should be correct. X is Button 14 Square is Button 15.
      }
   },
   {
      "XBox Controller USB",
      "XBoxController",
      10,
      {0, 1},
      {3, 4},
      { // XBoxController     10
         ControllerButton1,      // A
         ControllerButton2,      // B
         ControllerButton3,      // X
         ControllerButton4,      // Y
         ControllerButton6,      // RB
         ControllerButton5,      // LB
         ControllerButtonBack,   // <
         ControllerButtonStart,  // >
         0,
         0,
         ControllerButton7,
         ControllerButton8,
         0,
         0,
      }
   },
   {
      "XBox Controller",
      "XBoxControllerOnXBox",
      10,
      {0, 1},
      {3, 4},
      { // XBoxControllerOnXBox     <--- what's going on here?  On XBox??
         ControllerButton1,
         ControllerButton2,
         ControllerButton3,
         ControllerButton4,
         ControllerButton5,
         ControllerButton6,
         ControllerButton7,
         ControllerButton8,
         ControllerButtonStart,
         ControllerButtonBack,
         ControllerButtonDPadUp,
         ControllerButtonDPadDown,
         ControllerButtonDPadLeft,
         ControllerButtonDPadRight,
      }
   },
   // When adding more to PredefinedJoystickList, be sure to add more to ControllerTypeCount
};



void Joystick::populatePredefinedJoystickList()
{
}


ControllerTypeType Joystick::autodetectJoystickType()
{
   if (DetectedJoystickNameList.size() == 0)  // No controllers detected
      return NoController;

   ControllerTypeType ret = UnknownController;

   string controllerName = DetectedJoystickNameList[UseJoystickNumber];

   if(UseJoystickNumber >= 0)
   {
      if(controllerName == "WingMan")
         ret = LogitechWingman;

      else if(controllerName == "XBoxOnXBox")
         ret = XBoxControllerOnXBox;

      // Note that on the only XBox controller I've used on windows, the autodetect string was simply:
      // "Controller (XBOX 360 For Windows)".  I don't know if there are other variations out there.
      else if(controllerName.find("XBOX") != string::npos || controllerName.find("XBox") != string::npos)
         ret = XBoxController;

      else if(controllerName == "4 axis 16 button joystick")
         ret = PS2DualShock;                                         // http://ecx.images-amazon.com/images/I/412Q3RFHZVL._SS500_.jpg

      else if(controllerName == "PC Conversion Cable")
         ret = PS2DualShockConversionCable;                          // http://ecx.images-amazon.com/images/I/412Q3RFHZVL._SS500_.jpg

      // Note that on Linux, when I plug in the controller, DMSG outputs Sony PLAYSTATION(R)3 Controller.
      // Was connected.. Thus i have used it here. This may be different on Windows...
      else if(controllerName.find("PLAYSTATION(R)3") != string::npos)
         ret = PS3DualShock;                                         // http://ps3media.ign.com/ps3/image/article/705/705934/e3-2006-in-depth-with-the-ps3-controller-20060515010609802.jpg

      else if(controllerName == "P880")
         ret = SaitekDualAnalogP880;

      else if(controllerName == "Dual Analog Rumble Pad")
         ret = SaitekDualAnalogRumblePad;

      else if(controllerName.find("Logitech Dual Action") != string::npos)
         ret = LogitechDualAction;

      else if(controllerName.find("USB Joystick") != string::npos)
         ret = GenericController;

      else if(controllerName == "")     // Anything else -- joystick present but unknown
         ret = UnknownController;
      else                              // Not sure this can ever happen
         ret = NoController;
   }

   return ret;
}



U8 Joystick::remapJoystickButton(U8 button)
{
   // If not one of the predefined joysticks, just return the same button
   if(gIniSettings.joystickType >= ControllerTypeCount)
      return button;

   return PredefinedJoystickList[gIniSettings.joystickType].buttonMappings[button];
}


ControllerTypeType Joystick::stringToJoystickType(const char * strJoystick)
{
   for(S32 i=0; i < ControllerTypeCount; i++)
      if(!stricmp(strJoystick, PredefinedJoystickList[i].nameForINI))
         return ControllerTypeType(i);

   if (!stricmp(strJoystick, "GenericController"))
      return GenericController;
   else if (!stricmp(strJoystick, "UnknownController"))
      return UnknownController;
   else
      return NoController;
}


const char *Joystick::joystickTypeToString(S32 controllerType)
{
   if(controllerType < ControllerTypeCount)
      return PredefinedJoystickList[controllerType].nameForINI;

   switch (controllerType)
   {
   case GenericController:
      return "GenericController";
   case UnknownController:
      return "UnknownController";
   default:
      return "NoController";
   }
}


const char *Joystick::joystickTypeToPrettyString(S32 controllerType)
{
   if(controllerType < ControllerTypeCount)
      return PredefinedJoystickList[controllerType].name;
   switch (controllerType)
   {
   case GenericController:
      return "Generic Controller";
   case UnknownController:
      return "Unknown";
   default:
      return "No Controller";
   }
}

} /* namespace Zap */
