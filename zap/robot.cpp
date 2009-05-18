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

#include "robot.h"
#include "item.h"

#include "gameType.h"
#include "sparkManager.h"
#include "projectile.h"
#include "gameLoader.h"
#include "sfx.h"
#include "UI.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "gameConnection.h"
#include "shipItems.h"
#include "gameItems.h"
#include "gameWeapons.h"
#include "gameObjectRender.h"
#include "flagItem.h"
#include "goalZone.h"
#include "soccerGame.h"          // For lua object defs
#include "huntersGame.h"         // For lua object defs
#include "config.h"
#include "BotNavMeshZone.h"      // For BotNavMeshZone class definition
#include "luaGameInfo.h"
#include "../glut/glutInclude.h"

#define hypot _hypot    // Kill some warnings

namespace Zap
{

static Vector<GameObject *> fillVector;

// Constructor
LuaRobot::LuaRobot(lua_State *L)
{
   lua_atpanic(L, luaPanicked);                 // Register our panic function
   thisRobot = (Robot*)lua_touserdata(L, 1);    // Register our robot

   // The following sets scads of global vars in the Lua instance that mimic the use of the enums we use everywhere

   // Game Objects
   setEnum(ShipType);
   setEnum(BarrierType);
   setEnum(MoveableType);
   setEnum(BulletType);
   setEnum(ItemType);
   setEnum(ResourceItemType);
   setEnum(EngineeredType);
   setEnum(ForceFieldType);
   setEnum(LoadoutZoneType);
   setEnum(MineType);
   setEnum(TestItemType);
   setEnum(FlagType);
   setEnum(NexusFlagType);
   setEnum(TurretTargetType);
   setEnum(SlipZoneType);
   setEnum(HeatSeekerType);
   setEnum(SpyBugType);
   setEnum(NexusType);
   setEnum(BotNavMeshZoneType);
   setEnum(RobotType);
   setEnum(TeleportType);
   setEnum(GoalZoneType);
   setEnum(AsteroidType);
   setEnum(RepairItemType);
   setEnum(SoccerBallItemType);



   // Modules
   setEnum(ModuleShield);
   setEnum(ModuleBoost);
   setEnum(ModuleSensor);
   setEnum(ModuleRepair);
   setEnum(ModuleEngineer);
   setEnum(ModuleCloak);

   // Weapons
   setEnum(WeaponPhaser);
   setEnum(WeaponBounce);
   setEnum(WeaponTriple);
   setEnum(WeaponBurst);
   setEnum(WeaponMine);
   setEnum(WeaponSpyBug);
   setEnum(WeaponTurret);

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
LuaRobot::~LuaRobot()
{
  logprintf("deleted Lua Robot Object (%p)\n", this);
}


// Define the methods we will expose to Lua
// Methods defined here need to be defined in the LuaRobot in robot.h

Lunar<LuaRobot>::RegType LuaRobot::methods[] = {
   method(LuaRobot, getClassID),

   method(LuaRobot, getCPUTime),
   method(LuaRobot, getTime),

   method(LuaRobot, getAngle),
   method(LuaRobot, getLoc),

   method(LuaRobot, getZoneCenter),
   method(LuaRobot, getGatewayFromZoneToZone),
   method(LuaRobot, getZoneCount),
   method(LuaRobot, getCurrentZone),

   method(LuaRobot, setAngle),
   method(LuaRobot, setAngleXY),
   method(LuaRobot, getAngleXY),
   method(LuaRobot, hasLosXY),

   method(LuaRobot, hasFlag),
   method(LuaRobot, getWaypoint),

   method(LuaRobot, setThrustAng),
   method(LuaRobot, setThrustXY),
   method(LuaRobot, fire),
   method(LuaRobot, setWeapon),
   method(LuaRobot, setWeaponIndex),
   method(LuaRobot, hasWeapon),

   method(LuaRobot, activateModule),
   method(LuaRobot, activateModuleIndex),
   method(LuaRobot, setReqLoadout),
   method(LuaRobot, getCurrLoadout),
   method(LuaRobot, getReqLoadout),

   method(LuaRobot, globalMsg),
   method(LuaRobot, teamMsg),

   method(LuaRobot, getWeapon),

   method(LuaRobot, logprint),

   method(LuaRobot, findObjects),

   method(LuaRobot, findItems),
   method(LuaRobot, findGlobalItems),
   method(LuaRobot, getFiringSolution),

   {0,0}    // End method list
};


S32 LuaRobot::getClassID(lua_State *L)
{
   return returnInt(L, RobotType);
}


// Return CPU time... use for timing things
S32 LuaRobot::getCPUTime(lua_State *L)
{
   return returnInt(L, gServerGame->getCurrentTime());
}


// Turn to angle a (in radians)
S32 LuaRobot::setAngle(lua_State *L)
{
   static const char *methodName = "Robot:setAngle()";
   checkArgCount(L, 1, methodName);

   Move move = thisRobot->getCurrentMove();
   move.angle = getFloat(L, 1, methodName);
   thisRobot->setCurrentMove(move);

   return 0;
}


// Turn towards point XY
S32 LuaRobot::setAngleXY(lua_State *L)
{
   static const char *methodName = "Robot:setAngleXY()";
   checkArgCount(L, 1, methodName);
   LuaPoint *point = Lunar<LuaPoint>::check(L, 1);

   Move move = thisRobot->getCurrentMove();
   move.angle = thisRobot->getAngleXY(point->getPoint());
   thisRobot->setCurrentMove(move);

   return 0;
}

// Get angle toward point XY
S32 LuaRobot::getAngleXY(lua_State *L)
{
   static const char *methodName = "Robot:getAngleXY()";
   checkArgCount(L, 1, methodName);
   LuaPoint *point = Lunar<LuaPoint>::check(L, 1);

   lua_pushnumber(L, thisRobot->getAngleXY(point->getPoint()));
   return 1;
}


// Thrust at velocity v toward angle a
S32 LuaRobot::setThrustAng(lua_State *L)
{
   static const char *methodName = "Robot:setThrustAng()";
   checkArgCount(L, 2, methodName);
   F32 vel = getFloat(L, 1, methodName);
   F32 ang = getFloat(L, 2, methodName);

   Move move;

   move.up = sin(ang) <= 0 ? -vel * sin(ang) : 0;
   move.down = sin(ang) > 0 ? vel * sin(ang) : 0;
   move.right = cos(ang) >= 0 ? vel * cos(ang) : 0;
   move.left = cos(ang) < 0 ? -vel * cos(ang) : 0;

   thisRobot->setCurrentMove(move);

   return 0;
}


template <class T>
T *checkItem(lua_State *L)
{
   luaL_getmetatable(L, T::className);
   if(lua_rawequal(L, -1, -2))         // Lua object on the stack is of class <T>!
   {
      lua_pop(L, 2);                   // Remove both metatables
      return Lunar<T>::check(L, 1);    // Return our object
   }
   else                                // Object on stack is something else
   {
      lua_pop(L, 1);    // Remove <T>'s metatable, leave the other in place for further comparison
      return NULL;
   }
}


extern bool FindLowestRootInInterval(Point::member_type inA, Point::member_type inB, Point::member_type inC, Point::member_type inUpperBound, Point::member_type &outX);

// Given an object, which angle do we need to be at to fire to hit it?
// Returns nil if a workable solution can't be found
// Logic adapted from turret aiming algorithm
S32 LuaRobot::getFiringSolution(lua_State *L) 
{
   Item *target; 


   lua_getmetatable(L, 1);    // Get metatable for first item on the stack

   target = checkItem<Asteroid>(L);

   if(!target)
      target = checkItem<TestItem>(L);


   if(!target)    // Ultimately failed to figure out what this object is.
   {
      lua_pop(L, 1);                      // Clean up
      luaL_typerror(L, 1, "GameItem");    // Raise an error
      return returnNil(L);                // Return nil, but I don't think this will ever get run
   }

   Point aimPos = thisRobot->getActualPos(); 
   Point offset = target->getActualPos() - aimPos;    // Account for fact that robot doesn't fire from center
   offset.normalize(thisRobot->getRadius() * 1.2);    // 1.2 is a fudge factor to prevent robot from not shooting because it thinks it will hit itself
   aimPos += offset;

   if(target->getObjectTypeMask() & ( ShipType | RobotType))
   {
      Ship *potential = (Ship*)target;

      // Is it dead or cloaked?  If so, ignore
      if((potential->isModuleActive(ModuleCloak) && !potential->areItemsMounted()) || potential->hasExploded)
         return returnNil(L);
   }

   if(target->getTeam() == thisRobot->getTeam())      // Is target on our team?
      return returnNil(L);                            // ...if so, skip it!

   // Calculate where we have to shoot to hit this...
   Point Vs = target->getActualVel();

   WeaponInfo weap = gWeapons[thisRobot->getSelectedWeapon()];    // Robot's active weapon

   F32 S = weap.projVelocity;
   Point d = target->getRenderPos() - aimPos;

   F32 t;      // t is set in next statement
   if(!FindLowestRootInInterval(Vs.dot(Vs) - S * S, 2 * Vs.dot(d), d.dot(d), weap.projLiveTime * 0.001f, t))
      return returnNil(L);

   Point leadPos = target->getRenderPos() + Vs * t;

   // Calculate distance
   Point delta = (leadPos - aimPos);

   // Make sure we can see it...
   Point n;
   if(target->findObjectLOS(BarrierType, MoveObject::ActualState, aimPos, target->getActualPos(), t, n))
      return returnNil(L);

   // See if we're gonna clobber our own stuff...
   target->disableCollision();
   Point delta2 = delta;
   delta2.normalize(weap.projLiveTime * weap.projVelocity / 1000);
   GameObject *hitObject = target->findObjectLOS(ShipType | RobotType | BarrierType | EngineeredType, 0, aimPos, aimPos + delta2, t, n);
   target->enableCollision();

   if(hitObject && hitObject->getTeam() == thisRobot->getTeam())
      return returnNil(L);

   return returnFloat(L, delta.ATAN2());
}


// Thrust at velocity v toward point x,y
S32 LuaRobot::setThrustXY(lua_State *L)
{
   static const char *methodName = "Robot:setThrustXY()";
   checkArgCount(L, 2, methodName);
   F32 vel = getFloat(L, 1, methodName);
   LuaPoint *point = Lunar<LuaPoint>::check(L, 2);

   F32 ang = thisRobot->getAngleXY(point->getPoint()) - 0 * FloatHalfPi;

   Move move = thisRobot->getCurrentMove();

   move.up = sin(ang) < 0 ? -vel * sin(ang) : 0;
   move.down = sin(ang) > 0 ? vel * sin(ang) : 0;
   move.right = cos(ang) > 0 ? vel * cos(ang) : 0;
   move.left = cos(ang) < 0 ? -vel * cos(ang) : 0;

   thisRobot->setCurrentMove(move);

  return 0;
}


extern Vector<SafePtr<BotNavMeshZone>> gBotNavMeshZones;

// Get the coords of the center of mesh zone z
S32 LuaRobot::getZoneCenter(lua_State *L)
{
   static const char *methodName = "Robot:getZoneCenter()";
   checkArgCount(L, 1, methodName);
   S32 z = getInt(L, 1, methodName);

   // In case this gets called too early...
   if(gBotNavMeshZones.size() == 0)
      return returnNil(L);

   // Bounds checking...
   if(z < 0 || z >= gBotNavMeshZones.size())
      return returnNil(L);

   return returnPoint(L, gBotNavMeshZones[z]->getCenter());
}


// Get the coords of the gateway to the specified zone.  Returns point, nil if requested zone doesn't border current zone.
S32 LuaRobot::getGatewayFromZoneToZone(lua_State *L)
{
   static const char *methodName = "Robot:getGatewayFromZoneToZone()";
   checkArgCount(L, 2, methodName);
   S32 from = getInt(L, 1, methodName);
   S32 to = getInt(L, 2, methodName);

   // In case this gets called too early...
   if(gBotNavMeshZones.size() == 0)
      return returnNil(L);

   // Bounds checking...
   if(from < 0 || from >= gBotNavMeshZones.size() || to < 0 || to >= gBotNavMeshZones.size())
      return returnNil(L);

   // Is requested zone a neighbor?
   for(S32 i = 0; i < gBotNavMeshZones[from]->mNeighbors.size(); i++)
   {
      if(gBotNavMeshZones[from]->mNeighbors[i].zoneID == to)
      {
         Rect r(gBotNavMeshZones[from]->mNeighbors[i].borderStart, gBotNavMeshZones[from]->mNeighbors[i].borderEnd);
         return returnPoint(L, r.getCenter());
      }
   }

   // Did not find requested neighbor... returning nil
   return returnNil(L);
}


// Get the zone this robot is currently in.  If not in a zone, return nil
S32 LuaRobot::getCurrentZone(lua_State *L)
{
   S32 zone = thisRobot->getCurrentZone();
   return (zone == -1) ? returnNil(L) : returnInt(L, zone);
}

// Get a count of how many nav zones we have
S32 LuaRobot::getZoneCount(lua_State *L)
{
   return returnInt(L, gBotNavMeshZones.size());
}


// Fire current weapon if possible
S32 LuaRobot::fire(lua_State *L)
{
   Move move = thisRobot->getCurrentMove();
   move.fire = true;
   thisRobot->setCurrentMove(move);

   return 0;
}


// Can robot see point XY?
S32 LuaRobot::hasLosXY(lua_State *L)
{
   static const char *methodName = "Robot:hasLosXY()";
   checkArgCount(L, 1, methodName);
   LuaPoint *point = Lunar<LuaPoint>::check(L, 1);

   return returnBool(L, thisRobot->canSeePoint(point->getPoint()));
}


// Does robot have a flag?
S32 LuaRobot::hasFlag(lua_State *L)
{
   return returnBool(L, (thisRobot->carryingFlag() != GameType::NO_FLAG));
}


// Set weapon to index
S32 LuaRobot::setWeaponIndex(lua_State *L)
{
   static const char *methodName = "Robot:setWeaponIndex()";
   checkArgCount(L, 1, methodName);
   U32 weap = getInt(L, 1, methodName, 1, ShipWeaponCount);    // Acceptable range = (1, ShipWeaponCount)
   thisRobot->selectWeapon(weap - 1);     // Correct for the fact that index in C++ is 0 based

   return 0;
}


// Set weapon to specified weapon, if we have it
S32 LuaRobot::setWeapon(lua_State *L)
{
   static const char *methodName = "Robot:setWeapon()";
   checkArgCount(L, 1, methodName);
   U32 weap = getInt(L, 1, methodName, 0, WeaponCount - 1);

   for(S32 i = 0; i < ShipWeaponCount; i++)
      if(thisRobot->getWeapon(i) == weap)
      {
         thisRobot->selectWeapon(i);
         break;
      }

   // If we get here without having found our weapon, then nothing happens.  Better luck next time!

   return 0;
}


// Do we have a given weapon in our current loadout?
S32 LuaRobot::hasWeapon(lua_State *L)
{
   static const char *methodName = "Robot:hasWeapon()";
   checkArgCount(L, 1, methodName);
   U32 weap = getInt(L, 1, methodName, 0, WeaponCount - 1);

   for(S32 i = 0; i < ShipWeaponCount; i++)
      if(thisRobot->getWeapon(i) == weap)
         return returnBool(L, true);      // We have it!

   return returnBool(L, false);           // We don't!
}



/////////////////////////////

// Activate module this cycle --> takes module index
S32 LuaRobot::activateModuleIndex(lua_State *L)
{
   static const char *methodName = "Robot:activateModuleIndex()";

   checkArgCount(L, 1, methodName);
   U32 indx = getInt(L, 1, methodName, 0, ShipModuleCount);

   thisRobot->activateModule(indx);

   return 0;
}


// Activate module this cycle --> takes module enum.
// If specified module is not part of the loadout, does nothing.
S32 LuaRobot::activateModule(lua_State *L)
{
   static const char *methodName = "Robot:activateModule()";

   checkArgCount(L, 1, methodName);
   ShipModule mod = (ShipModule) getInt(L, 1, methodName, 0, ModuleCount - 1);

   for(S32 i = 0; i < ShipModuleCount; i++)
      if(thisRobot->getModule(i) == mod)
      {
         thisRobot->activateModule(i);
         break;
      }

   return 0;
}


// Sets loadout to specified --> takes 2 modules, 3 weapons
S32 LuaRobot::setReqLoadout(lua_State *L)
{
   checkArgCount(L, 1, "Robot:setReqLoadout()");

   LuaLoadout *loadout = Lunar<LuaLoadout>::check(L, 1);
   Vector<U32> vec;

   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      vec.push_back(loadout->getLoadoutItem(i));

   thisRobot->setLoadout(vec);

   return 0;
}


// Return current loadout
S32 LuaRobot::getCurrLoadout(lua_State *L)
{
   U32 loadoutItems[ShipModuleCount + ShipWeaponCount];

   for(S32 i = 0; i < ShipModuleCount; i++)
      loadoutItems[i] = (U32) thisRobot->getModule(i);

   for(S32 i = 0; i < ShipWeaponCount; i++)
      loadoutItems[i + ShipModuleCount] = (U32) thisRobot->getWeapon(i);

   LuaLoadout *loadout = new LuaLoadout(loadoutItems);
   Lunar<LuaLoadout>::push(L, loadout, false);     // true will allow Lua to delete this object when it goes out of scope

   return 1;
}


extern LoadoutItem gLoadoutModules[];
extern LoadoutItem gLoadoutWeapons[];

// Return requested loadout
S32 LuaRobot::getReqLoadout(lua_State *L)
{
   U32 loadoutItems[ShipModuleCount + ShipWeaponCount];

   for(S32 i = 0; i < ShipModuleCount; i++)
      loadoutItems[i] = (U32) gLoadoutModules[thisRobot->getModule(i)].index;

   for(S32 i = 0; i < ShipWeaponCount; i++)
      loadoutItems[i + ShipModuleCount] = (U32) gLoadoutWeapons[thisRobot->getWeapon(i)].index;

   LuaLoadout *loadout = new LuaLoadout(loadoutItems);
   Lunar<LuaLoadout>::push(L, loadout, true);     // true will allow Lua to delete this object when it goes out of scope

   return 1;
}


// Get WeaponIndex for current weapon
S32 LuaRobot::getWeapon(lua_State *L)
{
   return returnInt(L, thisRobot->getSelectedWeapon());
}


// Send message to all players
S32 LuaRobot::globalMsg(lua_State *L)
{
   static const char *methodName = "Robot:globalMsg()";
   checkArgCount(L, 1, methodName);

   GameType *gt = gServerGame->getGameType();
   if(gt)
      gt->s2cDisplayChatMessage(true, thisRobot->getName(), getString(L, 1, methodName));
   return 0;
}


// Send message to team (what happens when neutral/enemytoall robot does this???)
S32 LuaRobot::teamMsg(lua_State *L)
{
   static const char *methodName = "Robot:teamMsg()";
   checkArgCount(L, 1, methodName);

   GameType *gt = gServerGame->getGameType();
   if(gt)
      gt->s2cDisplayChatMessage(false, thisRobot->getName(), getString(L, 1, methodName));
   return 0;
}


// Returns current aim angle of ship  --> needed?
S32 LuaRobot::getAngle(lua_State *L)
{
  return returnFloat(L, thisRobot->getCurrentMove().angle);
}


// Returns current position of ship
S32 LuaRobot::getLoc(lua_State *L)
{
  return returnPoint(L, thisRobot->getActualPos());
}


// Write a message to the server logfile
S32 LuaRobot::logprint(lua_State *L)
{
   static const char *methodName = "Robot:logprint()";
   checkArgCount(L, 1, methodName);

   logprintf("RobotLog %s: %s", thisRobot->getName().getString(), getString(L, 1, methodName));
   return 0;
}


// Return list of all items of specified type within normal visible range... does no screening at this point
S32 LuaRobot::findItems(lua_State *L)
{
   Point pos = thisRobot->getActualPos();
   Rect queryRect(pos, pos);
   queryRect.expand( gServerGame->computePlayerVisArea(thisRobot) );

   return doFindItems( L, queryRect );
}


extern Rect gServerWorldBounds;

// Same but gets all visible items from whole game... out-of-scope items will be ignored
S32 LuaRobot::findGlobalItems(lua_State *L)
{
   return doFindItems(L, gServerWorldBounds);
}


S32 LuaRobot::doFindItems(lua_State *L, Rect scope)
{
   // objectType is a bitmask of all the different object types we might want to find.  We need to build it up here because
   // lua can't do the bitwise or'ing itself.
   U32 objectType = 0;

   S32 index = 1;
   while(lua_isnumber(L, index))
   {
      objectType |= lua_tointeger(L, index);
      index++;
   }


   fillVector.clear();

   thisRobot->findObjects(objectType, fillVector, scope);    // Get other objects on screen-visible area only

   lua_createtable(L, fillVector.size(), 0);    // Create a table, with enough slots pre-allocated for our data
   S32 tableIndx = lua_gettop(L);

   for(S32 i = 0; i < fillVector.size(); i++)
   { 
      if(fillVector[i]->getObjectTypeMask() & (ShipType | RobotType))      // Skip cloaked ships & robots!
      {
         Ship *ship = (Ship*)fillVector[i];

         // If it's dead or cloaked, ignore it
         if((ship->isModuleActive(ModuleCloak) && !ship->areItemsMounted()) || ship->hasExploded)
            continue;
      }

      GameObject *obj = fillVector[i];
      obj->push(L);
      lua_rawseti(L, tableIndx, i + 1);
   }

   return 1;
}



// Get rid of??
S32 LuaRobot::findObjects(lua_State *L)
{
   //LuaProtectStack x(this); <== good idea, not working right...  ;-(

    static const char *methodName = "findObjects";

   checkArgCount(L, 1, methodName);
   U32 objectType = getInt(L, 1, methodName);

   fillVector.clear();

   // ShipType BarrierType MoveableType BulletType ItemType ResourceItemType EngineeredType ForceFieldType LoadoutZoneType MineType TestItemType FlagType TurretTargetType SlipZoneType HeatSeekerType SpyBugType NexusType

   // thisRobot->findObjects(CommandMapVisType, fillVector, gServerWorldBounds);    // Get all globally visible objects
   //thisRobot->findObjects(ShipType, fillVector, Rect(thisRobot->getActualPos(), gServerGame->computePlayerVisArea(thisRobot)) );    // Get other objects on screen-visible area only
   thisRobot->findObjects(objectType, fillVector, gServerWorldBounds );    // Get other objects on screen-visible area only

   F32 bestRange = F32_MAX;
   Point bestPoint;

   GameType *gt = gServerGame->getGameType();
   TNLAssert(gt, "Invaid game type!!");

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      // Some special rules for narrowing in on the objects we really want
      if(fillVector[i]->getObjectTypeMask() & ShipType)
      {
         Ship *ship = (Ship*)fillVector[i];

         // If it's dead or cloaked, ignore it
         if((ship->isModuleActive(ModuleCloak) && !ship->areItemsMounted()) || ship->hasExploded)
            continue;
      }
      else if(fillVector[i]->getObjectTypeMask() & FlagType)
      {
         FlagItem *flag = (FlagItem*)fillVector[i];

         // Ignore flags mounted on ships
         if(flag->isMounted())
            continue;

         TNLAssert(gServerGame->getGameType(), "Need to add gameType check in flagCheck/findObjects");

         // Is flag in a zone?  Which kind?
         bool isInZone = flag->getZone();
         bool isInOwnZone = isInZone && flag->getZone()->getTeam() == thisRobot->getTeam();
         bool isNotInOwnZone = !isInOwnZone;
         bool isInEnemyZone = isInZone && !isInOwnZone;
         bool isInLeaderZone = isInZone && flag->getZone()->getTeam() == gServerGame->getGameType()->mLeadingTeam;
         bool isInNoZone = !isInZone;
         //-----
         //closest
         //-----


         if(!isNotInOwnZone)
            continue;

      }
      else if(fillVector[i]->getObjectTypeMask() & GoalZoneType)
      {
         TNLAssert(gServerGame->getGameType(), "Need to add gameType check in goalZoneCheck/findObjects");

         GoalZone *goal = (GoalZone*)fillVector[i];

         bool isOwnZone = goal->getTeam() == thisRobot->getTeam();
         bool isNeutralZone = goal->getTeam() == -1;
         bool isEnemyZone = !(isOwnZone || isNeutralZone);
         bool isNotOwnZone = !isOwnZone;
         bool isLeaderZone = goal->getTeam() == gServerGame->getGameType()->mLeadingTeam;
         //-----
         bool hasFlag = goal->mHasFlag;
         bool hasNoFlag = !hasFlag;
         //-----
         //closest
         //-----

         // Find only own goalzones
         if(!isOwnZone)
            continue;

         // For now, ignore goalzones with flags in them
         if(!hasNoFlag)
            continue;
      }

      GameObject *potential = fillVector[i];

      // If object needs to be in LOS, uncomment following
      //if( ! thisRobot->canSeePoint(potential->getActualPos()) )   // Can we see it?
      //   continue;      // No

      F32 dist = thisRobot->getActualPos().distanceTo(potential->getActualPos());

      if(dist < bestRange)
      {
         bestPoint  = potential->getActualPos();
         bestRange  = dist;
      }
   }

   // Write the results to Lua
   if(bestRange < F32_MAX)
      return returnPoint(L, bestPoint);

   // else no targets found
   return returnNil(L);
}


extern S32 findZoneContaining(Point p);


// Get next waypoint to head toward when traveling from current location to x,y
// Note that this function will be called frequently by various robots, so any
// optimizations will be helpful.
S32 LuaRobot::getWaypoint(lua_State *L)
{
   static const char *methodName = "Robot:getWaypoint()";
   checkArgCount(L, 1, methodName);
   LuaPoint *target = Lunar<LuaPoint>::check(L, 1);

   S32 targetZone = findZoneContaining(target->getPoint());       // Where we're going

   // Make sure target is still in the same zone it was in when we created our flightplan.
   // If we're not, our flightplan is invalid, and we need to skip forward and build a fresh one.
   if(targetZone == thisRobot->flightPlanTo)
   {

      // In case our target has moved, replace final point of our flightplan with the current target location
      thisRobot->flightPlan[0] = target->getPoint();

      // First, let's scan through our pre-calculated waypoints and see if we can see any of them.
      // If so, we'll just head there with no further rigamarole.  Remember that our flightplan is
      // arranged so the closest points are at the end of the list, and the target is at index 0.
      Point dest;
      bool found = false;
      bool first = true;

      while(thisRobot->flightPlan.size() > 0)
      {
         Point last = thisRobot->flightPlan.last();

         // We'll assume that if we could see the point on the previous turn, we can
         // still see it, even though in some cases, the turning of the ship around a
         // protruding corner may make it technically not visible.  This will prevent
         // rapidfire recalcuation of the path when it's not really necessary.
         if(first || thisRobot->canSeePoint(last))
         {
            dest = last;
            found = true;
            first = false;
            thisRobot->flightPlan.pop_back();   // Discard now possibly superfluous waypoint
         }
         else
            break;
      }

      // If we found one, that means we found a visible waypoint, and we can head there...
      if(found)
      {
         thisRobot->flightPlan.push_back(dest);    // Put dest back at the end of the flightplan
         return returnPoint(L, dest);
      }
   }

   // We need to calculate a new flightplan
   thisRobot->flightPlan.clear();

   S32 currentZone = thisRobot->getCurrentZone();     // Where we are
   if(currentZone == -1)      // We don't really know where we are... bad news!  Let's find closest visible zone and go that way.
      //return findAndReturnClosestZone(L, thisRobot->getActualPos());
      return returnNil(L);

   if(targetZone == -1)       // Our target is off the map.  See if it's visible from any of our zones, and, if so, go there
      //return findAndReturnClosestZone(L, target);
      return returnNil(L);

   // We're in, or on the cusp of, the zone containing our target.  We're close!!
   if(currentZone == targetZone)
   {
      Point p;
      if(!thisRobot->canSeePoint(target->getPoint()))    // Possible, if we're just on a boundary, and a protrusion's blocking a ship edge
      {
         p = gBotNavMeshZones[targetZone]->getCenter();
         thisRobot->flightPlan.push_back(p);
      }
      else
         p = target->getPoint();

      thisRobot->flightPlan.push_back(target->getPoint());
      return returnPoint(L, p);
   }

   // If we're still here, then we need to find a new path.  Either our original path was invalid for some reason,
   // or the path we had no longer applied to our current location
   thisRobot->flightPlanTo = targetZone;
   thisRobot->flightPlan = AStar::findPath(currentZone, targetZone, target->getPoint());

   if(thisRobot->flightPlan.size() > 0)
      return returnPoint(L, thisRobot->flightPlan.last());
   else
      return returnNil(L);    // Out of options, end of the road
}
//
//
//// Encapsulate some ugliness
//Point LuaRobot::getNextWaypoint()
//{
//   TNLAssert(thisRobot->flightPlan.size() > 1, "FlightPlan has too few zones!");
//
//   S32 currentzone = thisRobot->getCurrentZone();
//
//   S32 nextzone = thisRobot->flightPlan[thisRobot->flightPlan.size() - 2];
//
//   // Note that getNeighborIndex could return -1.  It shouldn't, but it could.
//   S32 neighborZoneIndex = gBotNavMeshZones[currentzone]->getNeighborIndex(nextzone);
//   TNLAssert(neighborZoneIndex >= 0, "Invalid neighbor zone index!");
//
//
//            /*   string plan = "";
//               for(S32 i = 0; i < thisRobot->flightPlan.size(); i++)
//               {
//                  char z[100];
//                  itoa(thisRobot->flightPlan[i], z, 10);
//                  plan = plan + " ==>"+z;
//               }*/
//
//
//   S32 i = 0;
//   Point currentwaypoint = gBotNavMeshZones[currentzone]->getCenter();
//   Point nextwaypoint = gBotNavMeshZones[currentzone]->mNeighbors[neighborZoneIndex].borderCenter;
//
//   while(thisRobot->canSeePoint(nextwaypoint))
//   {
//      currentzone = nextzone;
//      currentwaypoint = nextwaypoint;
//      i++;
//
//      if(thisRobot->flightPlan.size() - 2 - i < 0)
//         return nextwaypoint;
//
//      nextzone = thisRobot->flightPlan[thisRobot->flightPlan.size() - 2 - i];
//      S32 neighborZoneIndex = gBotNavMeshZones[currentzone]->getNeighborIndex(nextzone);
//      TNLAssert(neighborZoneIndex >= 0, "Invalid neighbor zone index!");
//
//      nextwaypoint = gBotNavMeshZones[currentzone]->mNeighbors[neighborZoneIndex].borderCenter;
//   }
//
//
//   return currentwaypoint;
//
//
//
//   //if(thisRobot->canSeePoint( gBotNavMeshZones[currentZone]->mNeighbors[neighborZoneIndex].center))
//   //   return gBotNavMeshZones[currentZone]->mNeighbors[neighborZoneIndex].center;
//   //else if(thisRobot->canSeePoint( gBotNavMeshZones[currentZone]->mNeighbors[neighborZoneIndex].borderCenter))
//   //   return gBotNavMeshZones[currentZone]->mNeighbors[neighborZoneIndex].borderCenter;
//   //else
//   //   return gBotNavMeshZones[currentZone]->getCenter();
//}
//

// Another helper function: finds closest zone to a given point
S32 LuaRobot::findClosestZone(Point point)
{
   F32 distsq = F32_MAX;
   S32 closest = -1;

   for(S32 i = 0; i < gBotNavMeshZones.size(); i++)
   {
      Point center = gBotNavMeshZones[i]->getCenter();

      if( gServerGame->getGridDatabase()->pointCanSeePoint(center, point) )
      {
         F32 d = center.distSquared(point);
         if(d < distsq)
         {
            closest = i;
            distsq = d;
         }
      }
   }

   return closest;
}


S32 LuaRobot::findAndReturnClosestZone(lua_State *L, Point point)
{
   S32 closest = findClosestZone(point);

   if(closest != -1)
      return returnPoint(L, gBotNavMeshZones[closest]->getCenter());
   else
      return returnNil(L);    // Really stuck.
}


const char LuaRobot::className[] = "LuaRobot";     // This is the class name as it appears to the Lua scripts


//------------------------------------------------------------------------

TNL_IMPLEMENT_NETOBJECT(Robot);


U32 Robot::mRobotCount = 0;

// Constructor
Robot::Robot(StringTableEntry robotName, S32 team, Point p, F32 m) : Ship(robotName, team, p, m)
{
   mObjectTypeMask = RobotType | MoveableType | CommandMapVisType | TurretTargetType;

   S32 junk = this->mCurrentZone;
   mNetFlags.set(Ghostable);

   mTeam = team;
   mass = m;            // Ship's mass
   L = NULL;
   mCurrentZone = -1;
   flightPlanTo = -1;

   // Need to provide some time on here to get timer to trigger robot to spawn.  It's timer driven.
   respawnTimer.reset(100, RobotRespawnDelay);

   hasExploded = true;     // Becase we start off "dead", but will respawn real soon now...

   disableCollision();
}


Robot::~Robot()
{
   if(getGame() && getGame()->isServer())
      mRobotCount--;

   // Close down our Lua interpreter
   if(L)
      lua_close(L);

   logprintf("Robot terminated [%s]", mFilename.c_str());
}


// Reset everything on the robot back to the factory settings
bool Robot::initialize(Point p)
{
   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].pos = p;
      mMoveState[i].angle = 0;
      mMoveState[i].vel = Point(0,0);
   }
   updateExtent();

   respawnTimer.clear();

   mHealth = 1.0;       // Start at full health
   hasExploded = false; // Haven't exploded yet!

   mCurrentZone = -1;   // Correct value will be calculated upon first request

   for(S32 i = 0; i < TrailCount; i++)          // Clear any vehicle trails
      mTrail[i].reset();

   mEnergy = (S32) ((F32) EnergyMax * .80);     // Start off with 80% energy
   for(S32 i = 0; i < ModuleCount; i++)         // and all modules disabled
      mModuleActive[i] = false;

   // Set initial module and weapon selections
   for(S32 i = 0; i < ShipModuleCount; i++)
      mModule[i] = (ShipModule) DefaultLoadout[i];

   for(S32 i = 0; i < ShipWeaponCount; i++)
      mWeapon[i] = (WeaponType) DefaultLoadout[i + ShipModuleCount];

   hasExploded = false;
   enableCollision();

   mActiveWeaponIndx = 0;
   mCooldown = false;

   // WarpPositionMask triggers the spinny spawning visual effect
   setMaskBits(RespawnMask | HealthMask | LoadoutMask | PositionMask | MoveMask | PowersMask | WarpPositionMask);      // Send lots to the client

   if(L)
      lua_close(L);

   L = lua_open();    // Create a new Lua interpreter

   // Register our connector types with Lua
   Lunar<LuaRobot>::Register(L);
   Lunar<LuaGameInfo>::Register(L);
   Lunar<LuaTeamInfo>::Register(L);
   Lunar<LuaTimer>::Register(L);

   Lunar<LuaWeaponInfo>::Register(L);
   Lunar<LuaModuleInfo>::Register(L);

   Lunar<LuaLoadout>::Register(L);
   Lunar<LuaPoint>::Register(L);

   Lunar<RepairItem>::Register(L);
   Lunar<ResourceItem>::Register(L);
   Lunar<TestItem>::Register(L);
   Lunar<Asteroid>::Register(L);
   //Lunar<FlagItem>::Register(L);
   //Lunar<SoccerBallItem>::Register(L);
   //Lunar<HuntersFlagItem>::Register(L);

   luaopen_base(L);     // Make some basic functions available to bots

   // Push a pointer to this Robot to the Lua stack
   lua_pushlightuserdata(L, (void *)this);

   // And set the global name of this pointer.  This is the name that we'll use to refer
   // to our robot from our Lua code.
   lua_setglobal(L, "Robot");

   try
   {
      // Load the script
      S32 retCode = luaL_loadfile(L, mFilename.c_str());

      if(retCode)
      {
         logError("Error loading file:" + string(lua_tostring(L, -1)) + ".  Shutting robot down.");
         return false;
      }

      // Run main script body
      lua_pcall(L, 0, 0, 0);
   }
   catch(string e)
   {
      logError("Error initializing robot: " + e + ".  Shutting robot down.");
      return false;
   }

   try
   {
      lua_getglobal(L, "getName");
      lua_call(L, 0, 1);                  // Passing 0 params, getting one back
      mPlayerName = lua_tostring(L, -1);
      lua_pop(L, 1);
   }
   catch (string e)
   {
      logError("Robot error running getName(): " + e + ".  Shutting robot down.");
      return false;
   }

   return true;
}


