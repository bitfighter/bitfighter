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

#include "ship.h"
#include "item.h"

#include "projectile.h"
#include "gameLoader.h"
#include "SoundSystem.h"
#include "gameType.h"
#include "NexusGame.h"
#include "gameConnection.h"
#include "shipItems.h"
#include "speedZone.h"
#include "gameWeapons.h"
#include "gameObjectRender.h"
#include "config.h"
#include "statistics.h"
#include "SlipZone.h"
#include "Colors.h"
#include "robot.h"            // For EventManager def
#include "stringUtils.h"      // For itos
//#include "game.h"
#include "shipItems.h"
#include "ClientInfo.h"

#ifdef TNL_OS_WIN32
#include <windows.h>   // For ARRAYSIZE
#endif

#ifndef ZAP_DEDICATED
#include "ClientGame.h"
#include "SDL_opengl.h"
#include "sparkManager.h"
#include "UI.h"
#include "UIMenus.h"
#include "UIGame.h"
#endif

#include <stdio.h>
#include <math.h>

#define hypot _hypot    // Kill some warnings

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

#define sq(a) ((a) * (a))

static const bool showCloakedTeammates = true;    // Set to true to allow players to see their cloaked teammates

namespace Zap
{


TNL_IMPLEMENT_NETOBJECT(Ship);

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

// Constructor
// Note that on client, we use all default values set in declaration; on the server, these values will be provided
// Most of these values are set in the initial packet set from the server (see packUpdate() below)
// Also, the following is also run by robot's constructor
Ship::Ship(ClientInfo *clientInfo, S32 team, Point p, F32 m, bool isRobot) : MoveObject(p, (F32)CollisionRadius), mSpawnPoint(p)
{
   mObjectTypeNumber = PlayerShipTypeNumber;
   mFireTimer = 0;

   // Set up module secondary delay timer
   for(S32 i = 0; i < ModuleCount; i++)
      mModuleSecondaryTimer[i].setPeriod(ModuleSecondaryTimerDelay);

   mSensorActiveZoomTimer.setPeriod(SensorZoomTime);
   mSensorEquipZoomTimer.setPeriod(SensorZoomTime);

   mNetFlags.set(Ghostable);

#ifndef ZAP_DEDICATED
   for(U32 i = 0; i < TrailCount; i++)
      mLastTrailPoint[i] = -1;   // Or something... doesn't really matter what
#endif

   mClientInfo = clientInfo;

   mTeam = team;
   mass = m;            // Ship's mass, not used

   // Name will be unique across all clients, but client and server may disagree on this name if the server has modified it to make it unique

   mIsRobot = isRobot;

   if(!isRobot)               // Robots will run this during their own initialization; no need to run it twice!
      initialize(p);
   else
      hasExploded = false;    // Client needs this false for unpackUpdate

#ifndef ZAP_DEDICATED
   mSparkElapsed = 0;
#endif

   // Create our proxy object for Lua access
   luaProxy = LuaShip(this);
}


// Destructor
Ship::~Ship()
{
   // Do nothing
}


// Initialize some things that both ships and bots care about... this will get run during the ship's constructor
// and also after a bot respawns and needs to reset itself
void Ship::initialize(Point &pos)
{
   // Does this ever evaluate to true?
   if(getGame())
      mRespawnTime = getGame()->getCurrentTime();

   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].pos = pos;
      mMoveState[i].angle = 0;
      mMoveState[i].vel = Point(0,0);
   }

   updateExtentInDatabase();

   mHealth = 1.0;       // Start at full health
   hasExploded = false; // Haven't exploded yet!

#ifndef ZAP_DEDICATED
   for(S32 i = 0; i < TrailCount; i++)          // Clear any vehicle trails
      mTrail[i].reset();
#endif

   mEnergy = (S32) ((F32) EnergyMax * .80);     // Start off with 80% energy
   for(S32 i = 0; i < ModuleCount; i++)         // and all modules disabled
   {
      mModulePrimaryActive[i] = false;
      mModuleSecondaryActive[i] = false;
   }

   setDefaultLoadout();

   mActiveWeaponIndx = 0;
   mCooldownNeeded = false;

   // Start spawn shield timer
   mSpawnShield.reset(SpawnShieldTime);
}


void Ship::setDefaultLoadout()
{
    // Set initial module and weapon selections
   for(S32 i = 0; i < ShipModuleCount; i++)
      mModule[i] = (ShipModule) DefaultLoadout[i];

   for(S32 i = 0; i < ShipWeaponCount; i++)
      mWeapon[i] = (WeaponType) DefaultLoadout[i + ShipModuleCount];
}


ClientInfo *Ship::getClientInfo()
{
   return mClientInfo;
}


// Push a LuaShip proxy onto the stack
void Ship::push(lua_State *L)
{
   Lunar<LuaShip>::push(L, &luaProxy, false);     // true ==> Lua will delete it's reference to this object when it's done with it
}


void Ship::onGhostRemove()
{
   Parent::onGhostRemove();
   for(S32 i = 0; i < ModuleCount; i++)
   {
      mModulePrimaryActive[i] = false;
      mModuleSecondaryActive[i] = false;
   }
   updateModuleSounds();
}


F32 Ship::getHealth()
{
   return mHealth;
}


S32 Ship::getEnergy()
{
   return mEnergy;
}


F32 Ship::getEnergyFraction()
{
   return (F32)mEnergy / (F32)EnergyMax;
}


S32 Ship::getMaxEnergy()
{
   return EnergyMax;
}


void Ship::changeEnergy(S32 deltaEnergy)
{
   mEnergy = max(0, min(static_cast<int>(EnergyMax), mEnergy + deltaEnergy));
}


bool Ship::isModulePrimaryActive(ShipModule mod)
{
   return mModulePrimaryActive[mod];
}


bool Ship::isModuleSecondaryActive(ShipModule mod)
{
   return mModuleSecondaryActive[mod];
}


void Ship::engineerBuildObject() 
{ 
   mEnergy -= getGame()->getModuleInfo(ModuleEngineer)->getPrimaryPerUseCost(); 
}


bool Ship::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc != 3)
      return false;

   Point pos;
   pos.read(argv + 1);
   pos *= game->getGridSize();
   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].pos = pos;
      mMoveState[i].angle = 0;
   }

   updateExtentInDatabase();

   return true;
}


void Ship::activateModulePrimary(U32 index)
{
   mCurrentMove.modulePrimary[index] = true;
}


void Ship::activateModuleSecondary(U32 index)
{
   mCurrentMove.moduleSecondary[index] = true;
}


Ship::SensorStatus Ship::getSensorStatus()
{
   if(!hasModule(ModuleSensor))
      return SensorStatusOff;

   if(mModulePrimaryActive[ModuleSensor])
      return SensorStatusActive;
   else
      return SensorStatusPassive;
}


void Ship::setActualPos(Point p, bool warp)
{
   mMoveState[ActualState].pos = p;
   mMoveState[RenderState].pos = p;

   if(warp)
      setMaskBits(PositionMask | WarpPositionMask | TeleportMask);
   else
      setMaskBits(PositionMask);
}


// Process a move.  This will advance the position of the ship, as well as adjust its velocity and angle.
void Ship::processMove(U32 stateIndex)
{
   const F32 ARMOR_ACCEL_PENALTY_FACT = 0.35f;
   const F32 ARMOR_SPEED_PENALTY_FACT = 1;

   mMoveState[LastProcessState] = mMoveState[stateIndex];

   F32 maxVel = (isModulePrimaryActive(ModuleBoost) ? BoostMaxVelocity : MaxVelocity) *
                (hasModule(ModuleArmor) ? ARMOR_SPEED_PENALTY_FACT : 1);

   F32 time = mCurrentMove.time * 0.001f;
   Point requestVel(mCurrentMove.x, mCurrentMove.y);

   // If going above this speed, you cannot change course
   const S32 MAX_CONTROLLABLE_SPEED = 1000;     // 1000 is completely arbitrary, but it seems to work well...
   if(mMoveState[stateIndex].vel.lenSquared() > MAX_CONTROLLABLE_SPEED * MAX_CONTROLLABLE_SPEED)
      requestVel.set(0,0);


   requestVel *= maxVel;
   F32 len = requestVel.len();

   if(len > maxVel)
      requestVel *= maxVel / len;

   Point velDelta = requestVel - mMoveState[stateIndex].vel;
   F32 accRequested = velDelta.len();


   // Apply turbo-boost if active, reduce accel and max vel when armor is present
   F32 maxAccel = (isModulePrimaryActive(ModuleBoost) ? BoostAcceleration : Acceleration) * time *
                  (hasModule(ModuleArmor) ? ARMOR_ACCEL_PENALTY_FACT : 1);
   maxAccel *= getSlipzoneSpeedMoficationFactor();

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


// Find objects of specified type that may be under the ship, and put them in fillVector
void Ship::findObjectsUnderShip(U8 type)
{
   Rect rect(getActualPos(), getActualPos());
   rect.expand(Point(CollisionRadius, CollisionRadius));

   fillVector.clear();           // This vector will hold any matching zones
   findObjects(type, fillVector, rect);
}


// Returns the zone in question if this ship is in a zone of type zoneType
// Note: If you are in multiple zones of type zoneTypeNumber, and aribtrary one will be returned, and the level designer will be flogged
GameObject *Ship::isInZone(U8 zoneTypeNumber)
{
   findObjectsUnderShip(zoneTypeNumber);

   if(fillVector.size() == 0)  // Ship isn't in extent of any objectType objects, can bail here
      return NULL;

   // Extents overlap...  now check for actual overlap

   Vector<Point> polyPoints;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      GameObject *zone = dynamic_cast<GameObject *>(fillVector[i]);

      // Get points that define the zone boundaries
      polyPoints.clear();
      zone->getCollisionPoly(polyPoints);

      if( PolygonContains2(polyPoints.address(), polyPoints.size(), getActualPos()) )
         return zone;
   }
   return NULL;
}


