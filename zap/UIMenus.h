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

#ifndef _UIMENUS_H_
#define _UIMENUS_H_

#include "UI.h"                  // Parent class
#include "SharedConstants.h"     // For MOTD_LEN
#include "UIMenuItems.h"
#include "config.h"

#include "tnlNetConnection.h"    // for TerminationReason

#include "boost/shared_ptr.hpp"


namespace Zap
{

using namespace std;


// This class is the template for most all of our menus...
class MenuUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   Vector<boost::shared_ptr<MenuItem> > mMenuItems;

   S32 checkMenuIndexBounds(S32 index);   // Returns corrected index
   Timer mScrollTimer;

   // For detecting keys being held down
   bool mRepeatMode;
   bool mKeyDown;

   virtual void renderExtras();     // For drawing something extra on a menu
   void advanceItem();              // What happens when we move on to the next menu item?

   virtual void initialize();


protected:
   S32 getOffset();                             // Calculates index of topmost visible item         
   virtual S32 getYStart();                     // Get vert pos of first menu item
   virtual S32 getTextSize(MenuItemSize size);  // Let menus set their own text size
   virtual S32 getGap(MenuItemSize size);       // Gap is the space between items

   S32 mMaxMenuSize;
   S32 mFirstVisibleItem;  // Some menus have items than will fit on the screen; this is the index of the first visible item

   bool mRenderInstructions;
   bool mRenderSpecialInstructions;
   bool mIgnoreNextMouseEvent;

   // Handle keyboard input while a menu is displayed
   virtual bool processMenuSpecificKeys(InputCode inputCode);
   virtual bool processKeys(InputCode inputCode);
   virtual S32 getSelectedMenuItem();

   S32 getTotalMenuItemHeight();    // Add up height of all menu items

   void sortMenuItems();
   MenuItem *getLastMenuItem();
   S32 getMaxFirstItemIndex();      // Calculates maximum index that the first item can have -- on non scrolling menus, this will be 0

public:
   // Constructor
   explicit MenuUserInterface(ClientGame *game);
   MenuUserInterface(ClientGame *game, const string &title);
   virtual ~MenuUserInterface();

   bool isScrollingMenu();

   void clearMenuItems();
   void addMenuItem(MenuItem *menuItem);
   void addWrappedMenuItem(boost::shared_ptr<MenuItem> menuItem);
   MenuItem *getMenuItem(S32 index);
   S32 getMenuItemCount();

   bool itemSelectedWithMouse;

   string mMenuTitle;
   string mMenuSubTitle;

   Color mMenuSubTitleColor;
   bool mMenuFooterContainsInstructions;

   void idle(U32 timeDelta); 

   S32 selectedIndex;                                    // Index of the currently highlighted menu item

   void getMenuResponses(Vector<string> &responses);     // Fill responses with values from menu

   void render();                                        // Draw the basic menu
   bool onKeyDown(InputCode inputCode);
   void onKeyUp(InputCode inputCode);
   void onTextInput(char ascii);
   void onMouseMoved();
   void processMouse();

   virtual void onActivate();
   virtual void onReactivate();
   virtual void onEscape();
};


class MenuUserInterfaceWithIntroductoryAnimation : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

private:
   static const S32 FadeInTime = 400;       // Post animation fade in time (ms) 

   static bool mFirstTime;    // Is this the first time an intro menu is being shown?
   Timer mFadeInTimer;        // Track the brief fade in interval the first time menu is shown
   bool mShowingAnimation;    // Is intro animation currently being played?

public:
   explicit MenuUserInterfaceWithIntroductoryAnimation(ClientGame *game);
   virtual ~MenuUserInterfaceWithIntroductoryAnimation();

   void onActivate();
   void idle(U32 timeDelta);
   void render();
   bool onKeyDown(InputCode inputCode);
   void processSelection(U32 index);
};


////////////////////////////////////////
////////////////////////////////////////

// <--- DO NOT SUBCLASS MainMenuUserInterface!! (unless you override onActivate) ---> //
class MainMenuUserInterface : public MenuUserInterfaceWithIntroductoryAnimation
{
   typedef MenuUserInterfaceWithIntroductoryAnimation Parent;

private:
   char mMOTD[MOTD_LEN];
   U32 motdArriveTime;
   Timer mColorTimer;
   Timer mColorTimer2;
   enum {
      ColorTime = 1000,
      ColorTime2 = 1700,
   };
   bool mTransDir;
   bool mTransDir2;
   bool mNeedToUpgrade;       // True if client is out of date and needs to upgrade, false if we're on the latest version
   bool mShowedUpgradeAlert;  // So we don't show the upgrade message more than once

   void renderExtras();

public:
   explicit MainMenuUserInterface(ClientGame *game);           // Constructor
   virtual ~MainMenuUserInterface();

   void onEscape();
   void render();
   void idle(U32 timeDelta); 
   void setMOTD(const char *motd);              // Message of the day, from Master
   void onActivate();
   void setNeedToUpgrade(bool needToUpgrade);   // Is client in need of an upgrade?