void Robot::onAddedToGame(Game *)
{
   // Make them always visible on cmdr map --> del
   if(!isGhost())
      setScopeAlways();

   if(getGame() && getGame()->isServer())
      mRobotCount++;
}


// Basically exists to override Ship::kill(info)
 void Robot::kill(DamageInfo *theInfo)
{
   kill();
}


void Robot::kill()
{
   hasExploded = true;
   respawnTimer.reset();
   setMaskBits(ExplosionMask);

   disableCollision();

   // Dump mounted items
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
      mMountedItems[i]->onMountDestroyed();
}


bool Robot::processArguments(S32 argc, const char **argv)
{
   if(argc != 2)
      return false;

   mTeam = atoi(argv[0]);     // Need some sort of bounds check here??

   mFilename = "robots/";
   mFilename += argv[1];

   return true;
}


// Some rudimentary robot error logging.  Perhaps, someday, will become a sort of in-game error console.
// For now, though, pass all errors through here.
void Robot::logError(string err)
{
   logprintf("***ROBOT ERROR*** in %s ::: %s", mFilename.c_str(), err.c_str());
}


S32 Robot::getCurrentZone()
{
   // We're in uncharted territory -- try to get the current zone
   if(mCurrentZone == -1)
   {
      mCurrentZone = findZoneContaining(getActualPos());
   }
   return mCurrentZone;
}


