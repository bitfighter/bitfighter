//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "../zap/GeomUtils.h"
#include "gtest/gtest.h"
#include <tnl.h>

namespace Zap
{

using namespace std;
using namespace TNL;

TEST(GeomPrecisionTest, mean2dLargeCoordinates)
{
   Vector<Point> pts;
   // 1,000,000.0625 is exactly representable in F32
   // 2^19 < 1,000,000 < 2^20. Mantissa has 24 bits. 24 - 20 = 4 bits for fraction.
   // 2^-4 = 0.0625.
   F32 val = 1000000.0625f;
   S32 count = 100;
   for(S32 i = 0; i < count; i++)
      pts.push_back(Point(val, val));

   Point m = mean2d(pts);

   // Expected mean: 1000000.0625
   // With F32 accumulation:
   // Sum reaches 16,777,216 (2^24). Beyond this, step is 1.0.
   // Adding 1,000,000.0625 will round the sum.
   // At some point, the .0625 will be lost in every addition.

   // We expect the result to be more accurate than what F32 accumulation would give.
   // F32 accumulation of 100 * 1000000.0625:
   // 100 * 1000000 = 100,000,000.
   // 100 * 0.0625 = 6.25.
   // Total should be 100,000,006.25.
   // In F32, 100,000,006.25 is between 2^26 and 2^27, step is 8.0.
   // So it would be rounded to 100,000,008 or 100,000,000.

   EXPECT_NEAR(1000000.0625f, m.x, 0.0001f);
}

TEST(GeomPrecisionTest, InsideTrianglePrecision)
{
   // Define a triangle and a point using F64 to ensure we are testing the function's internal precision
   // once we update its signature to F64.
   // For now, we call it with values that might be problematic.

   double offset = 1e7;
   double Ax = offset;
   double Ay = offset;
   double Bx = offset + 1000.0;
   double By = offset;
   double Cx = offset;
   double Cy = offset + 1000.0;

   // Point slightly inside
   double Px = offset + 1.0;
   double Py = offset + 1.0;

   EXPECT_TRUE(Triangulate::InsideTriangle((float)Ax, (float)Ay, (float)Bx, (float)By, (float)Cx, (float)Cy, (float)Px, (float)Py));

   // Point very close to the hypotenuse B-C
   // Hypotenuse is x + y = 2*offset + 1000
   // Point at x = offset + 500, y = offset + 500 is on the line.
   // Point at x = offset + 500, y = offset + 499.999 is slightly inside.

   Px = offset + 500.0;
   Py = offset + 499.999;

   // In F32:
   // offset + 500 = 10,000,500. Representable (step is 1.0).
   // offset + 499.999 = 10,000,499.999 -> 10,000,500.0 (rounded).
   // So in F32, the point is seen as ON the line.

   EXPECT_TRUE(Triangulate::InsideTriangle((float)Ax, (float)Ay, (float)Bx, (float)By, (float)Cx, (float)Cy, (float)Px, (float)Py));
}

} // namespace Zap
