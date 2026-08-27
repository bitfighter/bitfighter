//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "../zap/GeomUtils.h"
#include "../zap/MathUtils.h"
#include "gtest/gtest.h"
#include <tnl.h>
#include <map>
#include <stdarg.h>

namespace Zap
{

using namespace std;
using namespace TNL;

#define ARRAYDEF(...) __VA_ARGS__

void parsePoly(const char* lines[], S32 size, Vector<Point> &result)
{
	map<char, Point> points;
	for(S32 i = 0; i < size; ++i)
	{
		for(S32 j = 0; lines[i][j] != '\0'; ++j)
		{
			char c = lines[i][j];

			if((c >= '0' && c <= '9') ||
			   (c >= 'a' && c <= 'z') ||
			   (c >= 'A' && c <= 'Z'))
			{
				points.insert(make_pair(c, Point(j * 10, i * 10)));
			}
		}
	}

	// maps are sorted by key on insertion
	for(map<char, Point>::iterator it = points.begin(); it != points.end(); ++it)
	{
		result.push_back((*it).second);
	}
}


/**
 * Macro to allow testing of polygons drawn with ascii art.
 * Only the numbers matter; the lines and dots are to help the
 * feeble human mind.
 */
#define POLY(_name, _lines) \
Vector<Point> _name;                                                           \
const char* _name##lines[] = _lines;                                           \
parsePoly(_name##lines, sizeof(_name##lines) / sizeof(_name##lines[0]), _name) \

TEST(GeomUtilsTest, splitSelfIntersecting)
{
	POLY(poly, ARRAYDEF({
		" 1---2 ",
		"  . /  ",
		"   .   ",
		"  / .  ",
		" 3---4 "
	}));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	splitSelfIntersectingPolys(polys, result);
	ASSERT_EQ(2, result.size());
	EXPECT_EQ(3, result[0].size());
	EXPECT_EQ(3, result[1].size());
}


TEST(GeomUtilsTest, splitRepeatedlySelfIntersecting)
{
	POLY(poly, ARRAYDEF({
		" 1--2  9--a ",
		" |  |  |  | ",
		" |  |  |  | ",
		" c--+--+--b ",
		"    |  |    ",
		"    |  |    ",
		" 5--+--+--6 ",
		" |  |  |  | ",
		" |  |  |  | ",
		" 4--3  8--7 "
	}));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	splitSelfIntersectingPolys(polys, result);

	ASSERT_EQ(5, result.size());
	for(S32 i = 0; i < result.size(); ++i)
	{
		EXPECT_EQ(4, result[i].size());
	}
}


TEST(GeomUtilsTest, splitRevisitedVertex)
{
	/*
	 * Can't express this one with the poly parser
	 *
	 * Center point is visited twice
	 * Looks like this:
	 * 1-2
	 * | |
	 * 8-+-4
	 *   | |
	 *   6-5
	 */
	Vector<Point> poly;
	poly.push_back(Point( 0,  0));
	poly.push_back(Point( 0,  0));
	poly.push_back(Point(10,  0));
	poly.push_back(Point(10, 10));
	poly.push_back(Point(20, 10));
	poly.push_back(Point(20, 20));
	poly.push_back(Point(10, 20));
	poly.push_back(Point(10, 10));
	poly.push_back(Point( 0, 10));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	splitSelfIntersectingPolys(polys, result);

	/* known failure
	ASSERT_EQ(2, result.size());
	EXPECT_EQ(4, result[0].size());
	EXPECT_EQ(4, result[1].size());
	*/
}


TEST(GeomUtilsTest, triangulateCW)
{
	POLY(poly, ARRAYDEF({
		" 1-2 ",
		" | | ",
		" 4-3 "
	}));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(2, result.size());
}


TEST(GeomUtilsTest, triangulateCCW)
{
	POLY(poly, ARRAYDEF({
		" 1-4 ",
		" | | ",
		" 2-3 "
	}));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(2, result.size());
}


TEST(GeomUtilsTest, triangulateBarelyTouching)
{
	POLY(poly, ARRAYDEF({
		" 1-4   ",
		" | |   ",
		" 2-3.. "
		"   . . "
		"   ... "
	}));

	POLY(poly2, ARRAYDEF({
		" ...   "
		" . .   "
		" ..1-4 ",
		"   | | ",
		"   2-3 "
	}));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);
	polys.push_back(poly2);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(4, result.size());
}


TEST(GeomUtilsTest, triangulateOverlapping)
{
	POLY(poly, ARRAYDEF({
		" 1---4   ",
		" |   |   ",
		" | ..|.. ",
		" | . | . ",
		" 2---3 . "
		"   .   . ",
		"   ..... ",
	}));

	POLY(poly2, ARRAYDEF({
		" .....   ",
		" .   .   ",
		" . 1---4 ",
		" . | . | ",
		" ..|.. | ",
		"   |   | ",
		"   2---3 "
	}));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);
	polys.push_back(poly2);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_NE(0, result.size());
}


TEST(GeomUtilsTest, triangulateSelfIntersecting)
{
	POLY(poly, ARRAYDEF({
		" 1---2 ",
		"  ` /  ",
		"   `   ",
		"  / `  ",
		" 3---4 "
	}));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(2, result.size());
}


TEST(GeomUtilsTest, triangulateColinearAdjacentSegments)
{
	POLY(poly, ARRAYDEF({
		" 1---5 ",
		" |   | ",
		" 2   | ",
		" |   | ",
		" 3---4 "
	}));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(2, result.size());
}


TEST(GeomUtilsTest, triangulateOverlappingSegments)
{
	POLY(poly, ARRAYDEF({
		" 1-8     ",
		" | |     ",
		" 2-7-3-6 ",
		"     | | ",
		"     4-5 "
	}));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(4, result.size());
}


TEST(GeomUtilsTest, triangulateRepeatedlySelfIntersecting)
{
	POLY(poly, ARRAYDEF({
		" 1--2  9--a ",
		" |  |  |  | ",
		" |  |  |  | ",
		" c--+--+--b ",
		"    |  |    ",
		"    |  |    ",
		" 5--+--+--6 ",
		" |  |  |  | ",
		" |  |  |  | ",
		" 4--3  8--7 "
	}));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(10, result.size());
}


TEST(GeomUtilsTest, triangulateConsecutiveRepeatVertices)
{
	/*
	 * Center point is visited twice
	 * Looks like this:
	 * 1-+
	 * | |
	 * 5-4
	 */
	Vector<Point> poly;
	poly.push_back(Point( 0,  0));
	poly.push_back(Point(10,  0));
	poly.push_back(Point(10,  0));
	poly.push_back(Point(10, 10));
	poly.push_back(Point(20, 10));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(2, result.size());
}


TEST(GeomUtilsTest, triangulateWithRevisitedVertex)
{
	/*
	 * Center point is visited twice
	 * Looks like this:
	 * 1-2
	 * | |
	 * 8-+-4
	 *   | |
	 *   6-5
	 */
	Vector<Point> poly;
	poly.push_back(Point( 0,  0));
	poly.push_back(Point( 0,  0));
	poly.push_back(Point(10,  0));
	poly.push_back(Point(10, 10));
	poly.push_back(Point(20, 10));
	poly.push_back(Point(20, 20));
	poly.push_back(Point(10, 20));
	poly.push_back(Point(10, 10));
	poly.push_back(Point( 0, 10));

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(4, result.size());
}


TEST(GeomUtilsTest, triangulateLongPolygon)
{
	Vector<Point> poly = createPolygon(Point(), 100, 512, 0);

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
}


TEST(GeomUtilsTest, triangulateManyPolygons)
{
	Vector<Vector<Point> > polys;
	for(S32 i = 0; i < 512; ++i)
	{
		Vector<Point> poly = createPolygon(Point(i * 120, i * 120), 100, 4, 0);
		polys.push_back(poly);
	}

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(1024, result.size());
}


