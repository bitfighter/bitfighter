//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "InputCode.h"
#include "GameSettings.h"        // For access to UseJoystickNumber static

#include "stringUtils.h"         // For itos, isPrintable

#include "tnlLog.h"              // For logprintf

#ifndef ZAP_DEDICATED
#  include "SDL.h"
#  include "Joystick.h"
#endif


#ifdef TNL_OS_WIN32
#  include <windows.h>           // For ARRAYSIZE 
#endif


using namespace TNL;

namespace Zap
{

static bool inputCodeIsDown[MAX_INPUT_CODES];


// Constructor
BindingSet::BindingSet()
{
   // These bindings will be overwritten by config::setDefaultKeyBindings()...
   // we provide default values here just for the sake of sanity.  And testing.
   // Sanity and testing.  And just because.  Sanity, testing, and just because.
   // Also remember that we have multiple BindingSets (one for keyboard, one for
   // joystick, for example), so these defaults may not even apply in all cases.


   // Generates a block of code that looks like this:
   // inputSELWEAP1 = KEY_1;
   // inputSELWEAP2 = KEY_2;
   // ...

#define BINDING(a, b, memberName, defaultKeyboardBinding, e)  memberName = defaultKeyboardBinding;
    BINDING_TABLE
#undef BINDING
}


// Destructor
BindingSet::~BindingSet()
{
   // Do nothing
}


InputCode BindingSet::getBinding(BindingNameEnum bindingName) const
{
   // Produces a block of code that looks like this:
   // if(false) { }
   // else if(bindingName == BINDING_SELWEAP1) return inputSELWEAP1;
   // else if...
   // TNLAssert(false);
   // return KEY_NONE;

    if(false) { }     // Dummy conditional to let us use else if below
#define BINDING(enumName, b, memberName, d, e) else if(bindingName == enumName) return memberName;
    BINDING_TABLE
#undef BINDING

   // Just in case:
   TNLAssert(false, "Invalid key binding!");
   return KEY_NONE;
}


void BindingSet::setBinding(BindingNameEnum bindingName, InputCode key)
{
   // Produces a block of code that looks like this:
   // if(false) { }
   // else if(bindingName == BINDING_SELWEAP1) inputSELWEAP1 = key;
   // else if...
   // else TNLAssert(false);

   if(false) { }     // Dummy conditional to let us use else if below
#define BINDING(enumName, b, memberName, d, e) else if(bindingName == enumName) memberName = key;
    BINDING_TABLE
#undef BINDING
   else 
      TNLAssert(false, "Invalid key binding!");
}


// Return true if any bound keys are explicitly mapped to the numeric keypad
bool BindingSet::hasKeypad() const
{
   // Generates a statement that looks like:
   // return InputCodeManager::isKeypadKey(inputSELWEAP1) || 
   //        InputCodeManager::isKeypadKey(inputSELWEAP2) || 
   //        ...
   //        false;
   return 
#define BINDING(a, b, memberName, d, e) InputCodeManager::isKeypadKey(memberName) ||
    BINDING_TABLE
#undef BINDING
    false;
}


////////////////////////////////////////
////////////////////////////////////////

// Generates an array of bindingNames for the game
static const string BindingNames[] = {
#define BINDING(a, bindingName, c, d, e) bindingName, 
    BINDING_TABLE
#undef BINDING
};


// Generates an array of bindingNames for the editor
static const string EditorBindingNames[] = {
#define EDITOR_BINDING(a, bindingName, c, d) bindingName,
    EDITOR_BINDING_TABLE
#undef EDITOR_BINDING
};


// Generates an array of bindingCodes for the editor
static const string EditorBindingCodeNames[] = {
#define EDITOR_BINDING(a, bindingName, c, d) bindingName,
   EDITOR_BINDING_KEYCODE_TABLE
#undef EDITOR_BINDING
};

// Generates an array of bindingNames for the special keys
static const string SpecialBindingNames[] = {
#define SPECIAL_BINDING(a, bindingName, c, d, e) bindingName,
    SPECIAL_BINDING_TABLE
#undef SPECIAL_BINDING
};

////////////////////////////////////////
////////////////////////////////////////

// Constructor
EditorBindingSet::EditorBindingSet()
{
   // These bindings will be overwritten by config::setDefaultEditorKeyBindings()...
   // we provide default values here just for the sake of sanity.  And testing.
   // Sanity and testing.  And just because.  Sanity, testing, and just because.

   // Generates a block of code that looks like this:
   // FlipItemHorizontal = "H";
   // PasteSelection = "Ctrl+V";
   // ...

#define EDITOR_BINDING(a, b, memberName, defaultBinding) memberName = defaultBinding;
    EDITOR_BINDING_TABLE
#undef EDITOR_BINDING

#define EDITOR_BINDING(a, b, memberName, defaultBinding) memberName = defaultBinding;
       EDITOR_BINDING_KEYCODE_TABLE
#undef EDITOR_BINDING
}


EditorBindingSet::~EditorBindingSet()
{
   // Do nothing
}


string EditorBindingSet::getBinding(EditorBindingNameEnum bindingName) const
{
   // Produces a block of code that looks like this:
   // if(editorBindingName == BINDING_FLIP_HORIZ) return keyFlipItemHoriz;
   // if...
   // TNLAssert(false);
   // return "";

#define EDITOR_BINDING(enumName, b, memberName, d) if(bindingName == enumName) return memberName;
   EDITOR_BINDING_TABLE
#undef EDITOR_BINDING
   // Just in case:
   TNLAssert(false, "Invalid key binding!");
   return "";
}


InputCode EditorBindingSet::getBinding(EditorBindingCodeEnum bindingName) const
{
   // Similar to above. Produces a block of code that looks like this:
   // if(editorBindingName == BINDING_NO_GRID_SNAPPING) return inputDisableGridSnapping;
   // if...
   // TNLAssert(false);
   // return "";

#define EDITOR_BINDING(enumName, b, memberName, d) if(bindingName == enumName) return memberName;
   EDITOR_BINDING_KEYCODE_TABLE
#undef EDITOR_BINDING
   // Just in case:
   TNLAssert(false, "Invalid key binding!");
   return KEY_NONE;
}


void EditorBindingSet::setBinding(EditorBindingNameEnum bindingName, const string &key)
{
     // Produces a block of code that looks like this:
     // if(false) { }
     // else if(bindingName == BINDING_FLIP_HORIZ) keyFlipItemHoriz = key;
     // else if...
     // else TNLAssert(false);
     if(false) { }     // Dummy conditional to let us use else if below
#define EDITOR_BINDING(enumName, b, memberName, d) else if(bindingName == enumName) memberName = key;
    EDITOR_BINDING_TABLE
#undef EDITOR_BINDING
   else 
      TNLAssert(false, "Invalid key binding!");
}


void EditorBindingSet::setBinding(EditorBindingCodeEnum bindingName, InputCode code)
{
     // Produces a block of code that looks like this:
     // if(false) { }
     // else if(bindingName == BINDING_FLIP_HORIZ) keyFlipItemHoriz = key;
     // else if...
     // else TNLAssert(false);
     if(false) { }     // Dummy conditional to let us use else if below
#define EDITOR_BINDING(enumName, b, memberName, d) else if(bindingName == enumName) memberName = code;
     EDITOR_BINDING_KEYCODE_TABLE
#undef EDITOR_BINDING
   else 
      TNLAssert(false, "Invalid key binding!");
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SpecialBindingSet::SpecialBindingSet()
{
   // These bindings will be overwritten by config::setDefaultSpecialKeyBindings()...
   // we provide default values here just for the sake of sanity.  And testing.
   // Sanity and testing.  And just because.  Sanity, testing, and just because.
   // We have one set of binings that apply everywhere; some keys may allow multiple 
   // definitions so that one can be bound to a joystick button.

   // Generates a block of code that looks like this:
   // Screenshot_1 = "Ctrl+Q";
   // Screenshot_2 = "Ctrl+Q";
   // ...

#define SPECIAL_BINDING(a, b, memberName, defaultBinding, e) memberName = defaultBinding;
    SPECIAL_BINDING_TABLE
#undef SPECIAL_BINDING
}


SpecialBindingSet::~SpecialBindingSet()
{
   // Do nothing
}


string SpecialBindingSet::getBinding(SpecialBindingNameEnum bindingName) const
{
   // Produces a block of code that looks like this:
   // if(bindingName == BINDING_SCREENSHOT_1) return keyScreenshot1;
   // if...
   // TNLAssert(false);
   // return "";

#define SPECIAL_BINDING(enumName, b, memberName, d, e) if(bindingName == enumName) return memberName;
   SPECIAL_BINDING_TABLE
#undef SPECIAL_BINDING
   // Just in case:
   TNLAssert(false, "Invalid key binding!");
   return "";
}


void SpecialBindingSet::setBinding(SpecialBindingNameEnum bindingName, const string &key)
{
     // Produces a block of code that looks like this:
     // if(false) { }
     // else if(bindingName == BINDING_SCREENSHOT_1) keyScreenshot1 = key;
     // else if...
     // else TNLAssert(false);
     if(false) { }     // Dummy conditional to let us use else if below
#define SPECIAL_BINDING(enumName, b, memberName, d, e) else if(bindingName == enumName) memberName = key;
    SPECIAL_BINDING_TABLE
#undef SPECIAL_BINDING
   else 
      TNLAssert(false, "Invalid key binding!");
}


////////////////////////////////////////
////////////////////////////////////////

static Vector<InputCode> modifiers;


// Constructor
InputCodeManager::InputCodeManager()
{
   modifiers.clear();
   mBindingsHaveKeypadEntry = false;
   mInputMode = InputModeKeyboard;

   // Create two binding sets, one for keyboard controls, one for joystick
   mBindingSets.resize(2); 
   mSpecialBindingSets.resize(2);

   for(U32 i = FIRST_MODIFIER; i <= LAST_MODIFIER; i++)
      modifiers.push_back(InputCode(i));

   TNLAssert(modifiers.size() == 5, "Delete this assert whenever... it has already served its purpose.");
}


// Destructor
InputCodeManager::~InputCodeManager()
{
   // Do nothing
}


// Initialize state of keys... assume none are depressed, or even sad
void InputCodeManager::resetStates()
{
   for(S32 i = 0; i < MAX_INPUT_CODES; i++)
      inputCodeIsDown[i] = false;
}


// Prints list of any input codes that are down, for debugging purposes
void InputCodeManager::dumpInputCodeStates()
{
  for(S32 i = 0; i < MAX_INPUT_CODES; i++)
     if(inputCodeIsDown[i])
        logprintf("Key %s down", inputCodeToString((InputCode) i));
}


// Set state of a input code as Up (false) or Down (true)
void InputCodeManager::setState(InputCode inputCode, bool state)
{  
   inputCodeIsDown[(S32)inputCode] = state;
}


// Returns true of input code is down, false if it is up
bool InputCodeManager::getState(InputCode inputCode)
{
//   logprintf("State: key %d is %s", inputCode, keyIsDown[(int) inputCode] ? "down" : "up");
   return inputCodeIsDown[(S32)inputCode];
}


static const char InputStringJoiner = '+';

static InputCode getBaseKey(InputCode inputCode)
{
   // First, find the base key -- this will be the last non-modifier key we find, or one where the base key is the same as inputCode,
   // assuming the standard modifier-key combination
   for(S32 i = 0; i < MAX_INPUT_CODES; i++)
   {
      InputCode code = (InputCode)i;

      if(InputCodeManager::isKeyboardKey(code) && !InputCodeManager::isModifier(code) && InputCodeManager::getState(code))
         if(code == inputCode)
            return inputCode;

         // Otherwise, keep looking
   }

   return KEY_NONE;
}


// At any given time, for any combination of keys being pressed, there will be an official "input string" that looks a bit 
// like [Ctrl+T] or whatever.  
// This may be different than the keys actually being pressed.  For example, if the A and B keys are down, the inputString will be [A].
// In the event that two keys are both down, we'll prefer the one passed in inputCode, if possible.
// This generally works well most of the time, but may need to be cleaned up if it generates erroneous or misleading input strings.
// Modifiers will appear in order specified in the list of InputCodes... i.e. Ctrl+Shift+A, not Shift+Ctrl+A
string InputCodeManager::getCurrentInputString(InputCode inputCode)
{
   InputCode baseKey = getBaseKey(inputCode);

   if(baseKey == KEY_NONE)
      return "";
      
   string inputString = "";

   for(S32 i = 0; i < S32(modifiers.size()); i++)
      if(getState(modifiers[i]))
         inputString += string(inputCodeToString(modifiers[i])) + InputStringJoiner;
   
   inputString += inputCodeToString(baseKey);
   return inputString;
}


// Can pass in one of the above, or KEY_NONE to check if no modifiers are pressed
bool InputCodeManager::checkModifier(InputCode mod1)    
{
   S32 foundCount = 0;

   for(S32 i = 0; i < S32(modifiers.size()); i++)
      if(getState(modifiers[i]))                   // Modifier is down
      {
         if(modifiers[i] == mod1)      
            foundCount++;
         else                                      // Wrong modifier!               
            return false;        
      }

   return mod1 == KEY_NONE || foundCount == 1;
}


// Check if two modifiers are both pressed (i.e. Ctrl+Alt)
bool InputCodeManager::checkModifier(InputCode mod1, InputCode mod2)
{
   S32 foundCount = 0;

   for(S32 i = 0; i < S32(modifiers.size()); i++)
      if(getState(modifiers[i]))                   // Modifier is down
      {
         if(modifiers[i] == mod1 || modifiers[i] == mod2)      
            foundCount++;
         else                                      // Wrong modifier!               
            return false;        
      }

   return foundCount == 2;
}


// Check to see if three modifers are all pressed (i.e. Ctrl+Alt+Shift)
bool InputCodeManager::checkModifier(InputCode mod1, InputCode mod2, InputCode mod3)
{
   S32 foundCount = 0;

   for(S32 i = 0; i < S32(modifiers.size()); i++)
      if(getState(modifiers[i]))                   // Modifier is down
      {
         if(modifiers[i] == mod1 || modifiers[i] == mod2 || modifiers[i] == mod3)      
            foundCount++;
         else                                      // Wrong modifier!               
            return false;        
      }

   return foundCount == 3;
}


// Array tying InputCodes to string representations; used for translating one to the other 
static const char *keyNames[KEY_COUNT];
static Vector<string> modifierNames;


// Returns "" if inputString is unparsable
string InputCodeManager::normalizeInputString(const string &inputString)
{
   static const string INVALID = "";
   Vector<string> words;
   parseString(inputString, words, InputStringJoiner);

   // Modifiers will be first words... sort them, normalize capitalization, get them organized
   Vector<bool> hasModifier;
   hasModifier.resize(modifiers.size());

   for(S32 i = 0; i < modifiers.size(); i++)
      hasModifier[i] = false;

   for(S32 i = 0; i < words.size() - 1; i++)
   {
      InputCode inputCode = stringToInputCode(words[i].c_str());
      if(inputCode == KEY_UNKNOWN)     // Encountered something unexpected
         return INVALID;

      bool found = false;
      for(S32 j = 0; j < modifiers.size(); j++)
         if(inputCode == modifiers[j])
         {
            hasModifier[j] = true;
            found = true;
            break;
         }

      if(!found)     // InputCode we found was not a modifier, but was in a modifier position
         return INVALID;
   }

   // Now examine base key itself
   InputCode baseCode = stringToInputCode(words.last().c_str());
   if(baseCode == KEY_UNKNOWN)     // Unknown base key
      return INVALID;

   // baseCode cannot be a modifier -- "Ctrl" and "Alt+Shift" are not valid inputStrings
   if(isModifier(baseCode))
      return INVALID;

   string normalizedInputString = "";
   for(S32 i = 0; i < modifiers.size(); i++)
      if(hasModifier[i])
         normalizedInputString += string(keyNames[modifiers[i]]) + InputStringJoiner;

   normalizedInputString += string(keyNames[baseCode]);
   return normalizedInputString;
}


// A valid input string will consist of one or modifiers, separated by "+", followed by a valid inputCode.
// Modifier order and case are significant!!  Use normalizeInputString to get case and modifiers fixed up.
bool InputCodeManager::isValidInputString(const string &inputString)
{
   Vector<string> words;
   parseString(inputString, words, InputStringJoiner);

   const Vector<string> *mods = InputCodeManager::getModifierNames();


   S32 startMod = 0;    

   // Make sure all but the last word are modifiers
   for(S32 i = 0; i < words.size() - 1; i++)
   {
      bool found = false;
      for(S32 j = startMod; j < S32(modifiers.size()); j++)
         if(words[i] == mods->get(j))
         {
            found = true;
            startMod = j + 1;     // Helps ensure modifiers are in the correct order
            break;
         }

      if(!found)
         return false;
   }

   return stringToInputCode(words.last().c_str()) != KEY_UNKNOWN;
}


// It will be simpler if we translate joystick controls into keyboard actions here rather than check for them elsewhere.  
// This is possibly marginaly less efficient, but will reduce maintenance burdens over time.
InputCode InputCodeManager::convertJoystickToKeyboard(InputCode inputCode)
{
   switch((S32)inputCode)
   {
      case BUTTON_DPAD_LEFT:
         return KEY_LEFT;
      case BUTTON_DPAD_RIGHT:
         return KEY_RIGHT;
      case BUTTON_DPAD_UP:
         return KEY_UP;
      case BUTTON_DPAD_DOWN:
         return KEY_DOWN;

      case STICK_1_LEFT:
         return KEY_LEFT;
      case STICK_1_RIGHT:
         return KEY_RIGHT;
      case STICK_1_UP:
         return KEY_UP;
      case STICK_1_DOWN:
         return KEY_DOWN;

      case STICK_2_LEFT:
         return KEY_LEFT;
      case STICK_2_RIGHT:
         return KEY_RIGHT;
      case STICK_2_UP:
         return KEY_UP;
      case STICK_2_DOWN:
         return KEY_DOWN;

      case BUTTON_START:
         return KEY_ENTER;
      case BUTTON_BACK:
         return KEY_ESCAPE;
      case BUTTON_1:	    // Some game pads might not have a START button
         return KEY_ENTER;
      default:
         return inputCode;
   }
}


InputCode InputCodeManager::filterInputCode(InputCode inputCode)
{
   // We'll only apply numpad to standard key conversion if there are no keypad bindings
   if(mBindingsHaveKeypadEntry)
      return inputCode;

   return convertNumPadToNum(inputCode);
}


InputCode InputCodeManager::convertNumPadToNum(InputCode inputCode)
{
   switch(S32(inputCode))
   {
      case KEY_KEYPAD0:
	      return KEY_0;
      case KEY_KEYPAD1:
	      return KEY_1;
      case KEY_KEYPAD2:
	      return KEY_2;
      case KEY_KEYPAD3:
	      return KEY_3;
      case KEY_KEYPAD4:
	      return KEY_4;
      case KEY_KEYPAD5:
	      return KEY_5;
      case KEY_KEYPAD6:
	      return KEY_6;
      case KEY_KEYPAD7:
	      return KEY_7;
      case KEY_KEYPAD8:
	      return KEY_8;
      case KEY_KEYPAD9:
	      return KEY_9;
      case KEY_KEYPAD_PERIOD:
	      return KEY_PERIOD;
      case KEY_KEYPAD_DIVIDE:
	      return KEY_SLASH;
      case KEY_KEYPAD_MULTIPLY:
         return KEY_8;
      case KEY_KEYPAD_MINUS:
	      return KEY_MINUS;
      case KEY_KEYPAD_PLUS:
	      return KEY_PLUS;
      case KEY_KEYPAD_ENTER:
	      return KEY_ENTER;
      case KEY_KEYPAD_EQUALS:
	      return KEY_EQUALS;
      default:
         return inputCode;
   }
}


// If there is a printable ASCII code for the pressed key, return it
// Filter out some know spurious keystrokes
char InputCodeManager::keyToAscii(int unicode, InputCode inputCode)
{
   if((unicode & 0xFF80) != 0) 
      return 0;

   char ch = unicode & 0x7F;

   return isPrintable(ch) ? ch : 0;
}


// We'll be using this one most of the time
InputCode InputCodeManager::getBinding(BindingNameEnum bindingName) const
{
   return getBinding(bindingName, mInputMode);
}


// Only used for saving to INI and such where we need to bulk-read bindings
InputCode InputCodeManager::getBinding(BindingNameEnum bindingName, InputMode inputMode) const
{
   S32 mode = (S32)inputMode;    // 0 or 1 at present

   const BindingSet *bindingSet = &mBindingSets[mode];
   
   return bindingSet->getBinding(bindingName);
}


string InputCodeManager::getEditorBinding(EditorBindingNameEnum bindingName) const
{
   return mEditorBindingSet.getBinding(bindingName);
}


InputCode InputCodeManager::getEditorBinding(EditorBindingCodeEnum bindingName) const
{
   return mEditorBindingSet.getBinding(bindingName);
}


string InputCodeManager::getSpecialBinding(SpecialBindingNameEnum bindingName, InputMode inputMode) const
{
   S32 mode = (S32)inputMode;    // 0 or 1 at present

   const SpecialBindingSet *bindingSet = &mSpecialBindingSets[mode];
   
   return bindingSet->getBinding(bindingName);

}


void InputCodeManager::setBinding(BindingNameEnum bindingName, InputCode key)
{
   setBinding(bindingName, mInputMode, key);
}


void InputCodeManager::setBinding(BindingNameEnum bindingName, InputMode inputMode, InputCode key)
{
   S32 mode = (S32)inputMode;    // 0 or 1 at present

   BindingSet *bindingSet = &mBindingSets[mode];
   bindingSet->setBinding(bindingName, key);

   // Try to be efficient about checking for whether we have a keypad key assigned to something
   bool isKeypad = isKeypadKey(key);

   if(mBindingsHaveKeypadEntry != isKeypadKey(key))
   {
      if(isKeypad)
         mBindingsHaveKeypadEntry = true;
      else
         mBindingsHaveKeypadEntry = checkIfBindingsHaveKeypad(inputMode);
   }
}


void InputCodeManager::setEditorBinding(EditorBindingNameEnum bindingName, const string &inputString)
{
	mEditorBindingSet.setBinding(bindingName, inputString);
}


void InputCodeManager::setEditorBinding(EditorBindingCodeEnum bindingName, InputCode key)
{
	mEditorBindingSet.setBinding(bindingName, key);
}


// We'll be using this one most of the time
string InputCodeManager::getSpecialBinding(SpecialBindingNameEnum bindingName) const
{
   return getSpecialBinding(bindingName, mInputMode);
}


void InputCodeManager::setSpecialBinding(SpecialBindingNameEnum bindingName, InputMode inputMode, const string &inputString)
{
   S32 mode = (S32)inputMode;    // 0 or 1 at present

   SpecialBindingSet *bindingSet = &mSpecialBindingSets[mode];

	bindingSet->setBinding(bindingName, inputString);
}


void InputCodeManager::setInputMode(InputMode inputMode)
{
   mInputMode = inputMode;
   mBindingsHaveKeypadEntry = checkIfBindingsHaveKeypad(inputMode);
}


InputMode InputCodeManager::getInputMode() const
{
   return mInputMode;
}


// Returns display-friendly mode designator like "Keyboard" or "Joystick 1"
string InputCodeManager::getInputModeString() const
{
   return mInputMode == InputModeJoystick ?
         "Joystick " + itos(GameSettings::UseControllerIndex + 1) :    // Humans use 1-based indices!
         "Keyboard";
}


#ifndef ZAP_DEDICATED
// Translate SDL standard keys to our InputCodes
InputCode InputCodeManager::sdlKeyToInputCode(SDL_Keycode key)
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

		// TODO: SDL2 replacement for international keys, see SDL_Scancode

	   // Numeric keypad
	   case SDLK_KP_0:
		   return KEY_KEYPAD0;
	   case SDLK_KP_1:
		   return KEY_KEYPAD1;
	   case SDLK_KP_2:
		   return KEY_KEYPAD2;
	   case SDLK_KP_3:
		   return KEY_KEYPAD3;
	   case SDLK_KP_4:
		   return KEY_KEYPAD4;
	   case SDLK_KP_5:
		   return KEY_KEYPAD5;
	   case SDLK_KP_6:
		   return KEY_KEYPAD6;
	   case SDLK_KP_7:
		   return KEY_KEYPAD7;
	   case SDLK_KP_8:
		   return KEY_KEYPAD8;
	   case SDLK_KP_9:
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
	   case SDLK_NUMLOCKCLEAR:
		   return KEY_NUMLOCK;
	   case SDLK_CAPSLOCK:
		   return KEY_CAPSLOCK;
	   case SDLK_SCROLLLOCK:
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
	   case SDLK_RGUI:
	   case SDLK_LGUI:
		   return KEY_META;
	   case SDLK_MODE:
		   return KEY_MODE;
	   case SDLK_APPLICATION:
		   return KEY_COMPOSE;

	   // Miscellaneous function keys
	   case SDLK_HELP:
		   return KEY_HELP;
	   case SDLK_PRINTSCREEN:
		   return KEY_PRINT;
	   case SDLK_SYSREQ:
		   return KEY_SYSREQ;
      case SDLK_MENU:
         return KEY_MENU;
      case SDLK_POWER:
         return KEY_POWER;
	   case SDLK_UNDO:
		   return KEY_UNDO;

      // Identify some other keys we want to explicitly ignore without triggering the warning below
      case SDLK_VOLUMEUP:        
      case SDLK_VOLUMEDOWN:
      case SDLK_MUTE:
      case SDLK_AUDIONEXT:
      case SDLK_AUDIOPREV:
      case SDLK_AUDIOSTOP:
      case SDLK_AUDIOPLAY:
         return KEY_UNKNOWN;

      default:
         logprintf(LogConsumer::LogWarning, "Unknown key detected: %d", key);
         return KEY_UNKNOWN;
   }
}


