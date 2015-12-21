//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Joystick.h"
#include "GameSettings.h"
#include "IniFile.h"
#include "Colors.h"

#include "stringUtils.h"

#include "tnlLog.h"

#include "SDL.h"
#include "SDL_stdinc.h"

#include <map>


namespace Zap {


// Linker needs these declared like this, why?
// private
SDL_GameController *Joystick::sdlController = NULL;

// public
Vector<Joystick::JoystickInfo> Joystick::JoystickPresetList;

U32 Joystick::ButtonMask = 0;
S16 Joystick::rawAxesValues[SDL_CONTROLLER_AXIS_MAX]; // Array of the current axes values
S16 Joystick::LowerSensitivityThreshold = 4900;   // out of 32767, ~15%, any less than this is ends up as zero
S16 Joystick::UpperSensitivityThreshold = 30000;  // out of 32767, ~91%, any more than this is full amount

U32 Joystick::SelectedPresetIndex = 0;    // TODO: This should be non-static on ClientGame, I think


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


// Make sure "SDL_Init(0)" was done before calling this function, otherwise joystick will fail to work on windows.
bool Joystick::initJoystick(GameSettings *settings)
{
   GameSettings::DetectedControllerList.clear();

   // Allows multiple joysticks with each using a copy of Bitfighter
   // FIXME: If this still works, then great!  If not, we may need to set it
   // *before* SDL_Init(0) in main.cpp
   SDL_setenv("SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS", "1", 0);

   if(!SDL_WasInit(SDL_INIT_GAMECONTROLLER) &&
         SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER))
   {
      logprintf("Unable to initialize the game controller subsystem");
      return false;
   }

   // Load the controller database
   SDL_GameControllerAddMappingsFromFile(
         joindir(settings->getFolderManager()->getIniDir(), "gamecontrollerdb.txt").c_str()
         );

   // TODO Add user-specific gamecontroller database?

   // How many joysticks are there
   S32 joystickCount = SDL_NumJoysticks();

   // No joysticks found
   if(joystickCount <= 0)
      return false;

   logprintf("%d joystick(s) detected:", joystickCount);

   for(S32 i = 0; i < joystickCount; i++)
   {
      // A GameController is a specific type of joystick
      if(SDL_IsGameController(i))
      {
         const char *controllerName = SDL_GameControllerNameForIndex(i);

         logprintf("  %d. [GameController] \"%s\"", i + 1, controllerName);
         GameSettings::DetectedControllerList.insert(pair<S32,string>(i,controllerName));
      }

      // Not detected as a game controller
      else
      {
         const char *joystickName = SDL_JoystickNameForIndex(i);

         logprintf("  %d. [Joystick] (not compatible) \"%s\"", i + 1, joystickName);

         // TODO: Do some sort of auto-detection and string output of the
         // joystick hardware mappings and create a gamecontroller out of this
         // joystick.  Maybe integrate SDL/test/controllermap.c from SDL hg
      }
   }

   // Set the controller number we will use during the game unless it was already
   // set by a command line arg in GameSettings.cpp.  This will be the first
   // detected controller from above.
   if(GameSettings::UseControllerIndex == -1)
      GameSettings::UseControllerIndex = GameSettings::DetectedControllerList.begin()->first;

   return true;
}


bool Joystick::enableJoystick(GameSettings *settings, bool hasBeenOpenedBefore)
{
   // Need to close the controller to avoid having 2 being active at the same time
   if(sdlController != NULL) {
      SDL_GameControllerClose(sdlController);
      sdlController = NULL;
   }

   // Check that there is a controller available
   if(GameSettings::DetectedControllerList.size() == 0)
      return false;

   // Don't enable controller at all in keyboard mode
   if(settings->getInputMode() == InputModeKeyboard &&
        (hasBeenOpenedBefore || settings->getSetting<YesNo>(IniKey::AlwaysStartInKeyboardMode)))
         return true;

   // Enable controller events
   SDL_GameControllerEventState(SDL_ENABLE);

   // Start using the controller
   sdlController = SDL_GameControllerOpen(GameSettings::UseControllerIndex);
   if(sdlController == NULL)
   {
      logprintf("Error opening controller %d [%s]", GameSettings::UseControllerIndex, SDL_GameControllerNameForIndex(GameSettings::UseControllerIndex));

      return false;
   }

   logprintf("Using controller %d [%s]", GameSettings::UseControllerIndex, SDL_GameControllerNameForIndex(GameSettings::UseControllerIndex));

   // Now try and autodetect the joystick and update the game settings
   string joystickType = Joystick::autodetectJoystick(settings);

   // Set joystick type if we found anything
   // Otherwise, it makes more sense to remember what the user had last specified
   if(!hasBeenOpenedBefore && joystickType != NoJoystick)
   {
	   settings->setSetting(IniKey::JoystickType, joystickType);
      setSelectedPresetIndex(Joystick::getJoystickIndex(joystickType));
   }

   // Set primary input to joystick if any controllers were found, even a generic one
   if(hasBeenOpenedBefore)
      return true;  // Do nothing when this was opened before
   else if(joystickType == NoJoystick)
      settings->getInputCodeManager()->setInputMode(InputModeKeyboard);
   else
      settings->getInputCodeManager()->setInputMode(InputModeJoystick);

   return true;
}


