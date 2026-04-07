//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Point.h"
#include "gtest/gtest.h"
#include <set>

namespace Zap {

// This mimics the broken PointCompare logic that was in zap/UIEditor.cpp
struct BrokenPointCompare
{
   bool operator()( const Point& lhs, const Point& rhs ) const
   {
      return lhs.x != rhs.x || lhs.y != rhs.y;
   }
};

struct FixedPointCompare
{
   bool operator()( const Point& lhs, const Point& rhs ) const
   {
      if (lhs.x != rhs.x)
         return lhs.x < rhs.x;
      return lhs.y < rhs.y;
   }
};

TEST(PointCompareTest, BrokenCompare)
{
   std::set<Point, BrokenPointCompare> points;
   Point p1(1.0f, 1.0f);
   Point p2(2.0f, 2.0f);

   points.insert(p1);
   points.insert(p2);

   // The fundamental violation of strict weak ordering:
   // Asymmetry: if comp(a, b) is true, then comp(b, a) must be false.
   // With BrokenCompare, if p1 != p2, both are true.
   EXPECT_TRUE(BrokenPointCompare()(p1, p2));
   EXPECT_TRUE(BrokenPointCompare()(p2, p1));
}

TEST(PointCompareTest, FixedCompare)
{
   std::set<Point, FixedPointCompare> points;
   Point p1(1.0f, 1.0f);
   Point p2(2.0f, 2.0f);
   Point p3(1.0f, 2.0f);

   points.insert(p1);
   points.insert(p2);
   points.insert(p3);

   EXPECT_EQ(3, points.size());
   EXPECT_NE(points.find(p1), points.end());
   EXPECT_NE(points.find(p2), points.end());
   EXPECT_NE(points.find(p3), points.end());
}

TEST(PointCompareTest, Correctness)
{
   FixedPointCompare comp;
   Point p1(1.0f, 1.0f);
   Point p2(1.0f, 2.0f);
   Point p3(2.0f, 1.0f);

   // Irreflexivity: comp(a, a) is false
   EXPECT_FALSE(comp(p1, p1));
   EXPECT_FALSE(comp(p2, p2));
   EXPECT_FALSE(comp(p3, p3));

   // Asymmetry: if comp(a, b) is true, then comp(b, a) is false
   EXPECT_TRUE(comp(p1, p2));
   EXPECT_FALSE(comp(p2, p1));

   EXPECT_TRUE(comp(p1, p3));
   EXPECT_FALSE(comp(p3, p1));

   EXPECT_TRUE(comp(p2, p3));
   EXPECT_FALSE(comp(p3, p2));

   // Transitivity: if comp(a, b) and comp(b, c) are true, then comp(a, c) is true
   EXPECT_TRUE(comp(p1, p2));
   EXPECT_TRUE(comp(p2, p3));
   EXPECT_TRUE(comp(p1, p3));
}

} // namespace Zap