SDL_Keycode InputCodeManager::inputCodeToSDLKey(InputCode inputCode)
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
		// TODO: SDL2 replacement for international keys, see SDL_Scancode

	   // Numeric keypad
	   case KEY_KEYPAD0:
		   return SDLK_KP_0;
	   case KEY_KEYPAD1:
		   return SDLK_KP_1;
	   case KEY_KEYPAD2:
		   return SDLK_KP_2;
	   case KEY_KEYPAD3:
		   return SDLK_KP_3;
	   case KEY_KEYPAD4:
		   return SDLK_KP_4;
	   case KEY_KEYPAD5:
		   return SDLK_KP_5;
	   case KEY_KEYPAD6:
		   return SDLK_KP_6;
	   case KEY_KEYPAD7:
		   return SDLK_KP_7;
	   case KEY_KEYPAD8:
		   return SDLK_KP_8;
	   case KEY_KEYPAD9:
		   return SDLK_KP_9;
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
		   return SDLK_NUMLOCKCLEAR;
	   case KEY_CAPSLOCK:
		   return SDLK_CAPSLOCK;
	   case KEY_SCROLLOCK:
		   return SDLK_SCROLLLOCK;
	   case KEY_MODE:
		   return SDLK_MODE;
	   case KEY_COMPOSE:
		   return SDLK_APPLICATION;

	   // Miscellaneous function keys
	   case KEY_HELP:
		   return SDLK_HELP;
	   case KEY_PRINT:
		   return SDLK_PRINTSCREEN;
	   case KEY_SYSREQ:
		   return SDLK_SYSREQ;
	   case KEY_MENU:
		   return SDLK_MENU;
	   case KEY_POWER:
		   return SDLK_POWER;
	   case KEY_UNDO:
		   return SDLK_UNDO;
      default:
         logprintf(LogConsumer::LogWarning, "Unknown inputCode detected: %d", inputCode);
         return SDLK_UNKNOWN;
   }
}


