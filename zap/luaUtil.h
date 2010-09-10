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

#ifndef _LUAUTIL_H_
#define _LUAUTIL_H_

#include "luaObject.h"
#include "tnlRandom.h"

using namespace std;

namespace Zap
{

// Some util functions for scripts and levelgen files

class LuaUtil: public LuaObject
{

public:
   void logError(const char *format, ...);

   LuaUtil(lua_State *L);      // Lua constructor

   static const char className[];

   static Lunar<LuaUtil>::RegType methods[];

   // Lua methods
   S32 logprint(lua_State *L);
   S32 getMachineTime(lua_State *L) { return returnInt(L, Platform::getRealMilliseconds()); }
   S32 getRandomNumber(lua_State *L);

};

};

#endif

