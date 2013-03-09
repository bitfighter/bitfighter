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

#include "loadoutHelper.h"

#include "InputCode.h"     // For BindingName enum
#include "UIGame.h"
#include "UIMenus.h"
#include "UIManager.h"
#include "UIInstructions.h"
#include "gameType.h"
#include "Colors.h"
#include "ClientGame.h"
#include "JoystickRender.h"
#include "config.h"

#include "OpenglUtils.h"


namespace Zap
{

Vector<LoadoutItem> gLoadoutModules;
Vector<LoadoutItem> gLoadoutWeapons;

//LoadoutItem::LoadoutItem() { TNLAssert(false, "Do nothing, Should never be used"); }

LoadoutItem::LoadoutItem(ClientGame *game, InputCode key, InputCode button, U32 index)      // Shortcut for modules -- use info from ModuleInfos
{
   const ModuleInfo *moduleInfo = game->getModuleInfo((ShipModule) index);

   this->key = key;
   this->button = button;
   this->index = index;
   this->text = moduleInfo->getMenuName();
   this->help = moduleInfo->getMenuHelp();
   this->requires = ModuleNone;     // Currently, no modules depend on any other

   mGame = game;
}


LoadoutItem::LoadoutItem(ClientGame *game, InputCode key, InputCode button, U32 index, const char *text, const char *help, ShipModule requires) 
{
   this->key = key;
   this->button = button;
   this->index = index;
   this->text = text;
   this->help = help;
   this->requires = requires;

   mGame = game;
}

////////////////////////////////////////
////////////////////////////////////////

// Constructor
LoadoutHelper::LoadoutHelper(ClientGame *clientGame) : Parent(clientGame)
{
   mCurrentIndex = 0;
}


// Gets called at the beginning of every game; available options may change based on level
void LoadoutHelper::initialize(bool includeEngineer)
{
   gLoadoutModules.clear();
   gLoadoutWeapons.clear();

   gLoadoutModules.push_back(LoadoutItem(getGame(), KEY_1, BUTTON_1, ModuleBoost));
   gLoadoutModules.push_back(LoadoutItem(getGame(), KEY_2, BUTTON_2, ModuleShield));
   gLoadoutModules.push_back(LoadoutItem(getGame(), KEY_3, BUTTON_3, ModuleRepair));
   gLoadoutModules.push_back(LoadoutItem(getGame(), KEY_4, BUTTON_4, ModuleSensor));
   gLoadoutModules.push_back(LoadoutItem(getGame(), KEY_5, BUTTON_5, ModuleCloak));
   gLoadoutModules.push_back(LoadoutItem(getGame(), KEY_6, BUTTON_6, ModuleArmor));

   if(includeEngineer)
      gLoadoutModules.push_back(LoadoutItem(getGame(), KEY_7, BUTTON_7, ModuleEngineer));

   gLoadoutWeapons.push_back(LoadoutItem(getGame(), KEY_1, BUTTON_1, WeaponPhaser, "Phaser",     "", ModuleNone));
   gLoadoutWeapons.push_back(LoadoutItem(getGame(), KEY_2, BUTTON_2, WeaponBounce, "Bouncer",    "", ModuleNone));
   gLoadoutWeapons.push_back(LoadoutItem(getGame(), KEY_3, BUTTON_3, WeaponTriple, "Triple",     "", ModuleNone));
   gLoadoutWeapons.push_back(LoadoutItem(getGame(), KEY_4, BUTTON_4, WeaponBurst,  "Burster",    "", ModuleNone));
   gLoadoutWeapons.push_back(LoadoutItem(getGame(), KEY_5, BUTTON_5, WeaponMine,   "Mine Layer", "", ModuleNone));
   gLoadoutWeapons.push_back(LoadoutItem(getGame(), KEY_6, BUTTON_6, WeaponSeeker, "Seeker",     "", ModuleNone));
};


void LoadoutHelper::onMenuShow()
{
   Parent::onMenuShow();

   mCurrentIndex = 0;
}


// First, we work with modules, then with weapons
#define getList(ct)  ((ct < ShipModuleCount) ? &gLoadoutModules : &gLoadoutWeapons)

void LoadoutHelper::render()
{
   S32 xPos = getLeftEdgeOfMenuPos();
   S32 yPos = MENU_TOP;
   const S32 fontSize = 15;

   const Color loadoutMenuHeaderColor (Colors::red);
   char helpStr[100];

   if(mCurrentIndex < ShipModuleCount)
      dSprintf(helpStr, sizeof(helpStr), "Pick %d power modules for your ship:", ShipModuleCount);
   else
      dSprintf(helpStr, sizeof(helpStr), "Pick %d weapons for your ship:", ShipWeaponCount);


   drawMenuBorderLine(xPos, yPos, loadoutMenuHeaderColor);

   glColor(loadoutMenuHeaderColor);
   drawString(xPos, yPos, fontSize, helpStr);
   yPos += fontSize + 10;

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
      GameSettings *settings = getGame()->getSettings();
      InputMode inputMode = settings->getInputCodeManager()->getInputMode();

      bool showKeys = settings->getIniSettings()->showKeyboardKeys || inputMode == InputModeKeyboard;

      if(isValidItem(i))
      {
         U32 joystickIndex = Joystick::SelectedPresetIndex;

         if(inputMode == InputModeJoystick)     // Only draw joystick buttons when in joystick mode
            JoystickRender::renderControllerButton(F32(xPos + (showKeys ? 0 : 20)), (F32)yPos, 
                                                   joystickIndex, list->get(i).button, false);
         if(showKeys)
         {
            glColor(Colors::white);     
            JoystickRender::renderControllerButton(F32(xPos + 30), (F32)yPos, joystickIndex, list->get(i).key, false);
         }

         if(selected)
            glColor(1.0, 0.1f, 0.1f);      // Color of already selected item
         else
            glColor(0.1f, 1.0, 0.1f);      // Color of not-yet selected item

         S32 textPos = xPos + 50;
         textPos += drawStringAndGetWidth(textPos, yPos, fontSize, list->get(i).text) + 8;      // The loadout entry itself
         if(!selected)
            glColor(.2f, .8f, .8f);        // Color of help message

         drawString(textPos, yPos, fontSize, list->get(i).help);      // The loadout help string, if there is one

         yPos += fontSize + 7;
      }
   }

