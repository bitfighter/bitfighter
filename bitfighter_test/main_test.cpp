// Bitfighter Tests

#include "gtest/gtest.h"

#include "../zap/BfObject.h"
#include "../zap/EngineeredItem.h"
#include "../zap/gameType.h"
#include "../zap/LoadoutTracker.h"
#include "../zap/ServerGame.h"
#include "../zap/ship.h"


#include "tnlNetObject.h"
#include "tnlGhostConnection.h"
#include "tnlPlatform.h"

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


static void packUnpack(Move move1, Move &move2)
{
   PacketStream stream;    // Create a stream

   move1.pack(&stream, NULL, false);   // Write the move
   stream.setBitPosition(0);           // Move the stream's pointer back to the beginning
   move2.unpack(&stream, false);       // Read the move
}


// Generic pack/unpack tester -- can feed it any class that supports pack/unpack
template <class T>
void packUnpack(T input, T &output, U32 mask = 0xFFFFFFFF)
{
   BitStream stream;       
   GhostConnection conn;
   
   output.markAsGhost(); 

   input.packUpdate(&conn, mask, &stream);   // Write the object
   stream.setBitPosition(0);                 // Move the stream's pointer back to the beginning
   output.unpackUpdate(&conn, &stream);      // Read the object back
}


TEST_F(BfTest, LoadoutManagementTests)
{
   Address addr;
   GameSettings settings;
   ServerGame serverGame(addr, &settings, false, false);
   GameType gt;
   gt.addToGame(&serverGame, serverGame.getGameObjDatabase());

   Ship s;
   s.addToGame(&serverGame, serverGame.getGameObjDatabase());

   // Tests to ensure that currently selected weapon stays the same when changing loadout
   s.setLoadout(LoadoutTracker("Shield,Repair,Burst,Phaser,Bouncer"));        // Set initial loadout
   s.selectWeapon(2);                                                         // Make bouncers active weapon
   s.setLoadout(LoadoutTracker("Armor,Sensor,Phaser,Bouncer,Seeker"), false); // Set loadout in noisy mode
   ASSERT_EQ(s.getActiveWeapon(), WeaponBounce);                              // Bouncer should still be active weapon
   s.setLoadout(LoadoutTracker("Armor,Shield,Triple,Mine,Bouncer"), true);    // Set loadout in silent mode
   ASSERT_EQ(s.getActiveWeapon(), WeaponBounce);                              // Bouncer should _still_ be active weapon
   s.setLoadout(LoadoutTracker("Armor,Shield,Triple,Phaser,Mine"), false);    // Set loadout in noisy mode
   ASSERT_EQ(s.getActiveWeapon(), WeaponTriple);                              // Bouncer not in loadout, should select first weap (Triple)
   s.selectWeapon(2);                                                         // Select 3rd weapon, Mine
   ASSERT_EQ(s.getActiveWeapon(), WeaponMine);                                // Confirm we've selected it
   s.setLoadout(LoadoutTracker("Armor,Shield,Seeker,Phaser,Triple"), true);   // Set loadout in silent mode
   ASSERT_EQ(s.getActiveWeapon(), WeaponSeeker);                              // Mine not in loadout, should select first weap (Seeker)

   // Tests to ensure that resource items get dropped when changing loadout away from engineer.  We'll also add a flag
   // and verify that the flag is not similarly dropped.
   ResourceItem r;
   FlagItem f;

   r.addToGame(&serverGame, serverGame.getGameObjDatabase());
   f.addToGame(&serverGame, serverGame.getGameObjDatabase());

   s.setLoadout(LoadoutTracker("Engineer,Shield,Triple,Mine,Bouncer"));       // Ship has engineer
   r.mountToShip(&s);
   f.mountToShip(&s);
   ASSERT_TRUE(s.isCarryingItem(ResourceItemTypeNumber));
   ASSERT_TRUE(s.isCarryingItem(FlagTypeNumber));
   s.setLoadout(LoadoutTracker("Turbo,Shield,Triple,Mine,Bouncer"), false);   // Ship does not have engineer
   ASSERT_FALSE(s.isCarryingItem(ResourceItemTypeNumber));
   ASSERT_TRUE(s.isCarryingItem(FlagTypeNumber));

   // Same test, in silent mode
   s.setLoadout(LoadoutTracker("Engineer,Shield,Triple,Mine,Bouncer"));       // Ship has engineer
   r.mountToShip(&s);
   ASSERT_TRUE(s.isCarryingItem(ResourceItemTypeNumber));
   ASSERT_TRUE(s.isCarryingItem(FlagTypeNumber));
   s.setLoadout(LoadoutTracker("Turbo,Shield,Triple,Mine,Bouncer"), true);    // Ship does not have engineer
   ASSERT_FALSE(s.isCarryingItem(ResourceItemTypeNumber));
   ASSERT_TRUE(s.isCarryingItem(FlagTypeNumber));
}


