//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIEditorMenus.h"

#include "UIEditor.h"
#include "UIManager.h"
#include "DisplayManager.h"    // For canvasHeight
#include "ClientGame.h"    // For UIManager and callback

#include "Colors.h"

#include "stringUtils.h"
#include "RenderUtils.h"
#include "OpenglUtils.h"

namespace Zap
{

// Constructors
QuickMenuUI::QuickMenuUI(ClientGame *game) : Parent(game)
{
   initialize();
}


QuickMenuUI::QuickMenuUI(ClientGame *game, const string &title) : Parent(game, title)
{
   initialize();
}

// Destructor
QuickMenuUI::~QuickMenuUI()
{
   // Do nothing
}

void QuickMenuUI::initialize()
{
   mTopOfFirstMenuItem = -1;    
   mDisableHighlight = false;
}


string QuickMenuUI::getTitle()
{
   return mMenuTitle;
}


S32 QuickMenuUI::getTextSize(MenuItemSize size)
{
   return size == MENU_ITEM_SIZE_NORMAL ? 20 : 12;
}


S32 QuickMenuUI::getGap(MenuItemSize size)
{
   return 6;
}


void QuickMenuUI::render()
{
   // Draw the underlying editor screen
   getUIManager()->getPrevUI()->render();

   // mMenuLocation.x = 1000; mMenuLocation.y = -1000;      // <== For testing menu positioning code

   //TODO: Get rid of the following
   const S32 INSTRUCTION_SIZE = getTextSize(MENU_ITEM_SIZE_SMALL);     // Size of smaller bottom menu item, "Save and quit"

   string title = getTitle();

   //S32 count = getMenuItemCount() - 1;      // We won't count our last item, save-n-quit, because it's rendered separately
   // was S32 yStart = S32(mMenuLocation.y) - count * (getTextSize() + getGap()) - 10 - (getGap() + INSTRUCTION_SIZE);

   S32 yStart = S32(mMenuLocation.y);

   S32 menuHeight = getTotalMenuItemHeight() +                                         // Height of all menu items
                    getTextSize(MENU_ITEM_SIZE_SMALL) + getGap(MENU_ITEM_SIZE_NORMAL); // Height of title and title gam

   yStart = S32(mMenuLocation.y) - menuHeight;

   S32 width = max(getMenuWidth(), getStringWidth(INSTRUCTION_SIZE, title.c_str()));

   S32 hpad = 8;
   S32 vpad = 4;

   S32 naturalLeft = S32(mMenuLocation.x) - width / 2 - hpad;
   S32 naturalRight = S32(mMenuLocation.x) + width / 2 + hpad;

   S32 naturalTop =  yStart - vpad;
   S32 naturalBottom = yStart + menuHeight + vpad * 2 + 2;


   // Keep the menu on screen, no matter where the item being edited is located
   S32 keepingItOnScreenAdjFactorX = 0;
   S32 keepingItOnScreenAdjFactorY = 0;
   
   if(naturalLeft < 0)
       keepingItOnScreenAdjFactorX = -1 * naturalLeft;
   else if(naturalRight > DisplayManager::getScreenInfo()->getGameCanvasWidth())
      keepingItOnScreenAdjFactorX = DisplayManager::getScreenInfo()->getGameCanvasWidth() - naturalRight;

   if(naturalTop < 0)
      keepingItOnScreenAdjFactorY = -1 * naturalTop;
   else if(naturalBottom > DisplayManager::getScreenInfo()->getGameCanvasHeight())
      keepingItOnScreenAdjFactorY = DisplayManager::getScreenInfo()->getGameCanvasHeight() - naturalBottom;


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
   drawCenteredString(cenX, yStart, getTextSize(MENU_ITEM_SIZE_SMALL), title.c_str());

   // Then the menu items
   yStart += getGap(MENU_ITEM_SIZE_NORMAL) + getTextSize(MENU_ITEM_SIZE_SMALL) + 2;
   mTopOfFirstMenuItem = yStart;    // Save this -- it will be handy elsewhere!

   S32 y = yStart;

   for(S32 i = 0; i < getMenuItemCount(); i++)
   {
      MenuItemSize size = getMenuItem(i)->getSize();
      S32 itemTextSize = getTextSize(size);
      S32 itemGap = getGap(size);

      S32 specialCaseFix = 0;
      // Special case: increase gap beween main menu and "Save and Quit"
      if(i == getMenuItemCount() - 1)
      {
         specialCaseFix = 2;
         y += 5;
      }

      // Draw background highlight if this item's selected
      if(selectedIndex == i && !mDisableHighlight)
         drawMenuItemHighlight(left,  y - specialCaseFix, right, y + itemTextSize + 5);

      getMenuItem(i)->render(cenX, y, itemTextSize, selectedIndex == i);

      y += itemTextSize + itemGap;
   }

   /////
   // The last menu item is our save and exit item, which we want to draw smaller to balance out the menu title visually.
   // We'll blindly assume it's there, and also that it's last.
   //y += getGap(MENU_ITEM_SIZE_NORMAL);

   //// Draw background highlight if this item's selected
   //if(selectedIndex == getMenuItemCount() - 1)
   //   drawMenuItemHighlight(left,  y - 1, right, y + INSTRUCTION_SIZE + 3);

   //getLastMenuItem()->render(cenX, y, INSTRUCTION_SIZE, selectedIndex == (getMenuItemCount() - 1));

   /////
   // Render instructions just below the menu
   glColor(Colors::menuHelpColor);

   // Move instruction to top of the menu if there is not room to display it below.  I realize this code is a bit of a mess... not
   // sure how to write it more clearly, though I'm sure it could be done.
   S32 instrXPos, instrYPos;

   // Make sure help fits on the screen left-right-wise; note that this probably breaks down if text is longer than the screen is wide
   instrXPos = cenX;

   S32 HELP_TEXT_SIZE = getTextSize(MENU_ITEM_SIZE_NORMAL);

   // Amount help "sticks out" beyond menu box:
   S32 xoff = (getStringWidth(HELP_TEXT_SIZE, getMenuItem(selectedIndex)->getHelp().c_str()) - width) / 2; 
   if(xoff > 0)
      instrXPos += max(xoff - left, min(DisplayManager::getScreenInfo()->getGameCanvasWidth() - xoff - right, 0));

   // Now consider vertical position
   if(naturalBottom + vpad + HELP_TEXT_SIZE < DisplayManager::getScreenInfo()->getGameCanvasHeight())
      instrYPos = naturalBottom + keepingItOnScreenAdjFactorY + vpad;                           // Help goes below, in normal location
   else
      instrYPos = naturalTop + keepingItOnScreenAdjFactorY - HELP_TEXT_SIZE - getGap(MENU_ITEM_SIZE_NORMAL) - vpad;   // No room below, help goes above

   drawCenteredString(instrXPos, instrYPos, HELP_TEXT_SIZE, getMenuItem(selectedIndex)->getHelp().c_str());

}


S32 QuickMenuUI::getYStart()
{
   return mTopOfFirstMenuItem;
}


S32 QuickMenuUI::getSelectedMenuItem()
{
   if(getYStart() < 0)      // Haven't run render yet, still have no idea where the menu is!
      return 0;

   return Parent::getSelectedMenuItem();
}


bool QuickMenuUI::usesEditorScreenMode() const     // TODO: Rename this stupidly named method!!
{
   return true;
}


void QuickMenuUI::onDisplayModeChange()
{
   getUIManager()->getUI<EditorUserInterface>()->onDisplayModeChange();   // This is intended to run the same method in the editor

   // Reposition menu on screen, keeping same relative position as before
   Point pos(mMenuLocation.x * DisplayManager::getScreenInfo()->getGameCanvasWidth() /DisplayManager::getScreenInfo()->getPrevCanvasWidth(), 
             mMenuLocation.y * DisplayManager::getScreenInfo()->getGameCanvasHeight() / DisplayManager::getScreenInfo()->getPrevCanvasHeight());

   setMenuCenterPoint(pos);
}


S32 QuickMenuUI::getMenuWidth()
{
   S32 width = 0;

   for(S32 i = 0; i < getMenuItemCount(); i++)
   {
      S32 itemWidth = getMenuItem(i)->getWidth(getTextSize(getMenuItem(i)->getSize()));

      if(itemWidth > width)
         width = itemWidth;
   }

   return width;
}


// Escape cancels without saving
void QuickMenuUI::onEscape()
{
   cleanupAndQuit();
}


// Delete our menu items and reactivate the underlying UI
void QuickMenuUI::cleanupAndQuit()
{
   getUIManager()->reactivatePrevUI();    // Back to the editor!
   //clearMenuItems();    // Don't do this now... will be cleaned up eventually, but if we delete now, menu will crash
                          // if we try to reactivate it and it expects to find menu items and there are none
}


void QuickMenuUI::setMenuCenterPoint(const Point &location)
{
   mMenuLocation = location;
}


static void saveAndQuit(ClientGame *game, U32 unused)
{
   TNLAssert(dynamic_cast<QuickMenuUI *>(game->getUIManager()->getCurrentUI()), "Expected a QuickMenuUI here!");
   QuickMenuUI *ui = static_cast<QuickMenuUI *>(game->getUIManager()->getCurrentUI());

   ui->doneEditing();

   ui->cleanupAndQuit();
}


// This was cached, but refactor made that difficult, and hell, these are cheap to create!
void QuickMenuUI::addSaveAndQuitMenuItem()
{
   addSaveAndQuitMenuItem("Save and quit", "Saves and quits");
}


void QuickMenuUI::addSaveAndQuitMenuItem(const char *menuText, const char *helpText)
{
   MenuItem *menuItem = new MenuItem(menuText, saveAndQuit, helpText);
   menuItem->setSize(MENU_ITEM_SIZE_SMALL);

   addMenuItem(menuItem);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
EditorAttributeMenuUI::EditorAttributeMenuUI(ClientGame *game) : Parent(game)
{
   // Do nothing
}


// Destructor
EditorAttributeMenuUI::~EditorAttributeMenuUI()
{
   // Do nothing
}


string EditorAttributeMenuUI::getTitle()
{
   EditorUserInterface *ui = getUIManager()->getUI<EditorUserInterface>();

   S32 selectedObjCount = ui->getItemSelectedCount();
   return string("Attributes for ") + (selectedObjCount != 1 ? itos(selectedObjCount) + " " + mAssociatedObject->getPrettyNamePlural() : 
                                                               mAssociatedObject->getOnScreenName());
}


void EditorAttributeMenuUI::startEditingAttrs(BfObject *object) 
{ 
   mAssociatedObject = object; 

   EditorUserInterface *ui = getUIManager()->getUI<EditorUserInterface>();

   Point center = (mAssociatedObject->getVert(0) + mAssociatedObject->getVert(1)) * ui->getCurrentScale() / 2 + ui->getCurrentOffset();
   setMenuCenterPoint(center);  

   EditorAttributeMenuItemBuilder::startEditingAttrs(this, object);
}


void EditorAttributeMenuUI::doneEditing() 
{
   doneEditingAttrs(mAssociatedObject);
}


void EditorAttributeMenuUI::doneEditingAttrs(BfObject *object) 
{
   // Has to be object, not mAssociatedObject... this gets run once for every selected item of same type as mAssociatedObject, 
   // and we need to make sure that those objects (passed in as object), get updated
   EditorAttributeMenuItemBuilder::doneEditingAttrs(this, object);     

   // Only run on object that is the subject of this editor.  See TextItemEditorAttributeMenuUI::doneEditingAttrs() for explanation
   // of why this may be run on objects that are not actually the ones being edited (hence the need for passing an object in).
   if(object == mAssociatedObject)   
      getUIManager()->getUI<EditorUserInterface>()->doneEditingAttributes(this, mAssociatedObject); 
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
PluginMenuUI::PluginMenuUI(ClientGame *game, const string &title) :
      Parent(game, title)
{
   /* Do nothing */
}


PluginMenuUI::~PluginMenuUI()
{
   // Do nothing
}


void PluginMenuUI::setTitle(const string &title)
{
   mMenuTitle = title;
}


void PluginMenuUI::doneEditing()
{
   Vector<string> responses;
   
   getMenuResponses(responses);              // Fills responses
   responses.erase(responses.size() - 1);    // Remove last arg, which corresponds to our "save and quit" menu item

   getGame()->getUIManager()->getUI<EditorUserInterface>()->onPluginExecuted(responses);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SimpleTextEntryMenuUI::SimpleTextEntryMenuUI(ClientGame *game, const string &title, S32 data) :
      Parent(game, title)
{
   mData = data;

   mDisableHighlight = true;  // No text highlighting

   setMenuCenterPoint(Point(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, DisplayManager::getScreenInfo()->getGameCanvasHeight() / 2));
}


SimpleTextEntryMenuUI::~SimpleTextEntryMenuUI()
{
   // Do nothing
}


void SimpleTextEntryMenuUI::doneEditing()
{
   getGame()->getUIManager()->getUI<EditorUserInterface>()->doneWithSimpleTextEntryMenu(this, mData);
}


};
