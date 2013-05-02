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


namespace Zap
{

EditorAttributeMenuItemBuilder::EditorAttributeMenuItemBuilder()
{
   mInitialized = false;
}


void EditorAttributeMenuItemBuilder::initialize(ClientGame *game)
{
   mGame = game;
   mInitialized = true;
}


EditorAttributeMenuUI *EditorAttributeMenuItemBuilder::getAttributeMenu(BfObject *obj)
{
   TNLAssert(mInitialized, "Must initialize before use!");

   if(obj->getObjectTypeNumber() == AsteroidTypeNumber)
   {
      static EditorAttributeMenuUI *attributeMenuUI = NULL;

      // Lazily initialize this -- if we're in the game, we'll never need this to be instantiated
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
   else
      return obj->getAttributeMenu();
}


// Get the menu looking like what we want (static)
void EditorAttributeMenuItemBuilder::startEditingAttrs(EditorAttributeMenuUI *attributeMenu, BfObject *obj)
{
   if(obj->getObjectTypeNumber() == AsteroidTypeNumber)
      attributeMenu->getMenuItem(0)->setIntValue(static_cast<Asteroid *>(obj)->getCurrentSize());
   else
      obj->startEditingAttrs(attributeMenu);
}


// Retrieve the values we need from the menu (static)
void EditorAttributeMenuItemBuilder::doneEditingAttrs(EditorAttributeMenuUI *attributeMenu, BfObject *obj)
{
   if(obj->getObjectTypeNumber() == AsteroidTypeNumber)
      static_cast<Asteroid *>(obj)->setCurrentSize(attributeMenu->getMenuItem(0)->getIntValue());
   else
      obj->doneEditingAttrs(attributeMenu);
}


}