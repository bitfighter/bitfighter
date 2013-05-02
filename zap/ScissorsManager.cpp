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

#include "ScissorsManager.h"     // Class header

#include "ConfigEnum.h"          // For DisplayMode def
#include "ClientGame.h"
#include "ScreenInfo.h"
#include "OpenglUtils.h"


namespace Zap
{
   
extern ScreenInfo gScreenInfo;

// Store previous scissors settings
void ScissorsManager::enable(bool enable, ClientGame *game, F32 x, F32 y, F32 width, F32 height) 
{
   mManagerEnabled = enable;

   if(!enable)
      return;

   glGetBooleanv(GL_SCISSOR_TEST, &mScissorsWasEnabled);

   if(mScissorsWasEnabled)
      glGetIntegerv(GL_SCISSOR_BOX, &mScissorBox[0]);

   DisplayMode mode = game->getSettings()->getIniSettings()->displayMode;    // Windowed, full_screen_stretched, full_screen_unstretched

   static Point p1, p2;
   p1 = gScreenInfo.convertCanvasToWindowCoord(x, gScreenInfo.getGameCanvasHeight() - y - height, mode);
   p2 = gScreenInfo.convertCanvasToWindowCoord(width, height, mode);

   glScissor(p1.x, p1.y, p2.x, p2.y);

   glEnable(GL_SCISSOR_TEST);
}


// Restore previous scissors settings
void ScissorsManager::disable()
{
   if(!mManagerEnabled)
      return;

   if(mScissorsWasEnabled)
      glScissor(mScissorBox[0], mScissorBox[1], mScissorBox[2], mScissorBox[3]);
   else
      glDisable(GL_SCISSOR_TEST);

   mManagerEnabled = false;
}


};