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

#ifndef _MENU_ITEMS_H_
#define _MENU_ITEMS_H_

#include "keyCode.h"       // For keyCodes!
#include "teamInfo.h"      // For Team def
#include "point.h"

#include <string>


namespace Zap
{

using namespace std;

enum MenuItemTypes {
   MenuItemType,
   ToggleMenuItemType,
   CounterMenuItemType,
   EditableMenuItemType,   
   PlayerMenuItemType,
   TeamMenuItemType
};

enum PlayerType {
   PlayerTypePlayer,
   PlayerTypeAdmin,
   PlayerTypeRobot,
   PlayerTypeIrrelevant
};

////////////////////////////////////
////////////////////////////////////


class MenuItem
{
private:
   string mText;     // Text displayed on menu
   string mHelp;     // An optional help string
   S32 mIndex;
   bool mIsActive;

protected:
   Color color;      // Color in which item should be displayed
   void (*mCallback)(U32);

public:
   MenuItem() { TNLAssert(false, "Do not use this constructor!"); }    // Default constructor

   // Constructor
   MenuItem(S32 index, string text, void (*callback)(U32), string help, KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN, Color c = Color(1,1,1))
   {
      mText = text;
      key1 = k1;
      key2 = k2;
      color = c;
      mCallback = callback;
      mHelp = help;
      mIndex = (U32)index;
      mIsActive = false;
   }

   KeyCode key1;     // Allow two shortcut keys per menu item...
   KeyCode key2;

   virtual MenuItemTypes getItemType() { return MenuItemType; }
   virtual void render(S32 ypos, S32 textsize);
   const char *getHelp() { return mHelp.c_str(); }
   const char *getText() { return mText.c_str(); }
   string getString() { return mText; }
   
   virtual const char *getSpecialEditingInstructions() { return ""; }
   virtual string getValue() { return ""; }
   virtual S32 getIntValue() { return 0; }
   virtual void setValue(S32 val) { /* Do nothing */ }

   virtual void handleKey(KeyCode keyCode, char ascii);
   void setActive(bool isActive) { mIsActive = isActive; }
   bool isActive() { return mIsActive; }
};


////////////////////////////////////////
////////////////////////////////////////

class ToggleMenuItem : public MenuItem
{
private:
   string mValue;
   U32 mIndex;
   Vector<string> mOptions;
   bool mWrap;

public:
   ToggleMenuItem(string title, Vector<string> options, U32 currOption, bool wrap, void (*callback)(U32), string help, KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN, Color color = Color(1,1,1)) :
         MenuItem(-1, title, callback, help, k1, k2,  color)
   {
      mValue = "";
      mIndex = currOption;  
      mOptions = options;
      mWrap = wrap;
   }

   virtual MenuItemTypes getItemType() { return ToggleMenuItemType; }
   virtual string getValue() { return mValue; }
   virtual const char *getSpecialEditingInstructions() { return "Use [<-] and [->] keys to change value."; }
   virtual S32 getValueIndex() { return mIndex; }

   virtual void render(S32 ypos, S32 textsize);
   virtual void handleKey(KeyCode keyCode, char ascii);
};


////////////////////////////////////////
////////////////////////////////////////

class CounterMenuItem : public MenuItem
{
private:
   S32 mValue;
   S32 mStep;
   S32 mMinValue;
   S32 mMaxValue;
   string mUnits;
   string mMinMsg;

   virtual void increment(S32 fact = 1); 
   virtual void decrement(S32 fact = 1);

public:
   CounterMenuItem(string title, S32 value, S32 step, S32 minVal, S32 maxVal, string units, string minMsg, string help, 
                   KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN, Color color = Color(1,1,1)) :
      MenuItem(-1, title, NULL, help, k1, k2,  color)
   {
      mValue = value;
      mStep = step;
      mMinValue = minVal;
      mMaxValue = maxVal;
      mUnits = units;
      mMinMsg = minMsg;
   }

   virtual void render(S32 ypos, S32 textsize);

   virtual MenuItemTypes getItemType() { return CounterMenuItemType; }
   virtual string getValue() { return UserInterface::itos(mValue); }
   const char *getUnits() { return mUnits.c_str(); }
   virtual S32 getIntValue() { return mValue; }
   virtual void setValue(S32 value) { mValue = value; }
   virtual const char *getSpecialEditingInstructions() { return "Use [<-] and [->] keys to change value.  Use [Shift] for bigger change."; }
   virtual void handleKey(KeyCode keyCode, char ascii);
};


////////////////////////////////////
////////////////////////////////////

class EditableMenuItem : public MenuItem
{
private:
   LineEditor mLineEditor;
   string mEmptyVal;

public:
   // Contstuctor
   EditableMenuItem(string title, string val, string emptyVal, string help, U32 maxLen, KeyCode k1, KeyCode k2 = KEY_UNKNOWN, Color c = Color(1, 1, 1) ) :
            MenuItem(-1, title, NULL, help, k1, k2, c),
            mLineEditor(LineEditor(maxLen, val))
   {
      mEmptyVal = emptyVal;
   }

public:
   virtual MenuItemTypes getItemType() { return EditableMenuItemType; }
   virtual void render(S32 ypos, S32 textsize);
   virtual void handleKey(KeyCode keyCode, char ascii);

   LineEditor getLineEditor() { return mLineEditor; }
   void setLineEditor(LineEditor editor) { mLineEditor = editor; }

   virtual string getValue() { return mLineEditor.getString(); }
};


////////////////////////////////////
////////////////////////////////////

class PlayerMenuItem : public MenuItem
{
private:
   PlayerType mType;    // Type of player, for name menu

public:
   // Constructor
   PlayerMenuItem(S32 index, const char *text, void (*callback)(U32), KeyCode k1, Color color, PlayerType type) :
         MenuItem(index, text, callback, "", k1, KEY_UNKNOWN, color)
   {
      mType = type;
   }

   virtual MenuItemTypes getItemType() { return PlayerMenuItemType; }
   virtual void render(S32 ypos, S32 textsize);
};


////////////////////////////////////
////////////////////////////////////

class TeamMenuItem : public MenuItem
{
private:
   Team mTeam;
   bool mIsCurrent;     // Is this a player's current team? 

public:
   TeamMenuItem(S32 index, Team team, void (*callback)(U32), KeyCode keyCode, bool isCurrent) :
                  MenuItem(index, team.getName().getString(), callback, "", keyCode, KEY_UNKNOWN, team.color)
{
   mTeam = team;
   mIsCurrent = isCurrent;
}

   virtual MenuItemTypes getItemType() { return TeamMenuItemType; }
   void render(S32 ypos, S32 textsize);
};

};
#endif