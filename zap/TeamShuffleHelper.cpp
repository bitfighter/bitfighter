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

#include "TeamShuffleHelper.h"
#include "ClientGame.h"
#include "UIGame.h"
#include "UIMenus.h"
#include "ScreenInfo.h"

#include "SDL/SDL_opengl.h"

#include <math.h>


namespace Zap
{

// Constructor
TeamShuffleHelper::TeamShuffleHelper(ClientGame *clientGame) : Parent(clientGame)
{
   // Do nothing
}


void TeamShuffleHelper::onMenuShow()
{
   // Do nothing
}


void TeamShuffleHelper::render()
{
   S32 teamCount = getGame()->getTeamCount();
   S32 cols;

   switch(teamCount)
   {
   case 1:
      cols = 1;
      break;
   case 2:
   case 4:
      cols = 2;
      break;
   case 3:
   case 5:
   case 6:
   case 7:
   case 8:
   case 9:
     cols = 3;
     break;
   default:
      cols = 1;
   }


   S32 rows = (S32)ceil((F32)teamCount / (F32)cols);
   S32 maxColWidth = gScreenInfo.getGameCanvasWidth() / cols;

   S32 colWidth = -1;
   const S32 textSize = 15;
   const S32 margin = 10;

   for(S32 i = 0; i < getGame()->getPlayerCount(); i++)
   {
      S32 width = UserInterface::getStringWidth(textSize, getGame()->Game::getClientInfo(i)->getName().getString());

      if(width > colWidth)
      {
         if(width > maxColWidth)
         {
            colWidth = maxColWidth;
            break;
         }
         else
            colWidth = width;
      }
   }

   const S32 rowHeight = 80;
   const S32 topMargin = 100;
   const S32 leftMargin = 100;

   for(S32 i = 0; i < rows; i++)
      for(S32 j = 0; j < cols; j++)
      {
         if(i * cols + j >= teamCount)
            break;

         S32 x = leftMargin + j * (colWidth + margin);
         S32 y = topMargin + i * (rowHeight + margin);

         UserInterface::drawFilledRect(x, y, x + colWidth, y + rowHeight, Colors::black, getGame()->getTeamColor(i * cols + j));
      }
}


// Return true if key did something, false if key had no effect
// Runs on client
bool TeamShuffleHelper::processInputCode(InputCode inputCode)
{
   if(Parent::processInputCode(inputCode))    // Check for cancel keys
      return true;

   // We're interested in space, enter, and escape

   return true;
}


InputCode TeamShuffleHelper::getActivationKey()  { return KEY_NONE; }      // Only activated via chat cmd

bool TeamShuffleHelper::isMovementDisabled()  { return true; }


};

