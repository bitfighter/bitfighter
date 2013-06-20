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

#include "luaGameInfo.h"
#include "playerInfo.h"
#include "gameType.h"
#include "NexusGame.h"
#include "ServerGame.h"
#include "robot.h"


namespace Zap
{

// Constructor
LuaGameInfo::LuaGameInfo(lua_State *L)
{
   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
LuaGameInfo::~LuaGameInfo()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


/**
 *  @luaclass GameInfo
 *  @brief    Get information about the current game.
 *  @descr    You can get information about the current game with the %GameInfo object.
 *
 *  You only need get this object once, then you can use it as often as you like. It will
 *  always reflect the latest data.
 */
//                Fn name                  Param profiles            Profile count
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getGameType,          ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getGameTypeName,      ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getFlagCount,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getWinningScore,      ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getGameTimeTotal,     ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getGameTimeRemaining, ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getLeadingScore,      ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getLeadingTeam,       ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getTeamCount,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getLevelName,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getGridSize,          ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, isTeamGame,           ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getEventScore,        ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getPlayers,           ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, isNexusOpen,          ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getNexusTimeLeft,     ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getTeam,              ARRAYDEF({{ TEAM_INDX, END }}), 1 ) \

GENERATE_LUA_FUNARGS_TABLE(LuaGameInfo, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(LuaGameInfo, LUA_METHODS);

#undef LUA_METHODS


const char *LuaGameInfo::luaClassName = "GameInfo";  // Class name as it appears to Lua scripts
REGISTER_LUA_CLASS(LuaGameInfo);


extern ServerGame *gServerGame;

S32 LuaGameInfo::lua_getGameType(lua_State *L)
{
   TNLAssert(gServerGame->getGameType(), "Need Gametype check in getGameType");
   return returnInt(L, gServerGame->getGameType()->getGameTypeId());
}


S32 LuaGameInfo::lua_getGameTypeName(lua_State *L)      { return returnString(L, gServerGame->getGameType()->getGameTypeName()); }
S32 LuaGameInfo::lua_getFlagCount(lua_State *L)         { return returnInt   (L, gServerGame->getGameType()->getFlagCount()); }
S32 LuaGameInfo::lua_getWinningScore(lua_State *L)      { return returnInt   (L, gServerGame->getGameType()->getWinningScore()); }
S32 LuaGameInfo::lua_getGameTimeTotal(lua_State *L)     { return returnInt   (L, gServerGame->getGameType()->getTotalGameTime()); }
S32 LuaGameInfo::lua_getGameTimeRemaining(lua_State *L) { return returnInt   (L, gServerGame->getGameType()->getRemainingGameTime()); }
S32 LuaGameInfo::lua_getLeadingScore(lua_State *L)      { return returnInt   (L, gServerGame->getGameType()->getLeadingScore()); }
S32 LuaGameInfo::lua_getLeadingTeam(lua_State *L)       { return returnInt   (L, gServerGame->getGameType()->getLeadingTeam() + 1); }
                                                                         
S32 LuaGameInfo::lua_getTeamCount(lua_State *L)         { return returnInt   (L, gServerGame->getTeamCount()); }

S32 LuaGameInfo::lua_getLevelName(lua_State *L)         { return returnString(L, gServerGame->getGameType()->getLevelName()->getString()); }
S32 LuaGameInfo::lua_getGridSize(lua_State *L)          { return returnFloat (L, gServerGame->getGridSize()); }
S32 LuaGameInfo::lua_isTeamGame(lua_State *L)           { return returnBool  (L, gServerGame->getGameType()->isTeamGame()); }


S32 LuaGameInfo::lua_isNexusOpen(lua_State *L)
{
   GameType *gameType = gServerGame->getGameType();
   if(!gameType || gameType->getGameTypeId() != NexusGame)
      return returnNil(L);

   return returnBool(L, static_cast<NexusGameType *>(gameType)->mNexusIsOpen);
}


S32 LuaGameInfo::lua_getNexusTimeLeft(lua_State *L)
{
   GameType *gameType = gServerGame->getGameType();
   if(!gameType || gameType->getGameTypeId() != NexusGame)
      return returnNil(L);

   return returnInt(L, static_cast<NexusGameType *>(gameType)->getNexusTimeLeft());
}


S32 LuaGameInfo::lua_getEventScore(lua_State *L)
{
   static const char *methodName = "GameInfo:getEventScore()";
   checkArgCount(L, 1, methodName);
   ScoringEvent scoringEvent = (ScoringEvent) getInt(L, 1, methodName, 0, ScoringEventsCount - 1);

   return returnInt(L, gServerGame->getGameType()->getEventScore(GameType::TeamScore, scoringEvent, 0));
};


// Return a table listing all players on this team
S32 LuaGameInfo::lua_getPlayers(lua_State *L) 
{
   ServerGame *game = gServerGame;

   S32 pushed = 0;     // Count of pushed objects

   lua_newtable(L);    // Create a table, with no slots pre-allocated for our data

   for(S32 i = 0; i < game->getClientCount(); i++)
   {
      ClientInfo *clientInfo = game->getClientInfo(i);

      if(clientInfo->getPlayerInfo() == NULL || clientInfo->isRobot())     // Skip defunct players and bots
         continue;
      
      clientInfo->getPlayerInfo()->push(L);
      pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   for(S32 i = 0; i < game->getRobotCount(); i ++)
   {
      game->getBot(i)->getPlayerInfo()->push(L);
      pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   return 1;
}


S32 LuaGameInfo::lua_getTeam(lua_State *L)
{
   checkArgList(L, functionArgs, "GameInfo", "getTeam");

   S32 index = getTeamIndex(L, 1);
   
   if(U32(index) >= U32(gServerGame->getTeamCount())) // Out of range index?
      return returnNil(L);

   TNLAssert(dynamic_cast<Team*>(gServerGame->getTeam(index)), "Bad team pointer or bad type");
   return returnTeam(L, static_cast<Team*>(gServerGame->getTeam(index)));
}


};
