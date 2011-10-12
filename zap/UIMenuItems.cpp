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
MenuItem::MenuItem(ClientGame *game, S32 index, const string &prompt, void (*callback)(ClientGame *, U32), const string &help, InputCode k1, InputCode k2)
{
   //mGame = game;
   mPrompt = prompt;
   key1 = k1;
   key2 = k2;
   mCallback = callback;
   mHelp = help;
   mIndex = (U32)index;
   mEnterAdvancesItem = false;
   mSelectedColor = Colors::yellow;
   mUnselectedColor = Colors::white;
   mPromptAppendage = " >";
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
   UserInterface::drawCenteredStringf(xpos, ypos, textsize, "%s%s", getPrompt().c_str(), mPromptAppendage);
}


S32 MenuItem::getWidth(S32 textsize)
{
   return UserInterface::getStringWidthf(textsize, "%s%s", getPrompt().c_str(), mPromptAppendage);
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
ValueMenuItem::ValueMenuItem(ClientGame *game, S32 index, const string &value, void (*callback)(ClientGame *, U32), 
                             const string &help, InputCode k1, InputCode k2) :
      Parent(game, index, value, callback, help, k1, k2)
{
   mSelectedValueColor = Colors::cyan;
   mUnselectedValueColor = Colors::cyan;
}


////////////////////////////////////
////////////////////////////////////

ToggleMenuItem::ToggleMenuItem(ClientGame *game, string title, Vector<string> options, U32 currOption, bool wrap, void (*callback)(ClientGame *, U32), string help, InputCode k1, InputCode k2) :
      ValueMenuItem(game, -1, title, callback, help, k1, k2)
{
   mValue = "";
   mIndex = currOption;
   mOptions = options;
   mWrap = wrap;
   mEnterAdvancesItem = true;
}


const char ToggleMenuItem::className[] = "ToggleMenuItem";      // Class name as it appears to Lua scripts

// Lua Constructor
//ToggleMenuItem::ToggleMenuItem(lua_State *L) : ValueMenuItem(gClientGame, -1, NULL, NULL, "", KEY_NONE)
//{
//   TNLAssert(false, "Don't use this yet!!");
   //static const char *methodName = "ToggleMenuItem constructor";

   //checkArgCount(L, 2, methodName);
   //string value =  getValue(L, 1, methodName);      // Text
   //F32 y =  getFloat(L, 2, methodName);      // Callback (?)
   //string help =  getFloat(L, 2, methodName);      // Help
   //F32 key1 =  getFloat(L, 2, methodName);      // Key 1
   //F32 key2 =  getFloat(L, 2, methodName);      // Key 2

   //mPoint = ValueMenuItem(game, index, text, callback, help, key1, key2);
//}



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


////////////////////////////////////
////////////////////////////////////


YesNoMenuItem::YesNoMenuItem(ClientGame *game, string title, bool currOption, void (*callback)(ClientGame *, U32), string help, InputCode k1, InputCode k2) :
      ToggleMenuItem(game, title, Vector<string>(), currOption, true, callback, help, k1, k2)
{
   mValue = "";
   mIndex = currOption;
   mEnterAdvancesItem = true;

   mOptions.push_back("No");     // 0
   mOptions.push_back("Yes");    // 1
}


////////////////////////////////////
////////////////////////////////////

CounterMenuItem::CounterMenuItem(ClientGame *game, const string &title, S32 value, S32 step, S32 minVal, S32 maxVal, const string &units, 
                                 const string &minMsg, const string &help, InputCode k1, InputCode k2) :
   Parent(game, -1, title, NULL, help, k1, k2)
{
   mValue = value;
   mStep = step;
   mMinValue = minVal;
   mMaxValue = maxVal;
   mUnits = units;
   mMinMsg = minMsg;   

   mEnterAdvancesItem = true;
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
   mValue += mStep * fact; 

   if(mValue > mMaxValue) 
      mValue = mMaxValue; 
}


void CounterMenuItem::decrement(S32 fact) 
{ 
   mValue -= mStep * fact; 

   if(mValue < mMinValue) 
      mValue = mMinValue; 
}


////////////////////////////////////
////////////////////////////////////

TimeCounterMenuItem::TimeCounterMenuItem(ClientGame *game, const string &title, S32 value, S32 maxVal, const string &zeroMsg, const string &help,
                    S32 step, InputCode k1, InputCode k2) :
   CounterMenuItem(game, title, value, step, 0, maxVal, "", zeroMsg, help, k1, k2)
{
   // Do nothing
}


////////////////////////////////////
////////////////////////////////////

TimeCounterMenuItemSeconds::TimeCounterMenuItemSeconds(ClientGame *game, const string &title, S32 value, S32 maxVal, const string &zeroMsg, 
                                                       const string &help, InputCode k1, InputCode k2) :
   TimeCounterMenuItem(game, title, value, maxVal, zeroMsg, help, 1, k1, k2)
{
   // Do nothing
}


////////////////////////////////////
////////////////////////////////////

PlayerMenuItem::PlayerMenuItem(ClientGame *game, S32 index, const char *text, void (*callback)(ClientGame *, U32), InputCode k1, PlayerType type) :
      MenuItem(game, index, text, callback, "", k1, KEY_UNKNOWN)
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

TeamMenuItem::TeamMenuItem(ClientGame *game, S32 index, AbstractTeam *team, void (*callback)(ClientGame *, U32), InputCode inputCode, bool isCurrent) :
               MenuItem(game, index, team->getName().getString(), callback, "", inputCode, KEY_UNKNOWN)
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

EditableMenuItem::EditableMenuItem(ClientGame *game, string title, string val, string emptyVal, string help, U32 maxLen, InputCode k1, InputCode k2) :
         ValueMenuItem(game, -1, title, NULL, help, k1, k2),
         mLineEditor(LineEditor(maxLen, val))
{
   mEmptyVal = emptyVal;
   mEnterAdvancesItem = true;
   mTextEditedCallback = NULL;
}


string EditableMenuItem::getOptionText()
{
   return mLineEditor.getString() != "" ? mLineEditor.getDisplayString() : mEmptyVal;
}


void EditableMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
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


S32 EditableMenuItem::getWidth(S32 textsize)
{
   return UserInterface::getStringPairWidth(textsize, getPrompt().c_str(), getOptionText().c_str());
}


bool EditableMenuItem::handleKey(InputCode inputCode, char ascii) 
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


////////////////////////////////////
////////////////////////////////////

MaskedEditableMenuItem::MaskedEditableMenuItem(ClientGame *game, string title, string val, string emptyVal, string help, U32 maxLen, InputCode k1, InputCode k2) :
   EditableMenuItem(game, title, val, emptyVal, help, maxLen, k1, k2)
{
   mLineEditor.setSecret(true);
}

};

