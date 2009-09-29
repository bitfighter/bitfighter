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
#include "input.h"
#include "keyCode.h"
#include "point.h"

#include "../tnl/tnlNetBase.h"
#include "../tnl/tnlTypes.h"

#include <string>




namespace Zap
{

using namespace std;

enum PlayerType {
   PlayerTypePlayer,
   PlayerTypeAdmin,
   PlayerTypeRobot,
   PlayerTypeIrrelevant
};

struct MenuItem
{
   const char *mText;   // Text displayed on menu
   U32 mIndex;          // Unique int index
   KeyCode key1;        // Allow two shortcut keys per menu item...
   KeyCode key2;
   Color color;         // Color in which item should be displayed
   PlayerType mType;    // Type of player, for name menu
   S32 mPlayers;        // Num players, for team select menu
   S32 mScore;          // Team score, for team select menu
   bool mCurrTeam;      // Is this a player's current team? (for team select menu)

   // Constructor I
   MenuItem(const char *text = 0, U32 index = 0, KeyCode k1 = KEY_UNKNOWN, KeyCode k2 = KEY_UNKNOWN, Color c = Color(1, 1, 1))
   {
      mText = text;
      mIndex = index;
      key1 = k1;
      key2 = k2;
      color = c;
      mType = PlayerTypeIrrelevant;
      mPlayers = -1;
      mScore = -1;
   }

   // Constructor II
   MenuItem(const char *text, U32 index, KeyCode k1, KeyCode k2, Color c, PlayerType type)
   {
      mText = text;
      mIndex = index;
      key1 = k1;
      key2 = k2;
      color = c;
      mType = type;
      mPlayers = -1;
      mScore = -1;
   }

   // Constructor III
   MenuItem(const char *text, U32 index, KeyCode k1, KeyCode k2, Color c, bool currTeam, S32 players, S32 score)
   {
      mText = text;
      mIndex = index;
      key1 = k1;
      key2 = k2;
      color = c;
      mType = PlayerTypeIrrelevant;
      mPlayers = players;
      mScore = score;
      mCurrTeam = currTeam;
   }

};

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

protected:
   S32 currOffset;
   
public:
   MenuUserInterface();                // Constructor

   bool itemSelectedWithMouse;
   Vector<MenuItem> menuItems;

   enum {
      MouseScrollInterval = 100,
   };

   char menuTitle[255];
   const char *menuSubTitle;
   Color menuSubTitleColor;
   const char *menuFooter;
   bool menuFooterContainsInstructions;

   void idle(U32 timeDelta); 

   // Track if certain menu options are active, for keyboard shortcut entry
   U32 mAdminActiveOption;

   S32 selectedIndex;

   void render();    // Draw the basic menu
   virtual void renderExtras() { /* Do nothing */ }     // For drawing something extra on a menu, not currently used...

   void onKeyDown(KeyCode keyCode, char ascii);
   void onKeyUp(KeyCode keyCode);
   void onMouseMoved(S32 x, S32 y);
   void processMouse();

   void onActivate();
   void onReactivate();

   virtual void onEscape();
   virtual void processSelection(U32 index) = 0;
   virtual void processShiftSelection(U32 index) { /* Do nothing */ }

   // Handle keyboard input while a menu is displayed
   virtual void processMenuSpecificKeys(KeyCode keyCode);
   virtual void processStandardKeys(KeyCode keyCode);

   // Helper rendering functions
   static void renderArrowAbove(S32 pos);
   static void renderArrowBelow(S32 pos);

   static void renderMenuInstructions(S32 variant);

};

//////////
// <--- DO NOT SUBCLASS MainMenuUserInterface!! (unless you override onActivate) ---> //
class MainMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
   char motd[MOTDLen];
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

   Vector<string> mLevelLoadDisplayNames;    // For displaying levels as they're loaded in host mode
   S32 mLevelLoadDisplayTotal;

public:
   MainMenuUserInterface();                     // Constructor
   void processSelection(U32 index);
   void onEscape();
   void render();
   void idle(U32 timeDelta); 
   void setMOTD(const char *motdString);        // Message of the day, from Master
   void onActivate();
   void setNeedToUpgrade(bool needToUpgrade);   // Is client in need of an upgrade?

   bool firstTime;                              // Is this the first time the menu is shown?
   void showUpgradeAlert();                     // Display message to the user that it is time to upgrade
   bool getNeedToUpgrade();

   void addProgressListItem(string item);
   void renderProgressListItems();
   Timer levelLoadDisplayFadeTimer;
   bool levelLoadDisplayDisplay;
   void clearLevelLoadDisplay();
};

extern MainMenuUserInterface gMainMenuUserInterface;

//////////
class OptionsMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
public:
   OptionsMenuUserInterface();               // Constructor
   void processSelection(U32 index);         // Process selected menu item when right arrow is pressed
   void processShiftSelection(U32 index);    // And when the left arrow is pressed
   void onEscape();
   void setupMenus();
   void onActivate();
   void toggleFullscreen();
   void actualizeScreenMode(bool first = false);

   // For setting options based on INI values
   void setJoystick(ControllerTypeType jsType);
};

extern OptionsMenuUserInterface gOptionsMenuUserInterface;

//////////
class GameType;

class GameMenuUserInterface : public MenuUserInterface
{
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
   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();
};

extern GameMenuUserInterface gGameMenuUserInterface;

//////////
class LevelMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
public:
   LevelMenuUserInterface();        // Constructor
   void onActivate();
   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();
};

extern LevelMenuUserInterface gLevelMenuUserInterface;
//////////
class LevelMenuSelectUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
public:
   LevelMenuSelectUserInterface();        // Constructor
   string category;
   void onActivate();
   void processMenuSpecificKeys(KeyCode keyCode);  // Custom key handling for level selection menus

   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();
};

extern LevelMenuSelectUserInterface gLevelMenuSelectUserInterface;

//////////
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

//////////
class PlayerMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
public:
   PlayerMenuUserInterface();        // Constructor
   enum Action {
      Kick,
      ChangeTeam,
      ActionCount,
   } action;
   void render();
   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();
};

extern PlayerMenuUserInterface gPlayerMenuUserInterface;

//////////
class TeamMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;
public:
   TeamMenuUserInterface();        // Constructor
   void render();
   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();
   const char *nameToChange;
};

extern TeamMenuUserInterface gTeamMenuUserInterface;

};

#endif
