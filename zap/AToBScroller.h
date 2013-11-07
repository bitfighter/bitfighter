//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _A_TO_B_SCROLLER_H_
#define _A_TO_B_SCROLLER_H_


#include "Timer.h"
#include "ConfigEnum.h" // For DisplayMode def


using namespace TNL;


namespace Zap { 
   
class ClientGame;

namespace UI {




// Class for producing a scrolling transition between two objects (A and B).  Used, for example, to help transition
// between Modules and Weapons on the Loadout menu.
class AToBScroller
{
protected:
   Timer mScrollTimer;
   S32 getTransitionPos(S32 fromPos, S32 toPos) const;
   bool isActive() const;

   static const S32 NO_RENDER = S32_MAX;

   // These will return the top render position, or NO_RENDER if rendering can be skipped
   S32 prepareToRenderFromDisplay(DisplayMode displayMode, S32 top, S32 fromHeight, S32 toHeight = S32_MIN) const;
   S32 prepareToRenderToDisplay  (DisplayMode displayMode, S32 top, S32 fromHeight, S32 toHeight = S32_MIN) const;
   void doneRendering() const;

public:
   AToBScroller();            // Constructor
   virtual ~AToBScroller();   // Destructor

   virtual void onActivated();
   virtual void idle(U32 deltaT);

   void resetScrollTimer();
   void clearScrollTimer();

};

} } // Nested namespace


#endif
