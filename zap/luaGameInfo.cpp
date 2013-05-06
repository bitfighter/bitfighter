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
   checkArgList(L, functionArgs, "GameInfo", "setTeam");

   S32 index = getTeamIndex(L, 1);

   return returnTeam(L, static_cast<Team*>(gServerGame->getTeam(index)));
}



////////////////////////////////////
////////////////////////////////////


// C++ Constructor -- specify items
LuaLoadout::LuaLoadout(const Vector<U8> &loadout)
{
   // When creating a new loadout object, load it up with the
   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      mLoadout[i] = loadout[i];

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
LuaLoadout::~LuaLoadout()
{
   LUAW_DESTRUCTOR_CLEANUP;
   logprintf(LogConsumer::LogLuaObjectLifecycle, "Deleted LuaLoadout object (%p)\n", this);     // Never gets run...
}


/**
 *  @luaclass Loadout
 *  @brief    Get and set ship Loadout properties
 *  @descr    Use the %Loadout object to modify a ship or robots current loadout
 *
 *  You only need get this object once, then you can use it as often as you like. It will
 *  always reflect the latest data.
 */
//                Fn name                  Param profiles            Profile count
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, setWeapon,      ARRAYDEF({{ WEAP_SLOT, WEAP_ENUM, END }}), 1 )  \
   METHOD(CLASS, setModule,      ARRAYDEF({{ MOD_SLOT, MOD_ENUM, END }}), 1 )    \
   METHOD(CLASS, isValid,        ARRAYDEF({{ END }}), 1 )                        \
   METHOD(CLASS, equals,         ARRAYDEF({{ LOADOUT, END }}), 1 )               \
   METHOD(CLASS, getWeapon,      ARRAYDEF({{ WEAP_SLOT, END }}), 1 )             \
   METHOD(CLASS, getModule,      ARRAYDEF({{ MOD_SLOT, END }}), 1 )              \

GENERATE_LUA_FUNARGS_TABLE(LuaLoadout, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(LuaLoadout, LUA_METHODS);

#undef LUA_METHODS

const char *LuaLoadout::luaClassName = "Loadout";     // Class name as it appears to Lua scripts
REGISTER_LUA_CLASS(LuaLoadout);


S32 LuaLoadout::lua_setWeapon(lua_State *L)     // setWeapon(i, mod) ==> Set weapon at index i
{
   checkArgList(L, functionArgs, "Loadout", "setWeapon");

   U32 index = (U32) getInt(L, 1);
   U32 weapon = (U32) getInt(L, 2);

   mLoadout[index + ShipModuleCount - 1] = weapon;

   return 0;
}


S32 LuaLoadout::lua_setModule(lua_State *L)     // setModule(i, mod) ==> Set module at index i
{
   checkArgList(L, functionArgs, "Loadout", "setModule");

   U32 index = (U32) getInt(L, 1);
   U32 module  = (U32) getInt(L, 2);

   mLoadout[index - 1] = module;

   return 0;
}


S32 LuaLoadout::lua_isValid(lua_State *L)       // isValid() ==> Is loadout config valid?
{
   U32 mod[ShipModuleCount];
   bool hasSensor = false;

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      for(S32 j = 0; j < i; j++)
         if(mod[j] == mLoadout[i])     // Duplicate entry!
            return returnBool(L, false);
      mod[i] = mLoadout[i];
      if(mLoadout[i] == ModuleSensor)
         hasSensor = true;
   }

   U32 weap[ShipWeaponCount];
   bool hasSpyBug = false;

   for(S32 i = 0; i < ShipWeaponCount; i++)
   {
      for(S32 j = 0; j < i; j++)
         if(weap[j] == mLoadout[i + ShipModuleCount])     // Duplicate entry!
            return returnBool(L, false);
      weap[i] = mLoadout[i + ShipModuleCount];
      if(mLoadout[i] == WeaponSpyBug)
         hasSpyBug = true;
   }

   // Make sure we don't have any invalid combos
   if(hasSpyBug && !hasSensor)
      return returnBool(L, false);

   return returnBool(L, true);
}


S32 LuaLoadout::lua_equals(lua_State *L)        // equals(Loadout) ==> is loadout the same as Loadout?
{
   checkArgList(L, functionArgs, "Loadout", "equals");

   LuaLoadout *loadout = luaW_check<LuaLoadout>(L, 1);

   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      if(mLoadout[i] != loadout->getLoadoutItem(i))
         return returnBool(L, false);

   return returnBool(L, true);
}


// Private helper function for above
U8 LuaLoadout::getLoadoutItem(S32 index) const
{
   return mLoadout[index];
}


S32 LuaLoadout::lua_getWeapon(lua_State *L)     // getWeapon(i) ==> return weapon at index i (1, 2, 3)
{
   checkArgList(L, functionArgs, "Loadout", "getWeapon");

   U32 weapon = (U32) getInt(L, 1);

   return returnInt(L, mLoadout[weapon + ShipModuleCount - 1]);
}


S32 LuaLoadout::lua_getModule(lua_State *L)     // getModule(i) ==> return module at index i (1, 2)
{
   checkArgList(L, functionArgs, "Loadout", "getModule");

   U32 module = (U32) getInt(L, 1);

   return returnInt(L, mLoadout[module - 1]);
}


};