//GameObject *Ship::isInZone(GameObject *zone)
//{
//   // Get points that define the zone boundaries
//   Vector<Point> polyPoints;
//   polyPoints.clear();
//   zone->getCollisionPoly(polyPoints);
//
//   if( PolygonContains2(polyPoints.address(), polyPoints.size(), getActualPos()) )
//      return zone;
//   return NULL;
//}


F32 Ship::getSlipzoneSpeedMoficationFactor()
{
   SlipZone *slipzone = dynamic_cast<SlipZone *>(isInZone(SlipZoneTypeNumber));
   return slipzone ? slipzone->slipAmount : 1.0f;
}


// Returns the object in question if this ship is on an object of type objectType
DatabaseObject *Ship::isOnObject(U8 objectType)
{
   findObjectsUnderShip(objectType);

   if(fillVector.size() == 0)  // Ship isn't in extent of any objectType objects, can bail here
      return NULL;

   // Return first actually overlapping object on our candidate list
   for(S32 i = 0; i < fillVector.size(); i++)
      if(isOnObject(dynamic_cast<GameObject *>(fillVector[i])))
         return fillVector[i];
   return NULL;
}


// Given an object, see if the ship is sitting on it (useful for figuring out if ship is on top of a regenerated repair item, z.B.)
bool Ship::isOnObject(GameObject *object)
{
   Point center;
   float radius;
   static Vector<Point> polyPoints;
   polyPoints.clear();
   Rect rect;

   // Ships don't have collisionPolys, so this first check is utterly unneeded unless we change that
   /*if(getCollisionPoly(polyPoints))
      return object->collisionPolyPointIntersect(polyPoints);
   else */
   if(getCollisionCircle(MoveObject::ActualState, center, radius))
      return object->collisionPolyPointIntersect(center, radius);
   else
      return false;
}


F32 Ship::getSensorActiveZoomFraction()
{
   return 1 - mSensorActiveZoomTimer.getFraction();
}


F32 Ship::getSensorEquipZoomFraction()
{
   return 1 - mSensorEquipZoomTimer.getFraction();
}


 // Returns vector for aiming a weapon based on direction ship is facing
Point Ship::getAimVector()
{
   return Point(cos(mMoveState[ActualState].angle), sin(mMoveState[ActualState].angle) );
}


void Ship::selectNextWeapon()
{
   selectWeapon(mActiveWeaponIndx + 1);
}


void Ship::selectPrevWeapon()
{
   selectWeapon(mActiveWeaponIndx - 1);
}


// I *think* this runs only on the server
void Ship::selectWeapon(S32 weaponIdx)
{
   while(weaponIdx < 0)
      weaponIdx += ShipWeaponCount;

   mActiveWeaponIndx = weaponIdx % ShipWeaponCount;      // Advance index to selected weapon
}


WeaponType Ship::getWeapon(U32 indx)
{
   return mWeapon[indx];
}


ShipModule Ship::getModule(U32 indx)
{
   return mModule[indx];
}


void Ship::processWeaponFire()
{
   if(mFireTimer > 0)
      mFireTimer -= S32(mCurrentMove.time);

   if(!mCurrentMove.fire && mFireTimer < 0)
      mFireTimer = 0;

   mWeaponFireDecloakTimer.update(mCurrentMove.time);

   WeaponType curWeapon = mWeapon[mActiveWeaponIndx];

   GameType *gameType = getGame()->getGameType();

   if(mCurrentMove.fire && gameType)
   {
      // In a while loop, to catch up the firing rate for low Frame Per Second
      while(mFireTimer <= 0 && gameType->onFire(this) && mEnergy >= GameWeapon::weaponInfo[curWeapon].minEnergy)
      {
         mEnergy -= GameWeapon::weaponInfo[curWeapon].drainEnergy;      // Drain energy
         mWeaponFireDecloakTimer.reset(WeaponFireDecloakTime);          // Uncloak ship

         if(getClientInfo())
            getClientInfo()->getStatistics()->countShot(curWeapon);

         if(!isGhost())    // i.e. server only
         {
            Point dir = getAimVector();

            // TODO: To fix skip fire effect on jittery server, need to replace the 0 with... something...
            GameWeapon::createWeaponProjectiles(curWeapon, dir, mMoveState[ActualState].pos, 
                                                mMoveState[ActualState].vel, 0, CollisionRadius - 2, this);
         }

         mFireTimer += S32(GameWeapon::weaponInfo[curWeapon].fireDelay);

         // If we've fired, Spawn Shield turns off
         if(mSpawnShield.getCurrent() != 0)
         {
            setMaskBits(SpawnShieldMask);
            mSpawnShield.clear();
         }
      }
   }
}


void Ship::controlMoveReplayComplete()
{
   // Compute the delta between our current render position
   // and the server position after client-side prediction has
   // been run
   Point delta = mMoveState[ActualState].pos - mMoveState[RenderState].pos;
   F32 deltaLen = delta.len();

   // If the delta is either very small, or greater than the
   // max interpolation threshold, just warp to the new position
   if(deltaLen <= 0.5 || deltaLen > MaxControlObjectInterpDistance)
   {
#ifndef ZAP_DEDICATED
      // If it's a large delta, get rid of the movement trails
      if(deltaLen > MaxControlObjectInterpDistance)
         for(S32 i=0; i<TrailCount; i++)
            mTrail[i].reset();
#endif

      mMoveState[RenderState].pos = mMoveState[ActualState].pos;
      mMoveState[RenderState].vel = mMoveState[ActualState].vel;
      mInterpolating = false;
   }
   else
      mInterpolating = true;
}


void Ship::idle(GameObject::IdleCallPath path)
{
   // Don't process exploded ships
   if(hasExploded)
      return;

   if(path == GameObject::ServerIdleControlFromClient && getClientInfo())
      getClientInfo()->getStatistics()->mPlayTime += mCurrentMove.time;

   Parent::idle(path);

   if(path == GameObject::ServerIdleMainLoop && isControlled())
   {
      // If this is a controlled object in the server's main
      // idle loop, process the render state forward -- this
      // is what projectiles will collide against.  This allows
      // clients to properly lead other clients, instead of
      // piecewise stepping only when packets arrive from the client.
      processMove(RenderState);
      if(mMoveState[ActualState].vel != Point(0,0) || mMoveState[ActualState].pos != mMoveState[RenderState].pos)
         setMaskBits(PositionMask);
   }
   else
   {
      if((path == GameObject::ClientIdleControlMain || path == GameObject::ClientIdleMainRemote) && 
               mMoveState[ActualState].vel.lenSquared() != 0 && getControllingClient() && 
               getControllingClient()->lostContact())
         return;  // If we're out-of-touch, don't move the ship... moving won't actually hurt, but this seems somehow better


      // Apply impulse vector and reset it
      mMoveState[ActualState].vel += mImpulseVector;
      mImpulseVector.set(0,0);

      // For all other cases, advance the actual state of the
      // object with the current move.
      processMove(ActualState);

      // When not moving, Detect if on a GoFast - seems better to always detect...
      //if(mMoveState[ActualState].vel =f= Point(0,0))
      {
         SpeedZone *speedZone = dynamic_cast<SpeedZone *>(isOnObject(SpeedZoneTypeNumber));
         if(speedZone && speedZone->collide(this))
            speedZone->collided(this, ActualState);
      }


      if(path == GameObject::ServerIdleControlFromClient ||
         path == GameObject::ClientIdleControlMain ||
         path == GameObject::ClientIdleControlReplay)
      {
         // For different optimizer settings and different platforms
         // the floating point calculations may come out slightly
         // differently in the lowest mantissa bits.  So normalize
         // after each update the position and velocity, so that
         // the control state update will not differ from client to server.
         const F32 ShipVarNormalizeMultiplier = 128;
         const F32 ShipVarNormalizeFraction = 1 / ShipVarNormalizeMultiplier;

         mMoveState[ActualState].pos.scaleFloorDiv(ShipVarNormalizeMultiplier, ShipVarNormalizeFraction);
         mMoveState[ActualState].vel.scaleFloorDiv(ShipVarNormalizeMultiplier, ShipVarNormalizeFraction);
      }

      if(path == GameObject::ServerIdleMainLoop ||
         path == GameObject::ServerIdleControlFromClient)
      {
         // Update the render state on the server to match
         // the actual updated state, and mark the object
         // as having changed Position state.  An optimization
         // here would check the before and after positions
         // so as to not update unmoving ships.
         if(mMoveState[RenderState].angle != mMoveState[ActualState].angle ||
            mMoveState[RenderState].pos   != mMoveState[ActualState].pos   ||
            mMoveState[RenderState].vel   != mMoveState[ActualState].vel )
            setMaskBits(PositionMask);

         mMoveState[RenderState] = mMoveState[ActualState];
      }
      else if(path == GameObject::ClientIdleControlMain || path == GameObject::ClientIdleMainRemote)
      {
         // On the client, update the interpolation of this
         // object unless we are replaying control moves.
         mInterpolating = (getActualVel().lenSquared() < MoveObject::InterpMaxVelocity*MoveObject::InterpMaxVelocity);
         updateInterpolation();
      }

      if(path != GameObject::ClientIdleControlReplay) // don't want the replay to make timer count down much faster, while having high ping.
      {
         mSensorActiveZoomTimer.update(mCurrentMove.time);
         mSensorEquipZoomTimer.update(mCurrentMove.time);
         mCloakTimer.update(mCurrentMove.time);

         // Update spawn shield unless we move the ship - then it turns off .. server only
         if(mSpawnShield.getCurrent() != 0)
         {
            if(path == ServerIdleControlFromClient && (mCurrentMove.x != 0 || mCurrentMove.y != 0))
            {
               mSpawnShield.clear();
               setMaskBits(SpawnShieldMask);  // tell clients spawn shield did turn off due to moving
            }
            else
               mSpawnShield.update(mCurrentMove.time);
         }
      }
   }

   // Update the object in the game's extents database
   updateExtentInDatabase();

   // If this is a move executing on the server and it's
   // different from the last move, then mark the move to
   // be updated to the ghosts.
   if(path == GameObject::ServerIdleControlFromClient && !mCurrentMove.isEqualMove(&mLastMove))
      setMaskBits(MoveMask);

   mLastMove = mCurrentMove;

   if(path == GameObject::ServerIdleControlFromClient ||
      path == GameObject::ClientIdleControlMain ||
      path == GameObject::ClientIdleControlReplay)
   {
      // Process weapons and modules on controlled object objects
      // This handles all the energy reductions as well
      processWeaponFire();
      processModules();
   }
     
   if(path == GameObject::ClientIdleMainRemote)
   {
      // For ghosts, find some repair targets for rendering the repair effect
      if(isModulePrimaryActive(ModuleRepair))
         findRepairTargets();
   }
   if(path == GameObject::ServerIdleControlFromClient && isModulePrimaryActive(ModuleRepair))
      repairTargets();

#ifndef ZAP_DEDICATED
   if(path == GameObject::ClientIdleControlMain ||
      path == GameObject::ClientIdleMainRemote)
   {
      mWarpInTimer.update(mCurrentMove.time);

      // Emit some particles, trail sections and update the turbo noise
      emitMovementSparks();
      for(U32 i = 0; i < TrailCount; i++)
         mTrail[i].idle(mCurrentMove.time);
      updateModuleSounds();
   }
#endif
}


