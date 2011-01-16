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


#include "helperMenu.h"
#include "point.h"      // For Color def
#include "UIGame.h"     // For gGameUserInterface

namespace Zap
{

void HelperMenu::exitHelper() { gGameUserInterface.enterMode(GameUserInterface::PlayMode); }

// Returns true if key was handled, false if it should be further processed
bool HelperMenu::processKeyCode(KeyCode keyCode)
{
   // First, check navigation keys.  When in keyboard mode, we allow the loadout key to toggle menu on and off...
   // we can't do this in joystick mode because it is likely that the loadout key is also used to select items
   // from the loadout menu.
   if(keyCode == KEY_ESCAPE || keyCode == KEY_BACKSPACE ||
      keyCode == KEY_LEFT   || keyCode == BUTTON_DPAD_LEFT ||
      keyCode == BUTTON_BACK || (gIniSettings.inputMode == Keyboard && keyCode == getActivationKey()) )
   {
      exitHelper();      // Return to play mode, ship design unchanged
      if(gIniSettings.verboseHelpMessages)
         gGameUserInterface.displayMessage(Color(1.0, 0.5, 0.5), getCancelMessage());

      return true;
   }

   return false;
}

};