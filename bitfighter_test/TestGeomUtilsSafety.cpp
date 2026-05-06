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

TEST(GeomUtilsSafetyTest, polygonContainsPointEmpty)
{
   Point vertices[1]; // Not used if count is 0
   EXPECT_FALSE(polygonContainsPoint(vertices, 0, Point(0, 0)));
}

TEST(GeomUtilsSafetyTest, polygonCircleIntersectEmpty)
{
   Point vertices[1];
   Point outPoint;
   EXPECT_FALSE(polygonCircleIntersect(vertices, 0, Point(0, 0), 10.0f, outPoint));
}

TEST(GeomUtilsSafetyTest, polygonIntersectsSegmentEmpty)
{
   Vector<Point> points;
   EXPECT_FALSE(polygonIntersectsSegment(points, Point(0, 0), Point(10, 10)));
}

TEST(GeomUtilsSafetyTest, polygonsIntersectEmpty)
{
   Vector<Point> p1, p2;
   EXPECT_FALSE(polygonsIntersect(p1, p2));

   p1.push_back(Point(0,0));
   p1.push_back(Point(10,0));
   p1.push_back(Point(0,10));
   EXPECT_FALSE(polygonsIntersect(p1, p2));
   EXPECT_FALSE(polygonsIntersect(p2, p1));
}

TEST(GeomUtilsSafetyTest, polygonIntersectsSegmentDetailedEmpty)
{
   Point poly[1];
   F32 ct;
   Point normal;
   EXPECT_FALSE(polygonIntersectsSegmentDetailed(poly, 0, true, Point(0, 0), Point(10, 10), ct, normal));
   EXPECT_FALSE(polygonIntersectsSegmentDetailed(poly, 0, false, Point(0, 0), Point(10, 10), ct, normal));
}

TEST(GeomUtilsSafetyTest, PolygonSweptCircleIntersectEmpty)
{
   Point vertices[1];
   Point outPoint;
   F32 outFraction;
   EXPECT_FALSE(PolygonSweptCircleIntersect(vertices, 0, Point(0, 0), Point(10, 0), 5.0f, outPoint, outFraction));
}

TEST(GeomUtilsSafetyTest, areaEmpty)
{
   Vector<Point> contour;
   EXPECT_FLOAT_EQ(0.0f, area(contour));
}

TEST(GeomUtilsSafetyTest, cornersToEdgesEmpty)
{
   Vector<Point> corners, edges;
   cornersToEdges(corners, edges);
   EXPECT_TRUE(edges.empty());
}

TEST(GeomUtilsSafetyTest, barrierLineToSegmentDataEmpty)
{
   Vector<Point> inputLine;
   Vector<Vector<Point> > outData;
   barrierLineToSegmentData(inputLine, outData);
   EXPECT_TRUE(outData.empty());
}

} // namespace Zap