static Vector<DatabaseObject *> foundObjects;

// Returns true if we found a suitable target
bool Ship::findRepairTargets()
{
   // We use the render position in findRepairTargets so that
   // ships that are moving can repair each other (server) and
   // so that ships don't render funny repair lines to interpolating
   // ships (client)

   Point pos = getRenderPos();
   Rect r(pos, 2 * (RepairRadius + CollisionRadius));
   
   foundObjects.clear();
   findObjects((TestFunc)isWithHealthType, foundObjects, r);

   mRepairTargets.clear();
   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      Item *item = dynamic_cast<Item*>(foundObjects[i]);

      // Have to have an item to get a radius
      if(!item)
         continue;

      // Don't repair dead or fully healed objects...
      if(item->isDestroyed() || item->getHealth() >= 1)
         continue;

      // ...or ones not on our team or neutral
      if(item->getTeam() != TEAM_NEUTRAL && item->getTeam() != getTeam())
         continue;

      // Only repair items within a circle around the ship since we did an object search with a rectangle
      if((item->getPos() - pos).lenSquared() > sq(RepairRadius + CollisionRadius + item->getRadius()))
         continue;

      // In case of CoreItem, don't repair if no repair locations are returned
      if(item->getRepairLocations(pos).size() == 0)
         continue;

      mRepairTargets.push_back(item);
   }
   return mRepairTargets.size() != 0;
}


// Repairs ALL repair targets found above
void Ship::repairTargets()
{
   F32 totalRepair = RepairHundredthsPerSecond * 0.01f * mCurrentMove.time * 0.001f;

//   totalRepair /= mRepairTargets.size();

   DamageInfo di;
   di.damageAmount = -totalRepair;
   di.damagingObject = this;
   di.damageType = DamageTypePoint;

   for(S32 i = 0; i < mRepairTargets.size(); i++)
      mRepairTargets[i]->damageObject(&di);
}


void Ship::processModules()
{
   // Update some timers
   for(S32 i = 0; i < ModuleCount; i++)
      mModuleSecondaryTimer[i].update(mCurrentMove.time);

   // Save the previous module primary/secondary component states; reset them - to be set later
   bool wasModulePrimaryActive[ModuleCount];
   bool wasModuleSecondaryActive[ModuleCount];
   for(S32 i = 0; i < ModuleCount; i++)
   {
      wasModulePrimaryActive[i] = mModulePrimaryActive[i];
      wasModuleSecondaryActive[i] = mModuleSecondaryActive[i];
      mModulePrimaryActive[i] = false;
      mModuleSecondaryActive[i] = false;
   }

   // Only turn off cooldown if energy has risen above threshold, not if it falls below
   if(mEnergy > EnergyCooldownThreshold)
      mCooldownNeeded = false;

   // Make sure we're allowed to use modules
   bool allowed = getGame()->getGameType() && getGame()->getGameType()->okToUseModules(this);

   // Go through our loaded modules and see if they are currently turned on
   // Are these checked on the server side?
   for(S32 i = 0; i < ShipModuleCount; i++)   
   {
      // If you have passive module, it's always active, no restrictions, but is off for energy consumption purposes
      if(getGame()->getModuleInfo(mModule[i])->getPrimaryUseType() == ModulePrimaryUsePassive)
         mModulePrimaryActive[mModule[i]] = true;         // needs to be true to allow stats counting

      // Set loaded module states to 'on' if detected as so,
      // unless modules are disallowed or we need to cooldown
      if (!mCooldownNeeded && allowed)
      {
         if(mCurrentMove.modulePrimary[i])
            mModulePrimaryActive[mModule[i]] = true;
         if(mCurrentMove.moduleSecondary[i])
            mModuleSecondaryActive[mModule[i]] = true;
      }
   }

   // No Turbo or Pulse if we're not moving
   if(mModulePrimaryActive[ModuleBoost] && mCurrentMove.x == 0 && mCurrentMove.y == 0)
   {
      mModulePrimaryActive[ModuleBoost] = false;
      mModuleSecondaryActive[ModuleBoost] = false;
   }

   // No repair with no targets
   if(mModulePrimaryActive[ModuleRepair] && !findRepairTargets())
      mModulePrimaryActive[ModuleRepair] = false;

   // No cloak with nearby sensored people
   if(mModulePrimaryActive[ModuleCloak])
   {
      if(mWeaponFireDecloakTimer.getCurrent() != 0)
         mModulePrimaryActive[ModuleCloak] = false;
      //else
      //{
      //   Rect cloakCheck(getActualPos(), getActualPos());
      //   cloakCheck.expand(Point(CloakCheckRadius, CloakCheckRadius));

      //   fillVector.clear();
      //   findObjects(ShipType | RobotType, fillVector, cloakCheck);

      //   if(fillVector.size() > 0)
      //   {
      //      for(S32 i=0; i<fillVector.size(); i++)
      //      {
      //         Ship *s = dynamic_cast<Ship *>(fillVector[i]);

      //         if(!s) continue;

      //         if(s->getTeam() != getTeam() && s->isModuleActive(ModuleSensor))
      //         {
      //            mModuleActive[ModuleCloak] = false;
      //            break;
      //         }
      //      }
      //   }
      //}
   }

   // This will make the module primary active component use the proper
   // amount of energy per second since the game idle methods are run on milliseconds
   F32 scaleFactor = mCurrentMove.time * 0.001f;

   // Update things based on available energy...
   bool anyActive = false;
   for(S32 i = 0; i < ModuleCount; i++)
   {
      if(mModulePrimaryActive[i])
      {
         S32 EnergyUsed = S32(getGame()->getModuleInfo((ShipModule) i)->getPrimaryEnergyDrain() * scaleFactor);
         mEnergy -= EnergyUsed;
         anyActive = anyActive || (EnergyUsed != 0);   // to prevent armor and engineer stop energy recharge

         if(getClientInfo())
            getClientInfo()->getStatistics()->addModuleUsed(ShipModule(i), mCurrentMove.time);
      }

      // Fire the module secondary component if it is active and the delay timer has run out
      if(mModuleSecondaryActive[i] && mModuleSecondaryTimer[i].getCurrent() == 0)
      {
         S32 energyCost = gModuleInfo[i].getSecondaryPerUseCost();
         // If we have enough energy, fire the module
         if(mEnergy >= energyCost)
         {
            // Reduce energy
            mEnergy -= energyCost;

            // Sensor module needs to play a spybug (done server-side only)
            if(i == ModuleSensor && !isGhost())
            {
               Point direction = getAimVector();
               GameWeapon::createWeaponProjectiles(WeaponSpyBug, direction, mMoveState[ActualState].pos,
                                                   mMoveState[ActualState].vel, 0, CollisionRadius - 2, this);
            }
            // Pulse uses up all energy and applies an impulse vector
            else if(i == ModuleBoost)
            {
               // The impulse should be in the same direction you're already going
               mImpulseVector = mMoveState[ActualState].vel;

               // Change to Pulse speed based on current energy
               mImpulseVector.normalize((((F32)mEnergy/(F32)EnergyMax) * (PulseMaxVelocity - PulseMinVelocity)) + PulseMinVelocity);

               mEnergy = 0;
            }
         }
      }
   }

   // Set cool-down needed if energy is below threshold
   if(!anyActive && mEnergy <= EnergyCooldownThreshold)
      mCooldownNeeded = true;

   // Energy will recharge with special rules
   // It will not recharge if you have an activated module or spawn shield is up
   if(!anyActive && mSpawnShield.getCurrent() == 0)
   {
      // Idle = not moving or shooting
      bool isIdle = mCurrentMove.x == 0 && mCurrentMove.y == 0 && !mCurrentMove.fire;

      GameObject *object = isInZone(LoadoutZoneTypeNumber);

      // In load-out zone
      if(object)
      {
         /// IN HOSTILE ZONE
         if(object->getTeam() == TEAM_HOSTILE)
         {
            if(isIdle)
               mEnergy += S32(EnergyRechargeRateInHostileLoadoutZoneWhenIdle * scaleFactor);
            else
               mEnergy += S32(EnergyRechargeRateInHostileLoadoutZoneWhenActive * scaleFactor);
         }

         /// IN FRIENDLY ZONE
         else if(object->getTeam() == getTeam())
         {
            if(isIdle)
               mEnergy += S32(EnergyRechargeRateInFriendlyLoadoutZoneWhenIdle * scaleFactor);
            else
               mEnergy += S32(EnergyRechargeRateInFriendlyLoadoutZoneWhenActive * scaleFactor);
         }
         /// IN NEUTRAL ZONE
         else if(object->getTeam() == TEAM_NEUTRAL)
         {
            if(isIdle)
               mEnergy += S32(EnergyRechargeRateInNeutralLoadoutZoneWhenIdle * scaleFactor);
            else
               mEnergy += S32(EnergyRechargeRateInNeutralLoadoutZoneWhenActive * scaleFactor);
         }
         // In a zone that is neither hostile nor neutral nor one of your own -- must be an enemy 
         /// IN ENEMY ZONE
         else
         {
            if(isIdle)
               mEnergy += S32(EnergyRechargeRateInEnemyLoadoutZoneWhenIdle * scaleFactor);
            else
               mEnergy += S32(EnergyRechargeRateInEnemyLoadoutZoneWhenActive * scaleFactor);
         }
      }
      /// IN NO ZONE
      else 
      {
         if(isIdle)
            mEnergy += S32(EnergyRechargeRateWhenInNoZoneWhenIdle * scaleFactor);
         else
            mEnergy += S32(EnergyRechargeRateWhenInNoZoneWhenActive * scaleFactor);
      }
   }

   // What to do if our most current energy reduction put us below zero
   if(mEnergy <= 0)
   {
      mEnergy = 0;
      for(S32 i = 0; i < ModuleCount; i++)
      {
         mModulePrimaryActive[i] = false;
         mModuleSecondaryActive[i] = false;
      }
      mCooldownNeeded = true;
   }

   // If energy is greater than maximum, set to max
   if(mEnergy >= EnergyMax)
      mEnergy = EnergyMax;

   // Do logic triggered when module primary component state changes
   for(S32 i = 0; i < ModuleCount;i++)
   {
      if(mModulePrimaryActive[i] != wasModulePrimaryActive[i])
      {
         if(i == ModuleSensor)
         {
            mSensorActiveZoomTimer.reset();
            mSensorStartTime = getGame()->getCurrentTime();
            if(mModulePrimaryActive[i])
               mEnergy -= SensorInitialEnergyUsage; // inital energy use, prevents tapping to see cloaked
         }
         else if(i == ModuleCloak)
            mCloakTimer.reset(CloakFadeTime - mCloakTimer.getCurrent(), CloakFadeTime);

         setMaskBits(ModulePrimaryMask);
      }
   }

   // Do logic triggered when module secondary component state changes
   for(S32 i = 0; i < ModuleCount;i++)
   {
      if(mModuleSecondaryActive[i] != wasModuleSecondaryActive[i])
      {
         // If current state is active, reset the delay timer if it has run out
         if(mModuleSecondaryActive[i])
            if(mModuleSecondaryTimer[i].getCurrent() == 0)
               mModuleSecondaryTimer[i].reset();

         setMaskBits(ModuleSecondaryMask);
      }
   }
}


