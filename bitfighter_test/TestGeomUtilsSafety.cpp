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

TEST(GeomUtilsSafetyTest, polygonIntersectsSegmentEmpty)
{
    Vector<Point> points;
    Point start(0, 0);
    Point end(10, 10);
    EXPECT_FALSE(polygonIntersectsSegment(points, start, end));
}

TEST(GeomUtilsSafetyTest, polygonsIntersectEmpty)
{
    Vector<Point> p1;
    Vector<Point> p2;
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
    F32 collisionTime;
    Point normal;
    EXPECT_FALSE(polygonIntersectsSegmentDetailed(poly, 0, true, Point(0,0), Point(10,10), collisionTime, normal));
}

TEST(GeomUtilsSafetyTest, barrierLineToSegmentDataEmpty)
{
    Vector<Point> inputLine;
    Vector<Vector<Point> > outData;
    barrierLineToSegmentData(inputLine, outData);
    EXPECT_TRUE(outData.empty());
}

TEST(GeomUtilsSafetyTest, cornersToEdgesEmpty)
{
    Vector<Point> corners;
    Vector<Point> edges;
    cornersToEdges(corners, edges);
    EXPECT_TRUE(edges.empty());
}

} // namespace Zap
