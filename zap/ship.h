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
#include "BfObject.h"
#include "moveObject.h"
#include "LuaScriptRunner.h"
#include "SoundEffect.h"
#include "Timer.h"
#include "shipItems.h"
#include "gameWeapons.h"

#ifndef ZAP_DEDICATED
#  include "sparkManager.h"
#  include "ShipShape.h"
#endif

namespace Zap
{

class SpeedZone;
class Statistics;
class ClientInfo;
class Teleporter;

// class derived_class_name: public base_class_name
class Ship : public MoveObject
{
   typedef MoveObject Parent;

private:
   bool mIsRobot;

   U32 mRespawnTime;
   Vector<DatabaseObject *> mZones1;    // A list of zones the ship is currently in
   Vector<DatabaseObject *> mZones2;
   bool mZones1IsCurrent;
   bool mFastRecharge;

   SafePtr<Teleporter> mEngineeredTeleporter;

   // Find objects of specified type that may be under the ship, and put them in fillVector
   void findObjectsUnderShip(U8 typeNumber);

   // Idle helpers
   void checkForSpeedzones();    
   void checkForZones();

   Vector<DatabaseObject *> *getCurrZoneList();    // Get list of zones ship is currently in
   Vector<DatabaseObject *> *getPrevZoneList();    // Get list of zones ship was in last tick

protected:
   enum MaskBits {
      MoveMask            = Parent::FirstFreeMask << 0, // New user input
      HealthMask          = Parent::FirstFreeMask << 1,
      ModulePrimaryMask   = Parent::FirstFreeMask << 2, // Is module primary component active
      ModuleSecondaryMask = Parent::FirstFreeMask << 3, // Is module secondary component active
      LoadoutMask         = Parent::FirstFreeMask << 4,
      RespawnMask         = Parent::FirstFreeMask << 5, // For when robots respawn
      TeleportMask        = Parent::FirstFreeMask << 6, // Ship has just teleported
      ChangeTeamMask      = Parent::FirstFreeMask << 7, // Used for when robots change teams
      SpawnShieldMask     = Parent::FirstFreeMask << 8, // Used for the spawn shield
      FirstFreeMask       = Parent::FirstFreeMask << 9
   };

   SafePtr <ClientInfo> mClientInfo;

   bool mModulePrimaryActive[ModuleCount];       // Is the primary component of the module active at this moment?
   bool mModuleSecondaryActive[ModuleCount];     // Is the secondary component of the module active?

   ShipModule mModule[ShipModuleCount];   // Modules ship is carrying
   WeaponType mWeapon[ShipWeaponCount];
   Point mSpawnPoint;                     // Where ship or robot spawned.  Will only be valid on server, client doesn't currently get this.

   void initialize(const Point &pos);     // Some initialization code needed by both bots and ships
   void initialize(ClientInfo *clientInfo, S32 team, const Point &pos, bool isRobot);

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString(F32 gridSize) const;

public:
   static const S32 CollisionRadius = 24;
   static const S32 RepairRadius = 65;
   static const U32 SpawnShieldTime = 5000;        // Time spawn shields are active
   static const U32 SpawnShieldFlashTime = 1500;   // Time at which shields start to flash
   static const S32 PulseMaxVelocity = 2500;       // Maximum speed of Pulse
   static const S32 PulseMinVelocity = 1000;       // Minimum speed of Pulse
   static const S32 EnergyMax = 100000;            // Energy when fully charged   TODO: Make this lower resolution

   enum {
      MaxVelocity = 450,        // points per second
      Acceleration = 2500,      // points per second per second
      BoostMaxVelocity = 700,   // points per second
      BoostAcceleration = 5000, // points per second per second

      VisibilityRadius = 30,
      KillDeleteDelay = 1500,
      ExplosionFadeTime = 300,
      MaxControlObjectInterpDistance = 200,
      TrailCount = 2,

      // The following are all measured in units of energy/millisecond
      EnergyRechargeRate = 8,
      EnergyRechargeRateMovementModifier = -2,
      EnergyRechargeRateInHostileLoadoutZoneModifier = -20,
      EnergyRechargeRateInNeutralLoadoutZoneModifier = 4,
      EnergyRechargeRateInFriendlyLoadoutZoneModifier = 4,
      EnergyRechargeRateInEnemyLoadoutZoneModifier = -2,
      EnergyRechargeRateIdleRechargeCycle = 40,

      EnergyShieldHitDrain = 20000,       // Energy loss when shields stop a projectile (currently disabled)
      EnergyCooldownThreshold = 15000,
      WeaponFireDecloakTime = 350,
      SensorZoomTime = 300,
      SensorCloakInnerDetectionDistance = 300,
      SensorCloakOuterDetectionDistance = 500,
      CloakFadeTime = 300,
      CloakCheckRadius = 200,
      RepairHundredthsPerSecond = 16,
      MaxEngineerDistance = 100,
      WarpFadeInTime = 500,
   };

   S32 mFireTimer;
   Timer mWarpInTimer;
   F32 mHealth;
   S32 mEnergy;

   bool mCooldownNeeded;
   Point mImpulseVector;

   virtual bool canAddToEditor();

   F32 getSlipzoneSpeedMoficationFactor();

   SFXHandle mModuleSound[ModuleCount];

   U32 mActiveWeaponIndx;                 // Index of selected weapon on ship

