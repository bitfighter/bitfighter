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

#include "playerInfo.h"
#include "gameConnection.h"
#include "ClientInfo.h"
#include "robot.h"         // Used in RobotPlayerInfo below...

namespace Zap
{

// Constructor
LuaPlayerInfo::LuaPlayerInfo()
{
   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
LuaPlayerInfo::~LuaPlayerInfo()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


/**
 *  @luaclass PlayerInfo
 *  @brief    Get information about a specific player.
 *  @descr    The %PlayerInfo object contains data about each player, human or robot.
 *
 */
//                Fn name                  Param profiles            Profile count
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getName,             ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getShip,             ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getTeamIndx,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getRating,               ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getScore,                ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, isRobot,                 ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getScriptName,           ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_FUNARGS_TABLE(LuaPlayerInfo, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(LuaPlayerInfo, LUA_METHODS);

#undef LUA_METHODS


const char *LuaPlayerInfo::luaClassName = "PlayerInfo";  // Class name as it appears to Lua scripts
REGISTER_LUA_CLASS(LuaPlayerInfo);


S32 LuaPlayerInfo::lua_getName(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::lua_getShip(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::lua_getTeamIndx(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::lua_getRating(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::lua_getScore(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::lua_isRobot(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::lua_getScriptName(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


////////////////////////////////////////
////////////////////////////////////////

// C++ Constructor
PlayerInfo::PlayerInfo(ClientInfo *clientInfo)
{
   mClientInfo = clientInfo;
}


// Destuctor
PlayerInfo::~PlayerInfo()
{
   // Do nothing
}


S32 PlayerInfo::lua_getName(lua_State *L)
{
   return returnString(L, mClientInfo->getName().getString());
}


S32 PlayerInfo::lua_getScriptName(lua_State *L)
{
   return returnNil(L);
}


S32 PlayerInfo::lua_getTeamIndx(lua_State *L)
{
   // Lua indexes are 1-based
   return returnInt(L, mClientInfo->getTeamIndex() + 1);
}


S32 PlayerInfo::lua_getRating(lua_State *L)
{
   return returnFloat(L, mClientInfo->getRating());
}


S32 PlayerInfo::lua_getScore(lua_State *L)
{
   return returnInt(L, mClientInfo->getScore());
}


S32 PlayerInfo::lua_isRobot(lua_State *L)
{
   return returnBool(L, false);
}


S32 PlayerInfo::lua_getShip(lua_State *L)
{
	return returnShip(L, mClientInfo->getShip());
}


////////////////////////////////////////
////////////////////////////////////////

// C++ Constructor
RobotPlayerInfo::RobotPlayerInfo(Robot *robot)
{
   mRobot = robot;
}


RobotPlayerInfo::~RobotPlayerInfo()
{
   // Do nothing
}


S32 RobotPlayerInfo::lua_getName(lua_State *L)
{
   return returnString(L, mRobot->getClientInfo()->getName().getString());
}


S32 RobotPlayerInfo::lua_getShip(lua_State *L)
{
   return returnShip(L, mRobot);
}


S32 RobotPlayerInfo::lua_getScriptName(lua_State *L)
{
   return returnString(L, mRobot->getScriptName());
}


S32 RobotPlayerInfo::lua_getTeamIndx(lua_State *L)
{
   return returnInt(L, mRobot->getTeam() + 1);
}


S32 RobotPlayerInfo::lua_getRating(lua_State *L)
{
   return returnFloat(L, mRobot->getRating());
}


S32 RobotPlayerInfo::lua_getScore(lua_State *L)
{
   return returnInt(L, mRobot->getScore());
}


S32 RobotPlayerInfo::lua_isRobot(lua_State *L)
{
   return returnBool(L, true);
}


};
