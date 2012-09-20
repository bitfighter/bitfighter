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
#include "UIKeyDefMenu.h"
#include "UIChat.h"
#include "UIDiagnostics.h"
#include "UIGame.h"
#include "input.h"
#include "InputCode.h"
#include "IniFile.h"
#include "config.h"
#include "Colors.h"
#include "ScreenInfo.h"
#include "JoystickRender.h"
#include "ClientGame.h"
#include "Cursor.h"

#include "SDL.h"
#include "OpenglUtils.h"

#include <string>
#include <math.h>


namespace Zap
{

// Constructor
KeyDefMenuItem::KeyDefMenuItem(const char *text, U32 index, U32 col, InputCodeManager::BindingName PC, string helpStr)
{
   mText = text;
   mIndex = index;
   mColumn = col;
   primaryControl = PC;
   helpString = helpStr;
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
KeyDefMenuUserInterface::KeyDefMenuUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(KeyDefUI);
   mMenuTitle = "Define Keys";
   mMenuSubTitle = "";
   mMenuFooter = "UP, DOWN, LEFT, RIGHT to choose | ENTER to select | ESC exits menu";
}


// Some constants used for positioning menu items and coordinating mouse position
static S32 offset = 5; 
static S32 yStart = UserInterface::vertMargin + 115;
static S32 height = 30; 
static S32 firstItemInCol2 = 0;     // Set later


void KeyDefMenuUserInterface::onActivate()
{
   mDisableShipKeyboardInput = true;      // Keep keystrokes from getting to game
   selectedIndex = 0;                     // First item selected when we begin
   changingItem = -1;                     // Not changing anything at the moment...

   while(menuItems.size())                // Clear list, but for some reason .clear() method won't compile
      menuItems.pop_back();

   // Display an intitial message to users
   errorMsgTimer.reset(errorMsgDisplayTime);
   errorMsg = "";

   GameSettings *settings = getGame()->getSettings();
   InputCodeManager *inputCodeManager = settings->getInputCodeManager();
   InputMode inputMode = inputCodeManager->getInputMode();

   if(inputMode == InputModeJoystick)
      mMenuTitle = "Define Keys: [Joystick]";
   else
      mMenuTitle = "Define Keys: [Keyboard]";

   mMenuSubTitleColor = Colors::white;   

   if(inputMode == InputModeJoystick)
   {
      itemsPerCol = 7;           // Approx. half of the items we have

      // Col 1
      menuItems.push_back(KeyDefMenuItem("Advance Weapon",        0, 1, InputCodeManager::BINDING_ADVWEAP, 
                                         "Toggles your weapons, use as an alternative to Select Weapon commands"));
      menuItems.push_back(KeyDefMenuItem("Advance Weapon",        1, 1, InputCodeManager::BINDING_ADVWEAP2, 
                                         "Toggles your weapons, use as an alternative to Select Weapon commands"));
      menuItems.push_back(KeyDefMenuItem("Previous Weapon",       2, 1, InputCodeManager::BINDING_PREVWEAP, 
                                         "Toggles your weapons, use as an alternative to Select Weapon commands"));
      menuItems.push_back(KeyDefMenuItem("Activate Module 1",     3, 1, InputCodeManager::BINDING_MOD1, 
                                         "Module 1 will be active while this key/button is held down"));
      menuItems.push_back(KeyDefMenuItem("Activate Module 2",     4, 1, InputCodeManager::BINDING_MOD2, 
                                         "Module 2 will be active while this key/button is held down"));
      menuItems.push_back(KeyDefMenuItem("Drop Flag",             5, 1, InputCodeManager::BINDING_DROPITEM, ""));

      menuItems.push_back(KeyDefMenuItem("Configure Ship Loadouts", 6, 1, InputCodeManager::BINDING_LOADOUT, ""));
      menuItems.push_back(KeyDefMenuItem("Toggle Map Mode",       7, 1, InputCodeManager::BINDING_CMDRMAP, ""));
      menuItems.push_back(KeyDefMenuItem("Show Scoreboard",       8, 1, InputCodeManager::BINDING_SCRBRD,  
                                         "Scoreboard will be visible while this key/button is held down"));
      // Col 2
      firstItemInCol2 = 9;
      menuItems.push_back(KeyDefMenuItem("Select Weapon 1",       9, 2, InputCodeManager::BINDING_SELWEAP1, 
                                         "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Select Weapon 2",      10, 2, InputCodeManager::BINDING_SELWEAP2, 
                                         "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Select Weapon 3",      11, 2, InputCodeManager::BINDING_SELWEAP3, 
                                         "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Quick Chat",           12, 2, InputCodeManager::BINDING_QUICKCHAT, ""));
      menuItems.push_back(KeyDefMenuItem("Team Chat",            13, 2, InputCodeManager::BINDING_TEAMCHAT,  ""));
      menuItems.push_back(KeyDefMenuItem("Global Chat",          14, 2, InputCodeManager::BINDING_GLOBCHAT,  ""));
      menuItems.push_back(KeyDefMenuItem("Enter Command",        15, 2, InputCodeManager::BINDING_CMDCHAT,   ""));
      menuItems.push_back(KeyDefMenuItem("Record Voice Msg",     16, 2, InputCodeManager::BINDING_TOGVOICE,  ""));
   }
   else     // Keyboard mode
   {
      itemsPerCol = 9;           // Approx. half of the items we have

      // Col 1
      menuItems.push_back(KeyDefMenuItem("Ship Up",           0, 1, InputCodeManager::BINDING_UP, ""));
      menuItems.push_back(KeyDefMenuItem("Ship Down",         1, 1, InputCodeManager::BINDING_DOWN, ""));
      menuItems.push_back(KeyDefMenuItem("Ship Left",         2, 1, InputCodeManager::BINDING_LEFT, ""));
      menuItems.push_back(KeyDefMenuItem("Ship Right",        3, 1, InputCodeManager::BINDING_RIGHT,""));
      menuItems.push_back(KeyDefMenuItem("Fire",              4, 1, InputCodeManager::BINDING_FIRE, 
                                         "The mouse will always be used to aim your ship"));
      menuItems.push_back(KeyDefMenuItem("Activate Module 1", 5, 1, InputCodeManager::BINDING_MOD1, 
                                         "Module 1 will be active while this key/button is held down"));
      menuItems.push_back(KeyDefMenuItem("Activate Module 2", 6, 1, InputCodeManager::BINDING_MOD2, 
                                         "Module 2 will be active while this key/button is held down"));
      menuItems.push_back(KeyDefMenuItem("Drop Flag",         7, 1, InputCodeManager::BINDING_DROPITEM, 
                                         "Drop flag when this key is pressed"));
      menuItems.push_back(KeyDefMenuItem("Configure Ship Loadouts", 8, 1, InputCodeManager::BINDING_LOADOUT,   ""));
      menuItems.push_back(KeyDefMenuItem("Toggle Map Mode",   9, 1, InputCodeManager::BINDING_CMDRMAP, ""));
      menuItems.push_back(KeyDefMenuItem("Show Scoreboard",   10, 1, InputCodeManager::BINDING_SCRBRD, 
                                         "Scoreboard will be visible while this key/button is held down"));

      // Col 2
      firstItemInCol2 = 11;
      menuItems.push_back(KeyDefMenuItem("Select Weapon 1",         11, 2, InputCodeManager::BINDING_SELWEAP1, 
                                         "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Select Weapon 2",         12, 2, InputCodeManager::BINDING_SELWEAP2, 
                                         "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Select Weapon 3",         13, 2, InputCodeManager::BINDING_SELWEAP3, 
                                         "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Advance Weapon",          14, 2, InputCodeManager::BINDING_ADVWEAP, 
                                         "Toggles your weapons, use as an alternative to Select Weapon commands"));
      menuItems.push_back(KeyDefMenuItem("Advance Weapon",          15, 2, InputCodeManager::BINDING_ADVWEAP2, 
                                         "Toggles your weapons, use as an alternative to Select Weapon commands"));
      menuItems.push_back(KeyDefMenuItem("Previous Weapon",         16, 2, InputCodeManager::BINDING_PREVWEAP, 
                                         "Toggles your weapons, use as an alternative to Select Weapon commands"));
      menuItems.push_back(KeyDefMenuItem("Quick Chat",              17, 2, InputCodeManager::BINDING_QUICKCHAT, ""));
      menuItems.push_back(KeyDefMenuItem("Team Chat",               18, 2, InputCodeManager::BINDING_TEAMCHAT,  ""));
      menuItems.push_back(KeyDefMenuItem("Global Chat",             19, 2, InputCodeManager::BINDING_GLOBCHAT,  ""));
      menuItems.push_back(KeyDefMenuItem("Enter Command",           20, 2, InputCodeManager::BINDING_CMDCHAT,   ""));

      menuItems.push_back(KeyDefMenuItem("Record Voice Msg",        21, 2, InputCodeManager::BINDING_TOGVOICE,  ""));
   }
}


void KeyDefMenuUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);
   errorMsgTimer.update(timeDelta);
}


