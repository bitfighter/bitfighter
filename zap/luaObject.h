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

#ifndef _LUAOBJECT_H_
#define _LUAOBJECT_H_


#include "lua.h"
#include "../lua/include/lunar.h"
#include "tnl.h"
#include "point.h"

using namespace TNL;


// Set some enumeration helpers that we'll need to pass enumeration constants to programs
#define setEnum(name) { lua_pushinteger(L, name); lua_setglobal(L, #name); }
#define setGTEnum(name) { lua_pushinteger(L, GameType::name); lua_setglobal(L, #name); }

#define method(class, name) {#name, &class::name}


namespace Zap
{

class LuaObject
{

protected:
   static int luaPanicked(lua_State *L);
   static void clearStack(lua_State *L);
   static void checkArgCount(lua_State *L, S32 argsWanted, const char *functionName);
   static F32 getFloat(lua_State *L, S32 index, const char *functionName);
   static lua_Integer getInt(lua_State *L, S32 index, const char *functionName);
   static lua_Integer getInt(lua_State *L, S32 index, const char *functionName, S32 minVal, S32 maxVal);
   static const char *getString(lua_State *L, S32 index, const char *functionName);

   static void setfield (lua_State *L, const char *key, F32 value);

public:
   // All of these return<T> functions work in the same way.  Include at the and of a child class method.
   // Usage: return returnInt(L, int);
   static S32 returnPoint(lua_State *L, Point point);
   static S32 returnInt(lua_State *L, S32 num);
   static S32 returnFloat(lua_State *L, F32 num);
   static S32 returnString(lua_State *L, const char *str);
   static S32 returnBool(lua_State *L, bool boolean);
   static S32 returnNil(lua_State *L);

};

};

#endif
