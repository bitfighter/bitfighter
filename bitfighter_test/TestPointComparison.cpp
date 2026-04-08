//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Point.h"
#include "gtest/gtest.h"
#include <set>
#include <vector>
#include <algorithm>

namespace Zap {

// This struct is a copy of the one in UIEditor.cpp
// We want to test it specifically because it was broken
struct PointCompareTest
{
   bool operator()( const Point& lhs, const Point& rhs ) const
   {
      if(lhs.x != rhs.x)
         return lhs.x < rhs.x;
      return lhs.y < rhs.y;
   }
};

TEST(PointComparisonTest, StrictWeakOrdering)
{
   Point p1(1.0f, 2.0f);
   Point p2(2.0f, 1.0f);
   Point p3(1.0f, 2.0f);

   PointCompareTest cmp;

   // Asymmetry: if a < b then !(b < a)
   EXPECT_TRUE(cmp(p1, p2));
   EXPECT_FALSE(cmp(p2, p1));

   // Irreflexivity: a < a is false
   EXPECT_FALSE(cmp(p1, p1));

   // Equality
   EXPECT_FALSE(cmp(p1, p3));
   EXPECT_FALSE(cmp(p3, p1));
}

TEST(PointComparisonTest, SetDeDuplication)
{
   std::set<Point, PointCompareTest> pointSet;

   pointSet.insert(Point(1.0f, 2.0f));
   pointSet.insert(Point(2.0f, 1.0f));
   pointSet.insert(Point(1.0f, 2.0f)); // Duplicate
   pointSet.insert(Point(0.0f, 5.0f));
   pointSet.insert(Point(0.0f, 5.0f)); // Duplicate

   EXPECT_EQ(3, pointSet.size());

   // Verify elements are present
   EXPECT_TRUE(pointSet.find(Point(1.0f, 2.0f)) != pointSet.end());
   EXPECT_TRUE(pointSet.find(Point(2.0f, 1.0f)) != pointSet.end());
   EXPECT_TRUE(pointSet.find(Point(0.0f, 5.0f)) != pointSet.end());
}

TEST(PointComparisonTest, OperatorLess)
{
   // p1 and p2 have the same distance from origin: sqrt(1^2 + 2^2) = sqrt(5)
   Point p1(1.0f, 2.0f);
   Point p2(2.0f, 1.0f);

   // With the fix, they should be distinguishable
   EXPECT_TRUE(p1 < p2);
   EXPECT_FALSE(p2 < p1);
   EXPECT_FALSE(p1 == p2);

   EXPECT_TRUE(p1 <= p2);
   EXPECT_TRUE(p2 >= p1);
   EXPECT_TRUE(p2 > p1);
}

TEST(PointComparisonTest, DefaultSetBehavior)
{
   // std::set uses operator< by default
   std::set<Point> pointSet;

   pointSet.insert(Point(1.0f, 2.0f));
   pointSet.insert(Point(2.0f, 1.0f)); // Same distance, but different point

   EXPECT_EQ(2, pointSet.size());
}

} // namespace Zap
