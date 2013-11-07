//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_KEY_DEF_MENUS_H_
#define _UI_KEY_DEF_MENUS_H_


#include "UI.h"               // Parent

#include "Color.h"
#include "Timer.h"

#include "tnlTypes.h"

#include <string>

namespace Zap
{

using namespace std;

////////////////////////////////////////
////////////////////////////////////////

struct KeyDefMenuItem
{
   const char *text;
   U32 column;
   InputCodeManager::BindingNameEnum primaryControl;
   string helpString;

   KeyDefMenuItem(const char *text, U32 col, InputCodeManager::BindingNameEnum PC, string helpStr);     // Constructor
   virtual ~KeyDefMenuItem();
};


////////////////////////////////////////
////////////////////////////////////////

class KeyDefMenuUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   Timer errorMsgTimer;
   string errorMsg;

   Vector<KeyDefMenuItem> menuItems;      // Array of menu items
   S32 maxMenuItemsInAnyCol;

   const char *mMenuTitle;
   const char *mMenuSubTitle;
   Color mMenuSubTitleColor;

   const char *mMenuFooter;

   S32 selectedIndex;          // Highlighted menu item
   S32 changingItem;           // Index of key we're changing (in keyDef mode), -1 otherwise

   bool isDuplicate(S32 key, const Vector<KeyDefMenuItem> &menuItems);

public:
   explicit KeyDefMenuUserInterface(ClientGame *game);   // Constructor
   virtual ~KeyDefMenuUserInterface();

   void render();              // Draw the menu
   void idle(U32 timeDelta);
   bool onKeyDown(InputCode inputCode);

   void onMouseMoved();

   void onActivate();
};

};

#endif

