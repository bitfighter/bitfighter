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

#ifndef _UIGAMEPARAMS_H_
#define _UIGAMEPARAMS_H_

#include "../tnl/tnlTypes.h"

#include "UI.h"
#include "input.h"
#include "timer.h"
#include "UIMenus.h"
#include <string>

namespace Zap
{

using namespace std;

////////////////////////////////////
////////////////////////////////////

class SavedMenuItem
{
private:
   string mParamName;
   string mParamVal;

public:
   SavedMenuItem() { /* Unused */ }    // Default constructor

   SavedMenuItem(MenuItem *menuItem) { mParamName = menuItem->getPrompt(); setValues(menuItem); }

   void setValues(MenuItem *menuItem) { mParamVal = menuItem->getValueForWritingToLevelFile(); }

   string getParamName() { return mParamName; }
   string getParamVal() { return mParamVal; }
};

////////////////////////////////////
////////////////////////////////////

class GameParamUserInterface : public MenuUserInterface     // By subclassing this, I hoped to get the mouse stuff to automatically work, but it didn't.  <sigh>
{
private:
   void processSelection(U32 index) { }   // Needed for MenuUserInterface subclassing... does nothing

   bool anythingChanged();           // Compare list of parameters from before and after a session in the GameParams menu.  Did anything get changed??

   string origGameParams;            // Copy of the game parameters as specified when we activated the GameParameters, used to compare before and after to detect changes

   S32 mQuitItemIndex;               // Index of our quit item -- will vary depending on how many game-specific parameters there are
   S32 mGameSpecificParams;          // How many game specific parameters do we have?

   virtual S32 getTextSize() { return 18; }
   virtual S32 getGap() { return 12; }

   typedef map<const char *, boost::shared_ptr<MenuItem> > MenuItemMap;
   MenuItemMap mMenuItemMap;


public:
   GameParamUserInterface();   // Constructor

   S32 selectedIndex;          // Highlighted menu item
   S32 changingItem;           // Index of key we're changing (in keyDef mode), -1 otherwise

   void updateMenuItems(Game *game);

   Vector<SavedMenuItem> savedMenuItems;   

   void onActivate();
   void onEscape();
};

extern GameParamUserInterface gGameParamUserInterface;

};

#endif

