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
//                         Move::treadType, Move::heatSinkCount fields)
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

// Names of each xtank body, kept in the same order as the enum below.
// Used for on-screen display when the player cycles bodies.
extern const char *xtankBodyNames[];

// Enum of all 14 xtank vehicle bodies (in the order they appear in xtank's
// objects.c).  XtankBodyNone represents "show the normal BF ship".
namespace XtankBody
{
   enum Type
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
      Count,        // total number of xtank bodies
      None = -1     // sentinel: use the regular BF ship shape
   };
}

// Array of ShipShapeInfo descriptors for every xtank body.  Each entry is
// compatible with the existing renderShip() rendering path.
extern ShipShapeInfo xtankBodyInfos[XtankBody::Count];

// Pre-computed bounding-circle radius for each xtank body (maximum distance
// from origin to any hull vertex).  Used for collision detection so the
// hitbox reflects the actual hull size rather than the BF default.
extern F32 xtankBodyCollisionRadius[XtankBody::Count];


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

// One entry per XtankBody::Type value.
extern XtankBodyTurrets xtankTurretInfos[XtankBody::Count];


// ---------------------------------------------------------------------------
// Tank driving physics parameters for xtank vehicle bodies.
//
// All speed/acceleration values are in game-units per second (or per second²).
// turnRate is in radians per second.
//
// These represent a "middle-of-the-road" tuning for each body so they handle
// distinctly (heavy Rhino vs. nimble Lightcycle) while remaining playable.
// ---------------------------------------------------------------------------

struct TankPhysicsInfo
{
   F32 maxSpeed;          // Maximum forward speed (units/sec)
   F32 maxReverseSpeed;   // Maximum reverse speed (units/sec)
   F32 acceleration;      // Throttle acceleration (units/sec²)
   F32 friction;          // Passive deceleration when no throttle (units/sec²)
   F32 turnRate;          // Rotation rate of the hull (radians/sec)
   F32 armor;             // Incoming-damage multiplier (0 = immune, 1 = normal)
};

// One entry per XtankBody::Type value.
extern TankPhysicsInfo xtankPhysicsInfos[XtankBody::Count];


// ---------------------------------------------------------------------------
// Xtank weapon catalogue.
//
// Each xtank weapon maps to an existing Bitfighter WeaponType for projectile
// behavior and damage (so we reuse the well-tuned BF projectile system).
// Fire delay and energy cost are xtank-specific and stored here.
// ---------------------------------------------------------------------------

// Enum of all xtank weapons, in the order they appear in the original xtank
// game.  XtankWeapon::None represents "this turret slot carries no weapon".
namespace XtankWeapon
{
   enum Type
   {
      MachineGun = 0,
      Laser,
      Missile,
      Grenade,
      Rocket,
      Acid,
      Tracer,
      Bomb,
      Fire,
      Count,         // total number of xtank weapons
      None = -1      // sentinel: slot carries no weapon
   };
}

// Names for on-screen display, one per XtankWeapon::Type.
extern const char *xtankWeaponNames[];

// Per-weapon parameters.
struct XtankWeaponInfo
{
   const char    *name;          // Display name
   U32            fireDelay;     // Milliseconds between shots per turret
   U32            energyDrain;   // Energy units consumed per shot
   WeaponType     bfWeapon;      // Mapped BF weapon used to create the projectile
   U32            projVelocity;  // Projectile speed in BF units/sec (from xtank: spd*20)
   S32            projLiveTime;  // Projectile lifetime in ms  (from xtank: fr*50)
   ProjectileStyle style;        // Rendering style (xtank-specific look)
};

// One entry per XtankWeapon::Type value.
extern XtankWeaponInfo xtankWeaponInfos[XtankWeapon::Count];


// ---------------------------------------------------------------------------
// Engine types: player-selectable power plant affecting top speed and
// acceleration.  A heavier engine gives more power but adds bulk.
// ---------------------------------------------------------------------------

namespace XtankEngine
{
   enum Type
   {
      Light    = 0,  // Low mass, lower power
      Standard = 1,  // Balanced
      Heavy    = 2,  // High power, greater top speed and acceleration
      Count,
      Default  = Standard
   };
}

// Names for on-screen display, one per XtankEngine::Type.
extern const char *xtankEngineNames[];

// Per-engine gameplay multipliers applied on top of the body's base physics.
struct XtankEngineInfo
{
   const char *name;
   F32 speedMult;   // Multiplier on maxSpeed and maxReverseSpeed
   F32 accelMult;   // Multiplier on acceleration
};

// One entry per XtankEngine::Type value.
extern XtankEngineInfo xtankEngineInfos[XtankEngine::Count];


// ---------------------------------------------------------------------------
// Tread types: player-selectable track system affecting maneuverability.
// Rubber treads give nimble steering; heavy treads grip harder but turn slower.
// ---------------------------------------------------------------------------

namespace XtankTread
{
   enum Type
   {
      Rubber = 0,  // Light, nimble; faster turning, slightly less grip
      Metal  = 1,  // Balanced standard
      Heavy  = 2,  // Slow to turn, but high grip / rapid deceleration
      Count,
      Default = Metal
   };
}

// Names for on-screen display, one per XtankTread::Type.
extern const char *xtankTreadNames[];

// Per-tread gameplay multipliers applied on top of the body's base physics.
struct XtankTreadInfo
{
   const char *name;
   F32 turnMult;     // Multiplier on turnRate
   F32 frictionMult; // Multiplier on friction (passive deceleration)
};

// One entry per XtankTread::Type value.
extern XtankTreadInfo xtankTreadInfos[XtankTread::Count];


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

// Default weapons for each body (first slotCount entries are valid;
// slotCount comes from xtankTurretInfos[bodyIdx].count).
struct XtankBodyDefaultWeapons
{
   XtankWeapon::Type weapons[4];
};

// One entry per XtankBody::Type value.
extern XtankBodyDefaultWeapons xtankDefaultWeapons[XtankBody::Count];


// The player's active vehicle configuration.  Stored in Ship and communicated
// via Move (bodyIndex, weaponSlot[], engineType, treadType, heatSinkCount).
struct XtankDesign
{
   S8                bodyIndex;    // XtankBody::Type, -1 = normal BF ship
   XtankWeapon::Type weapons[4];  // active weapon per turret slot (extras = None)
   XtankEngine::Type engineType;  // selected engine
   XtankTread::Type  treadType;   // selected tread type
   S8                heatSinkCount; // number of heat sinks (1-6)

   XtankDesign();                        // Default constructor (bodyIndex = None)
   void initForBody(S32 bodyIdx);        // Set body + reset all components to defaults
};

} /* namespace Zap */
#endif /* _XTANK_SHAPE_H_ */
