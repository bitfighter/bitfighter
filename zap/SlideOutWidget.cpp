//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "SlideOutWidget.h"

#include "DisplayManager.h"          // For DisplayManager::getScreenInfo() def
#include "Point.h"
#include "Colors.h"
#include "OpenglUtils.h"

#include "tnlVector.h"

namespace Zap
{

static const F32 WidgetSpeed = 2.0f;    // Pixels / ms

// Constructor
SlideOutWidget::SlideOutWidget()
{
   mActivating = false;
   mWidth = 350;           // Menus should set this to a real value so they appear more quickly
   setStartingOffset(0);
}


// Destructor
SlideOutWidget::~SlideOutWidget()
{
   // Do nothing
}


void SlideOutWidget::idle(U32 deltaT)
{
   if(mAnimationTimer.update(deltaT))
   {
      if(mActivating)
         onWidgetOpened();
      else
         onWidgetClosed();
   }
}


// User requested widget to open
void SlideOutWidget::onActivated() 
{
   S32 currDisplayWidth = 0;
   mActivationDirection = true;     // Normal

   // If we're still playing the animation from a previous action, we need to grab it's width to
   // use as the starting point for animating this activation
   if(isActive())
      currDisplayWidth = getCurrentDisplayWidth();

   mActivating = true;
   setStartingOffset(currDisplayWidth);
   adjustAnimationTimer();
}


// User requested widget to close
void SlideOutWidget::onDeactivated()
{
   S32 currentWidth  = getCurrentDisplayWidth();
   S32 expectedWidth = getTotalDisplayWidth();
   setStartingOffset(0);
   setExpectedWidth(expectedWidth);

   mAnimationTimer.reset(currentWidth / WidgetSpeed, expectedWidth / WidgetSpeed);
   mActivating = false;
}


// Width of widget as currently displayed
S32 SlideOutWidget::getCurrentDisplayWidth() const
{
   if(mActivating && mActivationDirection)
      return mAnimationTimer.getElapsed() * WidgetSpeed + mStartingOffset;
   // else
   return mAnimationTimer.getCurrent() * WidgetSpeed + mStartingOffset;
}


// Width of widget when fully displayed
S32 SlideOutWidget::getTotalDisplayWidth() const
{
   return mAnimationTimer.getPeriod() * WidgetSpeed + mStartingOffset;
}


// Gets run when opening animation is complete
void SlideOutWidget::onWidgetOpened() 
{ 
   /* Do nothing */ 
}


// Gets run when closing animation is complete
void SlideOutWidget::onWidgetClosed() 
{ 
   // Do nothing
}  


S32 SlideOutWidget::getWidth() const
{
   return mWidth;
}


F32 SlideOutWidget::getInsideEdge() const
{
   return getFraction() * (mStartingOffset - mWidth);
}


S32 SlideOutWidget::getStartingOffset()
{
   return mStartingOffset;
}


void SlideOutWidget::adjustAnimationTimer()
{
   // Will almost always be true; only false when menu as shown is already wider than it needs to be
   mActivationDirection = (mWidth > mStartingOffset);

   U32 distToGo = abs(mWidth - mStartingOffset);
   U32 timeNeeded = U32(distToGo / WidgetSpeed);

   mAnimationTimer.reset(timeNeeded);  
}


void SlideOutWidget::setStartingOffset(S32 startingOffset)
{
   mStartingOffset = startingOffset;
}


void SlideOutWidget::setExpectedWidth(S32 width)
{
   mWidth = width;
}


// Only needed for multi-stage widgets like the LoadoutHelper that have multiple widths
void SlideOutWidget::setExpectedWidth_MidTransition(S32 width)
{
   mWidth = width;
   adjustAnimationTimer();
}


bool SlideOutWidget::isOpening() const
{
   return mActivating && mAnimationTimer.getCurrent() > 0;
}


bool SlideOutWidget::isActive() const
{
   return mAnimationTimer.getCurrent() > 0;
}


// Return true if menu is playing the closing animation
bool SlideOutWidget::isClosing() const
{
   return !mActivating && mAnimationTimer.getCurrent() > 0;
}


F32 SlideOutWidget::getFraction() const
{
   return mActivating ? mAnimationTimer.getFraction() : 1 - mAnimationTimer.getFraction();
}


S32 SlideOutWidget::getAnimationTime() const
{
   return mAnimationTimer.getPeriod();
}


void SlideOutWidget::setAnimationTime(U32 period)
{
   mAnimationTimer.setPeriod(period);
}


// Static method
void SlideOutWidget::renderSlideoutWidgetFrame(S32 ulx, S32 uly, S32 width, S32 height, const Color &borderColor)
{
   const S32 CornerSize = 15;      

   S32 left   = ulx;
   S32 right  = ulx + width;
   S32 top    = uly;
   S32 bottom = uly + height;

   bool topBox = false, leftBox = false, rightBox = false;

   if(top == 0)
      topBox = true;
   else if(right == DisplayManager::getScreenInfo()->getGameCanvasWidth())
      rightBox = true;
   else if(left == 0)
      leftBox = true;

   Vector<Point> points;

   if(leftBox)  // Clip UR corner -- going CW from top-left corner
   {
      left = -500;      // Make sure that, even with translate, the line extends to left edge of the screen.
                        // This is only an issue with LoadoutHelper at the moment.
      Point p[] = { Point(left, top), Point(right - CornerSize, top),  // Top
                    Point(right, top + CornerSize),                    // Right
                    Point(right, bottom), Point(left, bottom) };       // Bottom

      points = Vector<Point>(p, ARRAYSIZE(p));
   }
   else if(rightBox)              // Clip LL corner -- going CCW from top-right corner
   {
      Point p[] = { Point(right, top), Point(left, top),                       // Top
                    Point(left, bottom - CornerSize),                          // Edge
                    Point(left + CornerSize, bottom), Point(right, bottom) };  // Bottom

      points = Vector<Point>(p, ARRAYSIZE(p));
   }
   else if(topBox)               // Clip LL corner -- going CCW from top-left corner
   {
      Point p[] = { Point(left, top), Point(left, bottom - CornerSize),
                    Point(left + CornerSize, bottom),
                    Point(right, bottom), Point(right, top) };

      points = Vector<Point>(p, ARRAYSIZE(p));
   }
   else
      TNLAssert(false, "Expected one of the above to be true!");


   // Fill
   glColor(Colors::black, 0.70f);
   renderPointVector(&points, GL_TRIANGLE_FAN);

   // Border
   glColor(borderColor);
   renderPointVector(&points, GL_LINE_STRIP);
}



};
