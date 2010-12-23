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

#include "loadoutSelect.h"
#include "UIGame.h"
#include "gameConnection.h"
#include "game.h"
#include "shipItems.h"
#include "gameWeapons.h"
#include "gameObjectRender.h"
#include "UIMenus.h"
#include "config.h"
#include "gameType.h"

#include "../glut/glutInclude.h"
#include <ctype.h>


namespace Zap
{

Vector<LoadoutItem> gLoadoutModules;
Vector<LoadoutItem> gLoadoutWeapons;

// Gets called at the beginning of every game; available options may change based on level
void LoadoutHelper::initialize(bool includeEngineer)
{
   gLoadoutModules.clear();
   gLoadoutWeapons.clear();

   gLoadoutModules.push_back(LoadoutItem(KEY_1, BUTTON_1, ModuleBoost));
   gLoadoutModules.push_back(LoadoutItem(KEY_2, BUTTON_2, ModuleShield));
   gLoadoutModules.push_back(LoadoutItem(KEY_3, BUTTON_3, ModuleRepair));
   gLoadoutModules.push_back(LoadoutItem(KEY_4, BUTTON_4, ModuleSensor));
   gLoadoutModules.push_back(LoadoutItem(KEY_5, BUTTON_5, ModuleCloak));
   gLoadoutModules.push_back(LoadoutItem(KEY_6, BUTTON_6, ModuleArmor));

   if(includeEngineer)
      gLoadoutModules.push_back(LoadoutItem(KEY_7, BUTTON_7, ModuleEngineer));

   gLoadoutWeapons.push_back(LoadoutItem(KEY_1, BUTTON_1, WeaponPhaser,  "Phaser",          "", ModuleNone));
   gLoadoutWeapons.push_back(LoadoutItem(KEY_2, BUTTON_2, WeaponBounce,  "Bouncer",         "", ModuleNone));
   gLoadoutWeapons.push_back(LoadoutItem(KEY_3, BUTTON_3, WeaponTriple,  "Triple",          "", ModuleNone));
   gLoadoutWeapons.push_back(LoadoutItem(KEY_4, BUTTON_4, WeaponBurst,   "Burster",         "", ModuleNone));
   gLoadoutWeapons.push_back(LoadoutItem(KEY_5, BUTTON_5, WeaponMine,    "Mine Layer",      "", ModuleNone));
// { KEY_6, 5, WeaponHeatSeeker, "Heat Seeker"},      // Need to make changes below to support this
   gLoadoutWeapons.push_back(LoadoutItem(KEY_6, BUTTON_6, WeaponSpyBug,  "Spy Bug Placer",  "", ModuleSensor));  // Only visible when Enhanced Sensor is a selected module
};

const char *gLoadoutTitles[] = {
   "First:",
   "Second:",
   "First:",
   "Second:",
   "Third:",
};

LoadoutHelper::LoadoutHelper()
{
}

void LoadoutHelper::show(bool fromController)
{
   mFromController = fromController;
   mCurrentIndex = 0;
   mIdleTimer.reset(MenuTimeout);
}

extern void renderControllerButton(F32 x, F32 y, KeyCode keyCode, bool activated, S32 offset);
extern IniSettings gIniSettings;
extern S32 getControllerButtonRenderedSize(KeyCode keyCode);


// First, we work with modules, then with weapons
#define getList(ct)  ((ct < ShipModuleCount) ? &gLoadoutModules : &gLoadoutWeapons)

void LoadoutHelper::render()
{
   const Color loadoutMenuHeaderColor (1, 0, 0);
   S32 yPos = 300;
   const S32 fontSize = 15;
   char helpStr[100];

   if(mCurrentIndex < ShipModuleCount)
      dSprintf (helpStr, sizeof(helpStr), "Pick %d power modules for your ship:", ShipModuleCount);
   else
      dSprintf (helpStr, sizeof(helpStr), "Pick %d weapons for your ship:", ShipWeaponCount);

   glColor(loadoutMenuHeaderColor);
   UserInterface::drawStringf(UserInterface::horizMargin, yPos, fontSize, "%s", helpStr);
   yPos += fontSize + 4;

   Vector<LoadoutItem> *list = getList(mCurrentIndex);

   for(U32 i = 0; i < (U32)list->size(); i++)
   {
      bool selected = false;

      // Cases of mCurrentIndex == 0 and mCurrentIndex == ShipModuleCount should both fall through the if/else if
      // These are instances when a fresh new menu with nothing selected is displayed.

      if(mCurrentIndex > 0 && mCurrentIndex < ShipModuleCount)    // Picking modules, but not the first one
      {
         for(S32 j = 0; j < mCurrentIndex; j++)
            if(mModule[j] == i)
               selected = true;
      }
      else if(mCurrentIndex > ShipModuleCount)                    // Picking weapons, but not the first one
      {
         for(S32 j = 0; j < mCurrentIndex - ShipModuleCount; j++)
            if(mWeapon[j] == i)
               selected = true;
      }


      // Draw key controls for selecting loadout items

      if(isValidItem(i))
      {
         bool showKeys = gIniSettings.showKeyboardKeys || gIniSettings.inputMode == Keyboard;

         if(gIniSettings.inputMode == Joystick)     // Only draw joystick buttons when in joystick mode
            renderControllerButton(UserInterface::horizMargin + (showKeys ? 0 : 20), yPos, list->get(i).button, false);

         if(showKeys)
         {
            glColor3f(1, 1, 1);     // Render key in white
            renderControllerButton(UserInterface::horizMargin + 20, yPos, list->get(i).key, false);
         }

         if(selected)
            glColor3f(1.0, 0.1, 0.1);      // Color of already selected item
         else
            glColor3f(0.1, 1.0, 0.1);      // Color of not-yet selected item

         S32 xPos = UserInterface::horizMargin + 50;
         UserInterface::drawStringf(xPos, yPos, fontSize, "%s", list->get(i).text);      // The loadout entry itself
         if(!selected)
            glColor3f(.2, .8, .8);        // Color of help message
         xPos += UserInterface::getStringWidthf(fontSize, "%s ", list->get(i).text);
         UserInterface::drawStringf(xPos, yPos, fontSize, "%s", list->get(i).help);      // The loadout help string, if there is one

         yPos += fontSize + 7;
      }
   }
   // Add some help text
   glColor(loadoutMenuHeaderColor);
   S32 butSize = getControllerButtonRenderedSize(BUTTON_BACK);
   const S32 fontSizeSm = fontSize - 4;

   // RenderedSize will be -1 if the button is not defined
   if(gIniSettings.inputMode == Keyboard || butSize == -1)
   {
      UserInterface::drawStringf( UserInterface::horizMargin, yPos, fontSizeSm, "Press [%s] to cancel", keyCodeToString(KEY_ESCAPE) );
   }
   else
   {
      S32 xPos = UserInterface::horizMargin;
      UserInterface::drawString( xPos, yPos, fontSizeSm, "Press ");
      xPos += UserInterface::getStringWidth(fontSizeSm, "Press ");
      renderControllerButton(xPos, yPos, BUTTON_BACK, false, butSize / 2);
      xPos += butSize;
      glColor(loadoutMenuHeaderColor);
      UserInterface::drawString( xPos, yPos, fontSizeSm, " to cancel");
   }
}


bool LoadoutHelper::isValidItem(S32 index)
{
   Vector<LoadoutItem> *list = getList(mCurrentIndex);

   if(list->get(index).requires != ModuleNone)
   {
      for(S32 i = 0; i < min(mCurrentIndex, ShipModuleCount); i++)
         if(gLoadoutModules[mModule[i]].index == (U32)list->get(index).requires)    // Found prerequisite
            return true;
      return false;
   }
   else     // No prerequisites
      return true;
}


// Return true if key did something, false if key had no effect
// Runs on client
bool LoadoutHelper::processKeyCode(KeyCode keyCode)
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
         gGameUserInterface.displayMessage(Color(1.0, 0.5, 0.5), "%s", "Modifications canceled -- ship design unchanged.");

