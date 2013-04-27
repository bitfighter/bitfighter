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

#include "SlideOutWidget.h"
#include "ScreenInfo.h"          // For gScreenInfo def
#include "Point.h"
#include "tnlVector.h"
#include "Colors.h"
#include "OpenglUtils.h"


namespace Zap
{


// Constructor
SlideOutWidget::SlideOutWidget()
{
   mActivating = false;

   // To change animation time, simply use the setAnimationTime() method in the child's constructor
   mAnimationTimer.setPeriod(150);    // Transition time, in ms
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
   mAnimationTimer.invert();
   mActivating = true;
}


// User requested widget to close
void SlideOutWidget::onDeactivated()
{
   mAnimationTimer.invert();
   mActivating = false;
}


void SlideOutWidget::onWidgetOpened() { /* Do nothing */ }      // Gets run when opening animation is complete
void SlideOutWidget::onWidgetClosed() { /* Do nothing */ }      // Gets run when closing animation is complete


F32 SlideOutWidget::getInsideEdge() const
{
   // Magic number that seems to work well... no matter that the real menu might be a different width... by
   // using this constant, menus appear at a consistent rate.
   F32 width = 400;     

   return (mActivating ? -mAnimationTimer.getFraction() * width : 
                         (mAnimationTimer.getFraction() - 1) * width);
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


F32 SlideOutWidget::getFraction()
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



extern ScreenInfo gScreenInfo;

void SlideOutWidget::renderSlideoutWidgetFrame(S32 ulx, S32 uly, S32 width, S32 height, const Color &borderColor) const
{
   const S32 CORNER_SIZE = 15;      

   const S32 left   = ulx;
   const S32 right  = ulx + width;
   const S32 top    = uly;
   const S32 bottom = uly + height;

   bool topBox = false, leftBox = false, rightBox = false;

   if(top == 0)
      topBox = true;
   else if(right == gScreenInfo.getGameCanvasWidth())
      rightBox = true;
   else if(left == 0)
      leftBox = true;

   Vector<Point> points;

   if(leftBox)  // Clip UR corner -- going CW from top-left corner
   {
      Point p[] = { Point(left, top), Point(right - CORNER_SIZE, top),  // Top
                    Point(right, top + CORNER_SIZE),                    // Right
                    Point(right, bottom), Point(left, bottom) };        // Bottom

      points = Vector<Point>(p, ARRAYSIZE(p));
   }
   else if(rightBox)              // Clip LL corner -- going CCW from top-right corner
   {
      Point p[] = { Point(right, top), Point(left, top),                        // Top
                    Point(left, bottom - CORNER_SIZE),                          // Edge
                    Point(left + CORNER_SIZE, bottom), Point(right, bottom) };  // Bottom

      points = Vector<Point>(p, ARRAYSIZE(p));
   }
   else if(topBox)               // Clip LL corner -- going CCW from top-left corner
   {
      Point p[] = { Point(left, top), Point(left, bottom - CORNER_SIZE),
                    Point(left + CORNER_SIZE, bottom),
                    Point(right, bottom), Point(right, top) };

      points = Vector<Point>(p, ARRAYSIZE(p));
   }
   else
      TNLAssert(false, "Expected one of the above to be true!");


   // Fill
   glColor(Colors::black, 0.70f);
   renderPointVector(&points, GL_POLYGON);

   // Border
   glColor(borderColor);
   renderPointVector(&points, GL_LINE_STRIP);
}



};
