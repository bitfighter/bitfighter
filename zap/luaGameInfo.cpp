//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "luaGameInfo.h"
#include "playerInfo.h"
#include "gameType.h"
#include "NexusGame.h"
#include "ServerGame.h"
#include "robot.h"


namespace Zap
{

// Constructor
LuaGameInfo::LuaGameInfo(ServerGame *serverGame)
{
   mServerGame = serverGame;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
LuaGameInfo::~LuaGameInfo()
{
   LUAW_DESTRUCTOR_CLEANUP;
}

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
   METHOD(CLASS, isTeamGame,           ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getEventScore,        ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getPlayers,           ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, isNexusOpen,          ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getNexusTimeLeft,     ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getTeam,              ARRAYDEF({{ TEAM_INDX, END }}), 1 ) \

GENERATE_LUA_FUNARGS_TABLE(LuaGameInfo, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(LuaGameInfo, LUA_METHODS);

#undef LUA_METHODS


/**
 * @luaclass LuaGameInfo
 * 
 * @brief Get information about the current game.
 * 
 * @descr You can get information about the current game with the GameInfo
 * object. You only need get this object once, then you can use it as often as
 * you like. It will always reflect the latest data. You can get this object
 * with `bf:getGameInfo()` (see LuaScriptRunner::getGameInfo()).
 */
const char *LuaGameInfo::luaClassName = "GameInfo";  // Class name as it appears to Lua scripts
REGISTER_LUA_CLASS(LuaGameInfo);


/**
 * @luafunc GameType LuaGameInfo::getGameType()
 * 
 * @brief Get the \ref GameTypeEnum of the current game.
 * 
 * @desc For possible values, see \ref GameTypeEnum.
 * 
 * @code
 *   print(info:getGameType() == GameType.CTFGameType) -- `true` when playing CTF
 * @endcode
 * 
 * @return A member of \ref GameTypeEnum.
 */
S32 LuaGameInfo::lua_getGameType(lua_State *L)
{
   TNLAssert(mServerGame->getGameType(), "Need Gametype check in getGameType");
   return returnInt(L, mServerGame->getGameType()->getGameTypeId());
}


/**
 * @luafunc string LuaGameInfo::getGameTypeName()
 *
 * @brief Get a string representing the name of the game type.
 *
 * @return The name of the game type.
 */
S32 LuaGameInfo::lua_getGameTypeName(lua_State *L) { return returnString(L, mServerGame->getGameType()->getGameTypeName()); }


/**
 * @luafunc int LuaGameInfo::getFlagCount()
 *
 * @brief Get the number of flags in play.
 *
 * @return The number of flags in play.
 */
S32 LuaGameInfo::lua_getFlagCount(lua_State *L) { return returnInt (L, mServerGame->getGameType()->getFlagCount()); }


/**
 * @luafunc int LuaGameInfo::getWinningScore()
 *
 * @brief Get the winning score.
 *
 * @return The number of flags in play.
 */
S32 LuaGameInfo::lua_getWinningScore(lua_State *L) { return returnInt (L, mServerGame->getGameType()->getWinningScore()); }


/**
 * @luafunc int LuaGameInfo::getGameTimeTotal()
 *
 * @brief Get the initial time limit of the game in seconds.
 *
 * @return The initial time limit of the game in seconds.
 */
S32 LuaGameInfo::lua_getGameTimeTotal(lua_State *L) { return returnInt (L, mServerGame->getGameType()->getTotalGameTime()); }


/**
 * @luafunc int LuaGameInfo::getGameTimeRemaining()
 *
 * @brief Get the number of seconds until the game ends.
 *
 * @return The number of seconds until the game ends.
 */
S32 LuaGameInfo::lua_getGameTimeRemaining(lua_State *L) { return returnInt (L, mServerGame->getGameType()->getRemainingGameTime()); }


/**
 * @luafunc int LuaGameInfo::getLeadingScore()
 *
 * @brief Get the highest score among all teams.
 *
 * @return The highest score among all teams.
 */
S32 LuaGameInfo::lua_getLeadingScore(lua_State *L) { return returnInt (L, mServerGame->getGameType()->getLeadingScore()); }


/**
 * @luafunc int LuaGameInfo::getLeadingTeam()
 *
 * @brief Get the index of the team with the highest score.
 *
 * @return The index of the team with the highest score.
 */
S32 LuaGameInfo::lua_getLeadingTeam(lua_State *L) { return returnInt (L, mServerGame->getGameType()->getLeadingTeam() + 1); }


/**
 * @luafunc int LuaGameInfo::getTeamCount()
 * 
 * @brief Get the number of teams in the game.
 * 
 * @return The number of teams in the game.
 */
S32 LuaGameInfo::lua_getTeamCount(lua_State *L) { return returnInt (L, mServerGame->getTeamCount()); }


/**
 * @luafunc string LuaGameInfo::getLevelName()
 *
 * @brief Get a string representing the level's name.
 *
 * @return A string representing the level's name.
 */
S32 LuaGameInfo::lua_getLevelName(lua_State *L) { return returnString(L, mServerGame->getGameType()->getLevelName()->getString()); }


/**
 * @luafunc bool LuaGameInfo::isTeamGame()
 *
 * @return `true` if there is more than one team.
 */
S32 LuaGameInfo::lua_isTeamGame(lua_State *L) { return returnBool (L, mServerGame->getGameType()->isTeamGame()); }


/**
 * @luafunc bool LuaGameInfo::isNexusOpen()
 *
 * @brief Get whether the nexus is open
 *
 * @return `true` if the Nexus is open during a Nexus game.
 */
S32 LuaGameInfo::lua_isNexusOpen(lua_State *L)
{
   GameType *gameType = mServerGame->getGameType();
   if(!gameType || gameType->getGameTypeId() != NexusGame)
      return returnNil(L);

   return returnBool(L, static_cast<NexusGameType *>(gameType)->mNexusIsOpen);
}


/**
 * @luafunc int LuaGameInfo::getNexusTimeLeft()
 *
 * @brief The number of seconds until the nexus opens or closes.
 *
 * @return The number of seconds until the Nexus opens or closes, or nil if this
 * is not a nexus game.
 */
S32 LuaGameInfo::lua_getNexusTimeLeft(lua_State *L)
{
   GameType *gameType = mServerGame->getGameType();
   if(!gameType || gameType->getGameTypeId() != NexusGame)
      return returnNil(L);

   return returnInt(L, static_cast<NexusGameType *>(gameType)->getNexusTimeLeftMs() / 1000);
}


/**
 * @luafunc int LuaGameInfo::getEventScore(ScoringEvent event)
 *
 * @brief Get the value of a \ref ScoringEventEnum.
 *
 * @return The number of points earned when the \ref ScoringEventEnum `event`
 * occurs, or zero of an unknown event is specified.
 */
S32 LuaGameInfo::lua_getEventScore(lua_State *L)
{
   static const char *methodName = "GameInfo:getEventScore()";
   checkArgCount(L, 1, methodName);
   ScoringEvent scoringEvent = (ScoringEvent) getInt(L, 1, methodName, 0, ScoringEventsCount - 1);

   return returnInt(L, mServerGame->getGameType()->getEventScore(GameType::TeamScore, scoringEvent, 0));
};


/**
 * @luafunc table LuaGameInfo::getPlayers(int teamIndex)
 *
 * @brief Get a list of the players on a team.
 *
 * @return A table containing the LuaPlayerInfo for each player (and robot) on
 * the team.
 */
S32 LuaGameInfo::lua_getPlayers(lua_State *L) 
{
   ServerGame *game = mServerGame;

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


/**
 * @luafunc Team LuaGameInfo::getTeam(int teamIndex)
 *
 * @brief Get a Team by index.
 *
 * @return The Team with the specified index.
 */
S32 LuaGameInfo::lua_getTeam(lua_State *L)
{
   checkArgList(L, functionArgs, "GameInfo", "getTeam");

   S32 index = getTeamIndex(L, 1);
   
   if(U32(index) >= U32(mServerGame->getTeamCount())) // Out of range index?
      return returnNil(L);

   TNLAssert(dynamic_cast<Team*>(mServerGame->getTeam(index)), "Bad team pointer or bad type");
   return returnTeam(L, static_cast<Team*>(mServerGame->getTeam(index)));
}


};
