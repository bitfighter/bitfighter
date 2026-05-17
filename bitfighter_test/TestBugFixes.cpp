#include "MathUtils.h"
#include "gtest/gtest.h"
#include "tnlTypes.h"

namespace Zap {

TEST(BugFixes, RoundUpS32Min)
{
   // S32_MIN is -2147483648.
   // absMultiple = 2147483648

   EXPECT_EQ(0, roundUp(0, S32_MIN));
   EXPECT_EQ(S32_MIN, roundUp(1, S32_MIN));  // 1 rounded up to 2^31 is 2^31, which is S32_MIN
   EXPECT_EQ(0, roundUp(-1, S32_MIN));       // -1 rounded up to 2^31 is 0
}

TEST(BugFixes, RoundUpNormal)
{
   EXPECT_EQ(10, roundUp(7, 5));
   EXPECT_EQ(-5, roundUp(-7, 5));
}

}
