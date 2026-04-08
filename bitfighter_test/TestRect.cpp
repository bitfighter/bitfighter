//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "../zap/Rect.h"
#include "../zap/GeomUtils.h"
#include "gtest/gtest.h"
#include "tnlVector.h"
#include <cmath>

namespace Zap
{

using namespace TNL;

// ============================================================
// IntRect Tests
// ============================================================

TEST(IntRectTest, DefaultConstructor)
{
   IntRect r;
   EXPECT_EQ(0, r.minx);
   EXPECT_EQ(0, r.miny);
   EXPECT_EQ(0, r.maxx);
   EXPECT_EQ(0, r.maxy);
}

TEST(IntRectTest, ParameterizedConstructor)
{
   IntRect r(1, 2, 3, 4);
   EXPECT_EQ(1, r.minx);
   EXPECT_EQ(2, r.miny);
   EXPECT_EQ(3, r.maxx);
   EXPECT_EQ(4, r.maxy);
}

TEST(IntRectTest, ConstructorNormalizesMinMax)
{
   // When first coords are larger than second, should still get correct min/max
   IntRect r(10, 20, 1, 2);
   EXPECT_EQ(1,  r.minx);
   EXPECT_EQ(2,  r.miny);
   EXPECT_EQ(10, r.maxx);
   EXPECT_EQ(20, r.maxy);
}

TEST(IntRectTest, Set)
{
   IntRect r;
   r.set(5, 6, 7, 8);
   EXPECT_EQ(5, r.minx);
   EXPECT_EQ(6, r.miny);
   EXPECT_EQ(7, r.maxx);
   EXPECT_EQ(8, r.maxy);
}

TEST(IntRectTest, SetNormalizesMinMax)
{
   IntRect r;
   r.set(100, 200, 10, 20);
   EXPECT_EQ(10,  r.minx);
   EXPECT_EQ(20,  r.miny);
   EXPECT_EQ(100, r.maxx);
   EXPECT_EQ(200, r.maxy);
}

TEST(IntRectTest, NegativeCoordinates)
{
   IntRect r(-5, -10, -1, -2);
   EXPECT_EQ(-5, r.minx);
   EXPECT_EQ(-10, r.miny);
   EXPECT_EQ(-1, r.maxx);
   EXPECT_EQ(-2, r.maxy);
}

TEST(IntRectTest, ZeroSizeRect)
{
   IntRect r(3, 4, 3, 4);
   EXPECT_EQ(3, r.minx);
   EXPECT_EQ(4, r.miny);
   EXPECT_EQ(3, r.maxx);
   EXPECT_EQ(4, r.maxy);
}


// ============================================================
// Rect Constructors
// ============================================================

TEST(RectTest, DefaultConstructor)
{
   Rect r;
   EXPECT_FLOAT_EQ(0.0f, r.min.x);
   EXPECT_FLOAT_EQ(0.0f, r.min.y);
   EXPECT_FLOAT_EQ(0.0f, r.max.x);
   EXPECT_FLOAT_EQ(0.0f, r.max.y);
}

TEST(RectTest, ConstructorTwoPoints)
{
   Rect r(Point(1.0f, 2.0f), Point(3.0f, 4.0f));
   EXPECT_FLOAT_EQ(1.0f, r.min.x);
   EXPECT_FLOAT_EQ(2.0f, r.min.y);
   EXPECT_FLOAT_EQ(3.0f, r.max.x);
   EXPECT_FLOAT_EQ(4.0f, r.max.y);
}

TEST(RectTest, ConstructorTwoPointsReversed)
{
   // Points in "wrong" order — should still normalize to min/max
   Rect r(Point(3.0f, 4.0f), Point(1.0f, 2.0f));
   EXPECT_FLOAT_EQ(1.0f, r.min.x);
   EXPECT_FLOAT_EQ(2.0f, r.min.y);
   EXPECT_FLOAT_EQ(3.0f, r.max.x);
   EXPECT_FLOAT_EQ(4.0f, r.max.y);
}

TEST(RectTest, ConstructorFourFloats)
{
   Rect r(1.0f, 2.0f, 5.0f, 6.0f);
   EXPECT_FLOAT_EQ(1.0f, r.min.x);
   EXPECT_FLOAT_EQ(2.0f, r.min.y);
   EXPECT_FLOAT_EQ(5.0f, r.max.x);
   EXPECT_FLOAT_EQ(6.0f, r.max.y);
}

TEST(RectTest, ConstructorFourFloatsReversed)
{
   Rect r(5.0f, 6.0f, 1.0f, 2.0f);
   EXPECT_FLOAT_EQ(1.0f, r.min.x);
   EXPECT_FLOAT_EQ(2.0f, r.min.y);
   EXPECT_FLOAT_EQ(5.0f, r.max.x);
   EXPECT_FLOAT_EQ(6.0f, r.max.y);
}

TEST(RectTest, ConstructorFromPointer)
{
   Rect original(Point(1.0f, 2.0f), Point(3.0f, 4.0f));
   Rect copy(&original);
   EXPECT_FLOAT_EQ(1.0f, copy.min.x);
   EXPECT_FLOAT_EQ(2.0f, copy.min.y);
   EXPECT_FLOAT_EQ(3.0f, copy.max.x);
   EXPECT_FLOAT_EQ(4.0f, copy.max.y);
}

TEST(RectTest, ConstructorFromPointVector)
{
   Vector<Point> pts;
   pts.push_back(Point(1.0f, 5.0f));
   pts.push_back(Point(3.0f, 2.0f));
   pts.push_back(Point(-1.0f, 4.0f));
   Rect r(pts);
   EXPECT_FLOAT_EQ(-1.0f, r.min.x);
   EXPECT_FLOAT_EQ(2.0f,  r.min.y);
   EXPECT_FLOAT_EQ(3.0f,  r.max.x);
   EXPECT_FLOAT_EQ(5.0f,  r.max.y);
}

TEST(RectTest, ConstructorFromSinglePointVector)
{
   Vector<Point> pts;
   pts.push_back(Point(7.0f, 3.0f));
   Rect r(pts);
   EXPECT_FLOAT_EQ(7.0f, r.min.x);
   EXPECT_FLOAT_EQ(3.0f, r.min.y);
   EXPECT_FLOAT_EQ(7.0f, r.max.x);
   EXPECT_FLOAT_EQ(3.0f, r.max.y);
}

TEST(RectTest, ConstructorFromEmptyPointVector)
{
   // Empty vector should produce a zero rect
   Vector<Point> pts;
   Rect r(pts);
   EXPECT_FLOAT_EQ(0.0f, r.min.x);
   EXPECT_FLOAT_EQ(0.0f, r.min.y);
   EXPECT_FLOAT_EQ(0.0f, r.max.x);
   EXPECT_FLOAT_EQ(0.0f, r.max.y);
}

TEST(RectTest, ConstructorCenterAndRadius)
{
   Rect r(Point(5.0f, 5.0f), 3.0f);
   EXPECT_FLOAT_EQ(2.0f, r.min.x);
   EXPECT_FLOAT_EQ(2.0f, r.min.y);
   EXPECT_FLOAT_EQ(8.0f, r.max.x);
   EXPECT_FLOAT_EQ(8.0f, r.max.y);
}

TEST(RectTest, ConstructorCenterAndZeroRadius)
{
   Rect r(Point(3.0f, 4.0f), 0.0f);
   EXPECT_FLOAT_EQ(3.0f, r.min.x);
   EXPECT_FLOAT_EQ(4.0f, r.min.y);
   EXPECT_FLOAT_EQ(3.0f, r.max.x);
   EXPECT_FLOAT_EQ(4.0f, r.max.y);
}


// ============================================================
// Rect::getCenter()
// ============================================================

TEST(RectTest, GetCenter)
{
   Rect r(Point(0.0f, 0.0f), Point(4.0f, 6.0f));
   Point c = r.getCenter();
   EXPECT_FLOAT_EQ(2.0f, c.x);
   EXPECT_FLOAT_EQ(3.0f, c.y);
}

TEST(RectTest, GetCenterNegativeCoords)
{
   Rect r(Point(-4.0f, -6.0f), Point(4.0f, 6.0f));
   Point c = r.getCenter();
   EXPECT_FLOAT_EQ(0.0f, c.x);
   EXPECT_FLOAT_EQ(0.0f, c.y);
}

TEST(RectTest, GetCenterZeroSizeRect)
{
   Rect r(Point(5.0f, 7.0f), Point(5.0f, 7.0f));
   Point c = r.getCenter();
   EXPECT_FLOAT_EQ(5.0f, c.x);
   EXPECT_FLOAT_EQ(7.0f, c.y);
}


// ============================================================
// Rect::set() overloads
// ============================================================

TEST(RectTest, SetTwoPoints)
{
   Rect r;
   r.set(Point(2.0f, 3.0f), Point(5.0f, 8.0f));
   EXPECT_FLOAT_EQ(2.0f, r.min.x);
   EXPECT_FLOAT_EQ(3.0f, r.min.y);
   EXPECT_FLOAT_EQ(5.0f, r.max.x);
   EXPECT_FLOAT_EQ(8.0f, r.max.y);
}

TEST(RectTest, SetTwoPointsReversed)
{
   Rect r;
   r.set(Point(5.0f, 8.0f), Point(2.0f, 3.0f));
   EXPECT_FLOAT_EQ(2.0f, r.min.x);
   EXPECT_FLOAT_EQ(3.0f, r.min.y);
   EXPECT_FLOAT_EQ(5.0f, r.max.x);
   EXPECT_FLOAT_EQ(8.0f, r.max.y);
}

TEST(RectTest, SetFromRectReference)
{
   Rect src(Point(1.0f, 2.0f), Point(3.0f, 4.0f));
   Rect dst;
   dst.set(src);
   EXPECT_FLOAT_EQ(1.0f, dst.min.x);
   EXPECT_FLOAT_EQ(2.0f, dst.min.y);
   EXPECT_FLOAT_EQ(3.0f, dst.max.x);
   EXPECT_FLOAT_EQ(4.0f, dst.max.y);
}

TEST(RectTest, SetFromRectPointer)
{
   Rect src(Point(1.0f, 2.0f), Point(3.0f, 4.0f));
   Rect dst;
   dst.set(&src);
   EXPECT_FLOAT_EQ(1.0f, dst.min.x);
   EXPECT_FLOAT_EQ(2.0f, dst.min.y);
   EXPECT_FLOAT_EQ(3.0f, dst.max.x);
   EXPECT_FLOAT_EQ(4.0f, dst.max.y);
}

TEST(RectTest, SetFromPointVector)
{
   Vector<Point> pts;
   pts.push_back(Point(10.0f, 1.0f));
   pts.push_back(Point(-2.0f, 5.0f));
   pts.push_back(Point(3.0f, -3.0f));
   Rect r;
   r.set(pts);
   EXPECT_FLOAT_EQ(-2.0f, r.min.x);
   EXPECT_FLOAT_EQ(-3.0f, r.min.y);
   EXPECT_FLOAT_EQ(10.0f, r.max.x);
   EXPECT_FLOAT_EQ(5.0f,  r.max.y);
}

TEST(RectTest, SetFromEmptyPointVector)
{
   Vector<Point> pts;
   Rect r(Point(5.0f, 5.0f), Point(10.0f, 10.0f));
   r.set(pts);
   // Empty vector resets to zero rect
   EXPECT_FLOAT_EQ(0.0f, r.min.x);
   EXPECT_FLOAT_EQ(0.0f, r.min.y);
   EXPECT_FLOAT_EQ(0.0f, r.max.x);
   EXPECT_FLOAT_EQ(0.0f, r.max.y);
}

TEST(RectTest, SetCenterAndRadius)
{
   Rect r;
   r.set(Point(0.0f, 0.0f), 5.0f);
   EXPECT_FLOAT_EQ(-5.0f, r.min.x);
   EXPECT_FLOAT_EQ(-5.0f, r.min.y);
   EXPECT_FLOAT_EQ(5.0f,  r.max.x);
   EXPECT_FLOAT_EQ(5.0f,  r.max.y);
}


// ============================================================
// Rect::contains()
// ============================================================

TEST(RectTest, ContainsPointInside)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   EXPECT_TRUE(r.contains(Point(5.0f, 5.0f)));
}

