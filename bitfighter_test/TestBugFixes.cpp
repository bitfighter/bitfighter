#include "gtest/gtest.h"
#include "GeomUtils.h"
#include "tnlPlatform.h"
#include "tnlVector.h"
#include <unistd.h>

namespace Zap {

TEST(BugFixTest, Mean2DPrecision)
{
    TNL::Vector<Point> points;
    // Use large coordinates where F32 accumulation would lose precision
    // 10,000,000 + small values
    F32 large = 10000000.0f;
    points.push_back(Point(large + 1.0f, large + 1.0f));
    points.push_back(Point(large + 2.0f, large + 2.0f));
    points.push_back(Point(large + 3.0f, large + 3.0f));

    Point mean = mean2d(points);

    // Expected mean is large + 2.0
    // With F32: (10000000 + 10000000 + 10000000) / 3 = 10000000
    // But 10000001.0f might be represented as 10000000.0f

    EXPECT_NEAR(large + 2.0f, mean.x, 0.001f);
    EXPECT_NEAR(large + 2.0f, mean.y, 0.001f);
}

#ifndef TNL_OS_WIN32
TEST(BugFixTest, UnixTickCountMicroRobustness)
{
    U32 start = TNL::Platform::getRealMicroseconds();
    usleep(1100000); // Sleep 1.1 seconds to cross a second boundary
    U32 end = TNL::Platform::getRealMicroseconds();

    // If it only returns microseconds within the second, end could be less than start
    // or the difference could be very small.
    // It should be at least 1,100,000

    // Note: getRealMicroseconds currently returns:
    // return t.tv_usec - sg_uSecsOffset;
    // This is definitely broken.

    EXPECT_GT(end, start);
    EXPECT_GE(end - start, 1000000);
}
#endif

} // namespace Zap
