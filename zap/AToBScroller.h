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

#ifndef _A_TO_B_SCROLLER_H_
#define _A_TO_B_SCROLLER_H_

#include "ScissorsManager.h"
#include "Timer.h"

using namespace TNL;
using namespace Zap;


namespace UI
{

// Class for producing a scrolling transition between two objects (A and B).  Used, for example, to help transition
// between Modules and Weapons on the Loadout menu.
class AToBScroller
{
private:
   Timer mScrollTimer;
   ScissorsManager mScissorsManager;      // Could probably be static, practically...

protected:
   virtual void onActivated();
   virtual void idle(U32 deltaT);

   void resetScrollTimer();
   void clearScrollTimer();

   S32 getTransitionPos(S32 fromPos, S32 toPos) const;
   bool isActive() const;

   static const S32 NO_RENDER = S32_MAX;

   // These will return the top render position, or NO_RENDER if rendering can be skipped
   S32 prepareToRenderFromDisplay(ClientGame *game, S32 top, S32 fromHeight, S32 toHeight);
   S32 prepareToRenderToDisplay  (ClientGame *game, S32 top, S32 fromHeight, S32 toHeight);
   void doneRendering();

public:
   AToBScroller();      // Constructor
   virtual ~AToBScroller();      // Constructor
};

}

#endif  // _A_TO_B_SCROLLER_H_
