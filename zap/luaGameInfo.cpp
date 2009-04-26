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
#include "luaGameInfo.h"
#include "gameType.h"

namespace Zap
{

const char LuaGameInfo::className[] = "GameInfo";      // This is the class name as it appears to the Lua scripts

// Constructor
LuaGameInfo::LuaGameInfo(lua_State *L)
{
   //lua_atpanic(L, luaPanicked);                 // Register our panic function

   // Game Types
   setGTEnum(BitmatchGame);
   setGTEnum(CTFGame);
   setGTEnum(HTFGame);
   setGTEnum(NexusGame);
   setGTEnum(RabbitGame);
   setGTEnum(RetrieveGame);
   setGTEnum(SoccerGame);
   setGTEnum(ZoneControlGame);

   // Scoring Events
   setGTEnum(KillEnemy);
   setGTEnum(KillSelf);
   setGTEnum(KillTeammate);
   setGTEnum(KillEnemyTurret);
   setGTEnum(KillOwnTurret);
   setGTEnum(CaptureFlag);
   setGTEnum(CaptureZone);
   setGTEnum(UncaptureZone);
   setGTEnum(HoldFlagInZone);
   setGTEnum(RemoveFlagFromEnemyZone);
   setGTEnum(RabbitHoldsFlag);
   setGTEnum(RabbitKilled);
   setGTEnum(RabbitKills);
   setGTEnum(ReturnFlagsToNexus);
   setGTEnum(ReturnFlagToZone);
   setGTEnum(LostFlag);
   setGTEnum(ReturnTeamFlag);
   setGTEnum(ScoreGoalEnemyTeam);
   setGTEnum(ScoreGoalHostileTeam);
   setGTEnum(ScoreGoalOwnTeam);
}

// Destructor
LuaGameInfo::~LuaGameInfo(){
  logprintf("deleted Lua Game Object (%p)\n", this);
}


// Define the methods we will expose to Lua
// Methods defined here need to be defined in the LuaRobot in robot.h

Lunar<LuaGameInfo>::RegType LuaGameInfo::methods[] = {
   method(LuaGameInfo, getClassID),

   method(LuaGameInfo, getGameType),
   method(LuaGameInfo, getFlagCount),
   method(LuaGameInfo, getWinningScore),
   method(LuaGameInfo, getGameTimeTotal),
   method(LuaGameInfo, getGameTimeRemaining),
   method(LuaGameInfo, getLeadingScore),
   method(LuaGameInfo, getLeadingTeam),
   method(LuaGameInfo, getLevelName),
   method(LuaGameInfo, getGridSize),
   method(LuaGameInfo, getIsTeamGame),
   method(LuaGameInfo, getEventScore),

   {0,0}    // End method list
};


S32 LuaGameInfo::getClassID(lua_State *L)
{
   return returnInt(L, BIT(30));    // TODO: Make this a constant
}


S32 LuaGameInfo::getGameType(lua_State *L)
{
   TNLAssert(gServerGame->getGameType(), "Need Gametype check in getGameType");
   return returnInt(L, gServerGame->getGameType()->getGameType());
}


S32 LuaGameInfo::getFlagCount(lua_State *L)         { return returnInt(L, gServerGame->getGameType()->getFlagCount()); }
S32 LuaGameInfo::getWinningScore(lua_State *L)      { return returnInt(L, gServerGame->getGameType()->getWinningScore()); }
S32 LuaGameInfo::getGameTimeTotal(lua_State *L)     { return returnInt(L, gServerGame->getGameType()->getTotalGameTime()); }
S32 LuaGameInfo::getGameTimeRemaining(lua_State *L) { return returnInt(L, gServerGame->getGameType()->getRemainingGameTime()); }
S32 LuaGameInfo::getLeadingScore(lua_State *L)      { return returnInt(L, gServerGame->getGameType()->getLeadingScore()); }
S32 LuaGameInfo::getLeadingTeam(lua_State *L)       { return returnInt(L, gServerGame->getGameType()->getLeadingTeam()); }

S32 LuaGameInfo::getLevelName(lua_State *L)         { 
   return returnString(L, gServerGame->getGameType()->mLevelName.getString()); }
S32 LuaGameInfo::getGridSize(lua_State *L)          { return returnFloat(L, gServerGame->getGridSize()); }
S32 LuaGameInfo::getIsTeamGame(lua_State *L)        { return returnBool(L, gServerGame->getGameType()->isTeamGame()); }


S32 LuaGameInfo::getEventScore(lua_State *L) 
{ 
   S32 n = lua_gettop(L);  // Number of arguments
   if (n != 1)
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "getEventScore called with %d args, expected 1", n);
      logprintf(msg);
      throw(string(msg)); 
   }

   if(!lua_isnumber(L, 1))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "getEventScore called with non-numeric arg");
      logprintf(msg);
      throw(string(msg)); 
   }

   U32 scoringEvent = (U32) luaL_checkinteger(L, 1);
   if(scoringEvent > GameType::ScoringEventsCount)
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "getEventScore called with out-of-bounds arg: %d", scoringEvent);
      logprintf(msg);
      throw(string(msg));
   }

   return returnInt(L, gServerGame->getGameType()->getEventScore(GameType::TeamScore, (GameType::ScoringEvent) scoringEvent, 0));
};

};