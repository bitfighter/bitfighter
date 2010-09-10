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

#include "../glut/glutInclude.h"
#include <string>

namespace Zap
{

GameParamUserInterface gGameParamUserInterface;

// Constructor
GameParamUserInterface::GameParamUserInterface() : MenuUserInterface()
{
   setMenuID(GameParamsUI);
   menuTitle = "Game Parameters Menu";
   menuSubTitle = "";
   //menuSubTitleColor = Color(1,1,1);
}

void GameParamUserInterface::onActivate()
{
   selectedIndex = 0;      // First item selected when we begin
   updateMenuItems(gEditorUserInterface.mGameType);
   buildGameParamList();   // This will essentially "normalize" our parameter list, making sure that all params are present and in a standard order.
   origGameParams = gameParams;     // Save a copy of the params coming in for comparison when we leave to see if anything changed.
   glutSetCursor(GLUT_CURSOR_NONE);
}


extern const char *gGameTypeNames[];
extern S32 gDefaultGameTypeIndex;
extern S32 gMaxPlayers;

void GameParamUserInterface::updateMenuItems(const char *gt)
{
   // Figure out what index our gametype index is.  This is kind of ugly.  We'll assume that gt has a valid string because if not, game would have crashed already!
   S32 i;
   for(i = 0; gGameTypeNames[i]; i++)
      if(!strcmp(gt, gGameTypeNames[i]))
         break;

   if(!gGameTypeNames[i])    // No match was found, use default
      i = gDefaultGameTypeIndex;

   updateMenuItems(i);
}


// It would be more efficient to simply parse our list and create a lookup table
string GameParamUserInterface::getParamVal(string paramName)
{
   const string delimiters = " \t";       // Spaces or tabs delimit our lines

   for(S32 i = 0; i < gameParams.size(); i++)
   {
      string str = gameParams[i];
      string::size_type lastPos = str.find_first_not_of(delimiters, 0);    // Skip any leading delimiters
      string::size_type pos     = str.find_first_of(delimiters, lastPos);  // Find first "non-delimiter"

      string token = str.substr(lastPos, pos - lastPos);
      lastPos = min(str.find_first_not_of(delimiters, pos), str.size());   // Skip delimiters.  Note the "not_of"
      string val = str.substr(lastPos, str.size() - lastPos);

      if(token == paramName)
         return val;
   }

   return "";
}

static const S32 OPT_GAMETYPE = 0;
static const S32 OPT_FILENAME = 1;
static const S32 OPT_LEVEL_NAME = 2;
static const S32 OPT_LEVEL_DESCR = 3;
static const S32 OPT_CREDITS = 4;
static const S32 OPT_SCRIPT = 5;

static const S32 FIRST_GAME_SPECIFIC_PARAM = 6;

static S32 OPT_GRIDSIZE = OPT_SCRIPT + 1;
static S32 OPT_MIN_PLAYERS = OPT_GRIDSIZE + 1;
static S32 OPT_MAX_PLAYERS = OPT_MIN_PLAYERS + 1;


static void backToEditorCallback(U32 unused)
{
   gGameParamUserInterface.onEscape();
}


// Does the actual work of updating the menu items!
void GameParamUserInterface::updateMenuItems(S32 gtIndex)
{
   TNL::Object *theObject = TNL::Object::create(gGameTypeNames[gtIndex]);          // Instantiate our gameType object
   GameType *gameType = dynamic_cast<GameType*>(theObject);                        // and cast it to GameType

   // Save existing values of the menu items, so we can grab them again if the user selects the same game...
   //if(menuItems.size())
   //{
   //   for(S32 i = 0; i < menuItems.size(); i++)
   //   {
   //      bool found = false;
   //      if(savedMenuItems.size())
   //         for(S32 j = 0; j < savedMenuItems.size(); j++)
   //            if(!strcmp(savedMenuItems[j].getText(), menuItems[i]->getText())) // Found an already saved parameter with the same name... overwrite the saved values with a new ones
   //            {
   //               savedMenuItems[j].setValue(menuItems[i]->getValue());
   //               savedMenuItems[j].setIntValue(menuItems[i]->getIntValue());
   //               //savedMenuItems[j].mLineEditor = dynamic_cast<EditableMenuItem *>(menuItems[i])->getLineEditor();
   //               found = true;
   //               break;
   //            }
   //      if(!found)                                                     // We don't already have an item with this name.  We'd better add this to our list of saved items.
   //         savedMenuItems.push_back(*menuItems[i]);
   //   }
   //}


   menuItems.deleteAndClear();

   Vector<string> gameTypes;

   for(S32 i = 0; gGameTypeNames[i]; i++)
   {
      // The following seems rather wasteful, but this is hardly a performance sensitive area...
      GameType *gameType = dynamic_cast<GameType *>(TNL::Object::create(gGameTypeNames[i]));          // Instantiate our gameType object
      gameTypes.push_back(gameType->getGameTypeString());

      //if(toupper(gameType->getGameTypeString()[0]) == toupper(ascii))      // Compare first letter of game name with what user typed
      //{
      //   menuItems[selectedIndex]->mValI = i;
      //   updateMenuItems(i);
      //   break;
      //}
   }

   menuItems.push_back(new ToggleMenuItem("Game Type:",       
                                          gameTypes,
                                          gtIndex,
                                          true,
                                          NULL,
                                          gameType->getInstructionString(),
                                          KEY_G ));


   string fn = gEditorUserInterface.getLevelFileName();                 // We'll chop off the .level bit below...
   menuItems.push_back(new EditableMenuItem("Filename:",                         // name
                                            fn,                                  // val
                                            "",                                  // empty val
                                            "File where this level is stored",   // help
                                            MAX_FILE_NAME_LEN, 
                                            KEY_D));

   menuItems.push_back(new EditableMenuItem("Level Name:", 
                                            getParamVal("LevelName"), 
                                            "", 
                                            "The level's name -- pick a good one!",  
                                            MAX_GAME_NAME_LEN, 
                                            KEY_N));

   menuItems.push_back(new EditableMenuItem("Level Descr:", 
                                            getParamVal("LevelDescription"), 
                                            "", 
                                            "A brief description of the level",                     
                                            MAX_GAME_DESCR_LEN, 
                                            KEY_D));

   menuItems.push_back(new EditableMenuItem("Level By:", 
                                            getParamVal("LevelCredits"), 
                                            "", 
                                            "Who created this level",                                  
                                            MAX_GAME_DESCR_LEN, 
                                            KEY_B));

   menuItems.push_back(new EditableMenuItem("Levelgen Script:", 
                                            getParamVal("Script"), 
                                            "<None>", 
                                            "Levelgen script & args to be run when level is loaded",  
                                            255, 
                                            KEY_L));
   // Game specific items
   gameType->addGameSpecificParameterMenuItems(menuItems);

   mGameSpecificParams = menuItems.size() - FIRST_GAME_SPECIFIC_PARAM;

   OPT_GRIDSIZE = menuItems.size();
   OPT_MIN_PLAYERS = OPT_GRIDSIZE + 1;
   OPT_MAX_PLAYERS = OPT_MIN_PLAYERS + 1;


   // Some more standard items
   menuItems.push_back(new CounterMenuItem("Grid Size:",       
                                           Game::DefaultGridSize,    // value
                                           Game::MIN_GRID_SIZE,      // increment
                                           Game::MIN_GRID_SIZE,      // min val
                                           Game::MAX_GRID_SIZE,      // max val
                                           "pixels",                 // units
                                           "", 
                                           "\"Magnification factor.\" Larger values lead to larger levels.  Default is 255.",      
                                           KEY_S));

   menuItems.push_back(new CounterMenuItem("Min Players:",       
                                           0,                  // value
                                           1,                  // increment
                                           0,                  // min val
                                           gMaxPlayers,        // max val
                                           "players",          // units
                                           "N/A", 
                                           "Min. players you would recommend for this level (helps server select the next level)",      
                                           KEY_M));

   menuItems.push_back(new CounterMenuItem("Max Players:",       
                                           0,                  // value
                                           1,                  // increment
                                           0,                  // min val
                                           gMaxPlayers,        // max val
                                           "players",          // units
                                           "N/A",
                                           "Max. players you would recommend for this level (helps server select the next level)",      
                                           KEY_S));

   menuItems.push_back(new MenuItem(0, "RETURN TO EDITOR", backToEditorCallback, "", KEY_Q, KEY_R));

   mQuitItemIndex = menuItems.size() - 1;

   // Now populate the menu with values derived from our saved values

   for(S32 i = 0; i < gameParams.size(); i++)
      logprintf("%d == %s", i, gameParams[i].c_str());

   if(gameParams.size())
   {
      const string delimiters = " \t";       // Spaces or tabs will delimit our lines
      for(S32 i = 0; i < gameParams.size(); i++)
      {
         string str = gameParams[i];
         string::size_type lastPos = str.find_first_not_of(delimiters, 0);    // Skip any leading delimiters
         string::size_type pos     = str.find_first_of(delimiters, lastPos);  // Find first "non-delimiter"

         string token = str.substr(lastPos, pos - lastPos);
         lastPos = min(str.find_first_not_of(delimiters, pos), str.size());   // Skip delimiters.  Note the "not_of"
         string val = str.substr(lastPos, str.size() - lastPos);

         if(token == "GridSize")
         {
            S32 gridSize = max(min(atoi(val.c_str()), Game::MAX_GRID_SIZE), Game::MIN_GRID_SIZE);
            menuItems[OPT_GRIDSIZE]->setValue(gridSize);
         }
         else if(token == "MinPlayers")
         {
            S32 minPlayers = max(min(atoi(val.c_str()), gMaxPlayers), 0);
            menuItems[OPT_MIN_PLAYERS]->setValue(minPlayers);
         }
         else if(token == "MaxPlayers")
         {
            S32 maxPlayers = max(min(atoi(val.c_str()), gMaxPlayers), 0);
            menuItems[OPT_MAX_PLAYERS]->setValue(maxPlayers);
         }
      }

      // And apply our GameType arguments to the game specific parameter settings, if any were provided in a level file we loaded
      if(!ignoreGameParams)
         for(S32 i = 0; i < min(gEditorUserInterface.mGameTypeArgs.size(), mGameSpecificParams); i++)
            menuItems[i + FIRST_GAME_SPECIFIC_PARAM]->setValue(gEditorUserInterface.mGameTypeArgs[i]);
   }

   // Lastly, scan through our list of saved items and replace the default values with those modified here in this interface
   // This operates like a poor man's dictionary
   //if(savedMenuItems.size())
   //   for(S32 i = 1; i < menuItems.size(); i++)                    // Start with 1 because we don't want to overwrite our game type, which is #0!
   //      for(S32 j = 0; j < savedMenuItems.size(); j++)
   //         if(!strcmp(savedMenuItems[j].getText(), menuItems[i]->getText()))    // Found a match
   //         {
   //            menuItems[i]->mValI = savedMenuItems[j].mValI;
   //            dynamic_cast<EditableMenuItem *>(menuItems[i])->setLineEditor(savedMenuItems[j].mLineEditor);
   //            break;
   //         }
}


void GameParamUserInterface::idle(U32 timeDelta)
{
   LineEditor::updateCursorBlink(timeDelta);
}


static const U32 itemHeight = 30;
static const U32 yStart = UserInterface::vertMargin + 90;


// Runs as we're exiting the menu
void GameParamUserInterface::onEscape()
{
   S32 gameTypeIndex = dynamic_cast<ToggleMenuItem *>(menuItems[OPT_GAMETYPE])->getValueIndex();
   strcpy(gEditorUserInterface.mGameType, gGameTypeNames[gameTypeIndex]);   // Save current game type

   // Compose GameType string from GameType and level-specific params
   gEditorUserInterface.setLevelFileName(menuItems[OPT_FILENAME]->getValue());  // Save level file name if it changed. Or hell, even if it didn't
   gEditorUserInterface.setLevelGenScriptName(menuItems[OPT_SCRIPT]->getValue());

   gEditorUserInterface.setGridSize(menuItems[OPT_GRIDSIZE]->getIntValue()); 
   buildGameParamList();

   if(anythingChanged())
   {
      gEditorUserInterface.mNeedToSave = true;        // Need to save to retain our changes
      gEditorUserInterface.mAllUndoneUndoLevel = -1;  // This change can't be undone
      ignoreGameParams = false;
      gEditorUserInterface.validateLevel();
   }

   // Now back to our previously scheduled program...  (which will be the editor, of course)
   UserInterface::reactivatePrevUI();
}


// Take the info from our menus and create a list of lines for the level file
void GameParamUserInterface::buildGameParamList()
{
   gEditorUserInterface.mGameTypeArgs.clear();
   gameParams.clear();

   char str[gameTypeLen];
   
   S32 gameTypeIndex = dynamic_cast<ToggleMenuItem *>(menuItems[OPT_GAMETYPE])->getValueIndex();
   strcpy(str, gGameTypeNames[gameTypeIndex]);      // GameType string -- value stored in the LineEditor is a "pretty name".  This looks up the "official" value.

   // Build up GameType string parameter by parameter... all game specific params go on the GameType line
   for(S32 i = FIRST_GAME_SPECIFIC_PARAM; i < FIRST_GAME_SPECIFIC_PARAM + mGameSpecificParams; i++)
   {
      dSprintf(str, sizeof(str), "%s %d", str, menuItems[i]->getIntValue());
      gEditorUserInterface.mGameTypeArgs.push_back(menuItems[i]->getIntValue());  // Save the already-parsed GameType args in a vector for use if we re-enter this interface
   }

   // Compose other game description strings
   gameParams.push_back(str);
   gameParams.push_back("LevelName "        + string(menuItems[OPT_LEVEL_NAME]->getValue()));
   gameParams.push_back("LevelDescription " + string(menuItems[OPT_LEVEL_DESCR]->getValue()));
   gameParams.push_back("LevelCredits "     + string(menuItems[OPT_CREDITS]->getValue()));
   gameParams.push_back("Script "           + string(menuItems[OPT_SCRIPT]->getValue()));
   gameParams.push_back("GridSize "         + string(menuItems[OPT_GRIDSIZE]->getValue()));
   gameParams.push_back("MinPlayers "       + string(menuItems[OPT_MIN_PLAYERS]->getValue()));
   gameParams.push_back("MaxPlayers "       + string(menuItems[OPT_MAX_PLAYERS]->getValue()));
}


// Compare list of parameters from before and after a session in the GameParams menu.  Did anything get changed??
bool GameParamUserInterface::anythingChanged()
{
   if(origGameParams.size() != gameParams.size())
      return true;

   for(S32 i = 0; i < origGameParams.size(); i++)
     if(origGameParams[i] != gameParams[i])
         return true;

   return false;
}


void GameParamUserInterface::onMouseMoved(S32 x, S32 y)
{
   glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);            // Show cursor when user moves mouse

   Point mousePos = convertWindowToCanvasCoord(Point(x, y));

   selectedIndex = (S32)((mousePos.y - yStart + 6) / itemHeight);

   if(selectedIndex >= menuItems.size())
      selectedIndex = menuItems.size() - 1;

   if(selectedIndex < 0)
      selectedIndex = 0;
}


};

