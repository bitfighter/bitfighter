//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Tip: C multi-dimensional arrays are column-major. However, OpenGL and D3D use row-major.
// This class stores its data as row-major for interoperability, which means we must access
// elements as such: mData[col][row]

#include "Matrix.h"
#include <math.h>

namespace Zap
{

// Create square identity matrix
template<U32 size>
Matrix<size>::Matrix()
{
   makeIdentity();
}

// 'matrix' must be square and column-major
template<U32 size>
Matrix<size>::Matrix(const F32 *matrix)
{
   for(U32 c = 0; c < size; ++c)
      for(U32 r = 0; r < size; ++r)
         mData[c][r] = matrix[c * size + r];
}

// Loss of precision!
template<U32 size>
Matrix<size>::Matrix(const F64 *matrix)
{
   for(U32 c = 0; c < size; ++c)
      for(U32 r = 0; r < size; ++r)
         mData[c][r] = static_cast<F32>(matrix[c * size + r]);
}


template<U32 size>
Matrix<size>::~Matrix()
{
   // Do nothing
}

// Static
// Peform the dot product of 2 vectors: rowMatrix[][row] and colMatrix[col][]
template<U32 size>
F32 Matrix<size>::dotProduct(U32 row, const Matrix<size> &rowMatrix, U32 col, const Matrix<size> &colMatrix)
{
   F32 result = 0;
   for(U32 i = 0; i < size; ++i)
   {
      result += rowMatrix.mData[i][row] * colMatrix.mData[col][i];
   }

   return result;
}

template<U32 size>
void Matrix<size>::makeIdentity()
{
   for(U32 c = 0; c < size; ++c)
   {
      for(U32 r = 0; r < size; ++r)
      {
         if(c == r)
            mData[c][r] = 1;
         else
            mData[c][r] = 0;
      }
   }
}

template<U32 size>
F32 *Matrix<size>::getData()
{
   return &mData[0][0];
}

// Static
// Create an orthographic projection matrix.
// Source: https://en.wikipedia.org/wiki/Orthographic_projection
template<>
Matrix<4> Matrix<4>::orthoProjection(F32 left, F32 right, F32 bottom, F32 top, F32 nearZ, F32 farZ)
{
   // Essentially, we create a transformation matrix which will map the cube
   // defined by the arguments into a 2x2x2 cube centered at the origin.
   // When OpenGL will convert to screen coordinates, it will simply omit the z coordinate.
   Matrix<4> newMat;
   
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

// Matrix multiplication, both matrices must have the same size.
template<U32 size>
Matrix<size> Matrix<size>::operator*(const Matrix<size> &rhs)
{
   Matrix<size> newMat;
   for(U32 c = 0; c < size; ++c)
   {
      for(U32 r = 0; r < size; ++r)
      {
         newMat.mData[c][r] = dotProduct(r, *this, c, rhs);
      }
   }

   return newMat;
}

template<U32 size>
Matrix<size> Matrix<size>::scale(F32 x, F32 y, F32 z)
{
   Matrix<size> newMat(*this);
   if(size < 3)
      return newMat; // Do nothing

   newMat.mData[0][0] *= x;
   newMat.mData[1][1] *= y;
   newMat.mData[2][2] *= z;

   return newMat;
}

template<U32 size>
Matrix<size> Matrix<size>::translate(F32 x, F32 y, F32 z)
{
   if(size < 3)
      return *this; // Do nothing

   Matrix<size> translateMat;
   U32 c = size - 1;
   translateMat.mData[3][0] = x;
   translateMat.mData[3][1] = y;
   translateMat.mData[3][2] = z;

   // Apply translation BEFORE all current transformations.
   // This is the behavior of glTranslate.
   return (*this) * translateMat;
}

// Source: https://math.stackexchange.com/a/4155115
template<U32 size>
Matrix<size> Matrix<size>::rotate(F32 radAngle, F32 x, F32 y, F32 z)
{
   Matrix<size> rotMat;
   if(size < 3)
      return *this; // Do nothing

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

// Explicit template instantiation for the values we will use:
template class Matrix<4>;

}
