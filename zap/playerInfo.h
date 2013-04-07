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

#ifndef _PLAYERINFO_H_
#define _PLAYERINFO_H_

#include "gameType.h"      // For PlayerInfo below...

namespace Zap
{

// Interface class to describe players/robots
class LuaPlayerInfo : public LuaObject
{
public:
   LuaPlayerInfo();  // Constructor
   virtual ~LuaPlayerInfo();

   ///// Lua interface
   LUAW_DECLARE_CLASS(LuaPlayerInfo);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   // Overridden by child classes, Is there a better way to do this?
   virtual S32 lua_getName(lua_State *L);
   virtual S32 lua_getShip(lua_State *L);
   virtual S32 lua_getTeamIndx(lua_State *L);
   virtual S32 lua_getRating(lua_State *L);
   virtual S32 lua_getScore(lua_State *L);
   virtual S32 lua_isRobot(lua_State *L);
   virtual S32 lua_getScriptName(lua_State *L);
};


////////////////////////////////////////
////////////////////////////////////////

class PlayerInfo : public LuaPlayerInfo
{
private:
  typedef LuaPlayerInfo Parent;
  ClientInfo *mClientInfo;

public:
   explicit PlayerInfo(ClientInfo *clientInfo);   // C++ Constructor
   virtual ~PlayerInfo();                // Destructor

   S32 lua_getName(lua_State *L);
   S32 lua_getShip(lua_State *L);
   S32 lua_getScriptName(lua_State *L);
   S32 lua_getTeamIndx(lua_State *L);
   S32 lua_getRating(lua_State *L);
   S32 lua_getScore(lua_State *L);
   S32 lua_isRobot(lua_State *L);
};


////////////////////////////////////////
////////////////////////////////////////

class Robot;

class RobotPlayerInfo : public LuaPlayerInfo
{
private:
   typedef LuaPlayerInfo Parent;
   Robot *mRobot;

public:
   explicit RobotPlayerInfo(Robot *robot = NULL);     // C++ Constructor
   virtual ~RobotPlayerInfo();               // Destructor

   S32 lua_getName(lua_State *L);
   S32 lua_getShip(lua_State *L);
   S32 lua_getScriptName(lua_State *L);
   S32 lua_getTeamIndx(lua_State *L);
   S32 lua_getRating(lua_State *L);
   S32 lua_getScore(lua_State *L);
   S32 lua_isRobot(lua_State *L);
};

};

#endif


