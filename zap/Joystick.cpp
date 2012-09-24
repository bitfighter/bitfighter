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
#include "ClientGame.h"
#include "IniFile.h"
#include "Colors.h"

#include "SDL.h"

#include <map>

namespace Zap {

// Linker needs these declared like this, why?
// private
SDL_Joystick *Joystick::sdlJoystick = NULL;

// public
Vector<string> Joystick::DetectedJoystickNameList;
Vector<Joystick::JoystickInfo> Joystick::JoystickPresetList;

U32 Joystick::ButtonMask = 0;
F32 Joystick::rawAxis[Joystick::rawAxisCount];
S16 Joystick::LowerSensitivityThreshold = 4900;   // out of 32767, ~15%, any less than this is ends up as zero
S16 Joystick::UpperSensitivityThreshold = 30000;  // out of 32767, ~91%, any more than this is full amount
S32 Joystick::UseJoystickNumber = 0;
U32 Joystick::AxesInputCodeMask = 0;
U32 Joystick::HatInputCodeMask = 0;
U32 Joystick::SelectedPresetIndex = 0;


// Needs to be Aligned with JoystickAxesDirections
JoystickInput Joystick::JoystickInputData[MaxAxesDirections] = {
      { MoveAxesLeft,   MoveAxesLeftMask,   STICK_1_LEFT,  0.0f },
      { MoveAxesRight,  MoveAxesRightMask,  STICK_1_RIGHT, 0.0f },
      { MoveAxesUp,     MoveAxesUpMask,     STICK_1_UP,    0.0f },
      { MoveAxesDown,   MoveAxesDownMask,   STICK_1_DOWN,  0.0f },
      { ShootAxesLeft,  ShootAxesLeftMask,  STICK_2_LEFT,  0.0f },
      { ShootAxesRight, ShootAxesRightMask, STICK_2_RIGHT, 0.0f },
      { ShootAxesUp,    ShootAxesUpMask,    STICK_2_UP,    0.0f },
      { ShootAxesDown,  ShootAxesDownMask,  STICK_2_DOWN,  0.0f },
};


CIniFile joystickPresetsINI("dummy");

// Constructor
Joystick::Joystick()
{
   // Do nothing
}


// Destructor
Joystick::~Joystick()
{
   // Do nothing
}

bool Joystick::initJoystick(GameSettings *settings)
{
   // Make sure "SDL_Init(0)" was done before calling this function, otherwise joystick will fail to work on windows.
#if defined(TNL_OS_LINUX) && !SDL_VERSION_ATLEAST(2,0,0)
   // Hackety hack hack for some joysticks that seem calibrated horribly wrong.
   //
   // What happens is that SDL uses the newer event system at /dev/input/eventX for joystick enumeration
   // instead of the older /dev/input/jsX or /dev/jsX;  The problem is that calibration cannot be done on
   // the event (/dev/input/eventX) devices and therefore some joysticks, like the PS3, act strangely in-game
   //
   // If you specify "JoystickLinuxUseOldDeviceSystem" as "Yes" in the INI, then this code below will
   // add /dev/input/js0 to the list of enumerated joysticks (as joystick 0).  This means that if you have
   // a PS3 joystick plugged in, SDL will detect *two* joysticks:
   //
   // 1. joystick 0 = PS3 controller at /dev/input/js0
   // 2. joystick 1 = PS3 controller at /dev/input/eventX (where 'X' can be any number)
   //
   // See here for more info:
   //   http://superuser.com/questions/17959/linux-joystick-seems-mis-calibrated-in-an-sdl-game-freespace-2-open


   if(settings->getIniSettings()->joystickLinuxUseOldDeviceSystem)
   {
      string joystickEnv = "SDL_JOYSTICK_DEVICE=/dev/input/js" + itos(0);
      SDL_putenv((char *)joystickEnv.c_str());

      logprintf("Using older Linux joystick device system to workaround calibration problems");
   }
#endif

   DetectedJoystickNameList.clear();

   bool hasBeenOpenedBefore = (sdlJoystick != NULL);

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
      const char *joystickName = SDL_JoystickName(i);
      logprintf("%d.) Autodetect string = \"%s\"", i + 1, joystickName);
      DetectedJoystickNameList.push_back(joystickName);
   }

   // Enable joystick events
   SDL_JoystickEventState(SDL_ENABLE);


