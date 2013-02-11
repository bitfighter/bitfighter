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

#ifndef _LOADOUTSELECT_H_
#define _LOADOUTSELECT_H_

#include "helperMenu.h"

using namespace TNL;
#include "ship.h"


namespace Zap
{

struct LoadoutItem
{
   InputCode key;            // Keyboard key used to select in loadout menu
   InputCode button;         // Controller button used to select in loadout menu
   U32 index;
   const char *text;       // Longer name used on loadout menu
   const char *help;       // An additional bit of help text, also displayed on loadout menu
   ShipModule requires;    // Item requires this module be part of loadout (used only for spy-bugs)

   ClientGame *mGame;

   //LoadoutItem(); // { /* Do nothing */ };      // Should never be used

   LoadoutItem(ClientGame *game, InputCode key, InputCode button, U32 index);      // Shortcut for modules -- use info from ModuleInfos

   LoadoutItem(ClientGame *game, InputCode key, InputCode button, U32 index, const char *text, const char *help, ShipModule requires);
};


////////////////////////////////////////
////////////////////////////////////////

class UIManager;

class LoadoutHelper : public HelperMenu
{
   typedef HelperMenu Parent;

private:
   U32 mModule[ShipModuleCount];   // Modules selected by user -- 2
   U32 mWeapon[ShipWeaponCount];   // Weapons selected by user -- 3
   S32 mCurrentIndex;

   const char *getCancelMessage();
   InputCode getActivationKey();

   bool isValidItem(S32 index);    // Do we have the required prerequisites for this item?

public:
   explicit LoadoutHelper(ClientGame *clientGame);    // Constructor
   void initialize(bool includeEngineer);    // Set things up

   void render();                
   void onMenuShow();  
   bool processInputCode(InputCode inputCode);   

   void activateHelp(UIManager *uiManager);  // Open help to an appropriate page
};

};

#endif


