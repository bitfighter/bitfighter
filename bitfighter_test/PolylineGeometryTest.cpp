#include "Geometry.h"

#include "gtest/gtest.h"

using namespace Zap;
class PolylineGeometryTest: public testing::Test
{

};

TEST_F(PolylineGeometryTest, setGeomTest)
{
   Vector<Point> geom(0);
   geom.push_back(Point(0, 0));
   geom.push_back(Point(1, 0));
   geom.push_back(Point(1, 1));

   PolylineGeometry pg;
   pg.setGeom(geom);
   const Vector<Point> &newGeom = *pg.getOutline();

   // Any points with NaN coordinates should be ignored
   EXPECT_EQ(newGeom.size(), 3);
}

TEST_F(PolylineGeometryTest, setGeomWithNanPointTest)
{
   Vector<Point> geom(0);
   geom.push_back(Point(0, 0));
   geom.push_back(Point(1, 0));

   // 0.0 / 0.0 results in NaN rather than a division by zero error
   geom.push_back(Point(0.0 / 0.0, 0.0 / 0.0));

   PolylineGeometry pg;
   pg.setGeom(geom);

   const Vector<Point> &newGeom = *pg.getOutline();

   // Any points with NaN coordinates should be ignored
   EXPECT_EQ(newGeom.size(), 2);
}
