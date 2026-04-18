//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_XTANK_HELPER_H_
#define _UI_XTANK_HELPER_H_

#include "helperMenu.h"
#include "XtankShape.h"    // for XtankDesign, XtankBody, XtankWeapon
#include "tnlVector.h"


namespace Zap
{


// Vehicle design helper menu: lets the player choose an xtank body and assign
// engines, treads, heat sinks, and weapons to each turret slot.  Supports
// bidirectional carousel navigation (LEFT = go back, RIGHT/ENTER = confirm).
//
// Phase 0:  Select a vehicle body (14 options, keys 1-9, 0, A-D).
// Phase 1:  Select engine type   (options, keys 1-N).
// Phase 2:  Select tread type    (options, keys 1-N).
// Phase 3:  Select heat-sink count (6 options, keys 1-6).
// Phase 4+: For each turret slot select a weapon (options, keys 0-9/A-P).
//
// When all slots have been confirmed the design is applied to the local ship
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

   // Index of the currently highlighted (preview) item in the active phase's
   // item list.  Wraps within the valid range.  Cycled by UP/DOWN arrow keys.
   S32 mHighlightedIndex;

   void buildBodyItems();
   void buildEngineItems();
   void buildTreadItems();
   void buildHeatSinkItems();
   void buildWeaponItems();
   void updateItemColors(Vector<OverlayMenuItem> &items);  // Highlight selected item
   void advanceToNextPhaseOrFinish();  // Move to next phase, or finalise
   void navigateBackward();            // Move to previous phase (carousel back)
   void applyDesign();                 // Finalise and propagate the chosen design

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

   // Returns the item count for the current phase (used by UP/DOWN cycling).
   S32 currentPhaseItemCount() const;

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
