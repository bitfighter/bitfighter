//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LUAGAME_H_
#define _LUAGAME_H_


#include "LuaBase.h"
#include "LuaWrapper.h"

namespace Zap
{

// Forward declarations
class ServerGame;


class LuaGameInfo
{
private:
   ServerGame *mServerGame;

public:
   explicit LuaGameInfo(ServerGame *serverGame);     // Constructor
   virtual ~LuaGameInfo();                 // Destructor

   S32 lua_getGameType(lua_State *L);
   S32 lua_getGameTypeName(lua_State *L);
   S32 lua_getFlagCount(lua_State *L);
   S32 lua_getWinningScore(lua_State *L);
   S32 lua_getGameTimeTotal(lua_State *L);
   S32 lua_getGameTimeRemaining(lua_State *L);
   S32 lua_getLeadingScore(lua_State *L);
   S32 lua_getLeadingTeam(lua_State *L);
   S32 lua_getTeamCount(lua_State *L);

   S32 lua_getLevelName(lua_State *L);
   S32 lua_isTeamGame(lua_State *L);

   S32 lua_getEventScore(lua_State *L);

   S32 lua_getPlayers(lua_State *L);       // Return table of all players/bots in game
   S32 lua_isNexusOpen(lua_State *L);
   S32 lua_getNexusTimeLeft(lua_State *L);

   S32 lua_getTeam(lua_State *L);

   ///// Lua interface
   // Top level Lua methods
   LUAW_DECLARE_NON_INSTANTIABLE_CLASS(LuaGameInfo);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


};

#endif

