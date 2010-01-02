#ifndef _LUA_H_
#define _LUA_H_

// Need to include lua headers this way --> wrap them in a simpler looking .h file...

extern "C" {
#include "../lua/include/lua.h"  
#include "../lua/include/lualib.h"
#include "../lua/include/lauxlib.h"
}

#endif   // _LUA_H_
