//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_XTANK_HELPER_H_
#define _UI_XTANK_HELPER_H_

#include "helperMenu.h"
#include "XtankShape.h"    // for XtankDesign, XtankBody, XtankWeapon
#include "tnlVector.h"
#include <string>


namespace Zap
{


// Vehicle design helper menu: lets the player choose an xtank body and assign
// engines, treads, heat sinks, and weapons to each turret slot.  Supports
// bidirectional carousel navigation (Tab/Shift-Tab or arrow keys = phase navigation,
// Enter = confirm, Esc = cancel).
//
// Phase 0:  Select a vehicle body      (14 options, keys 1-9,0,A-D).
// Phase 1:  Select engine type         (16 options).
// Phase 2:  Select tread type          (5 options).
// Phase 3:  Select armor type          (9 options).
// Phase 4:  Allocate armor budget across Front/Back/Left/Right (Up/Down = select side, +/- = shift points).
// Phase 5:  Select suspension type     (4 options).
// Phase 6:  Select bumper type         (4 options).
// Phase 7:  Select special equipment   (12 toggleable options).
// Phase 8:  Select heat-sink count     (6 options).
// Phase 9+: For each turret slot select a weapon.

class UIXtankHelper : public HelperMenu
{
   typedef HelperMenu Parent;

private:
   // Phase constants.
   static const S32 PHASE_BODY        = 0;
   static const S32 PHASE_ENGINE      = 1;
   static const S32 PHASE_TREADS      = 2;
   static const S32 PHASE_ARMOR       = 3;
   static const S32 PHASE_ARMOR_SIDES = 4;  // per-side armor point allocation
   static const S32 PHASE_SUSPENSION  = 5;
   static const S32 PHASE_BUMPERS     = 6;
   static const S32 PHASE_SPECIALS    = 7;
   static const S32 PHASE_HEATSINK    = 8;
   static const S32 PHASE_WEAPONS     = 9;   // final phase category; mWeaponSide selects slot
   static const S32 TOTAL_PHASES      = 10;  // phase count (indices 0..9)

   // Holds the design being built during the selection process.
   XtankDesign mDesignInProgress;

   // Snapshot of the design when the helper was opened — restored on ESC.
   XtankDesign mOriginalDesign;

   // Current phase (see phase constants above).
   S32 mPhase;

   // Number of turret slots on the selected body (set once the body is chosen).
   S32 mSlotCount;

   // Which weapon slot (0-based) is currently being configured in PHASE_WEAPONS.
   S32 mWeaponSide;

   // Pre-built overlay item arrays — rebuilt in onActivated().
   Vector<OverlayMenuItem> mBodyItems;
   Vector<OverlayMenuItem> mEngineItems;
   Vector<OverlayMenuItem> mTreadItems;
   Vector<OverlayMenuItem> mArmorItems;
   Vector<OverlayMenuItem> mArmorSidesItems;  // 4 items: Front/Back/Left/Right
   Vector<OverlayMenuItem> mSuspensionItems;
   Vector<OverlayMenuItem> mBumperItems;
   Vector<OverlayMenuItem> mSpecialsItems;
   Vector<OverlayMenuItem> mHeatSinkItems;
   Vector<OverlayMenuItem> mWeaponItems;

   S32 mBodyButtonsWidth;
   S32 mBodyItemsDisplayWidth;
   S32 mEngineButtonsWidth;
   S32 mEngineItemsDisplayWidth;
   S32 mTreadButtonsWidth;
   S32 mTreadItemsDisplayWidth;
   S32 mArmorButtonsWidth;
   S32 mArmorItemsDisplayWidth;
   S32 mArmorSidesButtonsWidth;
   S32 mArmorSidesItemsDisplayWidth;
   S32 mSuspensionButtonsWidth;
   S32 mSuspensionItemsDisplayWidth;
   S32 mBumperButtonsWidth;
   S32 mBumperItemsDisplayWidth;
   S32 mSpecialsButtonsWidth;
   S32 mSpecialsItemsDisplayWidth;
   S32 mHeatSinkButtonsWidth;
   S32 mHeatSinkItemsDisplayWidth;
   S32 mWeaponButtonsWidth;
   S32 mWeaponItemsDisplayWidth;