TEST(RectTest, ContainsPointOutside)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   EXPECT_FALSE(r.contains(Point(11.0f, 5.0f)));
   EXPECT_FALSE(r.contains(Point(5.0f, 11.0f)));
   EXPECT_FALSE(r.contains(Point(-1.0f, 5.0f)));
   EXPECT_FALSE(r.contains(Point(5.0f, -1.0f)));
}

TEST(RectTest, ContainsPointOnEdge)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   // Boundary points are included
   EXPECT_TRUE(r.contains(Point(0.0f, 5.0f)));
   EXPECT_TRUE(r.contains(Point(10.0f, 5.0f)));
   EXPECT_TRUE(r.contains(Point(5.0f, 0.0f)));
   EXPECT_TRUE(r.contains(Point(5.0f, 10.0f)));
}

TEST(RectTest, ContainsCornerPoints)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   EXPECT_TRUE(r.contains(Point(0.0f, 0.0f)));
   EXPECT_TRUE(r.contains(Point(10.0f, 0.0f)));
   EXPECT_TRUE(r.contains(Point(0.0f, 10.0f)));
   EXPECT_TRUE(r.contains(Point(10.0f, 10.0f)));
}

TEST(RectTest, ContainsSinglePointRect)
{
   Rect r(Point(5.0f, 5.0f), Point(5.0f, 5.0f));
   EXPECT_TRUE(r.contains(Point(5.0f, 5.0f)));
   EXPECT_FALSE(r.contains(Point(5.001f, 5.0f)));
}


