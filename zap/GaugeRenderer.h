//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAUGE_RENDERER_
#define _GAUGE_RENDERER_

#include "UI.h"

#include "tnlTypes.h"
#include "DisplayManager.h"

using namespace TNL;

namespace Zap {
   namespace UI
   {

      class GaugeRenderer
      {
      public:
         static const S32 GaugeWidth = 200;
         //static const S32 GaugeBottomMargin = UserInterface::vertMargin + 10;
         static const S32 GaugeLeftMargin = UserInterface::horizMargin;
         static const S32 getGaugeBottom() {
            return DisplayManager::getScreenInfo()->getGameCanvasHeight() - UserInterface::vertMargin;
         }
         static const S32 SafetyLineExtend = 4;      // How far the safety line extends above/below the main bar

         static void render(S32 energy, F32 health);
      };


      class EnergyGaugeRenderer : public GaugeRenderer
      {
      public:
         static const S32 GaugeHeight = 15;
         static void render(S32 energy, S32 gaugeBottom);
      };


      class HealthGaugeRenderer : public GaugeRenderer
      {
      public:
         static const S32 GaugeHeight = 5;
         static void render(F32 health, S32 gaugeBottom);
      };


   }
} // Nested namespace


#endif
