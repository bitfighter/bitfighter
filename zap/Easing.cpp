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

#include "Easing.h"

#include "tnlAssert.h"
#include "tnlLog.h"

#include <math.h>


#ifndef M_PI
#define M_PI FloatPi
#define M_PI_2 FloatHalfPi
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4244) // Suppress warning about possible loss of data when using floats like 2.5
#endif


namespace Easing {

// Modeled after the line y = x
AHFloat LinearInterpolation(AHFloat p)
{
	return p;
}

// Modeled after the parabola y = x^2
AHFloat QuadraticEaseIn(AHFloat p)
{
	return p * p;
}

// Modeled after the parabola y = -x^2 + 2x
AHFloat QuadraticEaseOut(AHFloat p)
{
	return -(p * (p - 2));
}

// Modeled after the piecewise quadratic
// y = (1/2)((2x)^2)             ; [0, 0.5)
// y = -(1/2)((2x-1)*(2x-3) - 1) ; [0.5, 1]
AHFloat QuadraticEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		return 2 * p * p;
	}
	else
	{
		return (-2 * p * p) + (4 * p) - 1;
	}
}

// Modeled after the cubic y = x^3
AHFloat CubicEaseIn(AHFloat p)
{
	return p * p * p;
}

// Modeled after the cubic y = (x - 1)^3 + 1
AHFloat CubicEaseOut(AHFloat p)
{
	AHFloat f = (p - 1);
	return f * f * f + 1;
}

// Modeled after the piecewise cubic
// y = (1/2)((2x)^3)       ; [0, 0.5)
// y = (1/2)((2x-2)^3 + 2) ; [0.5, 1]
AHFloat CubicEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		return 4 * p * p * p;
	}
	else
	{
		AHFloat f = ((2 * p) - 2);
		return 0.5 * f * f * f + 1;
	}
}

// Modeled after the quartic x^4
AHFloat QuarticEaseIn(AHFloat p)
{
	return p * p * p * p;
}

// Modeled after the quartic y = 1 - (x - 1)^4
AHFloat QuarticEaseOut(AHFloat p)
{
	AHFloat f = (p - 1);
	return f * f * f * (1 - p) + 1;
}

// Modeled after the piecewise quartic
// y = (1/2)((2x)^4)        ; [0, 0.5)
// y = -(1/2)((2x-2)^4 - 2) ; [0.5, 1]
AHFloat QuarticEaseInOut(AHFloat p) 
{
	if(p < 0.5)
	{
		return 8 * p * p * p * p;
	}
	else
	{
		AHFloat f = (p - 1);
		return -8 * f * f * f * f + 1;
	}
}

// Modeled after the quintic y = x^5
AHFloat QuinticEaseIn(AHFloat p) 
{
	return p * p * p * p * p;
}

// Modeled after the quintic y = (x - 1)^5 + 1
AHFloat QuinticEaseOut(AHFloat p) 
{
	AHFloat f = (p - 1);
	return f * f * f * f * f + 1;
}

// Modeled after the piecewise quintic
// y = (1/2)((2x)^5)       ; [0, 0.5)
// y = (1/2)((2x-2)^5 + 2) ; [0.5, 1]
AHFloat QuinticEaseInOut(AHFloat p) 
{
	if(p < 0.5)
	{
		return 16 * p * p * p * p * p;
	}
	else
	{
		AHFloat f = ((2 * p) - 2);
		return  0.5 * f * f * f * f * f + 1;
	}
}

// Modeled after quarter-cycle of sine wave
AHFloat SineEaseIn(AHFloat p)
{
	return sin((p - 1) * M_PI_2) + 1;
}

// Modeled after quarter-cycle of sine wave (different phase)
AHFloat SineEaseOut(AHFloat p)
{
	return sin(p * M_PI_2);
}

// Modeled after half sine wave
AHFloat SineEaseInOut(AHFloat p)
{
	return 0.5 * (1 - cos(p * M_PI));
}

// Modeled after shifted quadrant IV of unit circle
AHFloat CircularEaseIn(AHFloat p)
{
	return 1 - sqrt(1 - (p * p));
}

// Modeled after shifted quadrant II of unit circle
AHFloat CircularEaseOut(AHFloat p)
{
	return sqrt((2 - p) * p);
}

// Modeled after the piecewise circular function
// y = (1/2)(1 - sqrt(1 - 4x^2))           ; [0, 0.5)
// y = (1/2)(sqrt(-(2x - 3)*(2x - 1)) + 1) ; [0.5, 1]
AHFloat CircularEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		return 0.5 * (1 - sqrt(1 - 4 * (p * p)));
	}
	else
	{
		return 0.5 * (sqrt(-((2 * p) - 3) * ((2 * p) - 1)) + 1);
	}
}

