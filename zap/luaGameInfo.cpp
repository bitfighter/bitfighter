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

const char LuaGameInfo::className[] = "GameInfo";      // Class name as it appears to Lua scripts

// Constructor
LuaGameInfo::LuaGameInfo(lua_State *L)
{
   // Do nothing
}


// Destructor
LuaGameInfo::~LuaGameInfo()
{
   logprintf("deleted Lua Game Object (%p)\n", this);     // Never gets run...
}


// Define the methods we will expose to Lua
Lunar<LuaGameInfo>::RegType LuaGameInfo::methods[] = {
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


extern ServerGame *gServerGame;

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



////////////////

const char LuaWeaponInfo::className[] = "WeaponInfo";      // Class name as it appears to Lua scripts

// C++ constructor
LuaWeaponInfo::LuaWeaponInfo(WeaponType weapon)
{
   mWeaponIndex = (S32) weapon;
}


// Lua constructor
LuaWeaponInfo::LuaWeaponInfo(lua_State *L)
{
   mWeaponIndex = 0;

   S32 n = lua_gettop(L);  // Number of arguments
   if (n != 1)
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "WeaponInfo() called with %d args, expected 1", n);
      logprintf(msg);
      throw(string(msg));
   }

   if(!lua_isnumber(L, 1))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "WeaponInfo() called with non-numeric arg");
      logprintf(msg);
      throw(string(msg));
   }

   U32 weapon = (U32) luaL_checkinteger(L, 1);    // Use U32 for simpler bounds checking
   if(weapon >= (U32) WeaponCount)
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "WeaponInfo() called with invalid weapon ID: %d", weapon);
      logprintf(msg);
      throw(string(msg));
   }

   mWeaponIndex = (S32) weapon;
}


// Destructor
LuaWeaponInfo::~LuaWeaponInfo()
{
   logprintf("deleted LuaWeaponInfo object (%p)\n", this);     // Never gets run...
}


// Define the methods we will expose to Lua
Lunar<LuaWeaponInfo>::RegType LuaWeaponInfo::methods[] = 
{
   method(LuaWeaponInfo, getName),
   method(LuaWeaponInfo, getID),

   method(LuaWeaponInfo, getRange),
   method(LuaWeaponInfo, getFireDelay),
   method(LuaWeaponInfo, getMinEnergy),
   method(LuaWeaponInfo, getEnergyDrain),
   method(LuaWeaponInfo, getProjVel),
   method(LuaWeaponInfo, getProjLife),
   method(LuaWeaponInfo, getDamage),
   method(LuaWeaponInfo, getCanDamageSelf),
   method(LuaWeaponInfo, getCanDamageTeammate),

   {0,0}    // End method list
};


S32 LuaWeaponInfo::getName(lua_State *L) { return returnString(L, gWeapons[mWeaponIndex].name.getString()); }					// Name of weapon ("Phaser", "Triple", etc.) (string)
S32 LuaWeaponInfo::getID(lua_State *L) { return returnInt(L, mWeaponIndex); }					                                 // ID of module (WeaponPhaser, WeaponTriple, etc.) (integer)

S32 LuaWeaponInfo::getRange(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].projVelocity * gWeapons[mWeaponIndex].projVelocity / 1000 ); }					   // Range of projectile (units) (integer)
S32 LuaWeaponInfo::getFireDelay(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].fireDelay); }					      // Delay between shots in ms (integer)
S32 LuaWeaponInfo::getMinEnergy(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].minEnergy); }				         // Minimum energy needed to use (integer)
S32 LuaWeaponInfo::getEnergyDrain(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].drainEnergy); }				   // Amount of energy weapon consumes (integer)
S32 LuaWeaponInfo::getProjVel(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].projVelocity); }					   // Speed of projectile (units/sec) (integer)
S32 LuaWeaponInfo::getProjLife(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].projLiveTime); }					   // Time projectile will live (ms) (integer, -1 == live forever)
S32 LuaWeaponInfo::getDamage(lua_State *L) { return returnFloat(L, gWeapons[mWeaponIndex].damageAmount); }				   	// Damage projectile does (0-1, where 1 = total destruction) (float)
S32 LuaWeaponInfo::getCanDamageSelf(lua_State *L) { return returnBool(L, gWeapons[mWeaponIndex].canDamageSelf); }			   // Will weapon damage self? (boolean)
S32 LuaWeaponInfo::getCanDamageTeammate(lua_State *L) { return returnBool(L, gWeapons[mWeaponIndex].canDamageTeammate); }	// Will weapon damage teammates? (boolean)



////////////////

const char LuaModuleInfo::className[] = "ModuleInfo";      // Class name as it appears to Lua scripts

// Constructor
LuaModuleInfo::LuaModuleInfo(lua_State *L)
{
   mModuleIndex = 0;

   S32 n = lua_gettop(L);  // Number of arguments
   if (n != 1)
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "ModuleInfo() called with %d args, expected 1", n);
      logprintf(msg);
      throw(string(msg));
   }

   if(!lua_isnumber(L, 1))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "ModuleInfo() called with non-numeric arg");
      logprintf(msg);
      throw(string(msg));
   }

   U32 mod = (U32) luaL_checkinteger(L, 1);    // Use U32 for simpler bounds checking
   if(mod >= (U32) ModuleCount)
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "ModuleInfo() called with invalid module ID: %d", mod);
      logprintf(msg);
      delete this;
      throw(string(msg));
   }

   mModuleIndex = (S32) mod;
}


// Destructor
LuaModuleInfo::~LuaModuleInfo()
{
   logprintf("deleted LuaModuleInfo object (%p)\n", this);     // Never gets run...
}


// Define the methods we will expose to Lua
Lunar<LuaModuleInfo>::RegType LuaModuleInfo::methods[] = 
{
   method(LuaModuleInfo, getName),
   method(LuaModuleInfo, getID),

   {0,0}    // End method list
};


extern const char *gModuleShortName[] ;

S32 LuaModuleInfo::getName(lua_State *L) { return returnString(L, gModuleShortName[mModuleIndex]); }					// Name of module ("Shield", "Turbo", etc.) (string)
S32 LuaModuleInfo::getID(lua_State *L) { return returnInt(L, mModuleIndex); }					                        // ID of module (ModuleShield, ModuleBoost, etc.) (integer)


};