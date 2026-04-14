//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "../zap/GeomUtils.h"
#include "gtest/gtest.h"
#include <tnl.h>

namespace Zap {

TEST(GeomPrecisionTest, PolygonContainsPointPrecision)
{
   // A thin rectangle, CCW: (0,0), (1,0), (1,10), (0,10)
   Point vertices[4];
   vertices[0] = Point(0, 0);
   vertices[1] = Point(1, 0);
   vertices[2] = Point(1, 10);
   vertices[3] = Point(0, 10);

   // A point just outside the left edge (x = 0)
   Point point(-0.01f, 5.0f);

   // With the S32 bug, this incorrectly returns true.
   // It should return false.
   EXPECT_FALSE(polygonContainsPoint(vertices, 4, point));
}

TEST(GeomPrecisionTest, IsClockwiseTrianglePrecision)
{
   // A very thin clockwise triangle
   // p1=(0,0), p2=(0,10), p3=(0.01, 5)
   // Cross product: (y2-y1)*(x3-x2) - (x2-x1)*(y3-y2)
   // (10-0)*(0.01-0) - (0-0)*(5-10) = 10 * 0.01 - 0 = 0.1
   // S32(0.1) is 0.
   // The current implementation of isClockwiseTriangle is NOT exported,
   // but it's used in constructBarrierPolygon.
   // We can't test it directly unless we include the .cpp or it's moved to header/exported.
   // However, isLeft is also used in polygonContainsPoint, which we tested above.
}

} // namespace Zap
