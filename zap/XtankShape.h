//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------
//
// XtankShape.h - Vehicle body shapes derived from the xtank open-source game
// (https://github.com/lidl/xtank).  These shapes are stored in the same
// ShipShapeInfo format as the native Bitfighter ship shapes so the existing
// renderer can draw them without modification.
//
// The polygon vertices were extracted from the xtank .obj files (view 0,
// facing east/+x) and rotated 90 degrees counter-clockwise so the nose of
// each vehicle points up (+y), matching the Bitfighter ship drawing
// convention.  Coordinates were then scaled to fit within the standard
// Ship::CollisionRadius of 24 units.
//
// Keep this file and its companion VehicleDesign.cpp cleanly separated from the
// rest of the Bitfighter codebase.  The only integration points are:
//   - ship.h / ship.cpp  (mXtankDesign.body/mXtankDesign fields, cycleXtankBody(), tank physics)
//   - UIGame.cpp         (Ctrl+Alt+Shift+X hotkey; BINDING_LOADOUT xtank design menu)
//   - move.h / move.cpp  (Move::bodyIndex, Move::weaponSlot[], Move::engineType,
//                         Move::treadType, Move::heatSinkCount, Move::specials fields)
//   - gameWeapons.h/cpp  (GameWeapon::createXtankProjectile)
//   - UIVehicleDesigner.h/cpp (vehicle design helper menu)
//   - LoadoutIndicator.h/cpp (HUD panel)
//------------------------------------------------------------------------------

#ifndef _XTANK_SHAPE_H_
#define _XTANK_SHAPE_H_

#include "ShipShape.h"     // for ShipShapeInfo
#include "WeaponInfo.h"    // for WeaponType (used in XtankWeaponInfo::bfWeapon) and ProjectileStyle
#include "LoadoutTracker.h"
#include "move.h"
#include "tnlTypes.h"
#include "tnlVector.h"
#include "Timer.h"

#include <array>

namespace Zap
{

//using std::array;

static const S32 WEAPON_SLOTS = 6;	   // Max number of weapons a vehicle can carry
static const S32 XTANK_FPS = 15;       //Xtank runs at 15 frames per second, so we use that to convert to time

// Starting money for xtank vehicles.  The xtank formula scales with the most-expensive vehicle
// in the match, but we use a constant here for now.  Revisit when per-match economy is added.
static const S32 STARTING_MONEY = 5000;


// Per-weapon runtime state for xtank vehicles.  One entry per WEAPON_SLOTS slot.
struct XtankWeaponState
{
   S32 ammo;           // Current ammo count (0 = empty)
   Timer reloadTimer;  // Time remaining until the weapon can fire again
   Timer refillTimer;  // Tracks refilling when inside a ReloadZone
   bool mIsActive;     // Weapon is on (or off)

   bool hasAmmo() const { return ammo > 0; }
   bool isActive() const { return mIsActive; }
   void setActive(bool isActive) { mIsActive = isActive; }
};


// These really don't belong here; they are part of the vehicle designer, which is in VehicleDesignerUserInterface at the moment.
enum class Phase
{
   BODY = 0,
   ENGINE,
   TREADS,
   ARMOR,
   ARMOR_SIDES,
   SUSPENSION,
   BUMPERS,
   SPECIALS,
   HEATSINK,
   WEAPONS,
   COUNT,
   NONE = -1,
};
constexpr S32 PhaseCount = (S32)Phase::COUNT;

#define MAX_ACCEL 2.5   // The amount a vehicle can accelerate under perfect conditions


   // Names of each xtank body, kept in the same order as the enum below.
   // Used for on-screen display when the player cycles bodies.
   extern const char *xtankBodyNames[];