TEST(GeomUtilsTest, pointInHexagon)
{
   // Some obviously bogus cases
   EXPECT_FALSE(pointInHexagon(Point( 10,  10), Point(0, 0), 5));
   EXPECT_FALSE(pointInHexagon(Point(-10,  10), Point(0, 0), 1));
   EXPECT_FALSE(pointInHexagon(Point(-10, -10), Point(0, 0), 8));
   EXPECT_FALSE(pointInHexagon(Point( 10, -10), Point(0, 0), 9));

   // Obviously true cases
   EXPECT_TRUE(pointInHexagon(Point(0, 0), Point(0, 0), 9));
   EXPECT_TRUE(pointInHexagon(Point(0, 0), Point(1, 1), 9));

   // Outside, but within bounding box
   EXPECT_FALSE(pointInHexagon(Point(-0.9, FloatSqrt3Half - .0001), Point(0, 0), 1));

   Point PointOnOutside(FloatSqrt3Half * cos(30 * DEGREES_TO_RADIANS), FloatSqrt3Half * sin(30 * DEGREES_TO_RADIANS));

   EXPECT_FALSE(pointInHexagon(Point(PointOnOutside.x + .000001, PointOnOutside.y),           Point(0, 0), 1));
   EXPECT_FALSE(pointInHexagon(Point(PointOnOutside.x,           PointOnOutside.y + .000001), Point(0, 0), 1));
   EXPECT_FALSE(pointInHexagon(Point(PointOnOutside.x + .000001, PointOnOutside.y + .000001), Point(0, 0), 1));

   // Just inside
   EXPECT_TRUE(pointInHexagon(Point(PointOnOutside.x - .000001, PointOnOutside.y),           Point(0, 0), 1));
   EXPECT_TRUE(pointInHexagon(Point(PointOnOutside.x,           PointOnOutside.y - .000001), Point(0, 0), 1));
   EXPECT_TRUE(pointInHexagon(Point(PointOnOutside.x - .000001, PointOnOutside.y - .000001), Point(0, 0), 1));
}


// ============================================================
// polygonContainsPoint
// ============================================================

TEST(GeomUtilsTest, polygonContainsPoint)
{
   // Square polygon
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));

   // Clearly inside
   EXPECT_TRUE(polygonContainsPoint(square.address(), square.size(), Point(5, 5)));

   // Clearly outside
   EXPECT_FALSE(polygonContainsPoint(square.address(), square.size(), Point(15, 5)));
   EXPECT_FALSE(polygonContainsPoint(square.address(), square.size(), Point(-1, 5)));
   EXPECT_FALSE(polygonContainsPoint(square.address(), square.size(), Point(5, -1)));
   EXPECT_FALSE(polygonContainsPoint(square.address(), square.size(), Point(5, 15)));

   // Near corners
   EXPECT_FALSE(polygonContainsPoint(square.address(), square.size(), Point(-0.1f, -0.1f)));
   EXPECT_FALSE(polygonContainsPoint(square.address(), square.size(), Point(10.1f, 10.1f)));
}

TEST(GeomUtilsTest, polygonContainsPointPrecisionBug)
{
   // Square polygon
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));

   // Point slightly outside on the left
   // isLeft should be -0.01, but truncated to 0, causing polygonContainsPoint to return true incorrectly
   EXPECT_FALSE(polygonContainsPoint(square.address(), square.size(), Point(-0.001f, 5.0f)));

   // Point slightly inside on the left
   // isLeft should be 0.01, but truncated to 0, causing polygonContainsPoint to return false incorrectly
   // Actually, for (0.001, 5), edge (0,10)->(0,0) is a downward crossing.
   // isLeft is (0-0)*(5-10) - (0.001-0)*(0-10) = 0.01
   // Counter starts at 1 from edge (10,0)->(10,10)
   // Downward crossing edge (0,10)->(0,0) should NOT decrement counter if point is left of it (inside)
   // 0.01 > 0, so it's not < 0. Counter remains 1. Correct.
   EXPECT_TRUE(polygonContainsPoint(square.address(), square.size(), Point(0.001f, 5.0f)));

   // Point slightly outside on the right
   // Edge (10,0) -> (10,10) is upward crossing.
   // isLeft((10,0), (10,10), (10.001, 5)) = (10-10)*(5-0) - (10.001-10)*(10-0) = -0.01
   // Truncated to 0. 0 > 0 is false. Counter remains 0. Correct (outside).

   // Point slightly inside on the right
   // isLeft((10,0), (10,10), (9.999, 5)) = (10-10)*(5-0) - (9.999-10)*(10-0) = 0.01
   // Truncated to 0. 0 > 0 is false. Counter remains 0. WRONG (inside).
   EXPECT_TRUE(polygonContainsPoint(square.address(), square.size(), Point(9.999f, 5.0f)));
}

TEST(GeomUtilsTest, polygonContainsPointTriangle)
{
   Vector<Point> tri;
   tri.push_back(Point(0, 0));
   tri.push_back(Point(10, 0));
   tri.push_back(Point(5, 10));

   EXPECT_TRUE(polygonContainsPoint(tri.address(), tri.size(), Point(5, 5)));
   EXPECT_FALSE(polygonContainsPoint(tri.address(), tri.size(), Point(0.1f, 9.0f)));
   EXPECT_FALSE(polygonContainsPoint(tri.address(), tri.size(), Point(9.9f, 9.0f)));
}

TEST(GeomUtilsTest, polygonContainsPointConcave)
{
   // L-shaped polygon
   Vector<Point> lshape;
   lshape.push_back(Point(0, 0));
   lshape.push_back(Point(10, 0));
   lshape.push_back(Point(10, 5));
   lshape.push_back(Point(5, 5));
   lshape.push_back(Point(5, 10));
   lshape.push_back(Point(0, 10));

   EXPECT_TRUE(polygonContainsPoint(lshape.address(), lshape.size(), Point(2, 2)));
   EXPECT_TRUE(polygonContainsPoint(lshape.address(), lshape.size(), Point(2, 8)));
   EXPECT_TRUE(polygonContainsPoint(lshape.address(), lshape.size(), Point(8, 2)));
   // In the "missing" upper-right part
   EXPECT_FALSE(polygonContainsPoint(lshape.address(), lshape.size(), Point(8, 8)));
}


// ============================================================
// segmentsIntersect
// ============================================================

TEST(GeomUtilsTest, segmentsIntersectCrossing)
{
   F32 ct;
   // Two crossing segments
   EXPECT_TRUE(segmentsIntersect(Point(0, 0), Point(10, 10),
                                 Point(0, 10), Point(10, 0), ct));
   EXPECT_NEAR(0.5f, ct, 0.001f);
}

TEST(GeomUtilsTest, segmentsIntersectNonCrossing)
{
   F32 ct;
   // Parallel segments
   EXPECT_FALSE(segmentsIntersect(Point(0, 0), Point(10, 0),
                                  Point(0, 5), Point(10, 5), ct));
}

TEST(GeomUtilsTest, segmentsIntersectCollinear)
{
   F32 ct;
   // Collinear — parallel so returns false
   EXPECT_FALSE(segmentsIntersect(Point(0, 0), Point(10, 0),
                                  Point(5, 0), Point(15, 0), ct));
}

TEST(GeomUtilsTest, segmentsIntersectTShape)
{
   F32 ct;
   // T-shaped intersection at midpoint of one segment
   EXPECT_TRUE(segmentsIntersect(Point(0, 5), Point(10, 5),
                                 Point(5, 0), Point(5, 10), ct));
   EXPECT_NEAR(0.5f, ct, 0.001f);
}

TEST(GeomUtilsTest, segmentsIntersectSharedEndpoint)
{
   F32 ct;
   // Segments sharing an endpoint
   EXPECT_TRUE(segmentsIntersect(Point(0, 0), Point(5, 5),
                                 Point(5, 5), Point(10, 0), ct));
   EXPECT_NEAR(1.0f, ct, 0.001f);
}

TEST(GeomUtilsTest, segmentsIntersectNearButNotCrossing)
{
   F32 ct;
   // Segments extend toward each other but do not overlap
   EXPECT_FALSE(segmentsIntersect(Point(0, 0), Point(4, 4),
                                  Point(6, 6), Point(10, 0), ct));
}

TEST(GeomUtilsTest, segmentsIntersectPrecisionBug)
{
   F32 ct;
   F32 offset = 1e6f;

   // A: (offset, offset) -> (offset + 10, offset + 10)
   // B: (offset, offset + 10) -> (offset + 10, offset)
   // These should intersect at (offset + 5, offset + 5)

   EXPECT_TRUE(segmentsIntersect(Point(offset, offset), Point(offset + 10, offset + 10),
                                 Point(offset, offset + 10), Point(offset + 10, offset), ct));
   EXPECT_NEAR(0.5f, ct, 0.001f);

   // Nearly parallel
   // A: (offset, offset) -> (offset + 100, offset + 100)
   // B: (offset, offset + 100.0001) -> (offset + 100, offset + 0.0001)
   // These should intersect at (offset + 50, offset + 50.0001)
   EXPECT_TRUE(segmentsIntersect(Point(offset, offset), Point(offset + 100, offset + 100),
                                 Point(offset, offset + 100.0001f), Point(offset + 100, offset + 0.0001f), ct));
}


