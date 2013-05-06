// Bitfighter Tests

#include "gtest/gtest.h"

#include "BfObject.h"
#include "LoadoutTracker.h"
#include "ServerGame.h"
#include "ship.h"

#include <string>

#ifdef TNL_OS_WIN32
#  include <windows.h>   // For ARRAYSIZE
#endif


using namespace Zap;
namespace Zap
{
void exitToOs(S32 errcode) { TNLAssert(false, "Should never be called!"); }
void shutdownBitfighter()  { TNLAssert(false, "Should never be called!"); };
}


class BfTest : public testing::Test
{
   // Config code can go here
};


TEST_F(BfTest, LoadoutTrackerTests) 
{
   LoadoutTracker t1;
   ASSERT_FALSE(t1.isValid());      // New trackers are not valid
   ASSERT_EQ(t1.toString(), "");   

   // Assign with a vector of items
   U8 items[] = { ModuleSensor, ModuleArmor, WeaponBounce, WeaponPhaser, WeaponBurst };
   Vector<U8> vals(items, ARRAYSIZE(items));

   t1.setLoadout(Vector<U8>(items, ARRAYSIZE(items)));
   ASSERT_EQ(t1.toString(), "Sensor,Armor,Bouncer,Phaser,Burst");

   // Check equality and assignment 
   LoadoutTracker t2;
   ASSERT_NE(t1, t2);
   t2 = t1;
   ASSERT_EQ(t1, t2);
   ASSERT_EQ(t2.toString(), "Sensor,Armor,Bouncer,Phaser,Burst");

   // Check creating from a string
   LoadoutTracker t3("Sensor,Armor,Bouncer,Phaser,Burst");
   ASSERT_EQ(t2, t3);

   // Check dumping to Vector
   Vector<U8> outItems = t3.toU8Vector();
   ASSERT_EQ(outItems.size(), ShipModuleCount + ShipWeaponCount);
   for(S32 i = 0; i < outItems.size(); i++)
      ASSERT_EQ(outItems[i], items[i]);
}

TEST_F(BfTest, LittleStory) 
{
   Address addr;
   GameSettings settings;
   ServerGame serverGame(addr, &settings, false, false);

   ASSERT_TRUE(serverGame.isSuspended());    // ServerGame starts suspended
   serverGame.unsuspendGame(false);          

   Ship ship;
   ship.addToGame(&serverGame, serverGame.getGameObjDatabase());

   ASSERT_EQ(ship.getPos(), Point(0,0));     // By default, the ship starts at 0,0

   ship.setActualVel(Point(100,0));          // Give the ship some velocity

   for(S32 i = 0; i < 20; i++)
   {
      serverGame.idle(10);
      printf("%d %s\n", i, ship.getPos().toString().c_str());
   }

   ASSERT_NE(ship.getPos(), Point(0,0));     // After idling, ship should have moved
}


int main(int argc, char **argv) 
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


