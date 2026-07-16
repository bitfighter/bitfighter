//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "barrier.h"
#include "gameType.h"
#include "ServerGame.h"
#include "EngineeredItem.h"

#include "TestUtils.h"

#include "gtest/gtest.h"

#include <string>

namespace Zap
{

TEST(BarrierTest, ForceFieldRecomputesAfterTerminatingWallDestroyed)
{
   ServerGame *serverGame = newServerGame();
   GameType *gt = new GameType();
   gt->addToGame(serverGame, serverGame->getGameObjDatabase());
   serverGame->unsuspendGame(false);

   // Create two destructible barriers forming a continuous line:
   // Barrier A from (100,200) to (300,200), Barrier B from (300,200) to (500,200)

   Vector<Point> barrierPointsA;
   barrierPointsA.push_back(Point(NAN, NAN));
   barrierPointsA.push_back(Point(100, 200));
   barrierPointsA.push_back(Point(300, 200));
   barrierPointsA.push_back(Point(500, 200));   // post = next barrier's start

   Barrier *barrierA = Barrier::createBarrier(barrierPointsA, 50, false);
   ASSERT_TRUE(barrierA != NULL);
   barrierA->mDestructible = true;
   barrierA->mMaxHitPoints = 100;
   barrierA->mHitPoints = 100;
   barrierA->addToGame(serverGame, serverGame->getGameObjDatabase());

   Vector<Point> barrierPointsB;
   barrierPointsB.push_back(Point(100, 200));   // pre = previous barrier's end
   barrierPointsB.push_back(Point(300, 200));
   barrierPointsB.push_back(Point(500, 200));
   barrierPointsB.push_back(Point(NAN, NAN));

   Barrier *barrierB = Barrier::createBarrier(barrierPointsB, 50, false);
   ASSERT_TRUE(barrierB != NULL);
   barrierB->mDestructible = true;
   barrierB->mMaxHitPoints = 100;
   barrierB->mHitPoints = 100;
   barrierB->addToGame(serverGame, serverGame->getGameObjDatabase());

   // Mount a forcefield projector on barrier A's midpoint, projecting toward barrier B
   SafePtr<ForceFieldProjector> ffp = new ForceFieldProjector(2, Point(200, 200), Point(1, 0));
   ffp->addToGame(serverGame, serverGame->getGameObjDatabase());
   serverGame->idle(10);

   ASSERT_TRUE(ffp.isValid());
   ASSERT_TRUE(ffp->isEnabled());

   // Get initial forcefield length
   Point initialStart, initialEnd;
   ffp->getForceFieldStartAndEndPoints(initialStart, initialEnd);
   F32 initialLength = initialEnd.x - initialStart.x;

   // Destroy barrier B (the one the forcefield terminates at)
   DamageInfo di;
   di.damageAmount = 200;
   di.damageType = DamageTypePoint;
   barrierB->damageObject(&di);

   serverGame->idle(10);
   serverGame->idle(10);

   // Get new forcefield length
   Point newStart, newEnd;
   ffp->getForceFieldStartAndEndPoints(newStart, newEnd);
   F32 newLength = newEnd.x - newStart.x;

   // Forcefield should have extended through the gap to max length
   EXPECT_GT(newLength, initialLength)
      << "Forcefield did not extend after its terminating wall was destroyed";
   EXPECT_NEAR(newLength, ForceField::MAX_FORCEFIELD_LENGTH, 10.0f)
      << "Forcefield should extend to max length since no other walls remain";

   delete serverGame;
}


TEST(BarrierTest, DestructibleWallRemovesMountedItems)
{
   // Create a server game
   ServerGame *serverGame = newServerGame();

   GameType *gt = new GameType();    // Will be deleted in serverGame destructor
   gt->addToGame(serverGame, serverGame->getGameObjDatabase());
   serverGame->unsuspendGame(false);

   // Create a destructible normal barrier (width 50) with spine from (100,200) to (300,200)
   // Points: [pre, start, end, post] — pre/post are dummy (NAN,NAN) for a standalone segment
   Vector<Point> barrierPoints;
   barrierPoints.push_back(Point(NAN, NAN));   // pre
   barrierPoints.push_back(Point(100, 200));   // start
   barrierPoints.push_back(Point(300, 200));   // end
   barrierPoints.push_back(Point(NAN, NAN));   // post

   Barrier *barrier = Barrier::createBarrier(barrierPoints, 50, false);
   ASSERT_TRUE(barrier != NULL);

   barrier->mDestructible = true;
   barrier->mMaxHitPoints = 100;
   barrier->mHitPoints = 100;
   barrier->addToGame(serverGame, serverGame->getGameObjDatabase());

   // Create a turret mounted at the midpoint of the barrier spine (200, 200)
   SafePtr<Turret> turret = new Turret(2, Point(200, 200), Point(0, 1));
   turret->addToGame(serverGame, serverGame->getGameObjDatabase());

   // Verify the turret is in the game
   serverGame->idle(10);
   ASSERT_TRUE(turret.isValid());

   // Create a forcefield projector mounted at another point on the spine
   SafePtr<ForceFieldProjector> ffp = new ForceFieldProjector(2, Point(150, 200), Point(0, 1));
   ffp->addToGame(serverGame, serverGame->getGameObjDatabase());

   // Enable the projector so it creates its forcefield
   ffp->onEnabled();
   serverGame->idle(10);
   ASSERT_TRUE(ffp.isValid());

   // Now destroy the barrier by dealing damage
   DamageInfo di;
   di.damageAmount = 200;   // More than enough to kill it
   di.damageType = DamageTypePoint;
   barrier->damageObject(&di);

   // Idle to let object deletions propagate
   serverGame->idle(10);
   serverGame->idle(10);

   // Both the turret and forcefield projector should be removed
   ASSERT_FALSE(turret.isValid()) << "Turret was not removed when its mount wall was destroyed";
   ASSERT_FALSE(ffp.isValid()) << "ForceFieldProjector was not removed when its mount wall was destroyed";

   delete serverGame;
}


TEST(BarrierTest, DestructiblePolyWallRemovesMountedItems)
{
   // Create a server game
   ServerGame *serverGame = newServerGame();

   GameType *gt = new GameType();
   gt->addToGame(serverGame, serverGame->getGameObjDatabase());
   serverGame->unsuspendGame(false);

   // Create a destructible polywall (solid) — a simple square
   Vector<Point> polyPoints;
   polyPoints.push_back(Point(100, 100));
   polyPoints.push_back(Point(300, 100));
   polyPoints.push_back(Point(300, 200));
   polyPoints.push_back(Point(100, 200));

   Barrier *barrier = Barrier::createBarrier(polyPoints, 1, true);   // solid = true
   ASSERT_TRUE(barrier != NULL);

   barrier->mDestructible = true;
   barrier->mMaxHitPoints = 100;
   barrier->mHitPoints = 100;
   barrier->addToGame(serverGame, serverGame->getGameObjDatabase());

   // Create a turret "mounted" at a point on the polywall edge (200, 100) — the top edge
   SafePtr<Turret> turret = new Turret(2, Point(200, 100), Point(0, -1));
   turret->addToGame(serverGame, serverGame->getGameObjDatabase());

   serverGame->idle(10);
   ASSERT_TRUE(turret.isValid());

   // Destroy the barrier
   DamageInfo di;
   di.damageAmount = 200;
   di.damageType = DamageTypePoint;
   barrier->damageObject(&di);

   // Let deletions propagate
   serverGame->idle(10);
   serverGame->idle(10);

   ASSERT_FALSE(turret.isValid()) << "Turret was not removed when its polywall mount was destroyed";

   delete serverGame;
}

}; // namespace Zap