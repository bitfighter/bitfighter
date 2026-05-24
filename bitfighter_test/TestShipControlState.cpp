//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Tests that verify Ship::writeControlState() and Ship::readControlState() are
// true mirrors of one another -- i.e. a round-trip through a BitStream produces
// the identical ship state that was written.

#include "ship.h"
#include "XtankShape.h"
#include "tnlBitStream.h"

#include "gtest/gtest.h"

#include <cmath>

namespace Zap
{

using namespace TNL;

// Helper: write control state from `src` into a BitStream, then read it back
// into `dst`.  The stream is rewound between the two operations.
static void roundTripControlState(Ship &src, Ship &dst)
{
   BitStream stream;
   src.writeControlState(&stream);
   stream.setBitPosition(0);
   dst.readControlState(&stream);
}

// ---------------------------------------------------------------------------
// Bitfighter ship (no xtank mode)
// ---------------------------------------------------------------------------

TEST(ShipControlStateTest, BitfighterShip_RoundTrip)
{
   Ship src, dst;

   // Position and velocity
   src.setPos(ActualState, Point(123.0f, 456.0f));
   src.setVel(ActualState, Point(-7.5f, 3.25f));

   roundTripControlState(src, dst);

   EXPECT_FLOAT_EQ(dst.getActualPos().x, 123.0f);
   EXPECT_FLOAT_EQ(dst.getActualPos().y, 456.0f);
   EXPECT_FLOAT_EQ(dst.getActualVel().x, -7.5f);
   EXPECT_FLOAT_EQ(dst.getActualVel().y, 3.25f);

}

// ---------------------------------------------------------------------------
// Xtank bodies: Lightcycle (first, index 0) and Panzy (last, COUNT-1)
// ---------------------------------------------------------------------------

static void setXtankBody(Ship &ship, XtankBody body)
{
   ship.setXtankBodyForTest(body);
}

TEST(ShipControlStateTest, Lightcycle_RoundTrip)
{
   Ship src, dst;
   setXtankBody(src, XtankBody::Lightcycle);

   // Give the ship a specific heading and speed so the test is non-trivial
   src.setTankHeadingAngle(1.23f);
   src.setTankSpeed(200.5f);
   src.setPos(ActualState, Point(10.0f, 20.0f));
   src.setVel(ActualState, Point(5.0f, -3.0f));

   roundTripControlState(src, dst);

   EXPECT_EQ(dst.getXtankBody(), XtankBody::Lightcycle);
   EXPECT_NEAR(dst.getTankHeadingAngle(), 1.23f, 1e-5f);
   EXPECT_NEAR(dst.getTankSpeed(),        200.5f, 1e-5f);
   EXPECT_FLOAT_EQ(dst.getActualPos().x, 10.0f);
   EXPECT_FLOAT_EQ(dst.getActualPos().y, 20.0f);
}

TEST(ShipControlStateTest, Panzy_RoundTrip)
{
   Ship src, dst;

   src.setTankHeadingAngle(-0.78f);
   src.setTankSpeed(-50.0f);   // reversing
   src.setPos(ActualState, Point(-300.0f, 99.0f));
   src.setVel(ActualState, Point(-10.0f, 0.0f));

   roundTripControlState(src, dst);

   EXPECT_NEAR(dst.getTankSpeed(),        -50.0f, 1e-5f);
}

// ---------------------------------------------------------------------------
// All xtank bodies: verify every value in XtankBody survives a round-trip
// without aliasing (the off-by-one bug would corrupt body 0 -> -1).
// ---------------------------------------------------------------------------

TEST(ShipControlStateTest, AllXtankBodies_RoundTrip)
{
   for(S32 i = 0; i < VehicleBodyCount; i++)
   {
      XtankBody body = (XtankBody)i;

      Ship src, dst;
      setXtankBody(src, body);

      // Unique heading per body so aliased values would be caught
      const F32 heading = (F32)i * 0.2f;
      src.setTankHeadingAngle(heading);

      roundTripControlState(src, dst);

      EXPECT_EQ(dst.getXtankBody(), body)
         << "Body index " << i << " did not survive round-trip";
      EXPECT_NEAR(dst.getTankHeadingAngle(), heading, 1e-5f)
         << "Heading for body index " << i << " did not survive round-trip";
   }
}

// ---------------------------------------------------------------------------
// CooldownNeeded flag survives round-trip in both ship modes
// ---------------------------------------------------------------------------

TEST(ShipControlStateTest, CooldownNeeded_Bitfighter)
{
   Ship src, dst;
   src.setCooldownNeeded(true);
   roundTripControlState(src, dst);
   EXPECT_TRUE(dst.getCooldownNeeded());
}

TEST(ShipControlStateTest, CooldownNeeded_Xtank)
{
   Ship src, dst;
   setXtankBody(src, XtankBody::Lightcycle);
   src.setCooldownNeeded(true);
   roundTripControlState(src, dst);
   EXPECT_TRUE(dst.getCooldownNeeded());
}

};  // namespace Zap