// Finds out if key is already assigned to something else
bool KeyDefMenuUserInterface::isDuplicate(S32 key, const Vector<KeyDefMenuItem> &menuItems)
{
   S32 size = menuItems.size();
   S32 count = 0;

   GameSettings *settings = getGame()->getSettings();

   InputCode targetInputCode = getInputCode(settings, menuItems[key].primaryControl);

   for(S32 i = 0; i < size && count < 2; i++)
      if(getInputCode(settings, menuItems[i].primaryControl) == targetInputCode)
         count++;

   return count >= 2;
}


void KeyDefMenuUserInterface::render()
{
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   // Draw the game screen, then dim it out so you can still see it under our overlay
   if(getGame()->getConnectionToServer())
   {
      getUIManager()->getGameUserInterface()->render();
      dimUnderlyingUI();
   }

   glColor(Colors::white);
   drawCenteredString(vertMargin, 30, mMenuTitle);
   drawCenteredString(vertMargin + 35, 18, mMenuSubTitle);

   glColor(Colors::menuHelpColor);
   drawCenteredString(vertMargin + 63, 14, "You can define different keys for keyboard or joystick mode.  Switch in Options menu.");

   glColor(Colors::white);
   drawCenteredString(gScreenInfo.getGameCanvasHeight() - vertMargin - 20, 18, mMenuFooter);

   if(selectedIndex >= menuItems.size())
      selectedIndex = 0;

   S32 size = menuItems.size();

   for(S32 i = 0; i < size; i++)
   {
      S32 y = yStart + (i - ((i < firstItemInCol2) ? 0 : firstItemInCol2)) * height;

		S32 Column_Width = canvasWidth / 2 - horizMargin;
		S32 xPos = (menuItems[i].mColumn - 1) * Column_Width + horizMargin * 2;

      if(selectedIndex == i)       // Highlight selected item
         drawFilledRect(xPos - horizMargin, y, 
                        xPos + Column_Width - horizMargin, y + height + 1, 
                        Colors::blue40, Colors::blue);

      // Draw item text
      glColor(Colors::cyan);
      drawString(xPos, y + offset, 15, menuItems[i].mText);

		xPos += Column_Width * 11 / 20;

      if(changingItem == i)
      {
         glColor(Colors::red);
         drawString(xPos, y + offset + 1, 13, "Press Key or Button");
      }
      else
      {
         bool dupe = isDuplicate(i, menuItems); 
         glColor(dupe ? Colors::red : Colors::white);

         JoystickRender::renderControllerButton(xPos, F32(y + offset), 
                               Joystick::SelectedPresetIndex, getInputCode(getGame()->getSettings(), menuItems[i].primaryControl), dupe);
      }
   }
   

   // Draw some suggestions
   glColor(Colors::yellow);
   if(getGame()->getSettings()->getInputCodeManager()->getInputMode() == InputModeJoystick)
      drawCenteredString(canvasHeight - vertMargin - 90, 15, "HINT: You will be using the left joystick to steer, the right to fire");
   else 
      drawCenteredString(canvasHeight - vertMargin - 90, 15, "HINT: You will be using the mouse to aim, so make good use of your mouse buttons");


   // Draw the help string
   glColor(Colors::green);
   drawCenteredString(canvasHeight - vertMargin - 110, 15, menuItems[selectedIndex].helpString.c_str());

   if(errorMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if (errorMsgTimer.getCurrent() < 1000)
         alpha = (F32) errorMsgTimer.getCurrent() / 1000;

      TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

      glColor(Colors::red, alpha);
      drawCenteredString(canvasHeight - vertMargin - 65, 15, errorMsg.c_str());
   }
}