   void selectNextWeapon();                   
   void selectPrevWeapon();
   void selectWeapon(S32 weaponIndex);    // Select weapon by index
   WeaponType getWeapon(U32 indx);        // Returns weapon in slot indx
   ShipModule getModule(U32 indx);        // Returns module in slot indx


   Timer mSensorEquipZoomTimer;
   Timer mWeaponFireDecloakTimer;
   Timer mCloakTimer;
   Timer mSpawnShield;
   Timer mModuleSecondaryTimer[ModuleCount];  // Timer to prevent accidentally firing in quick succession
   Timer mSpyBugPlacementTimer;
   Timer mIdleRechargeCycleTimer;
   static const U32 ModuleSecondaryTimerDelay = 500;
   static const U32 SpyBugPlacementTimerDelay = 800;
   static const U32 IdleRechargeCycleTimerDelay = 2000;

   void setChangeTeamMask();

#ifndef ZAP_DEDICATED
   U32 mSparkElapsed;
   S32 mLastTrailPoint[TrailCount];  // TrailCount = 2
   FXTrail mTrail[TrailCount];
   ShipShape::ShipShapeType mShapeType;
#endif

   F32 mass;            // Mass of ship, not used
   bool hasExploded;

   Vector<SafePtr<MountableItem> > mMountedItems;    // TODO: Make these protected
   Vector<SafePtr<BfObject> > mRepairTargets;

   virtual void render(S32 layerIndex);
   void calcThrustComponents(F32 *thrust);

   // Constructor
   Ship(ClientInfo *clientInfo, S32 team, const Point &p, bool isRobot = false);   // Standard constructor   
   Ship(lua_State *L = NULL);                                                      // Combined Lua / C++ default constructor
   ~Ship();                                                                        // Destructor

   F32 getHealth();
   S32 getEnergy();
   F32 getEnergyFraction();     // Only used by bots
   S32 getMaxEnergy();
   void changeEnergy(S32 deltaEnergy);

   void onGhostRemove();

   bool isModulePrimaryActive(ShipModule mod);
   bool isModuleSecondaryActive(ShipModule mod);

   void engineerBuildObject();

   bool hasModule(ShipModule mod);

   bool isDestroyed();
   bool isItemMounted();    // <== unused
   bool isVisible(bool viewerHasSensor);

   S32 carryingFlag();     // Returns index of first flag, or NO_FLAG if ship has no flags
   S32 getFlagCount();     // Returns the number of flags ship is carrying

   bool isCarryingItem(U8 objectType);
   MountableItem *unmountItem(U8 objectType);

   void setEngineeredTeleporter(Teleporter *teleporter);
   Teleporter *getEngineeredTeleporter();
   F32 getSensorZoomFraction();
   Point getAimVector();

   void deploySpybug();

   void getLoadout(Vector<U8> &loadout);    // Fills loadout
   bool setLoadout(const Vector<U8> &loadout, bool silent = false);
   bool isLoadoutSameAsCurrent(const Vector<U8> &loadout);
   void setDefaultLoadout();                 // Set the ship's loadout to the default values

   ClientInfo *getClientInfo();
   static string loadoutToString(const Vector<U8> &loadout);
   static bool stringToLoadout(string loadoutStr, Vector<U8> &loadout);


   virtual void idle(IdleCallPath path);

   virtual void processMove(U32 stateIndex);

   WeaponType getSelectedWeapon();   // Return currently selected weapon
   U32 getSelectedWeaponIndex();     // Return index of currently selected weapon (0, 1, 2)

   void processWeaponFire();
   void processModules();
   void rechargeEnergy();

   void updateModuleSounds();
   void emitMovementSparks();
   void findRepairTargets();
   void repairTargets();

   void controlMoveReplayComplete();
   void onAddedToGame(Game *game);

   void emitShipExplosion(Point pos);
   //void setActualPos(Point p);
   void setActualPos(Point p, bool warp);
   void activateModulePrimary(U32 indx);    // Activate the specified module primary component for the current move
   void activateModuleSecondary(U32 indx);  // Activate the specified module secondary component for the current move

   virtual void kill(DamageInfo *theInfo);
   virtual void kill();

   void destroyTeleporter();

   virtual void damageObject(DamageInfo *theInfo);

   static void computeMaxFireDelay();

   void writeControlState(BitStream *stream);
   void readControlState(BitStream *stream);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);
   F32 getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips);

   bool isRobot();

   BfObject *isInZone(U8 zoneType);           // Return whether the ship is currently in a zone of the specified type, and which one
   //BfObject *isInZone(BfObject *zone);
   DatabaseObject *isOnObject(U8 objectType); // Returns the object in question if this ship is on an object of type objectType

   bool isOnObject(BfObject *object);         // Return whether or not ship is sitting on a particular object

   virtual Ship *clone() const;

   TNL_DECLARE_CLASS(Ship);

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(Ship);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 isAlive(lua_State *L);
   S32 getAngle(lua_State *L);
   S32 hasFlag(lua_State *L);

   S32 getEnergy(lua_State *L);  // Return ship's energy as a fraction between 0 and 1
   S32 getHealth(lua_State *L);  // Return ship's health as a fraction between 0 and 1

   S32 getFlagCount(lua_State *L);

   S32 getPlayerInfo(lua_State *L);
   S32 isModActive(lua_State *L);
   S32 getMountedItems(lua_State *L);
   S32 getCurrLoadout(lua_State *L);
   S32 getReqLoadout(lua_State *L);

   S32 getActiveWeapon(lua_State *L);                // Get WeaponIndex for current weapon
};


};

#endif