   // Enum of all 14 xtank vehicle bodies (in the order they appear in xtank's
   // objects.c).  BITFIGHTER_SHIP represents "show the normal BF ship".
   enum class XtankBody
   {
      Lightcycle = 0,
      Trike,
      Hexo,
      Spider,
      Psycho,
      Tornado,
      Marauder,
      Tiger,
      Rhino,
      Medusa,
      Delta,
      Disk,
      Malice,
      Panzy,
      COUNT,
      NONE,
      BITFIGHTER_SHIP,
      DEFAULT = Lightcycle
   };
   constexpr S32 VehicleBodyCount = (S32)XtankBody::COUNT;


   struct XtankBodyInfo2
   {
      const char *name; // Display name
      S32 size;
      S32 weight;
      S32 weightLimit;
      S32 space;
      F32 drag;
      S32 handling;
      S32 turrets;
      S32 cost;
   };

   // Array of ShipShapeInfo descriptors for every xtank body.  Each entry is
   // compatible with the existing renderShip() rendering path.
   extern ShipShapeInfo xtankBodyInfos[VehicleBodyCount];

   // Pre-computed bounding-circle radius for each xtank body (maximum distance
   // from origin to any hull vertex).  Used for collision detection so the
   // hitbox reflects the actual hull size rather than the BF default.
   extern F32 xtankBodyCollisionRadius[VehicleBodyCount];

   // Y coordinate (in body space, +Y = forward/nose) of the foremost hull vertex.
   // Used by the vehicle overlay renderer to position the nose chevron and to
   // distribute heat-sink / engine indicators along the hull.
   extern F32 xtankBodyNoseY[VehicleBodyCount];

   // Y coordinate of the rearmost hull vertex (always negative or small positive).
   extern F32 xtankBodyRearY[VehicleBodyCount];

   // ---------------------------------------------------------------------------
   // Turret data for xtank vehicles.
   //
   // Each vehicle carries one or more turrets that are rendered separately from
   // the hull.  Mount positions are in body space: +Y points toward the nose,
   // using the same coordinate scale as the hull vertex data above.
   // ---------------------------------------------------------------------------

   // A single turret mount point in body space.
   struct XtankTurret
   {
      F32 x, y;
   };

   // All turret mount points for one vehicle body (up to 4).
   struct XtankBodyTurrets
   {
      S32 count;
      XtankTurret turrets[4];
   };

   // One entry per XtankBody value.
   extern XtankBodyTurrets xtankTurretInfos[VehicleBodyCount];

   // Original xtank body data (objects.c/body-defs.h style fields).
   // Movement is derived from this table plus selected engine/treads.
   extern XtankBodyInfo2 xTankBodyStats[];

   // ---------------------------------------------------------------------------
   // Xtank weapon catalogue.
   //
   // Each xtank weapon maps to an existing Bitfighter WeaponType for projectile
   // behavior and damage (so we reuse the well-tuned BF projectile system).
   // Fire delay and energy cost are xtank-specific and storedype here.
   // ---------------------------------------------------------------------------

   // Enum of all xtank weapons (native from xtank game).
   // XtankWeapon::NONE represents "this turret slot carries no weapon".
   enum class XtankWeapon
   {
      NONE = 0,            // Slot carries no weapon
      LIGHT_MACHINE_GUN,
      MACHINE_GUN,
      HEAVY_MACHINE_GUN,
      LIGHT_AUTOCANNON,
      AUTOCANNON,
      HEAVY_AUTOCANNON,
      LIGHT_RKT_LAUNCHER,
      RKT_LAUNCHER,
      HEAVY_RKT_LAUNCHER,
      ACID_SPRAYER,
      FLAME_THROWER,
      HEAT_SEEKER,
      POCKET_ROCKET,
      UNGUIDED_MISSLE,
      TELEGUIDED,
      TOW_MISSILE,
      LAND_TORPEDO,
      BLAST_CANNON,
      PULSE_LASER,
      MINE_LAYER,
      OIL_SLICK,
      HEAVY_MORTAR,
      TACTICAL_NUKE,
      ANTI_RADIATION,
      DISC_SHOOTER,
      COUNT,
   };
   constexpr S32 XtankWeaponCount = (S32)XtankWeapon::COUNT;

