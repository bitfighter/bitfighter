//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Timer.h"


namespace Zap
{


Timer::Timer(U32 period)
{
   mCurrentCounter = mPeriod = period;
}


Timer::~Timer()
{
   // Do nothing
}


bool Timer::update(U32 timeDelta)
{
   if(mCurrentCounter == 0)
      return false;

   if(timeDelta >= mCurrentCounter)
   {
      mCurrentCounter = 0;
      return true;
   }

   mCurrentCounter -= timeDelta;
   return false;
}


U32 Timer::getCurrent() const
{
   return mCurrentCounter;
}


F32 Timer::getFraction() const
{
   if(!mPeriod)
      return 0;
   return mCurrentCounter / F32(mPeriod);
}


void Timer::invert()
{
   mCurrentCounter = U32((1.0f - getFraction()) * mPeriod);
}


void Timer::setPeriod(U32 period)
{
   mPeriod = period;
}


U32 Timer::getPeriod() const
{
   return mPeriod;
}


U32 Timer::getElapsed() const
{
   return mPeriod - mCurrentCounter;
}


void Timer::reset()
{
   mCurrentCounter = mPeriod;
}


// Note that time could be negative to shorten timer!  -- TODO: Do we really want to alter the timer period here?
void Timer::extend(S32 time)
{
   U32 U32time = U32(abs(time));

   if(time > 0)
   {
      if(U32time > (U32_MAX - mPeriod))            // Overflow protection
         mPeriod = U32time;
      else
         mPeriod += U32time;

      if(U32time > (U32_MAX - mCurrentCounter))    // Overflow protection
         mCurrentCounter = U32_MAX;
      else
         mCurrentCounter += U32time;
   }

   else if(time < 0)
   {
      if(U32time > mPeriod)                        // Underflow protection
         mPeriod = 0;
      else
         mPeriod -= U32time;

      if(U32time > mCurrentCounter)                // Underflow protection
         mCurrentCounter = 0;
      else
         mCurrentCounter -= U32time;
   }
}


void Timer::reset(U32 newCounter, U32 newPeriod)
{
   if(newPeriod == 0)
      newPeriod = newCounter;

   mCurrentCounter = newCounter;
   mPeriod = newPeriod;
}


void Timer::clear()
{
   mCurrentCounter = 0;
}


} /* namespace Zap */
