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
   for(S32 i = 0; i < newGeom.size(); i++)
   {
      EXPECT_EQ(geom[i], newGeom[i]);
   }

   Vector<Point> secondGeom(0);
   secondGeom.push_back(Point(0, 0));
   secondGeom.push_back(Point(2, 0));
   secondGeom.push_back(Point(2, 2));
   pg.setGeom(secondGeom);

   // Any points with NaN coordinates should be ignored
   EXPECT_EQ(newGeom.size(), 3);
   for(S32 i = 0; i < newGeom.size(); i++)
   {
      EXPECT_EQ(secondGeom[i], newGeom[i]);
   }
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
   for(S32 i =0; i < newGeom.size(); i++)
   {
      // comparing against itself is false with NaN coordinates
      EXPECT_EQ(newGeom[i], newGeom[i]);
   }
}
