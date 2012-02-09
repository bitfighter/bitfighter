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

// Renders a simple message box, and waits for the user to hit a key

#include "UIErrorMessage.h"
#include "UIMenus.h"

#include <stdio.h>
#include <math.h>

namespace Zap
{

// Constructor
ErrorMessageUserInterface::ErrorMessageUserInterface(ClientGame *game) : UserInterface(game)
{
   setMenuID(ErrorMessageUI);
   reset();
}


void ErrorMessageUserInterface::onActivate()
{
   // Do nothing
}


void ErrorMessageUserInterface::reset()
{
   mTitle = "WE HAVE A PROBLEM";          // Default title
   mInstr = "Hit any key to continue";
   mPresentationId = 0;

   for(S32 i = 0; i < MAX_LINES; i++)
      mMessage[i] = "";
}


// First line is 1
void ErrorMessageUserInterface::setMessage(S32 id, const char *message)
{
   TNLAssert(id >= 1 && id <= MAX_LINES, "Invalid line id!");
   mMessage[id-1] = message;
}


void ErrorMessageUserInterface::setTitle(const char *message)
{
   mTitle = message;
}


void ErrorMessageUserInterface::setPresentation(S32 presentationId)
{
   mPresentationId = presentationId;
}


void ErrorMessageUserInterface::setInstr(const char *message)
{
   mInstr = message;
}


void ErrorMessageUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();      // to gMainMenuUserInterface
}


bool ErrorMessageUserInterface::onKeyDown(InputCode inputCode, char ascii)
{
   if(!Parent::onKeyDown(inputCode, ascii))
      quit();     // Quit the interface when any key is pressed...  any key at all.  Mostly.
   return true;
}


void ErrorMessageUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);
}


void ErrorMessageUserInterface::render()
{
   getUIManager()->renderPrevUI();
   
   if(mPresentationId == 0)      // Standard presentation
      renderMessageBox(mTitle, mInstr, mMessage, MAX_LINES);
   else if(mPresentationId == 1)
       renderUnboxedMessageBox(mTitle, mInstr, mMessage, MAX_LINES);
   else
      TNLAssert(false, "Unknown value of mPresentationId!");
}



};


