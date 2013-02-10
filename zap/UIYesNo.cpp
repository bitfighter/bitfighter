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

// Renders a simple message box, waits for the user to hit Y or N, and runs a registered function

#include "UIYesNo.h"
#include "UIMenus.h"

#include <stdio.h>

namespace Zap
{

// Constructor
YesNoUserInterface::YesNoUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(YesOrNoUI);

   mYesFunction = NULL;
   mNoFunction = NULL;
}


void YesNoUserInterface::registerYesFunction(void(*ptr)(ClientGame *game))
{
   mYesFunction = ptr;
}


void YesNoUserInterface::registerNoFunction(void(*ptr)(ClientGame *game))
{
   mNoFunction = ptr;
}


void YesNoUserInterface::reset()
{
   Parent::reset();

   mTitle = "YES OR NO";    // Default title

   for(S32 i = 0; i < MAX_LINES; i++)
      mMessage[i] = "";

   mInstr = "Press [Y] or [N]";

   mYesFunction = NULL;
   mNoFunction = NULL;
}


bool YesNoUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode))
      return true;

   if(inputCode == KEY_Y)
   {
      if(mYesFunction)
         mYesFunction(getGame());
      else
         quit();
   }
   else if(inputCode == KEY_N)
   {
      if(mNoFunction)
         mNoFunction(getGame());
      else
         quit();
   }
   else if(inputCode == KEY_ESCAPE)
      quit();
   else
      return false;

   // A key was handled
   return true;
}


};


