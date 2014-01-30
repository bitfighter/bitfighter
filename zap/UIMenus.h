//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UIMENUS_H_
#define _UIMENUS_H_

#include "UI.h"                  // Parent class

#include "SharedConstants.h"     // For MOTD_LEN
#include "PlayerActionEnum.h"
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
   S32 checkMenuIndexBounds(S32 index);   // Returns corrected index
   Timer mScrollTimer;

   Timer mFadingNoticeTimer;
   S32 mFadingNoticeVerticalPosition;
   string mFadingNoticeMessage;

   // For detecting keys being held down
   bool mRepeatMode;
   bool mKeyDown;

   virtual void renderExtras() const;     // For drawing something extra on a menu
   void advanceItem();                    // What happens when we move on to the next menu item?

   virtual void initialize();

protected:
   Vector<boost::shared_ptr<MenuItem> > mMenuItems;

   S32 getOffset();                                   // Calculates index of topmost visible item
   S32 getBaseYStart() const;                         // Base calculation for getYStart()
   virtual S32 getYStart() const;                     // Get vert pos of first menu item
   virtual S32 getTextSize(MenuItemSize size) const;  // Let menus set their own text size
   virtual S32 getGap(MenuItemSize size) const;       // Gap is the space between items

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

   BfObject *mAssociatedObject;     // Some menus can have an associated object... this is it

public:
   // Constructor
   explicit MenuUserInterface(ClientGame *game);
   MenuUserInterface(ClientGame *game, const string &title);
   virtual ~MenuUserInterface();

   bool isScrollingMenu();

   void clearMenuItems();
   S32 addMenuItem(MenuItem *menuItem);
   void addWrappedMenuItem(boost::shared_ptr<MenuItem> menuItem);
   MenuItem *getMenuItem(S32 index);
   S32 getMenuItemCount();

   bool itemSelectedWithMouse;

   S32 selectedIndex;               // Index of the currently highlighted menu item (public so tests can access this member)

   string mMenuTitle;
   string mMenuSubTitle;

   Color mMenuSubTitleColor;
   bool mMenuFooterContainsInstructions;

   void idle(U32 timeDelta); 

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

   BfObject *getAssociatedObject();
   void setAssociatedObject(BfObject *obj);

   void setFadingNotice(U32 time, S32 top, const string &message);  // Set a fading notice on a menu
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
   
   static const S32 ColorTime = 1000;
   static const S32 ColorTime2 = 1700;
   
   bool mTransDir;
   bool mTransDir2;
   bool mNeedToUpgrade;       // True if client is out of date and needs to upgrade, false if we're on the latest version
   bool mShowedUpgradeAlert;  // So we don't show the upgrade message more than once

   S32 getYStart() const;

   void renderExtras() const;

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

   void render();

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

class InGameHelpOptionsUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit InGameHelpOptionsUserInterface(ClientGame *game);        // Constructor
   virtual ~InGameHelpOptionsUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
};


////////////////////////////////////////
////////////////////////////////////////

class RobotOptionsMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit RobotOptionsMenuUserInterface(ClientGame *game);        // Constructor
   virtual ~RobotOptionsMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
   void saveSettings();
};


////////////////////////////////////////
////////////////////////////////////////

class ServerPasswordsMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit ServerPasswordsMenuUserInterface(ClientGame *game);        // Constructor
   virtual ~ServerPasswordsMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
   void saveSettings();
};


////////////////////////////////////////
////////////////////////////////////////

class HostMenuUserInterface : public MenuUserInterface
{
   enum MenuItems {
      OPT_HOST,
      OPT_ROBOTS,
      OPT_NAME,
      OPT_DESCR,
      OPT_GETMAP,
      OPT_MAX_PLAYERS,
      OPT_PORT,
      OPT_COUNT
   };

   static const S32 FIRST_EDITABLE_ITEM = OPT_NAME;

   typedef MenuUserInterface Parent;

private:
   S32 mEditingIndex;                        // Index of item we're editing, -1 if none

public:
   explicit HostMenuUserInterface(ClientGame *game);        // Constructor
   virtual ~HostMenuUserInterface();

   //void idle(U32 timeDelta);

   void onEscape();
   void setupMenus();
   void onActivate();
   void render();
   void saveSettings();       // Save parameters in INI file
};


////////////////////////////////////////
////////////////////////////////////////

// Player enters their name and password
class NameEntryUserInterface : public MenuUserInterfaceWithIntroductoryAnimation
{
   typedef MenuUserInterfaceWithIntroductoryAnimation Parent;

private:
   void renderExtras() const;
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

class RobotsMenuUserInterface : public MenuUserInterface
{
private:
   typedef MenuUserInterface Parent;

public:
   explicit RobotsMenuUserInterface(ClientGame *game);        // Constructor
   virtual ~RobotsMenuUserInterface();

   void onActivate();
   void onEscape();
};


////////////////////////////////////////
////////////////////////////////////////

class LevelMenuSelectUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

protected:
   Vector<string> mLevels;
private:
   Timer mStillTypingNameTimer;
   string mNameSoFar;

public:
   explicit LevelMenuSelectUserInterface(ClientGame *game);        // Constructor
   virtual ~LevelMenuSelectUserInterface();

   void idle(U32 timeDelta);

   string category;
   void onActivate();
   bool processMenuSpecificKeys(InputCode inputCode); // Custom key handling for level selection menus
      
   S32 getIndexOfNext(const string &startingWith);    // Public so tests can access this

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

   PlayerAction action;
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

