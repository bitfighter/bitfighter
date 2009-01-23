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

#include "robot.h"
#include "item.h"

#include "sparkManager.h"
#include "projectile.h"
#include "gameLoader.h"
#include "sfx.h"
#include "UI.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "gameType.h"
#include "gameConnection.h"
#include "shipItems.h"
#include "gameWeapons.h"
#include "gameObjectRender.h"
#include "config.h"
#include "BotNavMeshZone.h"      // For BotNavMeshZone class definition
#include "../glut/glutInclude.h"


#include <stdio.h>   // For what?

#define hypot _hypot    // Kill some warnings

namespace Zap
{

static Vector<GameObject *> fillVector;


// Constructor
//LuaGameObject::LuaGameObject(lua_State *L){
//   thisRobot = (Robot*)lua_touserdata(L, 1);
//}
//
//void LuaGameObject::setObject(lua_State *L){
//  thisRobot = (Robot*)lua_touserdata(L, 1);
//}


//// Turn to angle a (in radians)
//S32 LuaGameObject::setAngle(lua_State *L)
//{
//   if(!lua_isnil(L, 1))
//      thisRobot->mCurrentMove.angle = luaL_checknumber(L, 1);
//   return 0;
//}
//
//// Turn towards point XY
//S32 LuaGameObject::setAngleXY(lua_State *L)
//{
//   if(lua_isnil(L, 1) || lua_isnil(L, 2))
//      return 0;
//
//   F32 x = luaL_checknumber(L, 1);
//   F32 y = luaL_checknumber(L, 2);
//   thisRobot->mCurrentMove.angle = thisRobot->getAngleXY(x, y);
//   return 0;
//}
//
//// Get angle toward point XY
//S32 LuaGameObject::getAngleXY(lua_State *L)
//{
//   F32 x = luaL_checknumber(L, 1);
//   F32 y = luaL_checknumber(L, 2);
//   lua_pushnumber(L, thisRobot->getAngleXY(x, y));
//   return 1;
//}
//
//
//// Thrust at velocity v toward angle a
//S32 LuaGameObject::setThrustAng(lua_State *L)
//{
//   F32 vel, ang;
//
//   if(lua_isnil(L, 1) || lua_isnil(L, 2))
//   {
//      vel = 0;
//      ang = 0;
//   }
//   else
//   {
//      vel = luaL_checknumber(L, 1);
//      ang = luaL_checknumber(L, 2);
//   }
//
//   thisRobot->mCurrentMove.up = sin(ang) <= 0 ? -vel * sin(ang) : 0;
//   thisRobot->mCurrentMove.down = sin(ang) > 0 ? vel * sin(ang) : 0;
//   thisRobot->mCurrentMove.right = cos(ang) >= 0 ? vel * cos(ang) : 0;
//   thisRobot->mCurrentMove.left = cos(ang) < 0 ? -vel * cos(ang) : 0;
//
//  return 0;
//}
//
//
//
//// Thrust at velocity v toward point x,y
//S32 LuaGameObject::setThrustXY(lua_State *L)
//{
//   F32 vel, ang;
//
//   if(lua_isnil(L, 1) || lua_isnil(L, 2) || lua_isnil(L, 3))
//   {
//      vel = 0;
//      ang = 0;
//   }
//   else
//   {
//      vel = luaL_checknumber(L, 1);
//      F32 x = luaL_checknumber(L, 2);
//      F32 y = luaL_checknumber(L, 3);
//
//      ang = thisRobot->getAngleXY(x,y) - 0 * FloatHalfPi;
//   }
//
//   thisRobot->mCurrentMove.up = sin(ang) < 0 ? -vel * sin(ang) : 0;
//   thisRobot->mCurrentMove.down = sin(ang) > 0 ? vel * sin(ang) : 0;
//   thisRobot->mCurrentMove.right = cos(ang) > 0 ? vel * cos(ang) : 0;
//   thisRobot->mCurrentMove.left = cos(ang) < 0 ? -vel * cos(ang) : 0;
//
//  return 0;
//}
//
//extern Vector<SafePtr<BotNavMeshZone>> gBotNavMeshZones;
//
//
//// Get the coords of the center of mesh zone z
//S32 LuaGameObject::getZoneCenterXY(lua_State *L)
//{
//   // You give us a nil, we'll give it right back!
//   if(lua_isnil(L, 1))
//   {
//      lua_pushnil(L);
//      return 1;
//   }
//
//   S32 z = luaL_checknumber(L, 1);   // Desired zone
//
//   // In case this gets called too early...
//   if(gBotNavMeshZones.size() == 0)
//   {
//      lua_pushnil(L);
//      return 1;
//   }
//
//   // Bounds checking...
//   if(z < 0 || z >= gBotNavMeshZones.size())
//   {
//      lua_pushnil(L);
//      return 1;
//   }
//
//   Point c = gBotNavMeshZones[z]->getCenter();
//
//   lua_pushnumber(L, c.x);
//   lua_pushnumber(L, c.y);
//   return 2;
//}
//
//// Get the coords of the gateway to the specified zone.  Returns nil, nil if requested zone doesn't border current zone.
//S32 LuaGameObject::getGatewayToXY(lua_State *L)
//{
//   // You give us a nil, we'll give it right back!
//   if(lua_isnil(L, 1) || lua_isnil(L, 2))
//   {
//      lua_pushnil(L);
//      return 1;
//   }
//
//   S32 from = luaL_checknumber(L, 1);     // From this zone
//   S32 to = luaL_checknumber(L, 2);     // To this one
//
//   // In case this gets called too early...
//   if(gBotNavMeshZones.size() == 0)
//   {
//      lua_pushnil(L);
//      return 1;
//   }
//
//   // Bounds checking...
//   if(from < 0 || from >= gBotNavMeshZones.size() || to < 0 || to >= gBotNavMeshZones.size())
//   {
//      lua_pushnil(L);
//      return 1;
//   }
//
//   // Is requested zone a neighbor?
//   for(S32 i = 0; i < gBotNavMeshZones[from]->mNeighbors.size(); i++)
//   {
//      if(gBotNavMeshZones[from]->mNeighbors[i].zoneID == to)
//      {
//         Rect r(gBotNavMeshZones[from]->mNeighbors[i].borderStart, gBotNavMeshZones[from]->mNeighbors[i].borderEnd);
//         lua_pushnumber(L, r.getCenter().x);
//         lua_pushnumber(L, r.getCenter().y);
//         return 2;
//      }
//   }
//
//   // Did not find requested neighbor... returning nil
//   lua_pushnil(L);
//   return 1;
//}
//
//
//
//// Get the zone this robot is currently in.  If not in a zone, return nil
//S32 LuaGameObject::getCurrentZone(lua_State *L)
//{
//   S32 zone = thisRobot->getCurrentZone();
//
//   if(zone == -1)
//      lua_pushnil(L);
//   else
//      lua_pushnumber(L, zone);
//   return 1;
//}
//
//// Get a count of how many nav zones we have
//S32 LuaGameObject::getZoneCount(lua_State *L)
//{
//   lua_pushnumber(L, gBotNavMeshZones.size());
//   return 1;
//}
//
//// Fire current weapon if possible
//S32 LuaGameObject::fire(lua_State *L)
//{
//   thisRobot->mCurrentMove.fire = true;
//   return 0;
//}
//
//// Can robot see point XY?
//S32 LuaGameObject::hasLosXY(lua_State *L)
//{
//
//   F32 x, y;
//
//   if(lua_isnil(L, 1) || lua_isnil(L, 2))
//   {
//      lua_pushnil(L);
//      return 1;
//   }
//
//   x = luaL_checknumber(L, 1);
//   y = luaL_checknumber(L, 2);
//
//   F32 time;
//   Point coll;
//
//   lua_pushboolean(L, (thisRobot->findObjectLOS(BarrierType, MoveObject::ActualState, thisRobot->getActualPos(), Point(x,y), time, coll)) == NULL);
//   return 1;
//}
//
//
//
//// Set weapon  to 1, 2, or 3
//S32 LuaGameObject::setWeapon(lua_State *L)
//{
//   U32 weap = luaL_checknumber(L, 1);
//      thisRobot->selectWeapon(weap);
//   return 0;
//}


// Send message to all players
//S32 LuaGameObject::globalMsg(lua_State *L)
//{
//   //if(lua_isnil(L, 1))
//   //   return 0;
//
//   GameType *gt = gServerGame->getGameType();
//   //if(gt)
//   //   gt->s2cDisplayChatMessage(true, thisRobot->mPlayerName, lua_tostring(L, 1));
//   return 0;
//}

//// Send message to team (what happens when neutral/enemytoall robot does this???)
//S32 LuaGameObject::teamMsg(lua_State *L)
//{
//   if(lua_isnil(L, 1))
//      return 0;
//
//   GameType *gt = gServerGame->getGameType();
//   if(gt)
//      gt->s2cDisplayChatMessage(false, thisRobot->mPlayerName, lua_tostring(L, 1));
//   return 0;
//}
//
//
//// Returns current aim angle of ship  --> needed?
//S32 LuaGameObject::getAngle(lua_State *L)
//{
//  lua_pushnumber(L, thisRobot->mCurrentMove.angle);
//  return 1;
//}
//
//// Returns current position of ship
//S32 LuaGameObject::getPosXY(lua_State *L){
//  lua_pushnumber(L, thisRobot->getActualPos().x);
//  lua_pushnumber(L, thisRobot->getActualPos().y);
//  return 2;
//}
//
//
//// Temp method for testing development TODO: Delete
//S32 LuaGameObject::getAimAngle(lua_State *L){
//   Point loc;
//   if(thisRobot->findNearestShip(loc))
//      lua_pushnumber(L, thisRobot->getActualPos().angleTo(loc) );
//   else
//      lua_pushnumber(L, 90);     // Arbitrary!
//
//   return 1;
//}
//
//// Write a message to the server logfile
//S32 LuaGameObject::logprint(lua_State *L)
//{
//   if(!lua_isnil(L, 1))
//      logprintf("RobotLog %s: %s", thisRobot->mPlayerName.getString(), lua_tostring(L, 1));
//   return 0; 
//}  
//
//extern Rect gServerWorldBounds;
//
//
//// Assume that table is at the top of the stack
//static void setfield (lua_State *L, const char *key, F32 value) 
//{
//   lua_pushnumber(L, value);
//   lua_setfield(L, -2, key);
//}
//
//
////findClosest(enemy | health | friend | ...) --> ?
//S32 LuaGameObject::findObjects(lua_State *L)
//{
//
//   fillVector.clear();
//
//   // ShipType BarrierType MoveableType BulletType ItemType ResourceItemType EngineeredType ForceFieldType LoadoutZoneType MineType TestItemType FlagType TurretTargetType SlipZoneType HeatSeekerType SpyBugType NexusType
//
//   // thisRobot->findObjects(CommandMapVisType, fillVector, gServerWorldBounds);    // Get all globally visible objects
//   //thisRobot->findObjects(ShipType, fillVector, Rect(thisRobot->getActualPos(), gServerGame->computePlayerVisArea(thisRobot)) );    // Get other objects on screen-visible area only
//   thisRobot->findObjects(ShipType, fillVector, gServerWorldBounds );    // Get other objects on screen-visible area only
//   
//   F32 bestRange = F32_MAX;
//   Point bestPoint;
//
//   for(S32 i = 0; i < fillVector.size(); i++)
//   {
//      if(fillVector[i]->getObjectTypeMask() & (ShipType))
//      {
//         Ship *potential = (Ship*)fillVector[i];
//
//         // If it's dead or cloaked, ignore it
//         if((potential->isModuleActive(ModuleCloak) && !potential->areItemsMounted()) || potential->hasExploded)
//            continue;
//      }
//
//      GameObject *potential = fillVector[i];
//
//      // Can we see it?
//      F32 t;     // t is set in next statement
//      Point n;
//
//      // If this returns true, there is a BarrierType (i.e. wall) object obstructing our LOS
//      if(thisRobot->findObjectLOS(BarrierType, MoveObject::ActualState, thisRobot->getActualPos(), potential->getActualPos(), t, n))
//         continue;
//
//      F32 dist = thisRobot->getActualPos().distanceTo(potential->getActualPos());
//
//      if(dist < bestRange)
//      {
//         bestPoint  = potential->getActualPos();
//         bestRange  = dist;
//      }
//   }
//
//   // Write the results to Lua
//   if(bestRange < F32_MAX)
//   {
//      lua_createtable(L, 0, 2);        // creates a table  with 2 fields
//      setfield(L, "x", bestPoint.x);        // table.x = x 
//      setfield(L, "y", bestPoint.y);        // table.y = y 
//   }
//   else		// No targets found
//   {
//      lua_pushnil(L); 
//   }
//
//   return 1;
//}
//
//
//
//LuaGameObject::~LuaGameObject(){
//  logprintf("deleted Lua Object (%p)\n", this);
//}

  

//// Define the methods we will expose to Lua
//// Methods defined here need to be defined in the LuaGameObject in robot.h
//#define method(class, name) {#name, &class::name}
//
//Luna<LuaGameObject>::RegType LuaGameObject::methods[] = {
//         method(LuaGameObject, getAngle),
//         method(LuaGameObject, getPosXY),
//
//      method(LuaGameObject, getZoneCenterXY),
//      method(LuaGameObject, getGatewayToXY),
//      method(LuaGameObject, getZoneCount),
//      method(LuaGameObject, getCurrentZone),
//         
//      method(LuaGameObject, setAngle),
//      method(LuaGameObject, setAngleXY),
//      method(LuaGameObject, getAngleXY),
//      method(LuaGameObject, hasLosXY),
//
//
//      method(LuaGameObject, setThrustAng),
//      method(LuaGameObject, setThrustXY),
//      method(LuaGameObject, fire),
//      method(LuaGameObject, setWeapon),
//      method(LuaGameObject, globalMsg),
//      method(LuaGameObject, teamMsg),
//
//      method(LuaGameObject, logprint),
//
//      method(LuaGameObject, getAimAngle),
//      method(LuaGameObject, findObjects),
//      
//   {0,0}
//};
//

/* Control:
// setLoadout(m1, m2, w1, w2, w3)
// useModule(S32) // 1 && 2 && 4 of modules

// About us
author()
description()
weaponOK()  -> weapon ready to fire?



Info:
getWeapRange() -> curr weapon range
getWeapName() -> curr weapon name
getHealth()
getEnergy()
getWeapon() -> returns id of weapon

findClosest(enemy | health | friend | ...) --> ?
      ShipType BarrierType MoveableType BulletType ItemType ResourceItemType EngineeredType ForceFieldType LoadoutZoneType MineType TestItemType FlagType TurretTargetType SlipZoneType HeatSeekerType SpyBugType NexusType

findAll()

ship.speed
ship.health
ship.name

getPlayerName(ship)
getHealth(ship)
isRobot(ship)

getGameType()



*/


const char LuaGameObject::className[] = "LuaGameObject";
//class LuaPlus::LuaState;

//------------------------------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(Robot);

// Constructor
Robot::Robot(StringTableEntry robotName, S32 team, Point p, F32 m) : Ship(robotName, team, p, m)
{
   mObjectTypeMask = RobotType | MoveableType | CommandMapVisType | TurretTargetType;     // |ShipType

   mNetFlags.set(Ghostable);

   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].pos = p;
      mMoveState[i].angle = 0;
   }

   mTeam = team;
   mHealth = 1.0;       // Start at full health
   mass = m;            // Ship's mass
   hasExploded = false; // Haven't exploded yet!
   updateExtent();      // Set initial extent

   mCurrentZone = -1;   // Correct value will be calculated upon first request


   for(S32 i=0; i<TrailCount; i++)        // Don't draw any vehicle trails
      mTrail[i].reset();

   mEnergy = (S32) ((F32) EnergyMax * .80);             // Start off with 80% energy
   for(S32 i = 0; i < ModuleCount; i++)   // All modules disabled
      mModuleActive[i] = false;

   // Set initial module and weapon selections
   mModule[0] = ModuleBoost;
   mModule[1] = ModuleShield;

   mWeapon[0] = WeaponPhaser;
   mWeapon[1] = WeaponMine;
   mWeapon[2] = WeaponBurst;


   mActiveWeaponIndx = 0;

   mCooldown = false;


   // try http://csl.sublevel3.org/lua/

  // Init Lua
  //L = lua_open();    // Create a new Lua interpreter
  //LuaPlus::LuaState *L = LuaPlus::LuaState::Create();

