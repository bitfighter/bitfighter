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
#include "UI.h"
#include "ScreenInfo.h"    // For gScreenInfo stuff

#include "SDL/SDL_opengl.h"


namespace Zap
{

// Constructor
MenuItem::MenuItem()
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
MenuItem::MenuItem(const string &displayVal, void (*callback)(ClientGame *, U32), const char *help, InputCode k1, InputCode k2)
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
   mHelp = help.c_str();
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

   mEnterAdvancesItem = false;
   mSelectedColor = Colors::yellow;
   mUnselectedColor = Colors::white;
   mDisplayValAppendage = " >";
}


// Destructor
MenuItem::~MenuItem() 
{ 
   // Do nothing
} 


MenuUserInterface *MenuItem::getMenu()  
{ 
   return mMenu; 
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
   UserInterface::drawCenteredStringf(xpos, ypos, textsize, "%s%s", getPrompt().c_str(), mDisplayValAppendage);
}


S32 MenuItem::getWidth(S32 textsize)
{
   return UserInterface::getStringWidthf(textsize, "%s%s", getPrompt().c_str(), mDisplayValAppendage);
}


bool MenuItem::handleKey(InputCode inputCode, char ascii)
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
      Parent(-1, displayValue, callback, help, k1, k2)
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


////////////////////////////////////
////////////////////////////////////

ToggleMenuItem::ToggleMenuItem()
{
   // Do nothing
}


ToggleMenuItem::ToggleMenuItem(string title, Vector<string> options, U32 currOption, bool wrap, 
                               void (*callback)(ClientGame *, U32), string help, InputCode k1, InputCode k2) :
      ValueMenuItem(title, callback, help, k1, k2)
{
   //mValue = "";
   mOptions = options;
   mIndex = clamp(currOption, 0, mOptions.size() - 1);
   mWrap = wrap;
   mEnterAdvancesItem = true;
}


string ToggleMenuItem::getOptionText()
{
   return mIndex < U32(mOptions.size()) ? mOptions[mIndex] : "INDEX OUT OF RANGE";
}


void ToggleMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   UserInterface::drawCenteredStringPair(xpos, ypos, textsize, *getColor(isSelected), *getValueColor(isSelected), 
                                         getPrompt().c_str(), getOptionText().c_str());
}


S32 ToggleMenuItem::getWidth(S32 textsize)
{
   return UserInterface::getStringPairWidth(textsize, getPrompt().c_str(), getOptionText().c_str());
}


bool ToggleMenuItem::handleKey(InputCode inputCode, char ascii)
{
   U32 nextValAfterWrap = mWrap ? 0 : mIndex;

   if(inputCode == KEY_RIGHT || inputCode == MOUSE_LEFT)
   {
      mIndex = (mIndex == (U32)mOptions.size() - 1) ? nextValAfterWrap : mIndex + 1;

      if(mCallback)
         mCallback(getMenu()->getGame(), mIndex);

      UserInterface::playBoop();
      return true;
   }
   else if(inputCode == KEY_LEFT || inputCode == MOUSE_RIGHT)
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

   else if(ascii)     // Check for the first key of a menu entry. 
      for(S32 i = 0; i < mOptions.size(); i++)
      {
         S32 index = (i + mIndex + 1) % mOptions.size();
         if(tolower(ascii) == tolower(mOptions[index].data()[0]))
         {
            mIndex = index;
            
            if(mCallback)
               mCallback(getMenu()->getGame(), mIndex);

            UserInterface::playBoop();
            return true;
         }
      }

   return false;
}


//////////
// Lua interface
const char ToggleMenuItem::className[] = "ToggleMenuItem";      // Class name as it appears to Lua scripts

// Lua Constructor
ToggleMenuItem::ToggleMenuItem(lua_State *L)
{
   const char *methodName = "ToggleMenuItem constructor";

   // Required items -- will throw if they are missing or misspecified
   mDisplayVal = getString(L, 1, methodName);
   getStringVectorFromTable(L, 2, methodName, mOptions);    // Fills mOptions with elements in a table 

   // Optional (but recommended) items
   mIndex = clamp(getInt(L, 3, methodName, 1) - 1, 0,  mOptions.size() - 1);   // First - 1 for compatibility with Lua's 1-based array index
   mWrap = getBool(L, 4, methodName, false);
   mHelp = getString(L, 4, methodName, "");
}


// Define the methods we will expose to Lua
Lunar<ToggleMenuItem>::RegType ToggleMenuItem::methods[] =
{
   {0,0}    // End method list
};


void ToggleMenuItem::push(lua_State *L) 
{  
   Lunar<ToggleMenuItem>::push(L, this); 
}


////////////////////////////////////
////////////////////////////////////

// Constructors
YesNoMenuItem::YesNoMenuItem(string title, bool currOption, string help, InputCode k1, InputCode k2) :
      ToggleMenuItem(title, Vector<string>(), currOption, true)
{
   initialize();

   setIndex(currOption);
}


