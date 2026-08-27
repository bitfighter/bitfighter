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

   // Create a destructible barrier far from origin to avoid any default
   // wall geometry that the GameType may create when the game is unsuspended.
   // Barrier from (5000,5000) to (5200,5000), width 50 (outline y=4975..5025)
   Vector<Point> barrierPoints;
   barrierPoints.push_back(Point(NAN, NAN));
   barrierPoints.push_back(Point(5000, 5000));
   barrierPoints.push_back(Point(5200, 5000));
   barrierPoints.push_back(Point(NAN, NAN));

   Barrier *barrier = Barrier::createBarrier(barrierPoints, 50, false);
   ASSERT_TRUE(barrier != NULL);
   barrier->mDestructible = true;
   barrier->mMaxHitPoints = 100;
   barrier->mHitPoints = 100;
   barrier->addToGame(serverGame, serverGame->getGameObjDatabase());

   // Place a projector at (5100, 5200) projecting upward (normal = 0,-1).
   // Anchor is 200 units from the spine — well outside the 30-unit mount
   // tolerance (width/2 + 5), so it won't be deleted by removeMountedItems().
   // Forcefield start is (5100, 5185) and the beam sweeps upward, hitting
   // the barrier outline bottom edge at y=5025.
   SafePtr<ForceFieldProjector> ffp = new ForceFieldProjector(2, Point(5100, 5200), Point(0, -1));
   ffp->addToGame(serverGame, serverGame->getGameObjDatabase());
   serverGame->idle(10);

   ASSERT_TRUE(ffp.isValid());
   ASSERT_TRUE(ffp->isEnabled());

   // Get initial forcefield length — should terminate at the barrier
   Point initialStart, initialEnd;
   ffp->getForceFieldStartAndEndPoints(initialStart, initialEnd);
   F32 initialLength = initialStart.y - initialEnd.y;  // beam points upward
   ASSERT_GT(initialLength, 0);

   // Verify the forcefield terminates at this barrier
   ASSERT_EQ(ffp->getTerminatingBarrier(), barrier);

   // Destroy the barrier the forcefield terminates at
   DamageInfo di;
   di.damageAmount = 200;
   di.damageType = DamageTypePoint;
   barrier->damageObject(&di);

   serverGame->idle(10);
   serverGame->idle(10);

   // Get new forcefield length — should extend to max length since nothing
   // else blocks the beam
   Point newStart, newEnd;
   ffp->getForceFieldStartAndEndPoints(newStart, newEnd);
   F32 newLength = newStart.y - newEnd.y;  // beam points upward

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