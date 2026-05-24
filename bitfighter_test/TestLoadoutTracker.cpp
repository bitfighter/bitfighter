//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LoadoutTracker.h"

#ifdef TNL_OS_WIN32 
#  include <windows.h>        // For ARRAYSIZE def
#endif

#include "gtest/gtest.h"

namespace Zap
{

struct LoadoutTrackerTest : public ::testing::Test
{
   LoadoutTracker tracker; // Test on this one
   LoadoutTracker rubric;  // Compare to this one

   LoadoutTrackerTest()
      : rubric(LoadoutTracker("Sensor,Armor,Bouncer,Phaser,Burst"))
      { }
};


TEST_F(LoadoutTrackerTest, EqualityAndAssignment )
{
   ASSERT_NE(rubric, tracker);
   tracker = rubric;
   ASSERT_EQ(rubric, tracker);
   EXPECT_EQ(tracker.toString(true), "Sensor,Armor,Bouncer,Phaser,Burst");       // Compact mode
   EXPECT_EQ(tracker.toString(false), "Sensor, Armor, Bouncer, Phaser, Burst");  // Pretty mode
}


TEST_F(LoadoutTrackerTest, FromVector) 
{
   U8 items[] = { ModuleSensor, ModuleArmor, WeaponBounce, WeaponPhaser, WeaponBurst };
   
   ASSERT_FALSE(tracker.isValid());      // New trackers are not valid
   ASSERT_EQ(tracker.toString(true), "");   

   Vector<U8> vals(items, ARRAYSIZE(items));

   tracker.setLoadout(Vector<U8>(items, ARRAYSIZE(items)));
   ASSERT_EQ(tracker.toString(true), "Sensor,Armor,Bouncer,Phaser,Burst");
}


TEST_F(LoadoutTrackerTest, ToVector) 
{
   U8 items[] = { ModuleSensor, ModuleArmor, WeaponBounce, WeaponPhaser, WeaponBurst };

   Vector<U8> outItems = rubric.pack();
   ASSERT_EQ(outItems.size(), ShipModuleCount + ShipWeaponCount);
   for(S32 i = 0; i < outItems.size(); i++)
      ASSERT_EQ(outItems[i], items[i]);
}


TEST_F(LoadoutTrackerTest, ToString) 
{
   // Most string functionality tested elsewhere, just a couple more here:
   LoadoutTracker noItems;
   ASSERT_FALSE(noItems.isValid());
   ASSERT_EQ("", noItems.toString(true));
   ASSERT_NE("", noItems.toString(false));      // Expect something different than empty string
}


};


// ---------------------------------------------------------------------------
// hasModule tests
// ---------------------------------------------------------------------------

// A default tracker (all ModuleNone) must not report having any real module.
TEST(LoadoutTrackerHasModuleTest, DefaultTracker_NoModules)
{
   LoadoutTracker t;
   EXPECT_FALSE(t.hasModule(ModuleShield));
   EXPECT_FALSE(t.hasModule(ModuleBoost));
   EXPECT_FALSE(t.hasModule(ModuleSensor));
   EXPECT_FALSE(t.hasModule(ModuleRepair));
   EXPECT_FALSE(t.hasModule(ModuleEngineer));
   EXPECT_FALSE(t.hasModule(ModuleCloak));
   EXPECT_FALSE(t.hasModule(ModuleArmor));
}

// ModuleNone is a sentinel value, not real equipment; hasModule should never
// confirm it even though it is what default slots hold.
TEST(LoadoutTrackerHasModuleTest, ModuleNone_ReturnsFalse)
{
   LoadoutTracker t;
   EXPECT_FALSE(t.hasModule(ModuleNone));
}

// Each valid module is detectable when placed in slot 0.
TEST(LoadoutTrackerHasModuleTest, EachModuleDetectedInSlot0)
{
   for(S32 m = 0; m < ModuleCount; m++)
   {
      LoadoutTracker t;
      t.setModule(0, (ShipModule)m);
      EXPECT_TRUE(t.hasModule((ShipModule)m)) << "module " << m << " not found in slot 0";
   }
}

// Each valid module is detectable when placed in slot 1 (the second slot).
TEST(LoadoutTrackerHasModuleTest, EachModuleDetectedInSlot1)
{
   for(S32 m = 0; m < ModuleCount; m++)
   {
      LoadoutTracker t;
      t.setModule(1, (ShipModule)m);
      EXPECT_TRUE(t.hasModule((ShipModule)m)) << "module " << m << " not found in slot 1";
   }
}

// A module in slot 0 must not make hasModule return true for a different module.
TEST(LoadoutTrackerHasModuleTest, AbsentModule_ReturnsFalse)
{
   LoadoutTracker t;
   t.setModule(0, ModuleShield);
   t.setModule(1, ModuleArmor);

   EXPECT_FALSE(t.hasModule(ModuleBoost));
   EXPECT_FALSE(t.hasModule(ModuleSensor));
   EXPECT_FALSE(t.hasModule(ModuleRepair));
   EXPECT_FALSE(t.hasModule(ModuleEngineer));
   EXPECT_FALSE(t.hasModule(ModuleCloak));
}

