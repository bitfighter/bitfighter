#ifndef _LUAGAME_H_
#define _LUAGAME_H_


#include "luaObject.h"

namespace Zap
{

class LuaGameInfo : public LuaObject
{

public:
  // Constants

  // Initialize the pointer
  LuaGameInfo(lua_State *L);      // Constructor
  ~LuaGameInfo();                 // Destructor

   static const char className[];

   static Lunar<LuaGameInfo>::RegType methods[];

   S32 getClassID(lua_State *L);

   // Methods we will need to use
   S32 getGameType(lua_State *L);
   S32 getFlagCount(lua_State *L);
   S32 getWinningScore(lua_State *L);
   S32 getGameTimeTotal(lua_State *L);
   S32 getGameTimeRemaining(lua_State *L);
   S32 getLeadingScore(lua_State *L);
   S32 getLeadingTeam(lua_State *L);
  
   S32 getLevelName(lua_State *L);
   S32 getGridSize(lua_State *L);
   S32 getIsTeamGame(lua_State *L);

   S32 getEventScore(lua_State *L);
};

};

#endif
