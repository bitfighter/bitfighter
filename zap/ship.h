//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SHIP_H_
#define _SHIP_H_

#include "moveObject.h"
#include "LoadoutTracker.h"

#include "Timer.h"

#ifndef ZAP_DEDICATED
#  include "sparkManager.h"
#  include "ShipShape.h"
#  include "SoundEffect.h"         // For SFXHandle def
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
struct ControlObjectData;

// class derived_class_name: public base_class_name
class Ship : public MoveObject
{
   typedef MoveObject Parent;

   bool mIsRobot;

protected:
   Timer mSendSpawnEffectTimer;           // Only meaningful on the server
private:
   Vector<SafePtr<Zone> > mZones1;      // A list of zones the ship is currently in
   Vector<SafePtr<Zone> > mZones2;
   bool mZones1IsCurrent;
   bool mFastRecharging;

   F32 mLastProcessStateAngle;

   void setActiveWeapon(U32 weaponIndex);

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
   bool checkForSpeedzones(U32 stateIndex = ActualState); // Check to see if we collided with a GoFast
   void checkForZones();                           // See if ship entered or left any zones
   void getZonesShipIsIn(Vector<SafePtr<Zone> > &zoneList);     // Fill zoneList with a list of all zones that the ship is currently in
   bool isLocalPlayerShip(Game *game) const;       // Returns true if ship represents local player
  
   Vector<SafePtr<Zone> > &getCurrZoneList();    // Get list of zones ship is currently in
   Vector<SafePtr<Zone> > &getPrevZoneList();    // Get list of zones ship was in last tick

   bool doesShipActivateSensor(const Ship *ship);
   F32 getShipVisibility(const Ship *localShip);

   LoadoutTracker checkAndBuildLoadout(lua_State *L, S32 profile);

protected:
   SafePtr <ClientInfo> mClientInfo;
   StringTableEntry mPlayerName;

   Vector<SafePtr<MountableItem> > mMountedItems;   

   LoadoutTracker mLoadout;

   Point mSpawnPoint;                        // Where ship or robot spawned.  Will only be valid on server, client doesn't currently get this.

   void initialize(const Point &pos);        // Some initialization code needed by both bots and ships
   void initialize(ClientInfo *clientInfo, S32 team, const Point &pos, bool isRobot);

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toLevelCode() const;

public:
   enum MaskBits {
      MoveMask            = Parent::FirstFreeMask << 0, // New user input
      HealthMask          = Parent::FirstFreeMask << 1,
      ModulePrimaryMask   = Parent::FirstFreeMask << 2, // Is module primary component active
      ModuleSecondaryMask = Parent::FirstFreeMask << 3, // Is module secondary component active
      LoadoutMask         = Parent::FirstFreeMask << 4,
      RespawnMask         = Parent::FirstFreeMask << 5, // For when robots respawn
      TeleportMask        = Parent::FirstFreeMask << 6, // Ship has just teleported
      SpawnShieldMask     = Parent::FirstFreeMask << 7, // Used for the spawn shield
      FirstFreeMask       = Parent::FirstFreeMask << 8
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

   // For testing purposes
   WeaponType getActiveWeapon() const;
   string getLoadoutString() const;


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
                                             
   bool isServerCopyOf(const Ship &r) const; // Kind of like an equality comparitor, but accounting for differences btwn client and server

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

   bool hasModule(ShipModule mod) const;

   bool isDestroyed();
   bool isVisible(bool viewerHasSensor);

   void creditEnergy(S32 deltaEnergy);

   void setEngineeredTeleporter(Teleporter *teleporter);
   Teleporter *getEngineeredTeleporter();
   F32 getSensorZoomFraction() const;
   Point getAimVector() const;

   void deploySpybug();

   const LoadoutTracker *getLoadout() const;
   bool setLoadout(const LoadoutTracker &loadout, bool silent = false);
   bool isLoadoutSameAsCurrent(const LoadoutTracker &loadout);

   ClientInfo *getClientInfo() const;

   virtual void idle(IdleCallPath path);

   void setMove(const Move &move);      // Used by tests only
   F32 processMove(U32 stateIndex);

   void processWeaponFire();
   void processModules();
   void rechargeEnergy();
   void resetFastRecharge();

#ifndef ZAP_DEDICATED
private:
   SFXHandle mModuleSound[ModuleCount];
#endif
public:

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

   void setState(ControlObjectData *state);
   void getState(ControlObjectData *state) const;

   void writeControlState(BitStream *stream);
   void readControlState(BitStream *stream);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void findClientInfoFromName();
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void updateInterpolation();

   F32 getUpdatePriority(GhostConnection *connection, U32 updateMask, S32 updateSkips);

   bool isRobot();

   BfObject *isInZone(U8 zoneType) const; // Return whether the ship is currently in a zone of the specified type, and which one
   BfObject *isInAnyZone() const;         // Return whether the ship is currently in any zone, and which one

   DatabaseObject *isOnObject(U8 objectType, U32 stateIndex = ActualState); // Returns the object in question if this ship is on an object of type objectType

   bool isOnObject(BfObject *object, U32 stateIndex = ActualState);         // Return whether or not ship is sitting on a particular object

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

   S32 lua_getEnergy(lua_State *L);
   S32 lua_setEnergy(lua_State *L);
   S32 lua_getHealth(lua_State *L);
   S32 lua_setHealth(lua_State *L);

   S32 lua_getFlagCount(lua_State *L);

   S32 lua_getPlayerInfo(lua_State *L);
   S32 lua_isModActive(lua_State *L);
   S32 lua_getMountedItems(lua_State *L);
   S32 lua_getLoadout(lua_State *L);

   S32 lua_getActiveWeapon(lua_State *L);                // Get WeaponIndex for current weapon

   S32 lua_setLoadout(lua_State *L);
   S32 lua_setLoadoutNow(lua_State *L);
};


};

#endif