// When both slots hold the same module, hasModule must still return true
// (not crash or double-count in a confusing way).
TEST(LoadoutTrackerHasModuleTest, DuplicateModule_StillTrue)
{
   LoadoutTracker t;
   t.setModule(0, ModuleSensor);
   t.setModule(1, ModuleSensor);
   EXPECT_TRUE(t.hasModule(ModuleSensor));
}

// A fully-loaded tracker (string constructor) reports the correct modules
// and correctly rejects the ones that aren't present.
TEST(LoadoutTrackerHasModuleTest, FullLoadout_CorrectModules)
{
   LoadoutTracker t("Sensor,Armor,Bouncer,Phaser,Burst");
   EXPECT_TRUE(t.hasModule(ModuleSensor));
   EXPECT_TRUE(t.hasModule(ModuleArmor));
   EXPECT_FALSE(t.hasModule(ModuleShield));
   EXPECT_FALSE(t.hasModule(ModuleBoost));
   EXPECT_FALSE(t.hasModule(ModuleEngineer));
}

// After reset(), no module should be found.
TEST(LoadoutTrackerHasModuleTest, AfterReset_NoModules)
{
   LoadoutTracker t("Sensor,Armor,Bouncer,Phaser,Burst");
   t.reset();
   for(S32 m = 0; m < ModuleCount; m++)
      EXPECT_FALSE(t.hasModule((ShipModule)m)) << "module " << m << " found after reset";
   EXPECT_FALSE(t.hasModule(ModuleNone));
}


// ---------------------------------------------------------------------------
// hasWeapon tests
// ---------------------------------------------------------------------------

// A default tracker (all WeaponNone) must not report having any real weapon.
TEST(LoadoutTrackerHasWeaponTest, DefaultTracker_NoWeapons)
{
   LoadoutTracker t;
   EXPECT_FALSE(t.hasWeapon(WeaponPhaser));
   EXPECT_FALSE(t.hasWeapon(WeaponBounce));
   EXPECT_FALSE(t.hasWeapon(WeaponTriple));
   EXPECT_FALSE(t.hasWeapon(WeaponBurst));
   EXPECT_FALSE(t.hasWeapon(WeaponSeeker));
   EXPECT_FALSE(t.hasWeapon(WeaponMine));
   EXPECT_FALSE(t.hasWeapon(WeaponTurret));
}

// WeaponNone is a sentinel; hasWeapon must not confirm it.
TEST(LoadoutTrackerHasWeaponTest, WeaponNone_ReturnsFalse)
{
   LoadoutTracker t;
   EXPECT_FALSE(t.hasWeapon(WeaponNone));
}

// Each valid weapon is detectable when placed in each of the three slots.
TEST(LoadoutTrackerHasWeaponTest, EachWeaponDetectedInEachSlot)
{
   for(S32 w = 0; w < WeaponCount; w++)
   {
      for(S32 slot = 0; slot < ShipWeaponCount; slot++)
      {
         LoadoutTracker t;
         t.setWeapon(slot, (WeaponType)w);
         EXPECT_TRUE(t.hasWeapon((WeaponType)w))
            << "weapon " << w << " not found in slot " << slot;
      }
   }
}

// A weapon in one slot must not make hasWeapon return true for a different weapon.
TEST(LoadoutTrackerHasWeaponTest, AbsentWeapon_ReturnsFalse)
{
   LoadoutTracker t;
   t.setWeapon(0, WeaponPhaser);
   t.setWeapon(1, WeaponBounce);
   t.setWeapon(2, WeaponBurst);

   EXPECT_FALSE(t.hasWeapon(WeaponTriple));
   EXPECT_FALSE(t.hasWeapon(WeaponSeeker));
   EXPECT_FALSE(t.hasWeapon(WeaponMine));
   EXPECT_FALSE(t.hasWeapon(WeaponTurret));
}

// When all three slots hold the same weapon, hasWeapon must still return true.
TEST(LoadoutTrackerHasWeaponTest, DuplicateWeapon_StillTrue)
{
   LoadoutTracker t;
   t.setWeapon(0, WeaponBounce);
   t.setWeapon(1, WeaponBounce);
   t.setWeapon(2, WeaponBounce);
   EXPECT_TRUE(t.hasWeapon(WeaponBounce));
}

// A fully-loaded tracker (string constructor) reports the correct weapons
// and correctly rejects the ones that aren't present.
TEST(LoadoutTrackerHasWeaponTest, FullLoadout_CorrectWeapons)
{
   LoadoutTracker t("Sensor,Armor,Bouncer,Phaser,Burst");
   EXPECT_TRUE(t.hasWeapon(WeaponBounce));
   EXPECT_TRUE(t.hasWeapon(WeaponPhaser));
   EXPECT_TRUE(t.hasWeapon(WeaponBurst));
   EXPECT_FALSE(t.hasWeapon(WeaponTriple));
   EXPECT_FALSE(t.hasWeapon(WeaponSeeker));
   EXPECT_FALSE(t.hasWeapon(WeaponMine));
}

// After reset(), no weapon should be found.
TEST(LoadoutTrackerHasWeaponTest, AfterReset_NoWeapons)
{
   LoadoutTracker t("Sensor,Armor,Bouncer,Phaser,Burst");
   t.reset();
   for(S32 w = 0; w < WeaponCount; w++)
      EXPECT_FALSE(t.hasWeapon((WeaponType)w)) << "weapon " << w << " found after reset";
   EXPECT_FALSE(t.hasWeapon(WeaponNone));
}
