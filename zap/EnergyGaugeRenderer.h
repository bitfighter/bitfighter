//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _ENERGY_GAUGE_RENDERER_
#define _ENERGY_GAUGE_RENDERER_

#include "tnlTypes.h"
#include "UI.h"

using namespace TNL;

namespace Zap {   namespace UI
{

class EnergyGaugeRenderer
{
public:
   static const S32 GuageWidth = 200;
   static const S32 GaugeHeight = 20;
   static const S32 SafetyLineExtend = 4;      // How far the safety line extends above/below the main bar
   static const S32 GaugeLeftMargin   = UserInterface::horizMargin;
   static const S32 GaugeBottomMargin = UserInterface::vertMargin;

   static void render(S32 energy);
};

} } // Nested namespace


#endif

