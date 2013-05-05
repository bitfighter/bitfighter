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

#ifndef _HELPERMENU_H_
#define _HELPERMENU_H_

#include "SlideOutWidget.h"    // Parent class
#include "AToBScroller.h"      // Parent class, for internal transitions

#include "InputModeEnum.h"

using namespace TNL;
using namespace Zap::UI;

namespace Zap
{

#include "InputCodeEnum.h"

class ClientGame;
class UIManager;
class HelperManager;


struct OverlayMenuItem
{
   InputCode key;             // Keyboard key used to select in menu
   InputCode button;          // Controller button used to select in menu
   bool showOnMenu;           // Should this item actually be added to the menu?
   U32 itemIndex;             // Only used on Loadout menu
   const char *name;          // Name used on menu
   const Color *itemColor;
   const char *help;          // An additional bit of help text, also displayed on menu
   const Color *helpColor;    // Pointer to color with which to render the item
};


class HelperMenu : public SlideOutWidget, public AToBScroller
{
   typedef SlideOutWidget Slider;
   typedef AToBScroller   Scroller;
   
public:
   enum HelperMenuType {
      ChatHelperType,
      QuickChatHelperType,
      LoadoutHelperType,
      EngineerHelperType,
      ShuffleTeamsHelperType
   };

private:
   virtual const char *getCancelMessage() const;
   virtual InputCode getActivationKey();

   ClientGame *mClientGame;
   HelperManager *mHelperManager;

   S32 mOldBottom;
   S32 mOldCount;

   // Some render helpers
   S32 drawMenuItems(bool draw, const OverlayMenuItem *items, S32 count, S32 yPos, S32 bottom, bool newItems, bool renderKeysWithItemColor);
   void renderPressEscapeToCancel(S32 xPos, S32 yPos, const Color &baseColor, InputMode inputMode);
   void renderLegend(S32 xPos, S32 yPos, const char **legendtext, const Color **legendColors, S32 legendCount);


protected:
   static const S32 MENU_TOP = 175;    // Location of top of title of overlay menu frame

   // Shortcut helper function
   virtual void exitHelper();

   void drawItemMenu(const char *title, const OverlayMenuItem *items, S32 count, const OverlayMenuItem *prevItems, S32 prevCount,
                     const char **legendText = NULL, const Color **legendColors = NULL, S32 legendCount = 0);

   ClientGame *getGame() const;

   static const S32 MENU_FONT_SIZE        = 15;    // Size of standard items
   static const S32 MENU_FONT_SPACING     =  7;    // Vertical gap between lines
   static const S32 MENU_LEGEND_FONT_SIZE = 11;    // Smaller font of lengend items on QuickChat menus
   static const S32 ITEM_HELP_PADDING     =  5;    // Gap between item and its help text
   static const S32 ITEM_INDENT           = 50;    // Amount individual menu items are indented to make room for keys
   static const S32 MENU_PADDING          =  9;    // Padding around outer edge of overlay
   static const S32 TITLE_FONT_SIZE       = 20;    // Size of title of menu

   S32 mItemWidth;    // Calculated width of menu items

public:
   void initialize(ClientGame *game, HelperManager *helperManager);

   virtual void render() = 0;
   virtual void idle(U32 delta);
   virtual void onActivated();

   virtual void onWidgetClosed();                     // Gets run when closing animation is complete

   virtual bool processInputCode(InputCode inputCode);  
   virtual void onTextInput(char ascii);

   virtual void activateHelp(UIManager *uiManager);   // Open help to an appropriate page
   virtual bool isMovementDisabled() const;           // Is ship movement disabled while this helper is active?
   virtual bool isChatDisabled() const;               // Returns true if chat and friends should be disabled while this is active

   S32 getMaxItemWidth(const OverlayMenuItem *items, S32 count);

   virtual HelperMenuType getType() = 0;
};


};

#endif


