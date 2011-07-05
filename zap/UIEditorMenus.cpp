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

namespace Zap
{


void EditorAttributeMenuUI::onEscape()
{
   doneEditing(mObject);
   reactivatePrevUI();     // Back to the editor!
}


static const S32 ATTR_TEXTSIZE = 10;                                          // called attrSize in the editor

void EditorAttributeMenuUI::render()
{
   // Draw the underlying editor screen
   gEditorUserInterface.render();     // better way than global?

 /*  if(mRenderInstructions)
      renderMenuInstructions();*/
   
   S32 gap = 3;
   Point offset = gEditorUserInterface.getCurrentOffset();
   Point center = (mObject->getVert(0) + mObject->getVert(1)) * gEditorUserInterface.getCurrentScale() / 2 + offset;

   S32 count = menuItems.size();
   S32 yStart = S32(center.y) - count * (ATTR_TEXTSIZE + gap) - 10;

   for(S32 i = 0; i < count; i++)
   {
      S32 y = yStart + i * (ATTR_TEXTSIZE + gap);
      menuItems[i]->render(center.x, y, ATTR_TEXTSIZE, selectedIndex == i);
   }
}


void EditorAttributeMenuUI::doneEditing(EditorObject *object) 
{ 
   // Only run on object that is the subject of this editor.  See TextItemEditorAttributeMenuUI::doneEditing() for explanation
   // of why this may be run on objects that are not actually the ones being edited (hence the need for passing an object in).
   if(object == mObject)   
   {
      mObject->setIsBeingEdited(false);
      gEditorUserInterface.doneEditingAttributes(this, mObject); 
   }
}


////////////////////////////////////////
////////////////////////////////////////
   
static void setMenuColors(MenuItem *menuItem)
{
   menuItem->setSelectedColor(Colors::white);
   menuItem->setUnselectedColor(Colors::gray50);
   menuItem->setUnselectedValueColor(Colors::gray50);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
GoFastEditorAttributeMenuUI::GoFastEditorAttributeMenuUI()
{
   setMenuID(GoFastAttributeEditorUI);
   menuItems.resize(2);

   menuItems[0] = boost::shared_ptr<MenuItem>(new CounterMenuItem("Speed", 100, 100, SpeedZone::minSpeed, SpeedZone::maxSpeed, "", "Really slow", ""));
   setMenuColors(menuItems[0].get());

   menuItems[1] = boost::shared_ptr<MenuItem>(new YesNoMenuItem("Snapping", true, NULL, ""));
   setMenuColors(menuItems[1].get());
}


void GoFastEditorAttributeMenuUI::startEditing(EditorObject *object)
{
   Parent::startEditing(object);

   SpeedZone *goFast = dynamic_cast<SpeedZone *>(object);

   // Now transfer some attributes
   menuItems[0]->setIntValue(goFast->getSpeed());
   menuItems[1]->setValue(goFast->getSnapping() ? "yes" : "no");
}


void GoFastEditorAttributeMenuUI::doneEditing(EditorObject *object)
{
   SpeedZone *goFast = dynamic_cast<SpeedZone *>(object);

   goFast->setSpeed(menuItems[0]->getIntValue());
   goFast->setSnapping(menuItems[1]->getIntValue() != 0);

   Parent::doneEditing(object);
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

   menuItems.resize(1);

   EditableMenuItem *menuItem = new EditableMenuItem("Text: ", "Blah", "", "", MAX_TEXTITEM_LEN);

   menuItem->setTextEditedCallback(textEditedCallback);
   setMenuColors(menuItem);

   menuItems[0] = boost::shared_ptr<MenuItem>(menuItem);
}


void TextItemEditorAttributeMenuUI::startEditing(EditorObject *object)
{
   Parent::startEditing(object);

   TextItem *textItem = dynamic_cast<TextItem *>(object);

   // Now transfer some attributes
   menuItems[0]->setValue(textItem->getText());
}


void TextItemEditorAttributeMenuUI::doneEditing(EditorObject *object)
{
   Parent::doneEditing(object);

   // The following seems redundant because we have a callback that keeps the item's setText method updated throughout the editing process.
   // However, this is also called if we edit three textItems at once and we need to transfer the text to the other two.
   TextItem *textItem = dynamic_cast<TextItem *>(object);
   textItem->setText(menuItems[0]->getValue());
}

};
