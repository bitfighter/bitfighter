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

enum ParamType
{
   TypeShortString,     // gMaxGameNameLength max length
   TypeLongString,      // gMaxGameDescrLength max length
   TypeInt,
   TypeGameType,
   TypeFileName,
   TypeNone
};

struct MenuItem2     // An extended menu item complete with values good for editing!
{
   const char *mText;  // Text displayed on menu
   KeyCode key1;       // Allow two shortcut keys per menu item...
   KeyCode key2;

   // Note that menuItems can have both string and int values.  So we'll create a list of values of each type, and use menuValIsString to tell us which to use.
   string mValS;       // List of the values we've entered for each game param (strings)
   S32 mValI;          // List of the values we've entered for each game param (ints)
   S32 mMinVal;        // Minimum value for this parameter
   S32 mMaxVal;        // Maximum value for this parameter
   const char *mUnits; // What units do numeric values represent?
   const char *mHelp;  // A brief help string

   ParamType mValType; // What type of data to show for this item

   Color color;

   // Constructor
   MenuItem2() { }      // Not used, but compiler is giving me grief, so let's add this and see if all is in order
   // Constructor
   MenuItem2(const char *text, string strVal, S32 intVal, S32 min, S32 max, const char *units, const char *help, ParamType valType, KeyCode k1, KeyCode k2, Color c = Color(1, 1, 1))
   {
      mText = text;
      mValS = strVal;
      mValI = intVal;
      mUnits = units;
      mHelp = help;
      mValType = valType;
      mMinVal = min;
      mMaxVal = max;
      key1 = k1;
      key2 = k2;
      color = c;
   }
};


class GameParamUserInterface : public MenuUserInterface     // By subclassing this, I hoped to get the mouse stuff to automatically work, but it didn't.  <sigh>
{
private:
   void updateMenuItems(const char *gameType);  // Update list of menu items, based on provided game type. (Different game types require different params, after all)  Wraps the S32 version.
   void updateMenuItems(S32 gameTypeIndex);     // Update list of menu items, based on gGameTypeNames index
   void processSelection(U32 index) { }         // Needed for MenuUserInterface subclassing... does nothing

   bool didAnythingGetChanged();                // Compare list of parameters from before and after a session in the GameParams menu.  Did anything get changed??
   void buildGameParamList();                   // Take the info from our menus and create a list of lines we can stick in a level file (which we'll store in gameParams)

   Vector<string> origGameParams;    // Copy of the game parameters as specified when we activated the GameParameters, used to compare before and after to detect changes

   S32 mQuitItemIndex;               // Index of our quit item -- will vary depending on how many game-specific parameters there are
   S32 mGameSpecificParams;          // How many game specific parameters do we have?

public:
   GameParamUserInterface();         // Constructor
   const char *menuTitle;
   const char *menuSubTitle;
   const char *menuFooter;

   Vector<string> gameParams;  // Assorted game params, such as gridSize et al

   S32 selectedIndex;          // Highlighted menu item
   S32 changingItem;           // Index of key we're changing (in keyDef mode), -1 otherwise

   Vector<MenuItem2> savedMenuItems;
   Vector<MenuItem2> menuItems;
   void render();              // Draw the menu
   void idle(U32 timeDelta);
   void onKeyDown(KeyCode keyCode, char ascii);
   void onMouseMoved(S32 x, S32 y);


   void onActivate();
   void onEscape();

   bool ignoreGameParams; 
};

extern GameParamUserInterface gGameParamUserInterface;

};

#endif

