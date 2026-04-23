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
// Keep this file and its companion XtankShape.cpp cleanly separated from the
// rest of the Bitfighter codebase.  The only integration points are:
//   - ship.h / ship.cpp  (mXtankBodyIndex/mXtankDesign fields, cycleXtankBody(), tank physics)
//   - UIGame.cpp         (Ctrl+Alt+Shift+X hotkey; BINDING_LOADOUT xtank design menu)
//   - move.h / move.cpp  (Move::bodyIndex, Move::weaponSlot[], Move::engineType,
//                         Move::treadType, Move::heatSinkCount, Move::specials fields)
//   - gameWeapons.h/cpp  (GameWeapon::createXtankProjectile)
//   - UIXtankHelper.h/cpp (vehicle design helper menu)
//   - LoadoutIndicator.h/cpp (HUD panel)
//------------------------------------------------------------------------------

#ifndef _XTANK_SHAPE_H_
#define _XTANK_SHAPE_H_

#include "ShipShape.h"     // for ShipShapeInfo
#include "WeaponInfo.h"    // for WeaponType (used in XtankWeaponInfo::bfWeapon) and ProjectileStyle

namespace Zap
{

#define MAX_ACCEL 2.5   // The amount a vehicle can accelerate under perfect conditions
#define MAX_WEAPONS 6   // Number of weapons allowed on any one tank

   static const S32 XtankMaxWeapons = MAX_WEAPONS;

   // Names of each xtank body, kept in the same order as the enum below.
   // Used for on-screen display when the player cycles bodies.
   extern const char *xtankBodyNames[];

