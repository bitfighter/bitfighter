#ifndef _LUAGAME_H_
#define _LUAGAME_H_


#include "luaObject.h"

namespace Zap
{

class LuaGameInfo : public LuaObject
{

public:
  // Initialize the pointer
  LuaGameInfo(lua_State *L);      // Constructor
  ~LuaGameInfo();                 // Destructor

   static const char className[];

   static Lunar<LuaGameInfo>::RegType methods[];

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



class LuaWeaponInfo : public LuaObject
{

private:
   ShipWeaponInfo mWeaponInfo;

public:
   // Initialize the pointer
   LuaWeaponInfo(lua_State *L);      // Constructor
   ~LuaWeaponInfo();                 // Destructor

   static const char className[];

   static Lunar<LuaWeaponInfo>::RegType methods[];

   S32 getName(lua_State *L);
   S32 getFireDelay(lua_State *L);
   S32 getMinEnergy(lua_State *L);
   S32 getEnergyDrain(lua_State *L);
   S32 getProjVel(lua_State *L);
   S32 getProjLife(lua_State *L);
   S32 getDamage(lua_State *L);
   S32 getCanDamageSelf(lua_State *L);
   S32 getCanDamageTeammate(lua_State *L);

};

#endif
