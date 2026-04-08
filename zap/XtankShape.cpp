//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------
//
// XtankShape.cpp - Vehicle body shape data derived from the xtank game.
//
// Polygon vertices were extracted from the xtank Display/Objects/*.obj files
// (picinfo[0], the east-facing / 0-degree view) and then rotated 90 degrees
// counter-clockwise:  (x, y) -> (-y, x), so the vehicle nose points up (+y),
// matching the Bitfighter ship drawing convention.
//
// All shapes use the same coordinate scale to preserve relative sizes.
// This means some bodies extend beyond Ship::CollisionRadius and some are
// smaller -- that can be addressed later.
//
// Three bodies lack polygon segment data in the original xtank .obj files
// (Rhino, Delta, Disk) and use hand-crafted shapes sized to match the scale
// of neighbouring vehicles.
//
// No thruster flames are defined for xtank bodies.
//
// See XtankShape.h for the XtankBody and XtankWeapon enums that index the
// arrays in this file.
//------------------------------------------------------------------------------

#include "XtankShape.h"

namespace Zap
{

const char *xtankBodyNames[XtankBody::Count] =
{
   "Lightcycle",
   "Trike",
   "Hexo",
   "Spider",
   "Psycho",
   "Tornado",
   "Marauder",
   "Tiger",
   "Rhino",
   "Medusa",
   "Delta",
   "Disk",
   "Malice",
   "Panzy",
};


// Expands to four zero-count, zero-initialized flame fields (forward, reverse,
// port, starboard).  Xtank bodies have no thruster flames.
#define NO_FLAMES  0, { },   0, { },   0, { },   0, { }


// ---------------------------------------------------------------------------
// ShipShapeInfo array for all xtank vehicles.
// Array layout mirrors the order of XtankBody::Type enum values.
//
// All shapes use the same coordinate scale to preserve relative sizes.
// Raw extents (max coord from origin):
//   Trike 11  Lightcycle 15  Hexo/Spider/Malice 17  Tornado 18
//   Tiger 19  Psycho/Marauder 21  Delta ~21  Disk ~17  Medusa/Rhino 27  Panzy 31
// ---------------------------------------------------------------------------
ShipShapeInfo xtankBodyInfos[XtankBody::Count] =
{
   // -------------------------------------------------------------------------
   // XtankBody::Lightcycle  (max extent: 15)
   // Small pointed diamond -- the fastest, lightest vehicle.
   // -------------------------------------------------------------------------
   {
      4,   { 0,-15,   7,7,   0,14,   -7,7 },      // outer hull
      0,   { },                                     // inner hull
      4,   { 0,-15,   7,7,   0,14,   -7,7 },      // corners
      0,   { },                                     // flame ports
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Trike  (max extent: 11)
   // Three-wheeled vehicle; wide flat back, pointed nose.
   // -------------------------------------------------------------------------
   {
      5,   { -8,-11,   8,-11,   8,1,   0,11,   -8,1 },
      0,   { },
      5,   { -8,-11,   8,-11,   8,1,   0,11,   -8,1 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Hexo  (max extent: 17)
   // Six-sided body (pentagon); mid-size.
   // -------------------------------------------------------------------------
   {
      5,   { 7,-11,   13,0,   0,17,   -13,0,   -7,-11 },
      0,   { },
      5,   { 7,-11,   13,0,   0,17,   -13,0,   -7,-11 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Spider  (max extent: 17)
   // Regular hexagon; wide and squat.
   // -------------------------------------------------------------------------
   {
      6,   { 8,-13,   17,0,   8,13,   -8,13,   -17,0,   -8,-13 },
      0,   { },
      6,   { 8,-13,   17,0,   8,13,   -8,13,   -17,0,   -8,-13 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Psycho  (max extent: 21)
   // Asymmetric, slightly lopsided shape.
   // -------------------------------------------------------------------------
   {
      5,   { 16,-19,   16,4,   -7,21,   -14,17,   -14,-14 },
      0,   { },
      5,   { 16,-19,   16,4,   -7,21,   -14,17,   -14,-14 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Tornado  (max extent: 18)
   // Rectangular body (width < height), with inset rectangle detail.
   // -------------------------------------------------------------------------
   {
      4,   { 13,-18,   13,18,   -13,18,   -13,-18 },

      1,
      {
         { 5, { -10,-14,   10,-14,   10,14,   -10,14,   -10,-14 } },
      },

      4,   { 13,-18,   13,18,   -13,18,   -13,-18 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Marauder  (max extent: 21)
   // Slightly tapered rectangle (wider at back than front).
   // -------------------------------------------------------------------------
   {
      4,   { 15,-21,   11,21,   -11,21,   -15,-21 },
      0,   { },
      4,   { 15,-21,   11,21,   -11,21,   -15,-21 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Tiger  (max extent: 19)
   // Wide rectangle -- one of the larger tanks, with inset box detail.
   // -------------------------------------------------------------------------
   {
      4,   { 17,-19,   17,19,   -17,19,   -17,-19 },

      1,
      {
         { 5, { -13,-15,   13,-15,   13,15,   -13,15,   -13,-15 } },
      },

      4,   { 17,-19,   17,19,   -17,19,   -17,-19 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Rhino  (max extent: 27)
   // The heaviest vehicle.  The original xtank segment data is a degenerate
   // horizontal bar; approximated here as a tapered capsule whose length
   // matches the bar's half-extent of 27 units.
   // -------------------------------------------------------------------------
   {
      8,   { 6,-27,   12,-18,   12,18,   6,27,   -6,27,   -12,18,   -12,-18,   -6,-27 },

      1,
      {
         { 9, { 4,-21,   8,-14,   8,14,   4,21,   -4,21,   -8,14,   -8,-14,   -4,-21,   4,-21 } },
      },

      8,   { 6,-27,   12,-18,   12,18,   6,27,   -6,27,   -12,18,   -12,-18,   -6,-27 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Medusa  (max extent: 27)
   // Wide triangle -- similar silhouette to the BF Normal ship but larger.
   // -------------------------------------------------------------------------
   {
      3,   { 0,27,   -22,-17,   22,-17 },

      1,
      {
         { 4, { -14,-14,   0,23,   14,-14,   -14,-14 } },
      },

      3,   { 0,27,   -22,-17,   22,-17 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Delta  (max extent: ~21)
   // Delta-wing / flying-wing body.  The original xtank delta.obj stores only
   // turret positions, no polygon segments; hand-crafted to fit between
   // Psycho/Marauder (~21) and Tiger (~19) in the size ordering.
   // -------------------------------------------------------------------------
   {
      6,   { 0,20,   19,-11,   7,-7,   0,-18,   -7,-7,   -19,-11 },

      1,
      {
         { 5, { 0,16,   12,-8,   4,-5,   -4,-5,   -12,-8 } },
      },

      6,   { 0,20,   19,-11,   7,-7,   0,-18,   -7,-7,   -19,-11 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Disk  (max extent: ~17)
   // Circular "flying saucer" tank, approximated as an octagon.  The original
   // disk.obj stores only turret positions; sized to match the medium-sized
   // vehicles (Hexo, Spider, Malice) at ~17 units.
   // -------------------------------------------------------------------------
   {
      8,   { 0,-17,   12,-12,   17,0,   12,12,   0,17,   -12,12,   -17,0,   -12,-12 },

      1,
      {
         { 9, { 0,-12,   8,-8,   12,0,   8,8,   0,12,   -8,8,   -12,0,   -8,-8,   0,-12 } },
      },

      8,   { 0,-17,   12,-12,   17,0,   12,12,   0,17,   -12,12,   -17,0,   -12,-12 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Malice  (max extent: 17)
   // Wide pentagon; an offensive-looking wedge shape.
   // -------------------------------------------------------------------------
   {
      5,   { 7,-16,   17,-2,   0,17,   -17,-2,   -7,-16 },
      0,   { },
      5,   { 7,-16,   17,-2,   0,17,   -17,-2,   -7,-16 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Panzy  (max extent: 31)
   // The largest body; a big square tank with two L-shaped interior details.
   // -------------------------------------------------------------------------
   {
      4,   { -31,-31,   31,-31,   31,31,   -31,31 },

      2,
      {
         { 10, { -11,20,  -20,20,  -20,11,  -11,11,  -11,0,  -20,0,  -20,-9,  -11,-9,  -11,-20,  -20,-20 } },
         { 10, {  11,20,   20,20,   20,11,   11,11,   11,0,   20,0,   20,-9,   11,-9,   11,-20,   20,-20  } },
      },

      4,   { -31,-31,   31,-31,   31,31,   -31,31 },
      0,   { },
      NO_FLAMES,
   },

}; // xtankBodyInfos[]

#undef NO_FLAMES


// ---------------------------------------------------------------------------
// Pre-computed bounding-circle radius for each xtank body.
//
// Each value is the maximum Euclidean distance from the origin to any hull
// vertex (i.e. the radius of the smallest enclosing circle centred at the
// origin).  Derived from the vertex data above.  Used to give each body an
// accurate collision hitbox.
//
// Note: Panzy (43.8) is much larger than the standard Ship::CollisionRadius=24
// because it really is a giant body in xtank, nearly 3× the size of a normal
// ship.  This is intentional and faithful to the original game.  Panzy players
// should expect to fill narrow corridors.
// ---------------------------------------------------------------------------
F32 xtankBodyCollisionRadius[XtankBody::Count] =
{
   15.0f,  // Lightcycle  : farthest vertex (0,-15) → dist 15
   13.6f,  // Trike       : farthest vertex (-8,-11) → sqrt(64+121)≈13.6
   17.0f,  // Hexo        : farthest vertex (0,17)   → dist 17
   17.0f,  // Spider      : farthest vertex (±17,0)  → dist 17
   24.8f,  // Psycho      : farthest vertex (16,-19) → sqrt(256+361)≈24.8
   22.2f,  // Tornado     : farthest vertex corners  → sqrt(169+324)≈22.2
   25.8f,  // Marauder    : farthest vertex (15,-21) → sqrt(225+441)≈25.8
   25.5f,  // Tiger       : farthest vertex corners  → sqrt(289+361)≈25.5
   27.7f,  // Rhino       : farthest vertex (6,-27)  → sqrt(36+729)≈27.7
   27.8f,  // Medusa      : farthest vertex (±22,-17)→ sqrt(484+289)≈27.8
   21.9f,  // Delta       : farthest vertex (±19,-11)→ sqrt(361+121)≈21.9
   17.0f,  // Disk        : farthest vertex (±17,0)  → dist 17
   17.5f,  // Malice      : farthest vertex (7,-16)  → sqrt(49+256)≈17.5
   43.8f,  // Panzy       : farthest vertex (±31,±31)→ sqrt(961+961)≈43.8
}; // xtankBodyCollisionRadius[]


// ---------------------------------------------------------------------------
// Turret mount positions for each xtank vehicle body.
//
// Coordinates are in body space using the same scale as the hull vertices
// above (+Y = nose direction).  Positions are chosen to sit roughly in the
// centre of each hull so the barrel is visually unobstructed.
// ---------------------------------------------------------------------------
XtankBodyTurrets xtankTurretInfos[XtankBody::Count] =
{
   // Lightcycle  – 1 central turret
   { 1, { { 0,  0 } } },

   // Trike       – 1 turret slightly toward the nose
   { 1, { { 0, -2 } } },

   // Hexo        – 1 central turret
   { 1, { { 0,  3 } } },

   // Spider      – 1 central turret
   { 1, { { 0,  0 } } },

   // Psycho      – 1 turret near centre of mass
   { 1, { { 1,  2 } } },

   // Tornado     – 1 central turret
   { 1, { { 0,  0 } } },

   // Marauder    – 1 central turret
   { 1, { { 0,  0 } } },

   // Tiger       – 2 symmetric side turrets
   { 2, { { -7, 0 }, { 7, 0 } } },

   // Rhino       – 2 symmetric side turrets
   { 2, { { -5, 0 }, { 5, 0 } } },

   // Medusa      – 2 turrets spread across the wide nose
   { 2, { { -7,  5 }, { 7,  5 } } },

   // Delta       – 2 turrets on the swept wings
   { 2, { { -6,  0 }, { 6,  0 } } },

   // Disk        – 1 central turret (saucer style)
   { 1, { { 0,  0 } } },

   // Malice      – 1 turret near the forward point
   { 1, { { 0,  3 } } },

   // Panzy       – 4 corner turrets on the large square body
   { 4, { { -14, -14 }, { 14, -14 }, { -14, 14 }, { 14, 14 } } },
}; // xtankTurretInfos[]


// ---------------------------------------------------------------------------
// Per-body tank driving physics parameters.
//
// Ordered to match XtankBody::Type enum values.
//
// Design intent:
//   - Light/small bodies (Lightcycle, Trike) are fast and nimble.
//   - Medium bodies (Hexo, Spider, Tornado, Delta) offer balanced handling.
//   - Heavy bodies (Tiger, Rhino, Medusa, Malice, Panzy) are slow but
//     hit hard and are hard to redirect.
//   - Disk has unusually high turnRate (saucer can spin in place easily).
//   - Marauder and Psycho are spirited "sports" tanks.
//   - All reverse speeds are ~55-60 % of forward max.
//
// Units:  speed / reverseSpeed in units/sec  (BF ship MaxVelocity = 450)
//         acceleration / friction in units/sec²  (BF ship Acceleration = 2500)
//         turnRate in radians/sec
// ---------------------------------------------------------------------------
TankPhysicsInfo xtankPhysicsInfos[XtankBody::Count] =
{
   //                   maxSpd  maxRevSpd  accel   friction  turnRate  armor
   /* Lightcycle  */  {  600,    340,      3500,    180,      2.6f,    1.10f },
   /* Trike       */  {  560,    320,      3200,    200,      2.3f,    1.05f },
   /* Hexo        */  {  520,    290,      3000,    230,      2.0f,    0.95f },
   /* Spider      */  {  490,    275,      2800,    240,      2.2f,    0.90f },
   /* Psycho      */  {  545,    310,      3250,    210,      2.4f,    1.00f },
   /* Tornado     */  {  510,    285,      2950,    235,      2.1f,    0.90f },
   /* Marauder    */  {  475,    265,      2650,    260,      1.9f,    0.85f },
   /* Tiger       */  {  450,    250,      2500,    280,      1.6f,    0.80f },
   /* Rhino       */  {  400,    220,      2100,    310,      1.2f,    0.60f },
   /* Medusa      */  {  420,    235,      2250,    295,      1.5f,    0.70f },
   /* Delta       */  {  490,    275,      2750,    250,      1.9f,    0.85f },
   /* Disk        */  {  515,    290,      3000,    190,      3.0f,    0.95f },
   /* Malice      */  {  430,    240,      2300,    290,      1.4f,    0.75f },
   /* Panzy       */  {  370,    205,      1900,    340,      1.0f,    0.50f },
}; // xtankPhysicsInfos[]


// ---------------------------------------------------------------------------
// Xtank engine types: multipliers applied on top of the per-body base physics.
// ---------------------------------------------------------------------------

const char *xtankEngineNames[XtankEngine::Count] =
{
   "Lt Engine",
   "Std Engine",
   "Hvy Engine",
};

XtankEngineInfo xtankEngineInfos[XtankEngine::Count] =
{
   //                 name           speedMult  accelMult
   /* Light    */  { "Lt Engine",    0.80f,     0.85f },
   /* Standard */  { "Std Engine",   1.00f,     1.00f },
   /* Heavy    */  { "Hvy Engine",   1.25f,     1.20f },
}; // xtankEngineInfos[]


// ---------------------------------------------------------------------------
// Xtank tread types: multipliers applied on top of the per-body base physics.
// ---------------------------------------------------------------------------

const char *xtankTreadNames[XtankTread::Count] =
{
   "Rubber Trd",
   "Metal Trd",
   "Heavy Trd",
};

XtankTreadInfo xtankTreadInfos[XtankTread::Count] =
{
   //                name           turnMult  frictionMult
   /* Rubber */  { "Rubber Trd",   1.15f,    0.85f },
   /* Metal  */  { "Metal Trd",    1.00f,    1.00f },
   /* Heavy  */  { "Heavy Trd",    0.85f,    1.20f },
}; // xtankTreadInfos[]



//
// Ordered to match XtankWeapon::Type enum values.
//
// Design intent:
//   - MachineGun / Tracer are rapid-fire, low-energy weapons.
//   - Laser uses a railgun-style fast slug with high energy cost.
//   - Missile / Rocket / Bomb are area weapons with high energy cost.
//   - Acid bounces off walls (Bouncer).
//   - Fire sprays a triple spread.
//
// fireDelay in milliseconds; energyDrain in the same energy units as BF
// ships (EnergyMax = 100000, EnergyRechargeRate = 8/ms at rest).
// bfWeapon maps to an existing BF weapon class for projectile behavior.
// ---------------------------------------------------------------------------

const char *xtankWeaponNames[XtankWeapon::Count] =
{
   "Machine Gun",
   "Laser",
   "Missile",
   "Grenade",
   "Rocket",
   "Acid",
   "Tracer",
   "Bomb",
   "Fire",
};

XtankWeaponInfo xtankWeaponInfos[XtankWeapon::Count] =
{
   // Xtank weapon parameters are derived from the original xtank weapon-defs.h.
   // Speed conversion: xtank px/frame × 20 fps = BF units/sec.
   // Lifetime conversion: xtank frames × 50 ms/frame = BF ms.
   // Reload conversion: xtank reload_frames × 50 ms/frame = BF ms.
   //
   // Colour mapping from xtank display flags:
   //   F_BL=blue, F_OR=orange, F_YE=yellow, F_GR=green, F_VI=violet, F_BEAM=laser
   //
   //                  name            fireDelay energyDrain  bfWeapon      projVelocity  projLiveTime  style
   /* MachineGun */  { "Machine Gun",   150,       300,        WeaponPhaser,  340,          1100,         ProjectileStyleXtankBlue   },  // MG:   spd=17,fr=22,tm=3
   /* Laser      */  { "Laser",         150,       8000,       WeaponRailgun, 1200,         1000,         ProjectileStyleXtankLaser  },  // LASER:spd=60,fr=20,tm=3
   /* Missile    */  { "Missile",       750,       8000,       WeaponSeeker,  500,          2550,         ProjectileStyleXtankViolet },  // SEEKER:spd=25,fr=51,tm=15
   /* Grenade    */  { "Grenade",       150,       6000,       WeaponBurst,   440,           950,         ProjectileStyleXtankOrange },  // CANNON:spd=22,fr=19,tm=3
   /* Rocket     */  { "Rocket",        400,       7000,       WeaponBurst,   800,           750,         ProjectileStyleXtankYellow },  // ROCKET:spd=40,fr=15,tm=8
   /* Acid       */  { "Acid",          150,        800,       WeaponBounce,  200,           850,         ProjectileStyleXtankGreen  },  // ACID:  spd=10,fr=17,tm=3
   /* Tracer     */  { "Tracer",        100,        400,       WeaponTurret,  340,          1100,         ProjectileStyleXtankBlue   },  // LMG:   spd=17,fr=22,tm=2
   /* Bomb       */  { "Bomb",          400,       10000,      WeaponBurst,   500,          1000,         ProjectileStyleXtankYellow },  // HROCKET:spd=40,rng~500,tm=8
   /* Fire       */  { "Fire",           50,       1500,       WeaponTriple,  240,           850,         ProjectileStyleXtankGreen  },  // FLAME: spd=12,fr=17,tm=1
}; // xtankWeaponInfos[]


// ---------------------------------------------------------------------------
// Default weapon loadout for each xtank vehicle body.
//
// The first xtankTurretInfos[bodyIdx].count entries are valid; extras are
// XtankWeapon::None.
// ---------------------------------------------------------------------------

XtankBodyDefaultWeapons xtankDefaultWeapons[XtankBody::Count] =
{
   // Lightcycle  – 1 turret
   { { XtankWeapon::MachineGun, XtankWeapon::None, XtankWeapon::None, XtankWeapon::None } },

   // Trike       – 1 turret
   { { XtankWeapon::Tracer,     XtankWeapon::None, XtankWeapon::None, XtankWeapon::None } },

   // Hexo        – 1 turret
   { { XtankWeapon::Acid,       XtankWeapon::None, XtankWeapon::None, XtankWeapon::None } },

   // Spider      – 1 turret
   { { XtankWeapon::Fire,       XtankWeapon::None, XtankWeapon::None, XtankWeapon::None } },

   // Psycho      – 1 turret
   { { XtankWeapon::Laser,      XtankWeapon::None, XtankWeapon::None, XtankWeapon::None } },

   // Tornado     – 1 turret
   { { XtankWeapon::Grenade,    XtankWeapon::None, XtankWeapon::None, XtankWeapon::None } },

   // Marauder    – 1 turret
   { { XtankWeapon::Rocket,     XtankWeapon::None, XtankWeapon::None, XtankWeapon::None } },

   // Tiger       – 2 turrets
   { { XtankWeapon::MachineGun, XtankWeapon::MachineGun, XtankWeapon::None, XtankWeapon::None } },

   // Rhino       – 2 turrets
   { { XtankWeapon::Bomb,       XtankWeapon::Bomb, XtankWeapon::None, XtankWeapon::None } },

   // Medusa      – 2 turrets
   { { XtankWeapon::Missile,    XtankWeapon::Missile, XtankWeapon::None, XtankWeapon::None } },

   // Delta       – 2 turrets
   { { XtankWeapon::Laser,      XtankWeapon::Laser, XtankWeapon::None, XtankWeapon::None } },

   // Disk        – 1 turret
   { { XtankWeapon::Fire,       XtankWeapon::None, XtankWeapon::None, XtankWeapon::None } },

   // Malice      – 1 turret
   { { XtankWeapon::Rocket,     XtankWeapon::None, XtankWeapon::None, XtankWeapon::None } },

   // Panzy       – 4 turrets
   { { XtankWeapon::MachineGun, XtankWeapon::MachineGun,
       XtankWeapon::MachineGun, XtankWeapon::MachineGun } },
}; // xtankDefaultWeapons[]


// ---------------------------------------------------------------------------
// XtankDesign implementation
// ---------------------------------------------------------------------------

XtankDesign::XtankDesign()
{
   bodyIndex = (S8)XtankBody::None;
   for(S32 i = 0; i < 4; i++)
      weapons[i] = XtankWeapon::None;
   engineType    = XtankEngine::Default;
   treadType     = XtankTread::Default;
   heatSinkCount = (S8)XtankHeatSinkDefault;
}


void XtankDesign::initForBody(S32 bodyIdx)
{
   bodyIndex = (S8)bodyIdx;
   if(bodyIdx >= 0 && bodyIdx < XtankBody::Count)
   {
      for(S32 i = 0; i < 4; i++)
         weapons[i] = xtankDefaultWeapons[bodyIdx].weapons[i];
   }
   else
   {
      for(S32 i = 0; i < 4; i++)
         weapons[i] = XtankWeapon::None;
   }
   engineType    = XtankEngine::Default;
   treadType     = XtankTread::Default;
   heatSinkCount = (S8)XtankHeatSinkDefault;
}

} /* namespace Zap */
