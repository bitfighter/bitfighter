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

#include "sparkManager.h"
#include "projectile.h"
#include "gameLoader.h"
#include "sfx.h"
#include "UI.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "gameType.h"
#include "huntersGame.h"
#include "gameConnection.h"
#include "shipItems.h"
#include "speedZone.h"
#include "gameWeapons.h"
#include "gameObjectRender.h"
#include "config.h"
#include "statistics.h"
#include "SlipZone.h"

#include "stringUtils.h"      // For itos

#include "glutInclude.h"

#include <stdio.h>

#define hypot _hypot    // Kill some warnings

static bool showCloakedTeammates = false;    // Set to true to allow players to see their cloaked teammates

namespace Zap
{

static Vector<DatabaseObject *> fillVector;

TNL_IMPLEMENT_NETOBJECT(Ship);

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

// Constructor
// Note that most of these values are set in the initial packet set from the server (see packUpdate() below)
// Also, the following is also run by robot's constructor
Ship::Ship(StringTableEntry playerName, bool isAuthenticated, S32 team, Point p, F32 m, bool isRobot) : MoveObject(p, CollisionRadius), mSpawnPoint(p)
{
   mObjectTypeMask = ShipType | MoveableType | CommandMapVisType | TurretTargetType;

   mNetFlags.set(Ghostable);

   for(U32 i = 0; i < TrailCount; i++)
      mLastTrailPoint[i] = -1;   // Or something... doesn't really matter what

   mTeam = team;
   mass = m;            // Ship's mass, not used

   // Name will be unique across all clients, but client and server may disagree on this name if the server has modified it to make it unique
   mPlayerName = playerName;  
   mIsAuthenticated = isAuthenticated;

   mIsRobot = isRobot;

   if(!isRobot)         // Robots will run this during their own initialization; no need to run it twice!
      initialize(p);

   isBusy = false;      // On client, will be updated in initial packet set from server.  Not used on server.

   mSparkElapsed = 0;

   // Create our proxy object for Lua access
   luaProxy = LuaShip(this);
   //SlipZoneObject = NULL;
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
   if(getGame())
      mRespawnTime = getGame()->getCurrentTime();
   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].pos = pos;
      mMoveState[i].angle = 0;
      mMoveState[i].vel = Point(0,0);
   }

   updateExtent();

   mHealth = 1.0;       // Start at full health
   hasExploded = false; // Haven't exploded yet!

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

   mActiveWeaponIndx = 0;
   mCooldown = false;
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
      mModuleActive[i] = false;
   updateModuleSounds();
}


bool Ship::processArguments(S32 argc, const char **argv)
{
   if(argc != 3)
      return false;

   Point pos;
   pos.read(argv + 1);
   pos *= getGame()->getGridSize();
   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].pos = pos;
      mMoveState[i].angle = 0;
   }

   updateExtent();

   return true;
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
   const F32 ARMOR_ACCEL_PENALTY_FACT = 0.35;
   const F32 ARMOR_SPEED_PENALTY_FACT = 1;

   mMoveState[LastProcessState] = mMoveState[stateIndex];

   F32 maxVel = (isModuleActive(ModuleBoost) ? BoostMaxVelocity : MaxVelocity) * 
                (hasModule(ModuleArmor) ? ARMOR_SPEED_PENALTY_FACT : 1);

   F32 time = mCurrentMove.time * 0.001;
   Point requestVel(mCurrentMove.right - mCurrentMove.left, mCurrentMove.down - mCurrentMove.up);

   requestVel *= maxVel;
   F32 len = requestVel.len();

   if(len > maxVel)
      requestVel *= maxVel / len;

   Point velDelta = requestVel - mMoveState[stateIndex].vel;
   F32 accRequested = velDelta.len();


   // Apply turbo-boost if active, reduce accel and max vel when armor is present
   F32 maxAccel = (isModuleActive(ModuleBoost) ? BoostAcceleration : Acceleration) * time * 
                  (hasModule(ModuleArmor) ? ARMOR_ACCEL_PENALTY_FACT : 1);
	maxAccel *= slipZoneMultiply();

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
void Ship::findObjectsUnderShip(GameObjectType type)
{
   Rect rect(getActualPos(), getActualPos());
   rect.expand(Point(CollisionRadius, CollisionRadius));

   fillVector.clear();           // This vector will hold any matching zones
   findObjects(type, fillVector, rect);
}


extern bool PolygonContains2(const Point *inVertices, int inNumVertices, const Point &inPoint);

// Returns the zone in question if this ship is in a zone of type zoneType
GameObject *Ship::isInZone(GameObjectType zoneType)
{
   findObjectsUnderShip(zoneType);

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
GameObject *Ship::isInZone(GameObject *zone)
{
   // Get points that define the zone boundaries
   Vector<Point> polyPoints;
   polyPoints.clear();
   zone->getCollisionPoly(polyPoints);

   if( PolygonContains2(polyPoints.address(), polyPoints.size(), getActualPos()) )
      return zone;
   return NULL;
}

F32 Ship::slipZoneMultiply()
{
   SlipZone *slipzone = dynamic_cast<SlipZone *>(isInZone(SlipZoneType));
   return slipzone ? slipzone->slipAmount : 1.0;
}


// Returns the object in question if this ship is on an object of type objectType
DatabaseObject *Ship::isOnObject(GameObjectType objectType)
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
   Vector<Point> polyPoints;

   // Ships don't have collisionPolys, so this first check is utterly unneeded unless we change that
   if(getCollisionPoly(polyPoints))
      return object->collisionPolyPointIntersect(polyPoints);
   else if(getCollisionCircle(MoveObject::ActualState, center, radius))
      return object->collisionPolyPointIntersect(center, radius);

   else
      return false;
}


 // Returns vector for aiming a weapon based on direction ship is facing
