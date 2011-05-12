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
#include "Color.h"
#include "stringUtils.h"   // For itos
#include <string>


namespace Zap
{

using namespace std;

enum MenuItemTypes {
   MenuItemType,
   ToggleMenuItemType,
   CounterMenuItemType,
   TimeCounterMenuItemType,
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
   string mPrompt;     // Text displayed on menu
   string mHelp;       // An optional help string
   S32 mIndex;

protected:
   Color mSelectedColor;
   Color mUnselectedColor;

   bool mEnterAdvancesItem;
   void (*mCallback)(U32);

   const char *mPromptAppendage;

public:
   MenuItem() { TNLAssert(false, "Do not use this constructor!"); }    // Default constructor

   // Constructor
   MenuItem(S32 index, const string &prompt, void (*callback)(U32), const string &help, KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN)
   {
      mPrompt = prompt;
      key1 = k1;
      key2 = k2;
      mCallback = callback;
      mHelp = help;
      mIndex = (U32)index;
      mEnterAdvancesItem = false;
      mSelectedColor = yellow;
      mUnselectedColor = white;
      mPromptAppendage = " >";
   }

   KeyCode key1;     // Allow two shortcut keys per menu item...
   KeyCode key2;

   virtual MenuItemTypes getItemType() { return MenuItemType; }

   virtual void render(S32 ypos, S32 textsize, bool isSelected);              // Renders item horizontally centered on screen
   virtual void render(S32 xpos, S32 ypos, S32 textsize, bool isSelected);    // Renders item horizontally centered on xpos

   const Color *getColor(bool isSelected);

   const char *getHelp() { return mHelp.c_str(); }
   S32 getIndex() { return mIndex; }

   virtual string getPrompt() { return mPrompt; }
   virtual string getValue() { return mPrompt; } 

   virtual void setSecret(bool secret) { /* Do nothing */ }

   // When enter is pressed, should selection advance to the next item?
   virtual void setEnterAdvancesItem(bool enterAdvancesItem) { mEnterAdvancesItem = enterAdvancesItem; }
   
   virtual const char *getSpecialEditingInstructions() { return ""; }
   virtual string getValueForDisplayingInMenu() { return ""; }
   virtual S32 getIntValue() { return 0; }
   virtual string getValueForWritingToLevelFile() { return itos(getIntValue()); }
   virtual void setValue(const string &val) { /* Do nothing */ }
   virtual void setIntValue(S32 val) { /* Do nothing */ }

   virtual bool handleKey(KeyCode keyCode, char ascii);
   virtual void setFilter(LineEditor::LineEditorFilter filter) { /* Do nothing */ }
   virtual void activatedWithShortcutKey() { handleKey(MOUSE_LEFT, 0); }

   virtual bool enterAdvancesItem() { return mEnterAdvancesItem; }      

   void setSelectedColor(const Color &color)   { mSelectedColor = color; }
   void setUnselectedColor(const Color &color) { mUnselectedColor = color; }
   virtual void setSelectedValueColor(const Color &color)   { /* Override in children */ }
   virtual void setUnselectedValueColor(const Color &color) { /* Override in children */ }
};


////////////////////////////////////////
////////////////////////////////////////

class MessageMenuItem : public MenuItem
{
public:
   MessageMenuItem(string title, const Color &color) : MenuItem(-1, title, NULL, "")  
   { 
      mPromptAppendage = ""; 
      mUnselectedColor = color; 
   }
};


////////////////////////////////////////
////////////////////////////////////////

// Parent class for all things that have both a name and a value, i.e. anything that's not a regular menuItem
// Provides some additional functionality
class ValueMenuItem : public MenuItem
{
protected:
   Color mSelectedValueColor;       // Color of value when selected
   Color mUnselectedValueColor;     // Color of value when unselected

   const Color *getValueColor(bool isSelected) { return isSelected ? &mSelectedValueColor : &mUnselectedValueColor; }
   void setSelectedValueColor(const Color &color)   { mSelectedValueColor = color; }
   void setUnselectedValueColor(const Color &color) { mUnselectedValueColor = color; }

public:
   ValueMenuItem(S32 index, const string &text, void (*callback)(U32), const string &help, KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN) : 
         MenuItem(index, text, callback, help, k1, k2)
   {
      mSelectedValueColor = cyan;
      mUnselectedValueColor = cyan;
   }
};


////////////////////////////////////////
////////////////////////////////////////

class ToggleMenuItem : public ValueMenuItem
{

protected:
   string mValue;
   U32 mIndex;
   bool mWrap;


public:
   ToggleMenuItem(string title, Vector<string> options, U32 currOption, bool wrap, void (*callback)(U32), string help, KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN) :
         ValueMenuItem(-1, title, callback, help, k1, k2)
   {
      mValue = "";
      mIndex = currOption;  
      mOptions = options;
      mWrap = wrap;
      mEnterAdvancesItem = true;
   }