//
//LuaPlus::LuaStateOwner state;
//state->DoString("function Add(x, y) return x + y end");
//
//LuaPlus::LuaFunction<float> Add = state->GetGlobal("Add");
//printf("Add: %d\n", Add(2, 7));
//


   // Register some functions  
  //LuaPlus::LuaFunction<const void*> getMove = (LuaPlus::LuaFunction<const void*>) L->GetGlobal("getMove");


  //luaopen_math(L);

  //luaopen_base(L);
 // luaopen_table(L);
//  luaopen_io(L);  // Don't want robots writing to our terminal.  Besides, this crashes!!
  //luaopen_string(L);
  //luaopen_debug(L);

  // Register the LuaGameObject data type with Lua
  //Luna<LuaGameObject>::Register(L);

  // In C++ - Create a Robot for use in our program
  
  // Push a pointer to this Robot to the Lua stack
  //lua_pushlightuserdata(L, (void*)this);
  //// And set the global name of this pointer
  //lua_setglobal(L,"Robot");

}
  

Robot::~Robot()
{
   // Close down our Lua interpreter
     //lua_close(L);
     //LuaPlus::LuaState::Destroy(L);

//   lua_close(mLuaInterpreter);  
   logprintf("Robot dead");
}




void Robot::onAddedToGame(Game *)
{
// Make them always visible on cmdr map --> del
   if(!isGhost())
      setScopeAlways();
}


