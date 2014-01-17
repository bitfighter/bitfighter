//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_SLIDE_OUT_WIDGET_H_
#define _UI_SLIDE_OUT_WIDGET_H_


#include "tnlTypes.h"
#include "Timer.h"


using namespace TNL; 


namespace Zap
{

class Color;

class SlideOutWidget
{
private:
   bool mActivating;
   Timer mAnimationTimer;

protected:
   void setAnimationTime(U32 period);
   bool isOpening() const;
   bool isClosing() const;          // Return true if widget is playing the closing animation

   S32 mStarting;

   void renderSlideoutWidgetFrame(S32 ulx, S32 uly, S32 width, S32 height, const Color &borderColor) const;

public:
   SlideOutWidget();                // Constructor
   virtual ~SlideOutWidget();       // Destructor

   virtual void idle(U32 deltaT);
   virtual void onActivated();      // User requested widget to open
   virtual void onDeactivated();    // User requested widget to close

   F32 getInsideEdge() const;

   virtual void onWidgetOpened();   // Widget has finished opening
   virtual void onWidgetClosed();   // Widget has finished closing

   virtual bool isActive() const;

   F32 getFraction() const;         // Get fraction of openness
   virtual S32 getAnimationTime() const;
};


}

#endif
