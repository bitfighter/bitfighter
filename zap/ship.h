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

#ifndef _SHIP_H_
#define _SHIP_H_

#include "gameObject.h"     
#include "moveObject.h"
#include "sparkManager.h"
#include "sfx.h"
#include "timer.h"
#include "shipItems.h"
#include "gameWeapons.h"

namespace Zap
{

class Item;

static const S32 ShipModuleCount = 2;                // Modules a ship can carry
static const S32 ShipWeaponCount = 3;                // Weapons a ship can carry


// class derived_class_name: public base_class_name
class Ship : public MoveObject
{
private:
   typedef MoveObject Parent;
   bool isBusy;

protected:
   StringTableEntry mPlayerName;

public:
   enum {
      MaxVelocity = 450,        // points per second
      Acceleration = 2500,      // points per second per second
      BoostMaxVelocity = 700,   // points per second
      BoostAcceleration = 5000, // points per second per second

      RepairRadius = 65,
      RepairDisplayRadius = 18,
      CollisionRadius = 24,
      VisibilityRadius = 30,
      KillDeleteDelay = 1500,
      ExplosionFadeTime = 300,
      MaxControlObjectInterpDistance = 200,
      TrailCount = 2,
      EnergyMax = 100000,
      EnergyRechargeRate = 6000,          // How many percent/second
      EnergyBoostDrain = 15000,
      EnergyShieldDrain = 27000,
      EnergyRepairDrain = 15000,
      EnergySensorDrain = 8000,
      EnergyCloakDrain = 8000,
      EnergyEngineerCost = 75000,
      EnergyShieldHitDrain = 20000,       // Energy loss when shields stop a projectile (currently disabled)
      EnergyCooldownThreshold = 15000,
      WeaponFireDecloakTime = 350,
      SensorZoomTime = 300,
      CloakFadeTime = 300,
      CloakCheckRadius = 200,
      RepairHundredthsPerSecond = 16,
      MaxEngineerDistance = 100,
      WarpFadeInTime = 500,
   };

   enum MaskBits {
      InitialMask = BIT(0),      // Initial ship position
      PositionMask = BIT(1),     // Ship position to be sent
      MoveMask = BIT(2),         // New user input
      WarpPositionMask = BIT(3),
      ExplosionMask = BIT(4),
      HealthMask = BIT(5),
      PowersMask = BIT(6),       // Which modules are active
      LoadoutMask = BIT(7),
      RespawnMask = BIT(8),      // For when robots respawn
   };

   Timer mFireTimer;
   Timer mWarpInTimer;
   F32 mHealth;
   S32 mEnergy;
   bool mCooldown;
   U32 mSensorStartTime;
   Point mImpulseVector;

   StringTableEntry getName() { return mPlayerName; }

   U32 mModule[ShipModuleCount];          // Modules ship is carrying
   bool mModuleActive[ModuleCount];       // Is that module active at this moment?
   SFXHandle mModuleSound[ModuleCount];

   WeaponType mWeapon[ShipWeaponCount];
   U32 mActiveWeaponIndx;                 // Index of selected weapon on ship

   void selectWeapon();
   void selectWeapon(U32 weaponIndex);

   Timer mSensorZoomTimer;
   Timer mWeaponFireDecloakTimer;
   Timer mCloakTimer;

   U32 mSparkElapsed;
   S32 mLastTrailPoint[TrailCount];
   FXTrail mTrail[TrailCount];

   F32 mass;            // Mass of ship
   bool hasExploded;

   Vector<SafePtr<Item>> mMountedItems;
   Vector<SafePtr<GameObject>> mRepairTargets;

   virtual void render(S32 layerIndex);
   
   Ship(StringTableEntry playerName="", S32 team = -1, Point p = Point(0,0), F32 m = 1.0);      // Constructor

   F32 getHealth() { return mHealth; }

   void onGhostRemove();

   bool isModuleActive(ShipModule mod) { return mModuleActive[mod]; }

   bool engineerBuildObject()
   {
      if(mEnergy < EnergyEngineerCost)
         return false;
      mEnergy -= EnergyEngineerCost;
      return true;
   }

   bool hasModule(ShipModule mod)
   {
      bool hasmod = false;
      for(S32 i = 0; i < ShipModuleCount; i++)
            hasmod = hasmod || (mModule[i] == mod);
       return hasmod;
    }

   bool isDestroyed() { return hasExploded; }
   bool areItemsMounted() { return mMountedItems.size() != 0; }
   S32 carryingFlag();
   bool carryingResource();
   Item *unmountResource();

   F32 getSensorZoomFraction() { return 1 - mSensorZoomTimer.getFraction(); }
   Point getAimVector();

   void setLoadout(const Vector<U32> &loadout);

   virtual void idle(IdleCallPath path);

   virtual void processMove(U32 stateIndex);
   F32 getMaxVelocity();

   void processWeaponFire();
   void processEnergy();
   void updateModuleSounds();
   void emitMovementSparks();
   bool findRepairTargets();
   void repairTargets();

   void controlMoveReplayComplete();

   void emitShipExplosion(Point pos);
   void setActualPos(Point p);

   virtual void kill(DamageInfo *theInfo);
   virtual void kill();

   virtual void damageObject(DamageInfo *theInfo);

   void writeControlState(BitStream *stream);
   void readControlState(BitStream *stream);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   virtual bool processArguments(S32 argc, const char **argv);

   virtual bool isRobot() { return false; }

   GameObject *isInZone(GameObjectType zoneType);     // Return whether the ship is currently in a zone of the specified type, and which one
   bool isOnObject(GameObject *object);               // Return whether or not ship is sitting on an item

   TNL_DECLARE_CLASS(Ship);
};

};

#endif