void Ship::damageObject(DamageInfo *theInfo)
{
   if(mHealth == 0 || hasExploded) return; // Stop multi-kill problem. Might stop robots from getting invincible.

   // Deal with grenades and other explody things, even if they cause no damage
   if(theInfo->damageType == DamageTypeArea)
      mImpulseVector += theInfo->impulseVector;

   if(theInfo->damageAmount == 0)
      return;

   F32 damageAmount = theInfo->damageAmount;

   if(theInfo->damageAmount > 0)
   {
      if(!getGame()->getGameType()->objectCanDamageObject(theInfo->damagingObject, this))
         return;

      // Factor in shields
      if(isModulePrimaryActive(ModuleShield)) // && mEnergy >= EnergyShieldHitDrain)     // Commented code will cause
      {                                                                           // shields to drain when they
         //mEnergy -= EnergyShieldHitDrain;                                       // have been hit.
         return;
      }

      // No damage done if spawn shield is active
      if(mSpawnShield.getCurrent() != 0)
         return;

      // Having armor halves the damage
      if(hasModule(ModuleArmor))
      {
         // Except for bouncers - they do a little more damage
         Projectile* projectile = dynamic_cast<Projectile*>(theInfo->damagingObject);
         if(projectile && projectile->mWeaponType == WeaponBounce)
            damageAmount /= 1.3333f;  // Bouncers do 3/4 damage
         else
            damageAmount /= 2;        // Everything else does 1/2
      }
   }

   ClientInfo *damagerOwner = theInfo->damagingObject->getOwner();
   ClientInfo *victimOwner = this->getOwner();

   // Healing things do negative damage, thus adding to health
   mHealth -= damageAmount * ((victimOwner && damagerOwner == victimOwner) ? theInfo->damageSelfMultiplier : 1);
   setMaskBits(HealthMask);

   if(mHealth <= 0)
   {
      mHealth = 0;
      kill(theInfo);
   }
   else if(mHealth > 1)
      mHealth = 1;


   if(getClientInfo()) // could be NULL
   {
      Projectile *projectile = dynamic_cast<Projectile *>(theInfo->damagingObject);

      if(projectile)
         getClientInfo()->getStatistics()->countHitBy(projectile->mWeaponType);
 
      else if(mHealth == 0 && dynamic_cast<Asteroid *>(theInfo->damagingObject))
         getClientInfo()->getStatistics()->mCrashedIntoAsteroid++;
   }
}


// Runs when ship spawns -- runs on client and server
// Gets run on client every time ship spawns, gets run on server once per level
void Ship::onAddedToGame(Game *game)
{
   Parent::onAddedToGame(game);

   // From here on down, server only
   if(!isGhost())
   {
      mRespawnTime = getGame()->getCurrentTime();
      EventManager::get()->fireEvent(EventManager::ShipSpawnedEvent, this);
   }
}


void Ship::updateModuleSounds()
{
   const S32 moduleSFXs[ModuleCount] =
   {
      SFXShieldActive,
      SFXShipBoost,
      SFXSensorActive,
      SFXRepairActive,
      SFXUIBoop, // Need better sound...
      SFXCloakActive,
      SFXNone, // armor
   };

   for(U32 i = 0; i < ModuleCount; i++)
   {
      if(mModulePrimaryActive[i] && moduleSFXs[i] != SFXNone)
      {
         if(mModuleSound[i].isValid())
            SoundSystem::setMovementParams(mModuleSound[i], mMoveState[RenderState].pos, mMoveState[RenderState].vel);
         else if(moduleSFXs[i] != -1)
            mModuleSound[i] = SoundSystem::playSoundEffect(moduleSFXs[i], mMoveState[RenderState].pos, mMoveState[RenderState].vel);
      }
      else
      {
         if(mModuleSound[i].isValid())
         {
//            mModuleSound[i]->stop();
            SoundSystem::stopSoundEffect(mModuleSound[i]);
            mModuleSound[i] = NULL;
         }
      }
   }
}


static U32 MaxFireDelay = 0;

// static method, only run during init on both client and server
void Ship::computeMaxFireDelay()
{
   for(S32 i = 0; i < WeaponCount; i++)
   {
      if(GameWeapon::weaponInfo[i].fireDelay > MaxFireDelay)
         MaxFireDelay = GameWeapon::weaponInfo[i].fireDelay;
   }
}

const U32 negativeFireDelay = 123;  // how far into negative we are allowed to send.
// MaxFireDelay + negativeFireDelay, 900 + 123 = 1023, so writeRangedU32 are sending full range of 10 bits of information.

void Ship::writeControlState(BitStream *stream)
{
   stream->write(mMoveState[ActualState].pos.x);
   stream->write(mMoveState[ActualState].pos.y);
   stream->write(mMoveState[ActualState].vel.x);
   stream->write(mMoveState[ActualState].vel.y);
   stream->writeRangedU32(mEnergy, 0, EnergyMax);
   stream->writeFlag(mCooldownNeeded);
   if(mFireTimer < 0)   // mFireTimer could be negative.
      stream->writeRangedU32(MaxFireDelay + (mFireTimer < -S32(negativeFireDelay) ? negativeFireDelay : U32(-mFireTimer)),0, MaxFireDelay + negativeFireDelay);
   else
      stream->writeRangedU32(U32(mFireTimer), 0, MaxFireDelay + negativeFireDelay);
   stream->writeRangedU32(mActiveWeaponIndx, 0, WeaponCount);
}

void Ship::readControlState(BitStream *stream)
{
   stream->read(&mMoveState[ActualState].pos.x);
   stream->read(&mMoveState[ActualState].pos.y);
   stream->read(&mMoveState[ActualState].vel.x);
   stream->read(&mMoveState[ActualState].vel.y);
   mEnergy = stream->readRangedU32(0, EnergyMax);
   mCooldownNeeded = stream->readFlag();
   mFireTimer = S32(stream->readRangedU32(0, MaxFireDelay + negativeFireDelay));
   if(mFireTimer > S32(MaxFireDelay))
      mFireTimer =  S32(MaxFireDelay) - mFireTimer;

   U32 previousWeaponIndex = mActiveWeaponIndx;
   mActiveWeaponIndx = stream->readRangedU32(0, WeaponCount);

#ifndef ZAP_DEDICATED
   if(previousWeaponIndex != mActiveWeaponIndx && !getGame()->getSettings()->getIniSettings()->showWeaponIndicators)
      static_cast<ClientGame *>(getGame())->displayMessage(Colors::cyan, "%s selected.", GameWeapon::weaponInfo[mWeapon[mActiveWeaponIndx]].name.getString());
#endif
}