// ============================================================
// findIntersection
// ============================================================

TEST(GeomUtilsTest, findIntersectionCrossing)
{
   Point intersection;
   EXPECT_TRUE(findIntersection(Point(0, 0), Point(10, 10),
                                Point(0, 10), Point(10, 0), intersection));
   EXPECT_NEAR(5.0f, intersection.x, 0.001f);
   EXPECT_NEAR(5.0f, intersection.y, 0.001f);
}

TEST(GeomUtilsTest, findIntersectionPrecisionBug)
{
   Point intersection;
   F32 offset = 1e6f;

   // Intersection at (offset + 5, offset + 5)
   EXPECT_TRUE(findIntersection(Point(offset, offset), Point(offset + 10, offset + 10),
                                Point(offset, offset + 10), Point(offset + 10, offset), intersection));
   EXPECT_NEAR(offset + 5.0f, intersection.x, 1.0f);
   EXPECT_NEAR(offset + 5.0f, intersection.y, 1.0f);
}

TEST(GeomUtilsTest, findIntersectionParallel)
{
   Point intersection;
   EXPECT_FALSE(findIntersection(Point(0, 0), Point(10, 0),
                                 Point(0, 5), Point(10, 5), intersection));
}

TEST(GeomUtilsTest, findIntersectionNoOverlap)
{
   Point intersection;
   // Lines would intersect if extended, but segments don't
   EXPECT_FALSE(findIntersection(Point(0, 0), Point(4, 4),
                                 Point(6, 6), Point(10, 0), intersection));
}

TEST(GeomUtilsTest, findIntersectionAtEndpoint)
{
   Point intersection;
   EXPECT_TRUE(findIntersection(Point(0, 0), Point(5, 5),
                                Point(5, 5), Point(10, 0), intersection));
   EXPECT_NEAR(5.0f, intersection.x, 0.001f);
   EXPECT_NEAR(5.0f, intersection.y, 0.001f);
}


// ============================================================
// findNormalPoint
// ============================================================

TEST(GeomUtilsTest, findNormalPointOnSegment)
{
   Point closest;
   // Perpendicular from (5, 5) to segment (0,0)-(10,0) => should land at (5, 0)
   EXPECT_TRUE(findNormalPoint(Point(5, 5), Point(0, 0), Point(10, 0), closest));
   EXPECT_NEAR(5.0f, closest.x, 0.001f);
   EXPECT_NEAR(0.0f, closest.y, 0.001f);
}

TEST(GeomUtilsTest, findNormalPointBeyondEndpoint)
{
   Point closest;
   // Projection falls outside the segment
   EXPECT_FALSE(findNormalPoint(Point(15, 5), Point(0, 0), Point(10, 0), closest));
   EXPECT_FALSE(findNormalPoint(Point(-5, 5), Point(0, 0), Point(10, 0), closest));
}

TEST(GeomUtilsTest, findNormalPointMidVertical)
{
   Point closest;
   EXPECT_TRUE(findNormalPoint(Point(0, 5), Point(0, 0), Point(0, 10), closest));
   EXPECT_NEAR(0.0f, closest.x, 0.001f);
   EXPECT_NEAR(5.0f, closest.y, 0.001f);
}


// ============================================================
// pointOnSegment
// ============================================================

TEST(GeomUtilsTest, pointOnSegmentMidpoint)
{
   F32 closeEnough = 0.01f;
   EXPECT_TRUE(pointOnSegment(Point(5, 0), Point(0, 0), Point(10, 0), closeEnough));
}

TEST(GeomUtilsTest, pointOnSegmentAtEndpoint)
{
   F32 closeEnough = 0.01f;
   EXPECT_TRUE(pointOnSegment(Point(0, 0), Point(0, 0), Point(10, 0), closeEnough));
   EXPECT_TRUE(pointOnSegment(Point(10, 0), Point(0, 0), Point(10, 0), closeEnough));
}

TEST(GeomUtilsTest, pointOnSegmentOff)
{
   F32 closeEnough = 0.0001f;
   EXPECT_FALSE(pointOnSegment(Point(5, 5), Point(0, 0), Point(10, 0), closeEnough));
   EXPECT_FALSE(pointOnSegment(Point(15, 0), Point(0, 0), Point(10, 0), closeEnough));
}

TEST(GeomUtilsTest, pointOnSegmentCloseEnough)
{
   F32 closeEnough = 1.0f;  // Within 1 unit
   // Nearly on segment — distSq to endpoint = 0.25 < 1.0
   EXPECT_TRUE(pointOnSegment(Point(0.5f, 0), Point(0, 0), Point(10, 0), closeEnough));
}


// ============================================================
// circleCircleIntersect
// ============================================================

TEST(GeomUtilsTest, circleCircleIntersectOverlapping)
{
   EXPECT_TRUE(circleCircleIntersect(Point(0, 0), 5.0f, Point(8, 0), 5.0f));
}

TEST(GeomUtilsTest, circleCircleIntersectTouching)
{
   // Exactly touching — sum of radii equals distance
   EXPECT_FALSE(circleCircleIntersect(Point(0, 0), 5.0f, Point(10, 0), 5.0f));
}

TEST(GeomUtilsTest, circleCircleIntersectSeparate)
{
   EXPECT_FALSE(circleCircleIntersect(Point(0, 0), 3.0f, Point(10, 0), 3.0f));
}

TEST(GeomUtilsTest, circleCircleIntersectContained)
{
   // One circle inside the other
   EXPECT_TRUE(circleCircleIntersect(Point(0, 0), 10.0f, Point(1, 1), 2.0f));
}

TEST(GeomUtilsTest, circleCircleIntersectSameCenter)
{
   EXPECT_TRUE(circleCircleIntersect(Point(0, 0), 5.0f, Point(0, 0), 5.0f));
}


// ============================================================
// polygonCircleIntersect
// ============================================================

TEST(GeomUtilsTest, polygonCircleIntersectCenterInside)
{
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));

   Point outPoint;
   // Center inside polygon — any radius (even 0) should return true
   EXPECT_TRUE(polygonCircleIntersect(square.address(), square.size(),
                                       Point(5, 5), 0.0f, outPoint));
}

TEST(GeomUtilsTest, polygonCircleIntersectTouchingEdge)
{
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));

   Point outPoint;
   // Circle center at (13, 5), radius^2 = 16 (radius=4): edge at x=10 is 3 units away, within radius
   EXPECT_TRUE(polygonCircleIntersect(square.address(), square.size(),
                                       Point(13, 5), 16.0f /* radius^2=16, r=4 */, outPoint));
}

TEST(GeomUtilsTest, polygonCircleIntersectFarAway)
{
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));

   Point outPoint;
   // Circle center at (30, 30), radius^2 = 4 (radius=2): nearest square corner is ~28 units away
   EXPECT_FALSE(polygonCircleIntersect(square.address(), square.size(),
                                        Point(30, 30), 4.0f, outPoint));
}


// ============================================================
// circleIntersectsSegment
// ============================================================

TEST(GeomUtilsTest, circleIntersectsSegmentStartInside)
{
   float ct;
   // Start point inside circle
   EXPECT_TRUE(circleIntersectsSegment(Point(5, 5), 3.0f, Point(5, 5), Point(10, 5), ct));
   EXPECT_FLOAT_EQ(0.0f, ct);
}

TEST(GeomUtilsTest, circleIntersectsSegmentCrossing)
{
   float ct;
   EXPECT_TRUE(circleIntersectsSegment(Point(5, 0), 3.0f, Point(0, 0), Point(10, 0), ct));
   EXPECT_GT(ct, 0.0f);
   EXPECT_LE(ct, 1.0f);
}

TEST(GeomUtilsTest, circleIntersectsSegmentNoCrossing)
{
   float ct;
   // Circle at (0, 10), segment along x-axis from (0, 0) to (10, 0)
   EXPECT_FALSE(circleIntersectsSegment(Point(0, 10), 3.0f, Point(0, 0), Point(10, 0), ct));
}

