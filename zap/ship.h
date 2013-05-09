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

#include "moveObject.h"
#include "LoadoutTracker.h"

#include "Timer.h"

#ifndef ZAP_DEDICATED
#  include "sparkManager.h"
#  include "ShipShape.h"
#endif

#include "tnlVector.h"


namespace Zap
{

struct DamageInfo;
class ClientInfo;
class MountableItem;
class SpeedZone;
class Statistics;
class Teleporter;

// class derived_class_name: public base_class_name
class Ship : public MoveObject
{
   typedef MoveObject Parent;

private:
   bool mIsRobot;

   Timer mSendSpawnEffectTimer;           // Only meaningful on the server
   Vector<DatabaseObject *> mZones1;      // A list of zones the ship is currently in
   Vector<DatabaseObject *> mZones2;
   bool mZones1IsCurrent;
   bool mFastRecharging;

   F32 mLastProcessStateAngle;

   void setActiveWeapon(U32 weaponIndex); // Setter for mActiveWeaponIndx

   Teleporter *mEngineeredTeleporter;

   // Find objects of specified type that may be under the ship, and put them in fillVector.  This is a private helper
   // for isInZone() and isInAnyZone().
   template <typename T>
   void findObjectsUnderShip(T typeNumberOrFunction) const
   {
      Rect rect(getActualPos(), getActualPos());
      rect.expand(Point(CollisionRadius, CollisionRadius));

      fillVector.clear();           // This vector will hold any matching zones
      findObjects(typeNumberOrFunction, fillVector, rect);
   }


   BfObject *doIsInZone(const Vector<DatabaseObject *> &objects) const; // Private helper for isInZone() and isInAnyZone()

   // Idle helpers
   void checkForSpeedzones();                      // Check to see if we collided with a GoFast
   void checkForZones();                           // See if ship entered or left any zones
   void getZonesShipIsIn(Vector<DatabaseObject *> *zoneList);     // Fill zoneList with a list of all zones that the ship is currently in
   bool isLocalPlayerShip(Game *game) const;       // Returns true if ship represents local player
  
   Vector<DatabaseObject *> *getCurrZoneList();    // Get list of zones ship is currently in
   Vector<DatabaseObject *> *getPrevZoneList();    // Get list of zones ship was in last tick

   bool doesShipActivateSensor(const Ship *ship);
   F32 getShipVisibility(const Ship *localShip);


protected:
   SafePtr <ClientInfo> mClientInfo;

   Vector<SafePtr<MountableItem> > mMountedItems;   

   LoadoutTracker mLoadout;

   Point mSpawnPoint;                        // Where ship or robot spawned.  Will only be valid on server, client doesn't currently get this.

   void initialize(const Point &pos);        // Some initialization code needed by both bots and ships
   void initialize(ClientInfo *clientInfo, S32 team, const Point &pos, bool isRobot);

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toLevelCode(F32 gridSize) const;

public:
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


   static const S32 CollisionRadius = 24;          // This is the ship's radius
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
      BoostAccelFact = 2,       // Will modify Acceleration

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
   const char *getOnScreenName();

   void selectNextWeapon();                   
   void selectPrevWeapon();
   void selectWeapon(S32 weaponIndex);    // Select weapon by index

   Timer mSensorEquipZoomTimer;
   Timer mWeaponFireDecloakTimer;
   Timer mCloakTimer;
   Timer mSpawnShield;
   Timer mModuleSecondaryTimer[ModuleCount];  // Timer to prevent accidentally firing in quick succession
   Timer mSpyBugPlacementTimer;
   Timer mFastRechargeTimer;

   void setChangeTeamMask();

#ifndef ZAP_DEDICATED
   U32 mSparkElapsed;
   S32 mLastTrailPoint[TrailCount];  // TrailCount = 2
   UI::FxTrail mTrail[TrailCount];
   ShipShape::ShipShapeType mShapeType;
#endif

   F32 mass;            // Mass of ship, not used
   bool hasExploded;

   Vector<SafePtr<BfObject> > mRepairTargets;            // TODO: Make this protected

   virtual void renderLayer(S32 layerIndex);