// ============================================================
// Rect::unionPoint()
// ============================================================

TEST(RectTest, UnionPointExpand)
{
   Rect r(Point(0.0f, 0.0f), Point(5.0f, 5.0f));
   r.unionPoint(Point(10.0f, 8.0f));
   EXPECT_FLOAT_EQ(0.0f,  r.min.x);
   EXPECT_FLOAT_EQ(0.0f,  r.min.y);
   EXPECT_FLOAT_EQ(10.0f, r.max.x);
   EXPECT_FLOAT_EQ(8.0f,  r.max.y);
}

TEST(RectTest, UnionPointExpandLeft)
{
   Rect r(Point(0.0f, 0.0f), Point(5.0f, 5.0f));
   r.unionPoint(Point(-3.0f, 2.0f));
   EXPECT_FLOAT_EQ(-3.0f, r.min.x);
   EXPECT_FLOAT_EQ(0.0f,  r.min.y);
   EXPECT_FLOAT_EQ(5.0f,  r.max.x);
   EXPECT_FLOAT_EQ(5.0f,  r.max.y);
}

TEST(RectTest, UnionPointInsideNoChange)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   r.unionPoint(Point(5.0f, 5.0f));
   EXPECT_FLOAT_EQ(0.0f,  r.min.x);
   EXPECT_FLOAT_EQ(0.0f,  r.min.y);
   EXPECT_FLOAT_EQ(10.0f, r.max.x);
   EXPECT_FLOAT_EQ(10.0f, r.max.y);
}

