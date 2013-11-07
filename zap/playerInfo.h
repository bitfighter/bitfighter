//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _PLAYERINFO_H_
#define _PLAYERINFO_H_

#include "gameType.h"      // For PlayerInfo below...

namespace Zap
{

// Interface class to describe players/robots
class LuaPlayerInfo
{
public:
   LuaPlayerInfo();  // Constructor
   virtual ~LuaPlayerInfo();

   ///// Lua interface
   LUAW_DECLARE_NON_INSTANTIABLE_CLASS(LuaPlayerInfo);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   // Overridden by child classes, Is there a better way to do this?
   virtual S32 lua_getName(lua_State *L);
   virtual S32 lua_getShip(lua_State *L);
   virtual S32 lua_getTeamIndx(lua_State *L);
   virtual S32 lua_getTeamIndex(lua_State *L);
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
   S32 lua_getTeamIndex(lua_State *L);
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
   S32 lua_getTeamIndex(lua_State *L);
   S32 lua_getRating(lua_State *L);
   S32 lua_getScore(lua_State *L);
   S32 lua_isRobot(lua_State *L);
};

};

#endif


