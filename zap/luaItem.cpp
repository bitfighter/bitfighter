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

#include "luaItem.h"
#include "gameItems.h"

namespace Zap
{

const char LuaAsteroid::className[] = "Asteroid";      // Class name as it appears to Lua scripts

// Constructor
LuaAsteroid::LuaAsteroid(Asteroid *asteroid)
{
   thisAsteroid = asteroid;    // Register our asteroid
}


// Destructor
LuaAsteroid::~LuaAsteroid()
{
  logprintf("deleted Lua LuaAsteroid (%p)\n", this);
}


// Define the methods we will expose to Lua
Lunar<LuaAsteroid>::RegType LuaAsteroid::methods[] = 
{
   method(LuaAsteroid, getClassID),
   method(LuaAsteroid, getSize),
   method(LuaAsteroid, getSizeCount),
   method(LuaAsteroid, getLocation),
   method(LuaAsteroid, getRadius),     
   method(LuaAsteroid, getVelocity),  

   {0,0}    // End method list
};



S32 LuaAsteroid::getSize(lua_State *L) { return returnInt(L, thisAsteroid->getSizeIndex()); }          // Index of current asteroid size (0 = initial size, 1 = next smaller, 2 = ...) (returns int)
S32 LuaAsteroid::getSizeCount(lua_State *L) { return returnInt(L, thisAsteroid->getSizeCount()); }    // Number of indexes of size we can have (returns int)
S32 LuaAsteroid::getLocation(lua_State *L) { return returnPoint(L, thisAsteroid->getActualPos()); }   // Center of asteroid (returns point)
S32 LuaAsteroid::getRadius(lua_State *L) { return returnFloat(L, thisAsteroid->getRadius()); }        // Radius of asteroid (returns number)
S32 LuaAsteroid::getVelocity(lua_State *L) { return returnPoint(L, thisAsteroid->getActualVel()); }   // Speed of asteroid (returns point)



const char LuaTestItem::className[] = "TestItem";      // Class name as it appears to Lua scripts

// Constructor
LuaTestItem::LuaTestItem(TestItem *testItem)
{
   thisTestItem = testItem;    // Register our testItem
}


LuaTestItem::LuaTestItem(lua_State *L) 
{
   // Do nothing, for now...  should take object from stack and create testItem object
}



// Destructor
LuaTestItem::~LuaTestItem()
{
  logprintf("deleted Lua LuaTestItem (%p)\n", this);
}


// Define the methods we will expose to Lua
Lunar<LuaTestItem>::RegType LuaTestItem::methods[] = 
{
   method(LuaTestItem, getClassID),
   method(LuaTestItem, getLocation),
   method(LuaTestItem, getRadius),     
   method(LuaTestItem, getVelocity),  

   {0,0}    // End method list
};

S32 LuaTestItem::getLocation(lua_State *L) { return returnPoint(L, thisTestItem->getActualPos()); }   // Center of testItem (returns point)
S32 LuaTestItem::getRadius(lua_State *L) { return returnFloat(L, thisTestItem->getRadius()); }        // Radius of testItem (returns number)
S32 LuaTestItem::getVelocity(lua_State *L) { return returnPoint(L, thisTestItem->getActualVel()); }   // Speed of testItem (returns point)


};