TEST(RectTest, UnionPointAllFourDirections)
{
   Rect r(Point(0.0f, 0.0f), Point(5.0f, 5.0f));
   r.unionPoint(Point(-2.0f, 3.0f));   // Expand left
   r.unionPoint(Point(3.0f, -1.0f));   // Expand down (negative y)
   r.unionPoint(Point(8.0f, 3.0f));    // Expand right
   r.unionPoint(Point(3.0f, 9.0f));    // Expand up
   EXPECT_FLOAT_EQ(-2.0f, r.min.x);
   EXPECT_FLOAT_EQ(-1.0f, r.min.y);
   EXPECT_FLOAT_EQ(8.0f,  r.max.x);
   EXPECT_FLOAT_EQ(9.0f,  r.max.y);
}


// ============================================================
// Rect::unionRect()
// ============================================================

TEST(RectTest, UnionRectOverlapping)
{
   Rect r1(Point(0.0f, 0.0f), Point(5.0f, 5.0f));
   Rect r2(Point(3.0f, 3.0f), Point(8.0f, 8.0f));
   r1.unionRect(r2);
   EXPECT_FLOAT_EQ(0.0f, r1.min.x);
   EXPECT_FLOAT_EQ(0.0f, r1.min.y);
   EXPECT_FLOAT_EQ(8.0f, r1.max.x);
   EXPECT_FLOAT_EQ(8.0f, r1.max.y);
}

TEST(RectTest, UnionRectSeparate)
{
   Rect r1(Point(0.0f, 0.0f), Point(3.0f, 3.0f));
   Rect r2(Point(7.0f, 7.0f), Point(10.0f, 10.0f));
   r1.unionRect(r2);
   EXPECT_FLOAT_EQ(0.0f,  r1.min.x);
   EXPECT_FLOAT_EQ(0.0f,  r1.min.y);
   EXPECT_FLOAT_EQ(10.0f, r1.max.x);
   EXPECT_FLOAT_EQ(10.0f, r1.max.y);
}

