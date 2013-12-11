//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UIGAMEPARAMS_H_
#define _UIGAMEPARAMS_H_


#include "UIMenus.h"

#include "tnlTypes.h"

#include <string>
#include <map>

namespace Zap
{

using namespace std;

////////////////////////////////////
////////////////////////////////////

class SavedMenuItem
{
private:
   string mParamName;
   string mParamVal;

public:
   SavedMenuItem();    // Default constructor

   explicit SavedMenuItem(MenuItem *menuItem);
   virtual ~SavedMenuItem();

   void setValues(MenuItem *menuItem);

   string getParamName();
   string getParamVal();
};

////////////////////////////////////
////////////////////////////////////

// By subclassing MenuUserInterface, I hoped to get the mouse stuff to automatically work, but it didn't.  <sigh>
class GameParamUserInterface : public MenuUserInterface     
{
   typedef MenuUserInterface Parent;

private:
   void processSelection(U32 index); // Needed for MenuUserInterface subclassing... does nothing

   bool anythingChanged();           // Compare list of parameters from before and after a session in the GameParams menu.  Did anything get changed??

   string origGameParams;            // Copy of the game parameters as specified when we activated the GameParameters, used to compare before and after to detect changes

   S32 mQuitItemIndex;               // Index of our quit item -- will vary depending on how many game-specific parameters there are
   S32 mGameSpecificParams;          // How many game specific parameters do we have?

   virtual S32 getTextSize(MenuItemSize size) const;
   virtual S32 getGap(MenuItemSize size) const;

   S32 getYStart() const;


   typedef map<const string, boost::shared_ptr<MenuItem> > MenuItemMap;
   MenuItemMap mMenuItemMap;


public:
   explicit GameParamUserInterface(ClientGame *game);   // Constructor
   virtual ~GameParamUserInterface();

   S32 selectedIndex;          // Highlighted menu item
   S32 changingItem;           // Index of key we're changing (in keyDef mode), -1 otherwise

   void updateMenuItems();
   void clearCurrentGameTypeParams();

   void onActivate();
   void onEscape();
};

};

#endif

