//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LUA_INC_H_
#define _LUA_INC_H_

// Turn on Lua asserts and the C API checking for help with debugging
#ifdef TNL_DEBUG
#  define LUA_USE_ASSERT
#  define LUA_USE_APICHECK
#endif

// Need to include lua headers this way --> wrap them in a simpler looking .h file...

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#endif   // _LUA_INC_H_
