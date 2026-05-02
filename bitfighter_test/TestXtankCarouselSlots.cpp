//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIXtankHelper.h"

#include "gtest/gtest.h"

namespace Zap
{

   // WEAPONS = 9, so non-weapon phases are 0..8.
   // Each test calls the static overload directly — no live UIXtankHelper needed.

   static void fill(Phase curPhase, S32 curWeaponSlot, S32 slotsInUse, Phase phase[], S32 wSlot[])
   {
      UIXtankHelper::computeCarouselSlots(curPhase, curWeaponSlot, slotsInUse, phase, wSlot, 5);
   }


   // 1. Center slot always reflects the current phase.
   TEST(XtankComputeCarouselSlots, CenterSlotMatchesCurrentPhase)
   {
      Phase phase[5];  S32 wSlot[5];
      fill(Phase::ENGINE, 0, 1, phase, wSlot);
      EXPECT_EQ(Phase::ENGINE, phase[2]);
      EXPECT_EQ(-1, wSlot[2]);
   }


   // 2. Adjacent slots step one phase back and forward from center.
   TEST(XtankComputeCarouselSlots, AdjacentSlotsStepOnePhaseEachSide)
   {
      Phase phase[5];  S32 wSlot[5];
      fill(Phase::TREADS, 0, 1, phase, wSlot);
      EXPECT_EQ(Phase::ENGINE, phase[1]);   // one back
      EXPECT_EQ(Phase::TREADS, phase[2]);   // center
      EXPECT_EQ(Phase::ARMOR, phase[3]);   // one forward
   }


   // 3. At BODY (first phase) slots clamp — nothing before the start.
   TEST(XtankComputeCarouselSlots, ClampAtStart)
   {
      Phase phase[5];  S32 wSlot[5];
      fill(Phase::BODY, 0, 1, phase, wSlot);
      // Slots 0 and 1 should both clamp to BODY.
      EXPECT_EQ(Phase::NONE, phase[0]);
      EXPECT_EQ(Phase::NONE, phase[1]);
      EXPECT_EQ(Phase::BODY, phase[2]);
      EXPECT_EQ(Phase::ENGINE, phase[3]);
   }


   // 4. At the last weapon slot, slots clamp — nothing past the end.
   TEST(XtankComputeCarouselSlots, ClampAtEnd)
   {
      Phase phase[5];  S32 wSlot[5];
      // 3 weapon slots, currently on slot 3 (the last one).
      fill(Phase::WEAPONS, 3, 3, phase, wSlot);    // curPhase, curWeaponSlot, slotsInUse

      // Current panel
      EXPECT_EQ(Phase::WEAPONS, phase[2]);
      EXPECT_EQ(3, wSlot[2]);

      // Next panel
      EXPECT_EQ(Phase::WEAPONS, phase[3]);
      EXPECT_EQ(4, wSlot[3]);

      EXPECT_EQ(Phase::NONE, phase[4]);
      //EXPECT_EQ(3, wSlot[4]);

   }


   // 5. Non-weapon phases carry weaponSlot == -1.
   TEST(XtankComputeCarouselSlots, NonWeaponPhasesHaveNoWeaponSlot)
   {
      Phase phase[5];  S32 wSlot[5];
      fill(Phase::SUSPENSION, 0, 1, phase, wSlot);
      for(S32 i = 0; i < 5; i++)
         if(phase[i] != Phase::WEAPONS)
            EXPECT_EQ(-1, wSlot[i]) << "slot " << i;
   }


   // 6. Entering WEAPONS: center slot shows weapon sub-slot, not -1.
   TEST(XtankComputeCarouselSlots, WeaponPhaseCenter)
   {
      Phase phase[5];  S32 wSlot[5];
      fill(Phase::WEAPONS, 0, 3, phase, wSlot);
      EXPECT_EQ(Phase::WEAPONS, phase[2]);
      EXPECT_EQ(0, wSlot[2]);
   }


   // 7. Adjacent weapon sub-slots appear on neighbouring display slots.
   TEST(XtankComputeCarouselSlots, WeaponSubSlotNavigation)
   {
      Phase phase[5];  S32 wSlot[5];
      // 3 weapon slots, currently on slot 1 (middle).
      fill(Phase::WEAPONS, 1, 3, phase, wSlot);
      EXPECT_EQ(Phase::WEAPONS, phase[1]);
      EXPECT_EQ(0, wSlot[1]);   // weapon sub-slot 0 is one back
      EXPECT_EQ(Phase::WEAPONS, phase[2]);
      EXPECT_EQ(1, wSlot[2]);   // weapon sub-slot 1 is center
      EXPECT_EQ(Phase::WEAPONS, phase[3]);
      EXPECT_EQ(2, wSlot[3]);   // weapon sub-slot 2 is one forward
   }


   // 8. Transition from last non-weapon phase into first weapon sub-slot.
   TEST(XtankComputeCarouselSlots, BoundaryBetweenHeatsinkAndWeapons)
   {
      Phase phase[5];  S32 wSlot[5];
      fill(Phase::HEATSINK, 0, 2, phase, wSlot);
      // Slot 2 = HEATSINK (current), slot 3 = first weapon sub-slot.
      EXPECT_EQ(Phase::HEATSINK, phase[2]);
      EXPECT_EQ(-1, wSlot[2]);
      EXPECT_EQ(Phase::WEAPONS, phase[3]);
      EXPECT_EQ(0, wSlot[3]);
   }

} // namespace Zap