InputCode InputCodeManager::sdlControllerButtonToInputCode(U8 button)
{
   switch((SDL_GameControllerButton)button)
   {
      case SDL_CONTROLLER_BUTTON_A:
         return BUTTON_1;
      case SDL_CONTROLLER_BUTTON_B:
         return BUTTON_2;
      case SDL_CONTROLLER_BUTTON_X:
         return BUTTON_3;
      case SDL_CONTROLLER_BUTTON_Y:
         return BUTTON_4;
      case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
         return BUTTON_5;
      case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
         return BUTTON_6;
      case SDL_CONTROLLER_BUTTON_START:
         return BUTTON_START;
      case SDL_CONTROLLER_BUTTON_BACK:
         return BUTTON_BACK;
      case SDL_CONTROLLER_BUTTON_GUIDE:
         return BUTTON_GUIDE;
      case SDL_CONTROLLER_BUTTON_DPAD_UP:
         return BUTTON_DPAD_UP;
      case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
         return BUTTON_DPAD_DOWN;
      case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
         return BUTTON_DPAD_LEFT;
      case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
         return BUTTON_DPAD_RIGHT;
      case SDL_CONTROLLER_BUTTON_LEFTSTICK:
         return BUTTON_9;
      case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
         return BUTTON_10;

      default:
         return BUTTON_UNKNOWN;
   }
}