   // Add some help text
   drawMenuBorderLine(xPos, yPos - fontSize - 2, loadoutMenuHeaderColor);
   yPos += 8;
   drawMenuCancelText(xPos, yPos, loadoutMenuHeaderColor, fontSize);
}


// Checks if there are prerequisites for item, and returns true if there are none, or they are satisfied, false if they are unsatisified
bool LoadoutHelper::isValidItem(S32 index)
{
   Vector<LoadoutItem> *list = getList(mCurrentIndex);   // Gets list of modules or weapons, whichever is active

   if(list->get(index).requires == ModuleNone)           // Selection has no prerequisites
      return true;

   // There are prerequsities... check to make sure user has already selected them
   for(S32 i = 0; i < min(mCurrentIndex, ShipModuleCount); i++)
      if(gLoadoutModules[mModule[i]].index == (U32)list->get(index).requires)    // They have
         return true;

   // There are prerequisites, but the user doesn't have them
   return false;
}


// Return true if key did something, false if key had no effect
// Runs on client
bool LoadoutHelper::processInputCode(InputCode inputCode)
{
   if(Parent::processInputCode(inputCode))    // Check for cancel keys
      return true;
   
   U32 index;
   Vector<LoadoutItem> *list = getList(mCurrentIndex);

   for(index = 0; index < (U32)list->size(); index++)
      if(inputCode == list->get(index).key || inputCode == list->get(index).button)
         break;

   if(index >= (U32)list->size())
      return false;

   if(!isValidItem(index))
      return false;

   if(!list->get(index).text)
      return false;

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
         if(mWeapon[i - ShipModuleCount] == index)
            alreadyUsed = true;

   if(!alreadyUsed)
   {
      mModule[mCurrentIndex] = index;
      mCurrentIndex++;
   }

   if(mCurrentIndex == ShipModuleCount + ShipWeaponCount)     // All loadout options selected, process complete
   {
      // Load the loadouts into a vector, and send them off to the GameConnection
      Vector<U8> loadout;

      for(S32 i = 0; i < ShipModuleCount; i++)
         loadout.push_back(gLoadoutModules[mModule[i]].index);

      for(S32 i = 0; i < ShipWeaponCount; i++)
         loadout.push_back(gLoadoutWeapons[mWeapon[i]].index);

      

      GameConnection *conn = getGame()->getConnectionToServer();

      if(conn)
      {
         if(getGame()->getSettings()->getIniSettings()->verboseHelpMessages)
            getGame()->displayShipDesignChangedMessage(loadout, "Modifications canceled -- new ship design same as the current");     

         // Request loadout even if it was the same -- if I have loadout A, with on-deck loadout B, and I enter a new loadout
         // that matches A, it would be better to have loadout remain unchanged if I entered a loadout zone.
         // Tell server loadout has changed.  Server will activate it when we enter a loadout zone.
         conn->c2sRequestLoadout(loadout);     
      }
      exitHelper();     
   }

   return true;
}


InputCode LoadoutHelper::getActivationKey() 
{ 
   GameSettings *settings = getGame()->getSettings();
   return settings->getInputCodeManager()->getBinding(InputCodeManager::BINDING_LOADOUT); 
}


const char *LoadoutHelper::getCancelMessage()
{
   return "Modifications canceled -- ship design unchanged.";
}




void LoadoutHelper::activateHelp(UIManager *uiManager)
{
   // Go to module help page
   if(mCurrentIndex < ShipModuleCount)
      uiManager->getInstructionsUserInterface()->activatePage(InstructionsUserInterface::InstructionModules);
   // Go to weapons help page
   else
      uiManager->getInstructionsUserInterface()->activatePage(InstructionsUserInterface::InstructionWeaponProjectiles);
}


};

