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
#include "gameConnection.h"
#include "shipItems.h"
#include "gameWeapons.h"
#include "gameObjectRender.h"
#include "config.h"
#include "../glut/glutInclude.h"

#include <stdio.h>

#define hypot _hypot    // Kill some warnings

namespace Zap
{

static Vector<GameObject *> fillVector;

//------------------------------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(Ship);

// Constructor
Ship::Ship(StringTableEntry playerName, S32 team, Point p, F32 m) : MoveObject(p, CollisionRadius)
{
   mObjectTypeMask = ShipType | MoveableType | CommandMapVisType | TurretTargetType;

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

   mPlayerName = playerName;     // This will be unique across all clients, but client and server may disagree on this name if the server has modified it to make it unique

   for(S32 i=0; i<TrailCount; i++)        // Don't draw any vehicle trails
      mTrail[i].reset();

   mEnergy = (S32)(EnergyMax * .80f); // Start off with 80% energy
   for(S32 i = 0; i < ModuleCount; i++)    // All modules disabled
      mModuleActive[i] = false;


   // Set initial module and weapon selections
   mModule[0] = ModuleBoost;
   mModule[1] = ModuleShield;

   mWeapon[0] = WeaponPhaser;
   mWeapon[1] = WeaponMine;
   mWeapon[2] = WeaponBurst;


   mActiveWeaponIndx = 0;

   mCooldown = false;
}

void Ship::onGhostRemove()
{
   Parent::onGhostRemove();
   for(S32 i = 0; i < ModuleCount; i++)
      mModuleActive[i] = false;
   updateModuleSounds();
}

void Ship::processArguments(S32 argc, const char **argv)
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
}

void Ship::setActualPos(Point p)
{
   mMoveState[ActualState].pos = p;
   mMoveState[RenderState].pos = p;
   setMaskBits(PositionMask | WarpPositionMask);
}