// This must return a signed value since it can return
//    SDL_CONTROLLER_BUTTON_INVALID = -1
S16 InputCodeManager::inputCodeToControllerButton(InputCode inputCode)
{
   switch(inputCode)
   {
      case BUTTON_1:
         return SDL_CONTROLLER_BUTTON_A;
      case BUTTON_2:
         return SDL_CONTROLLER_BUTTON_B;
      case BUTTON_3:
         return SDL_CONTROLLER_BUTTON_X;
      case BUTTON_4:
         return SDL_CONTROLLER_BUTTON_Y;
      case BUTTON_5:
         return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
      case BUTTON_6:
         return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
      case BUTTON_START:
         return SDL_CONTROLLER_BUTTON_START;
      case BUTTON_BACK:
         return SDL_CONTROLLER_BUTTON_BACK;
      case BUTTON_GUIDE:
         return SDL_CONTROLLER_BUTTON_GUIDE;
      case BUTTON_9:
         return SDL_CONTROLLER_BUTTON_LEFTSTICK;
      case BUTTON_10:
         return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
      case BUTTON_DPAD_UP:
         return SDL_CONTROLLER_BUTTON_DPAD_UP;
      case BUTTON_DPAD_DOWN:
         return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
      case BUTTON_DPAD_LEFT:
         return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
      case BUTTON_DPAD_RIGHT:
         return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;

      // Some buttons are 'hybrid' and come from other sources (like SDL axes)
      case BUTTON_TRIGGER_LEFT:
         return Joystick::ControllerButtonTriggerLeft;
      case BUTTON_TRIGGER_RIGHT:
         return Joystick::ControllerButtonTriggerRight;

      default:
         return SDL_CONTROLLER_BUTTON_INVALID;
   }
}
#endif