   // Constructor
   Ship(ClientInfo *clientInfo, S32 team, const Point &p, bool isRobot = false);   // Standard constructor   
   explicit Ship(lua_State *L = NULL);                                             // Combined Lua / C++ default constructor
   virtual ~Ship();                                                                // Destructor

   F32 getHealth() const;
   S32 getEnergy() const;

   // Related to mounting and carrying items
   S32 getMountedItemCount() const;

   bool isCarryingItem(U8 objectType) const;
   MountableItem *dismountFirst(U8 objectType);

   void dismountAll();                          // Dismount all objects of any type
   void dismountAll(U8 objectType);             // Dismount all objects of specified type

   MountableItem *getMountedItem(S32 index) const;
   void addMountedItem(MountableItem *item);
   void removeMountedItem(MountableItem *item);

   S32 getFlagIndex();     // Returns index of first flag, or NO_FLAG if ship has no flags
   S32 getFlagCount();     // Returns the number of flags ship is carrying

   void onGhostRemove();

   bool hasModule(ShipModule mod);

   bool isDestroyed();
   bool isVisible(bool viewerHasSensor);

   void creditEnergy(S32 deltaEnergy);

   void setEngineeredTeleporter(Teleporter *teleporter);
   Teleporter *getEngineeredTeleporter();
   F32 getSensorZoomFraction() const;
   Point getAimVector() const;

   void deploySpybug();

   bool setLoadout(const LoadoutTracker &loadout, bool silent = false);
   bool isLoadoutSameAsCurrent(const LoadoutTracker &loadout);

   ClientInfo *getClientInfo() const;

   virtual void idle(IdleCallPath path);

   void setMove(const Move &move);      // Used by tests only
   F32 processMove(U32 stateIndex);

   void processWeaponFire();
   void processModules();
   void rechargeEnergy();

   void updateModuleSounds();
   void emitMovementSparks();
   void updateTrails();
   void findRepairTargets();
   void repairTargets();

   void controlMoveReplayComplete();
   void onAddedToGame(Game *game);

   void emitExplosion();
   void setActualPos(const Point &p, bool warp);
   bool isModulePrimaryActive(ShipModule module);

   ShipModule getModule(U32 modIndex);

   virtual void killAndScore(DamageInfo *theInfo);
   virtual void kill();

   void destroyPartiallyDeployedTeleporter();

   virtual void damageObject(DamageInfo *theInfo);

   static void computeMaxFireDelay();

   void writeControlState(BitStream *stream);
   void readControlState(BitStream *stream);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void updateInterpolation();

   F32 getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips);

   bool isRobot();

   BfObject *isInZone(U8 zoneType) const; // Return whether the ship is currently in a zone of the specified type, and which one
   BfObject *isInAnyZone() const;         // Return whether the ship is currently in any zone, and which one

   DatabaseObject *isOnObject(U8 objectType); // Returns the object in question if this ship is on an object of type objectType

   bool isOnObject(BfObject *object);         // Return whether or not ship is sitting on a particular object

   virtual Ship *clone() const;

   TNL_DECLARE_CLASS(Ship);

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(Ship);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_isAlive(lua_State *L);
   S32 lua_getAngle(lua_State *L);
   S32 lua_hasFlag(lua_State *L);

   S32 lua_getEnergy(lua_State *L);  // Return ship's energy as a fraction between 0 and 1
   S32 lua_getHealth(lua_State *L);  // Return ship's health as a fraction between 0 and 1

   S32 lua_getFlagCount(lua_State *L);

   S32 lua_getPlayerInfo(lua_State *L);
   S32 lua_isModActive(lua_State *L);
   S32 lua_getMountedItems(lua_State *L);
   S32 lua_getCurrLoadout(lua_State *L);
   S32 lua_getReqLoadout(lua_State *L);

   S32 lua_getActiveWeapon(lua_State *L);                // Get WeaponIndex for current weapon

   S32 lua_setReqLoadout(lua_State *L);        // Sets requested loadout to specified --> takes Loadout object
   S32 lua_setCurrLoadout(lua_State *L);       // Sets current loadout to specified --> takes Loadout object
};


};

#endif

