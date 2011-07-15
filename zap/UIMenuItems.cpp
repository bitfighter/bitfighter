
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
#include "UI.h"
#include "ScreenInfo.h"

#include "SDL/SDL_opengl.h"

namespace Zap
{

//extern void glColor(const Color &c, float alpha = 1.0);

MenuItem::MenuItem() { TNLAssert(false, "Do not use this constructor!"); }    // Default constructor

   // Constructor
MenuItem::MenuItem(S32 index, const string &prompt, void (*callback)(U32), const string &help, KeyCode k1, KeyCode k2)
{
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


bool MenuItem::handleKey(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_ENTER || keyCode == KEY_SPACE || keyCode == KEY_RIGHT || keyCode == MOUSE_LEFT)
   {
      UserInterface::playBoop();
      if(mCallback)
         mCallback(mIndex);

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

ValueMenuItem::ValueMenuItem(S32 index, const string &text, void (*callback)(U32), const string &help, KeyCode k1, KeyCode k2) :
      MenuItem(index, text, callback, help, k1, k2)
{
   mSelectedValueColor = Colors::cyan;
   mUnselectedValueColor = Colors::cyan;
}

////////////////////////////////////
////////////////////////////////////

ToggleMenuItem::ToggleMenuItem(string title, Vector<string> options, U32 currOption, bool wrap, void (*callback)(U32), string help, KeyCode k1, KeyCode k2) :
      ValueMenuItem(-1, title, callback, help, k1, k2)
{
   mValue = "";
   mIndex = currOption;
   mOptions = options;
   mWrap = wrap;
   mEnterAdvancesItem = true;
}


void ToggleMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   UserInterface::drawCenteredStringPair(xpos, ypos, textsize, *getColor(isSelected), *getValueColor(isSelected), 
      getPrompt().c_str(), mIndex < U32(mOptions.size()) ? mOptions[mIndex].c_str() : "INDEX OUT OF RANGE");
}


bool ToggleMenuItem::handleKey(KeyCode keyCode, char ascii)
{
   U32 nextValAfterWrap = mWrap ? 0 : mIndex;

   if(keyCode == KEY_RIGHT || keyCode == MOUSE_LEFT)
   {
      mIndex = (mIndex == (U32)mOptions.size() - 1) ? nextValAfterWrap : mIndex + 1;

      if(mCallback)
         mCallback(mIndex);

      UserInterface::playBoop();
      return true;
   }
   else if(keyCode == KEY_LEFT || keyCode == MOUSE_RIGHT)
   {      
      U32 nextValAfterWrap = mWrap ? mOptions.size() - 1 : mIndex;
      mIndex = (mIndex == 0) ? nextValAfterWrap : mIndex - 1;
      
      if(mCallback)
         mCallback(mIndex);

      UserInterface::playBoop();
      return true;
   }

   else if(keyCode == KEY_ENTER || keyCode == KEY_SPACE)
   {
      mIndex = (mIndex == (U32)mOptions.size() - 1) ? nextValAfterWrap : mIndex + 1;

      if(mCallback)
         mCallback(mIndex);

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
               mCallback(mIndex);

            UserInterface::playBoop();
            return true;
         }
      }

   return false;
}


////////////////////////////////////
////////////////////////////////////


YesNoMenuItem::YesNoMenuItem(string title, bool currOption, void (*callback)(U32), string help, KeyCode k1, KeyCode k2) :
      ToggleMenuItem(title, Vector<string>(), currOption, true, callback, help, k1, k2)
{
   mValue = "";
   mIndex = currOption;
   mEnterAdvancesItem = true;

   mOptions.push_back("No");     // 0
   mOptions.push_back("Yes");    // 1
}


////////////////////////////////////
////////////////////////////////////

CounterMenuItem::CounterMenuItem(const string &title, S32 value, S32 step, S32 minVal, S32 maxVal, const string &units, const string &minMsg, const string &help,
                KeyCode k1, KeyCode k2) :
   ValueMenuItem(-1, title, NULL, help, k1, k2)
{
   mValue = value;
   mStep = step;
   mMinValue = minVal;
   mMaxValue = maxVal;
   mUnits = units;
   mMinMsg = minMsg;
   mEnterAdvancesItem = true;
}

void CounterMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   if(mValue == mMinValue && mMinMsg != "")
      UserInterface::drawCenteredStringPair(xpos, ypos, textsize, *getColor(isSelected), *getValueColor(isSelected), getPrompt().c_str(), 
                                            mMinMsg.c_str());
   else
      UserInterface::drawCenteredStringPair(xpos, ypos, textsize, *getColor(isSelected), *getValueColor(isSelected), getPrompt().c_str(), 
                                           (getValueForDisplayingInMenu() + " " + getUnits()).c_str());
}


