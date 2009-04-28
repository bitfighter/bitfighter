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

#include "gameItems.h"
#include "luaObject.h"


namespace Zap
{

class Asteroid;

class LuaAsteroid : public LuaObject
{

private:
   Asteroid *thisAsteroid;             // Pointer to an actual C++ Asteroid object

public:
   LuaAsteroid(Asteroid *asteroid);    // Constructor
   ~LuaAsteroid();                     // Destructor

   static const char className[];

   static Lunar<LuaAsteroid>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, AsteroidType); }

   S32 getSize(lua_State *L);        // Index of current asteroid size (0 = initial size, 1 = next smaller, 2 = ...) (returns int)
   S32 getSizeCount(lua_State *L);   // Number of indexes of size we can have (returns int)
   S32 getLocation(lua_State *L);    // Center of asteroid (returns point)
   S32 getRadius(lua_State *L);      // Radius of asteroid (returns number)
   S32 getVelocity(lua_State *L);    // Speed of asteroid (returns point)
};


class TestItem;

class LuaTestItem : public LuaObject
{

private:
   TestItem *thisTestItem;                // Pointer to an actual C++ TestItem object

public:
   LuaTestItem(TestItem *thisTestItem);  // Constructor
   LuaTestItem(lua_State *L);             //  Other constructor
   ~LuaTestItem();                       // Destructor



   static const char className[];

   static Lunar<LuaTestItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, TestItemType); }

   S32 getLocation(lua_State *L);    // Center of testItem (returns point)
   S32 getRadius(lua_State *L);      // Radius of testItem (returns number)
   S32 getVelocity(lua_State *L);    // Speed of testItem (returns point)
};


};

#endif
