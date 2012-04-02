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

#ifndef _UIKEYDEFMENUS_H_
#define _UIKEYDEFMENUS_H_

#include "../tnl/tnlTypes.h"

#include "UI.h"
#include "input.h"
#include "Timer.h"
#include <string>

namespace Zap
{

using namespace std;

////////////////////////////////////////
////////////////////////////////////////

struct KeyDefMenuItem
{
   const char *mText;
   U32 mIndex;
   U32 mColumn;
   InputCodeManager::BindingName primaryControl;
   string helpString;

   KeyDefMenuItem(const char *text, U32 index, U32 col, InputCodeManager::BindingName PC, string helpStr);     // Constructor
};


////////////////////////////////////////
////////////////////////////////////////

class KeyDefMenuUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   S32 itemsPerCol;           // Approx. half of the items we have
   Timer errorMsgTimer;
   string errorMsg;
   static const S32 errorMsgDisplayTime = 6000;

   bool isDuplicate(S32 key, const Vector<KeyDefMenuItem> &menuItems);

public:
   KeyDefMenuUserInterface(ClientGame *game);   // Constructor
   Vector<KeyDefMenuItem> menuItems;      // Array of menu items
   const char *mMenuTitle;
   const char *mMenuSubTitle;
   Color mMenuSubTitleColor;

   const char *mMenuFooter;

   S32 selectedIndex;          // Highlighted menu item
   S32 changingItem;           // Index of key we're changing (in keyDef mode), -1 otherwise

   void render();              // Draw the menu
   void idle(U32 timeDelta);
   bool onKeyDown(InputCode inputCode);

   void onMouseMoved();

   void onActivate();
};

};

#endif