// Setter method, not a robot function!
void Robot::setCurrentZone(S32 zone)
{
   mCurrentZone = zone;
}


F32 Robot::getAngleXY(Point point)
{
   return getActualPos().angleTo(point);
}


// Process a move.  This will advance the position of the ship, as well as adjust its velocity and angle.
void Robot::processMove(U32 stateIndex)
{
   mMoveState[LastProcessState] = mMoveState[stateIndex];

   F32 maxVel = getMaxVelocity();

   F32 time = mCurrentMove.time * 0.001;
   Point requestVel(mCurrentMove.right - mCurrentMove.left, mCurrentMove.down - mCurrentMove.up);

   requestVel *= maxVel;
   F32 len = requestVel.len();

   // Remove this block to allow robots to exceed speed of 1
   if(len > maxVel)
      requestVel *= maxVel / len;

   Point velDelta = requestVel - mMoveState[stateIndex].vel;
   F32 accRequested = velDelta.len();


   // Apply turbo-boost if active
   F32 maxAccel = (isModuleActive(ModuleBoost) ? BoostAcceleration : Acceleration) * time;
   if(accRequested > maxAccel)
   {
      velDelta *= maxAccel / accRequested;
      mMoveState[stateIndex].vel += velDelta;
   }
   else
      mMoveState[stateIndex].vel = requestVel;

   mMoveState[stateIndex].angle = mCurrentMove.angle;
   move(time, stateIndex, false);
}


