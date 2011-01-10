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

#include "UIGame.h"
#include "engineerHelper.h"
#include "point.h"      // For Color def

namespace Zap
{


// Constructor
EngineerHelper::EngineerHelper()
{
   // Do nothing
}


void EngineerHelper::show(bool fromController)
{
   mFromController = fromController;
}



void EngineerHelper::render()
{
   UserInterface::drawStringf(UserInterface::horizMargin, 300, 15, "In engineer mode!!");
   //const Color loadoutMenuHeaderColor (1, 0, 0);
   //S32 yPos = 300;
   //const S32 fontSize = 15;
   //char helpStr[100];

   //if(mCurrentIndex < ShipModuleCount)
   //   dSprintf (helpStr, sizeof(helpStr), "Pick %d power modules for your ship:", ShipModuleCount);
   //else
   //   dSprintf (helpStr, sizeof(helpStr), "Pick %d weapons for your ship:", ShipWeaponCount);

   //glColor(loadoutMenuHeaderColor);
   //UserInterface::drawStringf(UserInterface::horizMargin, yPos, fontSize, "%s", helpStr);
   //yPos += fontSize + 4;

   //Vector<LoadoutItem> *list = getList(mCurrentIndex);

   //for(U32 i = 0; i < (U32)list->size(); i++)
   //{
   //   bool selected = false;

   //   // Cases of mCurrentIndex == 0 and mCurrentIndex == ShipModuleCount should both fall through the if/else if
   //   // These are instances when a fresh new menu with nothing selected is displayed.

   //   if(mCurrentIndex > 0 && mCurrentIndex < ShipModuleCount)    // Picking modules, but not the first one
   //   {
   //      for(S32 j = 0; j < mCurrentIndex; j++)
   //         if(mModule[j] == i)
   //            selected = true;
   //   }
   //   else if(mCurrentIndex > ShipModuleCount)                    // Picking weapons, but not the first one
   //   {
   //      for(S32 j = 0; j < mCurrentIndex - ShipModuleCount; j++)
   //         if(mWeapon[j] == i)
   //            selected = true;
   //   }

   //   // Draw key controls for selecting loadout items

   //   if(isValidItem(i))
   //   {
   //      bool showKeys = gIniSettings.showKeyboardKeys || gIniSettings.inputMode == Keyboard;

   //      if(gIniSettings.inputMode == Joystick)     // Only draw joystick buttons when in joystick mode
   //         renderControllerButton(UserInterface::horizMargin + (showKeys ? 0 : 20), yPos, list->get(i).button, false);

   //      if(showKeys)
   //      {
   //         glColor3f(1, 1, 1);     // Render key in white
   //         renderControllerButton(UserInterface::horizMargin + 20, yPos, list->get(i).key, false);
   //      }

   //      if(selected)
   //         glColor3f(1.0, 0.1, 0.1);      // Color of already selected item
   //      else
   //         glColor3f(0.1, 1.0, 0.1);      // Color of not-yet selected item

   //      S32 xPos = UserInterface::horizMargin + 50;
   //      UserInterface::drawStringf(xPos, yPos, fontSize, "%s", list->get(i).text);      // The loadout entry itself
   //      if(!selected)
   //         glColor3f(.2, .8, .8);        // Color of help message
   //      xPos += UserInterface::getStringWidthf(fontSize, "%s ", list->get(i).text);
   //      UserInterface::drawStringf(xPos, yPos, fontSize, "%s", list->get(i).help);      // The loadout help string, if there is one

   //      yPos += fontSize + 7;
   //   }
   //}
   //// Add some help text
   //glColor(loadoutMenuHeaderColor);
   //S32 butSize = getControllerButtonRenderedSize(BUTTON_BACK);
   //const S32 fontSizeSm = fontSize - 4;

   //// RenderedSize will be -1 if the button is not defined
   //if(gIniSettings.inputMode == Keyboard || butSize == -1)
   //   UserInterface::drawStringf( UserInterface::horizMargin, yPos, fontSizeSm, "Press [%s] to cancel", keyCodeToString(KEY_ESCAPE) );
   //else
   //{
   //   S32 xPos = UserInterface::horizMargin;
   //   xPos += UserInterface::drawStringAndGetWidth( xPos, yPos, fontSizeSm, "Press ");
   //   renderControllerButton(xPos, yPos, BUTTON_BACK, false, butSize / 2);
   //   xPos += butSize;
   //   glColor(loadoutMenuHeaderColor);
   //   UserInterface::drawString( xPos, yPos, fontSizeSm, " to cancel");
   //}
}


// Return true if key did something, false if key had no effect
// Runs on client
bool EngineerHelper::processKeyCode(KeyCode keyCode)
{
   // First, check navigation keys.  When in keyboard mode, we allow the loadout key to toggle menu on and off...
   // we can't do this in joystick mode because it is likely that the loadout key is also used to select items
   // from the loadout menu.
   if(keyCode == KEY_ESCAPE || keyCode == KEY_BACKSPACE ||
      keyCode == KEY_LEFT   || keyCode == BUTTON_DPAD_LEFT ||
      keyCode == BUTTON_BACK || (gIniSettings.inputMode == Keyboard && keyCode == keyLOADOUT[gIniSettings.inputMode]) )
   {
      gGameUserInterface.setPlayMode();      // Return to play mode, ship design unchanged
      if(gIniSettings.verboseHelpMessages)
         gGameUserInterface.displayMessage(Color(1.0, 0.5, 0.5), "Engineered item not deployed");

      return true;
   }

   return true;
}

};

