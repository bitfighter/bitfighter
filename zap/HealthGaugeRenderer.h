//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _HEALTH_GAUGE_RENDERER_
#define _HEALTH_GAUGE_RENDERER_

#include "tnlTypes.h"
#include "UI.h"

using namespace TNL;

namespace Zap {   namespace UI
{

class HealthGaugeRenderer
{
public:
   static const S32 GuageWidth = 200;
   static const S32 GaugeHeight = 5;
   static const S32 GaugeLeftMargin   = UserInterface::horizMargin;
   static const S32 GaugeBottomMargin = UserInterface::vertMargin;

   static void render(F32 health);
};

} } // Nested namespace


#endif  // _HEALTH_GAUGE_RENDERER_

