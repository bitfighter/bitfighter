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

#include "luaUtil.h"

namespace Zap
{


const char LuaUtil::className[] = "LuaUtil";      // Class name as it appears to Lua scripts


// Lua Constructor
LuaUtil::LuaUtil(lua_State *L)
{
   // Do nothing
}


// Define the methods we will expose to Lua... basically everything we want to use in lua code
// like LuaUtil:blah needs to be defined here.
Lunar<LuaUtil>::RegType LuaUtil::methods[] =
{
   method(LuaUtil, getMachineTime),
   method(LuaUtil, logprint),

   {0,0}    // End method list
};


// Write a message to the server logfile
S32 LuaUtil::logprint(lua_State *L)
{
   static const char *methodName = "LuaUtil:logprint()";
   checkArgCount(L, 2, methodName);

   logprintf("%s: %s", getString(L, 1, methodName), getString(L, 2, methodName));
   return 0;
}



};