TEST(RectTest, UnionRectContained)
{
   Rect r1(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   Rect r2(Point(3.0f, 3.0f), Point(6.0f, 6.0f));
   r1.unionRect(r2);
   // r1 should be unchanged since r2 is inside it
   EXPECT_FLOAT_EQ(0.0f,  r1.min.x);
   EXPECT_FLOAT_EQ(0.0f,  r1.min.y);
   EXPECT_FLOAT_EQ(10.0f, r1.max.x);
   EXPECT_FLOAT_EQ(10.0f, r1.max.y);
}

TEST(RectTest, UnionRectNegativeCoords)
{
   Rect r1(Point(-5.0f, -5.0f), Point(5.0f, 5.0f));
   Rect r2(Point(-8.0f, -8.0f), Point(-2.0f, -2.0f));
   r1.unionRect(r2);
   EXPECT_FLOAT_EQ(-8.0f, r1.min.x);
   EXPECT_FLOAT_EQ(-8.0f, r1.min.y);
   EXPECT_FLOAT_EQ(5.0f,  r1.max.x);
   EXPECT_FLOAT_EQ(5.0f,  r1.max.y);
}


// ============================================================
// Rect::intersects(Rect)
// ============================================================

TEST(RectTest, IntersectsRectOverlapping)
{
   Rect r1(Point(0.0f, 0.0f), Point(5.0f, 5.0f));
   Rect r2(Point(3.0f, 3.0f), Point(8.0f, 8.0f));
   EXPECT_TRUE(r1.intersects(r2));
   EXPECT_TRUE(r2.intersects(r1));
}

TEST(RectTest, IntersectsRectNonOverlapping)
{
   Rect r1(Point(0.0f, 0.0f), Point(3.0f, 3.0f));
   Rect r2(Point(5.0f, 5.0f), Point(9.0f, 9.0f));
   EXPECT_FALSE(r1.intersects(r2));
   EXPECT_FALSE(r2.intersects(r1));
}

TEST(RectTest, IntersectsRectTouching)
{
   // Touching on edge — strict intersection check returns false
   Rect r1(Point(0.0f, 0.0f), Point(5.0f, 5.0f));
   Rect r2(Point(5.0f, 0.0f), Point(10.0f, 5.0f));
   EXPECT_FALSE(r1.intersects(r2));
}

TEST(RectTest, IntersectsRectContained)
{
   Rect r1(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   Rect r2(Point(2.0f, 2.0f), Point(8.0f, 8.0f));
   EXPECT_TRUE(r1.intersects(r2));
   EXPECT_TRUE(r2.intersects(r1));
}

TEST(RectTest, IntersectsRectSeparatedByGap)
{
   Rect r1(Point(0.0f, 0.0f), Point(3.0f, 10.0f));
   Rect r2(Point(5.0f, 0.0f), Point(8.0f, 10.0f));
   EXPECT_FALSE(r1.intersects(r2));
}


// ============================================================
// Rect::intersectsOrBorders()
// ============================================================

TEST(RectTest, IntersectsOrBordersTouching)
{
   Rect r1(Point(0.0f, 0.0f), Point(5.0f, 5.0f));
   Rect r2(Point(5.0f, 0.0f), Point(10.0f, 5.0f));
   EXPECT_TRUE(r1.intersectsOrBorders(r2));
}

TEST(RectTest, IntersectsOrBordersOverlapping)
{
   Rect r1(Point(0.0f, 0.0f), Point(5.0f, 5.0f));
   Rect r2(Point(3.0f, 3.0f), Point(8.0f, 8.0f));
   EXPECT_TRUE(r1.intersectsOrBorders(r2));
}

TEST(RectTest, IntersectsOrBordersJustSeparate)
{
   Rect r1(Point(0.0f, 0.0f), Point(5.0f, 5.0f));
   Rect r2(Point(6.1f, 0.0f), Point(10.0f, 5.0f));
   EXPECT_FALSE(r1.intersectsOrBorders(r2));
}

TEST(RectTest, IntersectsOrBordersJustInsideTolerance)
{
   Rect r1(Point(0.0f, 0.0f), Point(5.0f, 5.0f));
   // Slightly outside by less than tolerance (0.001)
   Rect r2(Point(5.0005f, 0.0f), Point(10.0f, 5.0f));
   EXPECT_TRUE(r1.intersectsOrBorders(r2));
}


// ============================================================
// Rect::intersects(Point, Point) — segment intersection
// ============================================================

TEST(RectTest, IntersectsSegmentCrossing)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   // Segment fully crosses rect
   EXPECT_TRUE(r.intersects(Point(-2.0f, 5.0f), Point(12.0f, 5.0f)));
}

TEST(RectTest, IntersectsSegmentInside)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   // Segment entirely inside rect
   EXPECT_TRUE(r.intersects(Point(2.0f, 2.0f), Point(8.0f, 8.0f)));
}

