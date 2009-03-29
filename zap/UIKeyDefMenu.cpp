//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#include "../glut/glutInclude.h"
#include <string>


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
   menuTitle = "Define Keys";
   menuFooter = "UP, DOWN, LEFT, RIGHT to choose | ENTER to select | ESC exits menu";
}

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
      menuSubTitle = "Input Mode: Joystick";
   else
      menuSubTitle = "Input Mode: Keyboard";

   menuSubTitleColor = Color(1,1,1);

   if (gIniSettings.inputMode == Joystick)
   {
      itemsPerCol = 7;           // Approx. half of the items we have

      // Col 1
      menuItems.push_back(MenuItemExtended("Advance Weapon", 0, 1, &keyADVWEAP[Joystick], "Toggles your weapons, use as an alternative to Select Weapon commands"));
      menuItems.push_back(MenuItemExtended("Activate Module 1", 1, 1, &keyMOD1[Joystick], "Module will be active while this key/button is held down"));
      menuItems.push_back(MenuItemExtended("Activate Module 2", 2, 1, &keyMOD2[Joystick], "Module will be active while this key/button is held down"));
      menuItems.push_back(MenuItemExtended("Config. Ship Loadouts", 3, 1, &keyLOADOUT[Joystick], ""));
      menuItems.push_back(MenuItemExtended("Toggle Map Mode", 4, 1, &keyCMDRMAP[Joystick], ""));
      menuItems.push_back(MenuItemExtended("Show Scoreboard", 5, 1, &keySCRBRD[Joystick], "Scoreboard will be visible while this key/button is held down"));
      menuItems.push_back(MenuItemExtended("", 6, 1, NULL, ""));     // Dummy spacer item

      // Col 2
      menuItems.push_back(MenuItemExtended("Select Weapon 1", 7, 2, &keySELWEAP1[Joystick], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(MenuItemExtended("Select Weapon 2", 8, 2, &keySELWEAP2[Joystick], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(MenuItemExtended("Select Weapon 3", 9, 2, &keySELWEAP3[Joystick], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(MenuItemExtended("Quick Chat", 10, 2, &keyQUICKCHAT[Joystick], ""));
      menuItems.push_back(MenuItemExtended("Team Chat", 11, 2, &keyTEAMCHAT[Joystick], ""));
      menuItems.push_back(MenuItemExtended("Global Chat", 12, 2, &keyGLOBCHAT[Joystick], ""));
      menuItems.push_back(MenuItemExtended("Enter Command", 13, 2, &keyCMDCHAT[Joystick], ""));
      menuItems.push_back(MenuItemExtended("Record Voice Msg", 14, 2, &keyTOGVOICE[Joystick], ""));
   }
   else     // Keyboard mode
   {
      itemsPerCol = 9;           // Approx. half of the items we have

      // Col 1
      menuItems.push_back(MenuItemExtended("Ship Up", 0, 1, &keyUP[Keyboard], ""));
      menuItems.push_back(MenuItemExtended("Ship Down", 1, 1, &keyDOWN[Keyboard], ""));
      menuItems.push_back(MenuItemExtended("Ship Left", 2, 1, &keyLEFT[Keyboard], ""));
      menuItems.push_back(MenuItemExtended("Ship Right", 3, 1, &keyRIGHT[Keyboard], ""));
      menuItems.push_back(MenuItemExtended("Fire", 4, 1, &keyFIRE[Keyboard], "The mouse will always be used to aim your ship"));
      menuItems.push_back(MenuItemExtended("Activate Module 1", 5, 1, &keyMOD1[Keyboard], "Module will be active while this key/button is held down"));
      menuItems.push_back(MenuItemExtended("Activate Module 2", 6, 1, &keyMOD2[Keyboard], "Module will be active while this key/button is held down"));
      menuItems.push_back(MenuItemExtended("Toggle Map Mode", 7, 1, &keyCMDRMAP[Keyboard], ""));
      menuItems.push_back(MenuItemExtended("Show Scoreboard", 8, 1, &keySCRBRD[Keyboard], "Scoreboard will be visible while this key/button is held down"));

      // Col 2
      menuItems.push_back(MenuItemExtended("Select Weapon 1", 9, 2, &keySELWEAP1[Keyboard], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(MenuItemExtended("Select Weapon 2", 10, 2, &keySELWEAP2[Keyboard], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(MenuItemExtended("Select Weapon 3", 11, 2, &keySELWEAP3[Keyboard], "Use as an alternative to Advance Weapon"));
      menuItems.push_back(MenuItemExtended("Advance Weapon", 12, 2, &keyADVWEAP[Keyboard], "Toggles your weapons, use as an alternative to Select Weapon commands"));
      menuItems.push_back(MenuItemExtended("Configure Ship Loadouts", 13, 2, &keyLOADOUT[Keyboard], ""));
      menuItems.push_back(MenuItemExtended("Quick Chat", 14, 2, &keyQUICKCHAT[Keyboard], ""));
      menuItems.push_back(MenuItemExtended("Team Chat", 15, 2, &keyTEAMCHAT[Keyboard], ""));
      menuItems.push_back(MenuItemExtended("Global Chat", 16, 2, &keyGLOBCHAT[Keyboard], ""));
      menuItems.push_back(MenuItemExtended("Enter Command", 17, 2, &keyCMDCHAT[Keyboard], ""));

      menuItems.push_back(MenuItemExtended("Record Voice Msg", 18, 2, &keyTOGVOICE[Keyboard], ""));
   }
}

void KeyDefMenuUserInterface::idle(U32 timeDelta)
{
   errorMsgTimer.update(timeDelta);
}

// Finds out if key is a duplicate value
inline bool isDuplicate(S32 key)
{
   S32 size = gKeyDefMenuUserInterface.menuItems.size();
   S32 count = 0;

   for(S32 i = 0; i < size && count < 2; i++)
      if((gKeyDefMenuUserInterface.menuItems[i].mText[0] != '\0') && (*gKeyDefMenuUserInterface.menuItems[i].primaryControl == *gKeyDefMenuUserInterface.menuItems[key].primaryControl))
         count++;

   return count >= 2;
}

void KeyDefMenuUserInterface::render()
{
   // Draw the game screen, then dim it out so you can still see it under our overlay
   if(gClientGame->getConnectionToServer())
   {
      gGameUserInterface.render();
      glColor4f(0, 0, 0, 0.6);
      glEnable(GL_BLEND);
      glBegin(GL_POLYGON);
      glVertex2f(0, 0);
      glVertex2f(canvasWidth, 0);
      glVertex2f(canvasWidth, canvasHeight);
      glVertex2f(0, canvasHeight);
      glEnd();
      glDisable(GL_BLEND);
   }

   glColor3f(1, 1, 1);
   drawCenteredString(vertMargin, 30, menuTitle);
   drawCenteredString(vertMargin + 35, 18, menuSubTitle);
   glColor3f(0, 1, 0);
   drawCenteredString(vertMargin + 63, 14, "You can define different keys for keyboard or joystick mode.  Switch in Options menu.");
   glColor3f(1, 1, 1);

   drawCenteredString(canvasHeight - vertMargin - 20, 18, menuFooter);

   if(selectedIndex >= menuItems.size())
      selectedIndex = 0;

   S32 size = menuItems.size();

   S32 yStart = vertMargin + 120;
   //glColor3f(0,1,0);

   for(S32 i = 0; i < size; i++)
   {
      S32 y = yStart + (i - ((i < itemsPerCol) ? 0 : itemsPerCol)) * 30;

      if(selectedIndex == i)       // Highlight selected item
      {
         glColor3f(0, 0, 0.4);     // Fill
         glBegin(GL_POLYGON);
            glVertex2f((i < itemsPerCol ? horizMargin : canvasWidth / 2) , y - 5);
            glVertex2f((i < itemsPerCol ? canvasWidth / 2 - horizMargin: 800 - horizMargin), y - 5);
            glVertex2f((i < itemsPerCol ? canvasWidth / 2 - horizMargin: 800 - horizMargin), y + 25 + 1);
            glVertex2f((i < itemsPerCol ? horizMargin : canvasWidth / 2), y + 25 + 1);
         glEnd();

         glColor3f(0, 0, 1);       // Outline
         glBegin(GL_LINE_LOOP);
            glVertex2f((i < itemsPerCol ? horizMargin : canvasWidth / 2), y - 5);
            glVertex2f((i < itemsPerCol ? canvasWidth / 2 - horizMargin : 800 - horizMargin), y - 5);
            glVertex2f((i < itemsPerCol ? canvasWidth / 2 - horizMargin : 800 - horizMargin), y + 25 + 1);
            glVertex2f((i < itemsPerCol ? horizMargin : canvasWidth / 2 ), y + 25 + 1);
         glEnd();
      }

      if(menuItems[i].mText[0] != '\0')
      {
         // Draw item text
         glColor3f(0, 1, 1);
         drawString((i < itemsPerCol ? 2 * horizMargin : canvasWidth / 2 + horizMargin), y, 15, menuItems[i].mText);

         if(changingItem == i)
         {
            glColor3f(1, 0, 0);
            drawString(canvasWidth * (i < itemsPerCol ? 0.25 : 0.75) + horizMargin, y + 1, 13, "Press Key or Button");
         }
         else
         {
            bool stat;     // A total hack
            if(isDuplicate(i))
            {
               stat = true;
               glColor3f(1,0,0);
            }
            else
            {
               glColor3f(1, 1, 1);
               stat = false;
            }

            renderControllerButton(canvasWidth * (i < itemsPerCol ? 0.25 : 0.75) + horizMargin, y, *menuItems[i].primaryControl, stat, 10);
         }
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

      glEnable(GL_BLEND);
      glColor4f(1, 0, 0, alpha);
      drawCenteredString(canvasHeight - vertMargin - 65, 15, errorMsg.c_str());
      glDisable(GL_BLEND);
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
   if(keyCode == KEY_SPACE || keyCode == KEY_ENTER || keyCode == BUTTON_START)      // Set key for selected item
   {
      if(menuItems[selectedIndex].mText[0] == '\0')    // Can't change blank items
         return;

      UserInterface::playBoop();
      changingItem = selectedIndex;
   }
   else if(keyCode == KEY_RIGHT  || keyCode == BUTTON_DPAD_RIGHT || keyCode == KEY_LEFT || keyCode == BUTTON_DPAD_LEFT)    // Change col
   {
      UserInterface::playBoop();
      if(selectedIndex < itemsPerCol)
         selectedIndex += itemsPerCol;
      else
         selectedIndex -= itemsPerCol;
   }
   else if(keyCode == KEY_ESCAPE || keyCode == BUTTON_BACK)       // Quit
   {
      UserInterface::playBoop();
      saveSettingsToINI();

      UserInterface::reactivatePrevUI();      //gOptionsMenuUserInterface
   }
   else if(keyCode == KEY_UP || keyCode == BUTTON_DPAD_UP)        // Prev item
   {
      selectedIndex--;
      if(selectedIndex < 0)
         selectedIndex = menuItems.size() - 1;
      UserInterface::playBoop();
   }
   else if(keyCode == KEY_DOWN || keyCode == BUTTON_DPAD_DOWN)    // Next item
   {
      selectedIndex++;
      if(selectedIndex >= menuItems.size())
         selectedIndex = 0;
      UserInterface::playBoop();
   }
   else if(keyCode == keyOUTGAMECHAT)     // Turn on Global Chat overlay
   {
      gChatInterface.activate();
      UserInterface::playBoop();
   }

//   else if(keyCode == keyDIAG)            // Turn on diagnostic overlay
//   {
//      gDiagnosticInterface.activate();
//      UserInterface::playBoop();
//   }
}


};

