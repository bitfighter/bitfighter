#include "../zap/GeomUtils.h"
#include "gtest/gtest.h"
#include <tnl.h>

namespace Zap
{

using namespace std;
using namespace TNL;

TEST(GeomPrecisionTest, InsideTriangleLargeCoords)
{
   // Large triangle at 10^7
   F32 offset = 1e7f;
   F32 Ax = offset, Ay = offset;
   F32 Bx = offset + 1000.0f, By = offset;
   F32 Cx = offset, Cy = offset + 1000.0f;

   // Point inside
   EXPECT_TRUE(Triangulate::InsideTriangle(Ax, Ay, Bx, By, Cx, Cy, offset + 100.0f, offset + 100.0f));

   // Point outside
   EXPECT_FALSE(Triangulate::InsideTriangle(Ax, Ay, Bx, By, Cx, Cy, offset - 100.0f, offset - 100.0f));
   EXPECT_FALSE(Triangulate::InsideTriangle(Ax, Ay, Bx, By, Cx, Cy, offset + 1100.0f, offset + 100.0f));
}

TEST(GeomPrecisionTest, triangulatedFillContainsLargeCoords)
{
   F32 offset = 1e7f;
   Vector<Point> triangles;
   triangles.push_back(Point(offset, offset));
   triangles.push_back(Point(offset + 1000.0f, offset));
   triangles.push_back(Point(offset, offset + 1000.0f));

   // Point inside
   EXPECT_TRUE(triangulatedFillContains(&triangles, Point(offset + 100.0f, offset + 100.0f)));

   // Point outside
   EXPECT_FALSE(triangulatedFillContains(&triangles, Point(offset - 100.0f, offset - 100.0f)));
}

TEST(GeomPrecisionTest, mean2dLargeCoords)
{
   F32 offset = 1e7f;
   Vector<Point> pts;
   for(int i = 0; i < 1000; ++i)
   {
      pts.push_back(Point(offset + 10.0f, offset + 20.0f));
   }

   Point m = mean2d(pts);
   EXPECT_NEAR(offset + 10.0f, m.x, 1.0f);
   EXPECT_NEAR(offset + 20.0f, m.y, 1.0f);
}

} // namespace Zap
