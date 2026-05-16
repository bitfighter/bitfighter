//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "../zap/MathUtils.h"
#include "../tnl/tnlPlatform.h"
#include "gtest/gtest.h"
#include <unistd.h>

namespace Zap {

TEST(BugFixesTest, x86UNIXGetTickCountMicroMonotonicity)
{
   U32 start = Platform::getRealMicroseconds();
   usleep(100000); // 100ms
   U32 end = Platform::getRealMicroseconds();

   // Use S32 to handle potential wrap-around gracefully in the check,
   // although for a short duration it shouldn't wrap if fixed.
   // If the bug exists (wraps every second), there's a ~10% chance this fails
   // if we happen to cross a second boundary.
   EXPECT_GT(end, start);
   EXPECT_GE(end - start, 100000);
   EXPECT_LT(end - start, 200000); // Allow some leeway for system scheduling
}

// This test specifically tries to catch the second-wrapping bug.
// It may take a while to run, so we'll just do a 1.1s sleep.
TEST(BugFixesTest, x86UNIXGetTickCountMicroAcrossSecond)
{
   U32 start = Platform::getRealMicroseconds();
   usleep(1100000); // 1.1s
   U32 end = Platform::getRealMicroseconds();

   // If it wraps every second, end - start will likely be very small or negative (as U32 wrap).
   // Specifically, if it only returns tv_usec, then after 1.1s,
   // end will be approx start + 100,000 (mod 1,000,000).

   U32 elapsed = end - start;
   EXPECT_GE(elapsed, 1100000);
   EXPECT_LT(elapsed, 1300000);
}

TEST(BugFixesTest, roundUpS32Min)
{
   // abs(S32_MIN) is undefined behavior.
   // -S32_MIN is also undefined behavior because it overflows S32.
   // S32_MIN % multiple might also be problematic.

   S32 val = S32_MIN;
   S32 multiple = 10;

   // This should not crash or trigger UB (though hard to detect UB in gtest without special tools)
   S32 result = roundUp(val, multiple);

   // Expected behavior for roundUp(-2147483648, 10):
   // remainder = -2147483648 % 10 = -8
   // since it's < 0, returns val - remainder = -2147483648 - (-8) = -2147483640
   // -2147483640 is indeed the nearest multiple of 10 >= -2147483648.

   EXPECT_EQ(-2147483640, result);
   EXPECT_EQ(0, result % 10);
}

} // namespace Zap
