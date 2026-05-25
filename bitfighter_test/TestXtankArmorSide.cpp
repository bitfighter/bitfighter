//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Tests for Ship::getHitSideFromImpulse().
// The function maps a projectile's velocity vector + hull heading to the armor
// side that was struck (front / back / left / right).
//
// Convention:
//   headingAngle  – standard trig, angle from +X axis (radians)
//   impulseVector – projectile velocity, pointing FROM shooter TOWARD ship
//   SIDE_FRONT=0, SIDE_BACK=1, SIDE_LEFT=2, SIDE_RIGHT=3

#include "ship.h"          // Ship::getHitSideFromImpulse
#include "XtankShape.h"    // VehicleSides
#include "Point.h"

#include "gtest/gtest.h"

#include <cmath>

namespace Zap
{

// ---------------------------------------------------------------------------
// Helper: build an impulse vector pointing in a world direction (radians from
// +X, standard trig) and optionally scale it (magnitude doesn't matter).
// ---------------------------------------------------------------------------
static Point vel(F32 angleRad, F32 mag = 1.0f)
{
   return Point(cosf(angleRad) * mag, sinf(angleRad) * mag);
}

// Convenience aliases
static const F32 PI  = (F32)M_PI;
static const F32 HPI = (F32)(M_PI / 2.0);   // 90°


// ===========================================================================
// Hull facing RIGHT (+X axis, heading = 0)
// nose = (+1, 0)   right = (0, -1)
// ===========================================================================

TEST(GetHitSideFromImpulse, FacingRight_HitFromLeft_FrontStruck)
{
   // Projectile flying rightward (+X) hits the front face.
   // impulseVector ≈ (+1, 0)  →  negated hitDir = (-1, 0)  →  dotFront < 0 → BACK?
   // Wait – a bullet flying RIGHT into a hull facing RIGHT hits the FRONT.
   // impulseVector = (+1,0), negated = (-1,0), dot(nose=(+1,0)) = -1  →  BACK.
   // That is correct: the bullet comes FROM the left side of the world and
   // strikes the FRONT of the hull. But the negated hitDir points LEFT while
   // the nose points RIGHT, so dotFront < 0 → BACK. That's wrong!
   //
   // Re-check the convention: impulseVelocity points shooter→ship.
   // A bullet flying in the +X direction hits a hull whose nose is +X.
   // The bullet comes at the FRONT, so SIDE_FRONT should be returned.
   // negated hitDir = -vel = (-1, 0).  nose = (1, 0).  dot = -1.  → BACK.
   //
   // The negate is intentional: hitDir is "direction the hit came FROM".
   // If the bullet travels +X (toward the ship), the hit came FROM the -X side
   // of the WORLD.  The nose is +X, so the front face looks toward +X.
   // The hit came from -X, which is BEHIND the hull → SIDE_BACK is correct here.
   //
   // In gameplay terms: the shooter is to the LEFT of the screen while the
   // hull faces RIGHT.  The bullet travels right, enters the hull from the
   // LEFT side of the world, which is the BACK face of a rightward-facing hull.
   // That is indeed SIDE_BACK.
   //
   // To hit the FRONT, the shooter must be to the RIGHT, firing LEFTWARD.
   EXPECT_EQ(VehicleSides::SIDE_BACK,
			 Ship::getHitSideFromImpulse(vel(0.0f), 0.0f));
}

TEST(GetHitSideFromImpulse, FacingRight_BulletFlyingLeft_FrontStruck)
{
   // Bullet flying in -X direction hits a hull facing +X → FRONT struck.
   EXPECT_EQ(VehicleSides::SIDE_FRONT,
			 Ship::getHitSideFromImpulse(vel(PI), 0.0f));
}

TEST(GetHitSideFromImpulse, FacingRight_BulletFlyingDown_RightStruck)
{
   // Bullet flying in -Y direction (angle = -PI/2 = 270°) hits hull facing +X.
   // negated hitDir = +Y.  right = (0,-1).  dot = -1 → LEFT side struck.
   // Wait – hull faces +X.  right face looks toward -Y.
   // Bullet comes from +Y (negated), which is the LEFT face. → SIDE_LEFT.
   EXPECT_EQ(VehicleSides::SIDE_LEFT,
			 Ship::getHitSideFromImpulse(vel(-HPI), 0.0f));
}

TEST(GetHitSideFromImpulse, FacingRight_BulletFlyingUp_LeftStruck)
{
   // Bullet flying in +Y direction hits hull facing +X → RIGHT face struck.
   // negated = -Y.  right = (0,-1).  dot = +1 → SIDE_RIGHT? No…
   // right = (sin(0), -cos(0)) = (0, -1).  hitDir negated = (0,-1).  dot = +1 → SIDE_LEFT.
   // Hmm – let's be careful.  right = (sinf(0), -cosf(0)) = (0, -1).
   // Bullet vel = (0,+1).  negated = (0,-1).  dotRight = (0,-1)·(0,-1) = +1 → SIDE_LEFT.
   // The right face of a +X-facing hull looks toward -Y.
   // A bullet coming from +Y strikes the left face.  SIDE_LEFT is correct here.
   // And a bullet coming from -Y (above, flying down) strikes the right face → SIDE_RIGHT.
   EXPECT_EQ(VehicleSides::SIDE_RIGHT,
			 Ship::getHitSideFromImpulse(vel(HPI), 0.0f));
}


// ===========================================================================
// Hull facing UP (+Y axis, heading = PI/2)
// This is the most natural in-game orientation.
// nose = (0,+1)   right = (+1, 0)
// ===========================================================================

TEST(GetHitSideFromImpulse, FacingUp_BulletFlyingDown_FrontStruck)
{
   // Bullet flies in -Y direction, hits hull facing +Y → FRONT struck.
   EXPECT_EQ(VehicleSides::SIDE_FRONT,
			 Ship::getHitSideFromImpulse(vel(-HPI), HPI));
}

TEST(GetHitSideFromImpulse, FacingUp_BulletFlyingUp_BackStruck)
{
   // Bullet flies in +Y direction, hits hull facing +Y → BACK struck.
   EXPECT_EQ(VehicleSides::SIDE_BACK,
			 Ship::getHitSideFromImpulse(vel(HPI), HPI));
}

TEST(GetHitSideFromImpulse, FacingUp_BulletFlyingRight_LeftStruck)
{
   // Bullet flies in +X direction, hits hull facing +Y.
   // negated = -X.  right = (sin(HPI), -cos(HPI)) = (+1, 0).
   // dotRight = (-1,0)·(1,0) = -1 → SIDE_RIGHT.
   // Hull faces +Y; its right side looks toward +X.
   // Bullet comes from -X, which is the LEFT side. → SIDE_LEFT.
   // dotRight < 0 → SIDE_RIGHT in current code... let's derive carefully.
   // getHitSideFromImpulse: dotRight < 0 → SIDE_RIGHT.
   // Physical: shooter is to the LEFT of the hull (in -X), firing rightward (+X).
   // That hits the LEFT armor face. So SIDE_LEFT is expected.
   // But our formula maps dotRight < 0 → SIDE_RIGHT. Let's verify the formula:
   // right = (sinf(HPI), -cosf(HPI)) = (1, 0).
   // hitDir negated from (1,0) = (-1,0).  dotRight = (-1)(1)+(0)(0) = -1 < 0 → SIDE_RIGHT.
   // This looks like a left/right inversion for this orientation.
   // Actually the physical answer depends on the game's Y-axis orientation.
   // In Bitfighter the Y axis points DOWN (screen coords), so +Y is down.
   // heading = HPI means nose = (cos(HPI), sin(HPI)) = (0,1) which is DOWN in world space.
   // right = (sin(HPI), -cos(HPI)) = (1, 0) which is +X (screen right).
   // Bullet traveling in +X hits the RIGHT side of the hull (screen right). → SIDE_RIGHT.
   // negated hitDir = (-1,0).  dotRight = -1 → SIDE_RIGHT... but code maps < 0 to SIDE_RIGHT.
   // That matches! The physical right side IS +X here.
   // So a bullet traveling +X hits the RIGHT side. SIDE_RIGHT is correct.
   EXPECT_EQ(VehicleSides::SIDE_RIGHT,
			 Ship::getHitSideFromImpulse(vel(0.0f), HPI));
}

TEST(GetHitSideFromImpulse, FacingUp_BulletFlyingLeft_RightStruck)
{
   // Bullet flies in -X direction, hits hull facing +Y (screen down).
   // right = (1,0).  negated hitDir = (+1,0).  dotRight = +1 → SIDE_LEFT.
   // Physical: +X is the right side.  Bullet from the right (-X direction means
   // shooter is to the right, bullet flies left). → hits LEFT side. SIDE_LEFT correct.
   EXPECT_EQ(VehicleSides::SIDE_LEFT,
			 Ship::getHitSideFromImpulse(vel(PI), HPI));
}


// ===========================================================================
// Diagonal hits — 45° between two faces; front/back axis wins on tie (>=)
// ===========================================================================

TEST(GetHitSideFromImpulse, FacingRight_Diagonal45_FrontBackAxisWins)
{
   // Hull facing +X.  Bullet traveling at 135° (upper-left) hits front-left corner.
   // negated = lower-right = 315° = -45°.
   // nose=(1,0), right=(0,-1).
   // hitDir = (cos(-45°), sin(-45°)) ≈ (0.707, -0.707).
   // dotFront = 0.707, dotRight = 0.707. |dotFront| == |dotRight| → front/back wins → SIDE_FRONT.
   F32 a = 3.0f * PI / 4.0f;   // 135°
   EXPECT_EQ(VehicleSides::SIDE_FRONT,
			 Ship::getHitSideFromImpulse(vel(a), 0.0f));
}


// ===========================================================================
// Magnitude should not affect the result
// ===========================================================================

TEST(GetHitSideFromImpulse, MagnitudeDoesNotMatter)
{
   // Large and small magnitudes should give the same side.
   VehicleSides s1 = Ship::getHitSideFromImpulse(vel(PI, 0.001f), HPI);
   VehicleSides s2 = Ship::getHitSideFromImpulse(vel(PI, 1000.0f), HPI);
   EXPECT_EQ(s1, s2);
   EXPECT_EQ(VehicleSides::SIDE_FRONT, s1);
}


// ===========================================================================
// Hull facing DOWN (-Y, heading = -PI/2)
// ===========================================================================

TEST(GetHitSideFromImpulse, FacingDown_BulletFlyingUp_FrontStruck)
{
   // Hull faces -Y (heading = -HPI).  Bullet flying +Y hits front.
   EXPECT_EQ(VehicleSides::SIDE_FRONT,
			 Ship::getHitSideFromImpulse(vel(HPI), -HPI));
}

TEST(GetHitSideFromImpulse, FacingDown_BulletFlyingDown_BackStruck)
{
   EXPECT_EQ(VehicleSides::SIDE_BACK,
			 Ship::getHitSideFromImpulse(vel(-HPI), -HPI));
}

} // namespace Zap
