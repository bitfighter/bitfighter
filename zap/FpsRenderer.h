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

#ifndef _FPS_RENDERER_
#define _FPS_RENDERER_

#include "SlideOutWidget.h"

using namespace TNL;

namespace Zap 
{

class ClientGame;

namespace UI
{

class FpsRenderer : public SlideOutWidget
{
   typedef SlideOutWidget Parent;

private:
   static const S32 FPS_AVG_COUNT = 32;
   bool mFPSVisible;             // Are we displaying FPS info?

   F32 mFPSAvg;

   U32 mPing[FPS_AVG_COUNT];
   F32 mPingAvg;
   
   U32 mRecalcFPSTimer;          // Controls recalcing FPS running average
   U32 mFrameIndex;

   ClientGame *mGame;

   U32 mIdleTimeDelta[FPS_AVG_COUNT];

public:
   FpsRenderer(ClientGame *game);      // Constructor
   virtual ~FpsRenderer();

   void idle(U32 timeDelta);
   void render(S32 canvasWidth) const;
   void toggleVisibility();
};

} } // Nested namespace


#endif

