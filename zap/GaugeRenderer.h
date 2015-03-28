//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAUGE_RENDERER_
#define _GAUGE_RENDERER_

#include "tnlTypes.h"
#include "RenderManager.h"
#include "UI.h"

using namespace TNL;

namespace Zap {   namespace UI
{

class GaugeRenderer: RenderManager
{
public:
   static const S32 GaugeLeftMargin = UserInterface::horizMargin;
   static const S32 GaugeWidth = 200;
   static const S32 SafetyLineExtend = 4;      // How far the safety line extends above/below the main bar

   static void render(F32 ether, F32 maxEther, const F32 colors[], S32 bottomMargin, S32 height, F32 safetyThresh = -1);
};


class EnergyGaugeRenderer
{
public:
   static const S32 GaugeHeight = 15;
   static const S32 GaugeBottomMargin = UserInterface::vertMargin + 10;

   static void render(S32 energy);
};


class HealthGaugeRenderer
{
public:
   static void render(F32 health);
};


} } // Nested namespace


#endif

