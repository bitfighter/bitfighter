//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer Vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------


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

#define sq(a) ((a) * (a))


// Some angle conversion helpers:
#define RADIANS_TO_DEGREES  (360.0f * FloatInverse2Pi)
#define DEGREES_TO_RADIANS  (FloatInverse360 * Float2Pi)


namespace Zap {

inline F32 radiansToDegrees(F32 angle) { return angle * RADIANS_TO_DEGREES; }
inline F32 degreesToRadians(F32 angle) { return angle * DEGREES_TO_RADIANS; }
inline F32 radiansToUnit(F32 angle)    { return angle * FloatInverse2Pi; }
inline F32 unitToRadians(F32 angle)    { return angle * Float2Pi; }

extern F32 getAngleDiff(F32 a, F32 b);

bool findLowestRootInInterval(F32 inA, F32 inB, F32 inC, F32 inUpperBound, F32 &outX);

};


#endif
