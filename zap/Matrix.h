//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _MATRIX_H_
#define _MATRIX_H_

#include "tnlTypes.h"

using namespace TNL;

namespace Zap
{

template<U32 size>
class Matrix
{
private:
   F32 mData[size][size];
   static F32 dotProduct(U32 row, const Matrix<size> &rowMatrix, U32 col, const Matrix<size> &colMatrix);
   void makeIdentity();

public:
   Matrix();
   ~Matrix();

   static Matrix<4> orthoProjection(F32 left, F32 right, F32 bottom, F32 top, F32 nearZ, F32 farZ);
   Matrix<size> operator*(const Matrix<size> &rhs);

   Matrix<size> scale(F32 x, F32 y, F32 z);
   Matrix<size> translate(F32 x, F32 y, F32 z);
   Matrix<size> rotate(F32 radAngle, F32 x, F32 y, F32 z);
};

}

#endif // _MATRIX_H_ 
