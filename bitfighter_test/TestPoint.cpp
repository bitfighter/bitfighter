//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Point.h"
#include "gtest/gtest.h"

namespace Zap {

TEST(PointTest, Constructors)
{
   Point p1;
   EXPECT_FLOAT_EQ(0.0f, p1.x);
   EXPECT_FLOAT_EQ(0.0f, p1.y);

   Point p2(1.5f, 2.5f);
   EXPECT_FLOAT_EQ(1.5f, p2.x);
   EXPECT_FLOAT_EQ(2.5f, p2.y);

   Point p3(p2);
   EXPECT_FLOAT_EQ(1.5f, p3.x);
   EXPECT_FLOAT_EQ(2.5f, p3.y);

   Point p4(10, 20); // template constructor
   EXPECT_FLOAT_EQ(10.0f, p4.x);
   EXPECT_FLOAT_EQ(20.0f, p4.y);
}

TEST(PointTest, SetMethods)
{
   Point p;
   p.set(1.0f, 2.0f);
   EXPECT_FLOAT_EQ(1.0f, p.x);
   EXPECT_FLOAT_EQ(2.0f, p.y);

   Point p2(3.0f, 4.0f);
   p.set(p2);
   EXPECT_FLOAT_EQ(3.0f, p.x);
   EXPECT_FLOAT_EQ(4.0f, p.y);

   Point p3(5.0f, 6.0f);
   p.set(&p3);
   EXPECT_FLOAT_EQ(5.0f, p.x);
   EXPECT_FLOAT_EQ(6.0f, p.y);
}

TEST(PointTest, ArithmeticOperators)
{
   const Point p1(1.0f, 2.0f);
   const Point p2(3.0f, 4.0f);

   Point p3 = p1 + p2;
   EXPECT_FLOAT_EQ(4.0f, p3.x);
   EXPECT_FLOAT_EQ(6.0f, p3.y);

   Point p4 = p1 - p2;
   EXPECT_FLOAT_EQ(-2.0f, p4.x);
   EXPECT_FLOAT_EQ(-2.0f, p4.y);

   Point p5 = -p1;
   EXPECT_FLOAT_EQ(-1.0f, p5.x);
   EXPECT_FLOAT_EQ(-2.0f, p5.y);

   Point p6 = p1 * 2.0f;
   EXPECT_FLOAT_EQ(2.0f, p6.x);
   EXPECT_FLOAT_EQ(4.0f, p6.y);

   Point p7 = p2 / 2.0f;
   EXPECT_FLOAT_EQ(1.5f, p7.x);
   EXPECT_FLOAT_EQ(2.0f, p7.y);

   Point p8 = p1 * p2;
   EXPECT_FLOAT_EQ(3.0f, p8.x);
   EXPECT_FLOAT_EQ(8.0f, p8.y);

   Point p9 = p2 / Point(2.0f, 4.0f);
   EXPECT_FLOAT_EQ(1.5f, p9.x);
   EXPECT_FLOAT_EQ(1.0f, p9.y);
}

TEST(PointTest, AssignmentArithmeticOperators)
{
   Point p1(1.0f, 2.0f);
   p1 += Point(3.0f, 4.0f);
   EXPECT_FLOAT_EQ(4.0f, p1.x);
   EXPECT_FLOAT_EQ(6.0f, p1.y);

   p1 -= Point(1.0f, 1.0f);
   EXPECT_FLOAT_EQ(3.0f, p1.x);
   EXPECT_FLOAT_EQ(5.0f, p1.y);

   p1 *= 2.0f;
   EXPECT_FLOAT_EQ(6.0f, p1.x);
   EXPECT_FLOAT_EQ(10.0f, p1.y);

   p1 /= 2.0f;
   EXPECT_FLOAT_EQ(3.0f, p1.x);
   EXPECT_FLOAT_EQ(5.0f, p1.y);
}

TEST(PointTest, ComparisonOperators)
{
   Point p1(3.0f, 4.0f); // len = 5
   Point p2(3.0f, 4.0f);
   Point p3(1.0f, 1.0f); // len = sqrt(2)
   Point p4(10.0f, 10.0f); // len = sqrt(200)

   EXPECT_TRUE(p1 == p2);
   EXPECT_FALSE(p1 == p3);
   EXPECT_TRUE(p1 != p3);
   EXPECT_FALSE(p1 != p2);

   EXPECT_TRUE(p3 < p1);
   EXPECT_TRUE(p1 < p4);
   EXPECT_FALSE(p1 < p3);

   EXPECT_TRUE(p4 > p1);
   EXPECT_TRUE(p1 > p3);

   EXPECT_TRUE(p1 <= p2);
   EXPECT_TRUE(p3 <= p1);
   EXPECT_TRUE(p1 >= p2);
   EXPECT_TRUE(p4 >= p1);
}

TEST(PointTest, LengthMethods)
{
   Point p(3.0f, 4.0f);
   EXPECT_FLOAT_EQ(5.0f, p.len());
   EXPECT_FLOAT_EQ(25.0f, p.lenSquared());

   Point p2(0.0f, 0.0f);
   EXPECT_FLOAT_EQ(0.0f, p2.len());
}

TEST(PointTest, Normalization)
{
   Point p(3.0f, 4.0f);
   p.normalize();
   EXPECT_FLOAT_EQ(1.0f, p.len());
   EXPECT_FLOAT_EQ(0.6f, p.x);
   EXPECT_FLOAT_EQ(0.8f, p.y);

   Point p2(3.0f, 4.0f);
   p2.normalize(10.0f);
   EXPECT_FLOAT_EQ(10.0f, p2.len());
   EXPECT_FLOAT_EQ(6.0f, p2.x);
   EXPECT_FLOAT_EQ(8.0f, p2.y);

   Point zero(0.0f, 0.0f);
   zero.normalize();
   EXPECT_FLOAT_EQ(1.0f, zero.x);
   EXPECT_FLOAT_EQ(0.0f, zero.y);
}

TEST(PointTest, Interpolation)
{
   Point p1(0.0f, 0.0f);
   Point p2(10.0f, 10.0f);
   Point res;

   res.interp(1.0f, p1, p2);
   EXPECT_FLOAT_EQ(10.0f, res.x);
   EXPECT_FLOAT_EQ(10.0f, res.y);

   res.interp(0.0f, p1, p2);
   EXPECT_FLOAT_EQ(0.0f, res.x);
   EXPECT_FLOAT_EQ(0.0f, res.y);

   res.interp(0.5f, p1, p2);
   EXPECT_FLOAT_EQ(5.0f, res.x);
   EXPECT_FLOAT_EQ(5.0f, res.y);
}

TEST(PointTest, Distance)
{
   Point p1(1.0f, 1.0f);
   Point p2(4.0f, 5.0f);
   EXPECT_FLOAT_EQ(5.0f, p1.distanceTo(p2));
   EXPECT_FLOAT_EQ(25.0f, p1.distSquared(p2));
}

TEST(PointTest, Rotation)
{
   Point p(1.0f, 0.0f);
   Point p2 = p.rotate(FloatPi / 2.0f); // 90 degrees
   EXPECT_NEAR(0.0f, p2.x, 1e-6f);
   EXPECT_NEAR(1.0f, p2.y, 1e-6f);

   Point p3 = p.rotate(FloatPi); // 180 degrees
   EXPECT_NEAR(-1.0f, p3.x, 1e-6f);
   EXPECT_NEAR(0.0f, p3.y, 1e-6f);
}

TEST(PointTest, PolarCoordinates)
{
   Point p;
   p.setPolar(10.0f, FloatPi / 4.0f); // 45 degrees, len 10
   EXPECT_NEAR(7.0710678f, p.x, 1e-6f);
   EXPECT_NEAR(7.0710678f, p.y, 1e-6f);

   p.setAngle(FloatPi);
   EXPECT_NEAR(-10.0f, p.x, 1e-6f);
   EXPECT_NEAR(0.0f, p.y, 1e-6f);

   Point p2(0.0f, 1.0f);
   EXPECT_FLOAT_EQ(FloatPi / 2.0f, p2.ATAN2());
}

TEST(PointTest, DotAndDeterminant)
{
   Point p1(1.0f, 2.0f);
   Point p2(3.0f, 4.0f);
   EXPECT_FLOAT_EQ(11.0f, p1.dot(p2));
   EXPECT_DOUBLE_EQ(-2.0, p1.determinant(p2)); // 1*4 - 2*3 = 4 - 6 = -2
}

TEST(PointTest, DeterminantPrecision)
{
   // Use large values to test for precision issues
   // (1e7 * 1e7) - ( (1e7+1) * (1e7-1) ) = 1e14 - (1e14 - 1) = 1
   Point p1(10000000.0f, 10000001.0f);
   Point p2(9999999.0f,  10000000.0f);

   // F32 precision would likely result in 0 or large error
   EXPECT_DOUBLE_EQ(1.0, (F64)p1.determinant(p2));
}

TEST(PointTest, StringConversion)
{
   Point p(1.23f, 4.56f);
   // ftos by default seems to include many decimals or depends on its implementation
   // In Point.cpp, toString uses ftos(x) + ", " + ftos(y)
   EXPECT_EQ("1.23, 4.56", p.toString());
   EXPECT_EQ("1.23 4.56", p.toLevelCode());
}

} // namespace Zap
