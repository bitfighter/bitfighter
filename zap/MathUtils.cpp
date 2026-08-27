//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Note: This file has become a sort of catchall for various mathematical functions gleaned from
// various sources.


#include "MathUtils.h"


namespace Zap {

// Figure out the shortest path from a to b...   Returns between -FloatPi and FloatPi
F32 getAngleDiff(F32 a, F32 b)
{
   a = fmod(b - a, Float2Pi);
   if (a <= -FloatPi) a += Float2Pi;
   if (a > FloatPi) a -= Float2Pi;
   return a;
}


static void swap(F32 &f1, F32 &f2)
{
   F32 temp = f1;
   f1 = f2;
   f2 = temp;
}


// Solve the equation inA * x^2 + inB * x + inC == 0 for the lowest x in [0, inUpperBound].
// Returns true if there is such a solution and returns the solution in outX
bool findLowestRootInInterval(F32 inA, F32 inB, F32 inC, F32 inUpperBound, F32 &outX)
{
   // Handle linear case
   if (inA == 0.0f)
   {
      if (inB == 0.0f)
         return false;

      F32 x = -inC / inB;
      if (x >= 0.0f && x <= inUpperBound)
      {
         outX = x;
         return true;
      }
      return false;
   }

   // Quadratic case
   // Check if a solution exists
   F32 determinant = inB * inB - 4.0f * inA * inC;
   if (determinant < 0.0f)
      return false;

   // The standard way of doing this is by computing: x = (-b +/- Sqrt(b^2 - 4 a c)) / 2 a
   // is not numerically stable when a is close to zero.
   // Solve the equation according to "Numerical Recipes in C" paragraph 5.6
   F32 q = -0.5f * (inB + (inB < 0.0f? -1.0f : 1.0f) * sqrt(determinant));

   // Both of these can return +INF, -INF or NAN that's why we test both solutions to be in the specified range below
   F32 x1 = q / inA;
   F32 x2 = (q != 0.0f) ? inC / q : 1e30f;    // Avoid division by zero; x1 will be the correct root (0) if q is zero and inA is not

   // Order the results
   if (x2 < x1)
      swap(x1, x2);

   // Check if x1 is a solution
   if (x1 >= 0.0f && x1 <= inUpperBound)
   {
      outX = x1;
      return true;
   }

   // Check if x2 is a solution
   if (x2 >= 0.0f && x2 <= inUpperBound)
   {
      outX = x2;
      return true;
   }

   return false;
}


// Round numToRound up to the nearest multiple of multiple
S32 roundUp(S32 numToRound, S32 multiple)
{
   S64 multiple64 = std::abs((S64)multiple);

   if(multiple64 == 0)
      return numToRound;

   S64 numToRound64 = numToRound;
   S64 remainder = numToRound64 % multiple64;

   if(remainder == 0)
      return numToRound;

   if(numToRound64 < 0)
      return (S32)(numToRound64 - remainder);

   return (S32)(numToRound64 + multiple64 - remainder);
}

};