void Robot::processArguments(S32 argc, const char **argv)
{
   if(argc != 3)
      return;

   Point pos;
   pos.read(argv + 1);
   pos *= getGame()->getGridSize();
   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].pos = pos;
      mMoveState[i].angle = 0;
   }

   updateExtent();

   string filename = "gameobject.lua";
 try
   {
  
   //if(luaL_loadfile(L, filename.c_str()))
   //S32 retCode = L->DoFile(filename.c_str());
   //if(retCode)
   //{
   //   /*logprintf("Robot error in file %s: %s.  Shutting robot down.", filename.c_str(), lua_tostring(L, -1));*/
   //   logprintf("Robot error in file %s: %g.  Shutting robot down.", filename.c_str(), retCode);
   //   delete this;
   //}

      // Run main script body 
//      lua_pcall(L, 0, 0, 0);
   }
   catch (string e)
   {
      logprintf("Robot error initializing in file %s: %s.  Shutting robot down.", filename.c_str(), e.c_str());
      delete this;
   }
  
  try
  {
     /*LuaCall<S32>(L, "getName").call();
     mPlayerName = lua_tostring(L, -1);*/

     //LuaPlus::LuaFunction<const char*> getName = L->GetGlobal("getName");
     //mPlayerName = getName();
  }
  catch (string e)
  {
     logprintf("Robot error running getName in file %s: %s", filename.c_str(), e.c_str());
     mPlayerName = "BadName";
  }

}

