//  Copyright (c) 2011, Auerhaus Development, LLC
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What The Fuck You Want
//  To Public License, Version 2, as published by Sam Hocevar. See
//  http://sam.zoy.org/wtfpl/COPYING for more details.
//
//
// Original source: https://github.com/warrenm/AHEasing
//
// See http://easings.net/ for visualizations
//

#ifndef AH_EASING_H
#define AH_EASING_H

#include "tnlTypes.h"

using namespace TNL;

//#if defined(__LP64__) && !defined(AH_EASING_USE_DBL_PRECIS)
//#define AH_EASING_USE_DBL_PRECIS
//#endif

//#ifdef AH_EASING_USE_DBL_PRECIS
//#define AHFloat F64
//#else
#define AHFloat F32
//#endif

//typedef AHFloat (*AHEasingFunction)(AHFloat);

enum EasingType
{
   LINEAR_INTERPOLATION,
   QUADRATIC_EASE_IN,
   QUADRATIC_EASE_OUT,
   QUADRATIC_EASE_IN_OUT,
   CUBIC_EASE_IN,
   CUBIC_EASE_OUT,
   CUBIC_EASE_IN_OUT,
   QUARTIC_EASE_IN,
   QUARTIC_EASE_OUT,
   QUARTIC_EASE_IN_OUT,
   QUINTIC_EASE_IN,
   QUINTIC_EASE_OUT,
   QUINTIC_EASE_IN_OUT,
   SINE_EASE_IN,
   SINE_EASE_OUT,
   SINE_EASE_IN_OUT,
   CIRCULAR_EASE_IN,
   CIRCULAR_EASE_OUT,
   CIRCULAR_EASE_IN_OUT,
   EXPONENTIAL_EASE_IN,
   EXPONENTIAL_EASE_OUT,
   EXPONENTIAL_EASE_IN_OUT,
   ELASTIC_EASE_IN,
   ELASTIC_EASE_OUT,
   ELASTIC_EASE_IN_OUT,
   BACK_EASE_IN,
   BACK_EASE_OUT,
   BACK_EASE_IN_OUT,
   BOUNCE_EASE_IN,
   BOUNCE_EASE_OUT,
   BOUNCE_EASE_IN_OUT,
   EASING_TYPE_COUNT
};


AHFloat getEasedValue(EasingType easingType, F32 value);


namespace Easing {

// Linear interpolation (no easing)
AHFloat LinearInterpolation(AHFloat p);

// Quadratic easing; p^2
AHFloat QuadraticEaseIn(AHFloat p);
AHFloat QuadraticEaseOut(AHFloat p);
AHFloat QuadraticEaseInOut(AHFloat p);

// Cubic easing; p^3
AHFloat CubicEaseIn(AHFloat p);
AHFloat CubicEaseOut(AHFloat p);
AHFloat CubicEaseInOut(AHFloat p);

// Quartic easing; p^4
AHFloat QuarticEaseIn(AHFloat p);
AHFloat QuarticEaseOut(AHFloat p);
AHFloat QuarticEaseInOut(AHFloat p);

// Quintic easing; p^5
AHFloat QuinticEaseIn(AHFloat p);
AHFloat QuinticEaseOut(AHFloat p);
AHFloat QuinticEaseInOut(AHFloat p);

// Sine wave easing; sin(p * PI/2)
AHFloat SineEaseIn(AHFloat p);
AHFloat SineEaseOut(AHFloat p);
AHFloat SineEaseInOut(AHFloat p);

// Circular easing; sqrt(1 - p^2)
AHFloat CircularEaseIn(AHFloat p);
AHFloat CircularEaseOut(AHFloat p);
AHFloat CircularEaseInOut(AHFloat p);

// Exponential easing, base 2
AHFloat ExponentialEaseIn(AHFloat p);
AHFloat ExponentialEaseOut(AHFloat p);
AHFloat ExponentialEaseInOut(AHFloat p);

// Exponentially-damped sine wave easing
AHFloat ElasticEaseIn(AHFloat p);
AHFloat ElasticEaseOut(AHFloat p);
AHFloat ElasticEaseInOut(AHFloat p);

// Overshooting cubic easing; 
AHFloat BackEaseIn(AHFloat p);
AHFloat BackEaseOut(AHFloat p);
AHFloat BackEaseInOut(AHFloat p);

// Exponentially-decaying bounce easing
AHFloat BounceEaseIn(AHFloat p);
AHFloat BounceEaseOut(AHFloat p);
AHFloat BounceEaseInOut(AHFloat p);

}

#endif