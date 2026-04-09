//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Tests that verify client/server visual synchronization for ship movement,
// particularly for xtank vehicles (tank heading angle and speed) and regular
// ship aim-angle propagation across two connected clients.

#include "ship.h"
#include "ServerGame.h"
#include "ClientGame.h"
#include "GameManager.h"
#include "gameType.h"
#include "gridDB.h"

#include "TestUtils.h"
#include "LevelFilesForTesting.h"

#include "gtest/gtest.h"

#include <cmath>

namespace Zap
{

using namespace std;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Find the Ship object in 'game' that belongs to the player named 'playerName'.
// Returns NULL if not found.
static Ship *findShipForPlayer(Game *game, const string &playerName)
{
   ClientInfo *ci = game->findClientInfo(StringTableEntry(playerName.c_str()));
   return ci ? ci->getShip() : NULL;
}


// ---------------------------------------------------------------------------
// Unit tests: PositionMask pack/unpack for xtank vehicles
// ---------------------------------------------------------------------------

// Verify that mTankHeadingAngle and mTankSpeed are transmitted to observer
// clients as part of the regular PositionMask update — not only when the body
// design changes (XtankBodyMask).
//
// Before the fix, a second client observing an xtank vehicle would never
// receive updated heading/speed data during normal movement, causing the hull
// to appear to face the wrong direction on the observer's screen.
TEST(SyncTest, TankHeadingIncludedInPositionMaskPackUnpack)
{
   Ship serverShip, clientShip;

   // Do the initial full update so that clientShip enters xtank mode.
   // We have to set bodyIndex in the move so that processMove() will switch
   // mXtankBodyIndex from -1 to 0, which is what triggers XtankBodyMask.
   Move tankMove;
   tankMove.bodyIndex = 0;          // Spider body
   serverShip.setMove(tankMove);

   // processMove manually drives the server-side physics and body switch.
   // We use ActualState here because that is what the server normally advances.
   serverShip.processMove(BfObject::ActualState);

   ASSERT_EQ(0, serverShip.getXtankBodyIndex()) << "serverShip should be in xtank mode after processMove";

   // Transmit the full initial update (InitialMask covers XtankBodyMask).
   packUnpack(serverShip, clientShip);

   ASSERT_EQ(0, clientShip.getXtankBodyIndex()) << "clientShip should be in xtank mode after initial pack/unpack";

   // ---------- Steering phase ----------
   // Apply a steering move on the server so that mTankHeadingAngle changes.
   Move steerMove;
   steerMove.bodyIndex = 0;
   steerMove.x         = 1.0f;   // Full right turn
   steerMove.time      = 100;    // 100 ms frame

   serverShip.setMove(steerMove);
   serverShip.processMove(BfObject::ActualState);

   F32 serverHeading = serverShip.getTankHeadingAngle();
   F32 serverSpeed   = serverShip.getTankSpeed();

   // The heading should have rotated from the default -pi/2 after one frame of
   // steering (the exact value depends on physics constants).
   EXPECT_NE(-FloatHalfPi, serverHeading) << "Server heading should have changed from default after steering";

   // --- This is the core regression test ---
   // Pack only PositionMask (what the server sends to observer clients every
   // frame the ship is moving/steering) and verify the observer receives
   // the updated heading angle and speed.
   Ship observerShip;
   packUnpack(serverShip, observerShip, Ship::PositionMask);

   EXPECT_NEAR(serverHeading, observerShip.getTankHeadingAngle(), 0.001f)
      << "Observer should receive updated tank heading angle via PositionMask";
   EXPECT_NEAR(serverSpeed, observerShip.getTankSpeed(), 0.5f)
      << "Observer should receive updated tank speed via PositionMask";
}


// Verify that heading/speed are sent when the tank is rotating in place
// (steer input but zero throttle and zero speed).  Before the fix the
// observer always saw the initial heading because PositionMask was never
// set in this case (actual velocity stayed zero).
TEST(SyncTest, TankRotatingInPlaceSendsPositionMask)
{
   Ship serverShip, clientShip;

   // Enter xtank mode, no throttle.
   Move tankMove;
   tankMove.bodyIndex = 0;
   serverShip.setMove(tankMove);
   serverShip.processMove(BfObject::ActualState);
   packUnpack(serverShip, clientShip);

   F32 initialHeading = serverShip.getTankHeadingAngle();

   // Steer right with zero throttle — tank rotates but does not translate.
   Move rotateMove;
   rotateMove.bodyIndex = 0;
   rotateMove.x         = 1.0f;
   rotateMove.y         = 0.0f;
   rotateMove.time      = 200;

   serverShip.setMove(rotateMove);
   serverShip.processMove(BfObject::ActualState);

   F32 newHeading = serverShip.getTankHeadingAngle();
   EXPECT_NE(initialHeading, newHeading) << "Heading should have changed after steering";

   // Pack with PositionMask only and verify the observer gets the new heading.
   packUnpack(serverShip, clientShip, Ship::PositionMask);

   EXPECT_NEAR(newHeading, clientShip.getTankHeadingAngle(), 0.001f)
      << "Observer should receive updated tank heading when tank rotates in place";
}


// Verify that heading/speed are sent during coasting (non-zero speed, no input).
TEST(SyncTest, TankCoastingSpeedSentViaPositionMask)
{
   Ship serverShip, clientShip;

   // Enter xtank mode with forward throttle to build up some speed.
   Move driveMove;
   driveMove.bodyIndex = 0;
   driveMove.y         = -1.0f;  // Forward throttle (W key: move.y = -1 = forward)
   driveMove.time      = 200;

   serverShip.setMove(driveMove);
   serverShip.processMove(BfObject::ActualState);
   serverShip.processMove(BfObject::ActualState); // Two frames to build speed

   packUnpack(serverShip, clientShip);   // Initial sync

   // Now release throttle — tank should be coasting with non-zero speed.
   Move coastMove;
   coastMove.bodyIndex = 0;
   coastMove.x = 0;
   coastMove.y = 0;
   coastMove.time = 100;

   serverShip.setMove(coastMove);
   serverShip.processMove(BfObject::ActualState);

   F32 serverSpeed = serverShip.getTankSpeed();
   EXPECT_GT(serverSpeed, 0.0f) << "Tank should still be coasting (non-zero speed)";

   // Observer should receive the coasting speed via PositionMask.
   packUnpack(serverShip, clientShip, Ship::PositionMask);

   EXPECT_NEAR(serverSpeed, clientShip.getTankSpeed(), 0.5f)
      << "Observer should receive updated tank speed during coasting via PositionMask";
}


// ---------------------------------------------------------------------------
// Integration tests: two connected clients
// ---------------------------------------------------------------------------

// Verifies that when a player using an xtank vehicle steers, the second
// connected client sees the correct hull heading angle.
TEST(SyncTest, TankHeadingPropagatesAcrossTwoClients)
{
   // Create a local game server with two connected clients.
   GamePair gamePair(getLevelCode1(), 2);
   GamePair::idle(10, 30);   // Let ships spawn and initial ghost updates complete

   ServerGame *server   = gamePair.server;
   ClientGame *client1  = gamePair.getClient(0);
   ClientGame *client2  = gamePair.getClient(1);

   const string p1Name = client1->getPlayerName();
   const string p2Name = client2->getPlayerName();

   // Confirm both players have spawned.
   Ship *serverShipP1 = findShipForPlayer(server, p1Name);
   ASSERT_TRUE(serverShipP1 != NULL) << "Player 1 ship not found on server";

   // --- Switch player 1 to xtank mode via addPendingMove ---
   Move tankMove;
   tankMove.bodyIndex = 0;   // Spider body
   tankMove.time      = 32;
   client1->getConnectionToServer()->addPendingMove(&tankMove);
   GamePair::idle(10, 20);   // Allow XtankBodyMask to propagate to client 2

   // Confirm client 2 sees the xtank body.
   Ship *c2ObservedShip = findShipForPlayer(client2, p1Name);
   ASSERT_TRUE(c2ObservedShip != NULL) << "Player 1 ghost not found on client 2";
   EXPECT_EQ(0, c2ObservedShip->getXtankBodyIndex())
      << "Client 2 should see player 1 in xtank mode (body 0)";

   // --- Steer the tank to change heading ---
   Move steerMove;
   steerMove.bodyIndex = 0;
   steerMove.x         = 1.0f;   // Full right steer
   steerMove.y         = 0.0f;
   steerMove.time      = 32;

   // Submit 15 steering frames.
   for(S32 i = 0; i < 15; i++)
   {
      client1->getConnectionToServer()->addPendingMove(&steerMove);
      GamePair::idle(32, 1);
   }
   GamePair::idle(10, 10);   // Extra idle to flush any pending updates

   F32 serverHeading = serverShipP1->getTankHeadingAngle();
   F32 c2Heading     = c2ObservedShip->getTankHeadingAngle();

   // The heading should have rotated noticeably from the default (-pi/2).
   EXPECT_NE(-FloatHalfPi, serverHeading) << "Server should have a different heading after steering";

   EXPECT_NEAR(serverHeading, c2Heading, 0.1f)
      << "Client 2 should see approximately the same tank heading as the server";
}


// Verifies that when a player's xtank vehicle coasts to a stop, the second
// client's view of that vehicle converges to the server's position/speed.
TEST(SyncTest, TankCoastingPropagatesAcrossTwoClients)
{
   GamePair gamePair(getLevelCode1(), 2);
   GamePair::idle(10, 30);

   ServerGame *server  = gamePair.server;
   ClientGame *client1 = gamePair.getClient(0);

   const string p1Name = client1->getPlayerName();

   Ship *serverShipP1 = findShipForPlayer(server, p1Name);
   ASSERT_TRUE(serverShipP1 != NULL);

   // Switch to xtank mode and drive forward for a few frames.
   {
      Move tankMove;
      tankMove.bodyIndex = 0;
      tankMove.y         = -1.0f;   // Forward throttle
      tankMove.time      = 32;
      for(S32 i = 0; i < 10; i++)
      {
         client1->getConnectionToServer()->addPendingMove(&tankMove);
         GamePair::idle(32, 1);
      }
   }
   GamePair::idle(10, 20);   // Let things settle

   // Release throttle — tank starts coasting.
   {
      Move coastMove;
      coastMove.bodyIndex = 0;
      coastMove.x = 0;
      coastMove.y = 0;
      coastMove.time = 32;
      for(S32 i = 0; i < 5; i++)
      {
         client1->getConnectionToServer()->addPendingMove(&coastMove);
         GamePair::idle(32, 1);
      }
   }

   // Wait long enough for the tank to decelerate and for updates to propagate.
   GamePair::idle(10, 30);

   ClientGame *client2      = gamePair.getClient(1);
   Ship       *c2ObservedShip = findShipForPlayer(client2, p1Name);
   ASSERT_TRUE(c2ObservedShip != NULL);

   F32 serverSpeed = serverShipP1->getTankSpeed();
   F32 c2Speed     = c2ObservedShip->getTankSpeed();

   EXPECT_NEAR(serverSpeed, c2Speed, 5.0f)
      << "Client 2 tank speed should be close to server tank speed during coasting";

   // Positions should also match closely.
   Point serverPos = serverShipP1->getRenderPos();
   Point c2Pos     = c2ObservedShip->getActualPos();
   F32   posDiff   = (serverPos - c2Pos).len();
   EXPECT_LT(posDiff, 20.0f)
      << "Client 2 ship position should be close to server position during coasting";
}


// Verifies that the regular ship aim angle (turret / reticle direction) is
// synchronised between clients.  This is a regression guard — the aim angle
// has always been sent via MoveMask but this test makes it explicit.
TEST(SyncTest, ShipAimAnglePropagatesAcrossTwoClients)
{
   GamePair gamePair(getLevelCode1(), 2);
   GamePair::idle(10, 30);

   ServerGame *server  = gamePair.server;
   ClientGame *client1 = gamePair.getClient(0);
   ClientGame *client2 = gamePair.getClient(1);

   const string p1Name = client1->getPlayerName();

   Ship *serverShipP1 = findShipForPlayer(server, p1Name);
   ASSERT_TRUE(serverShipP1 != NULL);

   // Send a move with a specific aim angle (pointing right, 0 radians).
   Move aimMove;
   aimMove.angle = 0.0f;
   aimMove.time  = 32;
   for(S32 i = 0; i < 5; i++)
   {
      client1->getConnectionToServer()->addPendingMove(&aimMove);
      GamePair::idle(32, 1);
   }
   GamePair::idle(10, 10);

   Ship *c2ObservedShip = findShipForPlayer(client2, p1Name);
   ASSERT_TRUE(c2ObservedShip != NULL);

   // The aim angle on the server and client 2 should be the same.
   F32 serverAngle = serverShipP1->getRenderAngle();
   F32 c2Angle     = c2ObservedShip->getRenderAngle();

   EXPECT_NEAR(serverAngle, c2Angle, 0.1f)
      << "Client 2 should see the same aim angle as the server for a regular ship";
}

};  // namespace Zap