// Process a move.  This will advance the position of the ship, as well as adjust its velocity and angle.
void Ship::processMove(U32 stateIndex)
{
   mMoveState[LastProcessState] = mMoveState[stateIndex];

   F32 maxVel = isModuleActive(ModuleBoost) ? BoostMaxVelocity : MaxVelocity;

   F32 time = mCurrentMove.time * 0.001;
   Point requestVel(mCurrentMove.right - mCurrentMove.left, mCurrentMove.down - mCurrentMove.up);

   requestVel *= maxVel;
   F32 len = requestVel.len();

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

// Returns the zone in question if this ship is in a zone of type zoneType
GameObject *Ship::isInZone(GameObjectType zoneType)
 {
   // Create a small rectagle centered on the ship that we can use for findObjects
   Rect shipRect(getActualPos(), getActualPos());
   shipRect.expand(Point(.1,.1));

   fillVector.clear();           // This vector will hold any matching zones
   findObjects(zoneType, fillVector, shipRect);

   if (fillVector.size() == 0)  // Ship isn't in extent of any zoneType zones, can bail here
      return NULL;

   // Extents overlap...  now check for actual overlap

   Vector<Point> polyPoints;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      GameObject *zone = fillVector[i];

      // Get points that define the zone boundaries
      polyPoints.clear();
      zone->getCollisionPoly(polyPoints);

      if ( PolygonContains2(polyPoints.address(), polyPoints.size(), getActualPos()) )
         return zone;
   }
   return NULL;
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

   if(mCurrentMove.fire && mFireTimer.getCurrent() == 0)
   {
      if(mEnergy >= gWeapons[curWeapon].minEnergy)
      {
         mEnergy -= gWeapons[curWeapon].drainEnergy;
         mFireTimer.reset(gWeapons[curWeapon].fireDelay);
         mWeaponFireDecloakTimer.reset(WeaponFireDecloakTime);

         if(!isGhost())
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
      // if it's a large delta, get rid of the movement trails.
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
   // don't process exploded ships
   if(hasExploded)
      return;

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
         mMoveState[RenderState] = mMoveState[ActualState];
         setMaskBits(PositionMask);
      }
      else if(path == GameObject::ClientIdleControlMain || path == GameObject::ClientIdleMainRemote)
      {
         // On the client, update the interpolation of this
         // object unless we are replaying control moves.
         mInterpolating = (getActualVel().lenSquared() < MoveObject::InterpMaxVelocity*MoveObject::InterpMaxVelocity);
         updateInterpolation();
      }
   }

   // update the object in the game's extents database.
   updateExtent();

   // if this is a move executing on the server and it's
   // different from the last move, then mark the move to
   // be updated to the ghosts.
   if(path == GameObject::ServerIdleControlFromClient &&
         !mCurrentMove.isEqualMove(&mLastMove))
      setMaskBits(MoveMask);

   mLastMove = mCurrentMove;
   mSensorZoomTimer.update(mCurrentMove.time);
   mCloakTimer.update(mCurrentMove.time);

   //bool engineerWasActive = isModuleActive(ModuleEngineer);

   if(path == GameObject::ServerIdleControlFromClient ||
      path == GameObject::ClientIdleControlMain ||
      path == GameObject::ClientIdleControlReplay)
   {
      // process weapons and energy on controlled object objects
      processWeaponFire();
      processEnergy();
   }

   if(path == GameObject::ClientIdleMainRemote)
   {
      // for ghosts, find some repair targets for rendering the
      // repair effect.
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

// Returns true if we found a suitable target
bool Ship::findRepairTargets()
{
   // We use the render position in findRepairTargets so that
   // ships that are moving can repair each other (server) and
   // so that ships don't render funny repair lines to interpolating
   // ships (client)

   Vector<GameObject *> hitObjects;
   Point pos = getRenderPos();
   Point extend(RepairRadius, RepairRadius);
   Rect r(pos - extend, pos + extend);
   findObjects(ShipType | EngineeredType, hitObjects, r);

   mRepairTargets.clear();
   for(S32 i = 0; i < hitObjects.size(); i++)
   {
      GameObject *s = hitObjects[i];
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
   static U32 gEnergyDrain[ModuleCount] =
   {
      Ship::EnergyShieldDrain,
      Ship::EnergyBoostDrain,
      Ship::EnergySensorDrain,
      Ship::EnergyRepairDrain,
      0,    // ModuleEngineer
      Ship::EnergyCloakDrain,
   };

   bool modActive[ModuleCount];
   for(S32 i = 0; i < ModuleCount; i++)
   {
      modActive[i] = mModuleActive[i];
      mModuleActive[i] = false;
   }

   if(mEnergy > EnergyCooldownThreshold)
      mCooldown = false;

   for(S32 i = 0; i < ShipModuleCount; i++)
      if(mCurrentMove.module[i] && !mCooldown)  // No modules if we're too hot
         mModuleActive[mModule[i]] = true;

   // No boost if we're not moving.
    if(mModuleActive[ModuleBoost] &&
       mCurrentMove.up == 0 &&
       mCurrentMove.down == 0 &&
       mCurrentMove.left == 0 &&
       mCurrentMove.right == 0)
   {
      mModuleActive[ModuleBoost] = false;
   }

   // No repair with no targets.
   if(mModuleActive[ModuleRepair] && !findRepairTargets())
      mModuleActive[ModuleRepair] = false;

   // No cloak with nearby sensored people.
   if(mModuleActive[ModuleCloak])
   {
      if(mWeaponFireDecloakTimer.getCurrent() != 0)
         mModuleActive[ModuleCloak] = false;
      //else
      //{
      //   Rect cloakCheck(getActualPos(), getActualPos());
      //   cloakCheck.expand(Point(CloakCheckRadius, CloakCheckRadius));

      //   fillVector.clear();
      //   findObjects(ShipType, fillVector, cloakCheck);

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
         mEnergy -= S32(gEnergyDrain[i] * scaleFactor);
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
         setMaskBits(PowersMask);
      }
   }
}

void Ship::damageObject(DamageInfo *theInfo)
{
   if(theInfo->damageAmount > 0)
   {
      if(!getGame()->getGameType()->objectCanDamageObject(theInfo->damagingObject, this))
         return;

      // Factor in shields
      if(isModuleActive(ModuleShield)) // && mEnergy >= EnergyShieldHitDrain)     // Commented code will cause
      {                                                                           // shields to drain when they
         //mEnergy -= EnergyShieldHitDrain;                                       // have been hit.

         // Deal with grenades                              // Should probably find a way not to
         if(theInfo->damageType == DamageTypeArea)          // duplicate this block with that
            mImpulseVector += theInfo->impulseVector;       // down a little lower...
         return;
      }
   }

   mHealth -= theInfo->damageAmount;
   setMaskBits(HealthMask);
   if(mHealth <= 0)
   {
      mHealth = 0;
      kill(theInfo);
   }
   else if(mHealth > 1)
      mHealth = 1;

   // Deal with grenades
   if(theInfo->damageType == DamageTypeArea)
      mImpulseVector += theInfo->impulseVector;
}


void Ship::updateModuleSounds()
{
   static S32 moduleSFXs[ModuleCount] =
   {
      SFXShieldActive,
      SFXShipBoost,
      SFXSensorActive,
      SFXRepairActive,
      -1, // No engineer pack, yo!
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
            mModuleSound[i] = 0;
         }
      }
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

// Writes the player's ghost update from the server to the client
// Any changes here need to be reflected in Ship::unpackUpdate
U32  Ship::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   GameConnection *gameConnection = (GameConnection *) connection;

   if(isInitialUpdate())      // This stuff gets sent only once
   {
      stream->writeFlag(getGame()->getCurrentTime() - getCreationTime() < 300);  // If true, ship will appear to spawn on client
      stream->writeStringTableEntry(mPlayerName);
      stream->write(mass);
      stream->write(mTeam);

      //stream->writeRangedU32(mTeam + 1, 0, getGame()->getTeamCount());

      // now write all the mounts:
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
   }  // end initial update

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

   bool shouldWritePosition = (updateMask & InitialMask) || gameConnection->getControlObject() != this;

   stream->writeFlag(updateMask & WarpPositionMask);
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
      {
         mCurrentMove.pack(stream, NULL, false);      // Send current move
      }
      if(stream->writeFlag(updateMask & PowersMask))
      {
         for(S32 i = 0; i < ModuleCount; i++)         // Send info about which modules are active
            stream->writeFlag(mModuleActive[i]);
      }
   }
   return 0;
}

