//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIItemListSelectMenu.h"

#include "UIManager.h"

#include "ClientGame.h"
#include "gameType.h"            // Can get rid of this with some simple passthroughs
#include "DisplayManager.h"
#include "Cursor.h"
#include "stringUtils.h"

namespace Zap
{


// Constructor
ItemListSelectUserInterface::ItemListSelectUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   // When you start typing a name, any character typed within the mStillTypingNameTimer period will be considered
   // to be the next character of the name, rather than a new entry
   mStillTypingNameTimer.setPeriod(ONE_SECOND);

}


// Destructor
ItemListSelectUserInterface::~ItemListSelectUserInterface()
{
   // Do nothing
}


void ItemListSelectUserInterface::onActivate()
{
   Parent::onActivate();

   mNameSoFar = "";
   mStillTypingNameTimer.clear();

   clearMenuItems();    // We'll repopulate below...

   if(mItemSelectedWithMouse)
      onMouseMoved();
   //else
   //   mSelectedIndex = 0;
}


void ItemListSelectUserInterface::idle(U32 timeDelta)
{
   if(mStillTypingNameTimer.update(timeDelta))
      mNameSoFar = "";
}


bool ItemListSelectUserInterface::processMenuSpecificKeys(InputCode inputCode)
{
   string inputString = InputCodeManager::inputCodeToPrintableChar(inputCode);

   if(inputString == "")
      return false;
   
   mNameSoFar.append(inputString);

   string mNameSoFarLc = lcase(mNameSoFar);

   if(stringContainsAllTheSameCharacter(mNameSoFarLc))
   {
      mSelectedIndex = getIndexOfNext(mNameSoFarLc.substr(0, 1));

      if(mNameSoFar.size() > 1 && lcase(getMenuItem(mSelectedIndex)->getValue()).substr(0, mNameSoFar.length()) != mNameSoFarLc)
         mNameSoFar = mNameSoFar.substr(0, mNameSoFar.length() - 1);    // Remove final char, the one we just added above
   }
   else
      mSelectedIndex = getIndexOfNext(mNameSoFarLc);


   mStillTypingNameTimer.reset();
   mItemSelectedWithMouse = false;

   // Move the mouse to the new selection to make things "feel better"
   MenuItemSize size = getMenuItem(mFirstVisibleItem)->getSize();
   S32 y = getYStart();

   for(S32 j = mFirstVisibleItem; j < mSelectedIndex; j++)
   {
      size = getMenuItem(j)->getSize();
      y += getTextSize(size) + getGap(size);
   }

   y += getTextSize(size) / 2;

   // WarpMouse fires a mouse event, which will cause the cursor to become visible, which we don't want.  Therefore,
   // we must resort to the kind of gimicky/hacky method of setting a flag, telling us that we should ignore the
   // next mouse event that comes our way.  It might be better to handle this at the Event level, by creating a custom
   // method called WarpMouse that adds the suppression.  At this point, however, the only place we care about this
   // is here so...  well... this works.
#if SDL_VERSION_ATLEAST(2,0,0)
   SDL_WarpMouseInWindow(DisplayManager::getScreenInfo()->sdlWindow, (S32)DisplayManager::getScreenInfo()->getMousePos()->x, y);
#else
   SDL_WarpMouse(DisplayManager::getScreenInfo()->getMousePos()->x, y);
#endif
   Cursor::disableCursor();
   mIgnoreNextMouseEvent = true;
   playBoop();

   return true;
}


// Return index of next level starting with specified string; if none exists, returns current index.
// If startingWith is only one character, the entry we're looking for could be behind us.  See tests
// for examples of this.
S32 ItemListSelectUserInterface::getIndexOfNext(const string &startingWithLc) const
{
   TNLAssert(startingWithLc.length() > 0, "Did not expect an empty string here!");
   TNLAssert(startingWithLc == lcase(startingWithLc), "Expected a lowercased string here");

   bool first = true;
   bool multiChar = startingWithLc.length() > 1;
   S32 offset = multiChar ? 0 : 1;

   // Loop until we hit the end of the list, or we hit an item that sorts > our startingString (meaning we overshot).
   // But we only care about overshoots in multiChar mode because there could well be single-char hits behind us in the list.
   while(true)
   {
      if(mSelectedIndex + offset >= getMenuItemCount())    // Hit end of list -- loop to beginning
         offset = -mSelectedIndex;

      string prospectiveItem = lcase(getMenuItem(mSelectedIndex + offset)->getValue());

      if(prospectiveItem.substr(0, startingWithLc.size()) == startingWithLc)
         return mSelectedIndex + offset;

      if(offset == 0 && !first)
         break;

      offset++;
      first = false;
   }

   // Found no match; return current index
   return mSelectedIndex;
}