Point Ship::getAimVector()
{
   return Point(cos(mMoveState[ActualState].angle), sin(mMoveState[ActualState].angle) );
}


void Ship::selectWeapon()
{
   selectWeapon(mActiveWeaponIndx + 1);
}


extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;

void Ship::selectWeapon(U32 weaponIdx)
{
   mActiveWeaponIndx = weaponIdx % ShipWeaponCount;      // Advance index to next weapon

   // Display a message confirming new weapon choice if we're not showing the indicators
   if (!gIniSettings.showWeaponIndicators)
   {
      GameConnection *cc = getControllingClient();
      if(cc)
      {
         Vector<StringTableEntry> e;
         e.push_back(gWeapons[mWeapon[mActiveWeaponIndx]].name);
         static StringTableEntry msg("%e0 selected.");
         cc->s2cDisplayMessageE(GameConnection::ColorAqua, SFXUIBoop, msg, e);
      }
   }
}


void Ship::processWeaponFire()
{
   mFireTimer.update(mCurrentMove.time);
   mWeaponFireDecloakTimer.update(mCurrentMove.time);

   WeaponType curWeapon = mWeapon[mActiveWeaponIndx];

   if(mCurrentMove.fire && mFireTimer.getCurrent() == 0 && getGame()->getGameType() && getGame()->getGameType()->onFire(this))
   {
      if(mEnergy >= gWeapons[curWeapon].minEnergy)
      {
         mEnergy -= gWeapons[curWeapon].drainEnergy;
         mFireTimer.reset(gWeapons[curWeapon].fireDelay);
         mWeaponFireDecloakTimer.reset(WeaponFireDecloakTime);

         if(getControllingClient().isValid())
            getControllingClient()->getClientRef()->mStatistics.countShot(curWeapon);

         if(!isGhost())    // i.e. server only
         {
            Point dir = getAimVector();
            createWeaponProjectiles(curWeapon, dir, mMoveState[ActualState].pos, mMoveState[ActualState].vel, CollisionRadius - 2, this);
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

   // if the delta is either very small, or greater than the
   // max interpolation threshold, just warp to the new position
   if(deltaLen <= 0.5 || deltaLen > MaxControlObjectInterpDistance)
   {
      // If it's a large delta, get rid of the movement trails
      if(deltaLen > MaxControlObjectInterpDistance)
         for(S32 i=0; i<TrailCount; i++)
            mTrail[i].reset();

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

   Parent::idle(path);

   if(path == GameObject::ServerIdleMainLoop && isControlled())
   {
      // If this is a controlled object in the server's main
      // idle loop, process the render state forward -- this
      // is what projectiles will collide against.  This allows
      // clients to properly lead other clients, instead of
      // piecewise stepping only when packets arrive from the client.
      processMove(RenderState);
      setMaskBits(PositionMask);
   }
   else
   {
      // For all other cases, advance the actual state of the
      // object with the current move.
      processMove(ActualState);

      // Apply impulse vector and reset it
      mMoveState[ActualState].vel += mImpulseVector;
      mImpulseVector.set(0,0);

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
         if(    mMoveState[RenderState].angle != mMoveState[ActualState].angle
             || mMoveState[RenderState].pos != mMoveState[ActualState].pos
             || mMoveState[RenderState].vel != mMoveState[ActualState].vel )
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
   }

   // Update the object in the game's extents database
   updateExtent();

   // If this is a move executing on the server and it's
   // different from the last move, then mark the move to
   // be updated to the ghosts.
   if(path == GameObject::ServerIdleControlFromClient && !mCurrentMove.isEqualMove(&mLastMove))
      setMaskBits(MoveMask);

   mLastMove = mCurrentMove;
   mSensorZoomTimer.update(mCurrentMove.time);
   mCloakTimer.update(mCurrentMove.time);

   if(path == GameObject::ServerIdleControlFromClient ||
      path == GameObject::ClientIdleControlMain ||
      path == GameObject::ClientIdleControlReplay)
   {
      // Process weapons and energy on controlled object objects
      processWeaponFire();
      processEnergy();     // and modules
   }
     
   if(path == GameObject::ClientIdleMainRemote)
   {
      // For ghosts, find some repair targets for rendering the repair effect
      if(isModuleActive(ModuleRepair))
         findRepairTargets();
   }
   if(path == GameObject::ServerIdleControlFromClient && isModuleActive(ModuleRepair))
      repairTargets();

   if(path == GameObject::ClientIdleControlMain ||
      path == GameObject::ClientIdleMainRemote)
   {
      mWarpInTimer.update(mCurrentMove.time);
      // Emit some particles, trail sections and update the turbo noise
      emitMovementSparks();
      for(U32 i=0; i<TrailCount; i++)
         mTrail[i].tick(mCurrentMove.time);
      updateModuleSounds();
   }
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
   Point extend(RepairRadius, RepairRadius);
   Rect r(pos - extend, pos + extend);
   
   foundObjects.clear();
   findObjects(ShipType | RobotType | EngineeredType, foundObjects, r);

   mRepairTargets.clear();
   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      GameObject *s = dynamic_cast<GameObject *>(foundObjects[i]);
      if(s->isDestroyed() || s->getHealth() >= 1)                             // Don't repair dead or fully healed objects...
         continue;
      if((s->getRenderPos() - pos).len() > (RepairRadius + CollisionRadius))  // ...or ones too far away...
         continue;
      if(s->getTeam() != -1 && s->getTeam() != getTeam())                     // ...or ones not on our team or neutral
         continue;
      mRepairTargets.push_back(s);
   }
   return mRepairTargets.size() != 0;
}


// Repairs ALL repair targets found above
void Ship::repairTargets()
{
   F32 totalRepair = RepairHundredthsPerSecond * 0.01 * mCurrentMove.time * 0.001f;

//   totalRepair /= mRepairTargets.size();

   DamageInfo di;
   di.damageAmount = -totalRepair;
   di.damagingObject = this;
   di.damageType = DamageTypePoint;

   for(S32 i = 0; i < mRepairTargets.size(); i++)
      mRepairTargets[i]->damageObject(&di);
}


void Ship::processEnergy()
{
   bool modActive[ModuleCount];
   for(S32 i = 0; i < ModuleCount; i++)
   {
      modActive[i] = mModuleActive[i];
      mModuleActive[i] = false;
   }

   if(mEnergy > EnergyCooldownThreshold)     // Only turn off cooldown if energy has risen above threshold, not if it falls below
      mCooldown = false;

   // Make sure we're allowed to use modules
   bool allowed = getGame()->getGameType() && getGame()->getGameType()->okToUseModules(this);

   // Are these checked on the server side?
   for(S32 i = 0; i < ShipModuleCount; i++)   
      // If you have passive module, it's always active, no restrictions, but is off for energy consumption purposes
      if(getGame()->getModuleInfo(mModule[i])->getUseType() == ModuleUsePassive)    
         mModuleActive[mModule[i]] = false;         

      // No (active) modules if we're too hot or game has disallowed them
      else if(mCurrentMove.module[i] && !mCooldown && allowed)  
         mModuleActive[mModule[i]] = true;


   // No boost if we're not moving
    if(mModuleActive[ModuleBoost] &&
       mCurrentMove.up == 0 && mCurrentMove.down == 0 &&
       mCurrentMove.left == 0 && mCurrentMove.right == 0)
   {
      mModuleActive[ModuleBoost] = false;
   }

   // No repair with no targets
   if(mModuleActive[ModuleRepair] && !findRepairTargets())
      mModuleActive[ModuleRepair] = false;

   // No cloak with nearby sensored people
   if(mModuleActive[ModuleCloak])
   {
      if(mWeaponFireDecloakTimer.getCurrent() != 0)
         mModuleActive[ModuleCloak] = false;
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

   F32 scaleFactor = mCurrentMove.time * 0.001;

   // Update things based on available energy...
   bool anyActive = false;
   for(S32 i = 0; i < ModuleCount; i++)
   {
      if(mModuleActive[i])
      {
         mEnergy -= S32(getGame()->getModuleInfo((ShipModule) i)->getEnergyDrain() * scaleFactor);
         anyActive = true;
      }
   }

   if(!anyActive && mEnergy <= EnergyCooldownThreshold)
      mCooldown = true;

   if(mEnergy < EnergyMax)
   {
      // If we're not doing anything, recharge.
      if(!anyActive)
         mEnergy += S32(EnergyRechargeRate * scaleFactor);

      if(mEnergy <= 0)
      {
         mEnergy = 0;
         for(S32 i = 0; i < ModuleCount; i++)
            mModuleActive[i] = false;
         mCooldown = true;
      }
   }

   if(mEnergy >= EnergyMax)
      mEnergy = EnergyMax;

   for(S32 i = 0; i < ModuleCount;i++)
   {
      if(mModuleActive[i] != modActive[i])
      {
         if(i == ModuleSensor)
         {
            mSensorZoomTimer.reset(SensorZoomTime - mSensorZoomTimer.getCurrent(), SensorZoomTime);
            mSensorStartTime = getGame()->getCurrentTime();
         }
         else if(i == ModuleCloak)
            mCloakTimer.reset(CloakFadeTime - mCloakTimer.getCurrent(), CloakFadeTime);

         setMaskBits(ModulesMask);
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
      if(isModuleActive(ModuleShield)) // && mEnergy >= EnergyShieldHitDrain)     // Commented code will cause
      {                                                                           // shields to drain when they
         //mEnergy -= EnergyShieldHitDrain;                                       // have been hit.
         return;
      }

      // Having armor halves the damage
      if(hasModule(ModuleArmor))
         damageAmount /= 2;
   }

   GameConnection *damagerOwner = theInfo->damagingObject->getOwner();
   GameConnection *victimOwner = this->getOwner();

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
}


// Runs when ship spawns -- runs on client and server
void Ship::onAddedToGame(Game *game)
{
   Parent::onAddedToGame(game);

   // Detect if we spawned on a GoFast
   SpeedZone *speedZone = dynamic_cast<SpeedZone *>(isOnObject(SpeedZoneType));
   if(speedZone)
      speedZone->collide(this);

   // From here on down, server only
   if(!isGhost())
   {
      mRespawnTime = getGame()->getCurrentTime();
      Robot::getEventManager().fireEvent(EventManager::ShipSpawnedEvent, this);
   }
}


void Ship::updateModuleSounds()
{
   static S32 moduleSFXs[ModuleCount] =
   {
      SFXShieldActive,
      SFXShipBoost,
      SFXSensorActive,
      SFXRepairActive,
      SFXUIBoop, // Need better sound...
      SFXCloakActive,
   };

   for(U32 i = 0; i < ModuleCount; i++)
   {
      if(mModuleActive[i])
      {
         if(mModuleSound[i].isValid())
            mModuleSound[i]->setMovementParams(mMoveState[RenderState].pos, mMoveState[RenderState].vel);
         else if(moduleSFXs[i] != -1)
            mModuleSound[i] = SFXObject::play(moduleSFXs[i], mMoveState[RenderState].pos, mMoveState[RenderState].vel);
      }
      else
      {
         if(mModuleSound[i].isValid())
         {
            mModuleSound[i]->stop();
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
      if(gWeapons[i].fireDelay > MaxFireDelay)
         MaxFireDelay = gWeapons[i].fireDelay;
   }
}

void Ship::writeControlState(BitStream *stream)
{
   stream->write(mMoveState[ActualState].pos.x);
   stream->write(mMoveState[ActualState].pos.y);
   stream->write(mMoveState[ActualState].vel.x);
   stream->write(mMoveState[ActualState].vel.y);
   stream->writeRangedU32(mEnergy, 0, EnergyMax);
   stream->writeFlag(mCooldown);
   stream->writeRangedU32(mFireTimer.getCurrent(), 0, MaxFireDelay);
   stream->writeRangedU32(mActiveWeaponIndx, 0, WeaponCount);
}

void Ship::readControlState(BitStream *stream)
{
   stream->read(&mMoveState[ActualState].pos.x);
   stream->read(&mMoveState[ActualState].pos.y);
   stream->read(&mMoveState[ActualState].vel.x);
   stream->read(&mMoveState[ActualState].vel.y);
   mEnergy = stream->readRangedU32(0, EnergyMax);
   mCooldown = stream->readFlag();
   U32 fireTimer = stream->readRangedU32(0, MaxFireDelay);
   mFireTimer.reset(fireTimer);
   mActiveWeaponIndx = stream->readRangedU32(0, WeaponCount);
}


// Transmit ship status from server to client
// Any changes here need to be reflected in Ship::unpackUpdate
U32 Ship::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   GameConnection *gameConnection = (GameConnection *) connection;

   if(isInitialUpdate())      // This stuff gets sent only once per ship
   {
      stream->writeFlag(getGame()->getCurrentTime() - mRespawnTime < 300);  // If true, ship will appear to spawn on client
      //updateMask |= ChangeTeamMask;  // make this bit true to write team.

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

   if(stream->writeFlag(updateMask & AuthenticationMask))     // Player authentication status changed
   {
      stream->writeStringTableEntry(mPlayerName);
      stream->writeFlag(mIsAuthenticated);
   }

   if(stream->writeFlag(updateMask & ChangeTeamMask))   // A player with admin can change robots teams.
   {
      stream->write(mTeam);
   }


//if(isRobot())
//{
//Robot *robot = dynamic_cast<Robot *>(this);
//stream->write((S32)robot->mTarget.x);
//stream->write((S32)robot->mTarget.y);
//
//stream->write(robot->flightPlan.size());
//   for(S32 i = 0; i < robot->flightPlan.size(); i++)
//   {
//      stream->write(robot->flightPlan[i].x);
//      stream->write(robot->flightPlan[i].y);
//   }
//}


   // Respawn --> only used by robots, but will be set on ships if all mask bits
   // are set (as happens when a ship comes into scope).  Therefore, we'll force
   // this to be robot only.
   if(stream->writeFlag(updateMask & RespawnMask && isRobot()))
      stream->writeFlag(getGame()->getCurrentTime() - mRespawnTime < 300);  // If true, ship will appear to spawn on client

   if(stream->writeFlag(updateMask & HealthMask))     // Health
      stream->writeFloat(mHealth, 6);

   if(stream->writeFlag(updateMask & LoadoutMask))    // Module configuration
   {
      for(S32 i = 0; i < ShipModuleCount; i++)
         stream->writeEnum(mModule[i], ModuleCount);

      for(S32 i = 0; i < ShipWeaponCount; i++)
         stream->writeEnum(mWeapon[i], WeaponCount);
   }

   stream->writeFlag(hasExploded);
   stream->writeFlag(getControllingClient()->isBusy());

   stream->writeFlag(updateMask & WarpPositionMask && updateMask != 0xFFFFFFFF);   

   // Don't show warp effect when all mask flags are set, as happens when ship comes into scope
   stream->writeFlag(updateMask & TeleportMask && !(updateMask & InitialMask));      

   bool shouldWritePosition = (updateMask & InitialMask) || gameConnection->getControlObject() != this;
   if(!shouldWritePosition)
   {
      stream->writeFlag(false);
      stream->writeFlag(false);
      stream->writeFlag(false);
   }
   else     // Write mCurrentMove data...
   {
      if(stream->writeFlag(updateMask & PositionMask))
      {
         // Send position and speed
         gameConnection->writeCompressedPoint(mMoveState[RenderState].pos, stream);
         writeCompressedVelocity(mMoveState[RenderState].vel, BoostMaxVelocity + 1, stream);
      }
      if(stream->writeFlag(updateMask & MoveMask))
         mCurrentMove.pack(stream, NULL, false);      // Send current move

      if(stream->writeFlag(updateMask & ModulesMask))
         for(S32 i = 0; i < ModuleCount; i++)         // Send info about which modules are active
            stream->writeFlag(mModuleActive[i]);
   }
   return 0;
}


// Any changes here need to be reflected in Ship::packUpdate
void Ship::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool positionChanged = false;    // True when position changes a little
   bool shipwarped = false;         // True when position changes a lot

   bool wasInitialUpdate = false;
   bool playSpawnEffect = false;


   if(isInitialUpdate())
   {
      wasInitialUpdate = true;
      shipwarped = true;
      playSpawnEffect = stream->readFlag();

      // Read mounted items:
      while(stream->readFlag())
      {
         S32 index = stream->readInt(GhostConnection::GhostIdBitSize);
         Item *theItem = (Item *) connection->resolveGhost(index);
         theItem->mountToShip(this);
      }

   }  // initial update


   if(stream->readFlag())     // Player authentication status changed
   {
      stream->readStringTableEntry(&mPlayerName);
      mIsAuthenticated = stream->readFlag();
   }

   if(stream->readFlag())     // Team changed
   {
      stream->read(&mTeam);
   }

//if(isRobot())
//{
//Robot *robot = dynamic_cast<Robot *>(this);
//S32 x;
//S32 y;
//stream->read(&x);
//stream->read(&y);
//
//robot->mTarget.x = x;
//robot->mTarget.y = y;
//
//S32 ttt;
//stream->read(&ttt);
//robot->flightPlan.clear();
//for(S32 i = 0; i < ttt; i++)
//{
//   F32 x,y;
//   stream->read(&x);
//   stream->read(&y);
//   Point p(x,y);
//   robot->flightPlan.push_back(p);
//}
//
//}


   if(stream->readFlag())        // Respawn <--- will only occur on robots, will always be false with ships
   {
      hasExploded = false;
      playSpawnEffect = stream->readFlag();    // prevent spawn effect every time the robot goes into scope.
      shipwarped = true;
      if(! isCollisionEnabled()) enableCollision();
   }

   if(stream->readFlag())        // Health
      mHealth = stream->readFloat(6);

   if(stream->readFlag())        // New module configuration
   {
      for(S32 i = 0; i < ShipModuleCount; i++)
         mModule[i] = (ShipModule) stream->readEnum(ModuleCount);

      for(S32 i = 0; i < ShipWeaponCount; i++)
         mWeapon[i] = (WeaponType) stream->readEnum(WeaponCount);
   }

   bool explode = stream->readFlag();
   isBusy = stream->readFlag();

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

   if(stream->readFlag())     // ModulesMask
   {
      bool wasActive[ModuleCount];
      for(S32 i = 0; i < ModuleCount; i++)
      {
         wasActive[i] = mModuleActive[i];
         mModuleActive[i] = stream->readFlag();
         if(i == ModuleSensor && wasActive[i] != mModuleActive[i])
         {
            mSensorZoomTimer.reset(SensorZoomTime - mSensorZoomTimer.getCurrent(), SensorZoomTime);
            mSensorStartTime = gClientGame->getCurrentTime();
         }
         if(i == ModuleCloak && wasActive[i] != mModuleActive[i])
            mCloakTimer.reset(CloakFadeTime - mCloakTimer.getCurrent(), CloakFadeTime);
      }
   }

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


   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();

      if(!wasInitialUpdate)
         emitShipExplosion(mMoveState[ActualState].pos);    // Boom!
   }

   if(playSpawnEffect)
   {
      mWarpInTimer.reset(WarpFadeInTime);    // Make ship all spinny

      FXManager::emitTeleportInEffect(mMoveState[ActualState].pos, 1);
      SFXObject::play(SFXTeleportIn, mMoveState[ActualState].pos, Point());
   }

}  // unpackUpdate


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


// Returns index of first flag mounted on ship, or NO_FLAG if there aren't any
S32 Ship::carryingFlag()
{
   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid() && (mMountedItems[i]->getObjectTypeMask() & FlagType))
         return i;
   return GameType::NO_FLAG;
}


S32 Ship::getFlagCount()
{
   S32 count = 0;
   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid() && (mMountedItems[i]->getObjectTypeMask() & FlagType))
      {
         HuntersFlagItem *flag = dynamic_cast<HuntersFlagItem *>(mMountedItems[i].getPointer());
         if(flag != NULL)   // Nexus flag have multiple flags as one item.
            count += flag->getFlagCount();
         else
            count++;
      }
   return count;
}


bool Ship::isCarryingItem(GameObjectType objectType)
{
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
      if(mMountedItems[i].isValid() && mMountedItems[i]->getObjectTypeMask() & objectType)
         return true;
   return false;
}


Item *Ship::unmountItem(GameObjectType objectType)
{
   //logprintf("%s ship->unmountItem", isGhost()? "Client:" : "Server:");
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
   {
      if(mMountedItems[i]->getObjectTypeMask() & objectType)
      {
         Item *item = mMountedItems[i];
         item->dismount();
         return item;
      }
   }
   return NULL;
}


void Ship::setLoadout(const Vector<U32> &loadout)
{
   // Check to see if the new configuration is the same as the old.  If so, we have nothing to do.
   bool theSame = true;

   for(S32 i = 0; i < ShipModuleCount; i++)
      theSame = theSame && (loadout[i] == (U32)mModule[i]);

   for(S32 i = ShipModuleCount; i < ShipWeaponCount + ShipModuleCount; i++)
      theSame = theSame && (loadout[i] == (U32)mWeapon[i - ShipModuleCount]);

   if(theSame)      // Don't bother if ship config hasn't changed
      return;

   WeaponType currentWeapon = mWeapon[mActiveWeaponIndx];

   for(S32 i = 0; i < ShipModuleCount; i++)
      mModule[i] = (ShipModule) loadout[i];

   for(S32 i = ShipModuleCount; i < ShipWeaponCount + ShipModuleCount; i++)
      mWeapon[i - ShipModuleCount] = (WeaponType) loadout[i];

   setMaskBits(LoadoutMask);

   // Try to see if we can maintain the same weapon we had before.
   S32 i;
   for(i = 0; i < ShipWeaponCount; i++)
      if(mWeapon[i] == currentWeapon)
      {
         mActiveWeaponIndx = i;
         break;
      }

   if(i == ShipWeaponCount)   // Nope...
      selectWeapon(0);        // ... so select first weapon

   if(!hasModule(ModuleEngineer))        // We don't, so drop any resources we may be carrying
      for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
         if(mMountedItems[i]->getObjectTypeMask() & ResourceItemType)
            mMountedItems[i]->dismount();

   // And notifiy user
   GameConnection *cc = getControllingClient();
   if(!cc)
      cc = gClientGame->getConnectionToServer();      // Second try

   if(cc)
   {
      static StringTableEntry msg("Ship loadout configuration updated.");
      cc->s2cDisplayMessage(GameConnection::ColorAqua, SFXUIBoop, msg);
   }
}


void Ship::kill(DamageInfo *theInfo)
{
   if(isGhost())     // Server only, please...
      return;

   GameConnection *controllingClient = getControllingClient();
   if(controllingClient)
   {
      GameType *gt = getGame()->getGameType();
      if(gt)
         gt->controlObjectForClientKilled(controllingClient, this, theInfo->damagingObject);
   }

   kill();
}


void Ship::kill()
{
   if(!isGhost())
      Robot::getEventManager().fireEvent(EventManager::ShipKilledEvent, this);
   else
      S32 x = 0;     // TODO: Delete this

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
   Color(1, 0, 0),
   Color(0.9, 0.5, 0),
   Color(1, 1, 1),
   Color(1, 1, 0),
   Color(1, 0, 0),
   Color(0.8, 1.0, 0),
   Color(1, 0.5, 0),
   Color(1, 1, 1),
   Color(1, 0, 0),
   Color(0.9, 0.5, 0),
   Color(1, 1, 1),
   Color(1, 1, 0),
};

void Ship::emitShipExplosion(Point pos)
{
   SFXObject::play(SFXShipExplode, pos, Point());

   F32 a = TNL::Random::readF() * 0.4 + 0.5;
   F32 b = TNL::Random::readF() * 0.2 + 0.9;

   F32 c = TNL::Random::readF() * 0.15 + 0.125;
   F32 d = TNL::Random::readF() * 0.2 + 0.9;

   FXManager::emitExplosion(mMoveState[ActualState].pos, 0.9, ShipExplosionColors, NumShipExplosionColors);
   FXManager::emitBurst(pos, Point(a,c), Color(1,1,0.25), Color(1,0,0));
   FXManager::emitBurst(pos, Point(b,d), Color(1,1,0), Color(0,0.75,0));
}

void Ship::emitMovementSparks()
{
   //U32 deltaT = mCurrentMove.time;

   // Do nothing if we're under 0.1 vel
   if(hasExploded || mMoveState[ActualState].vel.len() < 0.1)
      return;

/*  Provisionally delete this...
   mSparkElapsed += deltaT;

   if(mSparkElapsed <= 32)  // What is the purpose of this?  To prevent sparks for the first 32ms of ship's life?!?
      return;
*/
   bool boostActive = isModuleActive(ModuleBoost);
   bool cloakActive = isModuleActive(ModuleCloak);

   Point corners[3];
   Point shipDirs[3];

   corners[0].set(-20, -15);
   corners[1].set(  0,  25);
   corners[2].set( 20, -15);

   F32 th = FloatHalfPi - mMoveState[RenderState].angle;

   F32 sinTh = sin(th);
   F32 cosTh = cos(th);
   F32 warpInScale = (WarpFadeInTime - mWarpInTimer.getCurrent()) / F32(WarpFadeInTime);

   for(S32 i=0; i<3; i++)
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

   if(isModuleActive(ModuleCloak))
      return;

   // Finally, do some particles
   Point velDir(mCurrentMove.right - mCurrentMove.left, mCurrentMove.down - mCurrentMove.up);
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
                Color dim(1, 0, 0);
                Color light(1, 1, boostActive ? 1.f : 0.f);
                Color thrust;

                F32 t = TNL::Random::readF();
                thrust.interp(t, dim, light);

                FXManager::emitSpark(mMoveState[RenderState].pos - shipDirs[i] * 13,
                     -shipDirs[i] * 100 + chaos, thrust, 1.5 * TNL::Random::readF());
             }
          }
      }
   }
}


