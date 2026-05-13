//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Timer.h"
#include "gtest/gtest.h"

namespace Zap
{

TEST(TimerTest, Construction)
{
   Timer t(100);
   EXPECT_EQ(100u, t.getPeriod());
   EXPECT_EQ(100u, t.getCurrent());
   EXPECT_EQ(0u, t.getElapsed());
}

TEST(TimerTest, Update)
{
   Timer t(100);
   EXPECT_FALSE(t.update(50));
   EXPECT_EQ(50u, t.getCurrent());
   EXPECT_EQ(50u, t.getElapsed());

   EXPECT_TRUE(t.update(50));
   EXPECT_EQ(0u, t.getCurrent());
   EXPECT_FALSE(t.update(10)); // Was already 0, should return false
}

TEST(TimerTest, UpdateAlreadyExpired)
{
   Timer t(100);
   EXPECT_TRUE(t.update(100));
   EXPECT_EQ(0u, t.getCurrent());
   EXPECT_FALSE(t.update(1)); // Already 0, should return false
}

TEST(TimerTest, GetFraction)
{
   Timer t(100);
   EXPECT_FLOAT_EQ(1.0f, t.getFraction());
   t.update(25);
   EXPECT_FLOAT_EQ(0.75f, t.getFraction());
   t.update(75);
   EXPECT_FLOAT_EQ(0.0f, t.getFraction());

   Timer t0(0);
   EXPECT_FLOAT_EQ(0.0f, t0.getFraction());
}

TEST(TimerTest, Invert)
{
   Timer t(100);
   t.update(30); // 70 left, 0.7 fraction
   EXPECT_EQ(70u, t.getCurrent());
   t.invert(); // should be (1 - 0.7) * 100 = 30
   EXPECT_EQ(30u, t.getCurrent());

   t.invert(); // should be (1 - 0.3) * 100 = 70
   EXPECT_EQ(70u, t.getCurrent());
}

TEST(TimerTest, Reset)
{
   Timer t(100);
   t.update(50);
   EXPECT_EQ(50u, t.getCurrent());
   t.reset();
   EXPECT_EQ(100u, t.getCurrent());
   EXPECT_EQ(100u, t.getPeriod());

   t.reset(50, 200);
   EXPECT_EQ(50u, t.getCurrent());
   EXPECT_EQ(200u, t.getPeriod());

   t.reset(75); // newPeriod = 0, so it sets period to 75
   EXPECT_EQ(75u, t.getCurrent());
   EXPECT_EQ(75u, t.getPeriod());
}

TEST(TimerTest, Clear)
{
   Timer t(100);
   t.clear();
   EXPECT_EQ(0u, t.getCurrent());
}

TEST(TimerTest, Extend)
{
   Timer t(100);
   t.update(40); 
   EXPECT_EQ(60u, t.getCurrent());

   t.extend(20);
   EXPECT_EQ(120u, t.getPeriod());
   EXPECT_EQ(80u, t.getCurrent());

   t.extend(-30);
   EXPECT_EQ(90u, t.getPeriod());
   EXPECT_EQ(50u, t.getCurrent());
}

TEST(TimerTest, ExtendOverflow)
{
   Timer t(U32_MAX - 50);
   t.reset(U32_MAX - 100, U32_MAX - 50);

   t.extend(60);
   // mPeriod was U32_MAX - 50. Adding 60 should cap it at U32_MAX.
   EXPECT_EQ(U32_MAX, t.getPeriod());
   EXPECT_EQ(U32_MAX - 100 + 60, t.getCurrent());
}

TEST(TimerTest, ExtendUnderflow)
{
   Timer t(100);
   t.update(90); // 10 left

   t.extend(-150);
   EXPECT_EQ(0u, t.getPeriod());
   EXPECT_EQ(0u, t.getCurrent());
}

TEST(TimerTest, InvertPrecisionBug)
{
   U32 largePeriod = 100000000; // 10^8
   Timer t(largePeriod);
   t.reset(1, largePeriod); // 1 unit left

   t.invert();

   // Expected: largePeriod - 1 = 99999999
   // If it has the bug, it might stay at largePeriod due to float precision
   EXPECT_EQ(99999999u, t.getCurrent());
}

TEST(TimerTest, ExtendS32MinBug)
{
   Timer t(1000);
   // S32_MIN is -2147483648
   t.extend(-2147483647 - 1); // S32_MIN

   EXPECT_EQ(0u, t.getPeriod());
   EXPECT_EQ(0u, t.getCurrent());
}

}
