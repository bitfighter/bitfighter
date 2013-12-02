//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "FpsRenderer.h"

#include "DisplayManager.h"
#include "ClientGame.h"
#include "FontManager.h"
#include "barrier.h"

#include "Colors.h"

#include "RenderUtils.h"
#include "OpenglUtils.h"

namespace Zap { 

namespace UI {


// Constructor
FpsRenderer::FpsRenderer(ClientGame *game)
{
   mGame = game;

   mFPSAvg = 0;
   mPingAvg = 0;

   mRecalcFPSTimer = 0;

   mFPSVisible = false;

   mFrameIndex = 0;

   for(S32 i = 0; i < FPS_AVG_COUNT; i++)
   {
      mIdleTimeDelta[i] = 50;
      mPing[i] = 100;
   }
}

// Destructor
FpsRenderer::~FpsRenderer()
{
   // Do nothing
}


void FpsRenderer::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   if(mFPSVisible)        // Only bother if we're displaying the value...
   {
      if(timeDelta > mRecalcFPSTimer)
      {
         U32 sum = 0, sumping = 0;

         for(S32 i = 0; i < FPS_AVG_COUNT; i++)
         {
            sum += mIdleTimeDelta[i];
            sumping += mPing[i];
         }

         mFPSAvg = (1000 * FPS_AVG_COUNT) / F32(sum);
         mPingAvg = F32(sumping) / 32;
         mRecalcFPSTimer += 750;
      }
      else
         mRecalcFPSTimer -= timeDelta;
   }


   U32 indx = mFrameIndex % FPS_AVG_COUNT;
   mIdleTimeDelta[indx] = timeDelta;

   if(mGame->getConnectionToServer())
      mPing[indx] = (U32)mGame->getConnectionToServer()->getRoundTripTime();

   mFrameIndex++;
}


void FpsRenderer::render(S32 canvasWidth) const
{
   if(!mFPSVisible && !isClosing())
      return;

   U32 visibleVertices = 0;
   if(mGame->getLocalPlayerShip())
   {
      Point shipPos = mGame->getLocalPlayerShip()->getPos();
      Point vis(DisplayManager::getScreenInfo()->getDrawAreaWidth(), DisplayManager::getScreenInfo()->getDrawAreaHeight());
      Rect visibleRect(
         shipPos.x - vis.x / 2,
         shipPos.y - vis.y / 2,
         shipPos.x + vis.x / 2,
         shipPos.y + vis.y / 2
      );

      // increment by two because each segment is two points
      for(S32 i = 0; i < Barrier::mRenderLineSegments.size(); i += 2)
      {
         if(visibleRect.contains(Barrier::mRenderLineSegments[i]))
            visibleVertices++;
      }
   }

   FontManager::pushFontContext(FPSContext);

   static const S32 horizMargin = 10;
   static const S32 vertMargin = 10;

   const S32 xpos = canvasWidth - horizMargin - S32(getInsideEdge());
   const S32 fontSize = 20;
   const S32 fontGap = 5;

   glColor(Colors::white);
   drawStringfr(xpos, vertMargin,                      fontSize, "%1.0f fps", mFPSAvg);
   glColor(Colors::yellow);
   drawStringfr(xpos, vertMargin + fontSize + fontGap, fontSize, "%1.0f ms",  mPingAvg);

   // vertex display is green at zero and red at 1000 or more visible vertices
   glColor4f(visibleVertices / 1000.0f, 1.0f - visibleVertices / 1000.0f, 0.0f, 1);
   drawStringfr(xpos, vertMargin + 2 * (fontSize + fontGap), fontSize, "%d vts",  visibleVertices);
   
   FontManager::popFontContext();
}


void FpsRenderer::toggleVisibility()
{
   mFPSVisible = !mFPSVisible;

   if(mFPSVisible)
      onActivated();
   else
      onDeactivated();
}


} }      // Nested namespaces
