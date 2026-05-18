//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "MathUtils.h"
#include "gtest/gtest.h"

namespace Zap {

TEST(MathUtilsTest, Macros)
{
   EXPECT_EQ(1, MIN(1, 2));
   EXPECT_EQ(1, MIN(2, 1));
   EXPECT_EQ(2, MAX(1, 2));
   EXPECT_EQ(2, MAX(2, 1));

   EXPECT_EQ(-1, SIGN(-10));
   EXPECT_EQ(1, SIGN(10));
   EXPECT_EQ(0, SIGN(0));

   EXPECT_EQ(5, CLAMP(5, 1, 10));
   EXPECT_EQ(1, CLAMP(0, 1, 10));
   EXPECT_EQ(10, CLAMP(15, 1, 10));

   EXPECT_EQ(25, sq(5));
}

TEST(MathUtilsTest, AngleConversions)
{
   EXPECT_NEAR(180.0f, radiansToDegrees(FloatPi), 1e-5f);
   EXPECT_NEAR(FloatPi, degreesToRadians(180.0f), 1e-6f);

   EXPECT_FLOAT_EQ(0.5f, radiansToUnit(FloatPi));
   EXPECT_FLOAT_EQ(FloatPi, unitToRadians(0.5f));
}

TEST(MathUtilsTest, GetAngleDiff)
{
   EXPECT_NEAR(0.0f, getAngleDiff(0.0f, 0.0f), 1e-6f);
   EXPECT_NEAR(0.1f, getAngleDiff(0.0f, 0.1f), 1e-6f);
   EXPECT_NEAR(-0.1f, getAngleDiff(0.1f, 0.0f), 1e-6f);

   EXPECT_NEAR(FloatPi, getAngleDiff(0.0f, FloatPi), 1e-6f);
   EXPECT_NEAR(FloatPi, getAngleDiff(FloatPi, 0.0f), 1e-6f); // Or -FloatPi, both are 180 degrees diff

   EXPECT_NEAR(0.2f, getAngleDiff(Float2Pi - 0.1f, 0.1f), 1e-5f);
   EXPECT_NEAR(-0.2f, getAngleDiff(0.1f, Float2Pi - 0.1f), 1e-5f);
}

TEST(MathUtilsTest, RoundUp)
{
   EXPECT_EQ(10, roundUp(7, 5));
   EXPECT_EQ(10, roundUp(10, 5));
   EXPECT_EQ(0, roundUp(0, 5));
   EXPECT_EQ(7, roundUp(7, 0));

   // Test with negative numbers
   EXPECT_EQ(-5, roundUp(-7, 5));
   EXPECT_EQ(-10, roundUp(-12, 5));
   EXPECT_EQ(-10, roundUp(-10, 5));

   // Test with negative multiple
   EXPECT_EQ(10, roundUp(7, -5));
   EXPECT_EQ(10, roundUp(10, -5));
   EXPECT_EQ(-5, roundUp(-7, -5));

   // Test with S32_MIN
   EXPECT_EQ(0, roundUp(0, S32_MIN));
   EXPECT_EQ(S32_MIN, roundUp(1, S32_MIN));
   EXPECT_EQ(0, roundUp(-1, S32_MIN));

   // Additional test cases
   EXPECT_EQ(100, roundUp(7, 100));
   EXPECT_EQ(200, roundUp(117, 100));
   EXPECT_EQ(500, roundUp(477, 100));
   EXPECT_EQ(1100, roundUp(1077, 100));
   EXPECT_EQ(60, roundUp(52, 20));
   EXPECT_EQ(90, roundUp(74, 30));
}

TEST(MathUtilsTest, FindLowestRootInInterval)
{
   F32 root;
   // x^2 - 3x + 2 = 0 -> roots are 1 and 2
   EXPECT_TRUE(findLowestRootInInterval(1.0f, -3.0f, 2.0f, 5.0f, root));
   EXPECT_FLOAT_EQ(1.0f, root);

   EXPECT_TRUE(findLowestRootInInterval(1.0f, -3.0f, 2.0f, 1.5f, root));
   EXPECT_FLOAT_EQ(1.0f, root);

   // lowest root in [0, 0.5] -> none
   EXPECT_FALSE(findLowestRootInInterval(1.0f, -3.0f, 2.0f, 0.5f, root));

   // x^2 + 1 = 0 -> no real roots
   EXPECT_FALSE(findLowestRootInInterval(1.0f, 0.0f, 1.0f, 10.0f, root));

   // Linear case (a=0): -3x + 2 = 0 -> x = 2/3
   EXPECT_TRUE(findLowestRootInInterval(0.0f, -3.0f, 2.0f, 1.0f, root));
   EXPECT_NEAR(0.6666666f, root, 1e-6f);

   // Case: inA = 0, inB = 0 (No solution or all solutions)
   EXPECT_FALSE(findLowestRootInInterval(0.0f, 0.0f, 1.0f, 10.0f, root));
   EXPECT_FALSE(findLowestRootInInterval(0.0f, 0.0f, 0.0f, 10.0f, root));

   // Case: small inA
   // 1e-10 * x^2 - 3x + 2 = 0
   // Linear root is 2/3 approx 0.6666666
   // Other root is approx 3 / 1e-10 = 3e10
   EXPECT_TRUE(findLowestRootInInterval(1e-10f, -3.0f, 2.0f, 1.0f, root));
   EXPECT_NEAR(0.6666666f, root, 1e-6f);

   // Case: inB = 0, inA != 0, inC != 0
   // x^2 - 4 = 0 -> roots are 2 and -2
   EXPECT_TRUE(findLowestRootInInterval(1.0f, 0.0f, -4.0f, 5.0f, root));
   EXPECT_FLOAT_EQ(2.0f, root);

   // Case: inC = 0
   // x^2 - 3x = 0 -> roots are 0 and 3
   EXPECT_TRUE(findLowestRootInInterval(1.0f, -3.0f, 0.0f, 5.0f, root));
   EXPECT_FLOAT_EQ(0.0f, root);
  
   // Bug: handle linear equations correctly (currently might return false or crash)
   // 2x - 4 = 0 -> x = 2
   EXPECT_TRUE(findLowestRootInInterval(0.0f, 2.0f, -4.0f, 5.0f, root));
   EXPECT_FLOAT_EQ(2.0f, root);

   // Bug: avoid division by zero when q = 0
   // 0x^2 + 0x + 0 = 0 -> any x is a root, but we should handle it gracefully
   EXPECT_FALSE(findLowestRootInInterval(0.0f, 0.0f, 0.0f, 5.0f, root));

   // Bug: handle case where q = 0, but inA != 0
   // x^2 = 0 -> x = 0
   EXPECT_TRUE(findLowestRootInInterval(1.0f, 0.0f, 0.0f, 5.0f, root));
   EXPECT_FLOAT_EQ(0.0f, root);
}

} // namespace Zap
