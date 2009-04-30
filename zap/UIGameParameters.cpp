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
GameParamUserInterface::GameParamUserInterface()
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

/*
// From http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
// TODO: This not used, delete?
void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " \t")
{
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);    // Skip delimiters at beginning.
    string::size_type pos     = str.find_first_of(delimiters, lastPos);  // Find first "non-delimiter".

    while (string::npos != pos || string::npos != lastPos)
    {
        tokens.push_back(str.substr(lastPos, pos - lastPos));     // Found a token, add it to the vector.
        lastPos = str.find_first_not_of(delimiters, pos);         // Skip delimiters.  Note the "not_of"
        pos = str.find_first_of(delimiters, lastPos);             // Find next "non-delimiter"
    }
}
*/

extern const char *gGameTypeNames[];
extern S32 gDefaultGameTypeIndex;
extern S32 gMaxPlayers;

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif


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

// Does the actual work of updating the menu items!
void GameParamUserInterface::updateMenuItems(S32 gtIndex)
{
   TNL::Object *theObject = TNL::Object::create(gGameTypeNames[gtIndex]);          // Instantiate our gameType object
   GameType *gameType = dynamic_cast<GameType*>(theObject);                        // and cast it to GameType
   Vector<GameType::ParameterDescription> params = gameType->describeArguments();  // Find out what this game wants

   mGameSpecificParams = params.size();

   // Save existing values of the menu items, so we can grab them again if the user selects the same game...
   if(menuItems.size())
   {
      for(S32 i = 0; i < menuItems.size(); i++)
      {
         bool found = false;
         if(savedMenuItems.size())
            for(S32 j = 0; j < savedMenuItems.size(); j++)
               if(!strcmp(savedMenuItems[j].mText, menuItems[i].mText)) // Found an already saved parameter with the same name... overwrite the saved values with a new ones
               {
                  savedMenuItems[j].mValI = menuItems[i].mValI;
                  savedMenuItems[j].mValS = menuItems[i].mValS;
                  found = true;
                  break;
               }
         if(!found)                                                     // We don't already have an item with this name.  We'd better add this to our list of saved items.
            savedMenuItems.push_back(menuItems[i]);
      }
   }
   menuItems.clear();

   string fn = gEditorUserInterface.getLevelFileName();                 // We'll chop off the .level bit below...
   menuItems.push_back(MenuItem2("Game Type:",    gameType->getGameTypeString(),  gtIndex,    0, 0, "", gameType->getInstructionString(),    TypeGameType,      KEY_G, KEY_UNKNOWN ));
   menuItems.push_back(MenuItem2("Filename:",     fn.substr(0,fn.find_last_of('.')), 0,       0, 0, "", "File this level is stored in",      TypeFileName,      KEY_F, KEY_UNKNOWN ));
   menuItems.push_back(MenuItem2("Level Name:",   "New Bitfighter Level",            0,       0, 0, "", "Choose a short, catchy name",       TypeShortString,   KEY_N, KEY_UNKNOWN ));
   menuItems.push_back(MenuItem2("Level Descr:",  "",                                0,       0, 0, "", "A brief description of the level",  TypeLongString,    KEY_D, KEY_UNKNOWN ));
   menuItems.push_back(MenuItem2("Level By:",     "",                                0,       0, 0, "", "Who created this level",            TypeLongString,    KEY_B, KEY_UNKNOWN ));

   S32 i;
   for(i = 0; i < mGameSpecificParams; i++)
      menuItems.push_back(MenuItem2(params[i].name,  "", params[i].value,  params[i].minval, params[i].maxval,   params[i].units,    params[i].help,   TypeInt,   KEY_UNKNOWN, KEY_UNKNOWN ));

   menuItems.push_back(MenuItem2("Grid Size:",       "", Game::DefaultGridSize, Game::minGridSize, Game::maxGridSize, "pixels",   "\"Magnification factor.\" Larger values lead to larger levels.  Default is 255.",      TypeInt,   KEY_S, KEY_UNKNOWN ));
   menuItems.push_back(MenuItem2("Min Players:",     "", 0,                                      1,      gMaxPlayers, "players",  "Min. players you would recommend for this level (helps server select the next level)", TypeInt,   KEY_M, KEY_UNKNOWN ));
   menuItems.push_back(MenuItem2("Max Players:",     "", 0,                                      1,      gMaxPlayers, "players",  "Max. players you would recommend for this level (helps server select the next level)", TypeInt,   KEY_M, KEY_UNKNOWN ));
   menuItems.push_back(MenuItem2("RETURN TO EDITOR", "", 0,                                      0,                0, "",         "",                                                                                     TypeNone,  KEY_Q, KEY_R       ));
   mQuitItemIndex = i + 8;

   // Now populate the menu with values derived from our saved values

   if(gameParams.size())
   {
      const string delimiters = " \t";       // Spaces or tabs will delimit our lines
      for(i = 0; i < gameParams.size(); i++)
      {
         string str = gameParams[i];
         string::size_type lastPos = str.find_first_not_of(delimiters, 0);    // Skip any leading delimiters
         string::size_type pos     = str.find_first_of(delimiters, lastPos);  // Find first "non-delimiter"

         string token = str.substr(lastPos, pos - lastPos);
         lastPos = min(str.find_first_not_of(delimiters, pos), str.size());   // Skip delimiters.  Note the "not_of"
         string val = str.substr(lastPos, str.size() - lastPos);

         if (token == "LevelName")
            menuItems[2].mValS = val.substr(0, gMaxGameNameLength);
         else if (token == "LevelDescription")
            menuItems[3].mValS = val.substr(0, gMaxGameDescrLength);
         else if (token == "LevelCredits")
            menuItems[4].mValS = val.substr(0, gMaxGameDescrLength);
         else if (token == "GridSize")
            menuItems[mGameSpecificParams + 5].mValI = max(min(atoi(val.c_str()), Game::maxGridSize), Game::minGridSize);
         else if (token == "MinPlayers")
            menuItems[mGameSpecificParams + 6].mValI = max(min(atoi(val.c_str()), gMaxPlayers), 0);
         else if (token == "MaxPlayers")
            menuItems[mGameSpecificParams + 7].mValI = max(min(atoi(val.c_str()), gMaxPlayers), 0);
      }

      // And apply our GameType arguments to the game specific parameter settings, if any were provided in a level file we loaded
      if(!ignoreGameParams)
         for(i = 0; i < min(gEditorUserInterface.mGameTypeArgs.size(), mGameSpecificParams); i++)
            menuItems[i + 5].mValI = gEditorUserInterface.mGameTypeArgs[i];
   }

   // Lastly, scan through our list of saved items and replace the default values with those modified here in this interface
   // This operates like a poor man's dictionary
   if(savedMenuItems.size())
   {
      for(S32 i = 1; i < menuItems.size(); i++)                         // Start with 1 because we don't want to overwrite our game type, which is #0!
      {
         for(S32 j = 0; j < savedMenuItems.size(); j++)
            if(!strcmp(savedMenuItems[j].mText, menuItems[i].mText))    // Found a match
            {
               menuItems[i].mValI = savedMenuItems[j].mValI;
               menuItems[i].mValS = savedMenuItems[j].mValS;
               break;
            }
      }
   }
}

