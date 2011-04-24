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

#include "UI.h"
#include "input.h"         // For InputMode def
#include "UIMenuItems.h"


namespace Zap
{

using namespace std;


// This class is the template for most all of our menus...
class MenuUserInterface : public UserInterface
{
private:
   S32 getYStart();     // Get vert pos of first menu item
   S32 getOffset(); 
   Timer mScrollTimer;

   // For detecting keys being held down
   bool mRepeatMode;
   bool mKeyDown;

   virtual S32 getTextSize() { return 23; }              // Let menus set their own text size
   virtual S32 getGap() { return 18; }

   virtual void renderExtras() { /* Do nothing */ }      // For drawing something extra on a menu
   void advanceItem();                                   // What happens when we move on to the next menu item?

protected:
   S32 currOffset;

   bool mRenderInstructions;

   // Handle keyboard input while a menu is displayed
   virtual bool processMenuSpecificKeys(KeyCode keyCode, char ascii);
   virtual bool processKeys(KeyCode keyCode, char ascii);
   

public:
   MenuUserInterface();                                     // Constructor
   ~MenuUserInterface() { menuItems.deleteAndClear(); }     // Destructor -- will this make valgrind happy???

   bool itemSelectedWithMouse;
   Vector<MenuItem *> menuItems;

   static const S32 MOUSE_SCROLL_INTERVAL = 100;

   string mMenuTitle;
   string mMenuSubTitle;

   Color mMenuSubTitleColor;
   bool mMenuFooterContainsInstructions;

   void idle(U32 timeDelta); 

   S32 selectedIndex;

   void render();    // Draw the basic menu
   void onKeyDown(KeyCode keyCode, char ascii);
   void onKeyUp(KeyCode keyCode);
   void onMouseMoved(S32 x, S32 y) { onMouseMoved(); }      // Redirect to argless version
   void onMouseMoved();
   void processMouse();

   void onActivate();
   void onReactivate();

   virtual void onEscape();
};

//////////
// <--- DO NOT SUBCLASS MainMenuUserInterface!! (unless you override onActivate) ---> //
class MainMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
   char motd[MOTD_LEN];
   U32 motdArriveTime;
   Timer mFadeInTimer;        // Track the brief fade in interval the first time menu is shown
   Timer mColorTimer;
   Timer mColorTimer2;
   enum {
      FadeInTime = 400,       // Time that fade in lasts (ms)
      ColorTime = 1000,
      ColorTime2 = 1700,
   };
   bool mTransDir;
   bool mTransDir2;
   bool mNeedToUpgrade;       // True if client is out of date and needs to upgrade, false if we're on the latest version
   bool mShowedUpgradeAlert;  // So we don't show the upgrade message more than once

   void renderExtras();

public:
   MainMenuUserInterface();                     // Constructor
   void processSelection(U32 index);
   void onEscape();
   void render();
   void idle(U32 timeDelta); 
   void setMOTD(const char *motdString);        // Message of the day, from Master
   void onActivate();
   void setNeedToUpgrade(bool needToUpgrade);   // Is client in need of an upgrade?

   bool showAnimation;                          // Is this the first time the menu is shown?
   bool mFirstTime;
   void showUpgradeAlert();                     // Display message to the user that it is time to upgrade
   bool getNeedToUpgrade();
};


extern MainMenuUserInterface gMainMenuUserInterface;


////////////////////////////////////////
////////////////////////////////////////

class OptionsMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;

public:
   OptionsMenuUserInterface();               // Constructor
   void processSelection(U32 index) { }         // Process selected menu item when right arrow is pressed
   void processShiftSelection(U32 index) { }    // And when the left arrow is pressed
   void onEscape();
   void setupMenus();
   void onActivate();
   void toggleDisplayMode();
};

extern OptionsMenuUserInterface gOptionsMenuUserInterface;


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
   HostMenuUserInterface();                  // Constructor
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

extern HostMenuUserInterface gHostMenuUserInterface;

////////////////////////////////////////
////////////////////////////////////////

class NameEntryUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
   void renderExtras();
   NetConnection::TerminationReason mReason;

public:
   NameEntryUserInterface();                    // Constructor
   void processSelection(U32 index) { }         // Process selected menu item when right arrow is pressed
   void processShiftSelection(U32 index) { }    // And when the left arrow is pressed
   void onEscape();
   void setupMenu();
   void onActivate();
   void setReactivationReason(NetConnection::TerminationReason r);
};


extern NameEntryUserInterface gNameEntryUserInterface;

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
   GameMenuUserInterface();            // Constructor

   void idle(U32 timeDelta);
   void onActivate();
   void onReactivate();
   void onEscape();
};

extern GameMenuUserInterface gGameMenuUserInterface;


////////////////////////////////////////
////////////////////////////////////////

class LevelMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;

public:
   LevelMenuUserInterface();        // Constructor
   void onActivate();
   void onEscape();
};

extern LevelMenuUserInterface gLevelMenuUserInterface;


////////////////////////////////////////
////////////////////////////////////////

class LevelMenuSelectUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
   Vector<string> mLevels;

public:
   LevelMenuSelectUserInterface();        // Constructor
   string category;
   void onActivate();
   bool processMenuSpecificKeys(KeyCode keyCode, char ascii);  // Custom key handling for level selection menus

   void processSelection(U32 index);
   void onEscape();
};

extern LevelMenuSelectUserInterface gLevelMenuSelectUserInterface;


////////////////////////////////////////
////////////////////////////////////////

class AdminMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
   SafePtr<GameType> mGameType;

public:
   AdminMenuUserInterface();      // Constructor
   void onActivate();
   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();
};


////////////////////////////////////////
////////////////////////////////////////

class PlayerMenuUserInterface : public MenuUserInterface
{
public:
   enum Action {
      Kick,
      ChangeTeam,
      ActionCount,
   } action;

private:
   typedef MenuUserInterface Parent;
public:
   PlayerMenuUserInterface();        // Constructor

   void render();
   void playerSelected(U32 index);
   void onEscape();
};

extern PlayerMenuUserInterface gPlayerMenuUserInterface;


////////////////////////////////////////
////////////////////////////////////////

class TeamMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
public:
   TeamMenuUserInterface();        // Constructor
   void render();
   void onEscape();
   const char *nameToChange;

   void processSelection(U32 index);
};

extern TeamMenuUserInterface gTeamMenuUserInterface;

};

#endif

