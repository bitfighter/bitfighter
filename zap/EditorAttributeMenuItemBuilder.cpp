//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo relased for Torque Network Library by GarageGames.com
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

#include "EditorAttributeMenuItemBuilder.h"
#include "BfObject.h"
#include "UIEditorMenus.h"
#include "moveObject.h"
#include "CoreGame.h"      // For CoreItem static values
#include "EngineeredItem.h"
#include "Spawn.h"
#include "PickupItem.h"

#include "textItem.h"


namespace Zap
{


EditorAttributeMenuItemBuilder::EditorAttributeMenuItemBuilder()
{
   mInitialized = false;
}

// Destructor
EditorAttributeMenuItemBuilder::~EditorAttributeMenuItemBuilder()
{
   // Do nothing
}


void EditorAttributeMenuItemBuilder::initialize(ClientGame *game)
{
   mGame = game;
   mInitialized = true;
}


// Since many of these attribute menus will never be shown in a given session, and each is relatively inexpensive to build,
// we'll create them lazily on an as-needed basis.  Each section below has a static pointer enclosed in a block so that it
// will be isloated from other similar variables with the same name.  Since these are statics, they will be destroyed only
// when this object is destroyed, which will be when the game exits.

EditorAttributeMenuUI *EditorAttributeMenuItemBuilder::getAttributeMenu(BfObject *obj)
{
   TNLAssert(mInitialized, "Must initialize before use!");

   switch(obj->getObjectTypeNumber())
   {
      case AsteroidTypeNumber:
      {
         static EditorAttributeMenuUI *attributeMenuUI = NULL;

         if(!attributeMenuUI)
         {
            attributeMenuUI = new EditorAttributeMenuUI(mGame);

            attributeMenuUI->addMenuItem(
                  new CounterMenuItem("Size:", Asteroid::ASTEROID_INITIAL_SIZELEFT, 1, 1, Asteroid::ASTEROID_SIZELEFT_MAX, "", "", "")
               );

            // Add our standard save and exit option to the menu
            attributeMenuUI->addSaveAndQuitMenuItem();
         }

         return attributeMenuUI;
      }

      case ShipSpawnTypeNumber:
      case CircleSpawnTypeNumber:
      case AsteroidSpawnTypeNumber:
      case FlagSpawnTypeNumber:
      {
         if(static_cast<AbstractSpawn *>(obj)->getDefaultRespawnTime() == -1)  // No editing RespawnTimer for Ship Spawn
            return NULL;

         static EditorAttributeMenuUI *attributeMenuUI = NULL;

         if(!attributeMenuUI)
         {
            ClientGame *clientGame = static_cast<ClientGame *>(mGame);

            attributeMenuUI = new EditorAttributeMenuUI(clientGame);

            CounterMenuItem *menuItem = new CounterMenuItem("Spawn Timer:", 999, 1, 0, 1000, "secs", "Never spawns", 
                                                            "Time it takes for each item to be spawned");
            attributeMenuUI->addMenuItem(menuItem);

            // Add our standard save and exit option to the menu
            attributeMenuUI->addSaveAndQuitMenuItem();
         }

          return attributeMenuUI;
      }

      case CoreTypeNumber:
      {
         static EditorAttributeMenuUI *attributeMenuUI = NULL;

         if(!attributeMenuUI)
         {
            ClientGame *clientGame = static_cast<ClientGame *>(mGame);

            attributeMenuUI = new EditorAttributeMenuUI(clientGame);

            attributeMenuUI->addMenuItem(new CounterMenuItem("Hit points:", CoreItem::CoreDefaultStartingHealth,
                                         1, 1, S32(CoreItem::DamageReductionRatio), "", "", ""));

            // Add our standard save and exit option to the menu
            attributeMenuUI->addSaveAndQuitMenuItem();
         }

         return attributeMenuUI;
      }


      case TurretTypeNumber:
      case ForceFieldProjectorTypeNumber:
      {
         static EditorAttributeMenuUI *attributeMenuUI = NULL;

         if(!attributeMenuUI)
         {
            ClientGame *clientGame = static_cast<ClientGame *>(mGame);

            attributeMenuUI = new EditorAttributeMenuUI(clientGame);

            // Value doesn't matter (set to 99 here), as it will be clobbered when startEditingAttrs() is called
            CounterMenuItem *menuItem = new CounterMenuItem("10% Heal:", 99, 1, 0, 100, "secs", "Disabled", 
                                                            "Time for this item to heal itself 10%");
            attributeMenuUI->addMenuItem(menuItem);

            // Add our standard save and exit option to the menu
            attributeMenuUI->addSaveAndQuitMenuItem();
         }

         return attributeMenuUI;
      }


      case RepairItemTypeNumber:
      case EnergyItemTypeNumber:
      {
         static EditorAttributeMenuUI *attributeMenuUI = NULL;

         if(!attributeMenuUI)
         {
            ClientGame *clientGame = static_cast<ClientGame *>(mGame);

            attributeMenuUI = new EditorAttributeMenuUI(clientGame);

            // Value doesn't matter (set to 99 here), as it will be clobbered when startEditingAttrs() is called
            CounterMenuItem *menuItem = new CounterMenuItem("Regen Time:", 99, 1, 0, 100, "secs", "No regen", 
                                                            "Time for this item to reappear after it has been picked up");

            attributeMenuUI->addMenuItem(menuItem);

            // Add our standard save and exit option to the menu
            attributeMenuUI->addSaveAndQuitMenuItem();
         }

         return attributeMenuUI;
      }
   
      case TextItemTypeNumber:
      {
         static EditorAttributeMenuUI *attributeMenuUI = NULL;

         // Lazily initialize this -- if we're in the game, we'll never need this to be instantiated
         if(!attributeMenuUI)
         {
            attributeMenuUI = new EditorAttributeMenuUI(static_cast<ClientGame *>(mGame));

            // "Blah" will be overwritten when startEditingAttrs() is called
            TextEntryMenuItem *menuItem = new TextEntryMenuItem("Text: ", "Blah", "", "", MAX_TEXTITEM_LEN);
            menuItem->setTextEditedCallback(TextItem::textEditedCallback);

            attributeMenuUI->addMenuItem(menuItem);

            // Add our standard save and exit option to the menu
            attributeMenuUI->addSaveAndQuitMenuItem();
         }

         return attributeMenuUI;
      }

      default:
         return obj->getAttributeMenu();
   }
}


// Get the menu looking like what we want (static)
void EditorAttributeMenuItemBuilder::startEditingAttrs(EditorAttributeMenuUI *attributeMenu, BfObject *obj)
{
   switch(obj->getObjectTypeNumber())
   {
      case AsteroidTypeNumber:
         attributeMenu->getMenuItem(0)->setIntValue(static_cast<Asteroid *>(obj)->getCurrentSize());
         break;

      case ShipSpawnTypeNumber:
      case CircleSpawnTypeNumber:
      case AsteroidSpawnTypeNumber:
      case FlagSpawnTypeNumber:
         attributeMenu->getMenuItem(0)->setIntValue(static_cast<AbstractSpawn *>(obj)->getSpawnTime());
         break;

      case CoreTypeNumber:
         attributeMenu->getMenuItem(0)->setIntValue(S32(static_cast<CoreItem *>(obj)->getStartingHealth() + 0.5));
         break;

      case TurretTypeNumber:
      case ForceFieldProjectorTypeNumber:
         attributeMenu->getMenuItem(0)->setIntValue(static_cast<EngineeredItem *>(obj)->getHealRate());
         break;

      case RepairItemTypeNumber:
      case EnergyItemTypeNumber:
         attributeMenu->getMenuItem(0)->setIntValue(static_cast<PickupItem *>(obj)->getRepopDelay());
         break;

      case TextItemTypeNumber:
         attributeMenu->getMenuItem(0)->setValue(static_cast<TextItem *>(obj)->getText());
         break;

      default:
         obj->startEditingAttrs(attributeMenu);
   }
}


// Retrieve the values we need from the menu (static)
void EditorAttributeMenuItemBuilder::doneEditingAttrs(EditorAttributeMenuUI *attributeMenu, BfObject *obj)
{
   switch(obj->getObjectTypeNumber())
   {
      case AsteroidTypeNumber:
         static_cast<Asteroid *>(obj)->setCurrentSize(attributeMenu->getMenuItem(0)->getIntValue());
         break;

      case ShipSpawnTypeNumber:
      case CircleSpawnTypeNumber:
      case AsteroidSpawnTypeNumber:
      case FlagSpawnTypeNumber:
         static_cast<AbstractSpawn *>(obj)->setSpawnTime(attributeMenu->getMenuItem(0)->getIntValue());
         break;

      case CoreTypeNumber:
         static_cast<CoreItem *>(obj)->setStartingHealth(F32(attributeMenu->getMenuItem(0)->getIntValue()));
         break;

         
      case TurretTypeNumber:
      case ForceFieldProjectorTypeNumber:
         static_cast<EngineeredItem *>(obj)->setHealRate(attributeMenu->getMenuItem(0)->getIntValue());

      case RepairItemTypeNumber:
      case EnergyItemTypeNumber:
         static_cast<PickupItem *>(obj)->setRepopDelay(attributeMenu->getMenuItem(0)->getIntValue());
         break;

      case TextItemTypeNumber:
         static_cast<TextItem *>(obj)->setText(attributeMenu->getMenuItem(0)->getValue());
         break;

      default:
         obj->doneEditingAttrs(attributeMenu);
   }
}


}