// Modeled after the exponential function y = 2^(10(x - 1))
AHFloat ExponentialEaseIn(AHFloat p)
{
	return (p == 0.0) ? p : pow(2, 10 * (p - 1));
}

// Modeled after the exponential function y = -2^(-10x) + 1
AHFloat ExponentialEaseOut(AHFloat p)
{
	return (p == 1.0) ? p : 1 - pow(2, -10 * p);
}

// Modeled after the piecewise exponential
// y = (1/2)2^(10(2x - 1))         ; [0,0.5)
// y = -(1/2)*2^(-10(2x - 1))) + 1 ; [0.5,1]
AHFloat ExponentialEaseInOut(AHFloat p)
{
	if(p == 0.0 || p == 1.0) return p;
	
	if(p < 0.5)
	{
		return 0.5 * pow(2, (20 * p) - 10);
	}
	else
	{
		return -0.5 * pow(2, (-20 * p) + 10) + 1;
	}
}

// Modeled after the damped sine wave y = sin(13pi/2*x)*pow(2, 10 * (x - 1))
AHFloat ElasticEaseIn(AHFloat p)
{
	return sin(13 * M_PI_2 * p) * pow(2, 10 * (p - 1));
}

// Modeled after the damped sine wave y = sin(-13pi/2*(x + 1))*pow(2, -10x) + 1
AHFloat ElasticEaseOut(AHFloat p)
{
	return sin(-13 * M_PI_2 * (p + 1)) * pow(2, -10 * p) + 1;
}

// Modeled after the piecewise exponentially-damped sine wave:
// y = (1/2)*sin(13pi/2*(2*x))*pow(2, 10 * ((2*x) - 1))      ; [0,0.5)
// y = (1/2)*(sin(-13pi/2*((2x-1)+1))*pow(2,-10(2*x-1)) + 2) ; [0.5, 1]
AHFloat ElasticEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		return 0.5 * sin(13 * M_PI_2 * (2 * p)) * pow(2, 10 * ((2 * p) - 1));
	}
	else
	{
		return 0.5 * (sin(-13 * M_PI_2 * ((2 * p - 1) + 1)) * pow(2, -10 * (2 * p - 1)) + 2);
	}
}

// Modeled after the overshooting cubic y = x^3-x*sin(x*pi)
AHFloat BackEaseIn(AHFloat p)
{
	return p * p * p - p * sin(p * M_PI);
}

// Modeled after overshooting cubic y = 1-((1-x)^3-(1-x)*sin((1-x)*pi))
AHFloat BackEaseOut(AHFloat p)
{
	AHFloat f = (1 - p);
	return 1 - (f * f * f - f * sin(f * M_PI));
}

// Modeled after the piecewise overshooting cubic function:
// y = (1/2)*((2x)^3-(2x)*sin(2*x*pi))           ; [0, 0.5)
// y = (1/2)*(1-((1-x)^3-(1-x)*sin((1-x)*pi))+1) ; [0.5, 1]
AHFloat BackEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		AHFloat f = 2 * p;
		return 0.5 * (f * f * f - f * sin(f * M_PI));
	}
	else
	{
		AHFloat f = (1 - (2*p - 1));
		return 0.5 * (1 - (f * f * f - f * sin(f * M_PI))) + 0.5;
	}
}

AHFloat BounceEaseIn(AHFloat p)
{
	return 1 - BounceEaseOut(1 - p);
}

AHFloat BounceEaseOut(AHFloat p)
{
	if(p < 4/11.0)
	{
		return (121 * p * p)/16.0;
	}
	else if(p < 8/11.0)
	{
		return (363/40.0 * p * p) - (99/10.0 * p) + 17/5.0;
	}
	else if(p < 9/10.0)
	{
		return (4356/361.0 * p * p) - (35442/1805.0 * p) + 16061/1805.0;
	}
	else
	{
		return (54/5.0 * p * p) - (513/25.0 * p) + 268/25.0;
	}
}

AHFloat BounceEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		return 0.5 * BounceEaseIn(p*2);
	}
	else
	{
		return 0.5 * BounceEaseOut(p * 2 - 1) + 0.5;
	}
}