extern bool PolygonContains2(const Point *inVertices, int inNumVertices, const Point &inPoint);

// Return coords of nearest ship... and experimental robot routine
bool Robot::findNearestShip(Point &loc)
{
   Vector<GameObject *> foundObjects;
   Point closest;

   Point pos = getActualPos();
   Point extend(2000, 2000);
   Rect r(pos - extend, pos + extend);

   findObjects(ShipType, foundObjects, r);

   if(!foundObjects.size())
      return false;

   F32 dist = F32_MAX;
   bool found = false;

   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      F32 d = foundObjects[i]->getActualPos().distanceTo(pos);
      if(d < dist && d > 0)      // d == 0 for self
      {
         dist = d;
         loc = foundObjects[i]->getActualPos();
         found = true;
      }
   }
   return found;
}


bool Robot::canSeePoint(Point point)
{
   // Need to check the two edge points perpendicular to the direction of looking to ensure we have an unobstructed
   // flight lane to point.  Radius of the robot is mRadius.  This keeps the ship from getting hung up on
   // obstacles that appear visible from the center of the ship, but are actually blocked.

   F32 ang = getActualPos().angleTo(point);
   F32 cosang = cos(ang) * mRadius;
   F32 sinang = sin(ang) * mRadius;

   Point edgePoint1 = getActualPos() + Point(sinang, -cosang);
   Point edgePoint2 = getActualPos() + Point(-sinang, cosang);

   return(
      gServerGame->getGridDatabase()->pointCanSeePoint(edgePoint1, point) &&
      gServerGame->getGridDatabase()->pointCanSeePoint(edgePoint2, point) );
}


