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
#include "ScreenInfo.h"    // For canvasHeight
#include "ClientGame.h"    // For UIManager and callback
#include "game.h"

namespace Zap
{

extern ScreenInfo gScreenInfo;

static const S32 ATTR_TEXTSIZE = 20;       // called attrSize in the editor

void QuickMenuUI::render()
{
   // Draw the underlying editor screen
   getUIManager()->getPrevUI()->render();

   const S32 INSTRUCTION_SIZE = ATTR_TEXTSIZE * 6 / 10;      // Size of bottom menu item, "Save and quit"

   string title = getTitle();

   S32 gap = ATTR_TEXTSIZE / 3;

   S32 count = getMenuItemCount() - 1;      // We won't count our last item, save-n-quit, here, because it's rendered separately
   S32 yStart = S32(mMenuLocation.y) - count * (ATTR_TEXTSIZE + gap) - 10 - (gap + INSTRUCTION_SIZE);

   S32 width = max(getMenuWidth(), getStringWidth(INSTRUCTION_SIZE, title.c_str()));

   S32 hpad = 8;
   S32 vpad = 4;

   S32 naturalLeft = S32(mMenuLocation.x) - width / 2 - hpad;
   S32 naturalRight = S32(mMenuLocation.x) + width / 2 + hpad;

   S32 naturalTop =  yStart - vpad;
   S32 naturalBottom = yStart + count * (ATTR_TEXTSIZE + gap) + 2 * (gap + INSTRUCTION_SIZE) + vpad * 2 + 2;

   // Keep the menu on screen, no matter where the item being edited is located
   S32 keepingItOnScreenAdjFactorX = 0;
   S32 keepingItOnScreenAdjFactorY = 0;
   
   if(naturalLeft < 0)
       keepingItOnScreenAdjFactorX = -1 * naturalLeft;
   else if(naturalRight > gScreenInfo.getGameCanvasWidth())
      keepingItOnScreenAdjFactorX = gScreenInfo.getGameCanvasWidth() - naturalRight;

   if(naturalTop < 0)
      keepingItOnScreenAdjFactorY = -1 * naturalTop;
   else if(naturalBottom > gScreenInfo.getGameCanvasHeight())
      keepingItOnScreenAdjFactorY = gScreenInfo.getGameCanvasHeight() - naturalBottom;


   yStart += keepingItOnScreenAdjFactorY;
   S32 cenX = S32(mMenuLocation.x) + keepingItOnScreenAdjFactorX;

   S32 left = naturalLeft  + keepingItOnScreenAdjFactorX;
   S32 right = naturalRight + keepingItOnScreenAdjFactorX;

   // Background rectangle
   drawFilledRect(left,  naturalTop    + keepingItOnScreenAdjFactorY, 
                  right, naturalBottom + keepingItOnScreenAdjFactorY, 
                  Color(.1), Color(.5));

   // Now that the background has been drawn, adjust left and right to create the inset for the menu item highlights
   left += 3;
   right -= 4;

   // First draw the menu title
   glColor(Colors::red);
   drawCenteredString(cenX, yStart, INSTRUCTION_SIZE, title.c_str());

   // Then the menu items
   yStart += INSTRUCTION_SIZE + gap + 2;

   for(S32 i = 0; i < count; i++)
   {
      S32 y = yStart + i * (ATTR_TEXTSIZE + gap);

      // Draw background highlight if this item's selected
      if(selectedIndex == i)
         drawMenuItemHighlight(left,  y, right, y + ATTR_TEXTSIZE + 5);

      getMenuItem(i)->render(cenX, y, ATTR_TEXTSIZE, selectedIndex == i);
   }

   /////
   // The last menu item is our save and exit item, which we want to draw smaller to balance out the menu title visually.
   // We'll blindly assume it's there, and also that it's last.
   S32 y = (yStart + count * (ATTR_TEXTSIZE + gap) + gap);

   // Draw background highlight if this item's selected
   if(selectedIndex == getMenuItemCount() - 1)
      drawMenuItemHighlight(left,  y - 1, right, y + INSTRUCTION_SIZE + 3);

   getLastMenuItem()->render(cenX, y, INSTRUCTION_SIZE, selectedIndex == getMenuItemCount() - 1);
}


S32 QuickMenuUI::getMenuWidth()
{
   S32 width = 0;

   for(S32 i = 0; i < getMenuItemCount(); i++)
   {
      S32 itemWidth = getMenuItem(i)->getWidth(ATTR_TEXTSIZE);

      if(itemWidth > width)
         width = itemWidth;
   }

   return width;
}


// Escape cancels without saving
void QuickMenuUI::onEscape()
{
   getUIManager()->reactivatePrevUI();     // Back to the editor!
}


// See Raptor, I am trying!
void QuickMenuUI::setMenuLocation(const Point &location)
{
   mMenuLocation = location;
}


static void saveAndQuit(ClientGame *game, U32 unused)
{
   QuickMenuUI *ui = dynamic_cast<QuickMenuUI *>(UserInterface::current);
   TNLAssert(ui, "Unexpcted UI here -- expected a QuickMenuUI or child class!");

   ui->doneEditing();
   ui->getUIManager()->reactivatePrevUI();
}


// This was cached, but refactor made that difficult, and hell, these are cheap to create!
void QuickMenuUI::addSaveAndQuitMenuItem()
{
   addMenuItem(new MenuItem(99, "Save and quit", saveAndQuit, "Saves and quits"));
}


////////////////////////////////////////
////////////////////////////////////////

// Escape cancels without saving
void EditorAttributeMenuUI::onEscape()
{
   mObject->setIsBeingEdited(false);
   Parent::onEscape();     // Back to the editor!
}


string EditorAttributeMenuUI::getTitle()
{
   EditorUserInterface *ui = getUIManager()->getEditorUserInterface();

   S32 selectedObjCount = ui->getItemSelectedCount();
   return string("Attributes for ") + (selectedObjCount != 1 ? itos(selectedObjCount) + " " + mObject->getPrettyNamePlural() : 
                                                                       mObject->getOnScreenName());
}


void EditorAttributeMenuUI::startEditingAttrs(EditorObject *object) 
{ 
   mObject = object; 

   EditorUserInterface *ui = getUIManager()->getEditorUserInterface();

   Point center = (mObject->getVert(0) + mObject->getVert(1)) * ui->getCurrentScale() / 2 + ui->getCurrentOffset();
   setMenuLocation(center);  

   object->startEditingAttrs(this);
}


void EditorAttributeMenuUI::doneEditing() 
{
   doneEditingAttrs(mObject);
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


////////////////////////////////////////
////////////////////////////////////////


void PluginMenuUI::doneEditing()
{
   Vector<string> responses;

   getMenuResponses(responses);     // Fills responses
   getGame()->getUIManager()->getEditorUserInterface()->onPluginMenuClosed(responses);
}


};
