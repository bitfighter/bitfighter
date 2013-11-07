//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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