TEST(GeomUtilsTest, circleIntersectsSegmentStartOnEdge)
{
   float ct;
   // Start point exactly on circle boundary
   EXPECT_TRUE(circleIntersectsSegment(Point(5, 0), 5.0f, Point(0, 0), Point(10, 0), ct));
   EXPECT_FLOAT_EQ(0.0f, ct);
}


// ============================================================
// polygonIntersectsSegment
// ============================================================

TEST(GeomUtilsTest, polygonIntersectsSegmentCrossing)
{
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));

   EXPECT_TRUE(polygonIntersectsSegment(square, Point(-5, 5), Point(15, 5)));
}

TEST(GeomUtilsTest, polygonIntersectsSegmentInside)
{
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));

   EXPECT_TRUE(polygonIntersectsSegment(square, Point(2, 2), Point(8, 8)));
}

TEST(GeomUtilsTest, polygonIntersectsSegmentOutside)
{
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));

   EXPECT_FALSE(polygonIntersectsSegment(square, Point(20, 0), Point(30, 10)));
}

TEST(GeomUtilsTest, polygonIntersectsSegmentParallelEdge)
{
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));

   // Segment parallel to and outside the edge
   EXPECT_FALSE(polygonIntersectsSegment(square, Point(0, -5), Point(10, -5)));
}


// ============================================================
// polygonsIntersect
// ============================================================

TEST(GeomUtilsTest, polygonsIntersectOverlapping)
{
   Vector<Point> p1, p2;
   p1.push_back(Point(0, 0)); p1.push_back(Point(10, 0));
   p1.push_back(Point(10, 10)); p1.push_back(Point(0, 10));
   p2.push_back(Point(5, 5)); p2.push_back(Point(15, 5));
   p2.push_back(Point(15, 15)); p2.push_back(Point(5, 15));

   EXPECT_TRUE(polygonsIntersect(p1, p2));
   EXPECT_TRUE(polygonsIntersect(p2, p1));
}

TEST(GeomUtilsTest, polygonsIntersectSeparate)
{
   Vector<Point> p1, p2;
   p1.push_back(Point(0, 0)); p1.push_back(Point(5, 0));
   p1.push_back(Point(5, 5)); p1.push_back(Point(0, 5));
   p2.push_back(Point(10, 10)); p2.push_back(Point(15, 10));
   p2.push_back(Point(15, 15)); p2.push_back(Point(10, 15));

   EXPECT_FALSE(polygonsIntersect(p1, p2));
}

TEST(GeomUtilsTest, polygonsIntersectOneContained)
{
   Vector<Point> outer, inner;
   outer.push_back(Point(0, 0)); outer.push_back(Point(20, 0));
   outer.push_back(Point(20, 20)); outer.push_back(Point(0, 20));
   inner.push_back(Point(5, 5)); inner.push_back(Point(10, 5));
   inner.push_back(Point(10, 10)); inner.push_back(Point(5, 10));

   EXPECT_TRUE(polygonsIntersect(outer, inner));
   EXPECT_TRUE(polygonsIntersect(inner, outer));
}


// ============================================================
// shortenSegment
// ============================================================

TEST(GeomUtilsTest, shortenSegmentBasic)
{
   // Segment of length 10 along x-axis, shorten by 2
   Point end = shortenSegment(Point(0, 0), Point(10, 0), 2.0f);
   EXPECT_NEAR(8.0f, end.x, 0.001f);
   EXPECT_NEAR(0.0f, end.y, 0.001f);
}

TEST(GeomUtilsTest, shortenSegmentDiagonal)
{
   // Segment of length 5 (3-4-5 triangle), shorten by 5 (to zero length)
   Point start(0, 0), endPt(3, 4);
   Point result = shortenSegment(start, endPt, 5.0f);
   EXPECT_NEAR(0.0f, result.x, 0.001f);
   EXPECT_NEAR(0.0f, result.y, 0.001f);
}

TEST(GeomUtilsTest, shortenSegmentNoReduction)
{
   // Reduce by 0
   Point end = shortenSegment(Point(0, 0), Point(10, 0), 0.0f);
   EXPECT_NEAR(10.0f, end.x, 0.001f);
   EXPECT_NEAR(0.0f, end.y, 0.001f);
}


// ============================================================
// area
// ============================================================

TEST(GeomUtilsTest, areaSquare)
{
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));
   // CCW winding => positive area
   EXPECT_NEAR(100.0f, fabs(area(square)), 0.001f);
}

TEST(GeomUtilsTest, areaTriangle)
{
   Vector<Point> tri;
   tri.push_back(Point(0, 0));
   tri.push_back(Point(6, 0));
   tri.push_back(Point(0, 4));
   // Area = 0.5 * base * height = 12
   EXPECT_NEAR(12.0f, fabs(area(tri)), 0.001f);
}

TEST(GeomUtilsTest, areaZeroForLine)
{
   Vector<Point> line;
   line.push_back(Point(0, 0));
   line.push_back(Point(5, 0));
   line.push_back(Point(10, 0));
   EXPECT_NEAR(0.0f, fabs(area(line)), 0.001f);
}

TEST(GeomUtilsTest, areaSignDependsOnWinding)
{
   Vector<Point> cw, ccw;
   // CW: 
   cw.push_back(Point(0, 0)); cw.push_back(Point(0, 10));
   cw.push_back(Point(10, 10)); cw.push_back(Point(10, 0));
   // CCW:
   ccw.push_back(Point(0, 0)); ccw.push_back(Point(10, 0));
   ccw.push_back(Point(10, 10)); ccw.push_back(Point(0, 10));

   // One positive, one negative
   EXPECT_TRUE(area(cw) * area(ccw) < 0.0f);
}

TEST(GeomUtilsTest, areaLargeCoordinates)
{
   F32 offset = 1e6f;
   Vector<Point> square;
   square.push_back(Point(offset, offset));
   square.push_back(Point(offset + 10, offset));
   square.push_back(Point(offset + 10, offset + 10));
   square.push_back(Point(offset, offset + 10));

   EXPECT_NEAR(100.0f, fabs(area(square)), 0.001f);
}


// ============================================================
// mean2d
// ============================================================

TEST(GeomUtilsTest, mean2dSquare)
{
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));
   Point m = mean2d(square);
   EXPECT_NEAR(5.0f, m.x, 0.001f);
   EXPECT_NEAR(5.0f, m.y, 0.001f);
}

TEST(GeomUtilsTest, mean2dSinglePoint)
{
   Vector<Point> pts;
   pts.push_back(Point(3, 7));
   Point m = mean2d(pts);
   EXPECT_NEAR(3.0f, m.x, 0.001f);
   EXPECT_NEAR(7.0f, m.y, 0.001f);
}

TEST(GeomUtilsTest, mean2dAsymmetric)
{
   Vector<Point> pts;
   pts.push_back(Point(0, 0));
   pts.push_back(Point(6, 0));
   pts.push_back(Point(0, 6));
   Point m = mean2d(pts);
   EXPECT_NEAR(2.0f, m.x, 0.001f);
   EXPECT_NEAR(2.0f, m.y, 0.001f);
}


// ============================================================
// findCentroid
// ============================================================

TEST(GeomUtilsTest, findCentroidSquare)
{
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));
   Point c = findCentroid(square);
   EXPECT_NEAR(5.0f, c.x, 0.01f);
   EXPECT_NEAR(5.0f, c.y, 0.01f);
}

TEST(GeomUtilsTest, findCentroidLargeCoordinates)
{
   F32 offset = 1e6f;
   Vector<Point> square;
   square.push_back(Point(offset, offset));
   square.push_back(Point(offset + 10, offset));
   square.push_back(Point(offset + 10, offset + 10));
   square.push_back(Point(offset, offset + 10));

   Point c = findCentroid(square);
   EXPECT_NEAR(offset + 5.0f, c.x, 0.01f);
   EXPECT_NEAR(offset + 5.0f, c.y, 0.01f);
}

TEST(GeomUtilsTest, findCentroidSmallPolygon)
{
   Vector<Point> poly;
   // Square (0,0) to (0.1, 0.1) with an extra vertex on one side
   poly.push_back(Point(0, 0));
   poly.push_back(Point(0.1, 0));
   poly.push_back(Point(0.1, 0.05)); // Extra vertex
   poly.push_back(Point(0.1, 0.1));
   poly.push_back(Point(0, 0.1));

   Point c = findCentroid(poly);
   // Should be (0.05, 0.05). If it falls back to mean2d, it will be (0.06, 0.05).
   EXPECT_NEAR(0.05f, c.x, 0.001f);
   EXPECT_NEAR(0.05f, c.y, 0.001f);
}

