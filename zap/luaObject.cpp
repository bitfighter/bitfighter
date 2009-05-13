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

#include "luaObject.h"
#include "tnlLog.h"         // For logprintf
#include <string>

using namespace std;

namespace Zap
{

// Returns a point to calling Lua function
S32 LuaObject::returnPoint(lua_State *L, Point point)
{
   lua_createtable(L, 0, 2);         // creates a table with 2 fields
   setfield(L, "x", point.x);        // table.x = x
   setfield(L, "y", point.y);        // table.y = y

   return 1;
}

// Returns an int to a calling Lua function
S32 LuaObject::returnInt(lua_State *L, S32 num)
{
   lua_pushinteger(L, num);
   return 1;
}


// Returns a float to a calling Lua function
S32 LuaObject::returnFloat(lua_State *L, F32 num)
{
   lua_pushnumber(L, num);
   return 1;
}


// Returns a boolean to a calling Lua function
S32 LuaObject::returnBool(lua_State *L, bool boolean)
{
   lua_pushboolean(L, boolean);
   return 1;
}


// Returns a string to a calling Lua function
S32 LuaObject::returnString(lua_State *L, const char *str)
{
   lua_pushstring(L, str);
   return 1;
}


// Returns nil to calling Lua function
S32 LuaObject::returnNil(lua_State *L)
{
   lua_pushnil(L);
   return 1;
}


// Overwrite Lua's panicky panic function with something that doesn't kill the whole game
// if something goes wrong!
int LuaObject::luaPanicked(lua_State *L)
{
   string msg = lua_tostring(L, 1);
   lua_getglobal(L, "ERROR");    // <-- what is this for?

   throw(msg);

   return 0;
}


void LuaObject::clearStack(lua_State *L)
{
   lua_pop(L, lua_gettop(L));
}


// Assume that table is at the top of the stack
void LuaObject::setfield (lua_State *L, const char *key, F32 value)
{
   lua_pushnumber(L, value);
   lua_setfield(L, -2, key);
}


// Make sure we got the number of args we wanted
void LuaObject::checkArgCount(lua_State *L, S32 argsWanted, const char *functionName)
{
   S32 args = lua_gettop(L);

   if(args != argsWanted)     // Problem!
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s called with %d args, expected %d", functionName, args, argsWanted);
      logprintf(msg);
      throw(string(msg));
   }
}


// Pop integer off stack, check its type, do bounds checking, and return it
lua_Integer LuaObject::getInt(lua_State *L, S32 index, const char *functionName, S32 minVal, S32 maxVal)
{
   lua_Integer val = getInt(L, index, functionName);

   if(val < minVal || val > maxVal)
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s called with out-of-bounds arg: %d (val=%d)", functionName, index, val);
      logprintf(msg);
      throw(string(msg));
   }

   return val;
}


// Pop integer off stack, check its type, and return it (no bounds check)
lua_Integer LuaObject::getInt(lua_State *L, S32 index, const char *functionName)
{
   if(!lua_isnumber(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected numeric arg at position %d", functionName, index);
      logprintf(msg);
      throw(string(msg));
   }

   return lua_tointeger(L, index);
}


// Pop a number off stack, convert to float, and return it (no bounds check)
F32 LuaObject::getFloat(lua_State *L, S32 index, const char *functionName)
{
   if(!lua_isnumber(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected numeric arg at position %d", functionName, index);
      logprintf(msg);
      throw(string(msg));
   }

   return (F32) lua_tonumber(L, index);
}


// Pop a string or string-like object off stack, check its type, and return it
const char *LuaObject::getString(lua_State *L, S32 index, const char *functionName)
{
   if(!lua_isstring(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected string arg at position %d", functionName, index);
      logprintf(msg);
      throw(string(msg));
   }

   return lua_tostring(L, index);
}

};
