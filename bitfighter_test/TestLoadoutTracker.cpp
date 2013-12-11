//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LoadoutTracker.h"
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
   ASSERT_EQ(tracker.toString(), "Sensor,Armor,Bouncer,Phaser,Burst");
}


TEST_F(LoadoutTrackerTest, FromVector) 
{
   U8 items[] = { ModuleSensor, ModuleArmor, WeaponBounce, WeaponPhaser, WeaponBurst };
   
   ASSERT_FALSE(tracker.isValid());      // New trackers are not valid
   ASSERT_EQ(tracker.toString(), "");   

   Vector<U8> vals(items, ARRAYSIZE(items));

   tracker.setLoadout(Vector<U8>(items, ARRAYSIZE(items)));
   ASSERT_EQ(tracker.toString(), "Sensor,Armor,Bouncer,Phaser,Burst");
}


TEST_F(LoadoutTrackerTest, ToVector) 
{
   U8 items[] = { ModuleSensor, ModuleArmor, WeaponBounce, WeaponPhaser, WeaponBurst };

   Vector<U8> outItems = rubric.toU8Vector();
   ASSERT_EQ(outItems.size(), ShipModuleCount + ShipWeaponCount);
   for(S32 i = 0; i < outItems.size(); i++)
      ASSERT_EQ(outItems[i], items[i]);
}

   
};