TEST(GeomUtilsTest, findCentroidFloatingPointPrecision)
{
   Vector<Point> poly;
   // A polygon with area between 0 and 1, specifically less than 0.5
   // Area = 0.5 * 0.4 * 0.4 = 0.08
   poly.push_back(Point(0, 0));
   poly.push_back(Point(0.4, 0));
   poly.push_back(Point(0, 0.4));

   Point c = findCentroid(poly);
   // Centroid of triangle is (x1+x2+x3)/3, (y1+y2+y3)/3
   // (0+0.4+0)/3 = 0.1333..., (0+0+0.4)/3 = 0.1333...
   // If area (0.08) was truncated to 0, it would fallback to mean2d:
   // (0+0.4+0)/3 = 0.1333... which happens to be the same for a triangle!
   // Let's use a square to distinguish.

   poly.clear();
   poly.push_back(Point(0, 0));
   poly.push_back(Point(0.4, 0));
   poly.push_back(Point(0.4, 0.4));
   poly.push_back(Point(0, 0.4));
   // Area = 0.16. sArea = 0.32.
   // Correct Centroid: (0.2, 0.2)
   // mean2d: (0+0.4+0.4+0)/4 = 0.2. Still the same.

   // Let's use an asymmetric one.
   poly.clear();
   poly.push_back(Point(0, 0));
   poly.push_back(Point(0.6, 0));
   poly.push_back(Point(0.6, 0.2));
   poly.push_back(Point(0, 0.2));
   // Area = 0.12. sArea = 0.24.
   // Correct Centroid: (0.3, 0.1)
   // mean2d: (0+0.6+0.6+0)/4 = 0.3, (0+0+0.2+0.2)/4 = 0.1. Argh.

   // Triangle again, but with extra points on one side.
   poly.clear();
   poly.push_back(Point(0, 0));
   poly.push_back(Point(0.6, 0));
   poly.push_back(Point(0.3, 0.6));
   poly.push_back(Point(0.15, 0.3)); // On edge (0,0)-(0.3,0.6)

   // This triangle has area 0.5 * 0.6 * 0.6 = 0.18. sArea = 0.36.
   // Centroid: (0+0.6+0.3)/3 = 0.3, (0+0+0.6)/3 = 0.2.
   // mean2d: (0+0.6+0.3+0.15)/4 = 0.2625, (0+0+0.6+0.3)/4 = 0.225.

   c = findCentroid(poly);
   EXPECT_NEAR(0.3f, c.x, 0.001f);
   EXPECT_NEAR(0.2f, c.y, 0.001f);
}

TEST(GeomUtilsTest, findCentroidEmptyReturnOrigin)
{
   Vector<Point> empty;
   Point c = findCentroid(empty);
   EXPECT_FLOAT_EQ(0.0f, c.x);
   EXPECT_FLOAT_EQ(0.0f, c.y);
}

TEST(GeomUtilsTest, findCentroidTriangle)
{
   Vector<Point> tri;
   tri.push_back(Point(0, 0));
   tri.push_back(Point(6, 0));
   tri.push_back(Point(0, 6));
   Point c = findCentroid(tri);
   EXPECT_NEAR(2.0f, c.x, 0.01f);
   EXPECT_NEAR(2.0f, c.y, 0.01f);
}

TEST(GeomUtilsTest, findCentroidOffCenterRect)
{
   Vector<Point> rect;
   rect.push_back(Point(2, 3));
   rect.push_back(Point(8, 3));
   rect.push_back(Point(8, 7));
   rect.push_back(Point(2, 7));
   Point c = findCentroid(rect);
   EXPECT_NEAR(5.0f, c.x, 0.01f);
   EXPECT_NEAR(5.0f, c.y, 0.01f);
}


// ============================================================
// angleOfLongestSide
// ============================================================

TEST(GeomUtilsTest, angleOfLongestSideHorizontal)
{
   // Wide rectangle: longest sides are horizontal
   Vector<Point> rect;
   rect.push_back(Point(0, 0));
   rect.push_back(Point(20, 0));
   rect.push_back(Point(20, 5));
   rect.push_back(Point(0, 5));
   F32 ang = angleOfLongestSide(rect);
   // Should be near 0 (or FloatPi, adjusted to be right-side-up)
   EXPECT_NEAR(0.0f, fabs(ang), 0.01f);
}

TEST(GeomUtilsTest, angleOfLongestSideOnlyOnePoint)
{
   Vector<Point> pts;
   pts.push_back(Point(0, 0));
   F32 ang = angleOfLongestSide(pts);
   EXPECT_FLOAT_EQ(0.0f, ang);
}


// ============================================================
// removeCollinearPoints
// ============================================================

TEST(GeomUtilsTest, removeCollinearPointsMiddle)
{
   // Three collinear points, middle should be removed
   Vector<Point> pts;
   pts.push_back(Point(0, 0));
   pts.push_back(Point(5, 0));  // collinear middle
   pts.push_back(Point(10, 0));
   pts.push_back(Point(10, 5));
   removeCollinearPoints(pts, false);
   // Middle collinear point should be gone
   EXPECT_EQ(3, pts.size());
}

TEST(GeomUtilsTest, removeCollinearPointsNoneToRemove)
{
   Vector<Point> pts;
   pts.push_back(Point(0, 0));
   pts.push_back(Point(10, 0));
   pts.push_back(Point(10, 10));
   pts.push_back(Point(0, 10));
   S32 origSize = pts.size();
   removeCollinearPoints(pts, false);
   EXPECT_EQ(origSize, pts.size());
}

TEST(GeomUtilsTest, removeCollinearPointsDuplicates)
{
   Vector<Point> pts;
   pts.push_back(Point(0, 0));
   pts.push_back(Point(0, 0));  // duplicate
   pts.push_back(Point(5, 0));
   pts.push_back(Point(10, 0));
   removeCollinearPoints(pts, false);
   // The duplicate should be removed; then collinear check on remaining (5,0 is removed as well)
   EXPECT_EQ(2, pts.size());
}

TEST(GeomUtilsTest, removeCollinearPointsTrailingDuplicate)
{
   Vector<Point> pts;
   pts.push_back(Point(0, 0));
   pts.push_back(Point(5, 5));
   pts.push_back(Point(5, 5));  // trailing duplicate
   removeCollinearPoints(pts, false);
   EXPECT_EQ(2, pts.size());
}

TEST(GeomUtilsTest, removeCollinearPointsDuplicatesAtEnd)
{
   Vector<Point> pts;
   pts.push_back(Point(0, 0));
   pts.push_back(Point(1, 1));
   pts.push_back(Point(1, 1));

   removeCollinearPoints(pts, false);

   ASSERT_EQ(2, pts.size());
   EXPECT_EQ(Point(0, 0), pts[0]);
   EXPECT_EQ(Point(1, 1), pts[1]);
}

TEST(GeomUtilsTest, removeCollinearPointsDegenerate)
{
   Vector<Point> pts;
   pts.push_back(Point(0, 0));
   removeCollinearPoints(pts, true);
   ASSERT_EQ(1, pts.size());

   pts.clear();
   pts.push_back(Point(0, 0));
   pts.push_back(Point(0, 0));
   removeCollinearPoints(pts, true);
   ASSERT_EQ(1, pts.size());
}


// ============================================================
// createPolygon / calcPolygonVerts
// ============================================================

TEST(GeomUtilsTest, createPolygonSquare)
{
   // A 4-sided polygon with radius 1 at origin, angle 0
   Vector<Point> poly = createPolygon(Point(0, 0), 1.0f, 4, FloatPi / 4.0f);
   ASSERT_EQ(4, poly.size());
   // Each vertex should be at distance 1 from origin
   for(S32 i = 0; i < poly.size(); ++i)
      EXPECT_NEAR(1.0f, poly[i].len(), 0.001f);
}

TEST(GeomUtilsTest, createPolygonCentered)
{
   // Polygon centered at (5, 5)
   Vector<Point> poly = createPolygon(Point(5, 5), 3.0f, 6, 0);
   ASSERT_EQ(6, poly.size());
   for(S32 i = 0; i < poly.size(); ++i)
      EXPECT_NEAR(3.0f, poly[i].distanceTo(Point(5, 5)), 0.001f);
}