// Transmit ship status from server to client
// Any changes here need to be reflected in Ship::unpackUpdate
U32 Ship::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   GameConnection *gameConnection = (GameConnection *) connection;

   if(isInitialUpdate())      // This stuff gets sent only once per ship
   {
      // We'll need the name (or some other identifier) to match the ship to its clientInfo on the client side
      stream->writeStringTableEntry(getClientInfo() ? getClientInfo()->getName() : StringTableEntry());

      // Now write all the mounts:
      for(S32 i = 0; i < mMountedItems.size(); i++)
      {
         if(mMountedItems[i].isValid())
         {
            S32 index = connection->getGhostIndex(mMountedItems[i]);
            if(index != -1)
            {
               stream->writeFlag(true);
               stream->writeInt(index, GhostConnection::GhostIdBitSize);
            }
         }
      }
      stream->writeFlag(false);
   }  // End initial update

   if(stream->writeFlag(updateMask & ChangeTeamMask))    // A player with admin can change robots teams
      writeThisTeam(stream);


   if(stream->writeFlag(updateMask & LoadoutMask))       // Module configuration
   {
      for(S32 i = 0; i < ShipModuleCount; i++)
         stream->writeEnum(mModule[i], ModuleCount);

      for(S32 i = 0; i < ShipWeaponCount; i++)
         stream->writeEnum(mWeapon[i], WeaponCount);
   }

   if(!stream->writeFlag(hasExploded))
   {
      if(stream->writeFlag(updateMask & (RespawnMask | SpawnShieldMask)))
      {
         stream->writeFlag((updateMask & RespawnMask) != 0 && getGame()->getCurrentTime() - mRespawnTime < 300);  // If true, ship will appear to spawn on client
         U32 sendNumber = (mSpawnShield.getCurrent() + (SpawnShieldTime / 16 / 2)) * 16 / SpawnShieldTime; // rounding
         if(stream->writeFlag(sendNumber != 0))
            stream->writeInt(sendNumber - 1, 4); 
      }

      if(stream->writeFlag(updateMask & HealthMask))     // Health
         stream->writeFloat(mHealth, 6);
   }

   stream->writeFlag((updateMask & WarpPositionMask) && updateMask != 0xFFFFFFFF);

   // Don't show warp effect when all mask flags are set, as happens when ship comes into scope
   stream->writeFlag((updateMask & TeleportMask) && !(updateMask & InitialMask));

   bool shouldWritePosition = (updateMask & InitialMask) || gameConnection->getControlObject() != this;

   if(!shouldWritePosition)
   {
      // The number of writeFlags here *must* match the same number in the else statement
      stream->writeFlag(false);
      stream->writeFlag(false);
      stream->writeFlag(false);
      stream->writeFlag(false);
   }
   else     // Write mCurrentMove data...
   {
      if(stream->writeFlag(updateMask & PositionMask))         // <=== ONE
      {
         // Send position and speed
         gameConnection->writeCompressedPoint(mMoveState[RenderState].pos, stream);
         writeCompressedVelocity(mMoveState[RenderState].vel, BoostMaxVelocity + 1, stream);
      }
      if(stream->writeFlag(updateMask & MoveMask))             // <=== TWO
         mCurrentMove.pack(stream, NULL, false);               // Send current move

      // If a module primary component is detected as on, pack it
      if(stream->writeFlag(updateMask & ModulePrimaryMask))    // <=== THREE
         for(S32 i = 0; i < ModuleCount; i++)                  // Send info about which modules are active
            stream->writeFlag(mModulePrimaryActive[i]);

      // If a module secondary component is detected as on, pack it
      if(stream->writeFlag(updateMask & ModuleSecondaryMask))  // <=== FOUR
         for(S32 i = 0; i < ModuleCount; i++)                  // Send info about which modules are active
            stream->writeFlag(mModuleSecondaryActive[i]);
   }
   return 0;
}


// Any changes here need to be reflected in Ship::packUpdate
void Ship::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
#ifndef ZAP_DEDICATED
   bool positionChanged = false;    // True when position changes a little
   bool shipwarped = false;         // True when position changes a lot

   bool wasInitialUpdate = false;
   bool playSpawnEffect = false;

   if(isInitialUpdate())
   {
      wasInitialUpdate = true;

      // Read the name and use it to find the clientInfo that should be waiting for us... hopefully
      StringTableEntry playerName;
      stream->readStringTableEntry(&playerName);

      ClientInfo *clientInfo = getGame()->findClientInfo(playerName);

      // A null player name is the tell-tale sign of a 'Ship' object in the level file
//      TNLAssert(clientInfo || playerName.isNull(), "We need a clientInfo for this ship!");

      mClientInfo = clientInfo;

      // Read mounted items:
      while(stream->readFlag())
      {
         S32 index = stream->readInt(GhostConnection::GhostIdBitSize);
         MoveItem *item = (MoveItem *) connection->resolveGhost(index);
         item->mountToShip(this);
      }

   }  // initial update

   if(stream->readFlag())        // Team changed (ChangeTeamMask)
      readThisTeam(stream);

   if(stream->readFlag())        // New module configuration
   {
      bool hadSensorThen = false;
      bool hasSensorNow = false;

      for(S32 i = 0; i < ShipModuleCount; i++)
      {
         // Check old loadout for sensor
         if(mModule[i] == ModuleSensor)
            hadSensorThen = true;

         // Set to new loadout
         mModule[i] = (ShipModule) stream->readEnum(ModuleCount);

         // Check new loadout for sensor
         if(mModule[i] == ModuleSensor)
            hasSensorNow = true;
      }

      // Set sensor zoom timer if sensor carrying status has switched
      if(hadSensorThen != hasSensorNow && !isInitialUpdate())  // ! isInitialUpdate(), don't do zoom out effect of ship spawn
         mSensorEquipZoomTimer.reset();

      for(S32 i = 0; i < ShipWeaponCount; i++)
         mWeapon[i] = (WeaponType) stream->readEnum(WeaponCount);
   }

   if(stream->readFlag())  // hasExploded
   {
      mHealth = 0;
      if(!hasExploded)
      {
         hasExploded = true;
         disableCollision();

         if(!wasInitialUpdate)
            emitShipExplosion(mMoveState[ActualState].pos);    // Boom!
      }
   }
   else
   {
      if(stream->readFlag())        // Respawn
      {
         if(hasExploded)
            enableCollision();
         hasExploded = false;
         playSpawnEffect = stream->readFlag();    // Prevent spawn effect every time the robot goes into scope
         shipwarped = true;
         if(stream->readFlag())
            mSpawnShield.reset((stream->readInt(4) + 1) * SpawnShieldTime / 16);
         else
            mSpawnShield.reset(0);
      }
      if(stream->readFlag())        // Health
         mHealth = stream->readFloat(6);
   }

   if(stream->readFlag())        // Ship made a large change in position
      shipwarped = true;

   if(stream->readFlag())        // Ship just teleported
   {
      shipwarped = true;
      mWarpInTimer.reset(WarpFadeInTime);    // Make ship all spinny (sfx, spiral bg are done by the teleporter itself)
   }

   if(stream->readFlag())     // UpdateMask
   {
      ((GameConnection *) connection)->readCompressedPoint(mMoveState[ActualState].pos, stream);
      readCompressedVelocity(mMoveState[ActualState].vel, BoostMaxVelocity + 1, stream);
      positionChanged = true;
   }

   if(stream->readFlag())     // MoveMask
   {
      mCurrentMove = Move();  // A new, blank move
      mCurrentMove.unpack(stream, false);
   }

   if(stream->readFlag())     // ModulePrimaryMask
   {
      bool wasPrimaryActive[ModuleCount];
      for(S32 i = 0; i < ModuleCount; i++)
      {
         wasPrimaryActive[i] = mModulePrimaryActive[i];
         mModulePrimaryActive[i] = stream->readFlag();

         // Module activity toggled
         if(wasPrimaryActive[i] != mModulePrimaryActive[i])
         {
            if(i == ModuleSensor)
            {
               mSensorStartTime = getGame()->getCurrentTime();
            }
            else if(i == ModuleCloak)
               mCloakTimer.reset(CloakFadeTime - mCloakTimer.getCurrent(), CloakFadeTime);
         }
      }
   }

   if(stream->readFlag())     // ModuleSecondaryMask
      for(S32 i = 0; i < ModuleCount; i++)
         mModuleSecondaryActive[i] = stream->readFlag();

   mMoveState[ActualState].angle = mCurrentMove.angle;


   if(positionChanged && !isRobot() )
   {
      mCurrentMove.time = (U32) connection->getOneWayTime();
      processMove(ActualState);
   }

   if(shipwarped)
   {
      mInterpolating = false;
      mMoveState[RenderState] = mMoveState[ActualState];

      for(S32 i=0; i<TrailCount; i++)
         mTrail[i].reset();
   }
   else
      mInterpolating = true;



   if(playSpawnEffect)
   {
      mWarpInTimer.reset(WarpFadeInTime);    // Make ship all spinny

      TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");

      static_cast<ClientGame *>(getGame())->emitTeleportInEffect(mMoveState[ActualState].pos, 1);

      SoundSystem::playSoundEffect(SFXTeleportIn, mMoveState[ActualState].pos);
   }

#endif
}  // unpackUpdate


F32 Ship::getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips)
{
   F32 value = Parent::getUpdatePriority(scopeObject, updateMask, updateSkips);

   if(getControllingClient())
      value += 2.3f;
   else
      value -= 2.3f;

   return value;
}

static F32 getAngleDiff(F32 a, F32 b)
{
   // Figure out the shortest path from a to b...
   // Restrict them to the range 0-360
   while(a<0)   a+=360;
   while(a>360) a-=360;

   while(b<0)   b+=360;
   while(b>360) b-=360;

   return  (fabs(b-a) > 180) ? 360-(b-a) : b-a;
}


bool Ship::hasModule(ShipModule mod)
{
   for(S32 i = 0; i < ShipModuleCount; i++)
      if(mModule[i] == mod)
         return true;
   return false;
}


bool Ship::isDestroyed()
{
   return hasExploded;
}


bool Ship::isItemMounted() 
{ 
   return mMountedItems.size() != 0; 
}


bool Ship::isVisible() 
{
   if(!isModulePrimaryActive(ModuleCloak))
      return true;

   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid() && mMountedItems[i]->isItemThatMakesYouVisibleWhileCloaked())
         return true;

   return false;
}


// Returns index of first flag mounted on ship, or NO_FLAG if there aren't any
S32 Ship::carryingFlag()
{
   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid() && (mMountedItems[i]->getObjectTypeNumber() == FlagTypeNumber))
         return i;
   return GameType::NO_FLAG;
}


