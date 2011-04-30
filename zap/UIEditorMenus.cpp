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

namespace Zap
{


void EditorAttributeMenuUI::onEscape()
{
   reactivatePrevUI();     // Back to the editor!
}


static const S32 ATTR_TEXTSIZE = 10;                                          // called attrSize in the editor

void EditorAttributeMenuUI::render()
{
   // Draw the editor screen
   gEditorUserInterface.render();     // better way than global?

 /*  if(mRenderInstructions)
      renderMenuInstructions();*/
   
   S32 gap = 3;
   Point offset = gEditorUserInterface.getCurrentOffset();
   Point center = (mObject->getVert(0) + mObject->getVert(1)) * gEditorUserInterface.getCurrentScale() / 2 + offset;

   S32 count = menuItems.size();
   S32 yStart = center.y - count * (ATTR_TEXTSIZE + gap) - 10;

   for(S32 i = 0; i < count; i++)
   {
      S32 y = yStart + i * (ATTR_TEXTSIZE + gap);
      menuItems[i]->render(center.x, y, ATTR_TEXTSIZE, selectedIndex == i);
   }
}


////////////////////////////////////////
////////////////////////////////////////
   
static void setMenuColors(MenuItem *menuItem)
{
   menuItem->setSelectedColor(white);
   menuItem->setUnselectedColor(gray50);
   menuItem->setUnselectedValueColor(gray50);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
GoFastEditorAttributeMenuUI::GoFastEditorAttributeMenuUI()
{
   setMenuID(GoFastAttributeEditorUI);

   menuItems.push_back(new CounterMenuItem("Speed", 100, 5, 1, 2000, "", "Really slow", ""));
   setMenuColors(menuItems.last());

   menuItems.push_back(new YesNoMenuItem("Snapping", true, NULL, ""));
   setMenuColors(menuItems.last());
}


////////////////////////////////////////
////////////////////////////////////////

extern const S32 MAX_TEXTITEM_LEN;

static void textEditedCallback(string text)
{
   TextItem *textItem = dynamic_cast<TextItem *>(TextItem::getAttributeEditorObject());
   textItem->setText(text);
   textItem->recalcTextSize();
}


// Constructor
TextItemEditorAttributeMenuUI::TextItemEditorAttributeMenuUI()
{
   setMenuID(TextItemAttributeEditorUI);

   EditableMenuItem *menuItem = new EditableMenuItem("Text: ", "Blah", "", "", MAX_TEXTITEM_LEN);

   menuItem->setTextEditedCallback(textEditedCallback);
   setMenuColors(menuItem);
   menuItems.push_back(menuItem);
}


};