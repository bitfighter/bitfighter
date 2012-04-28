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

const char LuaPlayerInfo::className[] = "PlayerInfo";      // Class name as it appears to Lua scripts

// Define the methods we will expose to Lua
Lunar<LuaPlayerInfo>::RegType LuaPlayerInfo::methods[] =
{
   method(LuaPlayerInfo, getName),
   method(LuaPlayerInfo, getShip),
   method(LuaPlayerInfo, getScriptName),     // Filename of script the bot is running
   method(LuaPlayerInfo, getTeamIndx),
   method(LuaPlayerInfo, getScore),
   method(LuaPlayerInfo, getRating),
   method(LuaPlayerInfo, isRobot),

   {0,0}    // End method list
};


// Constructor
LuaPlayerInfo::LuaPlayerInfo()
{
   defunct = false;
}


// Lua constructor
LuaPlayerInfo::LuaPlayerInfo(lua_State *L)
{
   // Do nothing
}


S32 LuaPlayerInfo::getName(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::getShip(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::getTeamIndx(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::getRating(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::getScore(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::isRobot(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


S32 LuaPlayerInfo::getScriptName(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return 0;
}


void LuaPlayerInfo::setDefunct()
{
   defunct = true;
}


bool LuaPlayerInfo::isDefunct()
{
   return defunct;
}


void LuaPlayerInfo::push(lua_State *L)
{
   // false ==> We do not want Lunar to try and delete the C++ object when Lua is done with it
   Lunar<LuaPlayerInfo>::push(L, this, false);
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


S32 PlayerInfo::getName(lua_State *L)
{
   return returnString(L, mClientInfo->getName().getString());
}


S32 PlayerInfo::getScriptName(lua_State *L)
{
   return returnNil(L);
}


S32 PlayerInfo::getTeamIndx(lua_State *L)
{
   // Lua indexes are 1-based
   return returnInt(L, mClientInfo->getTeamIndex() + 1);
}


S32 PlayerInfo::getRating(lua_State *L)
{
   return returnFloat(L, mClientInfo->getRating());
}


S32 PlayerInfo::getScore(lua_State *L)
{
   return returnInt(L, mClientInfo->getScore());
}


S32 PlayerInfo::isRobot(lua_State *L)
{
   return returnBool(L, false);
}


S32 PlayerInfo::getShip(lua_State *L)
{
	return isDefunct() ? returnNil(L) : returnShip(L, mClientInfo->getShip());
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


S32 RobotPlayerInfo::getName(lua_State *L)
{
   return returnString(L, mRobot->getClientInfo()->getName().getString());
}


S32 RobotPlayerInfo::getShip(lua_State *L)
{
   return isDefunct() ? returnNil(L) : returnShip(L, mRobot);
}


S32 RobotPlayerInfo::getScriptName(lua_State *L)
{
   return returnString(L, mRobot->getScriptName());
}


S32 RobotPlayerInfo::getTeamIndx(lua_State *L)
{
   return returnInt(L, mRobot->getTeam() + 1);
}


S32 RobotPlayerInfo::getRating(lua_State *L)
{
   return returnFloat(L, mRobot->getRating());
}


S32 RobotPlayerInfo::getScore(lua_State *L)
{
   return returnInt(L, mRobot->getScore());
}


S32 RobotPlayerInfo::isRobot(lua_State *L)
{
   return returnBool(L, true);
}


};