S32 Ship::getFlagCount()
{
   S32 count = 0;
   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid() && (mMountedItems[i]->getObjectTypeNumber() == FlagTypeNumber))
      {
         NexusFlagItem *flag = dynamic_cast<NexusFlagItem *>(mMountedItems[i].getPointer());
         if(flag != NULL)   // Nexus flag have multiple flags as one item.
            count += flag->getFlagCount();
         else
            count++;
      }
   return count;
}


bool Ship::isCarryingItem(U8 objectType)
{
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
      if(mMountedItems[i].isValid() && mMountedItems[i]->getObjectTypeNumber() == objectType)
         return true;
   return false;
}


MoveItem *Ship::unmountItem(U8 objectType)
{
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
      if(mMountedItems[i]->getObjectTypeNumber() == objectType)
      {
         MoveItem *item = mMountedItems[i];
         item->dismount();
         return item;
      }

   return NULL;
}


WeaponType Ship::getSelectedWeapon()
{
   return mWeapon[mActiveWeaponIndx];
}


U32 Ship::getSelectedWeaponIndex()
{
   return mActiveWeaponIndx;
}


// Fills loadout 
void Ship::getLoadout(Vector<U8> &loadout)
{
   loadout.clear();
   for(S32 i = 0; i < ShipModuleCount; i++)
      loadout.push_back(mModule[i]);

   for(S32 i = 0; i < ShipWeaponCount; i++)
      loadout.push_back(mWeapon[i]);
}


bool Ship::isLoadoutSameAsCurrent(const Vector<U8> &loadout)
{
   for(S32 i = 0; i < ShipModuleCount; i++)
      if(loadout[i] != (U8)mModule[i])
         return false;

   for(S32 i = ShipModuleCount; i < ShipWeaponCount + ShipModuleCount; i++)
      if(loadout[i] != (U8)mWeapon[i - ShipModuleCount])
         return false;

   return true;
}


// This actualizes the requested loadout... when, for example the user enters a loadout zone
// To set the "on-deck" loadout, use GameType->setClientShipLoadout()
// Returns true if loadout has changed
bool Ship::setLoadout(const Vector<U8> &loadout, bool silent)
{
   // Check to see if the new configuration is the same as the old.  If so, we have nothing to do.
   if(isLoadoutSameAsCurrent(loadout))      // Don't bother if ship config hasn't changed
      return false;

   if(getClientInfo())
      getClientInfo()->getStatistics()->mChangedLoadout++;

   WeaponType currentWeapon = mWeapon[mActiveWeaponIndx];

   for(S32 i = 0; i < ShipModuleCount; i++)
      mModule[i] = (ShipModule) loadout[i];

   for(S32 i = ShipModuleCount; i < ShipWeaponCount + ShipModuleCount; i++)
      mWeapon[i - ShipModuleCount] = (WeaponType) loadout[i];

   setMaskBits(LoadoutMask);

   if(silent) 
      return true;

   // Try to see if we can maintain the same weapon we had before.
   S32 i;
   for(i = 0; i < ShipWeaponCount; i++)
      if(mWeapon[i] == currentWeapon)
      {
         mActiveWeaponIndx = i;
         break;
      }

   if(i == ShipWeaponCount)               // Nope...
      selectWeapon(0);                    // ...so select first weapon

   if(!hasModule(ModuleEngineer))         // We don't have engineer, so drop any resources we may be carrying
   {
      for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
         if(mMountedItems[i]->getObjectTypeNumber() == ResourceItemTypeNumber)
            mMountedItems[i]->dismount();
   }

   // And notifiy user
   GameConnection *cc = getControllingClient();

#ifndef ZAP_DEDICATED
   // Safe to delete this block?  Ever triggered?  Anyone?  (-CE Jan 2012)
   //if(!cc)
   //{
   //   TNLAssert(false, "If this ever gets triggered, please remove the comment above, and add a note about how we get here...");
   //   TNLAssert(dynamic_cast<ClientGame *>(mGame) != NULL, "Not a ClientGame"); // If this asserts, need to revert to dynamic_cast with NULL check
   //   ClientGame *clientGame = static_cast<ClientGame *>(getGame());

   //   cc = clientGame->getConnectionToServer();      // Second try  ==> under what circumstances can this happen?
   //   TNLAssert(cc, "Problem!");
   //}
#endif

   if(cc)
   {
      static StringTableEntry msg("Ship loadout configuration updated.");
      cc->s2cDisplayMessage(GameConnection::ColorAqua, SFXUIBoop, msg);
   }

   return true;
}


// Will return an empty string if loadout looks invalid
string Ship::loadoutToString(const Vector<U8> &loadout)
{
   // Only expect missized loadout when presets haven't all been set, and loadout.size will be 0
   if(loadout.size() != ShipModuleCount + ShipWeaponCount)
      return "";

   Vector<string> loadoutStrings(ShipModuleCount + ShipWeaponCount);    // Reserving some space makes things a tiny bit more efficient

   // First modules
   for(S32 i = 0; i < ShipModuleCount; i++)
      loadoutStrings.push_back(Game::getModuleInfo((ShipModule) loadout[i])->getName());

   // Then weapons
   for(S32 i = ShipModuleCount; i < ShipWeaponCount + ShipModuleCount; i++)
      loadoutStrings.push_back(GameWeapon::weaponInfo[loadout[i]].name.getString());

   return listToString(loadoutStrings, ',');
}


// Fills loadout with appropriate values; returns true if string looks valid, false if not
// Note that even if we are able to parse the loadout successfully, it might still be invalid for a 
// particular server or gameType... engineer, for example, is not allowed everywhere.
bool Ship::stringToLoadout(string loadoutStr, Vector<U8> &loadout)
{
   loadout.clear();

   // If loadout preset hasn't been set, we'll get a blank string.  Handle that here so we don't log an error later.
   if(loadoutStr == "")
      return false;

   Vector<string> words;
   parseString(loadoutStr, words, ',');

   if(words.size() != ShipModuleCount + ShipWeaponCount)      // Invalid loadout string
   {
      logprintf(LogConsumer::ConfigurationError, "Misconfigured loadout preset found in INI");
      loadout.clear();

      return false;
   }

   loadout.reserve(ShipModuleCount + ShipWeaponCount);        // Preallocate the amount of space we expect to have

   bool found;

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      found = false;
      const char *word = words[i].c_str();

      for(S32 j = 0; j < ModuleCount; j++)
         if(!stricmp(word, Game::getModuleInfo((ShipModule) j)->getName()))     // Case insensitive
         {
            loadout.push_back(j);
            found = true;
            break;
         }

      if(!found)
      {
         logprintf(LogConsumer::ConfigurationError, "Unknown module found in loadout preset in INI file: %s", word);
         loadout.clear();

         return false;
      }
   }

   for(S32 i = ShipModuleCount; i < ShipWeaponCount + ShipModuleCount; i++)
   {
      found = false;
      const char *word = words[i].c_str();

      for(S32 j = 0; j < WeaponCount; j++)
         if(!stricmp(word, GameWeapon::weaponInfo[j].name.getString()))
         {
            loadout.push_back(j);
            found = true;
            break;
         }

      if(!found)
      {
         logprintf(LogConsumer::ConfigurationError, "Unknown weapon found in loadout preset in INI file: %s", word);
         loadout.clear();

         return false;
      }
   }

   return true;
}


void Ship::kill(DamageInfo *theInfo)
{
   if(isGhost())     // Server only, please...
      return;

   GameType *gt = getGame()->getGameType();
   if(gt)
      gt->controlObjectForClientKilled(getClientInfo(), this, theInfo->damagingObject);

   kill();
}


void Ship::kill()
{
   if(!isGhost())
   {
      EventManager::get()->fireEvent(EventManager::ShipKilledEvent, this);

      if(getOwner())
         getLoadout(getOwner()->mOldLoadout);
   }

   deleteObject(KillDeleteDelay);
   hasExploded = true;
   setMaskBits(ExplosionMask);
   disableCollision();

   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
      mMountedItems[i]->onMountDestroyed();
}


enum {
   NumShipExplosionColors = 12,
};

Color ShipExplosionColors[NumShipExplosionColors] = {
   Colors::red,
   Color(0.9, 0.5, 0),
   Colors::white,
   Colors::yellow,
   Colors::red,
   Color(0.8, 1.0, 0),
   Color(1, 0.5, 0),
   Colors::white,
   Colors::red,
   Color(0.9, 0.5, 0),
   Colors::white,
   Colors::yellow,
};

void Ship::emitShipExplosion(Point pos)
{
#ifndef ZAP_DEDICATED
   SoundSystem::playSoundEffect(SFXShipExplode, pos);

   F32 a = TNL::Random::readF() * 0.4f + 0.5f;
   F32 b = TNL::Random::readF() * 0.2f + 0.9f;

   F32 c = TNL::Random::readF() * 0.15f + 0.125f;
   F32 d = TNL::Random::readF() * 0.2f + 0.9f;

   TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");
   ClientGame *game = static_cast<ClientGame *>(getGame());

   game->emitExplosion(mMoveState[ActualState].pos, 0.9f, ShipExplosionColors, NumShipExplosionColors);
   game->emitBurst(pos, Point(a,c), Color(1,1,0.25), Colors::red);
   game->emitBurst(pos, Point(b,d), Colors::yellow, Color(0,0.75,0));
#endif
}

