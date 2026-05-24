//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#pragma once

#ifndef _LOADOUT_INDICATOR_H_
#define _LOADOUT_INDICATOR_H_

#include "AToBScroller.h"     // Parent
#include "LoadoutTracker.h"
#include "XtankShape.h"       // For XtankDesign

using namespace TNL;

namespace Zap { namespace UI {

static const S32 IndicatorFontSize = 15;
static const S32 IndicatorHorizPadding = 5;       // Gap between text and box
static const S32 IndicatorVertPadding = 3;


class LoadoutIndicator : public AToBScroller
{
   typedef AToBScroller Parent;

private:
   LoadoutTracker mCurrLoadout;
   LoadoutTracker mPrevLoadout;

   VehicleDesign mXtankDesign;    // Current xtank design (bodyIndex < 0 = none)

public:
   LoadoutIndicator();     // Constructor
   virtual ~LoadoutIndicator();

   static const S32 LoadoutIndicatorTopPos = 10;    // Gap between top of screen and top of indicator
   static const S32 LoadoutIndicatorLeftPos = 10;
   static const S32 LoadoutIndicatorHeight = IndicatorFontSize + IndicatorVertPadding * 2;
   static const S32 LoadoutIndicatorBottomPos = LoadoutIndicatorTopPos + LoadoutIndicatorHeight + 1;  // 1 accounts for line widths and such

   void reset();
   void newLoadoutHasArrived(const LoadoutTracker &loadout);
   void setActiveWeapon(U32 weaponIndex);

   void setModulePrimary(ShipModule module, bool isActive);
   void setModuleSecondary(ShipModule module, bool isActive);

   void setXtankDesign(const VehicleDesign &design);   // Update xtank HUD panel

   const LoadoutTracker *getLoadout() const;

   S32 render(ClientGame *game) const;
   S32 getWidth() const;
};


S32 renderComponentIndicator(S32 x, S32 y, const char *name);  // Renders an indicator at (x, y) for the given name.
S32 getComponentIndicatorWidth(const char *name);			   // Returns the rendered width.



} } // Nested namespace

#endif  // _LOADOUT_INDICATOR_H_