   enum class VehicleSides
   {
      SIDE_FRONT = 0,
      SIDE_BACK,
      SIDE_LEFT,
      SIDE_RIGHT,
      SIDE_TOP,
      SIDE_BOTTOM,
      COUNT  // 6
   };
   constexpr S32 VehicleSidesCount = (S32)VehicleSides::COUNT;
   static const char *sideNames[] = { "Front", "Back", "Left", "Right", "Top", "Bottom" };


   struct HeatSinkStat
   {
      S32 weight;
      S32 space;
      S32 cost;
   };

   struct SuspensionStat
   {
      const char *name;
      F32 friction;
      S32 cost;
   };

   struct BumperStat
   {
      const char *name;
      F32 elasticity; // (0 = no bounce, 1 = perfect bounce)
      S32 cost;
   };


   // Per-weapon parameters: native xtank stats + BF integration fields.
   class XtankWeaponInfo
   {
      public:
         const char *name;      // Display name
         S32 damage;            // Damage per hit (xtank native)
         S32 max_ammo;          // Maximum ammo capacity
         S32 reload_time;       // Reload time between shots (ms)
         S32 ammo_speed;        // Projectile speed (xtank units/frame)
         S32 weight;            // Weight of the weapon
         S32 space;             // Space required to mount
         S32 mount_space;       // Mounting space needed
         S32 frames;            // Projectile lifetime (xtank frames)
         S32 heat;              // Heat generated per shot
         S32 ammo_cost;         // Cost per shot
         S32 cost;              // Purchase cost
         S32 refill_time;       // Ammo refill time
         S32 safety;            // Safety distance
         S32 height;            // Projectile height
         S32 legalMounts;       // Where weapon can be mounted (bitflags)
         U64 other_flgs;        // Misc. flags
         U64 creat_flgs;        // Bullet creation flags
         U64 disp_flgs;         // Display flags
         U64 move_flgs;         // Movement flags
         U64 hit_flgs;          // Hit/damage flags
         // Bitfighter integration fields:
         WeaponType bfWeapon;   // Mapped BF weapon for projectile behavior
         ProjectileStyle style; // Rendering style

         S32 range() const
         {
            return frames * ammo_speed / XTANK_FPS;
         }
   };

   // One entry per XtankWeapon value.
   extern XtankWeaponInfo xtankWeaponInfos[];

   // Helper functions to compute BF-compatible values from native xtank fields.
   inline U32 xtankFireDelayMs(const XtankWeaponInfo &wi)
   {
      return (U32)(wi.reload_time * XTANK_FPS); // frames → milliseconds
   }

   inline U32 xtankProjVelocity(const XtankWeaponInfo &wi)
   {
      return (U32)(wi.ammo_speed * XTANK_FPS); // xtank units/frame → BF units/sec
   }

   inline S32 xtankProjLiveTime(const XtankWeaponInfo &wi)
   {
      return (S32)(wi.frames * XTANK_FPS); // frames → milliseconds
   }

   // ---------------------------------------------------------------------------
   // Engine types: player-selectable power plant affecting top speed and
   // acceleration.  A heavier engine gives more power but adds bulk.
   // ---------------------------------------------------------------------------

   enum class XtankEngine
   {
      Small_Electric,
      Medium_Electric,
      Large_Electric,
      Super_Electric,
      Small_Combustion,
      Medium_Combustion,
      Large_Combustion,
      Super_Combustion,
      Small_Turbine,
      Medium_Turbine,
      Large_Turbine,
      Turbojet_Turbine,
      Fuel_Cell,
      Fission,
      Breeder_Fission,
      Fusion,
      COUNT,
      DEFAULT = Super_Combustion
   };
   constexpr S32 XtankEngineCount = (S32)XtankEngine::COUNT;

   enum class XtankArmor
   {
      Steel,
      Kevlar,
      Hardened_Steel,
      Composite,
      Carapice,
      Porcelain,
      Compound_Steel,
      Titanium,
      Tungsten,
      COUNT,
      DEFAULT = Steel
   };
   constexpr S32 XtankArmorCount = (S32)XtankArmor::COUNT;