TEST(RectTest, IntersectsSegmentOutside)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   // Segment entirely outside rect
   EXPECT_FALSE(r.intersects(Point(12.0f, 0.0f), Point(20.0f, 10.0f)));
}

TEST(RectTest, IntersectsSegmentTouchingEdge)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   // Segment endpoint touching an edge
   EXPECT_TRUE(r.intersects(Point(-5.0f, 5.0f), Point(0.0f, 5.0f)));
}

TEST(RectTest, IntersectsSegmentWithCollisionTime)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   float ct = -1.0f;
   // Segment going left to right, entering the rect half way through
   EXPECT_TRUE(r.intersects(Point(-10.0f, 5.0f), Point(10.0f, 5.0f), ct));
   // ct should be ~0.5 (halfway along the 20-unit segment)
   EXPECT_NEAR(0.5f, ct, 0.001f);
}

TEST(RectTest, IntersectsSegmentContainedCollisionTimeZero)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   float ct = -1.0f;
   // Segment entirely inside — collision time should be 0
   EXPECT_TRUE(r.intersects(Point(2.0f, 2.0f), Point(8.0f, 8.0f), ct));
   EXPECT_FLOAT_EQ(0.0f, ct);
}


// ============================================================
// Rect::intersects(Point, radius) — circle intersection
// ============================================================

TEST(RectTest, IntersectsCircleCenter)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   EXPECT_TRUE(r.intersects(Point(5.0f, 5.0f), 1.0f));
}

TEST(RectTest, IntersectsCircleOverlapping)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   EXPECT_TRUE(r.intersects(Point(12.0f, 5.0f), 3.0f));
}

TEST(RectTest, IntersectsCircleTouchingEdge)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   EXPECT_TRUE(r.intersects(Point(12.0f, 5.0f), 2.0f));
}

TEST(RectTest, IntersectsCircleFarAway)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   EXPECT_FALSE(r.intersects(Point(20.0f, 5.0f), 3.0f));
}

TEST(RectTest, IntersectsCircleCorner)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   // Circle is near the corner of the rect, but close enough to intersect
   EXPECT_TRUE(r.intersects(Point(12.0f, 12.0f), 3.0f));   // ~2.83 from corner
   EXPECT_FALSE(r.intersects(Point(12.5f, 12.5f), 2.0f));  // ~3.54 from corner > 2
}

TEST(RectTest, IntersectsCircleTooFarFromCorner)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   EXPECT_FALSE(r.intersects(Point(12.0f, 12.0f), 2.0f));  // ~2.83 from corner > 2
}


// ============================================================
// Rect::expand()
// ============================================================

TEST(RectTest, ExpandPositiveDelta)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 10.0f));
   r.expand(Point(2.0f, 3.0f));
   EXPECT_FLOAT_EQ(-2.0f, r.min.x);
   EXPECT_FLOAT_EQ(-3.0f, r.min.y);
   EXPECT_FLOAT_EQ(12.0f, r.max.x);
   EXPECT_FLOAT_EQ(13.0f, r.max.y);
}

TEST(RectTest, ExpandZeroDelta)
{
   Rect r(Point(1.0f, 2.0f), Point(5.0f, 6.0f));
   r.expand(Point(0.0f, 0.0f));
   EXPECT_FLOAT_EQ(1.0f, r.min.x);
   EXPECT_FLOAT_EQ(2.0f, r.min.y);
   EXPECT_FLOAT_EQ(5.0f, r.max.x);
   EXPECT_FLOAT_EQ(6.0f, r.max.y);
}