   virtual MenuItemTypes getItemType() { return ToggleMenuItemType; }
   virtual string getValueForDisplayingInMenu() { return mValue; }
   virtual const char *getSpecialEditingInstructions() { return "Use [<-] and [->] keys to change value."; }
   virtual S32 getValueIndex() { return mIndex; }
   virtual void setValueIndex(U32 index) { mIndex = index; }
   
   virtual string getValue() { return mOptions[mIndex]; } 

   virtual void render(S32 xpos, S32 ypos, S32 textsize, bool isSelected);
   virtual bool handleKey(KeyCode keyCode, char ascii);

   virtual void activatedWithShortcutKey() { /* Do nothing */ }

   Vector<string> mOptions;
};


////////////////////////////////////////
////////////////////////////////////////

class YesNoMenuItem : public ToggleMenuItem
{
public:
   YesNoMenuItem(string title, bool currOption, void (*callback)(U32), string help, KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN) :
         ToggleMenuItem(title, Vector<string>(), currOption, true, callback, help, k1, k2)
   {
      mValue = "";
      mIndex = currOption;       
      mEnterAdvancesItem = true;

      mOptions.push_back("No");     // 0
      mOptions.push_back("Yes");    // 1
   }

   virtual string getValueForDisplayingInMenu() { return mIndex ? " Engineer" : ""; }
   virtual string getValueForWritingToLevelFile() { return mIndex ? "yes" : "no"; }
   virtual void setValue(const string &val) { mIndex = (val == "yes") ? 1 : 0; }
   virtual S32 getIntValue() { return mIndex; }
   virtual void setIntValue(S32 value) { mIndex = (value == 0) ? 0 : 1; }
};

////////////////////////////////////////
////////////////////////////////////////

class CounterMenuItem : public ValueMenuItem
{
protected:
   S32 mValue;
   S32 mStep;
   S32 mMinValue;
   S32 mMaxValue;
   string mUnits;
   string mMinMsg;

   virtual void increment(S32 fact = 1); 
   virtual void decrement(S32 fact = 1);
   virtual S32 getBigIncrement() { return 10; }    // How much our counter is incremented when shift is down (multiplier)

public:
   CounterMenuItem(const string &title, S32 value, S32 step, S32 minVal, S32 maxVal, const string &units, const string &minMsg, const string &help, 
                   KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN) :
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

   virtual void render(S32 xpos, S32 ypos, S32 textsize, bool isSelected);

   virtual MenuItemTypes getItemType() { return CounterMenuItemType; }
   virtual string getValueForDisplayingInMenu() { return itos(mValue); }
   virtual const char *getUnits() { return mUnits.c_str(); }
   virtual S32 getIntValue() { return mValue; }
   virtual void setValue(const string &val) { mValue = atoi(val.c_str()); }
   virtual string getValue() { return itos(mValue); }
   virtual void setIntValue(S32 val) { mValue = val; }
   virtual const char *getSpecialEditingInstructions() { return "Use [<-] and [->] keys to change value.  Use [Shift] for bigger change."; }
   virtual bool handleKey(KeyCode keyCode, char ascii);

   virtual void snap() { /* Do nothing */ }

   virtual void activatedWithShortcutKey() { /* Do nothing */ }
};


////////////////////////////////////////
////////////////////////////////////////

class TimeCounterMenuItem : public CounterMenuItem
{
protected:
   virtual S32 getBigIncrement() { return 12; }    // 12 * 5sec = 1 minute

public:
   TimeCounterMenuItem(const string &title, S32 value, S32 maxVal, const string &zeroMsg, const string &help, 
                       S32 step = 5, KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN) :
      CounterMenuItem(title, value, step, 0, maxVal, "", zeroMsg, help, k1, k2)
   {
      // Do nothing
   }

   virtual const char *getUnits() { return mValue >= 60 ? "mins" : "secs"; }

