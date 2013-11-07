//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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