extern bool gShowAimVector;
extern IniSettings gIniSettings;

void Ship::render(S32 layerIndex)
{
   if(layerIndex == 0) return;   // Only render on layers -1 and 1
   if(hasExploded) return;       // Don't render an exploded ship!

   // An angle of 0 means the ship is heading down the +X axis
   // since we draw the ship pointing up the Y axis, we should rotate
   // by the ship's angle, - 90 degrees
   glPushMatrix();
   glTranslatef(mMoveState[RenderState].pos.x, mMoveState[RenderState].pos.y, 0);

   F32 warpInScale = (WarpFadeInTime - mWarpInTimer.getCurrent()) / F32(WarpFadeInTime);

   // Render base ship
   F32 rotAmount = 0;      // We use rotAmount to add the spinny effect you see when a ship spawns or comes through a teleport
   if(warpInScale < 0.8)
      rotAmount = (0.8 - warpInScale) * 540;

   GameConnection *conn = gClientGame->getConnectionToServer();
   bool localShip = ! (conn && conn->getControlObject() != this);    // i.e. a ship belonging to a remote player
   S32 localPlayerTeam = (conn && conn->getControlObject()) ? conn->getControlObject()->getTeam() : Item::NO_TEAM; // To show cloaked teammates

   F32 alpha = isModuleActive(ModuleCloak) ? mCloakTimer.getFraction() : 1 - mCloakTimer.getFraction();

   if(!localShip && layerIndex == 1)      // Need to draw this before the glRotatef below, but only on layer 1...
   {
      string str = mPlayerName.getString();

      // Modify name if owner is "busy"
      if(isBusy)
         str = "<<" + str + ">>";

      glEnableBlend;
      F32 textAlpha = 0.5 * alpha;
      U32 textSize = 14;
#ifdef TNL_OS_XBOX
      textAlpha *= 1 - gClientGame->getCommanderZoomFraction();
      textSize = 23;
#else
      glLineWidth(gLineWidth1);
#endif
      glColor4f(1,1,1,textAlpha);
      UserInterface::drawStringc(0, 30, textSize, str.c_str());

      // Underline name if player is authenticated
      if(mIsAuthenticated)
      {
         S32 xoff = UserInterface::getStringWidth(textSize, str.c_str()) / 2;
         glBegin(GL_LINES);
            glVertex2f(-xoff, 33 + textSize);
            glVertex2f(xoff, 33 + textSize);
         glEnd();
      }

      glDisableBlend;
      glLineWidth(gDefaultLineWidth);
   }
   else
   {
      if(gGameUserInterface.mDebugShowShipCoords)
      {
         string str = string("@") + itos((S32) getActualPos().x) + "," + itos((S32) getActualPos().y);

         glEnableBlend;
            U32 textSize = 18;
            glLineWidth(gLineWidth1);
            glColor4f(1,1,1,0.5 * alpha);

            UserInterface::drawStringc(0, 30 + (localShip ? 0 : textSize + 3), textSize, str.c_str() );
         glDisableBlend;
         glLineWidth(gDefaultLineWidth);
      }
   }

   glRotatef(radiansToDegrees(mMoveState[RenderState].angle) - 90 + rotAmount, 0, 0, 1.0);
   glScalef(warpInScale, warpInScale, 1);

   if(layerIndex == -1)
   {
      // Draw the outline of the ship in solid black -- this will block out any stars and give
      // a tantalizing hint of motion when the ship is cloaked.  Could also try some sort of star-twinkling or
      // scrambling thing here as well...
      glColor3f(0,0,0);
      glDisableBlendfromLineSmooth;
      glBegin(GL_POLYGON);
         glVertex2f(-20, -15);
         glVertex2f(0, 25);
         glVertex2f(20, -15);
      glEnd();
      glEnableBlendfromLineSmooth;

      glPopMatrix();
      return;
   }

   // LayerIndex == 1

   GameType *g = gClientGame->getGameType();
   Color color;
   if(g)
      color = g->getShipColor(this);

   Point velDir(mCurrentMove.right - mCurrentMove.left, mCurrentMove.down - mCurrentMove.up);
   F32 len = velDir.len();
   F32 thrusts[4];
   for(U32 i = 0; i < 4; i++)
      thrusts[i] = 0;            // Reset thrusts

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
         thrusts[i] = shipDirs[i].dot(velDir);
   }

   // Tweak side thrusters to show rotational force
   F32 rotVel = getAngleDiff(mMoveState[LastProcessState].angle, mMoveState[RenderState].angle);

   if(rotVel > 0.001)
      thrusts[3] += 0.25;
   else if(rotVel < -0.001)
      thrusts[2] += 0.25;

   if(isModuleActive(ModuleBoost))
      for(U32 i = 0; i < 4; i++)
         thrusts[i] *= 1.3;

   // Don't completely hide local player or ships on same team
   if(localShip || (showCloakedTeammates && getTeam() == localPlayerTeam && g->isTeamGame()))
   {
      if(alpha < 0.25)
         alpha = 0.25;
   }
   else
   {
      // If local ship has sensor, it can see cloaked non-local ships
      Ship *ship = dynamic_cast<Ship *>(conn->getControlObject());
      if(ship && ship->isModuleActive(ModuleSensor) && alpha < 0.5)
         alpha = 0.5;
   }

   renderShip(color, alpha, thrusts, mHealth, mRadius, isModuleActive(ModuleCloak), isModuleActive(ModuleShield));

   if(gShowAimVector && gIniSettings.enableExperimentalAimMode && localShip)     // Only show for local ship
      renderAimVector();