   // Per-engine gameplay multipliers applied on top of the body's base physics.
   struct XtankEngineInfo
   {
      const char *name;
      F32 speedMult; // Multiplier on maxSpeed and maxReverseSpeed
      F32 accelMult; // Multiplier on acceleration
      S32 power;
      S32 weight;
      S32 space;
      S32 fuel;
      S32 fcap;
      S32 cost;
   };

   struct XtankArmorInfo
   {
      const char *name;
      S32 defense;
      S32 weight;
      S32 space;
      S32 cost;
   };

   // One entry per XtankEngine value.
   extern XtankEngineInfo xtankEngineInfos[XtankEngineCount];

   // ---------------------------------------------------------------------------
   // Tread types: player-selectable track system affecting maneuverability.
   // Rubber treads give nimble steering; heavy treads grip harder but turn slower.
   // ---------------------------------------------------------------------------

   enum class XtankTread
   {
      SMOOTH = 0,
      NORMAL,
      CHAINED,
      SPIKED,
      HOVER,
      COUNT,
      DEFAULT = NORMAL
   };
   constexpr S32 XtankTreadCount = (S32)XtankTread::COUNT;

   // Names for on-screen display, one per XtankTread.
   extern const char *xtankTreadNames[];

   // Per-tread gameplay multipliers applied on top of the body's base physics.
   struct XtankTreadInfo
   {
      const char *name;
      F32 friction; // Multiplier on turnRate
      S32 cost;
   };

   extern XtankTreadInfo xtankTreadInfos[];                 // Tread data, original


   // One entry per XtankTread value.
   extern XtankArmorInfo xtankArmorInfos[];
   extern HeatSinkStat heatSinkStat;

   enum class XtankSuspension
   {
      LIGHT = 0,
      NORMAL,
      HEAVY,
      ACTIVE,
      COUNT,
      DEFAULT = NORMAL
   };
   constexpr S32 XtankSuspensionCount = (S32)XtankSuspension::COUNT;

   extern SuspensionStat suspensionStat[];


   enum class XtankBumper
   {
      NONE = 0,
      NORMAL,
      RUBBER,
      RETRO,
      COUNT,
      DEFAULT = NONE
   };
   constexpr S32 XtankBumperCount = (S32)XtankBumper::COUNT;

   // Bumpers: 4 options (None, Normal, Rubber, Retro)
   extern BumperStat bumperStat[];

   // ---------------------------------------------------------------------------
   // Heat sinks: player-selectable count (1-6) that reduces weapon fire delay.
   // More heat sinks allow the weapons to cycle faster.
   // ---------------------------------------------------------------------------

   static const S32 MIN_HEATSINKS = 0;
   static const S32 MAX_HEAT_SINKS = 99;

   static const S32 XtankHeatSinkDefault = 0;

   // ---------------------------------------------------------------------------
   // Vehicle design: per-player xtank configuration (body + weapon loadout).
   // ---------------------------------------------------------------------------

   enum class XtankMountLocation
   {
      TURRET1 = 0,
      TURRET2,
      TURRET3,
      TURRET4,
      FRONT,
      BACK,
      LEFT,
      RIGHT,
      COUNT,
      LAST = COUNT - 1,
      NONE = -1,
      LAST_TURRET = TURRET4,
   };
   constexpr S32 XtankMountLocationCount = (S32)XtankMountLocation::COUNT;

   struct XtankWeaponAssignment
   {
      XtankWeapon weapon;
      XtankMountLocation mount;
   };

   // Default weapon assignments for each body.
   struct XtankBodyDefaultWeapons
   {
      XtankWeaponAssignment slots[WEAPON_SLOTS];
   };

   // One entry per XtankBody value.
   extern XtankBodyDefaultWeapons xtankDefaultWeapons[VehicleBodyCount];
   extern const char *getMountLabel(XtankMountLocation mount);