void GameParamUserInterface::idle(U32 timeDelta)
{
   updateCursorBlink(timeDelta);
}

extern void glColor(Color c, float alpha = 1);

void GameParamUserInterface::render()
{
   const U32 fontSize = 15;

   glColor3f(1, 1, 1);
   drawCenteredString(vertMargin, 30, menuTitle);
   drawCenteredString(vertMargin + 35, 18, menuSubTitle);

   glColor3f(1, 1, 1);
   drawCenteredString(canvasHeight - vertMargin - 20, 18, "UP, DOWN to choose | ESC exits menu");

   U32 yStart = vertMargin + 90;

   for(S32 i = 0; i < menuItems.size(); i++)
   {
      U32 y = yStart + i * 30;

      if(selectedIndex == i)           // Highlight selected item
      {
         glColor3f(0, 0, 0.4);         // Fill
         glBegin(GL_POLYGON);
            glVertex2f(0, y - 2);
            glVertex2f(800, y - 2);
            glVertex2f(800, y + fontSize + 8);
            glVertex2f(0, y + fontSize + 8);
         glEnd();

         glColor3f(0, 0, 1);           // Outline
         glBegin(GL_LINES);
            glVertex2f(0, y - 2);
            glVertex2f(799, y - 2);
            glVertex2f(799, y + fontSize + 8);
            glVertex2f(0, y + fontSize + 8);
         glEnd();
      }

      S32 xpos;
      S32 space = getStringWidth(fontSize, " ");
      string dispString = menuItems[i].mValS != "" ? menuItems[i].mValS : "<none>";

      if(menuItems[i].mValType == TypeShortString || menuItems[i].mValType == TypeLongString  ||
            menuItems[i].mValType == TypeGameType || menuItems[i].mValType == TypeFileName)
         xpos = (gScreenWidth - getStringWidth(fontSize, menuItems[i].mText) - getStringWidth(fontSize, dispString.c_str()) - space) / 2;
      else if(menuItems[i].mValType == TypeInt)
         xpos = (gScreenWidth - getStringWidthf(fontSize, "%d %s", menuItems[i].mValI, menuItems[i].mUnits) - getStringWidth(fontSize, menuItems[i].mText) - space) / 2;
      else
         xpos = (gScreenWidth - getStringWidth(fontSize, menuItems[i].mText)) / 2;

      glColor3f(0, 1, 1);
      drawString(xpos, y, fontSize, menuItems[i].mText);

      if(selectedIndex == i)
         glColor3f(1, 0, 0);
      else
         glColor3f(1, 1, 1);


      if(menuItems[i].mValType == TypeShortString || menuItems[i].mValType == TypeLongString || menuItems[i].mValType == TypeGameType || menuItems[i].mValType == TypeFileName)
         drawString(xpos + getStringWidth(fontSize, menuItems[i].mText) + space, y, fontSize, dispString.c_str());
      else if(menuItems[i].mValType == TypeInt)
         drawStringf(xpos + getStringWidth(fontSize, menuItems[i].mText) + space, y, fontSize, "%d %s", menuItems[i].mValI, menuItems[i].mUnits);
      // else draw nothing

      if(selectedIndex == i)     // This is the currently selected item
         // Draw chat cursor
         if((menuItems[i].mValType == TypeShortString || menuItems[i].mValType == TypeLongString || menuItems[i].mValType == TypeFileName) && cursorBlink)
            drawString(xpos + getStringWidth(fontSize, menuItems[i].mText) + space + getStringWidth(fontSize, menuItems[i].mValS.c_str()), y, fontSize, "_");
   }

   // Draw the help string
   glColor3f(0, 1, 0);
   S32 ypos = canvasHeight - vertMargin - 50;

   if(menuItems[selectedIndex].mValType == TypeInt)
   {
      drawCenteredString(ypos, fontSize,  "Use [<-] and [->] keys to change value.  Use [Shift] for bigger change.");
      ypos -= fontSize + 5;
   }

   drawCenteredString(ypos, fontSize, menuItems[selectedIndex].mHelp);
}

