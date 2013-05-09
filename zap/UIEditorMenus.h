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

   void onDisplayModeChange();
};


////////////////////////////////////////
////////////////////////////////////////

class EditorAttributeMenuUI : public QuickMenuUI
{
   typedef QuickMenuUI Parent;
      
private:
   string getTitle();

protected:
   BfObject *mObject;         // Object whose attributes are being edited

public:
   explicit EditorAttributeMenuUI(ClientGame *game);    // Constructor
   virtual ~EditorAttributeMenuUI();                    // Destructor

   BfObject *getObject();

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



}  // namespace

#endif
