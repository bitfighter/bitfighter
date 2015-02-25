//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAUGE_RENDERER_
#define _GAUGE_RENDERER_

#include "tnlTypes.h"
#include "UI.h"

using namespace TNL;

namespace Zap {   namespace UI
{

class EnergyGaugeRenderer
{
public:
   static void render(S32 energy);
};


class HealthGaugeRenderer
{
public:
   static void render(F32 health);
};


} } // Nested namespace


#endif

