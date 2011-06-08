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
#include "keyCode.h"
#include "IniFile.h"
#include "config.h"
#include "game.h"    // For gClientGame
#include "Colors.h"

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"

#include <string>
#include <math.h>


namespace Zap
{

KeyDefMenuUserInterface gKeyDefMenuUserInterface;

extern bool gDisableShipKeyboardInput;
extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;


// Constructor
KeyDefMenuUserInterface::KeyDefMenuUserInterface()
{
   setMenuID(KeyDefUI);
   mMenuTitle = "Define Keys";
   mMenuFooter = "UP, DOWN, LEFT, RIGHT to choose | ENTER to select | ESC exits menu";
}


// Some constants used for positioning menu items and coordinating mouse position
static S32 offset = 5; 
static S32 yStart = UserInterface::vertMargin + 115;
static S32 height = 30; 
static S32 firstItemInCol2 = 0;     // Set later


void KeyDefMenuUserInterface::onActivate()
{
   gDisableShipKeyboardInput = true;      // Keep keystrokes from getting to game
   selectedIndex = 0;                     // First item selected when we begin
   changingItem = -1;                     // Not changing anything at the moment...

   while(menuItems.size())                // Clear list, but for some reason .clear() method won't compile
      menuItems.pop_back();

   // Display an intitial message to users
   errorMsgTimer.reset(errorMsgDisplayTime);
   errorMsg = "";

   if(gIniSettings.inputMode == Joystick)
      mMenuSubTitle = "Input Mode: Joystick";
   else
      mMenuSubTitle = "Input Mode: Keyboard";

   mMenuSubTitleColor = Colors::white;   // white

   if (gIniSettings.inputMode == Joystick)
   {
      itemsPerCol = 7;           // Approx. half of the items we have

      // Col 1
      menuItems.push_back(KeyDefMenuItem("Advance Weapon", 0, 1, &keyADVWEAP[Joystick], "Toggles your weapons, use as an alternative to Select Weapon commands"));
      menuItems.push_back(KeyDefMenuItem("Activate Module 1", 1, 1, &keyMOD1[Joystick], "Module 1 will be active while this key/button is held down"));
      menuItems.push_back(KeyDefMenuItem("Activate Module 2", 2, 1, &keyMOD2[Joystick], "Module 2 will be active while this key/button is held down"));
      menuItems.push_back(KeyDefMenuItem("Drop Flag", 2, 1, &keyDROPITEM[Joystick], ""));

      menuItems.push_back(KeyDefMenuItem("Config. Ship Loadouts", 3, 1, &keyLOADOUT[Joystick], ""));
      menuItems.push_back(KeyDefMenuItem("Toggle Map Mode", 4, 1, &keyCMDRMAP[Joystick], ""));
      menuItems.push_back(KeyDefMenuItem("Show Scoreboard", 5, 1, &keySCRBRD[Joystick], "Scoreboard will be visible while this key/button is held down"));

      // Col 2
      firstItemInCol2 = 7;
      menuItems.push_back(KeyDefMenuItem("Select Weapon 1", 6, 2, &keySELWEAP1[Joystick], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Select Weapon 2", 7, 2, &keySELWEAP2[Joystick], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Select Weapon 3", 8, 2, &keySELWEAP3[Joystick], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Quick Chat", 9, 2, &keyQUICKCHAT[Joystick], ""));
      menuItems.push_back(KeyDefMenuItem("Team Chat", 10, 2, &keyTEAMCHAT[Joystick], ""));
      menuItems.push_back(KeyDefMenuItem("Global Chat", 11, 2, &keyGLOBCHAT[Joystick], ""));
      menuItems.push_back(KeyDefMenuItem("Enter Command", 12, 2, &keyCMDCHAT[Joystick], ""));
      menuItems.push_back(KeyDefMenuItem("Record Voice Msg", 13, 2, &keyTOGVOICE[Joystick], ""));
   }
   else     // Keyboard mode
   {
      itemsPerCol = 9;           // Approx. half of the items we have

      // Col 1
      menuItems.push_back(KeyDefMenuItem("Ship Up", 0, 1, &keyUP[Keyboard], ""));
      menuItems.push_back(KeyDefMenuItem("Ship Down", 1, 1, &keyDOWN[Keyboard], ""));
      menuItems.push_back(KeyDefMenuItem("Ship Left", 2, 1, &keyLEFT[Keyboard], ""));
      menuItems.push_back(KeyDefMenuItem("Ship Right", 3, 1, &keyRIGHT[Keyboard], ""));
      menuItems.push_back(KeyDefMenuItem("Fire", 4, 1, &keyFIRE[Keyboard], "The mouse will always be used to aim your ship"));
      menuItems.push_back(KeyDefMenuItem("Activate Module 1", 5, 1, &keyMOD1[Keyboard], "Module 1 will be active while this key/button is held down"));
      menuItems.push_back(KeyDefMenuItem("Activate Module 2", 6, 1, &keyMOD2[Keyboard], "Module 2 will be active while this key/button is held down"));
      menuItems.push_back(KeyDefMenuItem("Drop Flag", 2, 1, &keyDROPITEM[Keyboard], "Drop flag when this key is pressed"));
      menuItems.push_back(KeyDefMenuItem("Toggle Map Mode", 7, 1, &keyCMDRMAP[Keyboard], ""));
      menuItems.push_back(KeyDefMenuItem("Show Scoreboard", 8, 1, &keySCRBRD[Keyboard], "Scoreboard will be visible while this key/button is held down"));

      // Col 2
      firstItemInCol2 = 10;
      menuItems.push_back(KeyDefMenuItem("Select Weapon 1", 9, 2, &keySELWEAP1[Keyboard], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Select Weapon 2", 10, 2, &keySELWEAP2[Keyboard], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Select Weapon 3", 11, 2, &keySELWEAP3[Keyboard], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(KeyDefMenuItem("Advance Weapon", 12, 2, &keyADVWEAP[Keyboard], "Toggles your weapons, use as an alternative to Select Weapon commands"));
      menuItems.push_back(KeyDefMenuItem("Configure Ship Loadouts", 13, 2, &keyLOADOUT[Keyboard], ""));
      menuItems.push_back(KeyDefMenuItem("Quick Chat", 14, 2, &keyQUICKCHAT[Keyboard], ""));
      menuItems.push_back(KeyDefMenuItem("Team Chat", 15, 2, &keyTEAMCHAT[Keyboard], ""));
      menuItems.push_back(KeyDefMenuItem("Global Chat", 16, 2, &keyGLOBCHAT[Keyboard], ""));
      menuItems.push_back(KeyDefMenuItem("Enter Command", 17, 2, &keyCMDCHAT[Keyboard], ""));

      menuItems.push_back(KeyDefMenuItem("Record Voice Msg", 18, 2, &keyTOGVOICE[Keyboard], ""));
   }
}

void KeyDefMenuUserInterface::idle(U32 timeDelta)
{
   errorMsgTimer.update(timeDelta);
}

// Finds out if key is already assigned to something else
inline bool isDuplicate(S32 key)
{
   S32 size = gKeyDefMenuUserInterface.menuItems.size();
   S32 count = 0;

   for(S32 i = 0; i < size && count < 2; i++)
      if(*gKeyDefMenuUserInterface.menuItems[i].primaryControl == *gKeyDefMenuUserInterface.menuItems[key].primaryControl)
         count++;

   return count >= 2;
}


void KeyDefMenuUserInterface::render()
{
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   // Draw the game screen, then dim it out so you can still see it under our overlay
   if(gClientGame->getConnectionToServer())
   {
      gClientGame->mGameUserInterface->render();
      glColor4f(0, 0, 0, 0.6);
      glEnableBlend;
      glBegin(GL_POLYGON);
         glVertex2f(0, 0);
         glVertex2f(canvasWidth, 0);
         glVertex2f(canvasWidth, gScreenInfo.getGameCanvasHeight());
         glVertex2f(0, canvasHeight);
      glEnd();
      glDisableBlend;
   }

   glColor3f(1, 1, 1);
   drawCenteredString(vertMargin, 30, mMenuTitle);
   drawCenteredString(vertMargin + 35, 18, mMenuSubTitle);
   glColor3f(0, 1, 0);
   drawCenteredString(vertMargin + 63, 14, "You can define different keys for keyboard or joystick mode.  Switch in Options menu.");
   glColor3f(1, 1, 1);

   drawCenteredString(gScreenInfo.getGameCanvasHeight() - vertMargin - 20, 18, mMenuFooter);

   if(selectedIndex >= menuItems.size())
      selectedIndex = 0;

   S32 size = menuItems.size();

   for(S32 i = 0; i < size; i++)
   {
      S32 y = yStart + (i - ((i < firstItemInCol2) ? 0 : firstItemInCol2)) * height;

      if(selectedIndex == i)       // Highlight selected item
      {
         glColor3f(0, 0, 0.4);     // Fill
         glBegin(GL_POLYGON);
            glVertex2f((menuItems[i].mColumn == 1 ? horizMargin : canvasWidth / 2) , y);
            glVertex2f((menuItems[i].mColumn == 1 ? canvasWidth / 2 - horizMargin: canvasWidth - horizMargin), y);
            glVertex2f((menuItems[i].mColumn == 1 ? canvasWidth / 2 - horizMargin: canvasWidth - horizMargin), y + height+ 1);
            glVertex2f((menuItems[i].mColumn == 1 ? horizMargin : canvasWidth / 2), y + height + 1);
         glEnd();

         glColor3f(0, 0, 1);       // Outline
         glBegin(GL_LINE_LOOP);
            glVertex2f((menuItems[i].mColumn == 1 ? horizMargin : canvasWidth / 2), y);
            glVertex2f((menuItems[i].mColumn == 1 ? canvasWidth / 2 - horizMargin : canvasWidth - horizMargin), y);
            glVertex2f((menuItems[i].mColumn == 1 ? canvasWidth / 2 - horizMargin : canvasWidth - horizMargin), y + height + 1);
            glVertex2f((menuItems[i].mColumn == 1 ? horizMargin : canvasWidth / 2 ), y + height + 1);
         glEnd();
      }

      // Draw item text
      glColor3f(0, 1, 1);
      drawString((menuItems[i].mColumn == 1 ? 2 * horizMargin : canvasWidth / 2 + horizMargin), y + offset, 15, menuItems[i].mText);

      if(changingItem == i)
      {
         glColor3f(1, 0, 0);
         drawString((S32)(canvasWidth * (menuItems[i].mColumn == 1 ? 0.25 : 0.75)) + horizMargin, y + offset + 1, 13, "Press Key or Button");
      }
      else
      {
         bool dupe = isDuplicate(i); 
         if(dupe)
            glColor3f(1,0,0);
         else
            glColor3f(1, 1, 1);

         renderControllerButton((S32)(canvasWidth * (menuItems[i].mColumn == 1 ? 0.25 : 0.75)) + horizMargin, y + offset, *menuItems[i].primaryControl, dupe, 10);
      }
   }
   

   // Draw some suggestions
   glColor3f(1, 1, 0);
   if(gIniSettings.inputMode == Joystick)
      drawCenteredString(canvasHeight - vertMargin - 90, 15, "HINT: You will be using the left joystick to steer, the right to fire");
   else 
      drawCenteredString(canvasHeight - vertMargin - 90, 15, "HINT: You will be using the mouse to aim, so make good use of your mouse buttons");


   // Draw the help string
   glColor3f(0, 1, 0);
   drawCenteredString(canvasHeight - vertMargin - 110, 15, menuItems[selectedIndex].helpString.c_str());

   if(errorMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if (errorMsgTimer.getCurrent() < 1000)
         alpha = (F32) errorMsgTimer.getCurrent() / 1000;

      glEnableBlend;
      glColor4f(1, 0, 0, alpha);
      drawCenteredString(canvasHeight - vertMargin - 65, 15, errorMsg.c_str());
      glDisableBlend;
   }
}


void KeyDefMenuUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   // KeyCode entry
   if(changingItem > -1)
   {
      UserInterface::playBoop();

      if(keyCode == KEY_ESCAPE || keyCode == BUTTON_BACK)
      {
         changingItem = -1;
         return;
      }

      // Check for reserved keys (F1-F12, Ctrl, Esc, Button_Back)
      if(keyCode >= KEY_F1 && keyCode <= KEY_F12)
      {
         errorMsgTimer.reset(errorMsgDisplayTime);
         errorMsg = "Keys F1 - F12 are reserved.  You cannot redefine them.  Sorry!";
         return;
      }
      else if(keyCode == KEY_CTRL)
      {
         errorMsgTimer.reset(errorMsgDisplayTime);
         errorMsg = "Control key is reserved.  You cannot use it for binding.  Sorry!";
         return;
      }


      // Fail silently on joystick motion
      if(keyCode >= STICK_1_LEFT && keyCode <= STICK_2_DOWN)
         return;

      // Assign key
      *menuItems[changingItem].primaryControl = keyCode;
      changingItem = -1;

      if(keyCode == KEY_CTRL)
      {
         errorMsgTimer.reset(errorMsgDisplayTime);
         errorMsg = "Be careful when using the Ctrl key -- some keys will not work when Ctrl is pressed";
      }
      return;
   }

   // We're not doing KeyCode entry, so let's try menu navigation
   if(keyCode == KEY_SPACE || keyCode == KEY_ENTER || keyCode == BUTTON_START || keyCode == MOUSE_LEFT)      // Set key for selected item
   {
      UserInterface::playBoop();
      changingItem = selectedIndex;
   }
   else if(keyCode == KEY_RIGHT  || keyCode == BUTTON_DPAD_RIGHT || keyCode == KEY_LEFT || keyCode == BUTTON_DPAD_LEFT)    // Change col
   {
      UserInterface::playBoop();

      if(menuItems[selectedIndex].mColumn == 1)    // Switching to right column
         selectedIndex += firstItemInCol2;
      else                                         // Switching to left column
      {
         selectedIndex -= firstItemInCol2;
         while(menuItems[selectedIndex].mColumn == 2)
            selectedIndex--;
      }

      SDL_ShowCursor(SDL_DISABLE);    // Turn off cursor
   }
   else if(keyCode == KEY_ESCAPE || keyCode == BUTTON_BACK)       // Quit
   {
      UserInterface::playBoop();
      saveSettingsToINI();

      UserInterface::reactivatePrevUI();      //gOptionsMenuUserInterface
   }
   else if(keyCode == KEY_UP || keyCode == BUTTON_DPAD_UP)        // Prev item
   {
      UserInterface::playBoop();

      selectedIndex--;
      if(selectedIndex < 0)
         selectedIndex = menuItems.size() - 1;

      SDL_ShowCursor(SDL_DISABLE);    // Turn off cursor
   }
   else if(keyCode == KEY_DOWN || keyCode == BUTTON_DPAD_DOWN)    // Next item
   {
      UserInterface::playBoop();

      selectedIndex++;
      if(selectedIndex >= menuItems.size())
         selectedIndex = 0;

      SDL_ShowCursor(SDL_DISABLE);    // Turn off cursor
   }
   else if(keyCode == keyOUTGAMECHAT)     // Turn on Global Chat overlay
   {
      UserInterface::playBoop();
      gChatInterface.activate();
   }

//   else if(keyCode == keyDIAG)            // Turn on diagnostic overlay
//   {
//      gDiagnosticInterface.activate();
//      UserInterface::playBoop();
//   }
}


// Handle mouse input, figure out which menu item we're over, and highlight it
void KeyDefMenuUserInterface::onMouseMoved(S32 x, S32 y)
{
   SDL_ShowCursor(SDL_ENABLE);  // TODO:  was GLUT_CURSOR_RIGHT_ARROW  // Show cursor when user moves mouse

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


