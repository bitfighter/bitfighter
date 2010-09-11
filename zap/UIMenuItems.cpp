
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

namespace Zap
{

extern void glColor(const Color &c, float alpha = 1.0);

void MenuItem::render(S32 ypos, S32 textsize)
{
   glColor(color);
   UserInterface::drawCenteredStringf(ypos, textsize, "%s >", getText());
}


void MenuItem::handleKey(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_ENTER || keyCode == KEY_RIGHT || keyCode == MOUSE_LEFT)
   {
      if(mCallback)
         mCallback(mIndex);
   }
   else
   {
      // Check individual entries for any shortcut keys
   }
}

////////////////////////////////////
////////////////////////////////////

void ToggleMenuItem::render(S32 ypos, S32 textsize)
{
   UserInterface::drawCenteredStringPair(ypos, textsize, color, Color(0,1,1), getText(), mOptions[mIndex].c_str());
}


void ToggleMenuItem::handleKey(KeyCode keyCode, char ascii)
{
   U32 nextValAfterWrap = mWrap ? 0 : mIndex;

   if(keyCode == KEY_RIGHT || keyCode == MOUSE_LEFT)
   {
      mIndex = (mIndex == mOptions.size() - 1) ? nextValAfterWrap : mIndex + 1;

      if(mCallback)
         mCallback(mIndex);
   }
   else if(keyCode == KEY_LEFT || keyCode == MOUSE_RIGHT)
   {      
      U32 nextValAfterWrap = mWrap ? mOptions.size() - 1 : mIndex;
      mIndex = (mIndex == 0) ? nextValAfterWrap : mIndex - 1;
      
      if(mCallback)
         mCallback(mIndex);
   }

   else if(keyCode == KEY_ENTER)
   {
      mIndex = (mIndex == mOptions.size() - 1) ? nextValAfterWrap : mIndex + 1;

      if(mCallback)
         mCallback(mIndex);
   }
}


////////////////////////////////////
////////////////////////////////////

void CounterMenuItem::render(S32 ypos, S32 textsize)
{
   if(mValue == mMinValue && mMinMsg != "")
      UserInterface::drawCenteredStringPair(ypos, textsize, color, Color(0,1,1), getText(), mMinMsg.c_str());
   else
      UserInterface::drawCenteredStringPair(ypos, textsize, color, Color(0,1,1), getText(), (UserInterface::itos(mValue) + " " + mUnits).c_str());
}


void CounterMenuItem::handleKey(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_RIGHT || keyCode == MOUSE_LEFT)  
      increment(getKeyState(KEY_SHIFT) ? 10 : 1);

   else if(keyCode == KEY_LEFT || keyCode == MOUSE_RIGHT)
      decrement(getKeyState(KEY_SHIFT) ? 10 : 1);
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

void PlayerMenuItem::render(S32 ypos, S32 textsize)
{
   string temp = getText();

   // Add a player type prefix if requested
   if(mType == PlayerTypePlayer)
      temp = "[Player] " + temp;
   else if(mType == PlayerTypeAdmin)
      temp = "[Admin] " + temp;
   else if(mType == PlayerTypeRobot)
      temp = "[Robot] " + temp;

   glColor(color);
   UserInterface::drawCenteredString(ypos, textsize, temp.c_str());
}


////////////////////////////////////
////////////////////////////////////


void TeamMenuItem::render(S32 ypos, S32 textsize)
{
   glColor(color);
   UserInterface::drawCenteredStringf(ypos, textsize, "%s%s [%d /%d]", mIsCurrent ? "-> " : "", getText(), mTeam.numPlayers, mTeam.getScore());
}


////////////////////////////////////
////////////////////////////////////

void EditableMenuItem::render(S32 ypos, S32 textsize)
{
   S32 xpos = UserInterface::drawCenteredStringPair(ypos, textsize, color, Color(0,1,1), getText(), 
                                                    mLineEditor.getString() != "" ? mLineEditor.c_str() : mEmptyVal.c_str());
   if(isActive())
      mLineEditor.drawCursor(xpos, ypos, textsize);
}


void EditableMenuItem::handleKey(KeyCode keyCode, char ascii) 
{ 
   if(keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE)
      mLineEditor.handleBackspace(keyCode); 
   else
      mLineEditor.addChar(ascii);
}

};