// We'll also treat controller buttons like simulated keystrokes
bool InputCodeManager::isControllerButton(InputCode inputCode)
{
   return inputCode >= FIRST_CONTROLLER_BUTTON && inputCode <= LAST_CONTROLLER_BUTTON;
}       


bool InputCodeManager::isKeypadKey(InputCode inputCode)
{
   return inputCode >= FIRST_KEYPAD_KEY && inputCode <= LAST_KEYPAD_KEY;
}


bool InputCodeManager::isKeyboardKey(InputCode inputCode)
{
   return inputCode >= FIRST_KEYBOARD_KEY && inputCode <= LAST_KEYBOARD_KEY;
}


bool InputCodeManager::isCtrlKey(InputCode inputCode)
{
   return inputCode >= FIRST_CTRL_KEY && inputCode <= LAST_CTRL_KEY;
}


bool InputCodeManager::isAltKey(InputCode inputCode)
{
   return inputCode >= FIRST_ALT_KEY && inputCode <= LAST_ALT_KEY;
}


bool InputCodeManager::isModified(InputCode inputCode)
{
   return isCtrlKey(inputCode) || isAltKey(inputCode);
}


bool InputCodeManager::isModifier(InputCode inputCode)
{
   return inputCode >= FIRST_MODIFIER && inputCode <= LAST_MODIFIER;
}


InputCode InputCodeManager::getModifier(InputCode inputCode)
{
   if(isCtrlKey(inputCode))
      return KEY_CTRL;
   
   if(isAltKey(inputCode))
      return KEY_ALT;
   
   return KEY_NONE;

   // TODO: Add other modifiers here as needed, then also fix up isModified()
}


string InputCodeManager::getModifierString(InputCode inputCode)
{
   return inputCodeToString(getModifier(inputCode));
}


const Vector<string> *InputCodeManager::getModifierNames()
{
   TNLAssert(modifierNames.size() > 0, "modifierNames has not been intialized!");
   return &modifierNames;
}


InputCode InputCodeManager::getBaseKeySpecialSequence(InputCode inputCode)
{
   if(isCtrlKey(inputCode))
      return (InputCode)((S32)inputCode - FIRST_CTRL_KEY + KEY_0);

   if(isAltKey(inputCode))
      return (InputCode)((S32)inputCode - FIRST_ALT_KEY + KEY_0);

   TNLAssert(false, "Unknown input code!");
   return KEY_UNKNOWN;
}


string InputCodeManager::getBaseKeyString(InputCode inputCode)
{
   return inputCodeToString(getBaseKeySpecialSequence(inputCode));
}


