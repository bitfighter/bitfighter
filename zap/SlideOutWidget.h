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

   F32 getFraction();               // Get fraction of openness
   virtual S32 getAnimationTime() const;
};


}

#endif