void ItemListSelectUserInterface::onEscape()
{
   getUIManager()->reactivatePrevUI();    // to LevelMenuUserInterface
}

   
////////////////////////////////////////
////////////////////////////////////////

// Constructor
LevelMenuSelectUserInterface::LevelMenuSelectUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   // Do nothing
}


// Destructor
LevelMenuSelectUserInterface::~LevelMenuSelectUserInterface()
{
   // Do nothing
}


static void processLevelSelectionCallback(ClientGame *game, U32 index)             
{
   game->getUIManager()->getUI<LevelMenuSelectUserInterface>()->processSelection(index);
}


const U32 UPLOAD_LEVELS_BIT = 0x80000000;

void LevelMenuSelectUserInterface::processSelection(U32 index)     
{
   Parent::onActivate();
   GameConnection *gc = getGame()->getConnectionToServer();

   if((index & UPLOAD_LEVELS_BIT) && (index & (~UPLOAD_LEVELS_BIT)) < U32(mMenuDisplayItems.size()))
   {
      FolderManager *folderManager = mGameSettings->getFolderManager();
      string filename = strictjoindir(folderManager->getLevelDir(), mMenuDisplayItems[index & (~UPLOAD_LEVELS_BIT)]);

      if(!gc->TransferLevelFile(filename.c_str()))
         getGame()->displayErrorMessage("!!! Can't upload level: unable to read file");
   }
   else
      gc->c2sRequestLevelChange(index, false);     // The selection index is the level to load

   getUIManager()->reactivateGameUI();             // Back to the game
}


void LevelMenuSelectUserInterface::setCategory(const string &category)
{
   mCategory = category;
}


void LevelMenuSelectUserInterface::onActivate()
{
   Parent::onActivate();

   // Replace with a getLevelCount() method on Game?
   ClientGame *game = getGame();
   GameConnection *gc = game->getConnectionToServer();

   if(gc == NULL || gc->mLevelInfos.size() == 0)
      return;

   mMenuTitle = "CHOOSE LEVEL [" + mCategory + "]";
   mMenuDisplayItems.clear();

   char c[2];
   c[1] = 0;   // null termination

   if(mCategory == UPLOAD_LEVELS)
   {
      // Get all the playable levels in levelDir
      mMenuDisplayItems = mGameSettings->getLevelList();

      for(S32 i = 0; i < mMenuDisplayItems.size(); i++)
      {
         c[0] = mMenuDisplayItems[i][0];
         addMenuItem(new MenuItem(i | UPLOAD_LEVELS_BIT, mMenuDisplayItems[i], processLevelSelectionCallback, "", InputCodeManager::stringToInputCode(c)));
      }
   }
 
   for(S32 i = 0; i < gc->mLevelInfos.size(); i++)
   {
      if(gc->mLevelInfos[i].mLevelName == "")   // Skip levels with blank names --> but all should have names now!
         continue;

      if(strcmp(gc->mLevelInfos[i].getLevelTypeName(), mCategory.c_str()) == 0 || mCategory == ALL_LEVELS)
      {
         const char *levelName = gc->mLevelInfos[i].mLevelName.getString();
         c[0] = levelName[0];
         addMenuItem(new MenuItem(i, levelName, processLevelSelectionCallback, "", InputCodeManager::stringToInputCode(c)));
      }
   }

   sortMenuItems();
}


////////////////////////////////////////
////////////////////////////////////////

const static string NO_PLAYLIST = "DON'T USE PLAYLIST";

// Constructor
PlaylistMenuUserInterface::PlaylistMenuUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   // Do nothing
}


// Destructor
PlaylistMenuUserInterface::~PlaylistMenuUserInterface()
{
   // Do nothing
}


// Selected a playlist!  What do we do now?
static void processPlaylistSelectionCallback(ClientGame *game, U32 index)             
{
   game->getUIManager()->getUI<PlaylistMenuUserInterface>()->processSelection(index);
}


void PlaylistMenuUserInterface::processSelection(U32 index)     
{
   string playlistName = getMenuItem(index)->getPrompt();
   if(playlistName == NO_PLAYLIST)
      playlistName = "";

   setPlaylist(playlistName);             // Different implementations when hosting vs. when playing
   getUIManager()->reactivatePrevUI();    // To hosting menu
}


void PlaylistMenuUserInterface::setPlaylist(const string &playlistName) const
{
   getGame()->setPlaylist(playlistName);
}