bool InputCodeManager::checkIfBindingsHaveKeypad(InputMode inputMode) const
{
   S32 mode = (S32)inputMode;    // 0 or 1 at present
   return mBindingSets[mode].hasKeypad();    
}


// Is inputCode related to the mouse?
bool InputCodeManager::isMouseAction(InputCode inputCode)
{
   return inputCode >= MOUSE_LEFT && inputCode <= MOUSE_WHEEL_DOWN;
}


string InputCodeManager::getBindingName(BindingNameEnum bindingName)
{
   U32 index = (U32)bindingName;
   TNLAssert(index >= 0 && index < ARRAYSIZE(BindingNames), "Invalid value for bindingName!");

   return BindingNames[index];
}


string InputCodeManager::getEditorBindingName(EditorBindingNameEnum binding)
{
   U32 index = (U32)binding;

   TNLAssert(index >= 0 && index < ARRAYSIZE(EditorBindingNames), "Invalid value for binding!");

   return EditorBindingNames[index];
}


string InputCodeManager::getEditorBindingName(EditorBindingCodeEnum binding)
{
   U32 index = (U32)binding;

   TNLAssert(index >= 0 && index < ARRAYSIZE(EditorBindingCodeNames), "Invalid value for binding!");

   return EditorBindingCodeNames[index];
}


string InputCodeManager::getSpecialBindingName(SpecialBindingNameEnum binding)
{
   U32 index = (U32)binding;

   TNLAssert(index >= 0 && index < ARRAYSIZE(SpecialBindingNames), "Invalid value for binding!");

   return SpecialBindingNames[index];
}


// i.e. return KEY_1 when passed "SelWeapon1"
InputCode InputCodeManager::getKeyBoundToBindingCodeName(const string &name) const
{
   // Linear search is not at all efficient, but this will be called very infrequently, in non-performance sensitive area
   // Note that for some reason the { }s are needed below... without them this code does not work right.
   for(U32 i = 0; i < ARRAYSIZE(BindingNames); i++)
   {
      if(caseInsensitiveStringCompare(BindingNames[i], name))
         return this->getBinding(BindingNameEnum(i));
   }

   return KEY_UNKNOWN;
}


// i.e. return "H" when passed "FlipItemHorizontal"
string InputCodeManager::getEditorKeyBoundToBindingCodeName(const string &name) const
{
   // Linear search is of O(n) speed and therefore is not really efficient, but this will be called very infrequently,
   // Note that for some reason the { }s are needed below... without them this code does not work right.
   for(U32 i = 0; i < ARRAYSIZE(EditorBindingNames); i++)
   {
      if(caseInsensitiveStringCompare(EditorBindingNames[i], name))
         return this->getEditorBinding(EditorBindingNameEnum(i));
   }

   return "";
}


// Same as above, but for our special binding set
string InputCodeManager::getSpecialKeyBoundToBindingCodeName(const string &name) const
{
   // Linear search is of O(n) speed and therefore is not really efficient, but this will be called very infrequently,
   // Note that for some reason the { }s are needed below... without them this code does not work right.
   for(U32 i = 0; i < ARRAYSIZE(SpecialBindingNames); i++)
   {
      if(caseInsensitiveStringCompare(SpecialBindingNames[i], name))
         return this->getSpecialBinding(SpecialBindingNameEnum(i));
   }

   return "";
}


// static method
Vector<string> InputCodeManager::getValidKeyCodes(S32 width)
{
   Vector<string> lines;

   string line = "";
   bool first = true;

   for(S32 i = 0; i < KEY_COUNT; i++)
   {
      string name = inputCodeToString(InputCode(i));

      if(name == UNKNOWN_KEY_NAME)      // Filter out "Unknown key" values
         continue;

      if(line.length() + name.length() + 2 < 115)     // 115 = width
      {
         if(!first)
            line += ", ";
         else
            first = false;

         line += name;
      }
      else
      {
         lines.push_back(line);
         line = name;
      }
   }

   lines.push_back(line);

   return lines;
}


// static method
string InputCodeManager::getValidModifiers()
{
   TNLAssert(modifiers.size() > 0, "Modifiers not initialized!");

   string modifierList = "";

   for(S32 i = 0; i < modifiers.size(); i++)
   {
      if(i != 0)
         modifierList += ", ";
      modifierList += inputCodeToString(modifiers[i]);
   }

   TNLAssert(modifierList.length() < 100, "This certainly grew... consider returning a vector of strings!");
   return modifierList;
}


// Return two examples of modified strings; the first is valid, the second invalid
// static method
pair<string,string> InputCodeManager::getExamplesOfModifiedKeys()
{
   TNLAssert(modifiers.size() > 0, "Modifiers not initialized!");

   pair<string,string> examples;

   string base = "X";

   examples.first  = string(inputCodeToString(modifiers[0])) + InputStringJoiner + string(inputCodeToString(modifiers[1])) + InputStringJoiner + base;
   examples.second = string(inputCodeToString(modifiers[1])) + InputStringJoiner + string(inputCodeToString(modifiers[0])) + InputStringJoiner + base;

   return examples;
}