TEST(GeomUtilsTest, createPolygonTriangle)
{
   Vector<Point> poly = createPolygon(Point(0, 0), 10.0f, 3, 0);
   ASSERT_EQ(3, poly.size());
}

TEST(GeomUtilsTest, calcPolygonVerts)
{
   Vector<Point> pts;
   calcPolygonVerts(Point(0, 0), 8, 5.0f, 0, pts);
   ASSERT_EQ(8, pts.size());
   for(S32 i = 0; i < pts.size(); ++i)
      EXPECT_NEAR(5.0f, pts[i].len(), 0.001f);
}


// ============================================================
// isWoundClockwise
// ============================================================

TEST(GeomUtilsTest, isWoundClockwiseCW)
{
   Vector<Point> cw;
   cw.push_back(Point(0, 0));
   cw.push_back(Point(0, 10));
   cw.push_back(Point(10, 10));
   cw.push_back(Point(10, 0));
   EXPECT_TRUE(isWoundClockwise(cw));
}

TEST(GeomUtilsTest, isWoundClockwiseCCW)
{
   Vector<Point> ccw;
   ccw.push_back(Point(0, 0));
   ccw.push_back(Point(10, 0));
   ccw.push_back(Point(10, 10));
   ccw.push_back(Point(0, 10));
   EXPECT_FALSE(isWoundClockwise(ccw));
}

TEST(GeomUtilsTest, isWoundClockwiseEmpty)
{
   Vector<Point> empty;
   EXPECT_TRUE(isWoundClockwise(empty));
}

TEST(GeomUtilsTest, isWoundClockwiseLargeCoordinates)
{
   F32 offset = 1e7;
   Vector<Point> ccw;
   ccw.push_back(Point(offset, offset));
   ccw.push_back(Point(offset + 10, offset));
   ccw.push_back(Point(offset, offset + 10));
   EXPECT_FALSE(isWoundClockwise(ccw));

   Vector<Point> cw;
   cw.push_back(Point(offset, offset));
   cw.push_back(Point(offset, offset + 10));
   cw.push_back(Point(offset + 10, offset));
   EXPECT_TRUE(isWoundClockwise(cw));
}

TEST(GeomUtilsTest, isWoundClockwiseTriangleCW)
{
   Vector<Point> cw;
   cw.push_back(Point(0, 0));
   cw.push_back(Point(0, 10));
   cw.push_back(Point(10, 0));
   EXPECT_TRUE(isWoundClockwise(cw));
}

TEST(GeomUtilsTest, isWoundClockwiseTriangleCCW)
{
   Vector<Point> ccw;
   ccw.push_back(Point(0, 0));
   ccw.push_back(Point(10, 0));
   ccw.push_back(Point(0, 10));
   EXPECT_FALSE(isWoundClockwise(ccw));
}


// ============================================================
// triangulatedFillContains
// ============================================================

TEST(GeomUtilsTest, triangulatedFillContainsInside)
{
   // Two triangles forming a square
   Vector<Point> triangles;
   // Triangle 1: bottom-left
   triangles.push_back(Point(0, 0));
   triangles.push_back(Point(10, 0));
   triangles.push_back(Point(10, 10));
   // Triangle 2: top-right
   triangles.push_back(Point(0, 0));
   triangles.push_back(Point(10, 10));
   triangles.push_back(Point(0, 10));

   // These are all on the edge of one of the triangles, but inside the square described by the geometry
   EXPECT_TRUE(triangulatedFillContains(&triangles, Point(5, 5)));	 
   EXPECT_TRUE(triangulatedFillContains(&triangles, Point(1, 1)));	 
   EXPECT_TRUE(triangulatedFillContains(&triangles, Point(9, 9)));	 
}

TEST(GeomUtilsTest, triangulatedFillContainsOutside)
{
   Vector<Point> triangles;
   triangles.push_back(Point(0, 0));
   triangles.push_back(Point(10, 0));
   triangles.push_back(Point(10, 10));
   triangles.push_back(Point(0, 0));
   triangles.push_back(Point(10, 10));
   triangles.push_back(Point(0, 10));

   EXPECT_FALSE(triangulatedFillContains(&triangles, Point(15, 15)));
   EXPECT_FALSE(triangulatedFillContains(&triangles, Point(-1, 5)));
}


// ============================================================
// floatsToPoints
// ============================================================

TEST(GeomUtilsTest, floatsToPointsBasic)
{
   Vector<F32> floats;
   floats.push_back(0.0f);  floats.push_back(0.0f);
   floats.push_back(10.0f); floats.push_back(0.0f);
   floats.push_back(10.0f); floats.push_back(10.0f);
   floats.push_back(0.0f);  floats.push_back(10.0f);
   Vector<Point> pts = floatsToPoints(floats);
   ASSERT_EQ(4, pts.size());
   EXPECT_FLOAT_EQ(0.0f,  pts[0].x); EXPECT_FLOAT_EQ(0.0f,  pts[0].y);
   EXPECT_FLOAT_EQ(10.0f, pts[1].x); EXPECT_FLOAT_EQ(0.0f,  pts[1].y);
   EXPECT_FLOAT_EQ(10.0f, pts[2].x); EXPECT_FLOAT_EQ(10.0f, pts[2].y);
   EXPECT_FLOAT_EQ(0.0f,  pts[3].x); EXPECT_FLOAT_EQ(10.0f, pts[3].y);
}

TEST(GeomUtilsTest, floatsToPointsCollinearRemoved)
{
   // Collinear middle point should be removed
   Vector<F32> floats;
   floats.push_back(0.0f);  floats.push_back(0.0f);
   floats.push_back(5.0f);  floats.push_back(0.0f);  // collinear middle
   floats.push_back(10.0f); floats.push_back(0.0f);
   floats.push_back(10.0f); floats.push_back(10.0f);
   Vector<Point> pts = floatsToPoints(floats);
   EXPECT_EQ(3, pts.size());
}

TEST(GeomUtilsTest, floatsToPointsEmpty)
{
   Vector<F32> floats;
   Vector<Point> pts = floatsToPoints(floats);
   EXPECT_EQ(0, pts.size());
}


// ============================================================
// unpackPolygons
// ============================================================

TEST(GeomUtilsTest, unpackPolygonsSquare)
{
   Vector<Vector<Point> > solution;
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));
   solution.push_back(square);

   Vector<Point> lineSegments;
   unpackPolygons(solution, lineSegments);
   // 4 edges, 2 points each = 8 points
   EXPECT_EQ(8, lineSegments.size());
}

TEST(GeomUtilsTest, unpackPolygonsEmpty)
{
   Vector<Vector<Point> > solution;
   Vector<Point> lineSegments;
   unpackPolygons(solution, lineSegments);
   EXPECT_EQ(0, lineSegments.size());
}

TEST(GeomUtilsTest, unpackPolygonsMultiplePolygons)
{
   Vector<Vector<Point> > solution;

   Vector<Point> sq1;
   sq1.push_back(Point(0, 0)); sq1.push_back(Point(5, 0));
   sq1.push_back(Point(5, 5)); sq1.push_back(Point(0, 5));
   solution.push_back(sq1);

   Vector<Point> sq2;
   sq2.push_back(Point(10, 0)); sq2.push_back(Point(15, 0));
   sq2.push_back(Point(15, 5));
   solution.push_back(sq2);

   Vector<Point> lineSegments;
   unpackPolygons(solution, lineSegments);
   // 4 edges + 3 edges = 7 edges, 2 points each = 14
   EXPECT_EQ(14, lineSegments.size());
}


// ============================================================
// cornersToEdges
// ============================================================

TEST(GeomUtilsTest, cornersToEdgesSquare)
{
   Vector<Point> corners;
   corners.push_back(Point(0, 0));
   corners.push_back(Point(10, 0));
   corners.push_back(Point(10, 10));
   corners.push_back(Point(0, 10));

   Vector<Point> edges;
   cornersToEdges(corners, edges);
   // 4 corners -> 4 edges, 2 points each = 8
   EXPECT_EQ(8, edges.size());
}

TEST(GeomUtilsTest, cornersToEdgesTriangle)
{
   Vector<Point> corners;
   corners.push_back(Point(0, 0));
   corners.push_back(Point(5, 0));
   corners.push_back(Point(5, 5));

   Vector<Point> edges;
   cornersToEdges(corners, edges);
   EXPECT_EQ(6, edges.size());
}

