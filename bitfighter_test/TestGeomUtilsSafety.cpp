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
  
   Vector<Point> empty;
   EXPECT_FALSE(polygonContainsPoint(empty.address(), empty.size(), Point(0, 0)));

   // Test with fewer than 3 vertices (not a valid polygon)
   Vector<Point> line;
   line.push_back(Point(0, 0));
   line.push_back(Point(10, 0));
   EXPECT_FALSE(polygonContainsPoint(line.address(), line.size(), Point(5, 0)));
}

TEST(GeomUtilsSafetyTest, polygonCircleIntersectEmpty)
{
   Point vertices[1];
   Point outPoint;
   EXPECT_FALSE(polygonCircleIntersect(vertices, 0, Point(0, 0), 10.0f, outPoint));
  
   Vector<Point> empty;
   Point outPoint;
   EXPECT_FALSE(polygonCircleIntersect(empty.address(), empty.size(), Point(0, 0), 10.0f, outPoint));
}

TEST(GeomUtilsSafetyTest, polygonIntersectsSegmentEmpty)
{ 
   Vector<Point> empty;
   EXPECT_FALSE(polygonIntersectsSegment(empty, Point(0, 0), Point(10, 10)));
}

TEST(GeomUtilsSafetyTest, polygonsIntersectEmpty)
{
   Vector<Point> empty;
   Vector<Point> square;
   square.push_back(Point(0, 0));
   square.push_back(Point(10, 0));
   square.push_back(Point(10, 10));
   square.push_back(Point(0, 10));

   EXPECT_FALSE(polygonsIntersect(empty, square));
   EXPECT_FALSE(polygonsIntersect(square, empty));
   EXPECT_FALSE(polygonsIntersect(empty, empty));
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
   Vector<Point> empty;
   F32 collisionTime;
   Point normal;
   EXPECT_FALSE(polygonIntersectsSegmentDetailed(empty.address(), empty.size(), true, Point(0, 0), Point(10, 10), collisionTime, normal));
}

TEST(GeomUtilsSafetyTest, areaEmpty)
{
   Vector<Point> empty;
   EXPECT_FLOAT_EQ(0.0f, area(empty));
}

TEST(GeomUtilsSafetyTest, cornersToEdgesEmpty)
{
   Vector<Point> corners, edges;
   cornersToEdges(corners, edges);
   EXPECT_TRUE(edges.empty());
}

TEST(GeomUtilsSafetyTest, barrierLineToSegmentDataEmpty)
{
   Vector<Point> empty;
   Vector<Vector<Point> > outData;
   // This should not crash
   barrierLineToSegmentData(empty, outData);
   EXPECT_TRUE(outData.empty());
}

} // namespace Zap