// A helper function mainly for testing stuff, making it possible to cycle through easings to find just the right one
AHEasingFunction getNextEasing(AHEasingFunction currentEasing)
{
   if(currentEasing == LinearInterpolation)
   {
      logprintf("Using easing QuadraticEaseIn");
      return QuadraticEaseIn;
   }
   if(currentEasing == QuadraticEaseIn)
   {
      logprintf("Using easing QuadraticEaseOut");
      return QuadraticEaseOut;
   }
   if(currentEasing == QuadraticEaseOut)
   {
      logprintf("Using easing QuadraticEaseInO");
      return QuadraticEaseInOut;
   }
   if(currentEasing == QuadraticEaseInOut)
   {
      logprintf("Using easing CubicEaseIn");
      return CubicEaseIn;
   }
   if(currentEasing == CubicEaseIn)
   {
      logprintf("Using easing CubicEaseOut");
      return CubicEaseOut;
   }
   if(currentEasing == CubicEaseOut)
   {
      logprintf("Using easing CubicEaseInOut");
      return CubicEaseInOut;
   }
   if(currentEasing == CubicEaseInOut)
   {
      logprintf("Using easing QuarticEaseIn");
      return QuarticEaseIn;
   }
   if(currentEasing == QuarticEaseIn)
   {
      logprintf("Using easing QuarticEaseOut");
      return QuarticEaseOut;
   }
   if(currentEasing == QuarticEaseOut)
   {
      logprintf("Using easing QuarticEaseInOut");
      return QuarticEaseInOut;
   }
   if(currentEasing == QuarticEaseInOut)
   {
      logprintf("Using easing QuinticEaseIn");
      return QuinticEaseIn;
   }
   if(currentEasing == QuinticEaseIn)
   {
      logprintf("Using easing QuinticEaseOut");
      return QuinticEaseOut;
   }
   if(currentEasing == QuinticEaseOut)
   {
      logprintf("Using easing QuinticEaseInOut");
      return QuinticEaseInOut;
   }
   if(currentEasing == QuinticEaseInOut)
   {
      logprintf("Using easing SineEaseIn");
      return SineEaseIn;
   }
   if(currentEasing == SineEaseIn)
   {
      logprintf("Using easing SineEaseOut");
      return SineEaseOut;
   }
   if(currentEasing == SineEaseOut)
   {
      logprintf("Using easing SineEaseInOut");
      return SineEaseInOut;
   }
   if(currentEasing == SineEaseInOut)
   {
      logprintf("Using easing CircularEaseIn");
      return CircularEaseIn;
   }
   if(currentEasing == CircularEaseIn)
   {
      logprintf("Using easing CircularEaseOut");
      return CircularEaseOut;
   }
   if(currentEasing == CircularEaseOut)
   {
      logprintf("Using easing CircularEaseInOu");
      return CircularEaseInOut;
   }
   if(currentEasing == CircularEaseInOut)
   {
      logprintf("Using easing ExponentialEaseI");
      return ExponentialEaseIn;
   }
   if(currentEasing == ExponentialEaseIn)
   {
      logprintf("Using easing ExponentialEaseO");
      return ExponentialEaseOut;
   }
   if(currentEasing == ExponentialEaseOut)
   {
      logprintf("Using easing ExponentialEaseI");
      return ExponentialEaseInOut;
   }
   if(currentEasing == ExponentialEaseInOut)
   {
      logprintf("Using easing ElasticEaseIn");
      return ElasticEaseIn;
   }
   if(currentEasing == ElasticEaseIn)
   {
      logprintf("Using easing ElasticEaseOut");
      return ElasticEaseOut;
   }
   if(currentEasing == ElasticEaseOut)
   {
      logprintf("Using easing ElasticEaseInOut");
      return ElasticEaseInOut;
   }
   if(currentEasing == ElasticEaseInOut)
   {
      logprintf("Using easing BackEaseIn");
      return BackEaseIn;
   }
   if(currentEasing == BackEaseIn)
   {
      logprintf("Using easing BackEaseOut");
      return BackEaseOut;
   }
   if(currentEasing == BackEaseOut)
   {
      logprintf("Using easing BackEaseInOut");
      return BackEaseInOut;
   }
   if(currentEasing == BackEaseInOut)
   {
      logprintf("Using easing BounceEaseIn");
      return BounceEaseIn;
   }
   if(currentEasing == BounceEaseIn)
   {
      logprintf("Using easing BounceEaseOut");
      return BounceEaseOut;
   }
   if(currentEasing == BounceEaseOut)
   {
      logprintf("Using easing BounceEaseInOut");
      return BounceEaseInOut;
   }
   if(currentEasing == BounceEaseInOut)
   {
      logprintf("Using easing LinearInterpolat");
      return LinearInterpolation;   }


   TNLAssert(false, "Should never get here");
   return NULL;
}


}