TEST(GeomUtilsTest, cornersToEdgesClearsOutput)
{
   Vector<Point> corners;
   corners.push_back(Point(0, 0));
   corners.push_back(Point(5, 0));
   corners.push_back(Point(5, 5));

   Vector<Point> edges;
   edges.push_back(Point(99, 99));  // pre-existing data
   cornersToEdges(corners, edges);
   EXPECT_EQ(6, edges.size());
}


// ============================================================
// expandCenterlineToOutline
// ============================================================

TEST(GeomUtilsTest, expandCenterlineToOutlineHorizontal)
{
   Vector<Point> corners;
   expandCenterlineToOutline(Point(0, 0), Point(10, 0), 4.0f, corners);
   ASSERT_EQ(4, corners.size());
   // Width is 4, so cross-vec is ±2 in y direction
   // All x should be 0 or 10
   for(S32 i = 0; i < corners.size(); ++i)
   {
      EXPECT_NEAR(2.0f, fabs(corners[i].y), 0.001f);
      EXPECT_TRUE(corners[i].x == 0.0f || corners[i].x == 10.0f);
   }
}

TEST(GeomUtilsTest, expandCenterlineToOutlineVertical)
{
   Vector<Point> corners;
   expandCenterlineToOutline(Point(5, 0), Point(5, 10), 4.0f, corners);
   ASSERT_EQ(4, corners.size());
   for(S32 i = 0; i < corners.size(); ++i)
   {
      EXPECT_NEAR(2.0f, fabs(corners[i].x - 5.0f), 0.001f);
      EXPECT_TRUE(corners[i].y == 0.0f || corners[i].y == 10.0f);
   }
}

TEST(GeomUtilsTest, expandCenterlineToOutlineClearsCorners)
{
   Vector<Point> corners;
   corners.push_back(Point(99, 99));  // pre-existing
   expandCenterlineToOutline(Point(0, 0), Point(10, 0), 2.0f, corners);
   EXPECT_EQ(4, corners.size());
}


// ============================================================
// findClosestPoint
// ============================================================

TEST(GeomUtilsTest, findClosestPointBasic)
{
   Vector<Point> pts;
   pts.push_back(Point(0, 0));
   pts.push_back(Point(10, 0));
   pts.push_back(Point(20, 0));
   EXPECT_EQ(0, findClosestPoint(Point(-1, 0), pts));
   EXPECT_EQ(2, findClosestPoint(Point(25, 0), pts));
   EXPECT_EQ(1, findClosestPoint(Point(10, 1), pts));
}

TEST(GeomUtilsTest, findClosestPointSinglePoint)
{
   Vector<Point> pts;
   pts.push_back(Point(5, 5));
   EXPECT_EQ(0, findClosestPoint(Point(100, 100), pts));
}

TEST(GeomUtilsTest, findClosestPointEmptyReturnsNegative)
{
   Vector<Point> pts;
   EXPECT_EQ(-1, findClosestPoint(Point(0, 0), pts));
}


// ============================================================
// isConvex
// ============================================================

TEST(GeomUtilsTest, isConvexSquare)
{
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));
   EXPECT_TRUE(isConvex(&square));
}

TEST(GeomUtilsTest, isConvexTriangle)
{
   Vector<Point> tri;
   tri.push_back(Point(0, 0));
   tri.push_back(Point(10, 0));
   tri.push_back(Point(5, 10));
   EXPECT_TRUE(isConvex(&tri));
}

TEST(GeomUtilsTest, isConvexConcave)
{
   // L-shaped polygon is concave
   Vector<Point> lshape;
   lshape.push_back(Point(0, 0));
   lshape.push_back(Point(10, 0));
   lshape.push_back(Point(10, 5));
   lshape.push_back(Point(5, 5));
   lshape.push_back(Point(5, 10));
   lshape.push_back(Point(0, 10));
   EXPECT_FALSE(isConvex(&lshape));
}

TEST(GeomUtilsTest, isConvexLessThanThreePoints)
{
   Vector<Point> line;
   line.push_back(Point(0, 0));
   line.push_back(Point(10, 0));
   EXPECT_TRUE(isConvex(&line));

   Vector<Point> single;
   single.push_back(Point(5, 5));
   EXPECT_TRUE(isConvex(&single));

   Vector<Point> empty;
   EXPECT_TRUE(isConvex(&empty));
}

TEST(GeomUtilsTest, isConvexConcaveWithCollinear)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   poly.push_back(Point(0, 1));
   poly.push_back(Point(1, 1));
   poly.push_back(Point(0.5, 0.5)); // Concave point
   poly.push_back(Point(1, 0));
   poly.push_back(Point(0, -1)); // (0,-1), (0,0), (0,1) are collinear

   EXPECT_FALSE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexClockwiseWinding)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   poly.push_back(Point(0, 10));
   poly.push_back(Point(10, 0));

   EXPECT_TRUE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexWithRedundantPoints)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   poly.push_back(Point(5, 0));  // Redundant (collinear)
   poly.push_back(Point(10, 0));
   poly.push_back(Point(10, 5)); // Redundant
   poly.push_back(Point(10, 10));
   poly.push_back(Point(0, 10));
   poly.push_back(Point(0, 0));  // Duplicate point

   EXPECT_TRUE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexConcaveAtEnd)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   poly.push_back(Point(10, 0));
   poly.push_back(Point(10, 10));
   poly.push_back(Point(0, 10));
   poly.push_back(Point(2, 2));   // Concave point near the end of the list

   EXPECT_FALSE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexCCW)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   poly.push_back(Point(10, 0));
   poly.push_back(Point(10, 10));
   poly.push_back(Point(0, 10));

   EXPECT_TRUE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexRegularHexagon)
{
   Vector<Point> poly = createPolygon(Point(0, 0), 10.0f, 6, 0);
   EXPECT_TRUE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexConcaveSpike)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   poly.push_back(Point(10, 0));
   poly.push_back(Point(5, 1));  // Spike in
   poly.push_back(Point(10, 10));
   poly.push_back(Point(0, 10));

   EXPECT_FALSE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexDiamond)
{
   Vector<Point> poly;
   poly.push_back(Point(5, 0));
   poly.push_back(Point(10, 5));
   poly.push_back(Point(5, 10));
   poly.push_back(Point(0, 5));

   EXPECT_TRUE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexSelfIntersecting)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   poly.push_back(Point(10, 10));
   poly.push_back(Point(10, 0));
   poly.push_back(Point(0, 10));

   // Self-intersecting polygons are concave by definition (they have alternating turn directions)
   EXPECT_FALSE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexDegenerateLine)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   poly.push_back(Point(5, 0));
   poly.push_back(Point(10, 0));

   EXPECT_TRUE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexDegenerateTriangle)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   poly.push_back(Point(10, 0));
   poly.push_back(Point(5, 0));  // Backwards on same line

   // This is effectively a line that goes 0->10 then back to 5.
   // Turns are 180 degrees, so cross product is 0.
   EXPECT_TRUE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexCollinearStartCCW)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   poly.push_back(Point(5, 0));  // Collinear start
   poly.push_back(Point(10, 0));
   poly.push_back(Point(10, 10));
   poly.push_back(Point(0, 10));

   EXPECT_TRUE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexLargeConvex)
{
   Vector<Point> poly = createPolygon(Point(100, 100), 50.0f, 100, 0);
   EXPECT_TRUE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexLargeConcave)
{
   Vector<Point> poly = createPolygon(Point(100, 100), 50.0f, 100, 0);
   poly[50] = Point(100, 100); // Pull one vertex to center
   EXPECT_FALSE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexTiny)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   poly.push_back(Point(1, 1));
   EXPECT_TRUE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexPoint)
{
   Vector<Point> poly;
   poly.push_back(Point(0, 0));
   EXPECT_TRUE(isConvex(&poly));
}

TEST(GeomUtilsTest, isConvexLargeCoordinates)
{
   F32 offset = 1e6f;
   Vector<Point> poly;
   poly.push_back(Point(offset, offset));
   poly.push_back(Point(offset + 10, offset));
   poly.push_back(Point(offset + 10, offset + 10));
   poly.push_back(Point(offset, offset + 10));

   EXPECT_TRUE(isConvex(&poly));
}


// ============================================================
// segsOverlap (public wrapper)
// ============================================================

