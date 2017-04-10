//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "../zap/GeomUtils.h"
#include "../zap/MathUtils.h"
#include "gtest/gtest.h"
#include "BotNavMeshZone.h"
#include "tnlBitStream.h"
#include <map>
#include "barrier.h"
#include "Level.h"
#include "tnlNetInterface.h"
#include "ServerGame.h"
#include "TestUtils.h"
#include "gameType.h"

namespace Zap
{

using namespace std;
using namespace TNL;

#define ARRAYDEF(...) __VA_ARGS__

// Test points can be generated with this python script; copy three points from C++ to code, put answer in distanceTo call
// Note that Python uses F64 and we use F32 so there may be some resolution differences at high coordinates
// from sympy import Point, Line, Segment, Rational
// def lfp(p1, dir) :
//    return Line(Point(p1.x + dir.x, p1.y + dir.y), p1)
//
// p1, p2, p3 = Point(-15, 3.33333), Point(1.52738, 4.80359), Point(27.3345, -9.8876)
// print(lfp(p2, p3).projection(p1).evalf()) 	# - 12.6173865802032, 9.92012017389809

TEST(GeomUtilsTest, pointOnLine)
{
   EXPECT_LT(pointOnLine(Point(0,10), Point(0,0),     Point(10,10))  .distanceTo(Point(5,5)), .001);
   EXPECT_LT(pointOnLine(Point(0,10), Point(10,10),   Point(-10,-10)).distanceTo(Point(5,5)), .001);     // Same thing, reversed points
   EXPECT_LT(pointOnLine(Point(0,10), Point(-10,-10), Point(20,20))  .distanceTo(Point(5,5)), .001);     // Same thing, different line points
   EXPECT_LT(pointOnLine(Point(0,10), Point(-20,-20), Point(-20,-20)).distanceTo(Point(5,5)), .001);     // Same thing, different line points

   // These cases generated with code above; arbitrary coords
   EXPECT_LT(pointOnLine(Point(-15, 3.33333),        Point(1.52738, 4.80359), Point(27.3345, -9.8876)).distanceTo(Point(-12.6173865802032, 9.92012017389809)), .01);     
   EXPECT_LT(pointOnLine(Point(-16.39656, -3.36974), Point(3.14159, 8.7765),  Point(9.34006, 0.5753)) .distanceTo(Point(-17.0680324470228, 7.53169045982872)), .01); 
   EXPECT_LT(pointOnLine(Point(1.3887, 197.880),     Point(41.8535, 60.7973), Point(58.5479, 90.5406)).distanceTo(Point(92.4289448420256, 139.009005650654)),  .01);    

   // Horiz. and vert. lines
   EXPECT_LT(pointOnLine(Point(3, 4), Point(-10, 10), Point(0, 10)).distanceTo(Point(-10.0, 4.0)), .001);
   EXPECT_LT(pointOnLine(Point(3,4), Point(-10,3),  Point(-10,0)).distanceTo(Point(3,3)), .001); 

   // Colinear, and on endpoints
   EXPECT_LT(pointOnLine(Point(5,5), Point(-10,-10), Point(20,20)).distanceTo(Point(5,5)), .001); 
   EXPECT_LT(pointOnLine(Point(5,5), Point(5,5),     Point(20,20)).distanceTo(Point(5,5)), .001); 
   EXPECT_LT(pointOnLine(Point(5,5), Point(15,15),   Point(5,5))  .distanceTo(Point(5,5)), .001); 
}


void testZoneGeneration(const string &levelCode)
{
   ServerGame *game = newServerGame(levelCode);
   game->addObjectsToGame();    // Creates the barriers we need for building zones
   game->buildBotMeshZones(false);

   ASSERT_FALSE(game->getGameType()->mBotZoneCreationFailed);
   ASSERT_TRUE(game->getBotZoneList().size() > 0);
}


// These tests just capture some levels that have caused problems in the past.  
// The main thing is that they not crash; other than that, we won't test the output geometry.
TEST(GeomUtilsTest, zoneGeneration)
{
   Address addr;
   NetInterface net(addr);   // We never use this, but it will initialize TNL to get past an assert
 
   // Actual lines from a level file that caused zone generation to crash
   SCOPED_TRACE("Pass 1");
   testZoneGeneration(
         "LevelFormat 2\n"
         "RetrieveGameType 8 8\n"
         "LevelName \"All Santiago ZAP Rollercoasters\"\n"
         "LevelDescription \"All Rollercoasters Made by Santi, Enjoy!\"\n"
         "LevelCredits \"Santiago ZAP\"\n"
         "Team Blue 0 0 1\n"
         "Specials Engineer\n"
         "MinPlayers\n"
         "MaxPlayers\n"
         "BarrierMaker 1 3187.5 -1504.5 3187.5 -1632 3213 -1785\n"
         "BarrierMaker 1 3136.5 -1504.5 3136.5 -1657.5 3162 -1810.5\n"
         "Spawn 0 3060 -1453.5\n");
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


void TestSerializationRoundTrip(const Vector<Vector<Point> > &polys)
{
   for(S32 i = 0; i < polys.size(); i++)
   {
      BotNavMeshZone zone1(0);
      zone1.getGeometry().setGeometry(polys[i]);
      BitStream stream;
      zone1.getGeometry().getGeometry()->write(&stream);

      BotNavMeshZone zone2(0);
      string x((char *)stream.getBuffer(), stream.getBytePosition());
      zone2.getGeometry().getGeometry()->read(stream.getBuffer(), stream.getBytePosition());

      EXPECT_EQ(zone1.getGeometry().getOutline()->size(), zone2.getGeometry().getOutline()->size());
      for(S32 j = 0; j < zone1.getGeometry().getOutline()->size(); j++)
         EXPECT_EQ(zone1.getGeometry().getOutline()->get(j), 
                   zone2.getGeometry().getOutline()->get(j));
   }

   // Also test individual points
   for(S32 i = 0; i < polys.size(); i++)
   {
      for(S32 j = 0; j < polys[i].size(); j++)
      {
         BitStream stream;
         polys[i][j].write(&stream);
         Point x;
         x.read(stream.getBuffer(), stream.getBytePosition());
         EXPECT_EQ(x, polys[i][j]);
      }
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
}


TEST(GeomUtilsTest, triangulateLongPolygon)
{
	Vector<Point> poly = createPolygon(Point(), 100, 512, 0);

	Vector<Vector<Point> > polys;
	polys.push_back(poly);

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));

   TestSerializationRoundTrip(polys);
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

   TestSerializationRoundTrip(polys);
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