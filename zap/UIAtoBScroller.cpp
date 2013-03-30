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

#include "UIAToBScroller.h"
#include "ScreenInfo.h"

using namespace Zap;

namespace UI
{


// Constructor
AToBScroller::AToBScroller()
{
   mScrollTimer.setPeriod(150);     // Scroll time
}


void AToBScroller::onActivated()
{
   mScrollTimer.clear();
}


void AToBScroller::idle(U32 deltaT)
{
   mScrollTimer.update(deltaT);
}


void AToBScroller::clearScrollTimer()
{
   mScrollTimer.clear();
}


void AToBScroller::resetScrollTimer()
{
   mScrollTimer.reset();
}


// If we are transitioning between items of different sizes, we will gradually change the rendered size during the transition.
// This function caluclates the new position of an item given its original position and the one it's transitioning to.
S32 AToBScroller::getTransitionPos(S32 fromPos, S32 toPos) const
{
   F32 fraction = mScrollTimer.getFraction();

   return fromPos * fraction + toPos * (1 - fraction);
}


bool AToBScroller::isActive() const
{
   return mScrollTimer.getCurrent() > 0;
}


// Returns the y-pos that the caller should render its display for the scrolling effect, or NO_RENDER if the caller shouldn't bother
S32 AToBScroller::prepareToRenderFromDisplay(ClientGame *game, S32 top, S32 fromHeight, S32 toHeight)
{
   if(mScrollTimer.getCurrent() == 0)
      return NO_RENDER;

   S32 height = getTransitionPos(fromHeight, toHeight);

   mScissorsManager.enable(mScrollTimer.getCurrent() > 0, game, 0, top, 
                        gScreenInfo.getGameCanvasWidth(), getTransitionPos(height, 0));

   return top - fromHeight * (1 - mScrollTimer.getFraction());
}


// Returns the y-pos that the caller should render its display for the scrolling effect
S32 AToBScroller::prepareToRenderToDisplay(ClientGame *game, S32 top, S32 fromHeight, S32 toHeight)
{
   if(mScrollTimer.getCurrent() == 0)
      return top;

   S32 height = getTransitionPos(fromHeight, toHeight);

   mScissorsManager.enable(mScrollTimer.getCurrent() > 0, game, 0, top + getTransitionPos(height, 0), 
                           gScreenInfo.getGameCanvasWidth(), getTransitionPos(0, height));

   return top + fromHeight * mScrollTimer.getFraction();
}


void AToBScroller::doneRendering()
{
   mScissorsManager.disable();
}



}