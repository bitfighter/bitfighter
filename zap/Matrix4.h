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
   Matrix4(bool);

public:
   Matrix4();
   Matrix4(const F32* matrix);
   Matrix4(const F64 *matrix);
   ~Matrix4();

   F32 *getData();
   const F32 *getData() const;

   Matrix4 operator*(const Matrix4 &rhs);
   Matrix4 scale(F32 x, F32 y, F32 z);
   Matrix4 translate(F32 x, F32 y, F32 z);
   Matrix4 rotate(F32 radAngle, F32 x, F32 y, F32 z);
   static Matrix4 getOrthoProjection(F32 left, F32 right, F32 bottom, F32 top, F32 nearZ, F32 farZ);
};

}

#endif // _MATRIX4_H_ 
