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
}


LoadoutItem::LoadoutItem(ClientGame *game, InputCode key, InputCode button, U32 index, const char *text, const char *help, ShipModule requires) 
{
   this->key = key;
   this->button = button;
   this->index = index;
   this->text = text;
   this->help = help;
   this->requires = requires;
}

////////////////////////////////////////
////////////////////////////////////////

// May need to make these a class object in future if we ever do true multiplayer within one client... we could have
// conflicts if two players try to change loadouts at the same time.

static OverlayMenuItem loadoutModuleMenuItems[] = {
   { KEY_1, BUTTON_1, true, false, "Turbo Boost",           "" },
   { KEY_2, BUTTON_2, true, false, "Shield Generator",      "" },
   { KEY_3, BUTTON_3, true, false, "Repair Module",         "" },
   { KEY_4, BUTTON_4, true, false, "Enhanced Sensor",       "" },
   { KEY_5, BUTTON_5, true, false, "Cloak Field Modulator", "" },
   { KEY_6, BUTTON_6, true, false, "Armor",                 "" },
   { KEY_7, BUTTON_7, true, false, "Engineer",              "" },
};

static const S32 moduleEngineerIndex = 6;

static OverlayMenuItem loadoutWeaponMenuItems[] = {
   { KEY_1, BUTTON_1, true, false, "Phaser",     "" },
   { KEY_2, BUTTON_2, true, false, "Bouncer",    "" },
   { KEY_3, BUTTON_3, true, false, "Triple",     "" },
   { KEY_4, BUTTON_4, true, false, "Burster",    "" },
   { KEY_5, BUTTON_5, true, false, "Mine Layer", "" },
   { KEY_6, BUTTON_6, true, false, "Seeker",     "" },
};


// must be synchronized with above... TODO: put this in an xmacro!
static U8 moduleLookup[] = 
{
   ModuleBoost,
   ModuleShield,
   ModuleRepair,
   ModuleSensor,
   ModuleCloak,
   ModuleArmor,
   ModuleEngineer
};

static U8 weaponLookup[] = 
{
   WeaponPhaser,
   WeaponBounce,
   WeaponTriple,
   WeaponBurst, 
   WeaponMine,  
   WeaponSeeker
};

////////////////////////////////////////
////////////////////////////////////////

// Constructor
LoadoutHelper::LoadoutHelper()
{
   mCurrentIndex = 0;
}


HelperMenu::HelperMenuType LoadoutHelper::getType() { return LoadoutHelperType; }


// Gets called at the beginning of every game; available options may change based on level
void LoadoutHelper::pregameSetup(bool includeEngineer)
{
   loadoutModuleMenuItems[moduleEngineerIndex].showOnMenu = includeEngineer;
}


void LoadoutHelper::onActivated()
{
   Parent::onActivated();

   mCurrentIndex = 0;

   // Mark everything as unselected
   for(S32 i = 0; i < ARRAYSIZE(loadoutWeaponMenuItems); i++)
      loadoutWeaponMenuItems[i].markAsSelected = false;

   for(S32 i = 0; i < ARRAYSIZE(loadoutModuleMenuItems); i++)
      loadoutModuleMenuItems[i].markAsSelected = false;
}


void LoadoutHelper::render()
{
   S32 xPos = getLeftEdgeOfMenuPos();
   S32 yPos = MENU_TOP;
   const S32 fontSize = 15;

   const Color loadoutMenuHeaderColor (Colors::red);
   char title[100];

   if(mCurrentIndex < ShipModuleCount)
      dSprintf(title, sizeof(title), "Pick %d power modules for your ship:", ShipModuleCount);
   else
      dSprintf(title, sizeof(title), "Pick %d weapons for your ship:",       ShipWeaponCount);


   // Point list to either the weapon list or the module list, depending on mCurrentIndex
   OverlayMenuItem *list = (mCurrentIndex < ShipModuleCount) ? loadoutModuleMenuItems            : loadoutWeaponMenuItems;
   S32              len =  (mCurrentIndex < ShipModuleCount) ? ARRAYSIZE(loadoutModuleMenuItems) : ARRAYSIZE(loadoutWeaponMenuItems);

   drawItemMenu(getLeftEdgeOfMenuPos(), yPos, title, list, len);
}


// Return true if key did something, false if key had no effect
// Runs on client
bool LoadoutHelper::processInputCode(InputCode inputCode)
{
   if(Parent::processInputCode(inputCode))    // Check for cancel keys
      return true;
   
   U32 index;
   OverlayMenuItem *list = (mCurrentIndex < ShipModuleCount) ? loadoutModuleMenuItems            : loadoutWeaponMenuItems;
   U32              len =  (mCurrentIndex < ShipModuleCount) ? ARRAYSIZE(loadoutModuleMenuItems) : ARRAYSIZE(loadoutWeaponMenuItems);

   for(index = 0; index < len; index++)
      if(inputCode == list[index].key || inputCode == list[index].button)
         break;

   if(!list[index].showOnMenu)
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
      list[index].markAsSelected = true;
      mModule[mCurrentIndex] = index;
      mCurrentIndex++;
   }

   if(mCurrentIndex == ShipModuleCount + ShipWeaponCount)     // All loadout options selected, process complete
   {
      // Load the loadouts into a vector, and send them off to the GameConnection
      Vector<U8> loadout;

      for(S32 i = 0; i < ShipModuleCount; i++)
         loadout.push_back(moduleLookup[mModule[i]]);

      for(S32 i = 0; i < ShipWeaponCount; i++)
         loadout.push_back(weaponLookup[mWeapon[i]]);

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