void Robot::idle(GameObject::IdleCallPath path)
{
   U32 deltaT;

   if(path == GameObject::ServerIdleMainLoop)
   {
      U32 ms = Platform::getRealMilliseconds();
      deltaT = (ms - mLastMoveTime);
      mLastMoveTime = ms;
      mCurrentMove.time = deltaT;

      // Check to see if we need to respawn this robot
      if(hasExploded)
      {
         if(respawnTimer.update(mCurrentMove.time))
            gServerGame->getGameType()->spawnRobot(this);

         return;
      }
   }

   // Don't process exploded ships
   if(hasExploded)
      return;

   if(path == GameObject::ServerIdleMainLoop)      // Running on server
   {
      // Clear out current move.  It will get set just below with the lua call, but if that function
      // doesn't set the various move components, we want to make sure that they default to 0.
      mCurrentMove.fire = false;
      mCurrentMove.up = 0;
      mCurrentMove.down = 0;
      mCurrentMove.right = 0;
      mCurrentMove.left = 0;

      try
      {
         lua_getglobal(L, "getMove");
         lua_call(L, 0, 0);
      }
      catch (string e)
      {
         logError("Robot error running getMove(): " + e + ".  Shutting robot down.");
         delete this;
         return;
      }
      catch (...)    // Catches any other errors not caught above --> should never happen
      {
         logError("Robot error: Unknown exception.  Shutting robot down");
         delete this;
         return;
      }


      // If we've changed the mCurrentMove, then we need to set
      // the MoveMask to ensure that it is sent to the clients
      setMaskBits(MoveMask);

      processMove(ActualState);

      // Apply impulse vector and reset it
      mMoveState[ActualState].vel += mImpulseVector;
      mImpulseVector.set(0,0);

      // Update the render state on the server to match
      // the actual updated state, and mark the object
      // as having changed Position state.  An optimization
      // here would check the before and after positions
      // so as to not update unmoving ships.

      mMoveState[RenderState] = mMoveState[ActualState];
      setMaskBits(PositionMask);
   }
   else if(path == GameObject::ClientIdleMainRemote)  // Running on client (but not replaying saved game)
   {
      // On the client, update the interpolation of this object, unless we are replaying control moves
      mInterpolating = (getActualVel().lenSquared() < MoveObject::InterpMaxVelocity*MoveObject::InterpMaxVelocity);
      updateInterpolation();
   }

   updateExtent();            // Update the object in the game's extents database
   mLastMove = mCurrentMove;  // Save current move

   // Update module timers
   mSensorZoomTimer.update(mCurrentMove.time);
   mCloakTimer.update(mCurrentMove.time);

   //bool engineerWasActive = isModuleActive(ModuleEngineer);

   if(path == GameObject::ServerIdleMainLoop)    // Was ClientIdleControlReplay
   {
      // Process weapons and energy on controlled object objects
      processWeaponFire();
      processEnergy();
   }

   if(path == GameObject::ClientIdleMainRemote)       // Probably should be server
   {
      // For ghosts, find some repair targets for rendering the repair effect
      if(isModuleActive(ModuleRepair))
         findRepairTargets();
   }

   if(false && isModuleActive(ModuleRepair))     // Probably should be server
      repairTargets();

   // If we're on the client, do some effects
   if(path == GameObject::ClientIdleMainRemote)
   {
      mWarpInTimer.update(mCurrentMove.time);
      // Emit some particles, trail sections and update the turbo noise
      emitMovementSparks();
      for(U32 i=0; i<TrailCount; i++)
         mTrail[i].tick(mCurrentMove.time);
      updateModuleSounds();
   }
}


