#include "Point.h"
#include <set>
#include "gtest/gtest.h"

namespace Zap {

// PointCompare logic copied from zap/UIEditor.cpp to verify it
struct PointCompare
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
   Point p2(1.0f, 2.0f);
   Point p3(1.0f, 3.0f);
   Point p4(2.0f, 1.0f);

   PointCompare comp;

   // Irreflexivity: a < a is false
   EXPECT_FALSE(comp(p1, p1));

   // Asymmetry: if a < b then !(b < a)
   EXPECT_TRUE(comp(p1, p3));
   EXPECT_FALSE(comp(p3, p1));

   EXPECT_TRUE(comp(p1, p4));
   EXPECT_FALSE(comp(p4, p1));

   // Transitivity: if a < b and b < c then a < c
   // p1(1,2) < p3(1,3) and p3(1,3) < p4(2,1) => p1(1,2) < p4(2,1)
   EXPECT_TRUE(comp(p1, p3));
   EXPECT_TRUE(comp(p3, p4));
   EXPECT_TRUE(comp(p1, p4));

   // Equality/Equivalence
   EXPECT_FALSE(comp(p1, p2));
   EXPECT_FALSE(comp(p2, p1));
}

TEST(PointComparisonTest, SetDeduplication)
{
   std::set<Point, PointCompare> points;
   points.insert(Point(1.0f, 1.0f));
   points.insert(Point(2.0f, 2.0f));
   points.insert(Point(1.0f, 1.0f)); // Duplicate

   EXPECT_EQ(2, points.size());
}

} // namespace Zap