// static method
void InputCodeManager::initializeKeyNames()
{
   // Fill name list with default value
   for(S32 i = 0; i < KEY_COUNT; i++)
      keyNames[i] = UNKNOWN_KEY_NAME;

   // Now keys we know with our locally defined names
   keyNames[S32(KEY_BACKSPACE)]       = "Backspace";        
   keyNames[S32(KEY_DELETE)]          = "Del";              
   keyNames[S32(KEY_TAB)]             = "Tab";              
   keyNames[S32(KEY_ENTER)]           = "Enter";            
   keyNames[S32(KEY_ESCAPE)]          = "Esc";              
   keyNames[S32(KEY_SPACE)]           = "Space";      // First keyboardchar          
   keyNames[S32(KEY_0)]               = "0";                
   keyNames[S32(KEY_1)]               = "1";                
   keyNames[S32(KEY_2)]               = "2";                
   keyNames[S32(KEY_3)]               = "3";                
   keyNames[S32(KEY_4)]               = "4";                
   keyNames[S32(KEY_5)]               = "5";                
   keyNames[S32(KEY_6)]               = "6";                
   keyNames[S32(KEY_7)]               = "7";                
   keyNames[S32(KEY_8)]               = "8";                
   keyNames[S32(KEY_9)]               = "9";                
   keyNames[S32(KEY_A)]               = "A";                
   keyNames[S32(KEY_B)]               = "B";                
   keyNames[S32(KEY_C)]               = "C";                
   keyNames[S32(KEY_D)]               = "D";                
   keyNames[S32(KEY_E)]               = "E";                
   keyNames[S32(KEY_F)]               = "F";                
   keyNames[S32(KEY_G)]               = "G";                
   keyNames[S32(KEY_H)]               = "H";                
   keyNames[S32(KEY_I)]               = "I";                
   keyNames[S32(KEY_J)]               = "J";
   keyNames[S32(KEY_K)]               = "K";
   keyNames[S32(KEY_L)]               = "L";
   keyNames[S32(KEY_M)]               = "M";                
   keyNames[S32(KEY_N)]               = "N";                
   keyNames[S32(KEY_O)]               = "O";                
   keyNames[S32(KEY_P)]               = "P";                
   keyNames[S32(KEY_Q)]               = "Q";                
   keyNames[S32(KEY_R)]               = "R";                
   keyNames[S32(KEY_S)]               = "S";                
   keyNames[S32(KEY_T)]               = "T";                
   keyNames[S32(KEY_U)]               = "U";                
   keyNames[S32(KEY_V)]               = "V";                
   keyNames[S32(KEY_W)]               = "W";                
   keyNames[S32(KEY_X)]               = "X";                
   keyNames[S32(KEY_Y)]               = "Y";                
   keyNames[S32(KEY_Z)]               = "Z";                
   keyNames[S32(KEY_TILDE)]           = "~";                
   keyNames[S32(KEY_MINUS)]           = "-"; 
   keyNames[S32(KEY_PLUS)]            = "+"; 
   keyNames[S32(KEY_EQUALS)]          = "=";                
   keyNames[S32(KEY_OPENBRACKET)]     = "[";                
   keyNames[S32(KEY_CLOSEBRACKET)]    = "]";                
   keyNames[S32(KEY_BACKSLASH)]       = "\\";               
   keyNames[S32(KEY_SEMICOLON)]       = ";";                
   keyNames[S32(KEY_QUOTE)]           = "'";                
   keyNames[S32(KEY_COMMA)]           = ",";                
   keyNames[S32(KEY_PERIOD)]          = ".";                
   keyNames[S32(KEY_SLASH)]           = "/";       // last keyboardchar   

   keyNames[S32(KEY_EXCLAIM)]         = "!";
   keyNames[S32(KEY_HASH)]            = "#";

   keyNames[S32(KEY_PAGEUP)]          = "Page Up";          
   keyNames[S32(KEY_PAGEDOWN)]        = "Page Down";        
   keyNames[S32(KEY_END)]             = "End";              
   keyNames[S32(KEY_HOME)]            = "Home";             
   keyNames[S32(KEY_LEFT)]            = "Left Arrow";       
   keyNames[S32(KEY_UP)]              = "Up Arrow";         
   keyNames[S32(KEY_RIGHT)]           = "Right Arrow";      
   keyNames[S32(KEY_DOWN)]            = "Down Arrow";       
   keyNames[S32(KEY_INSERT)]          = "Insert";           
   keyNames[S32(KEY_F1)]              = "F1";               
   keyNames[S32(KEY_F2)]              = "F2";               
   keyNames[S32(KEY_F3)]              = "F3";               
   keyNames[S32(KEY_F4)]              = "F4";               
   keyNames[S32(KEY_F5)]              = "F5";               
   keyNames[S32(KEY_F6)]              = "F6";               
   keyNames[S32(KEY_F7)]              = "F7";               
   keyNames[S32(KEY_F8)]              = "F8";               
   keyNames[S32(KEY_F9)]              = "F9";               
   keyNames[S32(KEY_F10)]             = "F10";              
   keyNames[S32(KEY_F11)]             = "F11";              
   keyNames[S32(KEY_F12)]             = "F12";  

   keyNames[S32(KEY_SHIFT)]           = "Shift";            
   keyNames[S32(KEY_ALT)]             = "Alt";              
   keyNames[S32(KEY_CTRL)]            = "Ctrl";             
   keyNames[S32(KEY_META)]            = "Meta";             
   keyNames[S32(KEY_SUPER)]           = "Super";  

   keyNames[S32(MOUSE_LEFT)]          = "Left-mouse";       
   keyNames[S32(MOUSE_MIDDLE)]        = "Middle-mouse";     
   keyNames[S32(MOUSE_RIGHT)]         = "Right-mouse";      
   keyNames[S32(MOUSE_WHEEL_UP)]      = "Mouse Wheel Up";   
   keyNames[S32(MOUSE_WHEEL_DOWN)]    = "Mouse Wheel Down"; 

   keyNames[S32(BUTTON_1)]            = "Button 1";         
   keyNames[S32(BUTTON_2)]            = "Button 2";         
   keyNames[S32(BUTTON_3)]            = "Button 3";         
   keyNames[S32(BUTTON_4)]            = "Button 4";         
   keyNames[S32(BUTTON_5)]            = "Button 5";         
   keyNames[S32(BUTTON_6)]            = "Button 6";         
   keyNames[S32(BUTTON_TRIGGER_LEFT)] = "L Trigger";
   keyNames[S32(BUTTON_TRIGGER_RIGHT)]= "R Trigger";
   keyNames[S32(BUTTON_9)]            = "Button 9";         
   keyNames[S32(BUTTON_10)]           = "Button 10";        
   keyNames[S32(BUTTON_GUIDE)]        = "Guide";
   keyNames[S32(BUTTON_BACK)]         = "Back";             
   keyNames[S32(BUTTON_START)]        = "Start";            
   keyNames[S32(BUTTON_DPAD_UP)]      = "DPad Up";          
   keyNames[S32(BUTTON_DPAD_DOWN)]    = "DPad Down";        
   keyNames[S32(BUTTON_DPAD_LEFT)]    = "DPad Left";        
   keyNames[S32(BUTTON_DPAD_RIGHT)]   = "DPad Right";       
   keyNames[S32(STICK_1_LEFT)]        = "Stick 1 Left";     
   keyNames[S32(STICK_1_RIGHT)]       = "Stick 1 Right";    
   keyNames[S32(STICK_1_UP)]          = "Stick 1 Up";       
   keyNames[S32(STICK_1_DOWN)]        = "Stick 1 Down";     
   keyNames[S32(STICK_2_LEFT)]        = "Stick 2 Left";     
   keyNames[S32(STICK_2_RIGHT)]       = "Stick 2 Right";    
   keyNames[S32(STICK_2_UP)]          = "Stick 2 Up";       
   keyNames[S32(STICK_2_DOWN)]        = "Stick 2 Down";  

   keyNames[S32(MOUSE)]               = "Mouse";            
   keyNames[S32(LEFT_JOYSTICK)]       = "Left joystick";    
   keyNames[S32(RIGHT_JOYSTICK)]      = "Right joystick"; 

   keyNames[S32(KEY_CTRL_A)]          = "Ctrl+A";                 // First ctrl key
   keyNames[S32(KEY_CTRL_B)]          = "Ctrl+B";
   keyNames[S32(KEY_CTRL_C)]          = "Ctrl+C";
   keyNames[S32(KEY_CTRL_D)]          = "Ctrl+D";
   keyNames[S32(KEY_CTRL_E)]          = "Ctrl+E";
   keyNames[S32(KEY_CTRL_F)]          = "Ctrl+F";
   keyNames[S32(KEY_CTRL_G)]          = "Ctrl+G";
   keyNames[S32(KEY_CTRL_H)]          = "Ctrl+H";
   keyNames[S32(KEY_CTRL_I)]          = "Ctrl+I";
   keyNames[S32(KEY_CTRL_J)]          = "Ctrl+J";
   keyNames[S32(KEY_CTRL_K)]          = "Ctrl+K";
   keyNames[S32(KEY_CTRL_L)]          = "Ctrl+L";
   keyNames[S32(KEY_CTRL_M)]          = "Ctrl+M";
   keyNames[S32(KEY_CTRL_N)]          = "Ctrl+N";
   keyNames[S32(KEY_CTRL_O)]          = "Ctrl+O";
   keyNames[S32(KEY_CTRL_P)]          = "Ctrl+P";
   keyNames[S32(KEY_CTRL_Q)]          = "Ctrl+Q";
   keyNames[S32(KEY_CTRL_R)]          = "Ctrl+R";
   keyNames[S32(KEY_CTRL_S)]          = "Ctrl+S";
   keyNames[S32(KEY_CTRL_T)]          = "Ctrl+T";
   keyNames[S32(KEY_CTRL_U)]          = "Ctrl+U";
   keyNames[S32(KEY_CTRL_V)]          = "Ctrl+V";
   keyNames[S32(KEY_CTRL_W)]          = "Ctrl+W";
   keyNames[S32(KEY_CTRL_X)]          = "Ctrl+X";
   keyNames[S32(KEY_CTRL_Y)]          = "Ctrl+Y";
   keyNames[S32(KEY_CTRL_Z)]          = "Ctrl+Z";
   keyNames[S32(KEY_CTRL_0)]          = "Ctrl+0";         
   keyNames[S32(KEY_CTRL_1)]          = "Ctrl+1";                 
   keyNames[S32(KEY_CTRL_2)]          = "Ctrl+2";                 
   keyNames[S32(KEY_CTRL_3)]          = "Ctrl+3";
   keyNames[S32(KEY_CTRL_4)]          = "Ctrl+4";
   keyNames[S32(KEY_CTRL_5)]          = "Ctrl+5";
   keyNames[S32(KEY_CTRL_6)]          = "Ctrl+6";
   keyNames[S32(KEY_CTRL_7)]          = "Ctrl+7";
   keyNames[S32(KEY_CTRL_8)]          = "Ctrl+8";
   keyNames[S32(KEY_CTRL_9)]          = "Ctrl+9";                 // Last ctrl key

   keyNames[S32(KEY_ALT_0)]           = "Alt+0";                  // First alt key
   keyNames[S32(KEY_ALT_1)]           = "Alt+1";                  
   keyNames[S32(KEY_ALT_2)]           = "Alt+2";                  
   keyNames[S32(KEY_ALT_3)]           = "Alt+3";                  
   keyNames[S32(KEY_ALT_4)]           = "Alt+4";                  
   keyNames[S32(KEY_ALT_5)]           = "Alt+5";                  
   keyNames[S32(KEY_ALT_6)]           = "Alt+6";                  
   keyNames[S32(KEY_ALT_7)]           = "Alt+7";                  
   keyNames[S32(KEY_ALT_8)]           = "Alt+8";                  
   keyNames[S32(KEY_ALT_9)]           = "Alt+9";                  // Last alt key

   keyNames[S32(KEY_BACKQUOTE)]       = "`";                
   keyNames[S32(KEY_MENU)]            = "Menu";             
   keyNames[S32(KEY_KEYPAD_DIVIDE)]   = "Keypad /";         
   keyNames[S32(KEY_KEYPAD_MULTIPLY)] = "Keypad *";         
   keyNames[S32(KEY_KEYPAD_MINUS)]    = "Keypad -";         
   keyNames[S32(KEY_KEYPAD_PLUS)]     = "Keypad +";         
   keyNames[S32(KEY_PRINT)]           = "PrntScrn";         
   keyNames[S32(KEY_PAUSE)]           = "Pause";            
   keyNames[S32(KEY_SCROLLOCK)]       = "ScrollLock";       
   keyNames[S32(KEY_KEYPAD1)]         = "Keypad 1";         
   keyNames[S32(KEY_KEYPAD2)]         = "Keypad 2";         
   keyNames[S32(KEY_KEYPAD3)]         = "Keypad 3";         
   keyNames[S32(KEY_KEYPAD4)]         = "Keypad 4";         
   keyNames[S32(KEY_KEYPAD5)]         = "Keypad 5";         
   keyNames[S32(KEY_KEYPAD6)]         = "Keypad 6";         
   keyNames[S32(KEY_KEYPAD7)]         = "Keypad 7";         
   keyNames[S32(KEY_KEYPAD8)]         = "Keypad 8";         
   keyNames[S32(KEY_KEYPAD9)]         = "Keypad 9";         
   keyNames[S32(KEY_KEYPAD0)]         = "Keypad 0";         
   keyNames[S32(KEY_KEYPAD_PERIOD)]   = "Keypad .";         
   keyNames[S32(KEY_KEYPAD_ENTER)]    = "Keypad Enter";     
   keyNames[S32(KEY_LESS)]            = "Less";    

   for(S32 i = 0; i < modifiers.size(); i++)
      modifierNames.push_back(keyNames[S32(modifiers[i])]);            
}


// Translate an InputCode into a string name, primarily used for displaying keys in
// help and during rebind mode, and also when storing key bindings in INI files
// Static method
const char *InputCodeManager::inputCodeToString(InputCode inputCode)
{
   TNLAssert(U32(inputCode) < U32(KEY_COUNT) || inputCode == KEY_UNKNOWN, "Invalid inputCode!");

   if(U32(inputCode) >= U32(KEY_COUNT))
      return "";

   return keyNames[S32(inputCode)];
}


// Translate from a string key name into a InputCode
// (primarily for loading key bindings from INI files)
InputCode InputCodeManager::stringToInputCode(const char *inputName)
{
   for(S32 i = 0; i < KEY_COUNT; i++)
      if(stricmp(inputName, keyNames[i]) == 0)
         return InputCode(i);

   return KEY_UNKNOWN;
}


const char *InputCodeManager::inputCodeToPrintableChar(InputCode inputCode)
{
   if(inputCode == KEY_SPACE || (inputCode >= FIRST_PRINTABLE_KEY && inputCode <= LAST_PRINTABLE_KEY))
      return inputCodeToString(inputCode);

   return "";
}

};
