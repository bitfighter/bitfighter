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

#include "UIEditorMenus.h"
#include "textItem.h"
#include "speedZone.h"
#include "PickupItem.h"    // For PickupItem def
#include "game.h"

namespace Zap
{

void EditorAttributeMenuUI::onEscape()
{
   doneEditingAttrs(mObject);
   getUIManager()->reactivatePrevUI();     // Back to the editor!
}


static const S32 ATTR_TEXTSIZE = 10;       // called attrSize in the editor

void EditorAttributeMenuUI::render()
{
   // Draw the underlying editor screen
   getUIManager()->getPrevUI()->render();

 /*  if(mRenderInstructions)
      renderMenuInstructions();*/

   EditorUserInterface *ui = getUIManager()->getEditorUserInterface();
   
   S32 gap = 3;
   Point offset = ui->getCurrentOffset();
   Point center = (mObject->getVert(0) + mObject->getVert(1)) * ui->getCurrentScale() / 2 + offset;

   S32 count = menuItems.size();
   S32 yStart = S32(center.y) - count * (ATTR_TEXTSIZE + gap) - 10;

   for(S32 i = 0; i < count; i++)
   {
      S32 y = yStart + i * (ATTR_TEXTSIZE + gap);
      menuItems[i]->render((S32)center.x, y, ATTR_TEXTSIZE, selectedIndex == i);
   }
}


// Sets some standard menu colors.  Individual items can have different colors than these, but only if there is a very good reason.
void EditorAttributeMenuUI::setStandardMenuColors(MenuItem *menuItem)
{
   menuItem->setSelectedColor(Colors::white);
   menuItem->setUnselectedColor(Colors::gray50);
   menuItem->setUnselectedValueColor(Colors::gray50);
}


void EditorAttributeMenuUI::startEditingAttrs(EditorObject *object) 
{ 
   mObject = object; 
   object->startEditingAttrs(this);
}


void EditorAttributeMenuUI::doneEditingAttrs(EditorObject *object) 
{
   // Has to be object, not mObject... this gets run once for every selected item of same type as mObject, and we need to make
   // sure that those objects (passed in as object), get updated.
   object->doneEditingAttrs(this);     

   // Only run on object that is the subject of this editor.  See TextItemEditorAttributeMenuUI::doneEditingAttrs() for explanation
   // of why this may be run on objects that are not actually the ones being edited (hence the need for passing an object in).
   if(object == mObject)   
   {
      mObject->setIsBeingEdited(false);
      getUIManager()->getEditorUserInterface()->doneEditingAttributes(this, mObject); 
   }
}


};
