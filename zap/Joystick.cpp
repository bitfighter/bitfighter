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

using namespace TNL;

namespace Zap {

// Linker needs these declared like this, why?
// private
SDL_Joystick *Joystick::sdlJoystick = NULL;

// public
TNL::U32 Joystick::ButtonMask = 0;
TNL::Vector<const char *> Joystick::DetectedJoystickNameList;
TNL::S16 Joystick::SensitivityThreshold = 0;
TNL::S32 Joystick::UseJoystickNumber = 0;
TNL::Vector<JoystickInfo> Joystick::PredefinedJoystickList;
JoystickInput Joystick::JoystickInputData[MaxAxesDirections];
TNL::U32 Joystick::AxesKeyCodeMask = 0;


Joystick::Joystick() {
}

Joystick::~Joystick() {
}

bool Joystick::initJoystick()
{
   DetectedJoystickNameList.clear();

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

   // Close if already open.
   if (sdlJoystick != NULL)
   {
      SDL_JoystickClose(sdlJoystick);
      sdlJoystick = NULL;
   }

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
   UseJoystickNumber = 0;
   SensitivityThreshold = 3200;  // out of 32767
   AxesKeyCodeMask = 0;

//   // Needs to be Aligned with JoystickAxesDirections
   JoystickInput joystickInput[MaxAxesDirections] = {
         {MoveAxesLeft,   MoveAxesLeftMask,   STICK_1_LEFT,  0.0f},
         {MoveAxesRight,  MoveAxesRightMask,  STICK_1_RIGHT, 0.0f},
         {MoveAxesUp,     MoveAxesUpMask,     STICK_1_UP,    0.0f},
         {MoveAxesDown,   MoveAxesDownMask,   STICK_1_DOWN,  0.0f},
         {ShootAxesLeft,  ShootAxesLeftMask,  STICK_2_LEFT,  0.0f},
         {ShootAxesRight, ShootAxesRightMask, STICK_2_RIGHT, 0.0f},
         {ShootAxesUp,    ShootAxesUpMask,    STICK_2_UP,    0.0f},
         {ShootAxesDown,  ShootAxesDownMask,  STICK_2_DOWN,  0.0f},
   };

   *JoystickInputData = *joystickInput;


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
}


void Joystick::populatePredefinedJoystickList()
{
   // Initialize our static Vector
//   PredefinedJoystickList((S32)ControllerTypeCount);

   // What axes to use for firing?  Should we let users set this somehow?
   //
   // Unfortunately, Linux and Windows map joystick axes differently
   // XXX: are they still different with SDL?
   //                                               Wingmn  DualAct  P880  RumbPad   PS2   PS2-Conv  PS3     XBox  XBoxOnXBox
#ifdef TNL_OS_LINUX
   //                      ? = untested                ?     works     ?       ?       ?       ?       ?       ?       ?
   U32 shootAxes[ControllerTypeCount][2] = { {5, 6}, {2, 3}, {5, 2}, {5, 2}, {2, 5}, {5, 2}, {2, 3}, {3, 4}, {3, 4} };
#else
   U32 shootAxes[ControllerTypeCount][2] = { {5, 6}, {2, 5}, {5, 2}, {5, 2}, {2, 5}, {5, 2}, {2, 3}, {3, 4}, {3, 4} };
#endif

   U32 moveAxes[ControllerTypeCount][2] = { {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1} };

   // Button counts for each controller
   U32 buttonCount[ControllerTypeCount] = { 9, 10, 9, 10, 10, 10, 10, 10, 14};

   U32 buttonRemap[ControllerTypeCount][MaxControllerButtons] =
   {
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
         },
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
         },
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
         },
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
         },
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
         },
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
         },
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
   };

   // Finally tie it all together