   void showUpgradeAlert();                     // Display message to the user that it is time to upgrade
   bool getNeedToUpgrade();
};


////////////////////////////////////////
////////////////////////////////////////

class OptionsMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit OptionsMenuUserInterface(ClientGame *game);        // Constructor
   virtual ~OptionsMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
   void toggleDisplayMode();
};


////////////////////////////////////////
////////////////////////////////////////

class InputOptionsMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit InputOptionsMenuUserInterface(ClientGame *game);        // Constructor
   virtual ~InputOptionsMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
};


////////////////////////////////////////
////////////////////////////////////////

class SoundOptionsMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit SoundOptionsMenuUserInterface(ClientGame *game);        // Constructor
   virtual ~SoundOptionsMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
};

////////////////////////////////////////
////////////////////////////////////////


class HostMenuUserInterface : public MenuUserInterface
{
   enum MenuItems {
      OPT_HOST,
      OPT_NAME,
      OPT_DESCR,
      OPT_LVL_PASS,
      OPT_ADMIN_PASS,
      OPT_PASS,
      OPT_GETMAP,
      OPT_MAX_PLAYERS,
      OPT_PORT,
      OPT_COUNT
   };

   static const S32 FIRST_EDITABLE_ITEM = OPT_NAME;

   typedef MenuUserInterface Parent;

private:
   Vector<string> mLevelLoadDisplayNames;    // For displaying levels as they're loaded in host mode
   S32 mLevelLoadDisplayTotal;
   S32 mEditingIndex;                        // Index of item we're editing, -1 if none

public:
   explicit HostMenuUserInterface(ClientGame *game);        // Constructor
   virtual ~HostMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
   void render();
   void saveSettings();       // Save parameters in INI file

   // For displaying the list of levels as we load them:
   Timer levelLoadDisplayFadeTimer;
   bool levelLoadDisplayDisplay;
   void addProgressListItem(string item);
   void renderProgressListItems();
   void clearLevelLoadDisplay();
};

////////////////////////////////////////
////////////////////////////////////////

// Player enters their name and password
class NameEntryUserInterface : public MenuUserInterfaceWithIntroductoryAnimation
{
   typedef MenuUserInterfaceWithIntroductoryAnimation Parent;

private:
   void renderExtras();
   NetConnection::TerminationReason mReason;

public:
   explicit NameEntryUserInterface(ClientGame *game);    // Constructor
   virtual ~NameEntryUserInterface();

   void onEscape();
   void setupMenu();
   void onActivate();
   void setReactivationReason(NetConnection::TerminationReason r);
};


////////////////////////////////////////
////////////////////////////////////////

class GameType;

class GameMenuUserInterface : public MenuUserInterface
{
   enum {
      OPT_OPTIONS,
      OPT_HELP,
      OPT_ADMIN,
      OPT_KICK,
      OPT_LEVEL_CHANGE_PW,
      OPT_CHOOSELEVEL,
      OPT_ADD2MINS,
      OPT_RESTART,
      OPT_QUIT
   };

private:
   typedef MenuUserInterface Parent;
   SafePtr<GameType> mGameType;
   InputMode lastInputMode;
   void buildMenu();

public:
   explicit GameMenuUserInterface(ClientGame *game);            // Constructor
   virtual ~GameMenuUserInterface();

   void idle(U32 timeDelta);
   void onActivate();
   void onReactivate();
   void onEscape();
};


////////////////////////////////////////
////////////////////////////////////////

class LevelMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;

public:
   explicit LevelMenuUserInterface(ClientGame *game);        // Constructor
   virtual ~LevelMenuUserInterface();

   void onActivate();
   void onEscape();
};


////////////////////////////////////////
////////////////////////////////////////

class LevelMenuSelectUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

private:
   Vector<string> mLevels;

public:
   explicit LevelMenuSelectUserInterface(ClientGame *game);        // Constructor
   virtual ~LevelMenuSelectUserInterface();

   string category;
   void onActivate();
   bool processMenuSpecificKeys(InputCode inputCode);  // Custom key handling for level selection menus

   void processSelection(U32 index);
   void onEscape();
};


////////////////////////////////////////
////////////////////////////////////////

class AdminMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

private:
   SafePtr<GameType> mGameType;

public:
   AdminMenuUserInterface();      // Constructor
   virtual ~AdminMenuUserInterface();

   void onActivate();
   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();
};


////////////////////////////////////////
////////////////////////////////////////

class PlayerMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit PlayerMenuUserInterface(ClientGame *game);        // Constructor
   virtual ~PlayerMenuUserInterface();

   void render();
   void playerSelected(U32 index);
   void onEscape();

   enum Action {
      Kick,
      ChangeTeam,
      ActionCount,
   } action;
};

////////////////////////////////////////
////////////////////////////////////////

class TeamMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit TeamMenuUserInterface(ClientGame *game);        // Constructor
   virtual ~TeamMenuUserInterface();

   void render();
   void onEscape();
   string nameToChange;

   void processSelection(U32 index);
};

};


#endif      // #ifndef _UIMENUS_H_