// Any changes here need to be reflected in Ship::packUpdate
void Ship::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool positionChanged = false;
   bool wasInitialUpdate = false;
   bool playSpawnEffect = false;

   if(isInitialUpdate())
   {
      wasInitialUpdate = true;
      playSpawnEffect = stream->readFlag();

      stream->readStringTableEntry(&mPlayerName);
      stream->read(&mass);

      stream->read(&mTeam);
     // mTeam = stream->readRangedU32(0, gClientGame->getTeamCount()) - 1;

      // Read mounted items:
      while(stream->readFlag())
      {
         S32 index = stream->readInt(GhostConnection::GhostIdBitSize);
         Item *theItem = (Item *) connection->resolveGhost(index);
         theItem->mountToShip(this);
      }
   }  // initial update


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
   bool warp = stream->readFlag();
   if(warp)    // Ship just teleported/warped
   {
      mWarpInTimer.reset(WarpFadeInTime);

      // This is the controversial warping zoomy effect.  It has been disabled for now.  Uncomment the lines below to re-enable.
      // Now switch to commander's map and zoom in on new location.  Basically simulates leaving commander's map.
      // Only do this if the teleporting ship is the local ship.
      /*
      GameConnection *conn = gClientGame->getConnectionToServer();

      if(gClientGame && !gClientGame->getInCommanderMap() && conn && conn->getControlObject() == this)
         gClientGame->resetZoomDelta();
      */
   }

   if(stream->readFlag())
   {
      ((GameConnection *) connection)->readCompressedPoint(mMoveState[ActualState].pos, stream);
      readCompressedVelocity(mMoveState[ActualState].vel, BoostMaxVelocity + 1, stream);
      positionChanged = true;
   }
   if(stream->readFlag())
   {
      mCurrentMove = Move();  // A new, blank move
      mCurrentMove.unpack(stream, false);
   }
   if(stream->readFlag())
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

   if(positionChanged)
   {
      mCurrentMove.time = (U32) connection->getOneWayTime();
      processMove(ActualState);

      if(!warp)
      {
         mInterpolating = true;
         // If the actual velocity is in the direction of the actual position
         // then we'll set it into the render velocity
      }
      else           // Just warped
      {
         mInterpolating = false;
         mMoveState[RenderState] = mMoveState[ActualState];

         for(S32 i=0; i<TrailCount; i++)
            mTrail[i].reset();
      }
   } // if positionChanged

   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();

      if(!wasInitialUpdate)
         emitShipExplosion(mMoveState[ActualState].pos);    // Boom!
   }

   if(playSpawnEffect)     // Make ship all spinny-like
   {
      FXManager::emitTeleportInEffect(mMoveState[ActualState].pos, 1);
      SFXObject::play(SFXTeleportIn, mMoveState[ActualState].pos, Point());
   }

}  // unpackUpdate

