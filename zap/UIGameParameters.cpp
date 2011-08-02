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
#include "keyCode.h"
#include "IniFile.h"
#include "config.h"
#include "gameType.h"

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"

#include <string>

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

namespace Zap
{

// Constructor
GameParamUserInterface::GameParamUserInterface(Game *game) : Parent(game)
{
   setMenuID(GameParamsUI);
   
   mMenuTitle = "Game Parameters Menu";
   mMenuSubTitle = "";
}


void GameParamUserInterface::onActivate()
{
   selectedIndex = 0;                          // First item selected when we begin

   // Force rebuild of all params for current gameType; this will make sure we have the latest info if we've loaded a new level,
   // but will also preserve any values entered for gameTypes that are not current.
   clearCurrentGameTypeParams();

   updateMenuItems();   
   origGameParams = getGame()->toString();     // Save a copy of the params coming in for comparison when we leave to see what changed
   SDL_ShowCursor(SDL_DISABLE);
}


// Find and delete any parameters associated with the current gameType
void GameParamUserInterface::clearCurrentGameTypeParams()
{
   const char **keys = getGame()->getGameType()->getGameParameterMenuKeys();

   S32 i = 0;

   while(strcmp(keys[i], ""))
   {
      MenuItemMap::iterator iter = mMenuItemMap.find(keys[i]);

      boost::shared_ptr<MenuItem> menuItem;

      if(iter != mMenuItemMap.end())      
         mMenuItemMap.erase(iter);

      i++;
   }
}


extern const char *gGameTypeNames[];
extern S32 gDefaultGameTypeIndex;

static void changeGameTypeCallback(Game *game, U32 gtIndex)
{
   TNL::Object *theObject = TNL::Object::create(gGameTypeNames[gtIndex]);   // Instantiate our gameType object
   GameType *gt = dynamic_cast<GameType *>(theObject);                      // and cast it to GameType

   gt->addToGame(game, game->getEditorDatabase());

   // If we have a new gameType, we might have new game parameters; update the menu!
   game->getUIManager()->getGameParamUserInterface()->updateMenuItems();
}


static Vector<string> gameTypes;

static void buildGameTypeList()
{
   for(S32 i = 0; gGameTypeNames[i]; i++)
   {
      // The following seems rather wasteful, but this is hardly a performance sensitive area...
      GameType *gameType = dynamic_cast<GameType *>(TNL::Object::create(gGameTypeNames[i]));          // Instantiate our gameType object
      gameTypes.push_back(gameType->getGameTypeString());
   }
}


#ifndef WIN32
extern const S32 Game::MIN_GRID_SIZE;     // Needed by gcc, cause errors in VC++... and for Mac?
extern const S32 Game::MAX_GRID_SIZE;
#endif


static S32 getGameTypeIndex(const char *gt)
{
   for(S32 i = 0; gGameTypeNames[i]; i++)
      if(!strcmp(gt, gGameTypeNames[i]))
         return i;

   return -1;
}


void GameParamUserInterface::updateMenuItems()
{
   GameType *gameType = getGame()->getGameType();
   TNLAssert(gameType, "Invalid game type!");

   if(gameTypes.size() == 0)     // Should only be run once, as these gameTypes will not change during the session
      buildGameTypeList();

   menuItems.clear();

   menuItems.push_back(boost::shared_ptr<MenuItem>(new ToggleMenuItem(getGame(),
                                                                      "Game Type:",       
                                                                      gameTypes,
                                                                      getGameTypeIndex(gameType->getClassName()),
                                                                      true,
                                                                      changeGameTypeCallback,
                                                                      gameType->getInstructionString())));


   string fn = stripExtension(getGame()->getUIManager()->getEditorUserInterface()->getLevelFileName());
   menuItems.push_back(boost::shared_ptr<MenuItem>(new EditableMenuItem(getGame(),
                                                                        "Filename:",                         // name
                                                                        fn,                                  // val
                                                                        "",                                  // empty val
                                                                        "File where this level is stored",   // help
                                                                        MAX_FILE_NAME_LEN)));
   const char **keys = gameType->getGameParameterMenuKeys();

   S32 i = 0;
   while(strcmp(keys[i], ""))
   {
      MenuItemMap::iterator iter = mMenuItemMap.find(keys[i]);

      boost::shared_ptr<MenuItem> menuItem;

      if(iter != mMenuItemMap.end())      
         menuItem = iter->second;
      else                 // Item not found
      {
         menuItem = gameType->getMenuItem(getGame(), keys[i]);
         TNLAssert(menuItem.get(), "Failed to make a new menu item!");

         mMenuItemMap.insert(pair<const char *, boost::shared_ptr<MenuItem> >(keys[i], menuItem));
      }

      menuItems.push_back(menuItem);

      i++;
   }
}


// Runs as we're exiting the menu
void GameParamUserInterface::onEscape()
{
   S32 gameTypeIndex = dynamic_cast<ToggleMenuItem *>(menuItems[0].get())->getValueIndex();

   getGame()->getUIManager()->getEditorUserInterface()->setLevelFileName(menuItems[1]->getValue());  

   GameType *gameType = getGame()->getGameType();

   const char **keys = gameType->getGameParameterMenuKeys();

   S32 i = 0;

   while(strcmp(keys[i], ""))
   {
      MenuItemMap::iterator iter = mMenuItemMap.find(keys[i]);

      MenuItem *menuItem = iter->second.get();
      gameType->saveMenuItem(menuItem, keys[i]);
    
      i++;
   }

   if(anythingChanged())
   {
      EditorUserInterface *ui = getGame()->getUIManager()->getEditorUserInterface();

      ui->setNeedToSave(true);       // Need to save to retain our changes
      ui->mAllUndoneUndoLevel = -1;  // This change can't be undone
      ui->validateLevel();
   }

   // Now back to our previously scheduled program...  (which will be the editor, of course)
   UserInterface::reactivatePrevUI();
}


bool GameParamUserInterface::anythingChanged()
{
   return origGameParams != getGame()->toString();
}


};

