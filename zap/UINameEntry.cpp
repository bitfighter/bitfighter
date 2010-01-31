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

#include "UINameEntry.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "UIChat.h"
#include "UIEditor.h"
#include "game.h"
#include "gameConnection.h"
#include "config.h"

#include "md5wrapper.h"
#include "glutInclude.h"

#include <string>

namespace Zap
{

using namespace std;


void TextEntryUserInterface::onActivate()
{
   if(resetOnActivate)
      lineEditor.clear();
}

void TextEntryUserInterface::render()
{
   glColor3f(1,1,1);

   const S32 fontSize = 20;
   const S32 fontSizeBig = 30;

   S32 y = (canvasHeight / 2) - fontSize;

   drawCenteredString(y, fontSize, title);
   y += 45;
   glColor3f(0, 1, 0);
   drawCenteredString(canvasHeight - vertMargin - 2 * fontSize - 5, fontSize, instr1);
   drawCenteredString(canvasHeight - vertMargin - fontSize, fontSize, instr2);

   glColor3f(1,1,1);

   // Create a display version of the string that is all "*"s if string is secret
   string dispString = secret ? string(lineEditor.length(), '*') : lineEditor.getString();

   S32 x = drawCenteredString(y, fontSizeBig, dispString.c_str());
   lineEditor.drawCursor(x, y, fontSizeBig);
}


void TextEntryUserInterface::idle(U32 timeDelta)
{
   LineEditor::updateCursorBlink(timeDelta);
}


void TextEntryUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   switch (keyCode)
   {
      case KEY_ENTER:
         onAccept(lineEditor.c_str());
         break;
      case KEY_BACKSPACE:
         lineEditor.backspacePressed();
         break;
      case KEY_DELETE:
         lineEditor.deletePressed();
         break;
      case KEY_ESCAPE:
         onEscape();
         break;
      default:
         if(isValid(ascii))      // Allows us to override isValid and create character filters
            lineEditor.addChar(ascii);
   }
}


void TextEntryUserInterface::setString(string str)
{
   lineEditor.setString(str);
}


// By default, all chars are valid.  Override to be more restrictive
bool TextEntryUserInterface::isValid(char ascii)
{
   if (ascii)
      return true;
   return false;
}


////////////////////////////////////////
////////////////////////////////////////

NameEntryUserInterface gNameEntryUserInterface;
extern void exitGame();

void NameEntryUserInterface::onEscape()
{
   exitGame();
}

extern bool gReadyToConnectToMaster;
extern IniSettings gIniSettings;

NameEntryUserInterface::NameEntryUserInterface()      // Constructor
{
   setMenuID(NameEntryUI);
   title = "ENTER YOUR NICKNAME:";
   instr1 = "You can skip this screen by adding a Nickname=YourNameHere line";
   instr2 = "to the [Settings] section of Bitfighter.ini";
   resetOnActivate = false;
}


void NameEntryUserInterface::onAccept(const char *name)
{
   if(!strcmp(name, ""))     // Non-blank entries required!
      setString(gIniSettings.defaultName.c_str());

   gMainMenuUserInterface.activate();
   gReadyToConnectToMaster = true;
   gIniSettings.lastName = name;
   
   saveSettingsToINI();             // Get that baby into the INI file
}


////////////////////////////////////////
////////////////////////////////////////

PasswordEntryUserInterface gPasswordEntryUserInterface;

void PasswordEntryUserInterface::onEscape()
{
   gMainMenuUserInterface.activate();
}

void PasswordEntryUserInterface::onAccept(const char *text)
{
   joinGame(connectAddress, false, false);      // Not from master, not local
}


////////////////////////////////////////
////////////////////////////////////////

ReservedNamePasswordEntryUserInterface gReservedNamePasswordEntryUserInterface;

void ReservedNamePasswordEntryUserInterface::onEscape()
{
   gMainMenuUserInterface.activate();
}

void ReservedNamePasswordEntryUserInterface::onAccept(const char *text)
{
   joinGame(connectAddress, false, false);      // Not from master, not local
}


////////////////////////////////////////
////////////////////////////////////////

extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;

LevelNameEntryUserInterface gLevelNameEntryUserInterface;

void LevelNameEntryUserInterface::onEscape()
{
   UserInterface::playBoop();
   reactivatePrevUI();      //gMainMenuUserInterface
}

// Only allow alpha-numeric and _s
bool LevelNameEntryUserInterface::isValid(char ascii)
{
   if ( (ascii >= '0' && ascii <= '9') ||
        (ascii == '_') ||
        (ascii >= 'A' && ascii <= 'Z') ||
        (ascii >= 'a' && ascii <= 'z') )
      return true;

   else return false;
}

void LevelNameEntryUserInterface::onAccept(const char *name)
{
   gEditorUserInterface.setLevelFileName(name);
   UserInterface::playBoop();
   gEditorUserInterface.activate(false);
   
   // Get that baby into the INI file
   gIniSettings.lastEditorName = name;
   saveSettingsToINI();             

}


////////////////////////////////////////
////////////////////////////////////////

AdminPasswordEntryUserInterface gAdminPasswordEntryUserInterface;

void AdminPasswordEntryUserInterface::render()
{
   if(gClientGame->getConnectionToServer())
   {
      gGameUserInterface.render();
      glColor4f(0, 0, 0, 0.5);
      glEnable(GL_BLEND);
         glBegin(GL_POLYGON);
            glVertex2f(0, 0);
            glVertex2f(canvasWidth, 0);
            glVertex2f(canvasWidth, canvasHeight);
            glVertex2f(0, canvasHeight);
         glEnd();
      glDisable(GL_BLEND);
   }

   Parent::render();
}

void AdminPasswordEntryUserInterface::onEscape()
{
   reactivatePrevUI();
}

extern md5wrapper md5;

void AdminPasswordEntryUserInterface::onAccept(const char *text)
{
   GameConnection *gc = gClientGame->getConnectionToServer();
   if(gc)
   {
      gc->submitAdminPassword(text);

      reactivatePrevUI();                                                  // Reactivating clears subtitle message, so reactivate first...
      gGameMenuUserInterface.menuSubTitle = "** checking password **";     // ...then set the message
   }
   else
      reactivatePrevUI();                                                  // Otherwise, just reactivate the previous menu
}


////////////////////////////////////////
////////////////////////////////////////

LevelChangePasswordEntryUserInterface gLevelChangePasswordEntryUserInterface;

void LevelChangePasswordEntryUserInterface::render()
{
   if(gClientGame->getConnectionToServer())
   {
      gGameUserInterface.render();
      glColor4f(0, 0, 0, 0.5);
      glEnable(GL_BLEND);
         glBegin(GL_POLYGON);
            glVertex2f(0, 0);
            glVertex2f(canvasWidth, 0);
            glVertex2f(canvasWidth, canvasHeight);
            glVertex2f(0, canvasHeight);
         glEnd();
      glDisable(GL_BLEND);
   }

   Parent::render();
}

void LevelChangePasswordEntryUserInterface::onEscape()
{
   reactivatePrevUI();
}

void LevelChangePasswordEntryUserInterface::onAccept(const char *text)
{

   GameConnection *gc = gClientGame->getConnectionToServer();
   if(gc)
   {
      gc->submitLevelChangePassword(text);

      reactivatePrevUI();                                                  // Reactivating clears subtitle message, so reactivate first...
      gGameMenuUserInterface.menuSubTitle = "** checking password **";     // ...then set the message
   }
   else
      reactivatePrevUI();                                                  // Otherwise, just reactivate the previous menu
}


};


