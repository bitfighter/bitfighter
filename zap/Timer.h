//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo relased for Torque Network Library by GarageGames.com
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

#ifndef _TIMER_H_
#define _TIMER_H_

#include "../tnl/tnlTypes.h"

using namespace TNL;

namespace Zap
{

class Timer
{
private:
   U32 mPeriod;
   U32 mCurrentCounter;

public:
   explicit Timer(U32 period = 0);  // Constructor
   virtual ~Timer();       // Destructor

   // Update timer in idle loop -- returns true if timer has just expired, false if there's still time left
   bool update(U32 timeDelta);

   // Return amount of time left on timer
   U32 getCurrent() const;

   // Return fraction of original time left on timer
   F32 getFraction() const;

   void invert();    // Set current timer value to 1 - (currentFraction)

   void setPeriod(U32 period);
   U32 getPeriod() const;
   U32 getElapsed() const;

   void reset(); // Start timer over, using last time set

   // Extend will add or remove time from the timer in a way that preserves overall timer duration
   void extend(S32 time);

   // Start timer over, setting timer to the time specified
   void reset(U32 newCounter, U32 newPeriod = 0);

   // Remove all time from timer
   void clear();
};

} /* namespace Zap */
#endif