extern S32 findZoneContaining(Point p);

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


F32 Robot::getAngleXY(F32 x, F32 y)
{
   return atan2(y - getActualPos().y, x - getActualPos().x);
}

// Process a move.  This will advance the position of the ship, as well as adjust its velocity and angle.
void Robot::processMove(U32 stateIndex)
{
   mMoveState[LastProcessState] = mMoveState[stateIndex];

   F32 maxVel = isModuleActive(ModuleBoost) ? BoostMaxVelocity : MaxVelocity;

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



void Robot::idle(GameObject::IdleCallPath path)
{

   // Never ClientIdleControlReplay, ClientIdleControlMain, or ServerIdleControlFromClient
   TNLAssert(path == ServerIdleMainLoop || path == ClientIdleMainRemote , "Unexpected idle call path in Robot::idle!");       // TODO: Delete this line (after robots working)

   // Don't process exploded ships
   if(hasExploded)
      return;

   if(path == GameObject::ServerIdleMainLoop)      // Running on server
   {
      mCurrentMove.fire = false;   // Don't fire unless we're told to!

      try
      {
         //LuaCall<int>(L, "getMove").call();
         //getMove();
      }
      catch (string e)
      {
         logprintf("Robot error: %s", e.c_str());
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

extern F32 getAngleDiff(F32 a, F32 b);
extern bool gShowAimVector;
extern IniSettings gIniSettings;

void Robot::render(S32 layerIndex)
{
   Parent::render(layerIndex);
}


};
