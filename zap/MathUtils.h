//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------


#ifndef _MATH_UTILS_
#define _MATH_UTILS_

#include "tnlTypes.h"

#include <cmath>


using namespace TNL;


// Some helpful functions -- these are nominally defined in tommath.h, so only redefine if they are not somehow included here
#ifndef MIN
#  define MIN(x,y) (((x) < (y)) ? (x) : (y))
#  define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define sq(a) ((a) * (a))


// Some angle conversion helpers:
#define RADIANS_TO_DEGREES  (360.0f * FloatInverse2Pi)
#define DEGREES_TO_RADIANS  (FloatInverse360 * Float2Pi)


namespace Zap {

inline F32 radiansToDegrees(F32 angle) { return angle * RADIANS_TO_DEGREES; }
inline F32 degreesToRadians(F32 angle) { return angle * DEGREES_TO_RADIANS; }
inline F32 radiansToUnit(F32 angle)    { return angle * FloatInverse2Pi; }       // angle / 2pi
inline F32 unitToRadians(F32 angle)    { return angle * Float2Pi; }              // angle * 2pi

extern F32 getAngleDiff(F32 a, F32 b);

bool findLowestRootInInterval(F32 inA, F32 inB, F32 inC, F32 inUpperBound, F32 &outX);
S32 roundUp(S32 numToRound, S32 multiple);

};


#endif
