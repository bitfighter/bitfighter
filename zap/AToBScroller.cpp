//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "AToBScroller.h"
#include "DisplayManager.h"
#include "ScissorsManager.h"


using namespace Zap;

namespace Zap { namespace UI {


static ScissorsManager scissorsManager;


// Constructor
AToBScroller::AToBScroller()
{
   mScrollTimer.setPeriod(150);     // Scroll time
}


// Destructor
AToBScroller::~AToBScroller()
{
   // Do nothing
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
   if(toPos == S32_MIN)
      return fromPos;

   F32 fraction = mScrollTimer.getFraction();

   return S32(fromPos * fraction + toPos * (1 - fraction));
}


bool AToBScroller::isActive() const
{
   return mScrollTimer.getCurrent() > 0;
}


// Returns the y-pos that the caller should render its display for the scrolling effect, or NO_RENDER if the caller shouldn't bother
S32 AToBScroller::prepareToRenderFromDisplay(DisplayMode displayMode, S32 top, S32 fromHeight, S32 toHeight) const
{
   if(mScrollTimer.getCurrent() == 0)
      return NO_RENDER;

   S32 height = getTransitionPos(fromHeight, toHeight);

   scissorsManager.enable(mScrollTimer.getCurrent() > 0, displayMode, 0, top, 
                          DisplayManager::getScreenInfo()->getGameCanvasWidth(), getTransitionPos(height, 0));

   return top - fromHeight * (1 - mScrollTimer.getFraction());
}


// Returns the y-pos that the caller should render its display for the scrolling effect
S32 AToBScroller::prepareToRenderToDisplay(DisplayMode displayMode, S32 top, S32 fromHeight, S32 toHeight) const
{
   if(mScrollTimer.getCurrent() == 0)
      return top;

   S32 height = getTransitionPos(fromHeight, toHeight);

   S32 vertBuffer = 2;  // A little extra space to avoid clipping lines at the top of our clip area
   scissorsManager.enable(mScrollTimer.getCurrent() > 0, displayMode, 0, top + getTransitionPos(height, 0) - vertBuffer, 
                          DisplayManager::getScreenInfo()->getGameCanvasWidth(), getTransitionPos(0, height) + 2 * vertBuffer);

   return top + fromHeight * mScrollTimer.getFraction();
}


void AToBScroller::doneRendering() const
{
   scissorsManager.disable();
}



} } // Nested namespace