   virtual MenuItemTypes getItemType() { return TimeCounterMenuItemType; }
   virtual void setValue (const string &val) { mValue = S32((atof(val.c_str()) * 60 + 2.5) / 5) * 5 ; }     // Snap to nearest 5 second interval
   virtual string getValueForDisplayingInMenu() { return (mValue < 60) ? itos(mValue) : 
                                                   itos(mValue / 60) + ":" + ((mValue % 60) < 10 ? "0" : "") + itos(mValue % 60); }
   virtual string getValueForWritingToLevelFile() { return ftos((F32)mValue / 60.0f, 3); }
};


////////////////////////////////////////
////////////////////////////////////////

// As TimeCounterMenuItem, but reporting time in seconds, and with increments of 1 second, rather than 5
class TimeCounterMenuItemSeconds : public TimeCounterMenuItem
{
protected:
   virtual S32 getBigIncrement() { return 5; }

public:
   TimeCounterMenuItemSeconds(const string &title, S32 value, S32 maxVal, const string &zeroMsg, const string &help, 
                   KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN) :
      TimeCounterMenuItem(title, value, maxVal, zeroMsg, help, 1, k1, k2)
   {
      // Do nothing
   }

   virtual void setValue (const string &val) { mValue = atoi(val.c_str()); } 
   virtual string getValueForWritingToLevelFile() { return itos(mValue); }

   virtual void snap() { mValue = S32((mValue / getBigIncrement()) * getBigIncrement()); }
};

////////////////////////////////////
////////////////////////////////////

class EditableMenuItem : public ValueMenuItem
{
private:
   string mEmptyVal;

protected:
      LineEditor mLineEditor;
      void (*mTextEditedCallback)(string);

public:
   // Contstuctor
   EditableMenuItem(string title, string val, string emptyVal, string help, U32 maxLen, KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN) :
            ValueMenuItem(-1, title, NULL, help, k1, k2),
            mLineEditor(LineEditor(maxLen, val))
   {
      mEmptyVal = emptyVal;
      mEnterAdvancesItem = true;    
      mTextEditedCallback = NULL;
   }

   virtual MenuItemTypes getItemType() { return EditableMenuItemType; }
   virtual void render(S32 xpos, S32 ypos, S32 textsize, bool isSelected);
   virtual bool handleKey(KeyCode keyCode, char ascii);

   LineEditor getLineEditor() { return mLineEditor; }
   void setLineEditor(LineEditor editor) { mLineEditor = editor; }

   virtual string getValueForWritingToLevelFile() { return mLineEditor.getString(); }
   virtual string getValueForDisplayingInMenu() { return mLineEditor.getString(); }

   virtual string getValue() { return mLineEditor.getString(); } 
   void setValue(const string &val) { mLineEditor.setString(val); }

   virtual void setFilter(LineEditor::LineEditorFilter filter) { mLineEditor.setFilter(filter); }

   virtual void activatedWithShortcutKey() { /* Do nothing */ }

   virtual void setTextEditedCallback(void (*callback)(string)) { mTextEditedCallback = callback; }


   virtual void setSecret(bool secret) { mLineEditor.setSecret(secret); }
};

////////////////////////////////////
////////////////////////////////////

class MaskedEditableMenuItem : public EditableMenuItem
{
   MaskedEditableMenuItem(string title, string val, string emptyVal, string help, U32 maxLen, KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN) : 
      EditableMenuItem(title, val, emptyVal, help, maxLen, k1, k2)
   {
      mLineEditor.setSecret(true);
   }
};


////////////////////////////////////
////////////////////////////////////

class PlayerMenuItem : public MenuItem
{
private:
   PlayerType mType;    // Type of player, for name menu

public:
   // Constructor
   PlayerMenuItem(S32 index, const char *text, void (*callback)(U32), KeyCode k1, PlayerType type) :
         MenuItem(index, text, callback, "", k1, KEY_UNKNOWN)
   {
      mType = type;
   }

   virtual MenuItemTypes getItemType() { return PlayerMenuItemType; }
   virtual void render(S32 xpos, S32 ypos, S32 textsize, bool isSelected);

   virtual void activatedWithShortcutKey() { /* Do nothing */ }
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
                  MenuItem(index, team.getName().getString(), callback, "", keyCode, KEY_UNKNOWN)
{
   mTeam = team;
   mIsCurrent = isCurrent;
   mUnselectedColor = team.color;
   mSelectedColor = team.color;
}

   virtual MenuItemTypes getItemType() { return TeamMenuItemType; }
   void render(S32 xpos, S32 ypos, S32 textsize, bool isSelected);

   virtual void activatedWithShortcutKey() { /* Do nothing */ }

};

};
#endif


