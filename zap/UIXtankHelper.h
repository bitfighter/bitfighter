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
// engines, treads, heat sinks, and weapons to each turret slot.  Follows a
// sequential phase-based selection pattern analogous to LoadoutHelper.
//
// Phase 0:  Select a vehicle body (14 options, keys 1-9, 0, A-D).
// Phase 1:  Select engine type   (3 options, keys 1-3).
// Phase 2:  Select tread type    (3 options, keys 1-3).
// Phase 3:  Select heat-sink count (6 options, keys 1-6).
// Phase 4+: For each turret slot select a weapon (10 options, keys 0-9).
//
// When all slots have been assigned the design is applied to the local ship
// via GameUserInterface::applyXtankDesign().
class UIXtankHelper : public HelperMenu
{
   typedef HelperMenu Parent;

private:
   // Phase constants.
   static const S32 PHASE_BODY     = 0;
   static const S32 PHASE_ENGINE   = 1;
   static const S32 PHASE_TREADS   = 2;
   static const S32 PHASE_HEATSINK = 3;
   static const S32 PHASE_WEAPONS  = 4;  // weapon slots start here

   // Holds the design being built during the selection process.
   XtankDesign mDesignInProgress;

   // Current phase (see phase constants above).
   S32 mPhase;

   // Number of turret slots on the selected body (set once the body is chosen).
   S32 mSlotCount;

   // Pre-built overlay item arrays — rebuilt in onActivated().
   Vector<OverlayMenuItem> mBodyItems;
   Vector<OverlayMenuItem> mEngineItems;
   Vector<OverlayMenuItem> mTreadItems;
   Vector<OverlayMenuItem> mHeatSinkItems;
   Vector<OverlayMenuItem> mWeaponItems;

   S32 mBodyButtonsWidth;
   S32 mBodyItemsDisplayWidth;
   S32 mEngineButtonsWidth;
   S32 mEngineItemsDisplayWidth;
   S32 mTreadButtonsWidth;
   S32 mTreadItemsDisplayWidth;
   S32 mHeatSinkButtonsWidth;
   S32 mHeatSinkItemsDisplayWidth;
   S32 mWeaponButtonsWidth;
   S32 mWeaponItemsDisplayWidth;

   void buildBodyItems();
   void buildEngineItems();
   void buildTreadItems();
   void buildHeatSinkItems();
   void buildWeaponItems();
   void advanceToNextPhaseOrFinish();  // Move to next phase, or finalise
   void applyDesign();                 // Finalise and propagate the chosen design

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
