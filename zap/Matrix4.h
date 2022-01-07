//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _MATRIX4_H_
#define _MATRIX4_H_

#include "tnlTypes.h"

using namespace TNL;

namespace Zap
{

class Matrix4
{
private:
   F32 mData[4][4];

   Matrix4();
   static F32 dotProduct(U32 row, const Matrix4 &rowMatrix, U32 col, const Matrix4 &colMatrix);

public:
   Matrix4(const F32* matrix);
   Matrix4(const F64 *matrix);
   ~Matrix4();
   F32 *getData();

   static Matrix4 getIdentity();
   static Matrix4 getOrthoProjection(F32 left, F32 right, F32 bottom, F32 top, F32 nearZ, F32 farZ);

   Matrix4 operator*(const Matrix4 &rhs);

   Matrix4 scale(F32 x, F32 y, F32 z);
   Matrix4 translate(F32 x, F32 y, F32 z);
   Matrix4 rotate(F32 radAngle, F32 x, F32 y, F32 z);
};

}

#endif // _MATRIX4_H_ 