//   PredefinedJoystickList = {
//         {9,  {0,1}, shootAxes[LogitechWingman],             buttonRemap[LogitechWingman]},
//         {10, {0,1}, shootAxes[LogitechDualAction],          buttonRemap[LogitechDualAction]},
//         {9,  {0,1}, shootAxes[SaitekDualAnalogP880],        buttonRemap[SaitekDualAnalogP880]},
//         {10, {0,1}, shootAxes[SaitekDualAnalogRumblePad],   buttonRemap[SaitekDualAnalogRumblePad]},
//         {10, {0,1}, shootAxes[PS2DualShock],                buttonRemap[PS2DualShock]},
//         {10, {0,1}, shootAxes[PS2DualShockConversionCable], buttonRemap[PS2DualShockConversionCable]},
//         {10, {0,1}, shootAxes[PS3DualShock],                buttonRemap[PS3DualShock]},
//         {10, {0,1}, shootAxes[XBoxController],              buttonRemap[XBoxController]},
//         {14, {0,1}, shootAxes[XBoxControllerOnXBox],        buttonRemap[XBoxControllerOnXBox]},
//   };

   JoystickInfo info;
   for (S32 i = 0; i < ControllerTypeCount; i++)
   {
      info.buttonCount = buttonCount[i];
      info.moveAxesSdlIndex[0] = moveAxes[i][0];
      info.moveAxesSdlIndex[1] = moveAxes[i][1];
      info.shootAxesSdlIndex[0] = shootAxes[i][0];
      info.shootAxesSdlIndex[1] = shootAxes[i][1];
      for (S32 j = 0; j < MaxControllerButtons; j++)
         info.buttonMappings[j] = buttonRemap[i][j];

      PredefinedJoystickList.push_back(info);
   }

   logprintf("Predefined Joystick Count: %d", PredefinedJoystickList.size());
}


ControllerTypeType Joystick::autodetectJoystickType()
{
   if (DetectedJoystickNameList.size() == 0)  // No controllers detected
      return NoController;

   S32 ret = UnknownController;

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

   return (ControllerTypeType) ret;
}


extern IniSettings gIniSettings;

U8 Joystick::remapJoystickButton(U8 button)
{
   // If not one of the predefined joysticks, just return the same button
   if(gIniSettings.joystickType < ControllerTypeCount)
      return button;

   return PredefinedJoystickList[gIniSettings.joystickType].buttonMappings[button];
}


ControllerTypeType Joystick::stringToJoystickType(const char * strJoystick)
{
   if (strJoystick == "LogitechWingman")
      return LogitechWingman;
   else if (strJoystick == "LogitechDualAction")
      return LogitechDualAction;
   else if (strJoystick == "SaitekDualAnalogP880")
      return SaitekDualAnalogP880;
   else if (strJoystick == "SaitekDualAnalogRumblePad")
      return SaitekDualAnalogRumblePad;
   else if (strJoystick == "PS2DualShock")
      return PS2DualShock;
   else if (strJoystick == "PS2DualShockConversionCable")
      return PS2DualShockConversionCable;
   else if (strJoystick == "PS3DualShock")
      return PS3DualShock;
   else if (strJoystick == "XBoxController")
      return XBoxController;
   else if (strJoystick == "XBoxControllerOnXBox")
      return XBoxControllerOnXBox;
   else if (strJoystick == "GenericController")
      return GenericController;
   else if (strJoystick == "UnknownController")
      return UnknownController;
   else
      return NoController;
}


const char *Joystick::joystickTypeToString(S32 controllerType)
{
   switch (controllerType)
   {
   case LogitechWingman:
      return "LogitechWingman";
   case LogitechDualAction:
      return "LogitechDualAction";
   case SaitekDualAnalogP880:
      return "SaitekDualAnalogP880";
   case SaitekDualAnalogRumblePad:
      return "SaitekDualAnalogRumblePad";
   case PS2DualShock:
      return "PS2DualShock";
   case PS2DualShockConversionCable:
      return "PS2DualShockConversionCable";
   case PS3DualShock:
      return "PS3DualShock";
   case XBoxController:
      return "XBoxController";
   case XBoxControllerOnXBox:
      return "XBoxControllerOnXBox";
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
   switch (controllerType)
   {
   case LogitechWingman:
      return "Logitech Wingman Dual-Analog";
   case LogitechDualAction:
      return "Logitech Dual Action";
   case SaitekDualAnalogP880:
      return "Saitek P-880 Dual-Analog";
   case SaitekDualAnalogRumblePad:
      return "Saitek P-480 Dual-Analog";
   case PS2DualShock:
      return "PS2 Dualshock USB";
   case PS2DualShockConversionCable:
      return "PS2 Dualshock USB with Conversion Cable";
   case PS3DualShock:
      return "PS3 Sixaxis";
   case XBoxController:
      return "XBox Controller USB";
   case XBoxControllerOnXBox:
      return "XBox Controller";
   case GenericController:
      return "Generic Controller";
   case UnknownController:
      return "Unknown";
   default:
      return "No Controller";
   }
}

} /* namespace Zap */