void Robot::render(S32 layerIndex)
{
   Parent::render(layerIndex);

//   GameObject *controlObject = gClientGame->getConnectionToServer()->getControlObject();
//   Ship *u = dynamic_cast<Ship *>(controlObject);      // This is the local player's ship
//
//   Point position = u->getRenderPos();
//
//   F32 zoomFrac = 1;
//
//glPopMatrix();
//
//      // Set up the view to show the whole level.
//   Rect mWorldBounds = gClientGame->computeWorldObjectExtents(); // TODO: Cache this value!  ?  Or not?
//
//   Point worldCenter = mWorldBounds.getCenter();
//   Point worldExtents = mWorldBounds.getExtents();
//   worldExtents.x *= UserInterface::canvasWidth / F32(UserInterface::canvasWidth - (UserInterface::horizMargin * 2));
//   worldExtents.y *= UserInterface::canvasHeight / F32(UserInterface::canvasHeight - (UserInterface::vertMargin * 2));
//
//   F32 aspectRatio = worldExtents.x / worldExtents.y;
//   F32 screenAspectRatio = UserInterface::canvasWidth / F32(UserInterface::canvasHeight);
//   if(aspectRatio > screenAspectRatio)
//      worldExtents.y *= aspectRatio / screenAspectRatio;
//   else
//      worldExtents.x *= screenAspectRatio / aspectRatio;
//
   //Point offset = (worldCenter - position) * zoomFrac + position;
//   Point visSize = gClientGame->computePlayerVisArea(u) * 2;
//   Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;
//
   //Point visScale(UserInterface::canvasWidth / modVisSize.x,
                  //UserInterface::canvasHeight / modVisSize.y );

   //glPushMatrix();
   //glTranslatef(gScreenWidth / 2, gScreenHeight / 2, 0);    // Put (0,0) at the center of the screen

   //glScalef(visScale.x, visScale.y, 1);
   //glTranslatef(-offset.x, -offset.y, 0);

glColor3f(1,1,1);
   S32 xx = this->flightPlanTo;
   glBegin(GL_LINE_STRIP);
   for(S32 i = 0; i < flightPlan.size(); i++)
   {
      glVertex2f(flightPlan[i].x, flightPlan[i].y);
   }
   glEnd();

   //glPopMatrix();
}


};