TEST_F(BfTest, ShipTests)
{
   Ship serverShip, clientShip;
   ASSERT_TRUE(serverShip.isServerCopyOf(clientShip));   // New ships start out equal

   // Set some shippy stuff on serverShip
   serverShip.setRenderPos(Point(100,100));
   serverShip.setLoadout(LoadoutTracker("Shield,Repair,Burst,Phaser,Bouncer"));

   ASSERT_FALSE(serverShip.isServerCopyOf(clientShip));  // After updating server copy, ships no longer equal

   packUnpack(serverShip, clientShip);                   // Transmit server details to client

   ASSERT_TRUE(serverShip.isServerCopyOf(clientShip));   // Ships should be equal again
}


TEST_F(BfTest, MoveTests)
{
   Move move1, move2;
   
   // Demonstrate testing of a basic pack/unpack cycle... but prepare basically already does this so I'm not 
   // sure this test is really that interesting
   move1.set(0.125f, 0.75f, 33.3f);    
   move1.prepare();
   packUnpack(move1, move2);
   ASSERT_TRUE(move1.isEqualMove(&move2)); 

   // An area of concern by an earlier dev is that angles might get transmitted incorreclty.  I agree that the math
   // is confusing, so let's create some tests to verify that prepare() normalizes the input angle to between
   // 0 and 2pi, which will then be handled properly by writeInt (and nicely keeps angles sane).

   // Obvious cases
   move1.angle = FloatHalfPi;             move1.prepare();  ASSERT_EQ(move1.angle, FloatHalfPi);                     
   move1.angle = FloatPi + FloatHalfPi;   move1.prepare();  ASSERT_EQ(move1.angle, FloatPi + FloatHalfPi);         

   // Negative angles
   move1.angle = -FloatHalfPi;            move1.prepare();  ASSERT_EQ(move1.angle, FloatPi + FloatHalfPi);  // -1/4 turn = 3/4 turn  
   move1.angle = -FloatPi - FloatHalfPi;  move1.prepare();  ASSERT_EQ(move1.angle, FloatHalfPi);            // -3/4 turn = 1/4 turn  

   // Exactly one turn
   move1.angle =  Float2Pi;               move1.prepare();  ASSERT_EQ(move1.angle, 0);  // Wrap exactly once, pos. dir
   move1.angle = -Float2Pi;               move1.prepare();  ASSERT_EQ(move1.angle, 0);  // Wrap exactly once, neg. dir

   // Large angles
   move1.angle = Float2Pi + FloatHalfPi;  move1.prepare();  ASSERT_EQ(move1.angle, FloatHalfPi);            // Wrap in pos. dir         
   move1.angle = -Float2Pi - FloatHalfPi; move1.prepare();  ASSERT_EQ(move1.angle, FloatPi + FloatHalfPi);  // Wrap in neg. dir 

   // Really large angles -- we'll never see these in the game
   move1.angle = 432 * Float2Pi;          move1.prepare();  ASSERT_EQ(move1.angle, 0);  // Big angles 
   move1.angle =  -9 * Float2Pi;          move1.prepare();  ASSERT_EQ(move1.angle, 0);  // Big neg. angles 
} 


TEST_F(BfTest, LittleStory) 
{
   Address addr;
   GameSettings settings;
   { ServerGame serverGame(addr, &settings, false, false); }
   ServerGame serverGame(addr, &settings, false, false);

   GameType gt;
   gt.addToGame(&serverGame, serverGame.getGameObjDatabase());

   ASSERT_TRUE(serverGame.isSuspended());    // ServerGame starts suspended
   serverGame.unsuspendGame(false);          

   // When adding objects to the game, use new and a pointer -- the game will 
   // delete defunct objects, so a reference will not work.
   SafePtr<Ship> ship = new Ship;
   ship->addToGame(&serverGame, serverGame.getGameObjDatabase());

   ASSERT_EQ(ship->getPos(), Point(0,0));     // By default, the ship starts at 0,0
   ship->setMove(Move(0,0));
   serverGame.idle(10);
   ASSERT_EQ(ship->getPos(), Point(0,0));     // When processing move of 0,0, we expect the ship to stay put

   ship->setMove(Move(1,0));                  // Length 1 = max speed; moves stay active until replaced

   // Test that we can simulate several ticks, and the ship advances every cycle
   for(S32 i = 0; i < 20; i++)
   {
      Point prevPos = ship->getPos();
      serverGame.idle(10);                   // when i == 16 this locks up... why?
      ASSERT_NE(ship->getPos(), prevPos);    
   }

   // Note -- ship is over near (71, 0)


   // Uh oh, here comes a turret!
   Turret t(2, Point(71, -100), Point(0, 1));    // Turret is below the ship, pointing up
   t.addToGame(&serverGame, serverGame.getGameObjDatabase());

   bool shipDeleted = false;
   for(S32 i = 0; i < 100; i++)
   {
      ship->setMove(Move(0,0));
      serverGame.idle(100);
      if(!ship)
      {
         shipDeleted = true;
         break;
      }
   }
   ASSERT_TRUE(shipDeleted);     // Ship was killed, and object was cleaned up
}


int main(int argc, char **argv) 
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


