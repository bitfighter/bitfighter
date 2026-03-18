//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "BfObject.h"
#include "tnlBitStream.h"

#include "gtest/gtest.h"

namespace Zap
{

using namespace TNL;

namespace
{
Point roundTripCompressedVelocity(const Point &inputVel, U32 maxVelocity)
{
   BfObject object;
   BitStream stream;
   Point outputVel(0, 0);

   object.writeCompressedVelocity(inputVel, maxVelocity, &stream);
   stream.setBitPosition(0);
   object.readCompressedVelocity(outputVel, maxVelocity, &stream);

   return outputVel;
}
}

TEST(BfObjectTest, CompressedVelocityRoundTripsZeroVelocity)
{
   Point decoded = roundTripCompressedVelocity(Point(0, 0), 100);

   EXPECT_FLOAT_EQ(0.0f, decoded.x);
   EXPECT_FLOAT_EQ(0.0f, decoded.y);
}


TEST(BfObjectTest, CompressedVelocityRoundTripsVelocityWithinCompressedRange)
{
   const Point input(3.0f, 4.0f);  // Magnitude is exactly 5.
   Point decoded = roundTripCompressedVelocity(input, 10);

   EXPECT_NEAR(5.0f, decoded.len(), 0.05f);

   Point inputDirection = input;
   inputDirection.normalize();

   Point decodedDirection = decoded;
   decodedDirection.normalize();

   EXPECT_GT(inputDirection.dot(decodedDirection), 0.999f);
}


TEST(BfObjectTest, CompressedVelocityRoundTripsVelocityBeyondCompressedRange)
{
   const Point input(123.25f, -987.5f);
   Point decoded = roundTripCompressedVelocity(input, 100);

   EXPECT_FLOAT_EQ(input.x, decoded.x);
   EXPECT_FLOAT_EQ(input.y, decoded.y);
}

};
