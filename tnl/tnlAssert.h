//-----------------------------------------------------------------------------------
//
//   Torque Network Library
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _TNL_ASSERT_H_
#define _TNL_ASSERT_H_

#ifndef _TNL_TYPES_H_
#include "tnlTypes.h"
#endif

namespace TNL
{

/**
   sprintf style string formating into a fixed temporary buffer.
   @param in_msg sprintf style format string
   @returns pointer to fixed buffer containing formatted string

   <b>Example:</b>
   @code
   U8 a = 5;
   S16 b = -10;
   char *output = avar("hello %s! a=%u, b=%d", "world");
   ouput = "hello world! a=5, b=-10"
   @endcode
   @warning avar uses a static fixed buffer.  Treat the buffer as volatile data
   and use it immediately.  Other functions my use avar too and clobber the buffer.
 */
const char* avar(const char *in_msg, ...);

#ifdef TNL_ENABLE_ASSERTS

/// Assert functions for validating proper program state.
class Assert
{
public:
   static void processAssert(const char*  filename, U32 lineNumber, const char*  message);
   static bool processingAssert();
};


   /**
      Assert that the statement x is true, otherwise halt.
      If the statement x is true, continue processing.
      If the statement x is false, log the file and line where the assert occured,
      the message y and displaying a dialog containing the message y. The user then
      has the option to halt or continue causing the debugger to break.
      These asserts are only present when TNL_ENABLE_ASSERTS is defined.
      This assert is very useful for verifying data as well as function entry and
      exit conditions.
    */
   #define TNLAssert(x, y) { if (!bool(x)) { TNL::Assert::processAssert( __FILE__, __LINE__,  y); TNL_DEBUGBREAK(); } }

   /**
      TNLAssertV - same as TNLAssert, except that the message y must be a
      parenthesized printf-style variable argument list.
      These asserts are only present in DEBUG builds.
    */
   #define TNLAssertV(x, y) { if (!bool(x)) { TNL::Assert::processAssert(__FILE__, __LINE__,  avar y); TNL_DEBUGBREAK(); } }

#else
   #define TNLAssert(x, y)    { }
   #define TNLAssertV(x, y)   { }
#endif

};

#endif // _TNL_ASSERT_H_