bool KeyDefMenuUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode)) { /* Do nothing */ }

   // InputCode entry
   else if(changingItem > -1)
   {
      playBoop();

      if(inputCode == KEY_ESCAPE || inputCode == BUTTON_BACK)
      {
         changingItem = -1;
         return true;
      }

      // Check for reserved keys (F1-F12, Ctrl, Esc, Button_Back)
      if(inputCode >= KEY_F1 && inputCode <= KEY_F12)
      {
         errorMsgTimer.reset(errorMsgDisplayTime);
         errorMsg = "Keys F1 - F12 are reserved.  You cannot redefine them.  Sorry!";
         return true;
      }
      else if(inputCode == KEY_CTRL)
      {
         errorMsgTimer.reset(errorMsgDisplayTime);
         errorMsg = "Control key is reserved.  You cannot use it for binding.  Sorry!";
         return true;
      }

      // Fail silently on joystick motion
      if(inputCode >= STICK_1_LEFT && inputCode <= STICK_2_DOWN)
         return true;

      // Assign key
      setInputCode(getGame()->getSettings(), menuItems[changingItem].primaryControl, inputCode);
      changingItem = -1;

      if(inputCode == KEY_CTRL)
      {
         errorMsgTimer.reset(errorMsgDisplayTime);
         errorMsg = "Be careful when using the Ctrl key -- some keys will not work when Ctrl is pressed";
      }
      return true;
   }

   // We're not doing InputCode entry, so let's try menu navigation
   if(inputCode == KEY_SPACE || inputCode == KEY_ENTER || inputCode == BUTTON_START || inputCode == MOUSE_LEFT)      // Set key for selected item
   {
      playBoop();
      changingItem = selectedIndex;
   }
   else if(inputCode == KEY_RIGHT  || inputCode == BUTTON_DPAD_RIGHT || inputCode == KEY_LEFT || inputCode == BUTTON_DPAD_LEFT)    // Change col
   {
      playBoop();

      if(menuItems[selectedIndex].mColumn == 1)    // Switching to right column
         selectedIndex += firstItemInCol2;
      else                                         // Switching to left column
      {
         selectedIndex -= firstItemInCol2;
         while(menuItems[selectedIndex].mColumn == 2)
            selectedIndex--;
      }

      Cursor::disableCursor();                    // Turn off cursor
   }
   else if(inputCode == KEY_ESCAPE || inputCode == BUTTON_BACK)   // Quit
   {
      playBoop();
      saveSettingsToINI(&gINI, getGame()->getSettings());

      getUIManager()->reactivatePrevUI();      // to gOptionsMenuUserInterface
   }
   else if(inputCode == KEY_UP || inputCode == BUTTON_DPAD_UP)    // Prev item
   {
      playBoop();

      selectedIndex--;
      if(selectedIndex < 0)
         selectedIndex = menuItems.size() - 1;

      Cursor::disableCursor();    // Turn off cursor
   }
   else if(inputCode == KEY_DOWN || inputCode == BUTTON_DPAD_DOWN)   // Next item
   {
      playBoop();

      selectedIndex++;
      if(selectedIndex >= menuItems.size())
         selectedIndex = 0;

      Cursor::disableCursor();                       // Turn off cursor
   }
   else              // No key has been handled
      return false;

   // A key was handled
   return true;
}


// Handle mouse input, figure out which menu item we're over, and highlight it
void KeyDefMenuUserInterface::onMouseMoved()
{
   Parent::onMouseMoved();

   Cursor::enableCursor();  // Show cursor when user moves mouse

   const Point *mousePos = gScreenInfo.getMousePos();

   // Which column is the mouse in?  Left half of screen = 0, right half = 1
   S32 col = (mousePos->x < (gScreenInfo.getGameCanvasWidth() - horizMargin) / 2) ? 0 : 1;
   S32 row = min(max(static_cast<int>(floor(( mousePos->y - yStart ) / height)), 0), menuItems.size() - 1);

   selectedIndex = min(max(row + firstItemInCol2 * col, 0), menuItems.size() - 1);    // Bounds checking

   // If we're in the wrong column, get back to the right one.  This can happen if there are more
   // items in the right column than the left.  Note that our column numbering scheme is different
   // between col and the mColumn field.  Also corrects for that dummy null item in the joystick
   // section of the controls.
   if(col == 0)
      while(menuItems[selectedIndex].mColumn == 2)
         selectedIndex--;
   else
      while(menuItems[selectedIndex].mColumn == 1)
         selectedIndex++;
}


};


