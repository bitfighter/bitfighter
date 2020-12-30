/*
 * Asteroid.h
 *
 *  Created on: Dec 30, 2020
 *      Author: raptor
 */

#ifndef ZAP_ASTEROID_H_
#define ZAP_ASTEROID_H_

#include "tnlTypes.h"

static const S32 ASTEROID_DESIGNS = 4;
static const S32 ASTEROID_POINTS = 12;
static const F32 ASTEROID_SCALING_FACTOR = 1/89.0f;  // Where on earth does this come from?

static const F32 AsteroidCoords[ASTEROID_DESIGNS][2*ASTEROID_POINTS] =
{
   // x,y pairs
  {  80, -43,   47, -84,    5, -58,   -41, -81,   -79, -21,   -79,  0,   -79, 10,   -79, 47,   -49, 78,   43,   78,    80,  40,   46,   0 },
  { -41, -83,   18, -83,   81, -42,    83, -42,     7,  -2,    81, 38,    41, 79,    10, 56,   -48, 79,   -80,  15,   -80, -43,  -17, -43 },
  {  -2, -56,   40, -79,   81, -39,    34, -19,    82,  22,    32, 83,   -21, 59,   -40, 82,   -80, 42,   -57,   2,   -79, -38,  -31, -79 },
  {  42, -82,   82, -25,   82,   5,    21,  80,   -19,  80,    -8,  5,   -48, 79,   -79, 16,   -39, -4,   -79, -21,   -19, -82,   -4, -82 },
};

#endif /* ZAP_ASTEROID_H_ */
