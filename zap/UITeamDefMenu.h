//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UITEAMDEFMENU_H_
#define _UITEAMDEFMENU_H_

#include "UI.h"
#include "InputModeEnum.h"
#include "Color.h"
#include "Timer.h"
#include "SymbolShape.h"

namespace Zap
{

using namespace std;


class TeamDefUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   Timer errorMsgTimer;
   string errorMsg;

   UI::SymbolStringSet mMenuSubTitle;
   UI::SymbolStringSet mBottomInstructions;
   
   S32 selectedIndex;          // Highlighted menu item
   S32 changingItem;           // Index of key we're changing (in keyDef mode), -1 otherwise

   bool mEditing;              // true if editing selectedIndex, false if not

public:
   explicit TeamDefUserInterface(ClientGame *game);     // Constructor
   virtual ~TeamDefUserInterface();

   const char *mMenuTitle;
   const char *mMenuFooter;

   void render();              // Draw the menu
   void idle(U32 timeDelta);
   bool onKeyDown(InputCode inputCode);
   void onTextInput(char ascii);
   void onMouseMoved();

   void onActivate();
   void onEscape();
   void onColorPicked(const Color &color);
};

};

#endif