      return true;
   }

   U32 index;
   Vector<LoadoutItem> *list = getList(mCurrentIndex);

   for(index = 0; list->get(index).text; index++)
      if(keyCode == list->get(index).key || keyCode == list->get(index).button)
         break;

   if(!isValidItem(index))
      return false;

   if(!list->get(index).text)
      return false;
   mIdleTimer.reset(MenuTimeout);

   // Make sure user doesn't select the same loadout item twice
   bool alreadyUsed = false;

   if(mCurrentIndex < ShipModuleCount)    // We're working with modules
   {                                      // (note... braces required here!)
      for(S32 i = 0; i < mCurrentIndex && !alreadyUsed; i++)
         if(mModule[i] == index)
            alreadyUsed = true;
   }
   else                                   // We're working with weapons
      for(S32 i = ShipModuleCount; i < mCurrentIndex && !alreadyUsed; i++)
         if(mModule[i] == index)
            alreadyUsed = true;

   if(!alreadyUsed)
   {
      mModule[mCurrentIndex] = index;
      mCurrentIndex++;
   }

   if(mCurrentIndex == ShipModuleCount + ShipWeaponCount)     // All loadout options selected, process complete
   {
      // Load the loadouts into a vector, and send them off to the GameConnection
      Vector<U32> loadout;

      for(S32 i = 0; i < ShipModuleCount; i++)
         loadout.push_back(gLoadoutModules[mModule[i]].index);

      for(S32 i = 0; i < ShipWeaponCount; i++)
         loadout.push_back(gLoadoutWeapons[mWeapon[i]].index);

      gGameUserInterface.setPlayMode();                     // Exit loadout menu and resume play, however we leave this routine

      GameConnection *gc = gClientGame->getConnectionToServer();
      if(!gc)
         return true;

      Ship *ship = dynamic_cast<Ship *>(gc->getControlObject()); 
      if(!ship)
         return true;

      // Check to see if the new configuration is the same as the old.  If so, we have nothing to do.
      bool theSame = true;

      for(S32 i = 0; i < ShipModuleCount; i++)
         theSame = theSame && (gLoadoutModules[mModule[i]].index == (U32)ship->getModule(i));

      for(S32 i = ShipModuleCount; i < ShipWeaponCount + ShipModuleCount; i++)
         theSame = theSame && (gLoadoutWeapons[mWeapon[i - ShipModuleCount]].index == (U32)ship->getWeapon(i - ShipModuleCount));

      if(theSame)      // Don't bother if ship config hasn't changed
      {
         if(gIniSettings.verboseHelpMessages)
            gGameUserInterface.displayMessage(Color(1.0, 0.5, 0.5), "%s", "Modifications canceled -- new ship design same as the old.");
         return true;
      }

      gc->c2sRequestLoadout(loadout);     // Tell server our loadout has changed.  Server will check if we're in the zone and activate loadout, if needed

      GameType *gt = gClientGame->getGameType();
      bool spawnWithLoadout = gt->isSpawnWithLoadoutGame() || ! gt->levelHasLoadoutZone();

      // Check if we are in a loadout zone...  if so, it will be changed right away...
      // ...otherwise, display a notice to the player to head for a LoadoutZone
      // We've done a lot of work to get this message just right!  I hope players appreciate it!
      if(gIniSettings.verboseHelpMessages && !(ship && ship->isInZone(LoadoutZoneType)) )          
         gGameUserInterface.displayMessage(Color(1.0, 0.5, 0.5), 
                                           "Ship design changed -- %s%s", spawnWithLoadout ? "changes will be activated when you respawn" : "enter Loadout Zone to activate changes", 
                                           spawnWithLoadout && gt->levelHasLoadoutZone() ? " or enter Loadout Zone." : ".");
   }

   return true;
}

};