//YesNoMenuItem::YesNoMenuItem(string title, bool currOption, void (*callback)(ClientGame *, U32), string help, 
//                             InputCode k1, InputCode k2) :
//      ToggleMenuItem(title, Vector<string>(), currOption, true, callback)
//{
//   initialize();
//
//   setIndex(currOption);
//}


void YesNoMenuItem::initialize()
{
   //mValue = "";
   mEnterAdvancesItem = true;

   mOptions.push_back("No");     // 0
   mOptions.push_back("Yes");    // 1
}


void YesNoMenuItem::setIndex(S32 index)
{
   mIndex = clamp(index, 0, 1);
}


//////////
// Lua interface
const char YesNoMenuItem::className[] = "YesNoMenuItem";      // Class name as it appears to Lua scripts

// Lua Constructor
YesNoMenuItem::YesNoMenuItem(lua_State *L)
{
   initialize();

   const char *methodName = "YesNoMenuItem constructor";

   // Required items -- will throw if they are missing or misspecified
   mDisplayVal = getString(L, 1, methodName);

   // Optional (but recommended) items
   setIndex(getInt(L, 2, methodName, 1) - 1);                // - 1 for compatibility with Lua's 1-based array index
   mHelp = getString(L, 3, methodName, "");
}


// Define the methods we will expose to Lua
Lunar<YesNoMenuItem>::RegType YesNoMenuItem::methods[] =
{
   {0,0}    // End method list
};


void YesNoMenuItem::push(lua_State *L) 
{  
   Lunar<YesNoMenuItem>::push(L, this); 
}


////////////////////////////////////
////////////////////////////////////

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


void CounterMenuItem::initialize()
{
   mEnterAdvancesItem = true;
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
   UserInterface::drawCenteredStringPair(xpos, ypos, textsize, *getColor(isSelected), *getValueColor(isSelected), 
                                         getPrompt().c_str(), getOptionText().c_str());
}


S32 CounterMenuItem::getWidth(S32 textsize)
{
   return UserInterface::getStringPairWidth(textsize, getPrompt().c_str(), getOptionText().c_str());
}


