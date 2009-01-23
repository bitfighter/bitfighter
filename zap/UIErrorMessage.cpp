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

// Renders a simple message box, and waits for the user to hit a key

#include "UIErrorMessage.h"
#include "UIMenus.h"

#include "../glut/glutInclude.h"
#include <stdio.h>

namespace Zap
{

ErrorMessageUserInterface gErrorMsgUserInterface;

// Constructor
ErrorMessageUserInterface::ErrorMessageUserInterface()
{
   setMenuID(ErrorMessageUI);
   reset();
}


void ErrorMessageUserInterface::onActivate()
{
}

void ErrorMessageUserInterface::reset()
{
   mTitle = "WE HAVE A PROBLEM";    // Default title
   mInstr = "Hit any key to continue";
   for(S32 i = 0; i < mNumLines; i++)
      mMessage[i] = "";
}

// First line is 1
void ErrorMessageUserInterface::setMessage(U32 id, const char *message)
{
   if (id < 1 || id > mNumLines)       // Protect devs from themselves...
      return;

   mMessage[id-1] = message;
}

void ErrorMessageUserInterface::setTitle(const char *message)
{
   mTitle = message;
}

void ErrorMessageUserInterface::setInstr(const char *message)
{
   mInstr = message;
}

void ErrorMessageUserInterface::quit()
{
   UserInterface::reactivatePrevUI();      //gMainMenuUserInterface
}

void ErrorMessageUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   quit();     // Quit the interface when any key is pressed...  any key at all.
}


void ErrorMessageUserInterface::render()
{
   const S32 inset = 100;
   if (prevUIs.size())            // If there is an underlying menu...
      prevUIs.last()->render();   // ...render it

   glColor4f(.3, 0, 0, 1);        // Draw a box
   glEnable(GL_BLEND);
      glBegin(GL_POLYGON);
         glVertex2f(inset, inset);
         glVertex2f(canvasWidth - inset, inset);
         glVertex2f(canvasWidth - inset, canvasHeight - inset);
         glVertex2f(inset, canvasHeight-inset);
      glEnd();
   glDisable(GL_BLEND);

   glColor3f(1, 1, 1);           // Add a border
   glBegin(GL_LINE_LOOP);
      glVertex2f(inset, inset);
      glVertex2f(canvasWidth - inset, inset);
      glVertex2f(canvasWidth - inset, canvasHeight - inset);
      glVertex2f(inset, canvasHeight - inset);
   glEnd();

   // Draw title, message, and footer
   glColor3f(1,1,1);
   drawCenteredString(vertMargin + inset, 30, mTitle);

   for(S32 i = 0; i < mNumLines; i++)
      drawCenteredString(vertMargin + 40 + inset + i * 24, 18, mMessage[i]);

   drawCenteredString(canvasHeight - vertMargin - inset - 18, 18, mInstr);
}

};