   // Index of the currently highlighted (preview) item in the active phase's
   // item list.  Wraps within the valid range.  Cycled by UP/DOWN arrow keys.
   S32 mHighlightedIndex;

   // Carousel transition animation state.
   Timer mTransitionTimer;     // Tracks transition progress (0-1)
   S32 mTransitionFromPhase;   // Previous phase before transition (-1 = no transition)
   bool mTransitionForward;    // true = forward, false = backward

   void buildBodyItems();
   void buildEngineItems();
   void buildTreadItems();
   void buildArmorItems();
   void buildArmorSidesItems();  // 4 static items (Front/Back/Left/Right) for point allocation
   void buildSuspensionItems();
   void buildBumperItems();
   void buildSpecialsItems();
   void buildHeatSinkItems();
   void buildWeaponItems();
   void updateItemColors(Vector<OverlayMenuItem> &items);  // Highlight selected item
   void advanceToNextPhaseOrFinish();  // Move to next phase, or finalise
   void commitHighlightedSelection();  // Save highlighted item into design before navigating
   void navigateForward();             // Commit + move to next phase (carousel forward)
   void navigateBackward();            // Move to previous phase (carousel back)
   void applyDesign();                 // Finalise and propagate the chosen design

   // Returns a human-readable side label ("Front", "Rear", "Left", "Right", or "Center")
   // for a given turret slot on a given body, based on the turret's x/y mount position.
   static const char *getTurretSideLabel(S32 bodyIdx, S32 slot);

   // Restore mHighlightedIndex from mDesignInProgress when entering a phase.
   void setHighlightedIndexForPhase(S32 phase);

   // Returns the menu-panel display width for a given phase.
   S32 widthForPhase(S32 phase) const;

   // Draw the floating preview panel on the right side of the screen.
   void renderPreviewPanel() const;

   // Draw the carousel position dots + arrows inside the preview panel.
   void renderCarouselDots(S32 cx, S32 y) const;

   // Draw combined effective vehicle stats (speed/accel/turn/fire-rate).
   // previewBodyIdx, previewEngIdx, previewTreadIdx, previewHeatSinks are the
   // "what-if" values for whichever phase is currently being previewed.
   void renderFullBuildStats(S32 cx, S32 y,
                             S32 previewBodyIdx, S32 previewEngIdx,
                             S32 previewTreadIdx, S32 previewHeatSinks) const;

   // Draw custom floating-card selector UI (active center card + adjacent cards).
   // transitionFraction: 0.0 = no transition, 1.0 = fully transitioned (for animation)
   void renderFloatingMenus(F32 transitionFraction);

   // Draw a single card at the given screen rect.
   // centerFraction: 0.0=fully adjacent/background, 1.0=fully center/active
   void renderCard(S32 left, S32 top, S32 right, S32 bot, S32 phase, F32 centerFraction) const;

   // Draw detailed stats for the highlighted item (right column of center card).
   void renderItemStatsColumn(S32 left, S32 right, S32 yTop, F32 alpha = 1.0f) const;

   // Returns the item vector for a given phase.
   const Vector<OverlayMenuItem> *getItemsForPhase(S32 phase) const;

   // Returns display label for the selected value of the given phase.
   std::string getSelectedLabelForPhase(S32 phase) const;

   // Returns the item count for the current phase (used by UP/DOWN cycling).
   S32 currentPhaseItemCount() const;

public:
   UIXtankHelper();
   virtual ~UIXtankHelper();

   HelperMenu::HelperMenuType getType();

   void onActivated();
   void render();
   void idle(U32 delta);  // Update transition animation
   bool processInputCode(InputCode inputCode);

   void activateHelp(UIManager *uiManager);
};


} /* namespace Zap */
#endif /* _UI_XTANK_HELPER_H_ */