bool CounterMenuItem::handleKey(InputCode inputCode, char ascii)
{
   if(inputCode == KEY_RIGHT || inputCode == MOUSE_LEFT)  
   {
      if(checkModifier(KEY_SHIFT))
      {
         increment(getBigIncrement());
         snap();
      }
      else
         increment(1);

      return true;
   }
   else if(inputCode == KEY_LEFT || inputCode == MOUSE_RIGHT)
   {
      if(checkModifier(KEY_SHIFT))
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


//////////
// Lua interface
const char CounterMenuItem::className[] = "CounterMenuItem";      // Class name as it appears to Lua scripts

// Lua Constructor
CounterMenuItem::CounterMenuItem(lua_State *L)
{
   const char *methodName = "CounterMenuItem constructor";

   initialize();

   // Required items -- will throw if they are missing or misspecified
   mDisplayVal = getString(L, 1, methodName);
   // mValue =  getInt(L, 2, methodName);  ==> set this later, after we've determined mMinValue and mMaxValue

   // Optional (but recommended) items
   mStep =     getInt(L, 3, methodName, 1);   
   mMinValue = getInt(L, 4, methodName, 0);   
   mMaxValue = getInt(L, 5, methodName, 100);   
   mUnits =    getString(L, 6, methodName, "");
   mMinMsg =   getString(L, 7, methodName, "");
   mHelp =     getString(L, 8, methodName, "");

   // Second required item
   setIntValue(getInt(L, 2, methodName));  
}


// Define the methods we will expose to Lua
Lunar<CounterMenuItem>::RegType CounterMenuItem::methods[] =
{
   {0,0}    // End method list
};


void CounterMenuItem::push(lua_State *L) 
{  
   Lunar<CounterMenuItem>::push(L, this); 
}


////////////////////////////////////
////////////////////////////////////

TimeCounterMenuItem::TimeCounterMenuItem(const string &title, S32 value, S32 maxVal, const string &zeroMsg, const string &help,
                    S32 step, InputCode k1, InputCode k2) :
   CounterMenuItem(title, value, step, 0, maxVal, "", zeroMsg, help, k1, k2)
{
   // Do nothing
}


////////////////////////////////////
////////////////////////////////////

TimeCounterMenuItemSeconds::TimeCounterMenuItemSeconds(const string &title, S32 value, S32 maxVal, const string &zeroMsg, 
                                                       const string &help, InputCode k1, InputCode k2) :
   TimeCounterMenuItem(title, value, maxVal, zeroMsg, help, 1, k1, k2)
{
   // Do nothing
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
   UserInterface::drawCenteredString(xpos, ypos, textsize, getOptionText().c_str());
}


S32 PlayerMenuItem::getWidth(S32 textsize)
{
   return UserInterface::getStringWidth(textsize, getOptionText().c_str());
}


////////////////////////////////////
////////////////////////////////////

TeamMenuItem::TeamMenuItem(S32 index, AbstractTeam *team, void (*callback)(ClientGame *, U32), InputCode inputCode, bool isCurrent) :
               MenuItem(index, team->getName().getString(), callback, "", inputCode, KEY_UNKNOWN)
{
   mTeam = team;
   mIsCurrent = isCurrent;
   mUnselectedColor = team->getColor();
   mSelectedColor = team->getColor();
}


string TeamMenuItem::getOptionText()
{
   Team *team = (Team *)mTeam;

   // Static may help reduce allocation/deallocation churn at the cost of 2K memory; not sure either are really a problem
   static char buffer[2048];  
   dSprintf(buffer, sizeof(buffer), "%s%s [%d /%d]", mIsCurrent ? "-> " : "", getPrompt().c_str(), team->getPlayerCount(), team->getScore());

   return buffer;
}


void TeamMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   glColor(*getColor(isSelected));
   UserInterface::drawCenteredStringf(xpos, ypos, textsize, getOptionText().c_str());
}


S32 TeamMenuItem::getWidth(S32 textsize)
{
   return UserInterface::getStringWidth(textsize, getOptionText().c_str());
}


////////////////////////////////////
////////////////////////////////////

TextEntryMenuItem::TextEntryMenuItem(string title, string val, string emptyVal, string help, U32 maxLen, InputCode k1, InputCode k2) :
         ValueMenuItem(title, NULL, help, k1, k2),
         mLineEditor(LineEditor(maxLen, val))
{
   initialize();
   mEmptyVal = emptyVal;
}


void TextEntryMenuItem::initialize()
{
   mEnterAdvancesItem = true;
   mTextEditedCallback = NULL;
}


string TextEntryMenuItem::getOptionText()
{
   return mLineEditor.getString() != "" ? mLineEditor.getDisplayString() : mEmptyVal;
}


void TextEntryMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   Color textColor;     
   if(mLineEditor.getString() == "" && mEmptyVal != "")
      textColor.set(.4);
   else if(isSelected)
      textColor.set(Colors::red);
   else
      textColor.set(Colors::cyan);

   S32 xpos2 = UserInterface::drawCenteredStringPair(xpos, ypos, textsize, *getColor(isSelected), textColor, 
                                                     getPrompt().c_str(), getOptionText().c_str());

   glColor(Colors::red);      // Cursor is always red
   if(isSelected)
      mLineEditor.drawCursor(xpos2, ypos, textsize);
}


S32 TextEntryMenuItem::getWidth(S32 textsize)
{
   return UserInterface::getStringPairWidth(textsize, getPrompt().c_str(), getOptionText().c_str());
}


bool TextEntryMenuItem::handleKey(InputCode inputCode, char ascii) 
{ 
   if(inputCode == KEY_DELETE || inputCode == KEY_BACKSPACE)
   {
      mLineEditor.handleBackspace(inputCode); 

      if(mTextEditedCallback)
         mTextEditedCallback(mLineEditor.getString());

      return true;
   }
   else if(ascii > 0)
   {
      mLineEditor.addChar(ascii);

      if(mTextEditedCallback)
         mTextEditedCallback(mLineEditor.getString());

      return true;
   }
   
   return false;
}


//////////
// Lua interface
const char TextEntryMenuItem::className[] = "TextEntryMenuItem";      // Class name as it appears to Lua scripts

// Lua Constructor
TextEntryMenuItem::TextEntryMenuItem(lua_State *L)
{
   initialize();

   const char *methodName = "TextEntryMenuItem constructor";

   // Required items -- will throw if they are missing or misspecified
   mDisplayVal = getString(L, 1, methodName);

   // Optional (but recommended) items
   mLineEditor.setString(getString(L, 2, methodName, ""));
   mEmptyVal = getString(L, 3, methodName, "");
   mLineEditor.mMaxLen = getInt(L, 4, methodName, 32);
   mHelp = getString(L, 5, methodName, "");
}


// Define the methods we will expose to Lua
Lunar<TextEntryMenuItem>::RegType TextEntryMenuItem::methods[] =
{
   {0,0}    // End method list
};


void TextEntryMenuItem::push(lua_State *L) 
{  
   Lunar<TextEntryMenuItem>::push(L, this); 
}


////////////////////////////////////
////////////////////////////////////

MaskedTextEntryMenuItem::MaskedTextEntryMenuItem(string title, string val, string emptyVal, string help, U32 maxLen, InputCode k1, InputCode k2) :
   TextEntryMenuItem(title, val, emptyVal, help, maxLen, k1, k2)
{
   mLineEditor.setSecret(true);
}

};

