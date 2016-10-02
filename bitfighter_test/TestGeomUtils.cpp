//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "../zap/GeomUtils.h"
#include "../zap/MathUtils.h"
#include "gtest/gtest.h"
#include <map>
#include "BotNavMeshZone.h"

namespace Zap
{

using namespace std;
using namespace TNL;

#define ARRAYDEF(...) __VA_ARGS__


TEST(GeomUtilsTest, pointOnLine)
{
   EXPECT_LT(pointOnLine(Point(0,10), Point(0,0),     Point(10,10))  .distanceTo(Point(5,5)), .001);
   EXPECT_LT(pointOnLine(Point(0,10), Point(10,10),   Point(-10,-10)).distanceTo(Point(5,5)), .001);     // Same thing, reversed points
   EXPECT_LT(pointOnLine(Point(0,10), Point(-10,-10), Point(20,20))  .distanceTo(Point(5,5)), .001);     // Same thing, different line points
   EXPECT_LT(pointOnLine(Point(0,10), Point(-20,-20), Point(-20,-20)).distanceTo(Point(5,5)), .001);     // Same thing, different line points

   EXPECT_LT(pointOnLine(Point(-16.39656, -3.36974), Point(1.52738, 4.80359), Point(9.34006, 0.5753)).distanceTo(Point(-8.91, 10.45)), .01);     // Same thing, different line points

   // Horiz. and vert. lines
   EXPECT_LT(pointOnLine(Point(3,4), Point(-10,10), Point(0,10)).distanceTo(Point(3,10)),  .001); 
   EXPECT_LT(pointOnLine(Point(3,4), Point(-10,3),  Point(-10,0)).distanceTo(Point(-10,3)), .001); 

   // Colinear, and on endpoints
   EXPECT_LT(pointOnLine(Point(5,5), Point(-10,-10), Point(20,20)).distanceTo(Point(5,5)), .001); 
   EXPECT_LT(pointOnLine(Point(5,5), Point(5,5),     Point(20,20)).distanceTo(Point(5,5)), .001); 
   EXPECT_LT(pointOnLine(Point(5,5), Point(15,15),   Point(5,5))  .distanceTo(Point(5,5)), .001); 
}


void parsePoly(const char* lines[], S32 size, Vector<Point> &result)
{
	map<char, Point> points;
	for(S32 i = 0; i < size; i++)
	{
		for(S32 j = 0; lines[i][j] != '\0'; j++)
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
	for(map<char, Point>::iterator it = points.begin(); it != points.end(); it++)
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


void TestWkbRoundTrip(const Vector<Vector<Point> > &polys)
{
   for(S32 i = 0; i < polys.size(); i++)
   {
      BotNavMeshZone zone1(0);
      zone1.getGeometry().setGeometry(polys[i]);
      string geom = zone1.getGeometry().getGeometry()->toWkb();

      BotNavMeshZone zone2(0);
      zone2.getGeometry().getGeometry()->readWkb((U8 *)geom.c_str());

      EXPECT_EQ(zone1.getGeometry().getOutline()->size(), zone1.getGeometry().getOutline()->size());
      for(S32 j = 0; j < zone1.getGeometry().getOutline()->size(); j++)
         EXPECT_EQ(zone1.getGeometry().getOutline()->get(j), zone2.getGeometry().getOutline()->get(j));
   }

   // Also test individual points
   for(S32 i = 0; i < polys.size(); i++)
   {
      for(S32 j = 0; j < polys[i].size(); j++)
      {
         string geom = polys[i][j].toWkb();
         Point x;
         x.fromWkb((U8 *)geom.c_str());
         EXPECT_EQ(x, polys[i][j]);
      }
   }
}


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

   TestWkbRoundTrip(polys);
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
	for(S32 i = 0; i < result.size(); i++)
	{
		EXPECT_EQ(4, result[i].size());
	}

   TestWkbRoundTrip(polys);
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

   TestWkbRoundTrip(polys);
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

   TestWkbRoundTrip(polys);
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

   TestWkbRoundTrip(polys);
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

   TestWkbRoundTrip(polys);
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

   TestWkbRoundTrip(polys);
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

   TestWkbRoundTrip(polys);
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

   TestWkbRoundTrip(polys);
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

   TestWkbRoundTrip(polys);
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

   TestWkbRoundTrip(polys);
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

   TestWkbRoundTrip(polys);
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

   TestWkbRoundTrip(polys);
}


TEST(GeomUtilsTest, triangulateLongPolygon)
{
	Vector<Point> poly = createPolygon(Point(), 100, 512, 0);

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));

   TestWkbRoundTrip(polys);
}


TEST(GeomUtilsTest, triangulateManyPolygons)
{
	Vector<Vector<Point> > polys;
	for(S32 i = 0; i < 512; i++)
	{
		Vector<Point> poly = createPolygon(Point(i * 120, i * 120), 100, 4, 0);
		polys.push_back(poly);
	}

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(1024, result.size());

   TestWkbRoundTrip(polys);
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

   // Ouside, but within bounding box
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



};