if(isRobot())
{
   Robot *robot = dynamic_cast<Robot *>(this);
   if(robot)
   {
      glColor3f(0,1,1);
      glBegin(GL_LINES);
      Point shipPos = getRenderPos();
      glVertex2f(robot->mTarget.x , robot->mTarget.y );
      glVertex2f(0 ,0 );
      glEnd();
   }
}

   // Now render some "addons"  --> should these be in renderShip?
   glColor3f(1,1,1);
   if(isModuleActive(ModuleSensor))
   {
      U32 delta = getGame()->getCurrentTime() - mSensorStartTime;
      F32 radius = (delta & 0x1FF) * 0.002;
      drawCircle(Point(), radius * Ship::CollisionRadius + 4);
   }
   glPopMatrix();

   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid())
         mMountedItems[i]->renderItem(mMoveState[RenderState].pos);

   if(hasModule(ModuleArmor))
   {
      glLineWidth(gLineWidth3);
      glColor3f(1,1,0);

      drawPolygon(mMoveState[RenderState].pos, 5, 30, getAimVector().ATAN2());

      glLineWidth(gDefaultLineWidth);
   }

   if(isModuleActive(ModuleRepair))
   {
      glLineWidth(gLineWidth3);
      glColor3f(1,0,0);
      // render repair rays to all the repairing objects
      Point pos = mMoveState[RenderState].pos;

      for(S32 i = 0; i < mRepairTargets.size(); i++)
      {
         if(mRepairTargets[i].getPointer() == this)
            drawCircle(pos, RepairDisplayRadius);
         else if(mRepairTargets[i])
         {
            glBegin(GL_LINES);
            glVertex2f(pos.x, pos.y);

            Point shipPos = mRepairTargets[i]->getRenderPos();
            glVertex2f(shipPos.x, shipPos.y);
            glEnd();
         }
      }
      glLineWidth(gDefaultLineWidth);
   }
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


