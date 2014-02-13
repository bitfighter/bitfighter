//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "move.h"
#include "tnlBitStream.h"
#include "gtest/gtest.h"

namespace Zap
{

using namespace TNL;

struct MoveTest : public ::testing::Test
{
   Move move1;
   Move move2;  
};


TEST_F(MoveTest, PackUnpack)
{
   Move move1, move2;

   // Demonstrate testing of a basic pack/unpack cycle... but prepare basically
   // already does this so I'm not sure this test is really that interesting
   move1.set(0.125f, 0.75f, 33.3f);
   move1.prepare();

   PacketStream stream;                // Create a stream
   move1.pack(&stream, NULL, false);   // Write the move
   stream.setBitPosition(0);           // Move the stream's pointer back to the beginning
   move2.unpack(&stream, false);       // Read the move

   ASSERT_TRUE(move1.isEqualMove(&move2));
}


// The Move angle should always be between -pi and pi.  This conforms with
// the output of the arc-tangent of a triangles coordinates and atan2()
// We neeed to verify that prepare() keeps this output consistent, even
// with angles of greater magnitude in either direction
TEST_F(MoveTest, Simple)
{
   // Obvious cases
   move1.angle = FloatHalfPi;
   move1.prepare();
   ASSERT_EQ(move1.angle, FloatHalfPi);

   move1.angle = -FloatHalfPi;
   move1.prepare();
   ASSERT_EQ(move1.angle, -FloatHalfPi);
}


TEST_F(MoveTest, NormalizedAngles)
{
   // 3/4 turn = -1/4 turn
   move1.angle = FloatPi + FloatHalfPi;
   move1.prepare();
   ASSERT_EQ(move1.angle, -FloatHalfPi);

   // -3/4 turn = 1/4 turn  
   move1.angle = -FloatPi - FloatHalfPi;
   move1.prepare();
   ASSERT_EQ(move1.angle, FloatHalfPi);
}


TEST_F(MoveTest, WrapAround)
{
   // Exactly one turn
   move1.angle = Float2Pi;
   move1.prepare();
   ASSERT_EQ(move1.angle, 0);

   move1.angle = -Float2Pi;
   move1.prepare();
   ASSERT_EQ(move1.angle, 0);
}


TEST_F(MoveTest, LargeAngleWrapAround)
{
   // Wrap in pos. dir         
   move1.angle =  Float2Pi + FloatHalfPi;
   move1.prepare();
   ASSERT_EQ(move1.angle, FloatHalfPi);

   // Wrap in neg. dir 
   move1.angle = -Float2Pi - FloatHalfPi;
   move1.prepare();
   ASSERT_EQ(move1.angle, -FloatHalfPi);

   // Really large angles -- we'll never see these in the game
   move1.angle = 432 * Float2Pi;
   move1.prepare();
   ASSERT_EQ(move1.angle, 0);

   move1.angle =  -9 * Float2Pi;
   move1.prepare();
   ASSERT_EQ(move1.angle, 0);
}
   
};
