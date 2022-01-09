//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Tip: C multi-dimensional arrays are column-major. However, OpenGL and D3D use row-major.
// This class stores its data as row-major for interoperability, which means we must access
// elements as such: mData[col][row]

#ifndef BF_USE_LEGACY_GL

#include "Matrix4.h"
#include <math.h>

static const unsigned SIZE = 4;

namespace Zap
{

// Create an identity matrix
Matrix4::Matrix4()
{
   for(U32 c = 0; c < SIZE; ++c)
   {
      for(U32 r = 0; r < SIZE; ++r)
      {
         if(c == r)
            mData[c][r] = 1;
         else
            mData[c][r] = 0;
      }
   }
}

// 'matrix' must be square and column-major
Matrix4::Matrix4(const F32 *matrix)
{
   for(U32 c = 0; c < SIZE; ++c)
      for(U32 r = 0; r < SIZE; ++r)
         mData[c][r] = matrix[c * SIZE + r];
}

// Loss of precision!
Matrix4::Matrix4(const F64 *matrix)
{
   for(U32 c = 0; c < SIZE; ++c)
      for(U32 r = 0; r < SIZE; ++r)
         mData[c][r] = static_cast<F32>(matrix[c * SIZE + r]);
}

Matrix4::~Matrix4()
{
   // Do nothing
}

F32 *Matrix4::getData()
{
   return &mData[0][0];
}

const F32 *Matrix4::getData() const
{
   return &mData[0][0];
}

// Fast matrix multiplication
Matrix4 Matrix4::operator*(const Matrix4 &rhs)
{
   Matrix4 out;
   out.mData[0][0] = mData[0][0] * rhs.mData[0][0] + mData[1][0] * rhs.mData[0][1] + mData[2][0] * rhs.mData[0][2] + mData[3][0] * rhs.mData[0][3];
   out.mData[1][0] = mData[0][0] * rhs.mData[1][0] + mData[1][0] * rhs.mData[1][1] + mData[2][0] * rhs.mData[1][2] + mData[3][0] * rhs.mData[1][3];
   out.mData[2][0] = mData[0][0] * rhs.mData[2][0] + mData[1][0] * rhs.mData[2][1] + mData[2][0] * rhs.mData[2][2] + mData[3][0] * rhs.mData[2][3];
   out.mData[3][0] = mData[0][0] * rhs.mData[3][0] + mData[1][0] * rhs.mData[3][1] + mData[2][0] * rhs.mData[3][2] + mData[3][0] * rhs.mData[3][3];

   out.mData[0][1] = mData[0][1] * rhs.mData[0][0] + mData[1][1] * rhs.mData[0][1] + mData[2][1] * rhs.mData[0][2] + mData[3][1] * rhs.mData[0][3];
   out.mData[1][1] = mData[0][1] * rhs.mData[1][0] + mData[1][1] * rhs.mData[1][1] + mData[2][1] * rhs.mData[1][2] + mData[3][1] * rhs.mData[1][3];
   out.mData[2][1] = mData[0][1] * rhs.mData[2][0] + mData[1][1] * rhs.mData[2][1] + mData[2][1] * rhs.mData[2][2] + mData[3][1] * rhs.mData[2][3];
   out.mData[3][1] = mData[0][1] * rhs.mData[3][0] + mData[1][1] * rhs.mData[3][1] + mData[2][1] * rhs.mData[3][2] + mData[3][1] * rhs.mData[3][3];

   out.mData[0][2] = mData[0][2] * rhs.mData[0][0] + mData[1][2] * rhs.mData[0][1] + mData[2][2] * rhs.mData[0][2] + mData[3][2] * rhs.mData[0][3];
   out.mData[1][2] = mData[0][2] * rhs.mData[1][0] + mData[1][2] * rhs.mData[1][1] + mData[2][2] * rhs.mData[1][2] + mData[3][2] * rhs.mData[1][3];
   out.mData[2][2] = mData[0][2] * rhs.mData[2][0] + mData[1][2] * rhs.mData[2][1] + mData[2][2] * rhs.mData[2][2] + mData[3][2] * rhs.mData[2][3];
   out.mData[3][2] = mData[0][2] * rhs.mData[3][0] + mData[1][2] * rhs.mData[3][1] + mData[2][2] * rhs.mData[3][2] + mData[3][2] * rhs.mData[3][3];

   out.mData[0][3] = mData[0][3] * rhs.mData[0][0] + mData[1][3] * rhs.mData[0][1] + mData[2][3] * rhs.mData[0][2] + mData[3][3] * rhs.mData[0][3];
   out.mData[1][3] = mData[0][3] * rhs.mData[1][0] + mData[1][3] * rhs.mData[1][1] + mData[2][3] * rhs.mData[1][2] + mData[3][3] * rhs.mData[1][3];
   out.mData[2][3] = mData[0][3] * rhs.mData[2][0] + mData[1][3] * rhs.mData[2][1] + mData[2][3] * rhs.mData[2][2] + mData[3][3] * rhs.mData[2][3];
   out.mData[3][3] = mData[0][3] * rhs.mData[3][0] + mData[1][3] * rhs.mData[3][1] + mData[2][3] * rhs.mData[3][2] + mData[3][3] * rhs.mData[3][3];

   return out;
}

Matrix4 Matrix4::scale(F32 x, F32 y, F32 z)
{
   Matrix4 newMat(*this);
   newMat.mData[0][0] *= x;
   newMat.mData[1][1] *= y;
   newMat.mData[2][2] *= z;

   return newMat;
}

Matrix4 Matrix4::translate(F32 x, F32 y, F32 z)
{
   Matrix4 translateMat;
   translateMat.mData[3][0] = x;
   translateMat.mData[3][1] = y;
   translateMat.mData[3][2] = z;

   // Apply translation BEFORE all current transformations.
   // This is the behavior of glTranslate.
   return (*this) * translateMat;
}

// Source: https://math.stackexchange.com/a/4155115
Matrix4 Matrix4::rotate(F32 radAngle, F32 x, F32 y, F32 z)
{
   Matrix4 rotMat;

   // Normalize vector
   F32 length = static_cast<F32>(sqrt(x*x + y*y + z*z));
   F32 ax = x / length;
   F32 ay = y / length;
   F32 az = z / length;

   F32 C = static_cast<F32>(cos(radAngle));
   F32 S = static_cast<F32>(sin(radAngle));
   F32 U = 1 - C;

   // Build rotation matrix
   rotMat.mData[0][0] = U * ax * ax + C;
   rotMat.mData[0][1] = U * ax * ay + S * az;
   rotMat.mData[0][2] = U * ax * az - S * ay;

   rotMat.mData[1][0] = U * ax * ay - S * az;
   rotMat.mData[1][1] = U * ay * ay + C;
   rotMat.mData[1][2] = U * ay * az + S * ax;

   rotMat.mData[2][0] = U * ax * az + S * ay;
   rotMat.mData[2][1] = U * ay * az - S * ax;
   rotMat.mData[2][2] = U * az * az + C;

   // Apply rotation BEFORE all current transformations.
   return (*this) * rotMat;
}

// Static
// Create an orthographic projection matrix.
// Source: https://en.wikipedia.org/wiki/Orthographic_projection
Matrix4 Matrix4::getOrthoProjection(F32 left, F32 right, F32 bottom, F32 top, F32 nearZ, F32 farZ)
{
   // Essentially, we create a transformation matrix which will map the cube
   // defined by the arguments into a 2x2x2 cube centered at the origin.
   // When OpenGL will convert to screen coordinates, it will simply omit the z coordinate.
   Matrix4 newMat;

   // Translation
   newMat.mData[3][0] = -(right + left) / (right - left);
   newMat.mData[3][1] = -(top + bottom) / (top - bottom);
   newMat.mData[3][2] = -(farZ + nearZ) / (farZ - nearZ);

   // Scaling
   newMat.mData[0][0] = 2.0f / (right - left);
   newMat.mData[1][1] = 2.0f / (top - bottom);
   newMat.mData[2][2] = -2.0f / (farZ - nearZ);

   return newMat;
}

}

#endif // BF_USE_LEGACY_GL