   // Enum of all 14 xtank vehicle bodies (in the order they appear in xtank's
   // objects.c).  XtankBodyNone represents "show the normal BF ship".
   enum XtankBody
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
      XtankBodyCount,    // total number of xtank bodies
      XtankBodyNone = -1 // sentinel: use the regular BF ship shape
   };

   struct XtankBodyInfo
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
   extern ShipShapeInfo xtankBodyInfos[XtankBodyCount];

   // Pre-computed bounding-circle radius for each xtank body (maximum distance
   // from origin to any hull vertex).  Used for collision detection so the
   // hitbox reflects the actual hull size rather than the BF default.
   extern F32 xtankBodyCollisionRadius[XtankBodyCount];

   // Y coordinate (in body space, +Y = forward/nose) of the foremost hull vertex.
   // Used by the vehicle overlay renderer to position the nose chevron and to
   // distribute heat-sink / engine indicators along the hull.
   extern F32 xtankBodyNoseY[XtankBodyCount];

   // Y coordinate of the rearmost hull vertex (always negative or small positive).
   extern F32 xtankBodyRearY[XtankBodyCount];

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
   extern XtankBodyTurrets xtankTurretInfos[XtankBodyCount];

   // Original xtank body data (objects.c/body-defs.h style fields).
   // Movement is derived from this table plus selected engine/treads.
   extern XtankBodyInfo body_stat[];

   // ---------------------------------------------------------------------------
   // Xtank weapon catalogue.
   //
   // Each xtank weapon maps to an existing Bitfighter WeaponType for projectile
   // behavior and damage (so we reuse the well-tuned BF projectile system).
   // Fire delay and energy cost are xtank-specific and storedype here.
   // ---------------------------------------------------------------------------

   // Enum of all xtank weapons (native from xtank game).
   // XtankWeaponNone represents "this turret slot carries no weapon".
   enum XtankWeapon
   {
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
      XtankWeaponCount,    // total number of xtank weapons
      XtankWeaponNone = -1 // sentinel: slot carries no weapon
   };

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

   // Names for on-screen display, one per XtankWeapon.
   extern const char *xtankWeaponNames[];

   // Per-weapon parameters: native xtank stats + BF integration fields.
   struct XtankWeaponInfo
   {
      const char *name;      // Display name
      S32 damage;            // Damage per hit (xtank native)
      S32 max_ammo;          // Maximum ammo capacity
      S32 reload_time;       // Reload time between shots (xtank frames)
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
      S32 mount;             // Where weapon can be mounted (bitflags)
      U64 other_flgs;        // Misc. flags
      U64 creat_flgs;        // Bullet creation flags
      U64 disp_flgs;         // Display flags
      U64 move_flgs;         // Movement flags
      U64 hit_flgs;          // Hit/damage flags
      // Bitfighter integration fields:
      WeaponType bfWeapon;   // Mapped BF weapon for projectile behavior
      ProjectileStyle style; // Rendering style
   };

   // One entry per XtankWeapon value.
   extern XtankWeaponInfo xtankWeaponInfos[XtankWeaponCount];

   // Helper functions to compute BF-compatible values from native xtank fields.
   // Xtank runs at 20fps, so 1 frame = 50ms.
   inline U32 xtankFireDelayMs(const XtankWeaponInfo &wi)
   {
      return (U32)(wi.reload_time * 50);  // frames → milliseconds
   }

   inline U32 xtankProjVelocity(const XtankWeaponInfo &wi)
   {
      return (U32)(wi.ammo_speed * 20);  // xtank units/frame → BF units/sec
   }

   inline S32 xtankProjLiveTime(const XtankWeaponInfo &wi)
   {
      return (S32)(wi.frames * 50);  // frames → milliseconds
   }

   // ---------------------------------------------------------------------------
   // Engine types: player-selectable power plant affecting top speed and
   // acceleration.  A heavier engine gives more power but adds bulk.
   // ---------------------------------------------------------------------------

   enum XtankEngine
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
      XtankEngineCount,
      XtankEngineDefault = Super_Combustion
   };

   enum XtankArmor
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
      XtankArmorCount,
      XtankArmorDefault = Steel
   };

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

   enum XtankTread      // united
   {
      TREAD_SMOOTH = 0,
      TREAD_NORMAL,
      TREAD_CHAINED,
      TREAD_SPIKED,
      TREAD_HOVER,
      XtankTreadCount,
      XtankTreadDefault = TREAD_NORMAL
   };

   //enum XtankTread
   //{
   //   // AI values
   //   Rubber = 0, // Light, nimble; faster turning, slightly less grip
   //   Metal = 1,  // Balanced standard
   //   Heavy = 2,  // Slow to turn, but high grip / rapid deceleration

   //   XtankTreadCount,
   //   XtankTreadDefault = Metal
   //};

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

   // Suspension: 4 options (Light, Normal, Heavy, Active)
   static const S32 XtankSuspensionCount   = 4;
   static const S32 XtankSuspensionDefault = 1;  // Normal
   extern SuspensionStat suspensionStat[];

   // Bumpers: 4 options (None, Normal, Rubber, Retro)
   static const S32 XtankBumperCount   = 4;
   static const S32 XtankBumperDefault = 0;  // None
   extern BumperStat bumperStat[];

   // ---------------------------------------------------------------------------
   // Heat sinks: player-selectable count (1-6) that reduces weapon fire delay.
   // More heat sinks allow the weapons to cycle faster.
   // ---------------------------------------------------------------------------

   static const S32 XtankHeatSinkMin = 1;
   static const S32 XtankHeatSinkMax = 6;
   static const S32 XtankHeatSinkDefault = 1;

   // Returns the fire-delay multiplier for a given heat-sink count.
   // 1 sink → 1.00x, each additional sink reduces delay by 8%.
   // 6 sinks → 0.60x (40% faster cycling).
   inline F32 xtankHeatSinkFireDelayMult(S32 count)
   {
      return 1.0f - (count - 1) * 0.08f;
   }

   // ---------------------------------------------------------------------------
   // Vehicle design: per-player xtank configuration (body + weapon loadout).
   // ---------------------------------------------------------------------------

   enum XtankMountLocation
   {
      MOUNT_TURRET1 = 0,
      MOUNT_TURRET2,
      MOUNT_TURRET3,
      MOUNT_TURRET4,
      MOUNT_FRONT,
      MOUNT_BACK,
      MOUNT_LEFT,
      MOUNT_RIGHT,
      XtankMountCount,
      XtankMountNone = -1,
   };

   struct XtankWeaponAssignment
   {
      XtankWeapon weapon;
      XtankMountLocation mount;
   };

   // Default weapon assignments for each body.
   struct XtankBodyDefaultWeapons
   {
      XtankWeaponAssignment slots[XtankMaxWeapons];
   };

   // One entry per XtankBody value.
   extern XtankBodyDefaultWeapons xtankDefaultWeapons[XtankBodyCount];

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

   struct XtankSpecialInfo
   {
      const char *name;        // Display name
      const char *description; // Short gameplay effect description
      S32 weight;              // Equipment weight
      S32 space;               // Space required
      S32 cost;                // Purchase cost
   };

   extern XtankSpecialInfo xtankSpecialInfos[XtankSpecialCount];

   inline bool hasSpecial(U16 specials, XtankSpecial s)
   {
      return (specials & (U16)(1u << (U32)s)) != 0;
   }

   inline U16 toggleSpecial(U16 specials, XtankSpecial s)
   {
      return specials ^ (U16)(1u << (U32)s);
   }

   // The player's active vehicle configuration.  Stored in Ship and communicated
   // via Move (bodyIndex, weaponSlot[], weaponMount[], engineType, treadType,
   // heatSinkCount, specials).
   struct XtankDesign
   {
      S8 bodyIndex;           // XtankBody, -1 = normal BF ship
      XtankWeapon weapons[XtankMaxWeapons];      // active weapon per weapon number slot (0..5)
      S8 weaponMounts[XtankMaxWeapons];          // XtankMountLocation per weapon slot
      XtankEngine engineType; // selected engine
      XtankTread treadType;   // selected tread type
      S8 heatSinkCount;       // number of heat sinks (1-6)
      XtankArmor armorType;
      S8 suspensionType;      // index into suspensionStat[]
      S8 bumperType;          // index into bumperStat[]
      U16 specials;           // bitmask of XtankSpecial bits
      U8 armorSides[6];       // per-side armor points (front=0, back=1, left=2, right=3, top=4, bottom=5)

      XtankDesign();                 // Default constructor (bodyIndex = None)
      void initForBody(S32 bodyIdx); // Set body + reset all components to defaults
   };

/* Other options */
#define F_OUTP 0x0F /* mask.  lower 4 bits specify which levels of */
                    /* outposts can fire this weapon.  0 means the */
                    /* outpost will never fire this weapon. Otherwise */
                    /* should be set to 1..10 */

#define F_CHO (1 << 4) /* the bullet can hurt owner after first 10 frames of it's life */

/* Creation options */
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

#define BIG ((int) ((unsigned)(~0) >> 1))







} /* namespace Zap */
#endif /* _XTANK_SHAPE_H_ */