   // Start using joystick
   sdlJoystick = SDL_JoystickOpen(UseJoystickNumber);
   if(sdlJoystick == NULL)
   {
      logprintf("Error opening joystick %d [%s]", UseJoystickNumber + 1, SDL_JoystickName(UseJoystickNumber));
      return false;
   }
   logprintf("Using joystick %d - %s", UseJoystickNumber + 1, SDL_JoystickName(UseJoystickNumber));


   // Now try and autodetect the joystick and update the game settings
   string joystickType = Joystick::autodetectJoystick(settings);

   // Set joystick type if we found anything
   // Otherwise, it makes more sense to remember what the user had last specified
   if (!hasBeenOpenedBefore && joystickType != "NoJoystick")
   {
      settings->getIniSettings()->joystickType = joystickType;
      setSelectedPresetIndex(Joystick::getJoystickIndex(joystickType));
   }

   if(settings->getIniSettings()->alwaysStartInKeyboardMode)
   {
      settings->getInputCodeManager()->setInputMode(InputModeKeyboard);
      return true;
   }

   // Set primary input to joystick if any controllers were found, even a generic one
   if(hasBeenOpenedBefore)
      return true;  // Do nothing when this was opened before
   else if(joystickType == "NoJoystick")
      settings->getInputCodeManager()->setInputMode(InputModeKeyboard);
   else
      settings->getInputCodeManager()->setInputMode(InputModeJoystick);

