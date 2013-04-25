//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
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

// Note: This file has become a sort of catchall for various mathematical functions gleaned from
// various sources.


#include "MathUtils.h"
#include "Point.h"
#include "tnlVector.h"
#include "tnlAssert.h"


namespace Zap {

// Figure out the shortest path from a to b...   Returns between -FloatPi and FloatPi
F32 getAngleDiff(F32 a, F32 b)
{
   a = fmod(b - a + FloatPi, Float2Pi);         // fmod may return negative
   return a < 0 ? a + FloatPi : a - FloatPi;
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
   // Check if a solution exists
   F32 determinant = inB * inB - 4.0f * inA * inC;
   if (determinant < 0.0f)
      return false;

   // The standard way of doing this is by computing: x = (-b +/- Sqrt(b^2 - 4 a c)) / 2 a
   // is not numerically stable when a is close to zero.
   // Solve the equation according to "Numerical Recipies in C" paragraph 5.6
   F32 q = -0.5f * (inB + (inB < 0.0f? -1.0f : 1.0f) * sqrt(determinant));

   // Both of these can return +INF, -INF or NAN that's why we test both solutions to be in the specified range below
   F32 x1 = q / inA;
   F32 x2 = inC / q;

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

};
