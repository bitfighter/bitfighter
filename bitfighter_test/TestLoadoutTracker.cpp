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

   Vector<U8> outItems = rubric.toU8Vector();
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