   // ---------------------------------------------------------------------------
   // Xtank special equipment: player-selectable extras that provide gameplay bonuses.
   // Each special is independently toggleable (bitmask in XtankDesign::specials).
   // ---------------------------------------------------------------------------
   enum XtankSpecial
   {
      SPECIAL_CONSOLE     = 0,  // Chat/command access
      SPECIAL_MAPPER      = 1,  // Show full map
      SPECIAL_RADAR       = 2,  // Enhanced enemy detection
      SPECIAL_REPAIR      = 3,  // Auto-repair over time
      SPECIAL_RAMPLATE    = 4,  // Bonus damage when ramming
      SPECIAL_HUD         = 5,  // Heads-up display upgrades
      SPECIAL_STEALTH     = 6,  // Reduced sensor visibility
      SPECIAL_NAVIGATION  = 7,  // Navigation aids
      SPECIAL_NEW_RADAR   = 8,  // Next-gen radar
      SPECIAL_TACLINK     = 9,  // Tactical data-link with allies
      SPECIAL_CAMO        = 10, // Camouflage
      SPECIAL_RDF         = 11, // Radio direction-finding
      XtankSpecialCount   = 12,
   };

   const S32 MAX_SPECIALS = 1 << XtankSpecialCount;      // 2^12 = 4096 

   struct XtankSpecialInfo
   {
      const char *name;        // Display name
      const char *description; // Short gameplay effect description
      S32 weight;              // Equipment weight
      S32 space;               // Space required
      S32 cost;                // Purchase cost
   };

   extern XtankSpecialInfo xtankSpecialInfos[XtankSpecialCount];


   // The player's active vehicle configuration.  Stored in Ship and communicated
   // via Move (bodyIndex, weaponSlot[], weaponMount[], engineType, treadType,
   // heatSinkCount, specials).


S32 getXtankMountBit(XtankMountLocation mount);


const S32 MAX_ARMOR_PER_SIDE = 999;    // Arbitrary cap imported from original -- you could theoretically fit 2082 kevlar armor units on a Panzy

class VehicleDesign : public DesignTracker
{
   public:
      VehicleDesign();                    // Default constructor (bodyIndex = None)
      VehicleDesign(const Vector<U8> &design);   // Constructor from serialized

      Vector<U8> pack() const;                 // Serialize to byte vector for network transmission
      void unpack(const Vector<U8> design);    // Deserialize

      bool operator == (const DesignTracker &other) const override;

      XtankBody body;           // XtankBody, -1 = normal BF ship
      array<XtankWeapon, WEAPON_SLOTS> weapons;    // active weapon per weapon number slot (0..5)
      array<XtankMountLocation, WEAPON_SLOTS> weaponMounts;    // Where weapons are mounted
      array<XtankMountLocation, WEAPON_SLOTS> preferredMounts; // Where players might prefer weapon to be mounted
      XtankEngine engine; // selected engine
      XtankTread tread;   // selected tread type
      S32 heatSinks;       // number of heat sinks
      XtankArmor armor;
      XtankSuspension suspension;  // index into suspensionStat[]
      XtankBumper bumper;          // index into bumperStat[]
      U32 specials;                // bitmask of XtankSpecial bits
      array<S32, VehicleSidesCount> armorSides;  // per-side armor points (front=0, back=1, left=2, right=3, top=4, bottom=5)

      void init();      // Set body + reset all components to defaults; clear() plus a little more
      void reset();     // Reset this to its factory settings

      const char *getArmorName() const;      // Name of currently selected
      S32 getBodySize() const;
      bool isXtankVehicle() const;


      void reduceArmor(VehicleSides side, S32 amount = 1);
      void increaseArmor(VehicleSides side, S32 amount = 1);

      void nextArmor();
      void previousArmor();

      void nextMount(S32 slot);
      void previousMount(S32 slot);


      void writeToStream(BitStream *stream) override;
      void readFromStream(BitStream *stream) override;

      static bool isMountCompatible(XtankBody body, XtankWeapon weapon, XtankMountLocation mount);
      static XtankMountLocation firstValidMount(XtankBody body, XtankWeapon weapon, XtankMountLocation preferred);