TEST(RectTest, ExpandNegativeDelta)
{
   // Negative delta shrinks the rect
   Rect r(Point(-5.0f, -5.0f), Point(5.0f, 5.0f));
   r.expand(Point(-2.0f, -1.0f));
   EXPECT_FLOAT_EQ(-3.0f, r.min.x);
   EXPECT_FLOAT_EQ(-4.0f, r.min.y);
   EXPECT_FLOAT_EQ(3.0f,  r.max.x);
   EXPECT_FLOAT_EQ(4.0f,  r.max.y);
}


// ============================================================
// Rect::expandToInt()
// ============================================================

TEST(RectTest, ExpandToIntPositiveValues)
{
   Rect r(Point(0.2f, 0.3f), Point(9.7f, 9.6f));
   r.expandToInt(Point(0.0f, 0.0f));
   // Positive values: ceil => 10, 10; zero values may vary
   EXPECT_LE(r.min.x, 0.2f);  // min should be <= original
   EXPECT_GE(r.max.x, 9.7f);  // max should be >= original
}

TEST(RectTest, ExpandToIntNegativeMin)
{
   Rect r(Point(-3.5f, -4.5f), Point(3.5f, 4.5f));
   r.expandToInt(Point(0.0f, 0.0f));
   // Negative min: floor(-3.5) = -4; positive max: ceil(3.5) = 4
   EXPECT_FLOAT_EQ(-4.0f, r.min.x);
   EXPECT_FLOAT_EQ(-5.0f, r.min.y);
   EXPECT_FLOAT_EQ(4.0f,  r.max.x);
   EXPECT_FLOAT_EQ(5.0f,  r.max.y);
}


// ============================================================
// Rect::offset()
// ============================================================

TEST(RectTest, OffsetPositive)
{
   Rect r(Point(0.0f, 0.0f), Point(5.0f, 5.0f));
   r.offset(Point(3.0f, 4.0f));
   EXPECT_FLOAT_EQ(3.0f, r.min.x);
   EXPECT_FLOAT_EQ(4.0f, r.min.y);
   EXPECT_FLOAT_EQ(8.0f, r.max.x);
   EXPECT_FLOAT_EQ(9.0f, r.max.y);
}

TEST(RectTest, OffsetNegative)
{
   Rect r(Point(5.0f, 5.0f), Point(10.0f, 10.0f));
   r.offset(Point(-3.0f, -4.0f));
   EXPECT_FLOAT_EQ(2.0f, r.min.x);
   EXPECT_FLOAT_EQ(1.0f, r.min.y);
   EXPECT_FLOAT_EQ(7.0f, r.max.x);
   EXPECT_FLOAT_EQ(6.0f, r.max.y);
}

TEST(RectTest, OffsetPreservesSize)
{
   Rect r(Point(0.0f, 0.0f), Point(10.0f, 20.0f));
   float wBefore = r.getWidth();
   float hBefore = r.getHeight();
   r.offset(Point(100.0f, 200.0f));
   EXPECT_FLOAT_EQ(wBefore, r.getWidth());
   EXPECT_FLOAT_EQ(hBefore, r.getHeight());
}


// ============================================================
// Rect::toPoly()
// ============================================================

TEST(RectTest, ToPolyFourCorners)
{
   Rect r(Point(1.0f, 2.0f), Point(5.0f, 6.0f));
   Vector<Point> poly;
   r.toPoly(poly);
   ASSERT_EQ(4, poly.size());
   // Points should be the four corners in some order, with each unique
   // toPoly order: (max.x, max.y), (max.x, min.y), (min.x, min.y), (min.x, max.y)
   EXPECT_FLOAT_EQ(5.0f, poly[0].x); EXPECT_FLOAT_EQ(6.0f, poly[0].y);
   EXPECT_FLOAT_EQ(5.0f, poly[1].x); EXPECT_FLOAT_EQ(2.0f, poly[1].y);
   EXPECT_FLOAT_EQ(1.0f, poly[2].x); EXPECT_FLOAT_EQ(2.0f, poly[2].y);
   EXPECT_FLOAT_EQ(1.0f, poly[3].x); EXPECT_FLOAT_EQ(6.0f, poly[3].y);
}