   return true;
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


// Returns -1 if there is no match, otherwise returns the index of the match
S32 Joystick::checkJoystickString_exact_match(const string &controllerName)
{
   for(S32 i = 0; i < JoystickPresetList.size(); i++)
      if(!JoystickPresetList[i].isSearchStringSubstring)  // Use exact string
         if(controllerName == JoystickPresetList[i].searchString)
            return i;

   return -1;     // No match
}


// Returns -1 if there is no match, otherwise returns the index of the match
S32 Joystick::checkJoystickString_partial_match(const string &controllerName)
{
   for(S32 i = 0; i < JoystickPresetList.size(); i++)
      if(JoystickPresetList[i].isSearchStringSubstring)   // Use substring
         if(lcase(controllerName).find(lcase(JoystickPresetList[i].searchString)) != string::npos)
            return i;
               // end cascading fury of death <== metacomment: not useful, but entertaining.  Wait... fury of death or furry death?

   return -1;     // No match
}


// Returns a valid name of one of our joystick profiles
string Joystick::autodetectJoystick(GameSettings *settings)
{
   if(DetectedJoystickNameList.size() == 0)  // No controllers detected
      return "NoJoystick";

   string controllerName = DetectedJoystickNameList[UseJoystickNumber];

   S32 match;
   // First check against predefined joysticks that have exact search strings
   // We do this first so that a substring match doesn't override one of these (like with XBox controller)
   match = checkJoystickString_exact_match(controllerName);
   if(match >= 0)    
      return JoystickPresetList[match].identifier;

   // Then check against joysticks that use substrings to match
   match = checkJoystickString_partial_match(controllerName);
   if(match >= 0)    
      return JoystickPresetList[match].identifier;
   
   // If we've made it here, let's try the value stored in the INI
   string lastStickUsed = settings->getIniSettings()->joystickType;

   // Let's validate that, shall we?
   for(S32 i = 0; i < JoystickPresetList.size(); i++)
      if(JoystickPresetList[i].identifier == lastStickUsed)
         return JoystickPresetList[i].identifier;

   // It's beyond hope!  Return something that will *ALWAYS* be wrong.
   return "GenericJoystick";
}


Joystick::Button Joystick::remapSdlButtonToJoystickButton(U8 rawButton)
{
   for(S32 i = 0; i < MaxJoystickButtons; i++)
      if(JoystickPresetList[SelectedPresetIndex].buttonMappings[i].sdlButton == rawButton)
         return JoystickPresetList[SelectedPresetIndex].buttonMappings[i].button;

   return ButtonUnknown;
}


void Joystick::getAllJoystickPrettyNames(Vector<string> &nameList)
{
   for(S32 i = 0; i < JoystickPresetList.size(); i++)
      nameList.push_back(JoystickPresetList[i].name);
}


Joystick::Button Joystick::stringToJoystickButton(const string &buttonString)
{
   if(buttonString == "Button1")
      return Button1;
   else if(buttonString == "Button2")
      return Button2;
   else if(buttonString == "Button3")
      return Button3;
   else if(buttonString == "Button4")
      return Button4;
   else if(buttonString == "Button5")
      return Button5;
   else if(buttonString == "Button6")
      return Button6;
   else if(buttonString == "Button7")
      return Button7;
   else if(buttonString == "Button8")
      return Button8;
   else if(buttonString == "Button9")
      return Button9;
   else if(buttonString == "Button10")
      return Button10;
   else if(buttonString == "Button11")
      return Button11;
   else if(buttonString == "Button12")
      return Button12;
   else if(buttonString == "ButtonStart")
      return ButtonStart;
   else if(buttonString == "ButtonBack")
      return ButtonBack;
   else if(buttonString == "ButtonDPadUp")
      return ButtonDPadUp;
   else if(buttonString == "ButtonDPadDown")
      return ButtonDPadDown;
   else if(buttonString == "ButtonDPadLeft")
      return ButtonDPadLeft;
   else if(buttonString == "ButtonDPadRight")
      return ButtonDPadRight;

   return ButtonUnknown;
}


Joystick::ButtonShape Joystick::buttonLabelToButtonShape(const string &label)
{
   if(label == "Round")
      return ButtonShapeRound;
   else if (label == "Rect")
      return ButtonShapeRect;
   else if (label == "SmallRect")
      return ButtonShapeSmallRect;
   else if (label == "RoundedRect")
      return ButtonShapeRoundedRect;
   else if (label == "SmallRoundedRect")
      return ButtonShapeSmallRoundedRect;
   else if (label == "HorizEllipse")
      return ButtonShapeHorizEllipse;
   else if (label == "RightTriangle")
      return ButtonShapeRightTriangle;

   return ButtonShapeRound;  // Default
}


Joystick::ButtonSymbol Joystick::stringToButtonSymbol(const string &label)
{
   if(label == "PSCIRCLE")
      return ButtonSymbolPsCircle;
   else if(label == "PSCROSS")
      return ButtonSymbolPsCross;
   else if(label == "PSSQUARE")
      return ButtonSymbolPsSquare;
   else if(label == "PSTRIANGLE")
      return ButtonSymbolPsTriangle;
   else if(label == "SMALLLEFTTRIANGLE")
      return ButtonSymbolSmallLeftTriangle;
   else if(label == "SMALLRIGHTTRIANGLE")
      return ButtonSymbolSmallRightTriangle;

   return ButtonSymbolNone;
}


Color Joystick::stringToColor(const string &colorString)
{
   string lower = lcase(colorString);
   if(lower == "white")
      return Colors::white;
   else if (lower == "green")
      return Colors::green;
   else if (lower == "blue")
      return Colors::blue;
   else if (lower == "yellow")
      return Colors::yellow;
   else if (lower == "cyan")
      return Colors::cyan;
   else if (lower == "magenta")
      return Colors::magenta;
   else if (lower == "black")
      return Colors::black;
   else if (lower == "red")
      return Colors::red;
   else if (lower == "paleRed")
      return Colors::paleRed;
   else if (lower == "paleBlue")
      return Colors::paleBlue;
   else if (lower == "palePurple")
      return Colors::palePurple;
   else if (lower == "paleGreen")
      return Colors::paleGreen;

   return Colors::white;  // default
}


void Joystick::setSelectedPresetIndex(U32 joystickIndex)
{
   SelectedPresetIndex = joystickIndex;
}


Joystick::JoystickInfo *Joystick::getJoystickInfo(const string &joystickType)
{
   for(S32 i = 0; i < JoystickPresetList.size(); i++)
      if(joystickType == JoystickPresetList[i].identifier)
         return &JoystickPresetList[i];

   TNLAssert(false, "We should never get here!");
   return NULL;
}


Joystick::JoystickInfo Joystick::getGenericJoystickInfo()
{
   JoystickInfo joystickInfo;

   joystickInfo.identifier = "GenericJoystick";
   joystickInfo.name = "Generic Joystick";
   joystickInfo.searchString = "";
   joystickInfo.isSearchStringSubstring = false;
   joystickInfo.moveAxesSdlIndex[0] = 0;
   joystickInfo.moveAxesSdlIndex[1] = 1;
   joystickInfo.shootAxesSdlIndex[0] = 2;
   joystickInfo.shootAxesSdlIndex[1] = 3;

   // Make the button graphics all the same
   for(S32 i = 0; i < MaxJoystickButtons; i++)
   {
      joystickInfo.buttonMappings[i].button = (Joystick::Button)i;  // 'i' should be in line with Joystick::Button
      joystickInfo.buttonMappings[i].label = "";
      joystickInfo.buttonMappings[i].color = Colors::white;
      joystickInfo.buttonMappings[i].buttonShape = ButtonShapeRound;
      joystickInfo.buttonMappings[i].buttonSymbol = ButtonSymbolNone;
   }

   // Add some labels
   for(S32 i = 0; i < 8; i++)
      joystickInfo.buttonMappings[i].label = itos(i);

   joystickInfo.buttonMappings[ButtonBack].label = "9";
   joystickInfo.buttonMappings[ButtonStart].label = "10";
   
   return joystickInfo;
}


U32 Joystick::getJoystickIndex(const string &joystickType)
{
   for(S32 i = 0; i < JoystickPresetList.size(); i++)
      if(joystickType == JoystickPresetList[i].identifier)
         return (U32)i;

   TNLAssert(false, "We should never get here!");
   return JoystickPresetList.size() - 1;              // Return the generic joystick
}


void Joystick::loadJoystickPresets(GameSettings *settings)
{

// Load up the joystick presets INI
string dir;

#ifdef TNL_OS_MAC_OSX
   FolderManager *folderManager = settings->getFolderManager();
   dir = folderManager->iniDir;
#else
   dir = ".";
#endif

   joystickPresetsINI.SetPath(joindir(dir, "joystick_presets.ini"));
   joystickPresetsINI.ReadFile();

   // Loop through each section (joystick) and parse
   for (S32 sectionId = 0; sectionId < joystickPresetsINI.GetNumSections(); sectionId++)
   {
      JoystickInfo joystickInfo;

      // Names names names
      joystickInfo.identifier = joystickPresetsINI.sectionName(sectionId).c_str();
      joystickInfo.name = joystickPresetsINI.GetValue(sectionId, "Name").c_str();
      joystickInfo.searchString = joystickPresetsINI.GetValue(sectionId, "SearchString").c_str();
      joystickInfo.isSearchStringSubstring =
            lcase(joystickPresetsINI.GetValue(sectionId, "SearchStringIsSubstring")) == "yes";

      // Axis of evil
      joystickInfo.moveAxesSdlIndex[0] = stoi(joystickPresetsINI.GetValue(sectionId, "MoveAxisLeftRight"));
      joystickInfo.moveAxesSdlIndex[1] = stoi(joystickPresetsINI.GetValue(sectionId, "MoveAxisUpDown"));
      joystickInfo.shootAxesSdlIndex[0] = stoi(joystickPresetsINI.GetValue(sectionId, "ShootAxisLeftRight"));
      joystickInfo.shootAxesSdlIndex[1] = stoi(joystickPresetsINI.GetValue(sectionId, "ShootAxisUpDown"));

      Vector<string> sectionKeys;
      joystickPresetsINI.GetAllKeys(sectionId, sectionKeys);

      // Start the search for Button-related keys
      Vector<string> buttonKeyNames;
      string buttonName;
      for(S32 i = 0; i < sectionKeys.size(); i++)
      {
         buttonName = sectionKeys[i];
         if(buttonName.substr(0, 6) == "Button")   // Found a button!
            buttonKeyNames.push_back(buttonName);
      }

      // Now load up button values
      for(S32 i = 0; i < buttonKeyNames.size(); i++)
      {
         // Parse the complex string into key/value pairs
         map<string, string> buttonInfoMap;
         parseComplexStringToMap(joystickPresetsINI.GetValue(sectionId, buttonKeyNames[i]), buttonInfoMap);

         ButtonInfo buttonInfo;

         buttonInfo.button = stringToJoystickButton(buttonKeyNames[i]);
         buttonInfo.label = buttonInfoMap["Label"];
         buttonInfo.color = stringToColor(buttonInfoMap["Color"]);
         buttonInfo.buttonShape = buttonLabelToButtonShape(buttonInfoMap["Shape"]);
         buttonInfo.buttonSymbol = stringToButtonSymbol(buttonInfoMap["Label"]);
         buttonInfo.sdlButton = buttonInfoMap["Raw"] == "" ? FakeRawButton : U8(stoi(buttonInfoMap["Raw"]));

         // Set the button info with index of the Joystick::Button
         joystickInfo.buttonMappings[buttonInfo.button] = buttonInfo;
      }

      JoystickPresetList.push_back(joystickInfo);
   }

   // Now add a generic joystick for a fall back
   JoystickPresetList.push_back(getGenericJoystickInfo());
}


} /* namespace Zap */
