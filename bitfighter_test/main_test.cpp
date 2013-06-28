// Bitfighter Tests

#include "gtest/gtest.h"

#include "../zap/BfObject.h"
#include "../zap/EngineeredItem.h"
#include "../zap/gameType.h"
#include "../zap/LoadoutTracker.h"
#include "../zap/ServerGame.h"
#include "../zap/ship.h"
#include "../zap/HelpItemManager.h"
#include "../zap/ClientGame.h"
#include "../zap/ClientInfo.h"
#include "../zap/FontManager.h"

#include "SDL.h"
#include "../zap/VideoSystem.h"

#include "../zap/UIEditorMenus.h"
#include "../zap/LoadoutIndicator.h"

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


TEST_F(BfTest, LoadoutIndicatorTests)
{
   Address addr;
   GameSettings settings;

   // Need to initialize FontManager to use ClientGame... use false to avoid hassle of locating font files.
   // False will tell the FontManager to only use internally defined fonts; any TTF fonts will be replaced with Roman.
   FontManager::initialize(&settings, false);   
   ClientGame game(addr, &settings);

   UI::LoadoutIndicator indicator;
   indicator.newLoadoutHasArrived(LoadoutTracker("Turbo,Shield,Triple,Mine,Bouncer"));     // Sets the loadout

   // Make sure the calculated width matches the rendered width
   ASSERT_EQ(indicator.render(&game), indicator.getWidth());
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


TEST_F(BfTest, GameTypeTests)
{
   GameType gt;

   // Check the maxPlayersPerTeam fn
   //                                players--v  v--teams
   ASSERT_EQ(gt.getMaxPlayersPerBalancedTeam( 1, 1), 1);
   ASSERT_EQ(gt.getMaxPlayersPerBalancedTeam( 1, 2), 1);
   ASSERT_EQ(gt.getMaxPlayersPerBalancedTeam(10, 5), 2);
   ASSERT_EQ(gt.getMaxPlayersPerBalancedTeam(11, 5), 3);
}


static void checkQueues(const UI::HelpItemManager &himgr, S32 highSize, S32 lowSize, S32 displaySize, HelpItem displayItem = UnknownHelpItem)
{
   // Check that queue sizes match what we specified
   ASSERT_EQ(himgr.getHighPriorityQueue()->size(), highSize);
   ASSERT_EQ(himgr.getLowPriorityQueue()->size(), lowSize);
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), displaySize);

   // Check if displayItem is being displayed, unless displayItem is UnknownHelpItem
   if(displayItem != UnknownHelpItem && himgr.getHelpItemDisplayList()->size() > 0)
      ASSERT_EQ(himgr.getHelpItemDisplayList()->get(0), displayItem);
}


// Full cycle is PacedTimerPeriod; that is, every PacedTimerPeriod ms, a new item is displayed
// Within that cycle, and item is displayed for HelpItemDisplayPeriod, then faded for HelpItemDisplayFadeTime,
// at which point it is expired and removed from the screen.  Then we must wait until the remainder of 
// PacedTimerPeriod has elapsed before a new item will be shown.  The following methods try to makes sense of that.
static void idleUntilItemExpiredMinusOne(UI::HelpItemManager &himgr, const ClientGame &game)
{
   himgr.idle(himgr.HelpItemDisplayPeriod,       &game);        // Can't combine these periods... needs to be two different idle cycles
   himgr.idle(himgr.HelpItemDisplayFadeTime - 1, &game);        // First cycle to enter fade mode, second to exhaust fade timer
}


// Idles HelpItemDisplayPeriod + HelpItemDisplayFadeTime, long enough for a freshly displayed item to disappear
static void idleUntilItemExpired(UI::HelpItemManager &himgr, const ClientGame &game)
{
   idleUntilItemExpiredMinusOne(himgr, game);
   himgr.idle(1, &game);
}

static void idleRemainderOfFullCycleMinusOne(UI::HelpItemManager &himgr, const ClientGame &game)
{
   himgr.idle(himgr.PacedTimerPeriod - himgr.HelpItemDisplayPeriod - himgr.HelpItemDisplayFadeTime - 1, &game);
}

// Assumes we've idled HelpItemDisplayPeriod + HelpItemDisplayFadeTime, idles remainder of PacedTimerPeriod
static void idleRemainderOfFullCycle(UI::HelpItemManager &himgr, const ClientGame &game)
{
   idleRemainderOfFullCycleMinusOne(himgr, game);
   himgr.idle(1, &game);
}

static void idleFullCycleMinusOne(UI::HelpItemManager &himgr, const ClientGame &game)
{
   idleUntilItemExpired(himgr, game);
   idleRemainderOfFullCycleMinusOne(himgr, game);
}