S32 LuaShip::isAlive(lua_State *L) { return returnBool(L, thisShip.isValid()); }

// Note: All of these methods will return nil if the ship in question has been deleted.
S32 LuaShip::getRad(lua_State *L) { return thisShip ? returnFloat(L, thisShip->getRadius()) : returnNil(L); }
S32 LuaShip::getLoc(lua_State *L) { return thisShip ? returnPoint(L, thisShip->getActualPos()) : returnNil(L); }
S32 LuaShip::getVel(lua_State *L) { return thisShip ? returnPoint(L, thisShip->getActualVel()) : returnNil(L); }
S32 LuaShip::hasFlag(lua_State *L) { return thisShip ? returnBool(L, thisShip->getFlagCount()) : returnNil(L); }

// Returns number of flags ship is carrying (most games will always be 0 or 1)
S32 LuaShip::getFlagCount(lua_State *L) { return thisShip ? returnInt(L, thisShip->getFlagCount()) : returnNil(L); }


S32 LuaShip::getTeamIndx(lua_State *L) { return returnInt(L, thisShip->getTeam() + 1); }

S32 LuaShip::getPlayerInfo(lua_State *L) { return thisShip ? returnPlayerInfo(L, thisShip) : returnNil(L); }


S32 LuaShip::isModActive(lua_State *L) {
   static const char *methodName = "Ship:isModActive()";
   checkArgCount(L, 1, methodName);
   ShipModule module = (ShipModule) getInt(L, 1, methodName, 0, ModuleCount - 1);
   return thisShip ? returnBool(L, getObj()->isModuleActive(module)) : returnNil(L);
}

S32 LuaShip::getAngle(lua_State *L) { return thisShip ? returnFloat(L, getObj()->getCurrentMove().angle) : returnNil(L); }      // Get angle ship is pointing at
S32 LuaShip::getActiveWeapon(lua_State *L) { return thisShip ?  returnInt(L, getObj()->getSelectedWeapon()) : returnNil(L); }    // Get WeaponIndex for current weapon

S32 LuaShip::getEnergy(lua_State *L) { return thisShip ? returnFloat(L, thisShip->getEnergyFraction()) : returnNil(L); }        // Return ship's energy as a fraction between 0 and 1
S32 LuaShip::getHealth(lua_State *L) { return thisShip ? returnFloat(L, thisShip->getHealth()) : returnNil(L); }                // Return ship's health as a fraction between 0 and 1


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

};