void PlaylistMenuUserInterface::onActivate()
{
   Parent::onActivate();
   mMenuTitle = "CHOOSE PLAYLIST";

   // Replace with a getLevelCount() method on Game?
   ClientGame *game = getGame();

   Vector<string> playlists = getPlaylists();

   clearMenuItems();

   // Any items to select from?
   if(playlists.size() > 0)
   {
      char c[2];
      c[1] = 0;   // null termination

      for(S32 i = 0; i < playlists.size(); i++)
      {
         if(playlists[i] == "")   // Skip blanks, but there should be none
            continue;

         string playlistName = playlists[i];
         c[0] = playlistName[0];
         addMenuItem(new MenuItem(i, playlistName, processPlaylistSelectionCallback, "", InputCodeManager::stringToInputCode(c)));
      }

      sortMenuItems();
   }

   addMenuItem(new MenuItem(playlists.size(), NO_PLAYLIST, processPlaylistSelectionCallback, "", InputCodeManager::stringToInputCode("N")));
}


Vector<string> PlaylistMenuUserInterface::getPlaylists() const
{
   return getGame()->getServerPlaylists();
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
PlaylistInGameMenuUserInterface::PlaylistInGameMenuUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   // Do nothing
}


// Destructor
PlaylistInGameMenuUserInterface::~PlaylistInGameMenuUserInterface()
{
   // Do nothing
}


// Selected a playlist!  What do we do now?
static void processPlaylistInGameSelectionCallback(ClientGame *game, U32 index)             
{
   game->getUIManager()->getUI<PlaylistInGameMenuUserInterface>()->processSelection(index);
}


void PlaylistInGameMenuUserInterface::setPlaylist(const string &playlistName) const
{
   getGame()->setPlaylistAndAlertServer(playlistName);
}


Vector<string> PlaylistInGameMenuUserInterface::getPlaylists() const
{
   return getGame()->getServerPlaylists();      // In game playlist selection comes from server
}


////////////////////////////////////////
////////////////////////////////////////



// Constructor
PlayerMenuUserInterface::PlayerMenuUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   // Do nothing
}


// Destructor
PlayerMenuUserInterface::~PlayerMenuUserInterface()
{
   // Do nothing
}


static void playerSelectedCallback(ClientGame *game, U32 index) 
{
   game->getUIManager()->getUI<PlayerMenuUserInterface>()->playerSelected(index);
}


void PlayerMenuUserInterface::playerSelected(U32 index) const
{
   // When we created the menu, names were not sorted, and item indices were assigned in "natural order".  Then
   // the menu items were sorted by name, and now the indices are now jumbled.  This bit here tries to get the
   // new, actual list index of an item given its original index.
   for(S32 i = 0; i < getMenuItemCount(); i++)
      if(getMenuItem(i)->getIndex() == (S32)index)
      {
         index = i;
         break;
      }

   GameType *gt = getGame()->getGameType();

   if(action == PlayerActionChangeTeam)
   {
      TeamMenuUserInterface *ui = getUIManager()->getUI<TeamMenuUserInterface>();
      ui->nameToChange = getMenuItem(index)->getPrompt();

      getUIManager()->activate<TeamMenuUserInterface>();     // Show menu to let player select a new team
   }

   else if(gt)    // action == Kick
      gt->c2sKickPlayer(getMenuItem(index)->getPrompt());


   if(action != PlayerActionChangeTeam)      // Unless we need to move on to the change team screen...
      getUIManager()->reactivateGameUI();    // ...it's back to the game!
}


// By rebuilding everything every tick, menus can be dynamically updated
void PlayerMenuUserInterface::idle(U32 timeDelta)
{
   clearMenuItems();

   GameConnection *conn = getGame()->getConnectionToServer();
   if(!conn)
      return;

   char c[] = "A";      // Dummy shortcut key
   for(S32 i = 0; i < getGame()->getClientCount(); i++)
   {
      ClientInfo *clientInfo = ((Game *)getGame())->getClientInfo(i);      // Lame!

      strncpy(c, clientInfo->getName().getString(), 1);        // Grab first char of name for a shortcut key

      // Will be used to show admin/player/robot prefix on menu
      PlayerType pt = clientInfo->isRobot() ? PlayerTypeRobot : (clientInfo->isAdmin() ? PlayerTypeAdmin : PlayerTypePlayer);    

      PlayerMenuItem *newItem = new PlayerMenuItem(i, clientInfo->getName().getString(), playerSelectedCallback, 
                                                   InputCodeManager::stringToInputCode(c), pt);
      newItem->setUnselectedColor(getGame()->getTeamColor(clientInfo->getTeamIndex()));

      addMenuItem(newItem);
   }

   sortMenuItems();

   if(action == PlayerActionKick)
      mMenuTitle = "CHOOSE PLAYER TO KICK";
   else if(action == PlayerActionChangeTeam)
      mMenuTitle = "CHOOSE WHOSE TEAM TO CHANGE";
   else
      TNLAssert(false, "Unknown action!");
}


void PlayerMenuUserInterface::render() const
{
   Parent::render();
}


}