bool CounterMenuItem::handleKey(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_RIGHT || keyCode == MOUSE_LEFT)  
   {
      if(getKeyState(KEY_SHIFT))
      {
         increment(getBigIncrement());
         snap();
      }
      else
         increment(1);

      return true;
   }
   else if(keyCode == KEY_LEFT || keyCode == MOUSE_RIGHT)
   {
      if(getKeyState(KEY_SHIFT))
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

TimeCounterMenuItem::TimeCounterMenuItem(const string &title, S32 value, S32 maxVal, const string &zeroMsg, const string &help,
                    S32 step, KeyCode k1, KeyCode k2) :
   CounterMenuItem(title, value, step, 0, maxVal, "", zeroMsg, help, k1, k2)
{
   // Do nothing
}


////////////////////////////////////
////////////////////////////////////

TimeCounterMenuItemSeconds::TimeCounterMenuItemSeconds(const string &title, S32 value, S32 maxVal, const string &zeroMsg, const string &help,
                KeyCode k1, KeyCode k2) :
   TimeCounterMenuItem(title, value, maxVal, zeroMsg, help, 1, k1, k2)
{
   // Do nothing
}

////////////////////////////////////
////////////////////////////////////

PlayerMenuItem::PlayerMenuItem(S32 index, const char *text, void (*callback)(U32), KeyCode k1, PlayerType type) :
      MenuItem(index, text, callback, "", k1, KEY_UNKNOWN)
{
   mType = type;
}

void PlayerMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   string temp = getPrompt();

   // Add a player type prefix if requested
   if(mType == PlayerTypePlayer)
      temp = "[Player] " + temp;
   else if(mType == PlayerTypeAdmin)
      temp = "[Admin] " + temp;
   else if(mType == PlayerTypeRobot)
      temp = "[Robot] " + temp;

   glColor(*getColor(isSelected));
   UserInterface::drawCenteredString(xpos, ypos, textsize, temp.c_str());
}


////////////////////////////////////
////////////////////////////////////

TeamMenuItem::TeamMenuItem(S32 index, AbstractTeam *team, void (*callback)(U32), KeyCode keyCode, bool isCurrent) :
               MenuItem(index, team->getName().getString(), callback, "", keyCode, KEY_UNKNOWN)
{
   mTeam = team;
   mIsCurrent = isCurrent;
   mUnselectedColor = team->getColor();
   mSelectedColor = team->getColor();
}


void TeamMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   Team *team = (Team *)mTeam;

   glColor(*getColor(isSelected));
   UserInterface::drawCenteredStringf(xpos, ypos, textsize, "%s%s [%d /%d]", mIsCurrent ? "-> " : "", getPrompt().c_str(), 
                                     team->getPlayerCount(), team->getScore());
}


////////////////////////////////////
////////////////////////////////////

EditableMenuItem::EditableMenuItem(string title, string val, string emptyVal, string help, U32 maxLen, KeyCode k1, KeyCode k2) :
         ValueMenuItem(-1, title, NULL, help, k1, k2),
         mLineEditor(LineEditor(maxLen, val))
{
   mEmptyVal = emptyVal;
   mEnterAdvancesItem = true;
   mTextEditedCallback = NULL;
}

void EditableMenuItem::render(S32 xpos, S32 ypos, S32 textsize, bool isSelected)
{
   Color textColor;     
   if(mLineEditor.getString() == "" && mEmptyVal != "")
      textColor.set(.4, .4, .4);
   else if(isSelected)
      textColor.set(Colors::red);
   else
      textColor.set(Colors::cyan);

   S32 xpos2 = UserInterface::drawCenteredStringPair(xpos, ypos, textsize, *getColor(isSelected), textColor, getPrompt().c_str(), 
                                                    mLineEditor.getString() != "" ? mLineEditor.getDisplayString().c_str() : mEmptyVal.c_str());

   glColor3f(1,0,0);      // Cursor is always red
   if(isSelected)
      mLineEditor.drawCursor(xpos2, ypos, textsize);
}


bool EditableMenuItem::handleKey(KeyCode keyCode, char ascii) 
{ 
   if(keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE)
   {
      mLineEditor.handleBackspace(keyCode); 

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

MaskedEditableMenuItem::MaskedEditableMenuItem(string title, string val, string emptyVal, string help, U32 maxLen, KeyCode k1, KeyCode k2) :
   EditableMenuItem(title, val, emptyVal, help, maxLen, k1, k2)
{
   mLineEditor.setSecret(true);
}

};

