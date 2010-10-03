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

   S32 x = drawCenteredString(y, fontSizeBig, lineEditor.getDisplayString().c_str());
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
         lineEditor.addChar(ascii);
   }
}


void TextEntryUserInterface::setString(string str)
{
   lineEditor.setString(str);
}


////////////////////////////////////////
////////////////////////////////////////

extern IniSettings gIniSettings;

LevelNameEntryUserInterface gLevelNameEntryUserInterface;

void LevelNameEntryUserInterface::onEscape()
{
   UserInterface::playBoop();
   reactivatePrevUI();      //gMainMenuUserInterface
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

void PasswordEntryUserInterface::render()
{
   if(gClientGame->getConnectionToServer())
   {
      gGameUserInterface.render();
      glColor4f(0, 0, 0, 0.5);
      glEnable(GL_BLEND);
         glBegin(GL_POLYGON);
            glVertex2f(0, 0);
            glVertex2f(UserInterface::canvasWidth, 0);
            glVertex2f(UserInterface::canvasWidth, UserInterface::canvasHeight);
            glVertex2f(0, UserInterface::canvasHeight);
         glEnd();
      glDisable(GL_BLEND);
   }

   Parent::render();
}


////////////////////////////////////////
////////////////////////////////////////

void PreGamePasswordEntryUserInterface::onAccept(const char *text)
{
   joinGame(connectAddress, false, false);      // Not from master, not local
}


void PreGamePasswordEntryUserInterface::onEscape()
{
   gMainMenuUserInterface.activate();
}

////////////////////////////////////////
////////////////////////////////////////

ServerPasswordEntryUserInterface gServerPasswordEntryUserInterface;

// Constructor
ServerPasswordEntryUserInterface::ServerPasswordEntryUserInterface()        
{
   setMenuID(PasswordEntryUI);
   title = "ENTER SERVER PASSWORD:";
   instr1 = "";
   instr2 = "Enter the password required for access to the server";
}


////////////////////////////////////////
////////////////////////////////////////

ReservedNamePasswordEntryUserInterface gReservedNamePasswordEntryUserInterface;

// Constructor
ReservedNamePasswordEntryUserInterface::ReservedNamePasswordEntryUserInterface()       
{
   setMenuID(ReservedNamePasswordEntryUI);
   title = "ENTER USERNAME PASSWORD:";
   instr1 = "The username you are using has been reserved on this server.";
   instr2 = "Please enter the password to use this name here.";
}


////////////////////////////////////////
////////////////////////////////////////

void InGamePasswordEntryUserInterface::onAccept(const char *text)
{
   GameConnection *gc = gClientGame->getConnectionToServer();
   if(gc)
   {
      submitPassword(gc, text);

      reactivatePrevUI();                                                  // Reactivating clears subtitle message, so reactivate first...
      gGameMenuUserInterface.mMenuSubTitle = "** checking password **";     // ...then set the message
   }
   else
      reactivatePrevUI();                                                  // Otherwise, just reactivate the previous menu
}

void InGamePasswordEntryUserInterface::onEscape()
{
   reactivatePrevUI();
}

////////////////////////////////////////
////////////////////////////////////////

AdminPasswordEntryUserInterface gAdminPasswordEntryUserInterface;

// Constructor
AdminPasswordEntryUserInterface::AdminPasswordEntryUserInterface()      
{
   setMenuID(AdminPasswordEntryUI);
   title = "ENTER ADMIN PASSWORD:";
   instr1 = "";
   instr2 = "Enter the admin password to perform admin tasks and change levels on this server";
}


void AdminPasswordEntryUserInterface::submitPassword(GameConnection *gameConnection, const char *text)
{
   gameConnection->submitAdminPassword(text);
}


////////////////////////////////////////
////////////////////////////////////////

LevelChangePasswordEntryUserInterface gLevelChangePasswordEntryUserInterface;

// Constructor
LevelChangePasswordEntryUserInterface::LevelChangePasswordEntryUserInterface()      
{
   setMenuID(LevelChangePasswordEntryUI);
   title = "ENTER LEVEL CHANGE PASSWORD:";
   instr1 = "";
   instr2 = "Enter the level change password to change levels on this server";
}


void LevelChangePasswordEntryUserInterface::submitPassword(GameConnection *gameConnection, const char *text)
{
   gameConnection->submitLevelChangePassword(text);
}


////////////////////////////////////////
////////////////////////////////////////
};