// Run this as we're exiting the menu
void GameParamUserInterface::onEscape()
{
   strcpy(gEditorUserInterface.mGameType, gGameTypeNames[menuItems[0].mValI]);   // Save current game type

   // Compose GameType string from GameType and level-specific params
   gEditorUserInterface.setLevelFileName(menuItems[1].mValS);                    // Save level file name, if it changed.  Or hell, even if it didn't
   gEditorUserInterface.mGridSize = menuItems[mGameSpecificParams + 5].mValI;    // Set mGridSize for proper scaling of walls on map
   buildGameParamList();

   if(didAnythingGetChanged())
   {
      gEditorUserInterface.mNeedToSave = true;        // Need to save to retain our changes
      gEditorUserInterface.mAllUndoneUndoLevel = -1;  // This change can't be undone
      ignoreGameParams = false;
      gEditorUserInterface.validateLevel();
   }

   // Now back to our previously scheduled program...  (which will be the editor, of course)
   UserInterface::reactivatePrevUI();
}

// Take the info from our menus and create a list of lines we can stick in a level file without further ado
void GameParamUserInterface::buildGameParamList()
{
   gEditorUserInterface.mGameTypeArgs.clear();
   gameParams.clear();

   char str[gameTypeLen];
   strcpy(str, gGameTypeNames[menuItems[0].mValI]);      // GameType string -- value stored in mValS is a "pretty name".  This looks up the "official" value.

   // Build up GameType string parameter by parameter... all game specific params go on the GameType line
   for(S32 i = 0; i < mGameSpecificParams; i++)
   {
      dSprintf(str, sizeof(str), "%s %d", str, menuItems[i + 5].mValI);
      gEditorUserInterface.mGameTypeArgs.push_back(menuItems[i + 5].mValI);      // Save the already-parsed GameType args in a vector for use if we re-enter this interface
   }

   // Compose other game description strings
   gameParams.push_back(str);
   gameParams.push_back("LevelName " + menuItems[2].mValS);
   gameParams.push_back("LevelDescription " + menuItems[3].mValS);
   gameParams.push_back("LevelCredits " + menuItems[4].mValS);
   dSprintf(str, sizeof(str), "GridSize %d", menuItems[mGameSpecificParams + 5].mValI);
   gameParams.push_back(str);
   dSprintf(str, sizeof(str), "MinPlayers %d", menuItems[mGameSpecificParams + 6].mValI);
   gameParams.push_back(str);
   dSprintf(str, sizeof(str), "MaxPlayers %d", menuItems[mGameSpecificParams + 7].mValI);
   gameParams.push_back(str);
}


// Compare list of parameters from before and after a session in the GameParams menu.  Did anything get changed??
bool GameParamUserInterface::didAnythingGetChanged()
{
   if(origGameParams.size() != gameParams.size())
      return true;

   for(S32 i = 0; i < origGameParams.size(); i++)
     if(origGameParams[i] != gameParams[i])
         return true;

   return false;
}


void GameParamUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if ((selectedIndex != -1 && (menuItems[selectedIndex].mValType == TypeShortString ||  menuItems[selectedIndex].mValType == TypeLongString || 
                                menuItems[selectedIndex].mValType == TypeFileName)) && 
            (keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE))    // Do backspacey things
   {     // (braces required)
      if(menuItems[selectedIndex].mValS.size())
         menuItems[selectedIndex].mValS.erase(menuItems[selectedIndex].mValS.size()-1);
   }

   else if (menuItems[selectedIndex].mValType == TypeInt && (keyCode == KEY_RIGHT))   // Increment value by mMinVal, use shift to go faster!
   {
      S32 inc = getKeyState(KEY_SHIFT) ? menuItems[selectedIndex].mMinVal * 10 : menuItems[selectedIndex].mMinVal;

      if(menuItems[selectedIndex].mValI + inc <= menuItems[selectedIndex].mMaxVal)
         menuItems[selectedIndex].mValI += inc;
      else
         menuItems[selectedIndex].mValI = menuItems[selectedIndex].mMaxVal;
   }
   else if (menuItems[selectedIndex].mValType == TypeInt && (keyCode == KEY_LEFT))    // Decrement value by mMinVal, use shift to go faster!
   {
      S32 inc = getKeyState(KEY_SHIFT) ? menuItems[selectedIndex].mMinVal * 10 : menuItems[selectedIndex].mMinVal;

      if(menuItems[selectedIndex].mValI - inc >= menuItems[selectedIndex].mMinVal)
         menuItems[selectedIndex].mValI -= inc;
      else
         menuItems[selectedIndex].mValI = menuItems[selectedIndex].mMinVal;
   }

   else if (menuItems[selectedIndex].mValType == TypeGameType && (keyCode == KEY_RIGHT))    // Next game type
   {
      menuItems[selectedIndex].mValI++;
      ignoreGameParams = true;              // Game parameters specified in level file no longer make sense if we've changed game type!
      if(!gGameTypeNames[menuItems[selectedIndex].mValI])
         menuItems[selectedIndex].mValI = 0;

      //menuItems[selectedIndex].mValS = gGameTypeNames[menuItems[selectedIndex].mValI];
      updateMenuItems(menuItems[selectedIndex].mValI);
   }
   else if (menuItems[selectedIndex].mValType == TypeGameType && (keyCode == KEY_LEFT))    // Prev game type
   {
      menuItems[selectedIndex].mValI--;
      if(menuItems[selectedIndex].mValI < 0)
      {
         // Find last menuItem string
         S32 indx;
         for(indx = 0; gGameTypeNames[indx]; indx++)
            ;     // Do nothing...
         indx--;
         menuItems[selectedIndex].mValI = indx;
      }
       //menuItems[selectedIndex].mValS = gGameTypeNames[menuItems[selectedIndex].mValI];
       updateMenuItems(menuItems[selectedIndex].mValI);
   }

   else if(keyCode == KEY_ESCAPE || keyCode == BUTTON_BACK || (selectedIndex == mQuitItemIndex && keyCode == KEY_ENTER))       // Esc - Quit
   {
      UserInterface::playBoop();
      onEscape();
   }
   else if(keyCode == KEY_UP || keyCode == BUTTON_DPAD_UP)        // Prev item
   {
      UserInterface::playBoop();
      selectedIndex--;
      if(selectedIndex < 0)
         selectedIndex = menuItems.size() - 1;
   }
   else if(keyCode == KEY_DOWN || keyCode == BUTTON_DPAD_DOWN)    // Next item
   {
      UserInterface::playBoop();
      selectedIndex++;
      if(selectedIndex >= menuItems.size())
         selectedIndex = 0;
   }
   else if(keyCode == keyOUTGAMECHAT)     // Turn on Global Chat overlay
   {
      UserInterface::playBoop();
      gChatInterface.activate();
   }

   else if((menuItems[selectedIndex].mValType == TypeShortString || menuItems[selectedIndex].mValType == TypeLongString) && ascii)
   {
      // gMaxGameDescrLength is about the max number of characters you can display on the screen.  It's not an inherent limit.
      if(menuItems[selectedIndex].mValS.size() < (menuItems[selectedIndex].mValType == TypeShortString ? gMaxGameNameLength : gMaxGameDescrLength))
         menuItems[selectedIndex].mValS += char(ascii);
   }

   else if(menuItems[selectedIndex].mValType == TypeFileName && gLevelNameEntryUserInterface.isValid(ascii))
   {
      if(menuItems[selectedIndex].mValS.size() < MAX_SHORT_TEXT_LEN)  // MAX_SHORT_TEXT_LEN is the length we're restricting file names to, for somewhat arbitrary reasons.
         menuItems[selectedIndex].mValS += char(ascii);
   }
}

};
