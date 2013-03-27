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

#include "UISlideOutWidget.h"

namespace Zap
{


// Constructor
UISlideOutWidget::UISlideOutWidget()
{
   mActivating = false;

   // To change animation time, simply use the setAnimationTime() method in the child's constructor
   mAnimationTimer.setPeriod(150);    // Transition time, in ms
}


void UISlideOutWidget::idle(U32 deltaT)
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
void UISlideOutWidget::onActivated() 
{
   mAnimationTimer.invert();
   mActivating = true;
}


// User requested widget to close
void UISlideOutWidget::onDeactivated()
{
   mAnimationTimer.invert();
   mActivating = false;
}


void UISlideOutWidget::onWidgetOpened() { /* Do nothing */ }      // Gets run when opening animation is complete
void UISlideOutWidget::onWidgetClosed() { /* Do nothing */ }      // Gets run when closing animation is complete


F32 UISlideOutWidget::getInsideEdge()
{
   // Magic number that seems to work well... no matter that the real menu might be a different width... by
   // using this constant, menus appear at a consistent rate.
   F32 width = 400;     

   return (mActivating ? -mAnimationTimer.getFraction() * width : 
                         (mAnimationTimer.getFraction() - 1) * width);
}


bool UISlideOutWidget::isOpening() const
{
   return mActivating && mAnimationTimer.getCurrent() > 0;
}


bool UISlideOutWidget::isActive() const
{
   return mAnimationTimer.getCurrent() > 0;
}


// Return true if menu is playing the closing animation
bool UISlideOutWidget::isClosing() const
{
   return !mActivating && mAnimationTimer.getCurrent() > 0;
}


F32 UISlideOutWidget::getFraction()
{
   return mActivating ? mAnimationTimer.getFraction() : 1 - mAnimationTimer.getFraction();
}


S32 UISlideOutWidget::getAnimationTime() const
{
   return mAnimationTimer.getPeriod();
}


void UISlideOutWidget::setAnimationTime(U32 period)
{
   mAnimationTimer.setPeriod(period);
}


};