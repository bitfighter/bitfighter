#include "FpsRenderer.h"
#include "gameType.h"
#include "ClientGame.h"
#include "RenderUtils.h"
#include "OpenglUtils.h"
#include "stringUtils.h"
#include "ScreenInfo.h"
#include "ClientInfo.h"
#include "UI.h"                  // For horizMargin, vertMargin

#include <math.h>

namespace Zap { 

extern ScreenInfo gScreenInfo;

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


void FpsRenderer::render() const
{
   if(!mFPSVisible && !isClosing())
      return;

   FontManager::pushFontContext(FontManager::HUDContext);

   const S32 xpos = gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin - getInsideEdge();
   const S32 fontSize = 20;
   const S32 fontGap = 5;

   glColor(Colors::white);
   drawStringfr(xpos, UserInterface::vertMargin,                      fontSize, "%1.0f fps", mFPSAvg);
   glColor(Colors::yellow);
   drawStringfr(xpos, UserInterface::vertMargin + fontSize + fontGap, fontSize, "%1.0f ms",  mPingAvg);
   
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