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

#include "UIMenuItems.h"
#include "UIMenus.h"
#include "UIEditorMenus.h"

#include "ScreenInfo.h"    // For gScreenInfo stuff
#include "FontManager.h"

#include "LuaWrapper.h"
#include "LuaScriptRunner.h"

#include "Colors.h"

#include "stringUtils.h"
#include "RenderUtils.h"
#include "OpenglUtils.h"


namespace Zap
{

// Combined default C++ / Lua constructor
MenuItem::MenuItem(lua_State *L)
{
   initialize();
}


// Constructor
MenuItem::MenuItem(const string &displayVal)
{
   initialize();

   mDisplayVal = displayVal;
}


// Constructor
MenuItem::MenuItem(const string &displayVal, void (*callback)(ClientGame *, U32), const string &help, InputCode k1, InputCode k2)
{
   initialize();

   mDisplayVal = displayVal;
   mCallback = callback;
   mHelp = help;
   key1 = k1;
   key2 = k2;
}


// Constructor
MenuItem::MenuItem(S32 index, const string &displayVal, void (*callback)(ClientGame *, U32), 
                   const string &help, InputCode k1, InputCode k2)
{
   initialize();

   mDisplayVal = displayVal;
   mCallback = callback;
   mHelp = help;

   key1 = k1;
   key2 = k2;
   mIndex = (U32)index;
}


void MenuItem::initialize()
{
   mDisplayVal = "";
   key1 = KEY_UNKNOWN;
   key2 = KEY_UNKNOWN;
   mCallback = NULL;
   mHelp = "";
   mIndex = -1;
   mMenuItemSize = MENU_ITEM_SIZE_NORMAL;

   mEnterAdvancesItem = false;
   mSelectedColor = Colors::yellow;
   mUnselectedColor = Colors::white;
   mDisplayValAppendage = " >";

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
MenuItem::~MenuItem() 
{ 
   LUAW_DESTRUCTOR_CLEANUP;
} 


MenuItemTypes MenuItem::getItemType()
{
   return MenuItemType;
}


void MenuItem::setSize(MenuItemSize size)
{
   mMenuItemSize = size;
}


MenuItemSize MenuItem::getSize()
{
   return mMenuItemSize;
}


MenuUserInterface *MenuItem::getMenu()  
{ 
   return mMenu; 
}


S32 MenuItem::getIndex()
{
   return mIndex;
}


string MenuItem::getPrompt() const
{
   return mDisplayVal;
}


string MenuItem::getUnits() const
{
   return "";
}


void MenuItem::setSecret(bool secret)
{
   /* Do nothing */
}


const string MenuItem::getHelp()
{
   return mHelp;
}


void MenuItem::setMenu(MenuUserInterface *menu) 
{ 
   mMenu = menu; 
}


// Shouldn't need to be overridden -- all redering routines should include xpos
void MenuItem::render(S32 ypos, S32 textsize, bool isSelected)
{
   render(gScreenInfo.getGameCanvasWidth() / 2, ypos, textsize, isSelected);
}


const Color *MenuItem::getColor(bool isSelected)
{
   return isSelected ? &mSelectedColor : &mUnselectedColor;
}


void MenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   glColor(*getColor(isSelected));

   FontManager::pushFontContext(MenuContext);
      drawCenteredStringf(xpos, ypos, textsize, "%s%s", getPrompt().c_str(), mDisplayValAppendage);
   FontManager::popFontContext();
}


S32 MenuItem::getWidth(S32 textsize)
{
   return getStringWidthf(textsize, "%s%s", getPrompt().c_str(), mDisplayValAppendage);
}


bool MenuItem::handleKey(InputCode inputCode)
{
   if(inputCode == KEY_ENTER || inputCode == KEY_SPACE || inputCode == KEY_RIGHT || inputCode == MOUSE_LEFT)
   {
      UserInterface::playBoop();
      if(mCallback)
         mCallback(getMenu()->getGame(), mIndex);

      return true;
   }
   else
   {
      // Check individual entries for any shortcut keys
      return false;
   }
}


void MenuItem::handleTextInput(char ascii)
{
   // Do nothing
}


void MenuItem::setEnterAdvancesItem(bool enterAdvancesItem)
{
   mEnterAdvancesItem = enterAdvancesItem;
}


// Default implementations will be overridden by child classes
const char *MenuItem::getSpecialEditingInstructions() { return ""; } 
string      MenuItem::getValueForDisplayingInMenu()   { return ""; }
S32         MenuItem::getIntValue() const             { return 0;  }


void MenuItem::setValue(const string &val)                    { /* Do nothing */ }
void MenuItem::setIntValue(S32 val)                           { /* Do nothing */ }
void MenuItem::setFilter(LineEditor::LineEditorFilter filter) { /* Do nothing */ }


string MenuItem::getValueForWritingToLevelFile()
{
   return itos(getIntValue());
}


string MenuItem::getValue() const
{
   return mDisplayVal;
}


void MenuItem::activatedWithShortcutKey()
{
   handleKey(MOUSE_LEFT);
}


bool MenuItem::enterAdvancesItem()
{
   return mEnterAdvancesItem;
}


bool MenuItem::hasTextInput()
{
   return false;
}


void MenuItem::setSelectedColor(const Color &color)
{
   mSelectedColor = color;
}


void MenuItem::setUnselectedColor(const Color &color)
{
   mUnselectedColor = color;
}


void MenuItem::setSelectedValueColor(const Color &color)   { /* Override in children */ }
void MenuItem::setUnselectedValueColor(const Color &color) { /* Override in children */ }


/**
 *  @luaclass MenuItem
 *  @brief    Simple menu item that calls a method or opens a submenu when selected.
 *  @descr    %MenuItem is the parent class for all other MenuItems.  
 *
 *  Currently, you cannot instantiate a %MenuItem from Lua, though you can instatiate %MenuItem subclasses.
 */
//                Fn name                  Param profiles            Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_FUNARGS_TABLE(MenuItem, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(MenuItem, LUA_METHODS);

#undef LUA_METHODS


const char *MenuItem::luaClassName = "MenuItem";
REGISTER_LUA_CLASS(MenuItem);


////////////////////////////////////
////////////////////////////////////


MessageMenuItem::MessageMenuItem(string displayVal, const Color &color) : MenuItem(displayVal)
{
   mDisplayValAppendage = "";
   mUnselectedColor = color;
}


////////////////////////////////////
////////////////////////////////////

// Constructor
ValueMenuItem::ValueMenuItem()
{
   initialize();
}


// Constructor
ValueMenuItem::ValueMenuItem(const string &displayValue, void (*callback)(ClientGame *, U32), 
                             const string &help, InputCode k1, InputCode k2) :
      Parent(displayValue, callback, help, k1, k2)
{
   initialize();
}


void ValueMenuItem::initialize()
{
   mSelectedValueColor = Colors::cyan;
   mUnselectedValueColor = Colors::cyan;
}


S32 clamp(S32 val, S32 min, S32 max)
{
   if(val < min) return min;
   if(val > max) return max;
   return val;
}


const Color *ValueMenuItem::getValueColor(bool isSelected)
{
   return isSelected ? &mSelectedValueColor : &mUnselectedValueColor;
}


void ValueMenuItem::setSelectedValueColor(const Color &color)
{
   mSelectedValueColor = color;
}


void ValueMenuItem::setUnselectedValueColor(const Color &color)
{
   mUnselectedValueColor = color;
}


////////////////////////////////////
////////////////////////////////////

ToggleMenuItem::ToggleMenuItem()
{
   // Do nothing -- do not use this constructor, please!
}


// Constructor
ToggleMenuItem::ToggleMenuItem(string title, Vector<string> options, U32 currOption, bool wrap, 
                               void (*callback)(ClientGame *, U32), const string &help, InputCode k1, InputCode k2) :
      ValueMenuItem(title, callback, help, k1, k2)
{
   mOptions = options;
   mIndex = clamp(currOption, 0, mOptions.size() - 1);
   mWrap = wrap;
   mEnterAdvancesItem = true;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
ToggleMenuItem::~ToggleMenuItem()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


string ToggleMenuItem::getOptionText()
{
   return mIndex < U32(mOptions.size()) ? mOptions[mIndex] : "INDEX OUT OF RANGE";
}


void ToggleMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   drawCenteredStringPair(xpos, ypos, textsize, *getColor(isSelected), *getValueColor(isSelected), 
                          getPrompt().c_str(), getOptionText().c_str());
}


S32 ToggleMenuItem::getWidth(S32 textsize)
{
   return getStringPairWidth(textsize, getPrompt().c_str(), getOptionText().c_str());
}


bool ToggleMenuItem::handleKey(InputCode inputCode)
{
   U32 nextValAfterWrap = mWrap ? 0 : mIndex;

   if(inputCode == KEY_RIGHT || inputCode == MOUSE_LEFT || inputCode == MOUSE_WHEEL_DOWN)
   {
      mIndex = (mIndex == (U32)mOptions.size() - 1) ? nextValAfterWrap : mIndex + 1;

      if(mCallback)
         mCallback(getMenu()->getGame(), mIndex);

      UserInterface::playBoop();
      return true;
   }
   else if(inputCode == KEY_LEFT || inputCode == MOUSE_RIGHT || inputCode == MOUSE_WHEEL_UP)
   {      
      U32 nextValAfterWrap = mWrap ? mOptions.size() - 1 : mIndex;
      mIndex = (mIndex == 0) ? nextValAfterWrap : mIndex - 1;
      
      if(mCallback)
         mCallback(getMenu()->getGame(), mIndex);

      UserInterface::playBoop();
      return true;
   }

   else if(inputCode == KEY_ENTER || inputCode == KEY_SPACE)
   {
      mIndex = (mIndex == (U32)mOptions.size() - 1) ? nextValAfterWrap : mIndex + 1;

      if(mCallback)
         mCallback(getMenu()->getGame(), mIndex);

      UserInterface::playBoop();
      return true;
   }

   return false;
}


void ToggleMenuItem::handleTextInput(char ascii)
{
   if(ascii)     // Check for the first key of a menu entry.
      for(S32 i = 0; i < mOptions.size(); i++)
      {
         S32 index = (i + mIndex + 1) % mOptions.size();
         if(tolower(ascii) == tolower(mOptions[index].data()[0]))
         {
            mIndex = index;

            if(mCallback)
               mCallback(getMenu()->getGame(), mIndex);

            UserInterface::playBoop();
            return;
         }
      }
}


void ToggleMenuItem::activatedWithShortcutKey()
{
   /* Do nothing */
}


// Pulls values out of the table at specified index as strings, and puts them all into strings vector
static void getStringVectorFromTable(lua_State *L, S32 index, const char *methodName, Vector<string> &strings)
{
   strings.clear();

   if(!lua_istable(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected table arg (which I wanted to convert to a string vector) at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   // The following block loosely based on http://www.gamedev.net/topic/392970-lua-table-iteration-in-c---basic-walkthrough/

   lua_pushvalue(L, index);	// Push our table onto the top of the stack
   lua_pushnil(L);            // lua_next (below) will start the iteration, it needs nil to be the first key it pops

   // The table was pushed onto the stack at -1 (recall that -1 is equivalent to lua_gettop)
   // The lua_pushnil then pushed the table to -2, where it is currently located
   while(lua_next(L, -2))     // -2 is our table
   {
      // Grab the value at the top of the stack
      if(!lua_isstring(L, -1))
      {
         char msg[256];
         dSprintf(msg, sizeof(msg), "%s expected a table of strings -- invalid value at stack position %d, table element %d", 
                                    methodName, index, strings.size() + 1);
         logprintf(LogConsumer::LogError, msg);

         throw LuaException(msg);
      }

      strings.push_back(lua_tostring(L, -1));

      lua_pop(L, 1);    // We extracted that value, pop it off so we can push the next element
   }

   // We've got all the elements in the table, so clear it off the stack
   lua_pop(L, 1);
}


MenuItemTypes ToggleMenuItem::getItemType()
{
   return ToggleMenuItemType;
}


string ToggleMenuItem::getValueForDisplayingInMenu()
{
   return "";
}


const char *ToggleMenuItem::getSpecialEditingInstructions()
{
   return "Use [<-] and [->] keys or mouse wheel to change value.";
}


S32 ToggleMenuItem::getValueIndex()
{
   return mIndex;
}


void ToggleMenuItem::setValueIndex(U32 index)
{
   mIndex = index;
}


string ToggleMenuItem::getValue() const
{
   return mOptions[mIndex];
}


//////////
// Lua interface

/**
 *  @luaclass ToggleMenuItem
 *  @brief    Menu item that lets users choose one of several options.
 *  @descr    To create a %ToggleMenuItem from a plugin, use the following syntax:
 *
 *  \code
 * ToggleMenuItem.new(name, options, currentIndex, wrap, help)
 *  \endcode
 *
 *  @param name - A string representing the text shown on the menu item.
 *  @param options - A table of strings representing the options to be displayed.
 *  @param currentIndex - An integer representing the index of the item to be selected initially (1 = first item).
 *  @param wrap - bool, true if the items should wrap around when you reach the last index.
 *  @param help - A string with a bit of help text.
 *
 *  The %MenuItem will return the text of the item the user selected.
 *
 *  For example:
 *  \code
 *    m = ToggleMenuItem.new("Type", { "BarrierMaker", "LoadoutZone", "GoalZone" }, 1, true, "Type of item to insert")
 *  \endcode
 */
//                Fn name                  Param profiles            Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_FUNARGS_TABLE(ToggleMenuItem, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(ToggleMenuItem, LUA_METHODS);

#undef LUA_METHODS


const char *ToggleMenuItem::luaClassName = "ToggleMenuItem";
REGISTER_LUA_SUBCLASS(ToggleMenuItem, MenuItem);


// Lua Constructor, called from plugins
ToggleMenuItem::ToggleMenuItem(lua_State *L)
{
   const char *methodName = "ToggleMenuItem constructor";

   // Required items -- will throw if they are missing or misspecified
   mDisplayVal = getCheckedString(L, 1, methodName);
   getStringVectorFromTable(L, 2, methodName, mOptions);    // Fills mOptions with elements in a table 

   // Optional (but recommended) items
   mIndex = clamp(getInt(L, 3, 1) - 1, 0,  mOptions.size() - 1);   // First - 1 for compatibility with Lua's 1-based array index
   mWrap = getCheckedBool(L, 4, methodName, false);
   mHelp = getString(L, 4, "");

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


////////////////////////////////////
////////////////////////////////////

// Constructors
YesNoMenuItem::YesNoMenuItem(string title, bool currOption, const string &help, InputCode k1, InputCode k2) :
      ToggleMenuItem(title, Vector<string>(), currOption, true, NULL, help, k1, k2)
{
   initialize();

   setIndex(currOption);
}


// Destructor
YesNoMenuItem::~YesNoMenuItem()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


string YesNoMenuItem::getValueForDisplayingInMenu()
{
   TNLAssert(false, "Is this used?  If not, remove it!");
   return mIndex ? " Engineer" : "";
}


string YesNoMenuItem::getValueForWritingToLevelFile()
{
   return mIndex ? "yes" : "no";
}


void YesNoMenuItem::setValue(const string &val)
{
   mIndex = (val == "yes") ? 1 : 0;
}


S32 YesNoMenuItem::getIntValue() const
{
   return mIndex;
}


void YesNoMenuItem::setIntValue(S32 value)
{
   mIndex = (value == 0) ? 0 : 1;
}


void YesNoMenuItem::initialize()
{
   mEnterAdvancesItem = true;

   mOptions.push_back("No");     // 0
   mOptions.push_back("Yes");    // 1

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


void YesNoMenuItem::setIndex(S32 index)
{
   mIndex = clamp(index, 0, 1);
}


//////////
// Lua interface

/**
 *  @luaclass YesNoMenuItem
 *  @brief    A specialized ToggleMenuItem prepopulated with Yes and No.
 *  @descr    To create a %YesNoMenuItem from a plugin, use the following syntax:
 *
 *  \code
 * YesNoMenuItem.new(name, currentIndex, help)
 *  \endcode
 *
 *  @param name - A string representing the text shown on the menu item.
 *  @param currentIndex - An integer representing the index of the item to be selected initially (1 = Yes, 2 = No).
 *  @param help - A string with a bit of help text.
 *
 *  The %YesNoMenuItem will return 1 if the user selected Yes, 2 if No.
 *
 *  For example:
 *  \code
 *    m = YesNoMenuItem.new("Hostile", 1, "Should this turret be hostile?")
 *  \endcode
 */
//                Fn name                  Param profiles            Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_FUNARGS_TABLE(YesNoMenuItem, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(YesNoMenuItem, LUA_METHODS);

#undef LUA_METHODS


const char *YesNoMenuItem::luaClassName = "YesNoMenuItem";
REGISTER_LUA_SUBCLASS(YesNoMenuItem, ToggleMenuItem);


// Lua Constructor
YesNoMenuItem::YesNoMenuItem(lua_State *L)
{
   initialize();

   dumpStack(L);

   const char *methodName = "YesNoMenuItem constructor";

   // Required items -- will throw if they are missing or misspecified
   mDisplayVal = getCheckedString(L, 1, methodName);

   // Optional (but recommended) items
   setIndex(getInt(L, 2, 1) - 1);                // - 1 for compatibility with Lua's 1-based array index
   mHelp = getString(L, 3, "");
}


////////////////////////////////////
////////////////////////////////////

// Constructor
CounterMenuItem::CounterMenuItem(const string &title, S32 value, S32 step, S32 minVal, S32 maxVal, const string &units, 
                                 const string &minMsg, const string &help, InputCode k1, InputCode k2) :
   Parent(title, NULL, help, k1, k2)
{
   initialize();

   mStep = step;
   mMinValue = minVal;
   mMaxValue = maxVal;
   mUnits = units;
   mMinMsg = minMsg;   

   setIntValue(value);     // Needs to be done after mMinValue and mMaxValue are set
}


CounterMenuItem::CounterMenuItem()
{
   // Do nothing
}


// Destructor
CounterMenuItem::~CounterMenuItem()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


void CounterMenuItem::initialize()
{
   mEnterAdvancesItem = true;
   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


void CounterMenuItem::setValue(const string &val)
{
   setIntValue(atoi(val.c_str()));
}


void CounterMenuItem::setIntValue(S32 val)
{
   mValue = clamp(val, mMinValue, mMaxValue);
}


string CounterMenuItem::getOptionText()
{
   return (mValue == mMinValue && mMinMsg != "") ? mMinMsg : getValueForDisplayingInMenu() + " " + getUnits();
}


void CounterMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   drawCenteredStringPair(xpos, ypos, textsize, *getColor(isSelected), *getValueColor(isSelected), 
                          getPrompt().c_str(), getOptionText().c_str());
}


S32 CounterMenuItem::getWidth(S32 textsize)
{
   return getStringPairWidth(textsize, getPrompt().c_str(), getOptionText().c_str());
}


bool CounterMenuItem::handleKey(InputCode inputCode)
{
   if(inputCode == KEY_RIGHT || inputCode == MOUSE_LEFT || inputCode == MOUSE_WHEEL_UP)  
   {
      if(InputCodeManager::checkModifier(KEY_SHIFT))
      {
         increment(getBigIncrement());
         snap();
      }
      else
         increment(1);

      return true;
   }
   else if(inputCode == KEY_LEFT || inputCode == MOUSE_RIGHT || inputCode == MOUSE_WHEEL_DOWN)
   {
      if(InputCodeManager::checkModifier(KEY_SHIFT))
      {
         decrement(getBigIncrement());
         snap();
      }
      else
         decrement(1);

      return true;
   }

   return false;
}


void CounterMenuItem::increment(S32 fact) 
{ 
   setIntValue(mValue + mStep * fact);
}


void CounterMenuItem::decrement(S32 fact) 
{ 
   setIntValue(mValue - mStep * fact);
}


S32 CounterMenuItem::getBigIncrement()
{
   return 10;
}


MenuItemTypes CounterMenuItem::getItemType()
{
   return CounterMenuItemType;
}


string CounterMenuItem::getValueForDisplayingInMenu()
{
   return itos(mValue);
}


S32 CounterMenuItem::getIntValue() const
{
   return mValue;
}


string CounterMenuItem::getValue() const
{
   return itos(mValue);
}


const char *CounterMenuItem::getSpecialEditingInstructions()
{
   return "Use [<-] and [->] keys or mouse wheel to change value. Hold [Shift] for bigger change.";
}


string CounterMenuItem::getUnits() const
{
   return mUnits;
}


void CounterMenuItem::snap()
{
   /* Do nothing */
}


void CounterMenuItem::activatedWithShortcutKey()
{
   /* Do nothing */
}


//////////
// Lua interface

/**
 *  @luaclass CounterMenuItem
 *  @brief    Menu item for entering a numeric value, with increment and decrement controls.
 *
 *  \code
 * CounterMenuItem.new(name, startingVal, step, minVal, maxVal, units, minText, help)
 *  \endcode
 *
 *  @param name -        A string representing the text shown on the menu item.
 *  @param startingVal - A number representing the starting value of the menu item.     
 *  @param step -        A number specifying how much the value should increase or decrease when the arrow keys are used.
 *  @param minVal -      A number representing the minimum allowable value that can be entered.  
 *  @param maxVal -      A number representing the maximum allowable value that can be entered.  
 *  @param units -       A string representing the units to be shown alongside the numeric item.  Pass "" if you don't want to display units.
 *  @param minText -     A string representing the text shown on the menu item when the minimum value has been reached.  Pass "" to simply display the minimum value.  
 *  @param help -        A string with a bit of help text.
 *
 *  The %MenuItem will return the value entered.
 *
 *  For example:
 *  \code
 *    m = CounterMenuItem.new("Wall Thickness", 50, 1, 1, 50, "grid units", "", "Thickness of wall to be created")
 *  \endcode
 */
//                Fn name                  Param profiles            Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_FUNARGS_TABLE(CounterMenuItem, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(CounterMenuItem, LUA_METHODS);

#undef LUA_METHODS


const char *CounterMenuItem::luaClassName = "CounterMenuItem";
REGISTER_LUA_SUBCLASS(CounterMenuItem, MenuItem);


// Lua Constructor, called from scripts
CounterMenuItem::CounterMenuItem(lua_State *L)
{
   const char *methodName = "CounterMenuItem constructor";

   initialize();

   try
   {
      // Required items -- will throw if they are missing or misspecified
      mDisplayVal = getCheckedString(L, 1, methodName);
      // mValue =  getInt(L, 2, methodName);  ==> set this later, after we've determined mMinValue and mMaxValue

      // Optional (but recommended) items
      mStep =     getInt(L, 3, 1);   
      mMinValue = getInt(L, 4, 0);   
      mMaxValue = getInt(L, 5, 100);   
      mUnits =    getString(L, 6, "");
      mMinMsg =   getString(L, 7, "");
      mHelp =     getString(L, 8, "");

      // Second required item
      setIntValue(getCheckedInt(L, 2, methodName));    // Set this one last so we'll know mMinValue and mMaxValue
   }
   catch(LuaException &e)
   {
      logprintf(LogConsumer::LogError, "Error constructing CounterMenuItem -- please see documentation");
      logprintf(LogConsumer::ConsoleMsg, "Usage: CounterMenuItem(<display val (str)> [step (i)] [min val (i)] [max val (i)] [units (str)] [min msg (str)] [help (str)] <value (int))");
      throw e;
   }
}


////////////////////////////////////
////////////////////////////////////

TimeCounterMenuItem::TimeCounterMenuItem(const string &title, S32 value, S32 maxVal, const string &zeroMsg, const string &help,
                                         S32 step, InputCode k1, InputCode k2) :
   CounterMenuItem(title, value, step, 0, maxVal, "", zeroMsg, help, k1, k2)
{
   // Do nothing
}


S32 TimeCounterMenuItem::getBigIncrement()
{
   return 12;  // 12 * 5sec = 1 minute
}


string TimeCounterMenuItem::getUnits() const
{
   return mValue >= 60 ? "mins" : "secs";
}


MenuItemTypes TimeCounterMenuItem::getItemType()
{
   return TimeCounterMenuItemType;
}


void TimeCounterMenuItem::setValue(const string &val)
{
   mValue = S32((atof(val.c_str()) * 60 + 2.5) / 5) * 5;  // Snap to nearest 5 second interval
}


string TimeCounterMenuItem::getValueForDisplayingInMenu()
{
   return (mValue < 60) ? itos(mValue) : itos(mValue / 60) + ":" + ((mValue % 60) < 10 ? "0" : "") + itos(mValue % 60);
}


string TimeCounterMenuItem::getValueForWritingToLevelFile()
{
   return ftos((F32) mValue / 60.0f, 3);  // Time in minutes, with fraction
}


////////////////////////////////////
////////////////////////////////////

TimeCounterMenuItemSeconds::TimeCounterMenuItemSeconds(const string &title, S32 value, S32 maxVal, const string &zeroMsg, 
                                                       const string &help, InputCode k1, InputCode k2) :
   TimeCounterMenuItem(title, value, maxVal, zeroMsg, help, 1, k1, k2)
{
   // Do nothing
}


S32 TimeCounterMenuItemSeconds::getBigIncrement()
{
   return 5;
}


void TimeCounterMenuItemSeconds::setValue(const string &val)
{
   mValue = atoi(val.c_str());
}


string TimeCounterMenuItemSeconds::getValueForWritingToLevelFile()
{
   return itos(mValue);
}


void TimeCounterMenuItemSeconds::snap()
{
   mValue = S32((mValue / getBigIncrement()) * getBigIncrement());
}


////////////////////////////////////
////////////////////////////////////

PlayerMenuItem::PlayerMenuItem(S32 index, const char *text, void (*callback)(ClientGame *, U32), InputCode k1, PlayerType type) :
      MenuItem(index, text, callback, "", k1, KEY_UNKNOWN)
{
   mType = type;
}


string PlayerMenuItem::getOptionText()
{
   string text = getPrompt();

   // Add a player type prefix if requested
   if(mType == PlayerTypePlayer)
      text = "[Player] " + text;
   else if(mType == PlayerTypeAdmin)
      text = "[Admin] " + text;
   else if(mType == PlayerTypeRobot)
      text = "[Robot] " + text;

   return text;
}


void PlayerMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   glColor(*getColor(isSelected));
   drawCenteredString(xpos, ypos, textsize, getOptionText().c_str());
}


S32 PlayerMenuItem::getWidth(S32 textsize)
{
   return getStringWidth(textsize, getOptionText().c_str());
}


MenuItemTypes PlayerMenuItem::getItemType()
{
   return PlayerMenuItemType;
}


void PlayerMenuItem::activatedWithShortcutKey()
{
   /* Do nothing */
}


////////////////////////////////////
////////////////////////////////////

TeamMenuItem::TeamMenuItem(S32 index, AbstractTeam *team, void (*callback)(ClientGame *, U32), InputCode inputCode, bool isCurrent) :
               MenuItem(index, team->getName().getString(), callback, "", inputCode, KEY_UNKNOWN)
{
   mTeam = team;
   mIsCurrent = isCurrent;
   mUnselectedColor = *team->getColor();
   mSelectedColor = *team->getColor();
}


string TeamMenuItem::getOptionText()
{
   Team *team = (Team *)mTeam;

   // Static may help reduce allocation/deallocation churn at the cost of 2K memory; not sure either are really a problem
   static char buffer[2048];  
   dSprintf(buffer, sizeof(buffer), "%s%s  [ %d | %d | %d ]", mIsCurrent ? "* " : "", getPrompt().c_str(), 
                                                           team->getPlayerCount(), team->getBotCount(), team->getScore());

   return buffer;
}


void TeamMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   glColor(*getColor(isSelected));
   drawCenteredStringf(xpos, ypos, textsize, getOptionText().c_str());
}


S32 TeamMenuItem::getWidth(S32 textsize)
{
   return getStringWidth(textsize, getOptionText().c_str());
}


MenuItemTypes TeamMenuItem::getItemType()
{
   return TeamMenuItemType;
}


void TeamMenuItem::activatedWithShortcutKey()
{
   /* Do nothing */
}


////////////////////////////////////
////////////////////////////////////

TextEntryMenuItem::TextEntryMenuItem(string title, string val, string emptyVal, const string &help, U32 maxLen, InputCode k1, InputCode k2) :
         ValueMenuItem(title, NULL, help, k1, k2),
         mLineEditor(LineEditor(maxLen, val))
{
   initialize();
   mEmptyVal = emptyVal;
}


// Destructor
TextEntryMenuItem::~TextEntryMenuItem()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


void TextEntryMenuItem::initialize()
{
   mEnterAdvancesItem = true;
   mTextEditedCallback = NULL;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


string TextEntryMenuItem::getOptionText()
{
   return mLineEditor.getString() != "" ? mLineEditor.getDisplayString() : mEmptyVal;
}


void TextEntryMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   Color textColor;     
   if(mLineEditor.getString() == "" && mEmptyVal != "")
      textColor.set(.4, .4, .4);
   else if(isSelected)
      textColor.set(Colors::red);
   else
      textColor.set(Colors::cyan);

   S32 xpos2 = drawCenteredStringPair(xpos, ypos, textsize, *getColor(isSelected), textColor, 
                                      getPrompt().c_str(), getOptionText().c_str());

   glColor(Colors::red);      // Cursor is always red
   if(isSelected)
      mLineEditor.drawCursor(xpos2, ypos, textsize);
}


S32 TextEntryMenuItem::getWidth(S32 textsize)
{
   return getStringPairWidth(textsize, getPrompt().c_str(), getOptionText().c_str());
}


bool TextEntryMenuItem::handleKey(InputCode inputCode)
{ 
   if(inputCode == KEY_DELETE || inputCode == KEY_BACKSPACE)
   {
      mLineEditor.handleBackspace(inputCode); 
      if(mTextEditedCallback)
         mTextEditedCallback(mLineEditor.getString(), static_cast<EditorAttributeMenuUI *>(mMenu)->getObject());

      return true;
   }

   return false;
}


bool TextEntryMenuItem::hasTextInput()
{
   return true;
}


void TextEntryMenuItem::handleTextInput(char ascii)
{
   if(ascii)
   {
      mLineEditor.addChar(ascii);

      if(mTextEditedCallback)
         mTextEditedCallback(mLineEditor.getString(), static_cast<EditorAttributeMenuUI *>(mMenu)->getObject());
   }
}


MenuItemTypes TextEntryMenuItem::getItemType()
{
   return TextEntryMenuItemType;
}


LineEditor TextEntryMenuItem::getLineEditor()
{
   return mLineEditor;
}


void TextEntryMenuItem::setLineEditor(LineEditor editor)
{
   mLineEditor = editor;
}


string TextEntryMenuItem::getValueForWritingToLevelFile()
{
   return mLineEditor.getString() != "" ? mLineEditor.getString() : mEmptyVal;
}


string TextEntryMenuItem::getValueForDisplayingInMenu()
{
   return mLineEditor.getString();
}


string TextEntryMenuItem::getValue() const
{
   return mLineEditor.getString();
}


void TextEntryMenuItem::setValue(const string &val)
{
   mLineEditor.setString(val);
}


void TextEntryMenuItem::setFilter(LineEditor::LineEditorFilter filter)
{
   mLineEditor.setFilter(filter);
}


void TextEntryMenuItem::activatedWithShortcutKey()
{
   /* Do nothing */
}


void TextEntryMenuItem::setTextEditedCallback(void(*callback)(string, BfObject *))
{
   mTextEditedCallback = callback;
}


void TextEntryMenuItem::setSecret(bool secret)
{
   mLineEditor.setSecret(secret);
}


//////////
// Lua interface

/**
 *  @luaclass TextEntryMenuItem
 *  @brief    Menu item allowing users to enter a text value.
 */
//                Fn name                  Param profiles            Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_FUNARGS_TABLE(TextEntryMenuItem, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(TextEntryMenuItem, LUA_METHODS);

#undef LUA_METHODS


const char *TextEntryMenuItem::luaClassName = "TextEntryMenuItem";
REGISTER_LUA_SUBCLASS(TextEntryMenuItem, MenuItem);


// Lua Constructor
TextEntryMenuItem::TextEntryMenuItem(lua_State *L)
{
   initialize();

   const char *methodName = "TextEntryMenuItem constructor";

   // Required items -- will throw if they are missing or misspecified
   mDisplayVal = getString(L, 1, methodName);

   // Optional (but recommended) items
   mLineEditor.setString(getString(L, 2, ""));
   mEmptyVal = getString(L, 3, "");
   mLineEditor.mMaxLen = getInt(L, 4, 32);
   mHelp = getString(L, 5, "");
}


////////////////////////////////////
////////////////////////////////////

MaskedTextEntryMenuItem::MaskedTextEntryMenuItem(string title, string val, string emptyVal, const string &help, 
                                                 U32 maxLen, InputCode k1, InputCode k2) :
   TextEntryMenuItem(title, val, emptyVal, help, maxLen, k1, k2)
{
   mLineEditor.setSecret(true);
}

};

