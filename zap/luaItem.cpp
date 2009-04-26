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

#include "luaObject.h"
#include "luaItem.h"
#include "gameType.h"

namespace Zap
{

const char LuaAsteroid::className[] = "Asteroid";      // Class name as it appears to Lua scripts

// Constructor
LuaAsteroid::LuaAsteroid(lua_State *L)
{

}


// Destructor
LuaAsteroid::~LuaAsteroid()
{
  logprintf("deleted Lua LuaAsteroid (%p)\n", this);
}


// Define the methods we will expose to Lua

Lunar<LuaAsteroid>::RegType LuaAsteroid::methods[] = {
   method(LuaAsteroid, getClassID),


   {0,0}    // End method list
};


S32 LuaAsteroid::getClassID(lua_State *L)
{
   return returnInt(L, AsteroidType);    
}




};




// Template for new item:

/*

const char LuaAsteroid::className[] = "Asteroid";      // Class name as it appears to Lua scripts

// Constructor
LuaAsteroid::LuaAsteroid(lua_State *L)
{

}


// Destructor
LuaAsteroid::~LuaAsteroid()
{
  logprintf("deleted Lua LuaAsteroid (%p)\n", this);
}


// Define the methods we will expose to Lua

Lunar<LuaAsteroid>::RegType LuaAsteroid::methods[] = {
   method(LuaAsteroid, getClassID),


   {0,0}    // End method list
};


S32 LuaAsteroid::getClassID(lua_State *L)
{
   return returnInt(L, AsteroidType);    
}

*/