TEST(GeomUtilsTest, segsOverlapCollinearOverlapping)
{
   Point overlapStart, overlapEnd;
   // p1-p2 along x-axis [0,10], p3-p4 along x-axis [5,15]: should overlap [5,10]
   EXPECT_TRUE(segsOverlap(Point(0, 0), Point(10, 0),
                            Point(5, 0), Point(15, 0),
                            overlapStart, overlapEnd));
}

TEST(GeomUtilsTest, segsOverlapNotCollinear)
{
   Point overlapStart, overlapEnd;
   // Perpendicular segments
   EXPECT_FALSE(segsOverlap(Point(0, 0), Point(10, 0),
                             Point(5, -5), Point(5, 5),
                             overlapStart, overlapEnd));
}

TEST(GeomUtilsTest, segsOverlapParallelNotOnSameLine)
{
   Point overlapStart, overlapEnd;
   // Parallel but offset
   EXPECT_FALSE(segsOverlap(Point(0, 0), Point(10, 0),
                             Point(0, 1), Point(10, 1),
                             overlapStart, overlapEnd));
}


// ============================================================
// segmentsColinear (public wrapper)
// ============================================================

TEST(GeomUtilsTest, segmentsColinearOnSameLine)
{
   EXPECT_TRUE(segmentsColinear(Point(0, 0), Point(10, 0),
                                 Point(5, 0), Point(15, 0), 1.0f));
}

TEST(GeomUtilsTest, segmentsColinearParallelOffset)
{
   EXPECT_FALSE(segmentsColinear(Point(0, 0), Point(10, 0),
                                  Point(0, 1), Point(10, 1), 1.0f));
}

TEST(GeomUtilsTest, segmentsColinearDiagonal)
{
   EXPECT_TRUE(segmentsColinear(Point(0, 0), Point(5, 5),
                                 Point(5, 5), Point(10, 10), 1.0f));
}

TEST(GeomUtilsTest, segmentsColinearPerpendicular)
{
   EXPECT_FALSE(segmentsColinear(Point(0, 0), Point(10, 0),
                                  Point(5, 5), Point(5, -5), 1.0f));
}

TEST(GeomUtilsTest, areaRightTriangle)
{
   Vector<Point> tri;
   tri.push_back(Point(0, 0));
   tri.push_back(Point(10, 0));
   tri.push_back(Point(0, 10));
   // Area = 0.5 * 10 * 10 = 50
   EXPECT_NEAR(50.0f, fabs(area(tri)), 0.001f);
}

TEST(GeomUtilsTest, areaTinyPolygonLargeCoords)
{
   F32 offset = 1e7f;
   Vector<Point> square;
   square.push_back(Point(offset, offset));
   square.push_back(Point(offset + 1, offset));
   square.push_back(Point(offset + 1, offset + 1));
   square.push_back(Point(offset, offset + 1));

   // Area = 1 * 1 = 1
   // With float, 1e7 * 1e7 = 1e14, float has ~7 digits, 1e14 + 1 is lost.
   EXPECT_NEAR(1.0f, fabs(area(square)), 0.001f);
}

TEST(GeomUtilsTest, findCentroidLShape)
{
   // L-shape from (0,0) to (10,10) with width 5
   Vector<Point> lshape;
   lshape.push_back(Point(0, 0));
   lshape.push_back(Point(10, 0));
   lshape.push_back(Point(10, 5));
   lshape.push_back(Point(5, 5));
   lshape.push_back(Point(5, 10));
   lshape.push_back(Point(0, 10));

   // Area = (10*5) + (5*5) = 75
   // Centroid of (10x5) rect at (5, 2.5), weighted by area 50
   // Centroid of (5x5) rect at (2.5, 7.5), weighted by area 25
   // x = (50*5 + 25*2.5) / 75 = (250 + 62.5) / 75 = 312.5 / 75 = 4.1666
   // y = (50*2.5 + 25*7.5) / 75 = (125 + 187.5) / 75 = 312.5 / 75 = 4.1666
   Point c = findCentroid(lshape);
   EXPECT_NEAR(4.1666f, c.x, 0.01f);
   EXPECT_NEAR(4.1666f, c.y, 0.01f);
}

TEST(GeomUtilsTest, findCentroidDegenerateLine)
{
   Vector<Point> line;
   line.push_back(Point(0, 0));
   line.push_back(Point(10, 0));
   line.push_back(Point(20, 0));

   // Area is 0, should fall back to mean2d
   Point c = findCentroid(line);
   EXPECT_NEAR(10.0f, c.x, 0.01f);
   EXPECT_NEAR(0.0f, c.y, 0.01f);
}

TEST(GeomUtilsTest, isConvexVeryThinTriangle)
{
   Vector<Point> tri;
   tri.push_back(Point(0, 0));
   tri.push_back(Point(1000, 0));
   tri.push_back(Point(500, 0.01)); // Very thin

   EXPECT_TRUE(isConvex(&tri));
}

TEST(GeomUtilsTest, isConvexConcaveC)
{
   // C-shape
   Vector<Point> cshape;
   cshape.push_back(Point(0, 0));
   cshape.push_back(Point(10, 0));
   cshape.push_back(Point(10, 10));
   cshape.push_back(Point(0, 10));
   cshape.push_back(Point(0, 8));
   cshape.push_back(Point(8, 8));
   cshape.push_back(Point(8, 2));
   cshape.push_back(Point(0, 2));

   EXPECT_FALSE(isConvex(&cshape));
}

TEST(GeomUtilsTest, polygonCircleIntersectDegenerateEdge)
{
   // Single vertex polygon - forms a degenerate edge with itself (v1=v2)
   Point vertex(100, 100);
   Point center(100.5f, 100.5f);
   F32 radiusSq = 1.0f; // radius 1, dist is sqrt(0.5) < 1
   Point outPoint;

   // This should return true and not crash/divide by zero
   EXPECT_TRUE(polygonCircleIntersect(&vertex, 1, center, radiusSq, outPoint));
   EXPECT_EQ(vertex, outPoint);
}

TEST(GeomUtilsTest, TriangulateInsideTriangleWinding)
{
   // CCW Triangle
   // (0,0), (10,0), (5,10)
   EXPECT_TRUE(Triangulate::InsideTriangle(0, 0, 10, 0, 5, 10, 5, 5));
   EXPECT_FALSE(Triangulate::InsideTriangle(0, 0, 10, 0, 5, 10, 15, 5));

   // CW Triangle
   // (0,0), (5,10), (10,0)
   EXPECT_TRUE(Triangulate::InsideTriangle(0, 0, 5, 10, 10, 0, 5, 5));
   EXPECT_FALSE(Triangulate::InsideTriangle(0, 0, 5, 10, 10, 0, 15, 5));
}

TEST(GeomUtilsTest, TriangulateInsideTriangleLargeCoords)
{
   // Large world coordinates where float precision might fail
   // float has about 7 decimal digits of precision.
   // 1e7 is 10,000,000, which is near the limit of exact integer representation for float (2^24 ~ 1.6e7)
   F32 offset = 1e7;

   // Triangle at large offset: (offset, offset), (offset+10, offset), (offset+5, offset+10)
   float Ax = offset, Ay = offset;
   float Bx = offset + 10.0f, By = offset;
   float Cx = offset + 5.0f, Cy = offset + 10.0f;

   // Point inside: (offset+5, offset+5)
   EXPECT_TRUE(Triangulate::InsideTriangle(Ax, Ay, Bx, By, Cx, Cy, offset + 5.0f, offset + 5.0f));

   // Point outside: (offset+5, offset-5)
   EXPECT_FALSE(Triangulate::InsideTriangle(Ax, Ay, Bx, By, Cx, Cy, offset + 5.0f, offset - 5.0f));

   // A point very close to the edge of a large triangle
   // (1e7, 1e7), (1e7 + 10, 1e7), (1e7, 1e7 + 10)
   // Inside point: (1e7 + 1, 1e7 + 1)
   // Outside point: (1e7 + 11, 1e7 + 1)
   EXPECT_TRUE(Triangulate::InsideTriangle(offset, offset, offset + 10.0f, offset, offset, offset + 10.0f, offset + 1.0f, offset + 1.0f));
   EXPECT_FALSE(Triangulate::InsideTriangle(offset, offset, offset + 10.0f, offset, offset, offset + 10.0f, offset + 11.0f, offset + 1.0f));

   // Point exactly on the edge
   EXPECT_TRUE(Triangulate::InsideTriangle(offset, offset, offset + 10.0f, offset, offset, offset + 10.0f, offset + 5.0f, offset));
}

}; // namespace Zap