F32 getAngleDiff(F32 a, F32 b)
{
   // Figure out the shortest path from a to b...
   // Restrict them to the range 0-360
   while(a<0)   a+=360;
   while(a>360) a-=360;

   while(b<0)   b+=360;
   while(b>360) b-=360;

   if(fabs(b-a) > 180)
   {
      // Go the other way
      return  360-(b-a);
   }
   else
   {
      return b-a;
   }
}

bool Ship::carryingResource()
{
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
      if(mMountedItems[i].isValid() && mMountedItems[i]->getObjectTypeMask() & ResourceItemType)
         return true;
   return false;
}

Item *Ship::unmountResource()
{
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
   {
      if(mMountedItems[i]->getObjectTypeMask() & ResourceItemType)
      {
         Item *ret = mMountedItems[i];
         ret->dismount();
         return ret;
      }
   }
   return NULL;
}


void Ship::setLoadout(const Vector<U32> &loadout)
{
   // Check to see if the new configuration is the same as the old.  If so, we have nothing to do.
   bool theSame = true;

   for(S32 i = 0; i < ShipModuleCount; i++)
      theSame = theSame && (loadout[i] == mModule[i]);

   for(S32 i = ShipModuleCount; i < ShipWeaponCount + ShipModuleCount; i++)
      theSame = theSame && (loadout[i] == mWeapon[i - ShipModuleCount]);

   if(theSame)      // Don't bother if ship config hasn't changed
      return;

   WeaponType currentWeapon = mWeapon[mActiveWeaponIndx];

   for(S32 i = 0; i < ShipModuleCount; i++)
      mModule[i] = loadout[i];

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
   if(isGhost())     // Servers only, please...
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
   U32 deltaT = mCurrentMove.time;

   // Do nothing if we're under 0.1 vel
   if(hasExploded || mMoveState[ActualState].vel.len() < 0.1)
      return;

   mSparkElapsed += deltaT;

   if(mSparkElapsed <= 32)
      return;

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

   F32 alpha = 1.0;

   if(isModuleActive(ModuleCloak))
      alpha = mCloakTimer.getFraction();
   else
      alpha = 1 - mCloakTimer.getFraction();


   if(!localShip && layerIndex == 1)      // Need to draw this before the glRotatef below, but only on layer 1...
   {
      const char *string = mPlayerName.getString();
      glEnable(GL_BLEND);
      F32 textAlpha = 0.5 * alpha;
      U32 textSize = 14;
#ifdef TNL_OS_XBOX
      textAlpha *= 1 - gClientGame->getCommanderZoomFraction();
      textSize = 23;
#else
      glLineWidth(1);
#endif
      glColor4f(1,1,1,textAlpha);
      UserInterface::drawString((S32)(UserInterface::getStringWidth(textSize, string) * -0.5), 30, textSize, string );
      glDisable(GL_BLEND);
      glLineWidth(gDefaultLineWidth);
   }


   glRotatef(radiansToDegrees(mMoveState[RenderState].angle) - 90 + rotAmount, 0, 0, 1.0);
   glScalef(warpInScale, warpInScale, 1);

   if(layerIndex == -1)
   {
      // Draw the outline of the ship in solid black -- this will block out any stars and give
      // a tantalizing hint of motion when the ship is cloaked.  Could also try some sort of star-twinkling or
      // scrambling thing here as well...
      glColor3f(0,0,0);
      glBegin(GL_POLYGON);
      glVertex2f(-20, -15);
      glVertex2f(0, 25);
      glVertex2f(20, -15);
      glEnd();

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


   if(!localShip)
   {
      // Sensor can see cloaked ships
      Ship *ship = dynamic_cast<Ship *>(conn->getControlObject());
      if(ship && ship->isModuleActive(ModuleSensor) && alpha < 0.5)
         alpha = 0.5;
   }
   else     // Ship belongs to a local player
   {
      if(alpha < 0.25)
         alpha = 0.25;
   }


   renderShip(color, alpha, thrusts, mHealth, mRadius, isModuleActive(ModuleCloak), isModuleActive(ModuleShield));

   if(gShowAimVector && gIniSettings.enableExperimentalAimMode && localShip)     // Only show for local ship
      renderAimVector();

   // Now render some "addons"
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

   if(isModuleActive(ModuleRepair))
   {
      glLineWidth(3);
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

};
