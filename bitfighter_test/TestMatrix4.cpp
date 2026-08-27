//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Matrix4.h"
#include "gtest/gtest.h"
#include <cmath>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace Zap {

TEST(Matrix4Test, Identity)
{
   Matrix4 identity;
   const F32 *data = identity.getData();

   for(int c = 0; c < 4; ++c)
   {
      for(int r = 0; r < 4; ++r)
      {
         if(c == r)
            EXPECT_FLOAT_EQ(1.0f, data[c * 4 + r]);
         else
            EXPECT_FLOAT_EQ(0.0f, data[c * 4 + r]);
      }
   }
}

TEST(Matrix4Test, ConstructorF32)
{
   F32 initial[16] = {
      0, 1, 2, 3,
      4, 5, 6, 7,
      8, 9, 10, 11,
      12, 13, 14, 15
   };

   Matrix4 m(initial);
   const F32 *data = m.getData();

   for(int i = 0; i < 16; ++i)
      EXPECT_FLOAT_EQ(initial[i], data[i]);
}

TEST(Matrix4Test, Multiply)
{
   // A = [ 1 2 ]  B = [ 5 6 ]  AB = [ 1*5+2*7 1*6+2*8 ] = [ 19 22 ]
   //     [ 3 4 ]      [ 7 8 ]       [ 3*5+4*7 3*6+4*8 ]   [ 43 50 ]

   // Our Matrix4 is 4x4 and column-major. mData[col][row].
   // data[col * 4 + row]

   F32 dataA[16] = {0};
   dataA[0*4 + 0] = 1; dataA[1*4 + 0] = 2;
   dataA[0*4 + 1] = 3; dataA[1*4 + 1] = 4;
   dataA[2*4 + 2] = 1; dataA[3*4 + 3] = 1;

   F32 dataB[16] = {0};
   dataB[0*4 + 0] = 5; dataB[1*4 + 0] = 6;
   dataB[0*4 + 1] = 7; dataB[1*4 + 1] = 8;
   dataB[2*4 + 2] = 1; dataB[3*4 + 3] = 1;

   Matrix4 mA(dataA);
   Matrix4 mB(dataB);
   Matrix4 mAB = mA * mB;

   const F32 *data = mAB.getData();
   EXPECT_FLOAT_EQ(19.0f, data[0*4 + 0]);
   EXPECT_FLOAT_EQ(22.0f, data[1*4 + 0]);
   EXPECT_FLOAT_EQ(43.0f, data[0*4 + 1]);
   EXPECT_FLOAT_EQ(50.0f, data[1*4 + 1]);
}

TEST(Matrix4Test, Translate)
{
   Matrix4 m;
   m = m.translate(10.0f, 20.0f, 30.0f);

   // For column-major 4x4 matrix, translation is in the 4th column (index 3).
   // mData[3][0], mData[3][1], mData[3][2]
   const F32 *data = m.getData();
   EXPECT_FLOAT_EQ(10.0f, data[3*4 + 0]);
   EXPECT_FLOAT_EQ(20.0f, data[3*4 + 1]);
   EXPECT_FLOAT_EQ(30.0f, data[3*4 + 2]);
}

TEST(Matrix4Test, Scale)
{
   Matrix4 m;
   m = m.scale(2.0f, 3.0f, 4.0f);

   const F32 *data = m.getData();
   EXPECT_FLOAT_EQ(2.0f, data[0*4 + 0]);
   EXPECT_FLOAT_EQ(3.0f, data[1*4 + 1]);
   EXPECT_FLOAT_EQ(4.0f, data[2*4 + 2]);
   EXPECT_FLOAT_EQ(1.0f, data[3*4 + 3]);
}

TEST(Matrix4Test, RotateX)
{
   Matrix4 m;
   F32 angle = M_PI / 2.0f; // 90 degrees
   m = m.rotate(angle, 1.0f, 0.0f, 0.0f);

   const F32 *data = m.getData();
   // Rotation about X:
   // [ 1  0       0     0 ]
   // [ 0 cos(a) -sin(a) 0 ]
   // [ 0 sin(a)  cos(a) 0 ]
   // [ 0  0       0     1 ]

   EXPECT_NEAR(1.0f, data[0*4 + 0], 1e-6f);
   EXPECT_NEAR(0.0f, data[1*4 + 1], 1e-6f); // cos(pi/2) = 0
   EXPECT_NEAR(1.0f, data[1*4 + 2], 1e-6f); // sin(pi/2) = 1
   EXPECT_NEAR(-1.0f, data[2*4 + 1], 1e-6f); // -sin(pi/2) = -1
   EXPECT_NEAR(0.0f, data[2*4 + 2], 1e-6f); // cos(pi/2) = 0
}

TEST(Matrix4Test, Ortho)
{
   Matrix4 m = Matrix4::getOrthoProjection(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
   const F32 *data = m.getData();

   // Ortho with these params should be identity (mostly, Z is negated in many ortho implementations)
   // Wikipedia:
   // [ 2/(r-l)      0          0     -(r+l)/(r-l) ]
   // [    0      2/(t-b)       0     -(t+b)/(t-b) ]
   // [    0         0      -2/(f-n)  -(f+n)/(f-n) ]
   // [    0         0          0           1      ]

   EXPECT_FLOAT_EQ(1.0f, data[0*4 + 0]);
   EXPECT_FLOAT_EQ(1.0f, data[1*4 + 1]);
   EXPECT_FLOAT_EQ(-1.0f, data[2*4 + 2]);
   EXPECT_FLOAT_EQ(1.0f, data[3*4 + 3]);
}

TEST(Matrix4Test, TransformationOrder)
{
   // Bitfighter's Matrix4 applies transformations like glTranslate/glRotate:
   // newMat = currentMat * transformationMat

   Matrix4 m;
   m = m.translate(10.0f, 0.0f, 0.0f);
   m = m.scale(2.0f, 1.0f, 1.0f);

   const F32 *data = m.getData();
   EXPECT_FLOAT_EQ(2.0f, data[0*4 + 0]);
   EXPECT_FLOAT_EQ(10.0f, data[3*4 + 0]);
}

} // namespace Zap
