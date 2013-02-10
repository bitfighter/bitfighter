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

#include "UIMenus.h"
#include "UIGameParameters.h"
#include "UIChat.h"
#include "UIDiagnostics.h"
#include "UIEditor.h"
#include "UINameEntry.h"
#include "input.h"
#include "InputCode.h"
#include "IniFile.h"
#include "config.h"
#include "gameType.h"
#include "ClientGame.h"
#include "Cursor.h"

#include "SDL.h"
#ifdef TNL_OS_MOBILE
#include "SDL_opengles.h"
#else
#include "SDL_opengl.h"
#endif

#include <string>


namespace Zap
{

// Default constructor
SavedMenuItem::SavedMenuItem()
{
   /* Unused */
}


SavedMenuItem::SavedMenuItem(MenuItem *menuItem)
{
   mParamName = menuItem->getPrompt();
   setValues(menuItem);
}


void SavedMenuItem::setValues(MenuItem *menuItem)
{
   mParamVal = menuItem->getValueForWritingToLevelFile();
}


string SavedMenuItem::getParamName()
{
   return mParamName;
}


string SavedMenuItem::getParamVal()
{
   return mParamVal;
}


////////////////////////////////////
////////////////////////////////////


// Constructor
GameParamUserInterface::GameParamUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(GameParamsUI);
   
   mMenuTitle = "Game Parameters Menu";
   mMenuSubTitle = "";
   mMaxMenuSize = S32_MAX;                // We never want scrolling on this menu!

   mGameSpecificParams = 0;
   selectedIndex = 0;
   mQuitItemIndex = 0;
   changingItem = -1;
}


void GameParamUserInterface::onActivate()
{
   selectedIndex = 0;                          // First item selected when we begin

   // Force rebuild of all params for current gameType; this will make sure we have the latest info if we've loaded a new level,
   // but will also preserve any values entered for gameTypes that are not current.
   clearCurrentGameTypeParams();

   updateMenuItems();   
   origGameParams = getGame()->toLevelCode();   // Save a copy of the params coming in for comparison when we leave to see what changed
   Cursor::disableCursor();
}


// Find and delete any parameters associated with the current gameType
void GameParamUserInterface::clearCurrentGameTypeParams()
{
   const Vector<string> keys = getGame()->getGameType()->getGameParameterMenuKeys();

   for(S32 i = 0; i < keys.size(); i++)
   {
      MenuItemMap::iterator iter = mMenuItemMap.find(keys[i]);

      boost::shared_ptr<MenuItem> menuItem;

      if(iter != mMenuItemMap.end())      
         mMenuItemMap.erase(iter);
   }
}


static void changeGameTypeCallback(ClientGame *game, U32 gtIndex)
{
   // Instantiate our gameType object and cast it to GameType
   TNL::Object *theObject = TNL::Object::create(GameType::getGameTypeClassName((GameTypeId)gtIndex));  
   GameType *gt = dynamic_cast<GameType *>(theObject);   

   gt->addToGame(game, NULL);    // GameType::addToGame() ignores database (and what would it do with one, anyway?), so we can pass NULL

   // If we have a new gameType, we might have new game parameters; update the menu!
   game->getUIManager()->getGameParamUserInterface()->updateMenuItems();
}



#ifndef WIN32
extern const S32 Game::MIN_GRID_SIZE;     // Needed by gcc, cause errors in VC++... and for Mac?
extern const S32 Game::MAX_GRID_SIZE;
#endif


extern S32 QSORT_CALLBACK alphaSort(string *a, string *b);


void GameParamUserInterface::updateMenuItems()
{
   static Vector<string> gameTypes;

   GameType *gameType = getGame()->getGameType();
   TNLAssert(gameType, "Missing game type!");

   if(gameTypes.size() == 0)     // Should only be run once, as these gameTypes will not change during the session
   {
      gameTypes = GameType::getGameTypeNames();
      gameTypes.sort(alphaSort);
   }

   clearMenuItems();

   addMenuItem(new ToggleMenuItem("Game Type:",       
                                  gameTypes,
                                  gameTypes.getIndex(gameType->getGameTypeName()),
                                  true,
                                  changeGameTypeCallback,
                                  gameType->getInstructionString()));


   string fn = stripExtension(getUIManager()->getEditorUserInterface()->getLevelFileName());
   addMenuItem(new TextEntryMenuItem("Filename:",                         // name
                                     fn,                                  // val
                                     "",                                  // empty val
                                     "File where this level is stored",   // help
                                     MAX_FILE_NAME_LEN));

   const Vector<string> keys = gameType->getGameParameterMenuKeys();

   for(S32 i = 0; i < keys.size(); i++)
   {
      MenuItemMap::iterator iter = mMenuItemMap.find(keys[i]);

      boost::shared_ptr<MenuItem> menuItem;

      if(iter != mMenuItemMap.end())      // What is this supposed to do?  I can't seem to make this condition occur.
         menuItem = iter->second;
      else                 // Item not found
      {
         menuItem = gameType->getMenuItem(keys[i]);
         TNLAssert(menuItem, "Failed to make a new menu item!");

         mMenuItemMap.insert(pair<string, boost::shared_ptr<MenuItem> >(keys[i], menuItem));
      }

      addWrappedMenuItem(menuItem);
   }
}


// Runs as we're exiting the menu
void GameParamUserInterface::onEscape()
{
   getUIManager()->getEditorUserInterface()->setLevelFileName(getMenuItem(1)->getValue());  

   GameType *gameType = getGame()->getGameType();

   const Vector<string> keys = gameType->getGameParameterMenuKeys();

   for(S32 i = 0; i < keys.size(); i++)
   {
      MenuItemMap::iterator iter = mMenuItemMap.find(keys[i]);

      MenuItem *menuItem = iter->second.get();
      gameType->saveMenuItem(menuItem, keys[i]);
   }

   if(anythingChanged())
   {
      EditorUserInterface *ui = getUIManager()->getEditorUserInterface();

      ui->setNeedToSave(true);       // Need to save to retain our changes
      ui->mAllUndoneUndoLevel = -1;  // This change can't be undone
      ui->validateLevel();
   }

   // Now back to our previously scheduled program...  (which will be the editor, of course)
   getUIManager()->reactivatePrevUI();
}


void GameParamUserInterface::processSelection(U32 index)
{
   // Do nothing
}


bool GameParamUserInterface::anythingChanged()
{
   return origGameParams != getGame()->toLevelCode();
}


S32 GameParamUserInterface::getTextSize(MenuItemSize size)
{
   return 18;
}


S32 GameParamUserInterface::getGap(MenuItemSize size)
{
   return 12;
}


};

