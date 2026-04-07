//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_XTANK_HELPER_H_
#define _UI_XTANK_HELPER_H_

#include "helperMenu.h"
#include "XtankShape.h"    // for XtankDesign, XtankBody, XtankWeapon

namespace Zap
{


// Vehicle design helper menu: lets the player choose an xtank body and assign
// weapons to each turret slot.  Follows the same two-phase sequential
// selection pattern as LoadoutHelper.
//
// Phase 1:  Select a vehicle body (14 options, keys 1-9, 0, A-D).
// Phase 2+: For each turret slot select a weapon (10 options, keys 0-9).
//
// When all slots have been assigned the design is applied to the local ship
// via GameUserInterface::applyXtankDesign().
class UIXtankHelper : public HelperMenu
{
   typedef HelperMenu Parent;

private:
   // Holds the design being built during the selection process.
   XtankDesign mDesignInProgress;

   // Which phase we are in:
   //   0          = selecting body
   //   1..N       = selecting weapon for slot (mPhase - 1)
   S32 mPhase;

   // Number of turret slots on the selected body (set once the body is chosen).
   S32 mSlotCount;

   // Pre-built overlay item arrays — rebuilt in onActivated() so button-width
   // calculations include the current input mode.
   Vector<OverlayMenuItem> mBodyItems;
   Vector<OverlayMenuItem> mWeaponItems;

   S32 mBodyButtonsWidth;
   S32 mBodyItemsDisplayWidth;
   S32 mWeaponButtonsWidth;
   S32 mWeaponItemsDisplayWidth;

   void buildBodyItems();
   void buildWeaponItems();
   void advanceToNextPhaseOrFinish();  // Move to next weapon slot, or finalise
   void applyDesign();    // Finalise and propagate the chosen design

public:
   UIXtankHelper();
   virtual ~UIXtankHelper();

   HelperMenu::HelperMenuType getType();

   void onActivated();
   void render();
   bool processInputCode(InputCode inputCode);

   void activateHelp(UIManager *uiManager);
};


} /* namespace Zap */
#endif /* _UI_XTANK_HELPER_H_ */
