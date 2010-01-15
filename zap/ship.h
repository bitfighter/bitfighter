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

#ifndef _SHIP_H_
#define _SHIP_H_

#include "item.h"    // for Item and LuaItem
#include "gameObject.h"
#include "moveObject.h"
#include "luaObject.h"
#include "sparkManager.h"
#include "sfx.h"
#include "timer.h"
#include "shipItems.h"
#include "gameWeapons.h"

namespace Zap
{

static const S32 ShipModuleCount = 2;                // Modules a ship can carry
static const S32 ShipWeaponCount = 3;                // Weapons a ship can carry
static const U32 DefaultLoadout[] = { ModuleBoost, ModuleShield, WeaponPhaser, WeaponMine, WeaponBurst };

//////////////////////////////////////////////

class LuaShip : public LuaItem
{

private:
   SafePtr<Ship> thisShip;    // Reference to actual C++ ship object

public:
   
   LuaShip(Ship *ship);                        // C++ constructor
   LuaShip() { /* do nothing */ };             // C++ default constructor ==> not used.  Constructor with Ship (above) used instead
   LuaShip(lua_State *L) { /* do nothing */ }; // Lua constructor ==> not used.  Class only instantiated from C++.

   ~LuaShip(){
       logprintf("Killing luaShip %d", mId);  
   };                      // Destructor

   static S32 id;
   S32 mId;

   static const char className[];

   static Lunar<LuaShip>::RegType methods[];

   virtual S32 getClassID(lua_State *L) { return returnInt(L, ShipType); }    // Robot will override this
   
   S32 getAngle(lua_State *L);
   S32 getLoc(lua_State *L);
   S32 getRad(lua_State *L);
   S32 getVel(lua_State *L);

   S32 getTeamIndx(lua_State *L);
   S32 isModActive(lua_State *L);

   GameObject *getGameObject();
   static const char *getClassName() { return "LuaShip"; }

   void push(lua_State *L) {  Lunar<LuaShip>::push(L, this, true); }      // Push item onto stack

   S32 getActiveWeapon(lua_State *L);                // Get WeaponIndex for current weapon

   virtual Ship *getObj() { return thisShip; }       // Access to underlying object, robot will override
   S32 isValid(lua_State *L);                       // Returns whether or not ship is still alive
};

//////////////////////////////////////////////

// class derived_class_name: public base_class_name
class Ship : public MoveObject
{
private:
   typedef MoveObject Parent;
   bool isBusy;
   void push(lua_State *L);      // Push a LuaShip proxy object onto the stack
   bool mIsRobot;

   S32 mJustTeleported;

protected:
   StringTableEntry mPlayerName;
   bool mModuleActive[ModuleCount];       // Is that module active at this moment?

   ShipModule mModule[ShipModuleCount];   // Modules ship is carrying
   WeaponType mWeapon[ShipWeaponCount];

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
      WarpPositionMask = BIT(3), // When ship makes a big jump in position
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

   SFXHandle mModuleSound[ModuleCount];

   U32 mActiveWeaponIndx;                 // Index of selected weapon on ship

   void selectWeapon();                   // Select next weapon
   void selectWeapon(U32 weaponIndex);    // Select weapon by index
   WeaponType getWeapon(U32 indx) { return mWeapon[indx]; }    // Returns weapon in slot indx
   ShipModule getModule(U32 indx) { return mModule[indx]; }    // Returns module in slot indx


   Timer mSensorZoomTimer;
   Timer mWeaponFireDecloakTimer;
   Timer mCloakTimer;

   U32 mSparkElapsed;
   S32 mLastTrailPoint[TrailCount];  // TrailCount = 2
   FXTrail mTrail[TrailCount];

   F32 mass;            // Mass of ship
   bool hasExploded;

   Vector<SafePtr<Item> > mMountedItems;
   Vector<SafePtr<GameObject> > mRepairTargets;

   virtual void render(S32 layerIndex);

   Ship(StringTableEntry playerName="", S32 team = -1, Point p = Point(0,0), F32 m = 1.0, bool isRobot = false);      // Constructor
   ~Ship();           // Destructor

   F32 getHealth() { return mHealth; }
   S32 getEnergy() { return mEnergy; }
   S32 getMaxEnergy() { return EnergyMax; }
   void changeEnergy(S32 deltaEnergy) { mEnergy = max(0, min(EnergyMax, mEnergy + deltaEnergy)); }

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

   WeaponType getSelectedWeapon() { return mWeapon[mActiveWeaponIndx]; }   // Return currently selected weapon
   U32 getSelectedWeaponIndex() { return mActiveWeaponIndx; }              // Return index of currently selected weapon (0, 1, 2)

   void processWeaponFire();
   void processEnergy();
   void updateModuleSounds();
   void emitMovementSparks();
   bool findRepairTargets();
   void repairTargets();

   void controlMoveReplayComplete();

   void emitShipExplosion(Point pos);
   //void setActualPos(Point p);
   void setActualPos(Point p, bool warp);
   void activateModule(U32 indx) { mCurrentMove.module[indx] = true; }     // Activate the specified module for the current move


   virtual void kill(DamageInfo *theInfo);
   virtual void kill();

   virtual void damageObject(DamageInfo *theInfo);

   void writeControlState(BitStream *stream);
   void readControlState(BitStream *stream);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   virtual bool processArguments(S32 argc, const char **argv);

   bool isRobot() { return mIsRobot; }

   GameObject *isInZone(GameObjectType zoneType);     // Return whether the ship is currently in a zone of the specified type, and which one
   bool isOnObject(GameObject *object);               // Return whether or not ship is sitting on an item

   TNL_DECLARE_CLASS(Ship);
};


};

#endif

