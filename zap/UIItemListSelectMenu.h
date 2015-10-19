//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_ITEM_LIST_SELECT_MENU_H_
#define _UI_ITEM_LIST_SELECT_MENU_H_

#include "UIMenus.h"             // Parent class

#include "tnlVector.h"    


namespace Zap
{

using namespace std;


class ItemListSelectUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

private:
   Timer mStillTypingNameTimer;
   string mNameSoFar;

public:
   ItemListSelectUserInterface(ClientGame *game, UIManager *uiManager);   // Constructor
   virtual ~ItemListSelectUserInterface();

   virtual void onActivate();

   virtual void idle(U32 timeDelta);

   bool processMenuSpecificKeys(InputCode inputCode);

   S32 getIndexOfNext(const string &startingWith) const;    // Public so tests can access this  TODO: Fix test access

   void onEscape();
};

////////////////////////////////////////
////////////////////////////////////////


class LevelMenuSelectUserInterface : public ItemListSelectUserInterface
{
   typedef ItemListSelectUserInterface Parent;

private:
   string mCategory;

protected:
   Vector<string> mMenuDisplayItems;

public:
   LevelMenuSelectUserInterface(ClientGame *game, UIManager *uiManager);   // Constructor
   virtual ~LevelMenuSelectUserInterface();

   void onActivate();

   virtual void processSelection(U32 index);

   void setCategory(const string &category);
};


////////////////////////////////////////
////////////////////////////////////////

class PlaylistMenuUserInterface : public ItemListSelectUserInterface
{
private:
   typedef ItemListSelectUserInterface Parent;

public:
   explicit PlaylistMenuUserInterface(ClientGame *game, UIManager *uiManager);      // Constructor
   virtual ~PlaylistMenuUserInterface();

   void onActivate();

   void processSelection(U32 index);
};

}


#endif