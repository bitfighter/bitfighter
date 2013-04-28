#ifndef _LUA_H_
#define _LUA_H_

// Turn on lua_assert in our debug builds
#ifdef TNL_DEBUG
#  define lua_assert
#endif

// Need to include lua headers this way --> wrap them in a simpler looking .h file...

extern "C" {
#include "../lua/lua-vec/src/lua.h"  
#include "../lua/lua-vec/src/lualib.h"
#include "../lua/lua-vec/src/lauxlib.h"
}

#endif   // _LUA_H_
