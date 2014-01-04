//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "gameType.h"
#include "ServerGame.h"
#include "EngineeredItem.h"

#include "TestUtils.h"

#include "gtest/gtest.h"

#include <string>
#include <cmath>

namespace Zap
{

TEST(ServerGameTest, ProcessEmptyLevelLine)
{
   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(""));

   ServerGame g(addr, settings, levelSource, false, false);
   GameType gt;
   gt.addToGame(&g, g.getGameObjDatabase());

   // Empty level lines caused crashes at one point
   g.loadLevelFromString(g.toLevelCode() + "\r\n\r\n", g.getGameObjDatabase());
}


TEST(ServerGameTest, KillStreakTests)
{
   ServerGame *game = newServerGame();
   GameType *gt = new GameType();      // Cleaned up in game destructor
   gt->addToGame(game, game->getGameObjDatabase());

   GameConnection conn;
   conn.setObjectMovedThisGame(true);     // Hacky way to avoid getting disconnected when the game is over, which will 
                                          // cause the tests to crash.  Will probably find a better way as we develop further.
   FullClientInfo *ci = new FullClientInfo(game, &conn, "Noman", ClientInfo::ClassHuman);      // Cleaned up somewhere
   conn.setClientInfo(ci);

   LevelInfo levelInfo("Level", BitmatchGame);     // Need a levelInfo for when we change levels

   game->addClient(ci);
   game->addLevel(levelInfo);
   
   game->setGameTime(1.0f / 60.0f); // 1 second, in minutes

   ASSERT_EQ(0, game->getClientInfo(0)->getKillStreak());
   game->getClientInfo(0)->addKill();
   ASSERT_EQ(1, game->getClientInfo(0)->getKillStreak());
   game->idle(1000);                // 1 second, in ms... game ends

   U32 timeDelta = 1000;
   TNLAssert(timeDelta < ServerGame::MaxTimeDelta, "Reduce timeDelta, please!");

   S32 iters = (S32)ceil((F32)ServerGame::LevelSwitchTime / (F32)timeDelta) - 1;

   // Idle for 4000ms more... in 1000 ms chunks because of timeDelta limitations in ServerGame
   for(S32 i = 0; i < iters; i++)
      game->idle(timeDelta);

   // New game has begun... kill streak should be reset to 0
   ASSERT_EQ(0, game->getClientInfo(0)->getKillStreak());

   delete game;
}

TEST(ServerGameTest, LittleStory) 
{
   ServerGame *serverGame = newServerGame();

   GameType *gt = new GameType();    // Will be deleted in serverGame destructor
   gt->addToGame(serverGame, serverGame->getGameObjDatabase());

   ASSERT_TRUE(serverGame->isSuspended());    // ServerGame starts suspended
   serverGame->unsuspendGame(false);         

   // When adding objects to the game, use new and a pointer -- the game will 
   // delete defunct objects, so a reference will not work.
   SafePtr<Ship> ship = new Ship;
   ship->addToGame(serverGame, serverGame->getGameObjDatabase());

   ASSERT_EQ(ship->getPos(), Point(0,0));     // By default, the ship starts at 0,0
   ship->setMove(Move(0,0));
   serverGame->idle(10);
   ASSERT_EQ(ship->getPos(), Point(0,0));     // When processing move of 0,0, we expect the ship to stay put

   ship->setMove(Move(1,0));                  // Length 1 = max speed; moves stay active until replaced

   // Test that we can simulate several ticks, and the ship advances every cycle
   for(S32 i = 0; i < 20; i++)
   {
      Point prevPos = ship->getPos();
      serverGame->idle(10);                   // when i == 16 this locks up... why?
      ASSERT_NE(ship->getPos(), prevPos);    
   }

   // Note -- ship is over near (71, 0)


   // Uh oh, here comes a turret!  (will be deleted in serverGame destructor)
   Turret *t = new Turret(2, Point(71, -100), Point(0, 1));    // Turret is below the ship, pointing up
   t->addToGame(serverGame, serverGame->getGameObjDatabase());

   bool shipDeleted = false;
   for(S32 i = 0; i < 100; i++)
   {
      ship->setMove(Move(0,0));
      serverGame->idle(100);
      if(!ship)
      {
         shipDeleted = true;
         break;
      }
   }
   ASSERT_TRUE(shipDeleted);     // Ship was killed, and object was cleaned up

   delete serverGame;
}