void Ship::emitMovementSparks()
{
#ifndef ZAP_DEDICATED
   //U32 deltaT = mCurrentMove.time;

   // Do nothing if we're under 0.1 vel
   if(hasExploded || mMoveState[ActualState].vel.len() < 0.1)
      return;

   bool boostActive = isModulePrimaryActive(ModuleBoost);
   bool cloakActive = isModulePrimaryActive(ModuleCloak);

   Point corners[3];
   Point shipDirs[3];

   corners[0].set(-20, -15);
   corners[1].set(  0,  25);
   corners[2].set( 20, -15);

   F32 th = FloatHalfPi - mMoveState[RenderState].angle;

   F32 sinTh = sin(th);
   F32 cosTh = cos(th);
   F32 warpInScale = (WarpFadeInTime - mWarpInTimer.getCurrent()) / F32(WarpFadeInTime);

   for(S32 i = 0; i < 3; i++)
   {
      shipDirs[i].x = corners[i].x * cosTh + corners[i].y * sinTh;
      shipDirs[i].y = corners[i].y * cosTh - corners[i].x * sinTh;
      shipDirs[i] *= warpInScale;
   }

   Point leftVec ( mMoveState[ActualState].vel.y, -mMoveState[ActualState].vel.x);
   Point rightVec(-mMoveState[ActualState].vel.y,  mMoveState[ActualState].vel.x);

   leftVec.normalize();
   rightVec.normalize();

   S32 bestId = -1, leftId, rightId;
   F32 bestDot = -1;

   // Find the left-wards match
   for(S32 i = 0; i < 3; i++)
   {
      F32 d = leftVec.dot(shipDirs[i]);
      if(d >= bestDot)
      {
         bestDot = d;
         bestId = i;
      }
   }

   leftId = bestId;
   Point leftPt = mMoveState[RenderState].pos + shipDirs[bestId];

   // Find the right-wards match
   bestId = -1;
   bestDot = -1;

   for(S32 i = 0; i < 3; i++)
   {
      F32 d = rightVec.dot(shipDirs[i]);
      if(d >= bestDot)
      {
         bestDot = d;
         bestId = i;
      }
   }

   rightId = bestId;
   Point rightPt = mMoveState[RenderState].pos + shipDirs[bestId];

   // Stitch things up if we must...
   if(leftId == mLastTrailPoint[0] && rightId == mLastTrailPoint[1])
   {
      mTrail[0].update(leftPt,  boostActive, cloakActive);
      mTrail[1].update(rightPt, boostActive, cloakActive);
      mLastTrailPoint[0] = leftId;
      mLastTrailPoint[1] = rightId;
   }
   else if(leftId == mLastTrailPoint[1] && rightId == mLastTrailPoint[0])
   {
      mTrail[1].update(leftPt,  boostActive, cloakActive);
      mTrail[0].update(rightPt, boostActive, cloakActive);
      mLastTrailPoint[1] = leftId;
      mLastTrailPoint[0] = rightId;
   }
   else
   {
      mTrail[0].update(leftPt,  boostActive, cloakActive);
      mTrail[1].update(rightPt, boostActive, cloakActive);
      mLastTrailPoint[0] = leftId;
      mLastTrailPoint[1] = rightId;
   }

   if(isModulePrimaryActive(ModuleCloak))
      return;

   // Finally, do some particles
   Point velDir(mCurrentMove.x, mCurrentMove.y);
   F32 len = velDir.len();

   if(len > 0)
   {
      if(len > 1)
         velDir *= 1 / len;

      Point shipDirs[4];
      shipDirs[0].set(cos(mMoveState[RenderState].angle), sin(mMoveState[RenderState].angle) );
      shipDirs[1].set(-shipDirs[0]);
      shipDirs[2].set(shipDirs[0].y, -shipDirs[0].x);
      shipDirs[3].set(-shipDirs[0].y, shipDirs[0].x);

      for(U32 i = 0; i < 4; i++)
      {
         F32 th = shipDirs[i].dot(velDir);

          if(th > 0.1)
          {
             // shoot some sparks...
             if(th >= 0.2*velDir.len())
             {
                Point chaos(TNL::Random::readF(),TNL::Random::readF());
                chaos *= 5;

                // interp give us some nice enginey colors...
                Color dim(Colors::red);
                Color light(1, 1, boostActive ? 1.f : 0.f);
                Color thrust;

                F32 t = TNL::Random::readF();
                thrust.interp(t, dim, light);

                TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");

                static_cast<ClientGame *>(getGame())->emitSpark(mMoveState[RenderState].pos - shipDirs[i] * 13,
                                          -shipDirs[i] * 100 + chaos, thrust, 1.5f * TNL::Random::readF());
             }
          }
      }
   }
#endif
}


extern bool gShowAimVector;

void Ship::render(S32 layerIndex)
{
#ifndef ZAP_DEDICATED
   if(layerIndex == 0)  // Only render on layers -1 and 1
      return;

   if(hasExploded)      // Don't render an exploded ship!
      return;

   F32 warpInScale = (WarpFadeInTime - mWarpInTimer.getCurrent()) / F32(WarpFadeInTime);
   F32 rotAmount = 0;      // We use rotAmount to add the spinny effect you see when a ship spawns or comes through a teleport
   if(warpInScale < 0.8f)
      rotAmount = (0.8f - warpInScale) * 540;

   // An angle of 0 means the ship is heading down the +X axis
   // since we draw the ship pointing up the Y axis, we should rotate
   // by the ship's angle, - 90 degrees
   ClientGame *clientGame = static_cast<ClientGame *>(getGame());
   GameConnection *conn = clientGame->getConnectionToServer();

   bool localShip = !(conn && conn->getControlObject() != this);    // i.e. the ship belongs to the player viewing the rendering
   S32 localPlayerTeam = (conn && conn->getControlObject()) ? conn->getControlObject()->getTeam() : NO_TEAM; // To show cloaked teammates

   // Now adjust if using cloak module
   F32 alpha = isModulePrimaryActive(ModuleCloak) ? mCloakTimer.getFraction() : 1 - mCloakTimer.getFraction();

   glPushMatrix();
   glTranslatef(mMoveState[RenderState].pos.x, mMoveState[RenderState].pos.y, 0);

   ClientInfo *clientInfo = getClientInfo();

   if(!localShip && layerIndex == 1 && clientInfo)      // Need to draw this before the glRotatef below, but only on layer 1...
   {
      string str = getClientInfo() ? clientInfo->getName().getString() : string();

      // Modify name if owner is busy
      if(clientInfo->isBusy())
         str = "<<" + str + ">>";

      TNLAssert(glIsEnabled(GL_BLEND), "Blending should be enabled here!");

      F32 textAlpha = 0.5f * alpha;
      S32 textSize = 14;

      glLineWidth(gLineWidth1);

      glColor(Colors::white, textAlpha);
      UserInterface::drawStringc(0, 30 + textSize, textSize, str.c_str());

      // Underline name if player is authenticated
      if(getClientInfo() && getClientInfo()->isAuthenticated())
      {
         S32 xoff = UserInterface::getStringWidth(textSize, str.c_str()) / 2;
         drawHorizLine(-xoff, xoff, 33 + textSize);
      }

      glLineWidth(gDefaultLineWidth);
   }

   if(clientGame->isShowingDebugShipCoords() && layerIndex == 1)
      renderShipCoords(getActualPos(), localShip, alpha);

   glRotatef(radiansToDegrees(mMoveState[RenderState].angle) - 90 + rotAmount, 0, 0, 1.0);
   glScale(warpInScale);

   if(layerIndex == -1)    // TODO: Get rid of this if we stop sending location of cloaked ship to clients
   {
      // Draw the outline of the ship in solid black -- this will block out any stars and give
      // a tantalizing hint of motion when the ship is cloaked.  Could also try some sort of star-twinkling or
      // scrambling thing here as well...
      glColor(Colors::black);

      glDisable(GL_BLEND);

      glBegin(GL_POLYGON);
         glVertex2f(-20, -15);
         glVertex2f(0, 25);
         glVertex2f(20, -15);
      glEnd();

      glEnable(GL_BLEND);


      glPopMatrix();
      return;
   }

   // LayerIndex == 1

   GameType *gameType = clientGame->getGameType();
   TNLAssert(gameType, "gameType should always be valid here");

   if(!gameType)
      return;     

   F32 thrusts[4];
   calcThrustComponents(thrusts);      // Calculate the various thrust components for rendering purposes


   // Don't completely hide local player or ships on same team
   if(localShip || (showCloakedTeammates && getTeam() == localPlayerTeam && gameType->isTeamGame()))
      alpha = max(alpha, 0.25f);     // Make sure we have at least .25 alpha
   
   if(!localShip)    // Only apply sensor-makes-cloaked-ships-visible to other ships
   {
      // If local ship has sensor, it can see cloaked non-local ships
      Ship *ship = dynamic_cast<Ship *>(conn->getControlObject());      // <-- this is our local ship
      if(ship && ship->isModulePrimaryActive(ModuleSensor) && alpha < 0.5)
         alpha = 0.5;
   }


   renderShip(gameType->getShipColor(this), alpha, thrusts, mHealth, mRadius, clientGame->getCurrentTime() - mSensorStartTime, 
              isModulePrimaryActive(ModuleCloak), isModulePrimaryActive(ModuleShield), isModulePrimaryActive(ModuleSensor), 
              isModulePrimaryActive(ModuleRepair), hasModule(ModuleArmor));

   if(localShip && gShowAimVector && mGame->getSettings()->getEnableExperimentalAimMode())   // Only show for local ship
      renderAimVector();

   glPopMatrix();

   if(mSpawnShield.getCurrent() != 0)  // Add spawn shield -- has a period of being on solidly, then blinks yellow 
   {
      static const U32 blinkStartTime = 1500;
      static const U32 blinkCycleDuration = 300;
      static const U32 blinkDuration = blinkCycleDuration / 2;       // Time shield is yellow or green during

      if(mSpawnShield.getCurrent() > blinkStartTime || mSpawnShield.getCurrent() % blinkCycleDuration > blinkDuration)
         glColor(Colors::green65);  
      else
         glColor(Colors::yellow40);

      // This rather gross looking variable helps manage problems with the resolution of F32s when getRealMilliseconds() returns a large value
      const S32 biggishNumber = 21988;
      F32 offset = F32(Platform::getRealMilliseconds() % biggishNumber) * FloatTau / biggishNumber;
      drawDashedHollowArc(mMoveState[RenderState].pos, CollisionRadius + 5, CollisionRadius + 10, 8, FloatTau / 24.0f, offset);

      // bink fading code... don't like it
      //      S32 aaa = 300;  // blink rate
      //S32 t = mSpawnShield.getCurrent() % aaa;     // t < 150, shield is off, t > 150, shield is on
      //S32 x = 100;  // duration of fade
      //if(mSpawnShield.getCurrent() > 1500 || t < x || t > aaa/2 + x)
      //{
      //   F32 tfact = 1;
      //   if(mSpawnShield.getCurrent() < 1500)
      //   {
      //      if(t > aaa/2 && t < aaa/2 + x)  // 130  // getting brighter
      //         tfact = (F32)(t - aaa/2 ) /  (F32)x;
      //      else if(t < x)  // dimming down
      //         tfact = (F32)(x-t) / (F32)x;
      //   logprintf("%f",tfact);
      //   }

   }

   if(isModulePrimaryActive(ModuleRepair) && alpha != 0)     // Don't bother when completely transparent
      renderShipRepairRays(mMoveState[RenderState].pos, this, mRepairTargets, alpha);

   // Render mounted items
   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid())
         mMountedItems[i]->renderItem(mMoveState[RenderState].pos);