// Idles full PacedTimerPeriod, broken into strategic parts
static void idleFullCycle(UI::HelpItemManager &himgr, const ClientGame &game)
{
   idleFullCycleMinusOne(himgr, game);
   himgr.idle(1, &game);
}


TEST_F(BfTest, HelpItemManagerTests)
{
   Address addr;
   GameSettings settings;

   // Need to initialize FontManager to use ClientGame... use false to avoid hassle of locating font files.
   // False will tell the FontManager to only use internally defined fonts; any TTF fonts will be replaced with Roman.
   FontManager::initialize(&settings, false);   
   ClientGame game(addr, &settings);

   HelpItem helpItem = NexusSpottedItem;

   /////
   // Here we verify that displayed help items are corretly stored in the INI

   UI::HelpItemManager himgr(&settings);
   ASSERT_EQ(himgr.getAlreadySeenString()[helpItem], 'N');  // Item not marked as seen originally

   himgr.addInlineHelpItem(helpItem);                       // Add up a help item -- should not get added during initial delay period
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Verify no help item is being displayed

   himgr.idle(himgr.InitialDelayPeriod, &game);             // Idle out of initial delay period

   himgr.addInlineHelpItem(helpItem);                       // Requeue helpItem -- initial delay period has expired, so should get added
   ASSERT_EQ(himgr.getHelpItemDisplayList()->get(0), helpItem);

   ASSERT_EQ(himgr.getAlreadySeenString()[helpItem], 'Y');  // Item marked as seen
   ASSERT_EQ(settings.getIniSettings()->helpItemSeenList, himgr.getAlreadySeenString());  // Verify changes made it to the INI

   idleUntilItemExpired(himgr, game);

   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Verify help item is no longer displayed
   himgr.addInlineHelpItem(TeleporterSpotedItem);
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Still in flood control period, new item not added

   himgr.idle(himgr.FloodControlPeriod - himgr.HelpItemDisplayPeriod - himgr.HelpItemDisplayFadeTime, &game);
   himgr.addInlineHelpItem(helpItem);                       // We've already added this item, so it should not be displayed again
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Verify help item is not displayed again
   himgr.addInlineHelpItem(TeleporterSpotedItem);           // Now, new item should be added (we tried adding this above, and it didn't go)
   ASSERT_EQ(himgr.getHelpItemDisplayList()->get(0), TeleporterSpotedItem);   // Verify help item is now visible

   /////
   // Test that high priority queued items are displayed before lower priority items

   himgr = UI::HelpItemManager(&settings);      // Get a new, clean manager
   
   HelpItem highPriorityItem = ControlsKBItem;
   HelpItem lowPriorityItem  = CmdrsMapItem;

   ASSERT_EQ(himgr.getItemPriority(highPriorityItem), UI::HelpItemManager::PacedHigh);    // Prove that these items
   ASSERT_EQ(himgr.getItemPriority(lowPriorityItem),  UI::HelpItemManager::PacedLow);     // have the expected priority

   himgr.addInlineHelpItem(lowPriorityItem);    // Queue both items up
   himgr.addInlineHelpItem(highPriorityItem);

   // Verify that both have been queued -- one item in each queue, and there are no items in the display list
   checkQueues(himgr, 1, 1, 0);

   himgr.idle(himgr.InitialDelayPeriod - 1, &game);
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);

   himgr.idle(1, &game);    // InitialDelayPeriod has expired

   // Verify that our high priority item has been moved to the active display list
   checkQueues(himgr, 0, 1, 1, highPriorityItem);

   himgr.idle(himgr.HelpItemDisplayPeriod, &game); himgr.idle(himgr.HelpItemDisplayFadeTime - 1, &game);  // (can't combine)

   // Verify nothing has changed
   checkQueues(himgr, 0, 1, 1, highPriorityItem);
   
   himgr.idle(1, &game);    // Now, help item should no longer be displayed
   checkQueues(himgr, 0, 1, 0);

   himgr.idle(himgr.PacedTimerPeriod - himgr.HelpItemDisplayPeriod - himgr.HelpItemDisplayFadeTime - 1, &game); // Idle to cusp of pacedTimer expiring
   // Verify nothing has changed
   checkQueues(himgr, 0, 1, 0);
   himgr.idle(1, &game);    // pacedTimer has expired, a new pacedItem will be added to the display list

   // Second message should now be moved from the lowPriorityQueue to the active display list, and will be visible
   checkQueues(himgr, 0, 0, 1, lowPriorityItem);

   // Idle until list is clear
   idleUntilItemExpired(himgr, game);
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);       // No items displayed

   // Verify bug with two high priority paced items preventing the first from being displayed
   // Start fresh with a new HelpItemManager
   himgr = UI::HelpItemManager(&settings);
   // Verify they have HighPaced priority, then queue them up
   ASSERT_EQ(himgr.getItemPriority(ControlsKBItem),      UI::HelpItemManager::PacedHigh);
   ASSERT_EQ(himgr.getItemPriority(ControlsModulesItem), UI::HelpItemManager::PacedHigh);
   himgr.addInlineHelpItem(ControlsKBItem);
   himgr.addInlineHelpItem(ControlsModulesItem);
   checkQueues(himgr, 2, 0, 0);                       // Two high priority items queued, none displayed

   himgr.idle(himgr.InitialDelayPeriod, &game);       // Wait past the intial delay period; first item will be displayed
   checkQueues(himgr, 1, 0, 1, ControlsKBItem);       // One item queued, one displayed

   idleUntilItemExpiredMinusOne(himgr, game);
   checkQueues(himgr, 1, 0, 1, ControlsKBItem);       // One item queued, one displayed
   himgr.idle(1, &game);
   idleRemainderOfFullCycleMinusOne(himgr, game);
   checkQueues(himgr, 1, 0, 0);
   himgr.idle(1, &game);
   checkQueues(himgr, 0, 0, 1, ControlsModulesItem);  // No items queued, one displayed

   idleUntilItemExpired(himgr, game);                 // Clear the decks
   checkQueues(himgr, 0, 0, 0);
   checkQueues(himgr, 0, 0, 0);

   // Check the increasingly complex logic inside moveItemFromQueueToActiveList()
   // We already know that the logic with high-priority and low-priority lists is working under ordinary circumstances
   // So let's ensure it is working for the special situation of AddBotsItem, which should be ignored if there are bots in the game 
   // Remember that we are in mid-cycle, so we need to finish the cycle* before new items are displayed
   himgr.addInlineHelpItem(AddBotsItem);
   himgr.addInlineHelpItem(CmdrsMapItem);
   checkQueues(himgr, 0, 2, 0);  // Both items should be queued, but none displayed

   idleRemainderOfFullCycle(himgr, game);             // *Cycle is completed, ready for a new item
   checkQueues(himgr, 0, 1, 1, AddBotsItem);          // No bots are in game ==> AddBotsItem should be displayed

   // Reset
   idleFullCycle(himgr, game);
   checkQueues(himgr, 0, 0, 1, CmdrsMapItem);
   idleFullCycleMinusOne(himgr, game);
   checkQueues(himgr, 0, 0, 0);

   himgr.clearAlreadySeenList();    // Allows us to add these same items again

   // Add the same two items again
   himgr.addInlineHelpItem(AddBotsItem);
   himgr.addInlineHelpItem(CmdrsMapItem);
   checkQueues(himgr, 0, 2, 0);     // Both items should be queued, none yet displayed

   // Add a bot -- requires us to first add a team
   Team *team = new Team();         // Teams will be deleted by ClientGame destructor
   game.addTeam(team);
   // Create a ClientInfo for a robot -- it will be deleted by ClientGame destructor
   RemoteClientInfo *clientInfo = new RemoteClientInfo(&game, "Robot", true, 0, true, ClientInfo::RoleNone, false, false);
   clientInfo->setTeamIndex(0);        // Assign it to our team
   game.addToClientList(clientInfo);   // Add it to the game
   ASSERT_EQ(game.getBotCount(), 1);   // Confirm there is a bot in the game

   himgr.idle(1, &game);
   checkQueues(himgr, 0, 1, 1, CmdrsMapItem); // With bot in game, AddBotsItem should be bypassed, should see CmdrsMapItem

   idleUntilItemExpired(himgr, game);
   checkQueues(himgr, 0, 1, 0);              // Bot help still not showing
   idleFullCycle(himgr, game);
   checkQueues(himgr, 0, 1, 0);              // Bot help still not showing
   idleFullCycle(himgr, game);
   checkQueues(himgr, 0, 1, 0);              // And again!

   game.removeFromClientList(clientInfo);    // Remove the bot
   ASSERT_EQ(game.getBotCount(), 0);         // Confirm no bots in the game

   idleFullCycleMinusOne(himgr, game);       // Not exact, just a big chunk of time
   checkQueues(himgr, 0, 0, 1, AddBotsItem); // With no bot, AddBotsItem will be displayed, though not necessarily immediately
}


TEST_F(BfTest, ClientGameTests) 
{
   Address addr;
   GameSettings settings;

   // Need to initialize FontManager to use ClientGame... use false to avoid hassle of locating font files.
   // False will tell the FontManager to only use internally defined fonts; any TTF fonts will be replaced with Roman.
   FontManager::initialize(&settings, false);   
   ClientGame game(addr, &settings);

   // TODO: Add some tests
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