      void selected(Phase phase, S32 index, S32 slot);
      void setWeapon(S32 slot, XtankWeapon weapon, XtankMountLocation mountPoint);
      S32 slotsInUse() const;

      S32 heatDissipation() const;

      inline bool hasSpecial(S32 s) const
      {
         return (specials & (U16)(1u << (U32)s)) != 0;      // Return true if the sth bit of specials is set
      }


      inline void toggleSpecial(S32 s)
      {
         specials = specials ^ (U16)(1u << (U32)s);             // Toggle the sth bit of specials
      }



      S32 getSpecialsSpace() const;
      S32 getSpecialsWeight() const;
      S32 getSpecialsCost() const;
      string getValidMountList(XtankWeapon weapon) const;

      bool isValid() const override;   
      bool isValidForLevel(bool engineerAllowed) const override;

      void set(const string &loadoutStr) override;    // Deserialize from a string representation
      void saveDesignStats(Statistics &statistics) const override;
};


/* Other options */
#define F_OUTP 0x0F /* mask.  lower 4 bits specify which levels of */
                    /* outposts can fire this weapon.  0 means the */
                    /* outpost will never fire this weapon. Otherwise */
                    /* should be set to 1..10 */

#define F_CHO (1 << 4) /* the bullet can hurt owner after first 10 frames of it's life */

/* Weapon creation options */
// Probably delete these
#define F_CR3 (1 << 0)  /* create 3 in a fan (oil slicks) */
#define F_MAP (1 << 1)  /* bullet is fired by clicking on map window */
#define F_BOTH (1 << 2) /* bullet is fired from map or anim window */
#define F_NREL (1 << 3) /* bullet is never fired w/relative velocity */

/* Display options */   // Probably delete these
#define F_BL 1          /* blue hud-line (default (0) is grey) */
#define F_RE 2          /* red */
#define F_OR 3          /* orange */
#define F_YE 4          /* yellow */
#define F_GR 5          /* green */
#define F_VI 6          /* violet */
#define F_CM 7          /* mask for above colors */
#define F_NOHD (1 << 3) /* don't draw a hud-line for this weapon */
#define F_TELE (1 << 4) /* shooter can swich to bullet view */
#define F_ROT (1 << 5)  /* bitmap is a "movie", like mine */
#define F_TRL (1 << 6)  /* bullet needs an exaust trail */
#define F_BEAM (1 << 7) /* bullet is drawn as a line segment (laser) */
#define F_NOPT (1 << 8) /* don't display this as a point */
#define F_TAC (1 << 9)  /* show the bullet on tac-link */

/* Movement options */
#define F_KEYB (1 << 0) /* steer w/keyboard (i.e. tow missles) */
#define F_MINE (1 << 1) /* move 5 frames, then stop */
#define F_DET (1 << 2)  /* (for area weapons) explode at end of life */

/* Hit options */
#define AREA (1 << 0)    /* area explosion (damages all sides) */
#define F_NOHIT (1 << 1) /* the bullet won't hit vehicles (e.g. nukes) */
#define F_SLICK (1 << 2) /* oil slicks */
#define F_HOVER (1 << 3) /* (for height=-1 bullets) random chance to hit hovers */

/* one of these are passed to the special hit function */
#define HIT_VEH 1
#define HIT_OUTP 2
#define HIT_WALL 3

/* Mount flags */
#define M_FRONT (1 << 0)
#define M_BACK (1 << 1)
#define M_LEFT (1 << 2)
#define M_RIGHT (1 << 3)
#define M_TURRET (1 << 4)


#define M_SIDES M_FRONT | M_BACK | M_LEFT | M_RIGHT
#define M_LR M_LEFT | M_RIGHT
#define M_ALL M_SIDES | M_TURRET

   /*
    * Table of initial height values
    */

#define LOW -1
#define NORM 0
#define HIGH 1
#define FLY 9

#define BIG S32_MAX







} /* namespace Zap */
#endif /* _XTANK_SHAPE_H_ */
