//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#ifndef _LUAITEM_H_
#define _LUAITEM_H_

#include "luaObject.h"


namespace Zap
{

class LuaAsteroid : public LuaObject
{

public:
  // Initialize the pointer
  LuaAsteroid(lua_State *L);      // Constructor
  ~LuaAsteroid();                 // Destructor

   static const char className[];

   static Lunar<LuaAsteroid>::RegType methods[];

   S32 getClassID(lua_State *L);

   // Methods we will need to use



};

};

#endif


// Template for additional items

/*
class LuaAsteroid : public LuaObject
{

public:
  // Initialize the pointer
  LuaAsteroid(lua_State *L);      // Constructor
  ~LuaAsteroid();                 // Destructor

   static const char className[];

   static Lunar<LuaAsteroid>::RegType methods[];

   S32 getClassID(lua_State *L);

   // Methods we will need to use
};

*/