void Joystick::shutdownJoystick()
{
   if(sdlController != NULL) {
      SDL_GameControllerClose(sdlController);
      sdlController = NULL;
   }

   if(SDL_WasInit(SDL_INIT_GAMECONTROLLER))
      SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
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
   if(GameSettings::DetectedControllerList.size() == 0)  // No controllers detected
      return NoJoystick;

   string controllerName = GameSettings::DetectedControllerList[GameSettings::UseControllerIndex];

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
   string lastStickUsed = settings->getSetting<string>(IniKey::JoystickType);

   // Let's validate that, shall we?
   for(S32 i = 0; i < JoystickPresetList.size(); i++)
      if(JoystickPresetList[i].identifier == lastStickUsed)
         return JoystickPresetList[i].identifier;

   // It's beyond hope!  Return something that will *ALWAYS* be wrong.
   return "GenericJoystick";
}


void Joystick::getAllJoystickPrettyNames(Vector<string> &nameList)
{
   for(S32 i = 0; i < JoystickPresetList.size(); i++)
      nameList.push_back(JoystickPresetList[i].name);
}


JoystickButton Joystick::stringToJoystickButton(const string &buttonString)
{
   if(buttonString == "Button1")
      return JoystickButton1;
   else if(buttonString == "Button2")
      return JoystickButton2;
   else if(buttonString == "Button3")
      return JoystickButton3;
   else if(buttonString == "Button4")
      return JoystickButton4;
   else if(buttonString == "Button5")
      return JoystickButton5;
   else if(buttonString == "Button6")
      return JoystickButton6;
   else if(buttonString == "Button7")
      return JoystickButton7;
   else if(buttonString == "Button8")
      return JoystickButton8;
   else if(buttonString == "Button9")
      return JoystickButton9;
   else if(buttonString == "Button10")
      return JoystickButton10;
   else if(buttonString == "Button11")
      return JoystickButton11;
   else if(buttonString == "Button12")
      return JoystickButton12;
   else if(buttonString == "ButtonStart")
      return JoystickButtonStart;
   else if(buttonString == "ButtonBack")
      return JoystickButtonBack;
   else if(buttonString == "ButtonDPadUp")
      return JoystickButtonDPadUp;
   else if(buttonString == "ButtonDPadDown")
      return JoystickButtonDPadDown;
   else if(buttonString == "ButtonDPadLeft")
      return JoystickButtonDPadLeft;
   else if(buttonString == "ButtonDPadRight")
      return JoystickButtonDPadRight;

   return JoystickButtonUnknown;
}


