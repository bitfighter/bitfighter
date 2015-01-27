//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_EDITOR_MENU_H_
#define _UI_EDITOR_MENU_H_


#include "UIMenus.h"


namespace Zap
{


class EditorMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

private:
   void setupMenus();
   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();
   void addStandardQuitItem();

protected:
   void onActivate();

public:
   explicit EditorMenuUserInterface(ClientGame *game, UIManager *uiManager);    // Constructor
   virtual ~EditorMenuUserInterface();

   void unlockQuit();
};


};

#endif