#endif
}


void Ship::calcThrustComponents(F32 *thrusts)
{
   Point velDir(mCurrentMove.x, mCurrentMove.y);
   F32 len = velDir.len();

   for(U32 i = 0; i < 4; i++)
      thrusts[i] = 0;            // Reset thrusts

   if(len > 0)
   {
      if(len > 1)
         velDir *= 1 / len;

      Point shipDirs[4];
      shipDirs[0].set(cos(mMoveState[RenderState].angle), sin(mMoveState[RenderState].angle) );
      shipDirs[1].set(-shipDirs[0]);
      shipDirs[2].set( shipDirs[0].y, -shipDirs[0].x);
      shipDirs[3].set(-shipDirs[0].y,  shipDirs[0].x);

      for(U32 i = 0; i < ARRAYSIZE(shipDirs); i++)
         thrusts[i] = shipDirs[i].dot(velDir);
   }

   // Tweak side thrusters to show rotational force
   F32 rotVel = getAngleDiff(mMoveState[LastProcessState].angle, mMoveState[RenderState].angle);

   if(rotVel > 0.001)
      thrusts[3] += 0.25;
   else if(rotVel < -0.001)
      thrusts[2] += 0.25;

   
   if(isModulePrimaryActive(ModuleBoost))
      for(U32 i = 0; i < 4; i++)
         thrusts[i] *= 1.3f;
}


bool Ship::isRobot()
{
   return mIsRobot;
}


S32 LuaShip::id = 99;

const char LuaShip::className[] = "Ship";      // Class name as it appears to Lua scripts

// Note that when adding a method here, also add it to LuaRobot so that it can inherit these methods
Lunar<LuaShip>::RegType LuaShip::methods[] = {
   method(LuaShip, getClassID),
   method(LuaShip, isAlive),

   method(LuaShip, getLoc),
   method(LuaShip, getRad),
   method(LuaShip, getVel),
   method(LuaShip, getTeamIndx),
   method(LuaShip, getPlayerInfo),

   method(LuaShip, isModActive),
   method(LuaShip, getEnergy),
   method(LuaShip, getHealth),
   method(LuaShip, hasFlag),

   method(LuaShip, getAngle),
   method(LuaShip, getActiveWeapon),
   method(LuaShip, getMountedItems),
   method(LuaShip, getCurrLoadout),
   method(LuaShip, getReqLoadout),

   {0,0}    // End method list
};


// C++ constructor -- automatically constructed when a ship is created
// This is the only constructor that's used.
LuaShip::LuaShip(Ship *ship): thisShip(ship)
{
   id++;
   mId = id;
   logprintf(LogConsumer::LogLuaObjectLifecycle, "Creating luaship %d", mId);
}


// C++ default constructor ==> not used.  Constructor with Ship (above) used instead
LuaShip::LuaShip()
{
   /* Do nothing */
};


// Lua constructor ==> not used.  Class only instantiated from C++.
LuaShip::LuaShip(lua_State *L)
{
   /* Do nothing */
};


// Destructor
LuaShip::~LuaShip()
{
   logprintf(LogConsumer::LogLuaObjectLifecycle, "Killing luaShip %d", mId);
};


S32 LuaShip::getClassID(lua_State *L) { return returnInt(L, PlayerShipTypeNumber); }
S32 LuaShip::isAlive(lua_State *L) { return returnBool(L, thisShip.isValid()); }

// Note: All of these methods will return nil if the ship in question has been deleted.
S32 LuaShip::getRad(lua_State *L)  { return thisShip ? returnFloat(L, thisShip->getRadius())    : returnNil(L); }
S32 LuaShip::getLoc(lua_State *L)  { return thisShip ? returnPoint(L, thisShip->getActualPos()) : returnNil(L); }
S32 LuaShip::getVel(lua_State *L)  { return thisShip ? returnPoint(L, thisShip->getActualVel()) : returnNil(L); }
S32 LuaShip::hasFlag(lua_State *L) { return thisShip ? returnBool(L,  thisShip->getFlagCount()) : returnNil(L); }

// Returns number of flags ship is carrying (most games will always be 0 or 1)
S32 LuaShip::getFlagCount(lua_State *L) { return thisShip ? returnInt(L, thisShip->getFlagCount()) : returnNil(L); }


S32 LuaShip::getTeamIndx(lua_State *L) { return returnInt(L, thisShip->getTeam() + 1); }

S32 LuaShip::getPlayerInfo(lua_State *L) { return thisShip ? returnPlayerInfo(L, thisShip) : returnNil(L); }


S32 LuaShip::isModActive(lua_State *L) {
   static const char *methodName = "Ship:isModActive()";
   checkArgCount(L, 1, methodName);
   ShipModule module = (ShipModule) getInt(L, 1, methodName, 0, ModuleCount - 1);
   return thisShip ? returnBool(L, getObj()->isModulePrimaryActive(module) || getObj()->isModuleSecondaryActive(module)) : returnNil(L);
}

S32 LuaShip::getAngle(lua_State *L) { return thisShip ? returnFloat(L, getObj()->getCurrentMove().angle) : returnNil(L); }      // Get angle ship is pointing at
S32 LuaShip::getActiveWeapon(lua_State *L) { return thisShip ?  returnInt(L, getObj()->getSelectedWeapon()) : returnNil(L); }    // Get WeaponIndex for current weapon

S32 LuaShip::getEnergy(lua_State *L) { return thisShip ? returnFloat(L, thisShip->getEnergyFraction()) : returnNil(L); }        // Return ship's energy as a fraction between 0 and 1
S32 LuaShip::getHealth(lua_State *L) { return thisShip ? returnFloat(L, thisShip->getHealth()) : returnNil(L); }                // Return ship's health as a fraction between 0 and 1

S32 LuaShip::getMountedItems(lua_State *L)
{
   bool hasArgs = lua_isnumber(L, 1);
   Vector<GameObject *> tempVector;

   // Loop through all the mounted items
   for(S32 i = 0; i < thisShip->mMountedItems.size(); i++)
   {
      // Add every item to the list if no arguments were specified
      if(!hasArgs)
         tempVector.push_back(dynamic_cast<GameObject *>(thisShip->mMountedItems[i].getPointer()));

      // Else, compare against argument type and add to the list if matched
      else
      {
         S32 index = 1;
         while(lua_isnumber(L, index))
         {
            U8 objectType = (U8) lua_tointeger(L, index);

            if(thisShip->mMountedItems[i]->getObjectTypeNumber() == objectType)
            {
               tempVector.push_back(dynamic_cast<GameObject *>(thisShip->mMountedItems[i].getPointer()));
               break;
            }

            index++;
         }
      }
   }

   clearStack(L);

   if(!thisShip) return 0;  // if NULL, what to do here?

   lua_createtable(L, thisShip->mMountedItems.size(), 0);    // Create a table, with enough slots pre-allocated for our data

   // Now push all found items back to LUA
   S32 pushed = 0;      // Count of items actually pushed onto the stack

   for(S32 i = 0; i < tempVector.size(); i++)
   {
      tempVector[i]->push(L);
      pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   return 1;
}

// Return current loadout
S32 LuaShip::getCurrLoadout(lua_State *L)
{
   U8 loadoutItems[ShipModuleCount + ShipWeaponCount];

   for(S32 i = 0; i < ShipModuleCount; i++)
      loadoutItems[i] = (U8)thisShip->getModule(i);

   for(S32 i = 0; i < ShipWeaponCount; i++)
      loadoutItems[i + ShipModuleCount] = (U8)thisShip->getWeapon(i);

   LuaLoadout *loadout = new LuaLoadout(loadoutItems);
   Lunar<LuaLoadout>::push(L, loadout, true);     // true will allow Lua to delete this object when it goes out of scope

   return 1;
}


// Return requested loadout
S32 LuaShip::getReqLoadout(lua_State *L)
{
   U8 loadoutItems[ShipModuleCount + ShipWeaponCount];
   ClientInfo *clientInfo = thisShip->getOwner();

   const Vector<U8> requestedLoadout = clientInfo ? clientInfo->getLoadout() : Vector<U8>();

   if(!clientInfo || requestedLoadout.size() != ShipModuleCount + ShipWeaponCount)    // Robots and clients starts at zero size requested loadout
      return getCurrLoadout(L);

   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      loadoutItems[i] = requestedLoadout[i];

   LuaLoadout *loadout = new LuaLoadout(loadoutItems);
   Lunar<LuaLoadout>::push(L, loadout, true);     // true will allow Lua to delete this object when it goes out of scope

   return 1;
}


GameObject *LuaShip::getGameObject()
{
   if(thisShip.isNull())    // This will only happen when thisShip is dead, and therefore developer has made a mistake.  So let's throw up a scolding error message!
   {
      logprintf(LogConsumer::LuaBotMessage, "Bad programmer!");
      return NULL;      // Not right
   }
   else
      return getObj();
}


const char *LuaShip::getClassName() const
{
   return "LuaShip";
}


void LuaShip::push(lua_State *L)
{
   Lunar<LuaShip>::push(L, this, false);
}


Ship *LuaShip::getObj()
{
   return thisShip;
}


Ship *Ship::clone() const
{
   return new Ship(*this);
}




};

