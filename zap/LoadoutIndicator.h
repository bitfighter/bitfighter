//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LOADOUT_INDICATOR_H_
#define _LOADOUT_INDICATOR_H_

#include "AToBScroller.h"     // Parent
#include "LoadoutTracker.h"

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

public:
   LoadoutIndicator();     // Constructor
   virtual ~LoadoutIndicator();

   static const S32 LoadoutIndicatorTopPos = 10;    // Gap between top of screen and top of indicator
   static const S32 LoadoutIndicatorLeftPos = 10;
   static const S32 LoadoutIndicatorHeight = IndicatorFontSize + IndicatorVertPadding * 2;
   static const S32 LoadoutIndicatorBottomPos = LoadoutIndicatorTopPos + LoadoutIndicatorHeight + 1;  // 1 accounts for line widths and such

   void newLoadoutHasArrived(const LoadoutTracker &loadout);
   void setActiveWeapon(U32 weaponIndex);

   void setModulePrimary(ShipModule module, bool isActive);
   void setModuleSecondary(ShipModule module, bool isActive);

   const LoadoutTracker *getLoadout() const;

   S32 render(ClientGame *game) const;
   S32 getWidth() const;
};

} } // Nested namespace

#endif  // _LOADOUT_INDICATOR_H_