Joystick::ButtonShape Joystick::buttonLabelToButtonShape(const string &label)
{
   if(label == "Round")
      return ButtonShapeRound;
   else if(label == "Rect")
      return ButtonShapeRect;
   else if(label == "SmallRect")
      return ButtonShapeSmallRect;
   else if(label == "RoundedRect")
      return ButtonShapeRoundedRect;
   else if(label == "SmallRoundedRect")
      return ButtonShapeSmallRoundedRect;
   else if(label == "HorizEllipse")
      return ButtonShapeHorizEllipse;
   else if(label == "RightTriangle")
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
   else if(lower == "green")
      return Colors::green;
   else if(lower == "blue")
      return Colors::blue;
   else if(lower == "yellow")
      return Colors::yellow;
   else if(lower == "cyan")
      return Colors::cyan;
   else if(lower == "magenta")
      return Colors::magenta;
   else if(lower == "black")
      return Colors::black;
   else if(lower == "red")
      return Colors::red;
   else if(lower == "paleRed")
      return Colors::paleRed;
   else if(lower == "paleBlue")
      return Colors::paleBlue;
   else if(lower == "palePurple")
      return Colors::palePurple;
   else if(lower == "paleGreen")
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


bool Joystick::isButtonDefined(S32 presetIndex, S32 buttonIndex) 
{
   TNLAssert(buttonIndex >= 0 && buttonIndex < JoystickButtonCount, "Button index out of range!");

   return Joystick::JoystickPresetList[presetIndex].buttonMappings[buttonIndex].sdlButton != Joystick::FakeRawButton
      || Joystick::JoystickPresetList[presetIndex].buttonMappings[buttonIndex].rawAxis != Joystick::FakeRawButton;
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
   for(S32 i = 0; i < JoystickButtonCount; i++)
   {
      joystickInfo.buttonMappings[i].button = (JoystickButton)i;  // 'i' should be in line with JoystickButton
      joystickInfo.buttonMappings[i].label = "";
      joystickInfo.buttonMappings[i].color = Colors::white;
      joystickInfo.buttonMappings[i].buttonShape = ButtonShapeRound;
      joystickInfo.buttonMappings[i].buttonSymbol = ButtonSymbolNone;
   }

   // Add some labels
   for(S32 i = 0; i < 8; i++)
   {
      joystickInfo.buttonMappings[i].label = itos(i + 1);
      joystickInfo.buttonMappings[i].sdlButton = i;
   }

   joystickInfo.buttonMappings[JoystickButtonBack].label = "9";
   joystickInfo.buttonMappings[JoystickButtonBack].sdlButton = 9;

   joystickInfo.buttonMappings[JoystickButtonStart].label = "10";
   joystickInfo.buttonMappings[JoystickButtonStart].sdlButton = 10;
   
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
   joystickPresetsINI.SetPath(joindir(settings->getFolderManager()->getIniDir(), "joystick_presets.ini"));
   joystickPresetsINI.ReadFile();

   // Loop through each section (each section is a joystick) and parse
   for (S32 sectionId = 0; sectionId < joystickPresetsINI.GetNumSections(); sectionId++)
   {
      JoystickInfo joystickInfo;

      // Names names names
      joystickInfo.identifier              = joystickPresetsINI.sectionName(sectionId).c_str();
      joystickInfo.name                    = joystickPresetsINI.GetValue   (sectionId, "Name").c_str();
      joystickInfo.searchString            = joystickPresetsINI.GetValue   (sectionId, "SearchString").c_str();
      joystickInfo.isSearchStringSubstring = joystickPresetsINI.GetValueYN (sectionId, "SearchStringIsSubstring", false);

      TNLAssert(
            (lcase(joystickPresetsINI.GetValue(sectionId, "SearchStringIsSubstring")) == "yes") ==
            joystickPresetsINI.GetValueYN(sectionId, "SearchStringIsSubstring", false),
            "Should be equal... can delete this assert after a mid June 2013 or so...");


      // Axis of evil
      joystickInfo.moveAxesSdlIndex[0]  = atoi(joystickPresetsINI.GetValue(sectionId, "MoveAxisLeftRight").c_str());
      joystickInfo.moveAxesSdlIndex[1]  = atoi(joystickPresetsINI.GetValue(sectionId, "MoveAxisUpDown").c_str());
      joystickInfo.shootAxesSdlIndex[0] = atoi(joystickPresetsINI.GetValue(sectionId, "ShootAxisLeftRight").c_str());
      joystickInfo.shootAxesSdlIndex[1] = atoi(joystickPresetsINI.GetValue(sectionId, "ShootAxisUpDown").c_str());

      Vector<string> sectionKeys;
      joystickPresetsINI.GetAllKeys(sectionId, sectionKeys);

      // Start the search for Button-related keys
      // Button4=Raw:3;Label:4;Color:White;Shape:Round
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

         buttonInfo.button = stringToJoystickButton(buttonKeyNames[i]); // Converts "Button3" to JoystickButton3

         // Our button was not detected properly (misspelling?)
         if(buttonInfo.button == JoystickButtonUnknown)
         {
            string message = "Joystick preset button not found: " + buttonKeyNames[i];
            settings->addConfigurationError(message);
            logprintf(LogConsumer::ConfigurationError, message.c_str());

            continue;      // On to the next button
         }

         buttonInfo.label = buttonInfoMap["Label"];
         buttonInfo.color = stringToColor(buttonInfoMap["Color"]);
         buttonInfo.buttonShape = buttonLabelToButtonShape(buttonInfoMap["Shape"]);
         buttonInfo.buttonSymbol = stringToButtonSymbol(buttonInfoMap["Label"]);
         buttonInfo.sdlButton = buttonInfoMap["Raw"] == "" ? FakeRawButton : U8(atoi(buttonInfoMap["Raw"].c_str()));
         buttonInfo.rawAxis = buttonInfoMap["Axis"] == "" ? FakeRawButton : U8(atoi(buttonInfoMap["Axis"].c_str()));

         // Set the button info with index of the JoystickButton
         joystickInfo.buttonMappings[buttonInfo.button] = buttonInfo;
      }

      JoystickPresetList.push_back(joystickInfo);
   }

   // Now add a generic joystick for a fall back
   JoystickPresetList.push_back(getGenericJoystickInfo());
}


Joystick::ButtonInfo::ButtonInfo() {
   button = JoystickButtonUnknown;
   sdlButton = FakeRawButton;
   rawAxis = FakeRawButton;
   label = "";
   color = Colors::white;
   buttonShape = ButtonShapeRound;
   buttonSymbol = ButtonSymbolNone;
}


} /* namespace Zap */
