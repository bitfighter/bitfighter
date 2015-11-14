//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_ITEM_LIST_SELECT_MENU_H_
#define _UI_ITEM_LIST_SELECT_MENU_H_

#include "UIMenus.h"             // Parent class
#include "PlayerActionEnum.h"

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

// For choosing a playlist while hosting
class PlaylistMenuUserInterface : public ItemListSelectUserInterface
{
   typedef ItemListSelectUserInterface Parent;

private:
   virtual Vector<string> getPlaylists() const;

public:
   explicit PlaylistMenuUserInterface(ClientGame *game, UIManager *uiManager);      // Constructor
   virtual ~PlaylistMenuUserInterface();

   void onActivate();

   void processSelection(U32 index);
   virtual void setPlaylist(const string& playlistName) const;
};


////////////////////////////////////////
////////////////////////////////////////

// For choosing a playlist in-game
class PlaylistInGameMenuUserInterface : public PlaylistMenuUserInterface
{
private:
   typedef PlaylistMenuUserInterface Parent;
   Vector<string> getPlaylists() const;

public:
   explicit PlaylistInGameMenuUserInterface(ClientGame *game, UIManager *uiManager);      // Constructor
   virtual ~PlaylistInGameMenuUserInterface();

   void setPlaylist(const string& playlistName) const;
};


////////////////////////////////////////
////////////////////////////////////////

class PlayerMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit PlayerMenuUserInterface(ClientGame *game, UIManager *uiManager);  // Constructor
   virtual ~PlayerMenuUserInterface();

   void idle(U32 timeDelta);
   void render() const;
   void playerSelected(U32 index) const;

   PlayerAction action;
};

}


#endif