// ============================================================
// Rect::getWidth() / getHeight() / getExtents()
// ============================================================

TEST(RectTest, GetWidthAndHeight)
{
   Rect r(Point(1.0f, 2.0f), Point(5.0f, 8.0f));
   EXPECT_FLOAT_EQ(4.0f, r.getWidth());
   EXPECT_FLOAT_EQ(6.0f, r.getHeight());
}

TEST(RectTest, GetWidthHeightZero)
{
   Rect r(Point(3.0f, 4.0f), Point(3.0f, 4.0f));
   EXPECT_FLOAT_EQ(0.0f, r.getWidth());
   EXPECT_FLOAT_EQ(0.0f, r.getHeight());
}

TEST(RectTest, GetExtents)
{
   Rect r(Point(1.0f, 2.0f), Point(5.0f, 8.0f));
   Point e = r.getExtents();
   EXPECT_FLOAT_EQ(4.0f, e.x);
   EXPECT_FLOAT_EQ(6.0f, e.y);
}

TEST(RectTest, GetExtentsIsWidthAndHeight)
{
   Rect r(Point(-3.0f, -2.0f), Point(4.0f, 7.0f));
   Point e = r.getExtents();
   EXPECT_FLOAT_EQ(r.getWidth(),  e.x);
   EXPECT_FLOAT_EQ(r.getHeight(), e.y);
}


// ============================================================
// Rect::toString()
// ============================================================

TEST(RectTest, ToString)
{
   Rect r(Point(1.0f, 2.0f), Point(3.0f, 4.0f));
   string s = r.toString();
   EXPECT_FALSE(s.empty());
   // It should contain the coordinates in some form
   EXPECT_NE(string::npos, s.find("1"));
   EXPECT_NE(string::npos, s.find("2"));
   EXPECT_NE(string::npos, s.find("3"));
   EXPECT_NE(string::npos, s.find("4"));
}


// ============================================================
// Rect::operator== and operator=
// ============================================================

TEST(RectTest, EqualityOperator)
{
   Rect r1(Point(1.0f, 2.0f), Point(3.0f, 4.0f));
   Rect r2(Point(1.0f, 2.0f), Point(3.0f, 4.0f));
   Rect r3(Point(0.0f, 2.0f), Point(3.0f, 4.0f));
   EXPECT_TRUE(r1 == r2);
   EXPECT_FALSE(r1 == r3);
}

TEST(RectTest, AssignmentOperator)
{
   Rect r1(Point(1.0f, 2.0f), Point(3.0f, 4.0f));
   Rect r2;
   r2 = r1;
   EXPECT_TRUE(r1 == r2);
   // Modifying r2 should not affect r1
   r2.offset(Point(1.0f, 0.0f));
   EXPECT_FALSE(r1 == r2);
}

TEST(RectTest, SelfAssignment)
{
   Rect r(Point(1.0f, 2.0f), Point(3.0f, 4.0f));
   r = r;
   EXPECT_FLOAT_EQ(1.0f, r.min.x);
   EXPECT_FLOAT_EQ(2.0f, r.min.y);
   EXPECT_FLOAT_EQ(3.0f, r.max.x);
   EXPECT_FLOAT_EQ(4.0f, r.max.y);
}


// ============================================================
// Edge cases / negative coordinates
// ============================================================

TEST(RectTest, NegativeCoordinates)
{
   Rect r(Point(-10.0f, -10.0f), Point(-1.0f, -1.0f));
   EXPECT_FLOAT_EQ(-10.0f, r.min.x);
   EXPECT_FLOAT_EQ(-1.0f,  r.max.x);
   EXPECT_TRUE(r.contains(Point(-5.0f, -5.0f)));
   EXPECT_FALSE(r.contains(Point(0.0f, 0.0f)));
}

TEST(RectTest, LargeCoordinates)
{
   Rect r(Point(-100000.0f, -100000.0f), Point(100000.0f, 100000.0f));
   EXPECT_FLOAT_EQ(200000.0f, r.getWidth());
   EXPECT_FLOAT_EQ(200000.0f, r.getHeight());
   EXPECT_TRUE(r.contains(Point(0.0f, 0.0f)));
   EXPECT_TRUE(r.contains(Point(99999.0f, 99999.0f)));
   EXPECT_FALSE(r.contains(Point(100001.0f, 0.0f)));
}

} // namespace Zap
