//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Test that Clipper2 is compiled and linked into the build

#include "gtest/gtest.h"
#include <clipper2/clipper.h>

namespace Zap
{

TEST(Clipper2Test, VersionDefined)
{
   // Verify the version constant is defined and non-empty
   EXPECT_STREQ("2.0.1", CLIPPER2_VERSION);
}

TEST(Clipper2Test, SimpleBooleanUnion)
{
   using namespace Clipper2Lib;

   // Two overlapping squares: union should produce a single rectilinear shape
   Paths64 subject = { MakePath({0, 0, 10, 0, 10, 10, 0, 10}) };
   Paths64 clip    = { MakePath({5, 5, 15, 5, 15, 15, 5, 15}) };

   Paths64 result = BooleanOp(ClipType::Union, FillRule::NonZero, subject, clip);

   // The union of these two overlapping squares should result in one path
   EXPECT_EQ(result.size(), 1);

   // The resulting shape is an L-shaped polygon with 8 vertices
   Path64 &path = result[0];
   EXPECT_EQ(path.size(), 8u);

   // Quick area sanity check: two 10x10 squares (area 100 each) overlap
   // in a 5x5 region (area 25), so union area = 100 + 100 - 25 = 175.
   double area = Area(path);
   EXPECT_DOUBLE_EQ(area, 175.0);
}

TEST(Clipper2Test, Intersection)
{
   using namespace Clipper2Lib;

   // Two overlapping squares: intersection is a 5x5 square
   Paths64 subject = { MakePath({0, 0, 10, 0, 10, 10, 0, 10}) };
   Paths64 clip    = { MakePath({5, 5, 15, 5, 15, 15, 5, 15}) };

   Paths64 result = BooleanOp(ClipType::Intersection, FillRule::NonZero, subject, clip);

   EXPECT_EQ(result.size(), 1);

   Path64 &path = result[0];
   EXPECT_EQ(path.size(), 4u);

   double area = Area(path);
   EXPECT_DOUBLE_EQ(area, 25.0);
}

TEST(Clipper2Test, PointInPolygon)
{
   using namespace Clipper2Lib;

   Path64 square = MakePath({0, 0, 100, 0, 100, 100, 0, 100});

   // Point inside
   EXPECT_TRUE(PointInPolygon(Point64(50, 50), square) != PointInPolygonResult::IsOutside);

   // Point outside
   EXPECT_TRUE(PointInPolygon(Point64(200, 200), square) == PointInPolygonResult::IsOutside);

   // Point on edge
   EXPECT_TRUE(PointInPolygon(Point64(50, 0), square) == PointInPolygonResult::IsOn);
}

} // namespace Zap
