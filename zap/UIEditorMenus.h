//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UIEDITORMENUS_H_
#define _UIEDITORMENUS_H_


#include "UIMenus.h"    // Parent class

namespace Zap
{

// This class is now a container for various attribute editing menus; these are rendered differently than regular menus, and
// have other special attributes.  This class has been refactored such that it can be used directly, and no longer needs to be
// subclassed for each type of entity we want to edit attributes for.

class QuickMenuUI : public MenuUserInterface    // There's really nothing quick about it!
{
   typedef MenuUserInterface Parent;

private:
   virtual void initialize();
   virtual string getTitle();
   S32 getMenuWidth();     
   Point mMenuLocation;

   S32 getYStart();

   virtual S32 getTextSize(MenuItemSize size);     // Let menus set their own text size
   virtual S32 getGap(MenuItemSize size);          // Gap is the space between items

   // Calculated during rendering, used for figuring out which item mouse is over.  Will always be positive during normal use, 
   // but will be intialized to negative so that we know not to use it before menu has been rendered, and this value caluclated.
   S32 mTopOfFirstMenuItem;       

protected:
   bool mDisableHighlight;   // Disable highlighting of selected menu item

   virtual S32 getSelectedMenuItem();
   bool usesEditorScreenMode();

public:
   // Constructors
   explicit QuickMenuUI(ClientGame *game);
   QuickMenuUI(ClientGame *game, const string &title);
   virtual ~QuickMenuUI();

   void render();

   virtual void onEscape();

   void addSaveAndQuitMenuItem();
   void addSaveAndQuitMenuItem(const char *menuText, const char *helpText);
   void setMenuCenterPoint(const Point &location);    // Sets the point at which the menu will be centered about
   virtual void doneEditing() = 0;

   void cleanupAndQuit();                             // Delete our menu items and reactivate the underlying UI

   void onDisplayModeChange();
};


////////////////////////////////////////
////////////////////////////////////////

class EditorAttributeMenuUI : public QuickMenuUI
{
   typedef QuickMenuUI Parent;
      
private:
   string getTitle();

public:
   explicit EditorAttributeMenuUI(ClientGame *game);    // Constructor
   virtual ~EditorAttributeMenuUI();                    // Destructor

   virtual void startEditingAttrs(BfObject *object);
   virtual void doneEditing();
   virtual void doneEditingAttrs(BfObject *object);
};


////////////////////////////////////////
////////////////////////////////////////

class PluginMenuUI : public QuickMenuUI
{
   typedef QuickMenuUI Parent;

public:
   PluginMenuUI(ClientGame *game, const string &title);    // Constructor
   virtual ~PluginMenuUI();                                // Destructor

   void setTitle(const string &title);
   virtual void doneEditing();
};


////////////////////////////////////////
////////////////////////////////////////

class SimpleTextEntryMenuUI : public QuickMenuUI
{
   typedef QuickMenuUI Parent;

private:
   S32 mData;          // See SimpleTextEntryType in UIEditor.h

public:
   SimpleTextEntryMenuUI(ClientGame *game, const string &title, S32 data);    // Constructor
   virtual ~SimpleTextEntryMenuUI();                                          // Destructor

   virtual void doneEditing();
};



}  // namespace

#endif
