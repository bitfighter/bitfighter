#include "../zap/GeomUtils.h"
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
	for(S32 i = 0; i < result.size(); i++)
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
	for(S32 i = 0; i < 512; i++)
	{
		Vector<Point> poly = createPolygon(Point(i * 120, i * 120), 100, 4, 0);
		polys.push_back(poly);
	}

	Vector<Vector<Point> > result;
	EXPECT_TRUE(triangulate(polys, result));
	EXPECT_EQ(1024, result.size());
}


};