TEST(ServerGameTest, LoadoutManagementTests)
{
   ServerGame *serverGame = newServerGame();
   GameType *gt = new GameType();    // Cleaned up by database
   gt->addToGame(serverGame, serverGame->getGameObjDatabase());

   Ship *s = new Ship();             // Cleaned up by database
   s->addToGame(serverGame, serverGame->getGameObjDatabase());

   // Tests to ensure that currently selected weapon stays the same when changing loadout
   s->setLoadout(LoadoutTracker("Shield,Repair,Burst,Phaser,Bouncer"));        // Set initial loadout
   s->selectWeapon(2);                                                         // Make bouncers active weapon
   s->setLoadout(LoadoutTracker("Armor,Sensor,Phaser,Bouncer,Seeker"), false); // Set loadout in noisy mode
   EXPECT_EQ(s->getActiveWeapon(), WeaponPhaser);
   s->setLoadout(LoadoutTracker("Armor,Shield,Triple,Mine,Bouncer"), true);    // Set loadout in silent mode
   EXPECT_EQ(s->getActiveWeapon(), WeaponTriple);
   s->setLoadout(LoadoutTracker("Armor,Shield,Triple,Phaser,Mine"), false);    // Set loadout in noisy mode
   EXPECT_EQ(s->getActiveWeapon(), WeaponTriple);                              // Bouncer not in loadout, should select first weap (Triple)
   s->selectWeapon(2);                                                         // Select 3rd weapon, Mine
   EXPECT_EQ(s->getActiveWeapon(), WeaponMine);                                // Confirm we've selected it
   s->setLoadout(LoadoutTracker("Armor,Shield,Seeker,Phaser,Triple"), true);   // Set loadout in silent mode
   EXPECT_EQ(s->getActiveWeapon(), WeaponSeeker);                              // Mine not in loadout, should select first weap (Seeker)

   // Tests to ensure that resource items get dropped when changing loadout away from engineer.  We'll also add a flag
   // and verify that the flag is not similarly dropped.  These cleaned up by database.
   ResourceItem *r = new ResourceItem();
   FlagItem     *f = new FlagItem();

   r->addToGame(serverGame, serverGame->getGameObjDatabase());
   f->addToGame(serverGame, serverGame->getGameObjDatabase());

   s->setLoadout(LoadoutTracker("Engineer,Shield,Triple,Mine,Bouncer"));       // Ship has engineer
   r->mountToShip(s);
   f->mountToShip(s);
   EXPECT_TRUE(s->isCarryingItem(ResourceItemTypeNumber));
   EXPECT_TRUE(s->isCarryingItem(FlagTypeNumber));
   s->setLoadout(LoadoutTracker("Turbo,Shield,Triple,Mine,Bouncer"), false);   // Ship does not have engineer
   EXPECT_FALSE(s->isCarryingItem(ResourceItemTypeNumber));
   EXPECT_TRUE(s->isCarryingItem(FlagTypeNumber));

   // Same test, in silent mode
   s->setLoadout(LoadoutTracker("Engineer,Shield,Triple,Mine,Bouncer"));       // Ship has engineer
   r->mountToShip(s);
   EXPECT_TRUE(s->isCarryingItem(ResourceItemTypeNumber));
   EXPECT_TRUE(s->isCarryingItem(FlagTypeNumber));
   s->setLoadout(LoadoutTracker("Turbo,Shield,Triple,Mine,Bouncer"), true);    // Ship does not have engineer
   EXPECT_FALSE(s->isCarryingItem(ResourceItemTypeNumber));
   EXPECT_TRUE(s->isCarryingItem(FlagTypeNumber));

   delete serverGame;
}


};
