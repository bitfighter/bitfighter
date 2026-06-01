//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------
//
// VehicleDesign.cpp - Vehicle body shape data derived from the xtank game.
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
// See VehicleDesign.h for the XtankBody and XtankWeapon enums that index the
// arrays in this file.
//------------------------------------------------------------------------------

#include "VehicleDesign.h"
#include "UIVehicleDesigner.h"
#include "MathUtils.h"
#include "TnlBitStream.h"

using std::array;

namespace Zap
{

const char *xtankBodyNames[] =
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


XtankArmorInfo xtankArmorInfos[] =
{
   /* name           def wgt spc cost */
   {"Steel",          0, 5, 3, 10  },
   {"Kevlar",         0, 3, 3, 20  },
   {"Hardened Steel", 1, 8, 3, 20  },
   {"Composite",      1, 4, 3, 30  },
   {"Carapice",       1, 7, 2, 80  },
   {"Porcelain",      1, 2, 5, 80  },
   {"Compound Steel", 2, 8, 3, 40  },
   {"Titanium",       2, 5, 3, 70  },
   {"Tungsten",       3, 8, 3, 120 },
};



// Expands to four zero-count, zero-initialized flame fields (forward, reverse,
// port, starboard).  Xtank bodies have no thruster flames.
#define NO_FLAMES  0, { },   0, { },   0, { },   0, { }


// ---------------------------------------------------------------------------
// ShipShapeInfo array for all xtank vehicles.
// Array layout mirrors the order of XtankBodyType enum values.
//
// All shapes use the same coordinate scale to preserve relative sizes.
// Raw extents (max coord from origin):
//   Trike 11  Lightcycle 15  Hexo/Spider/Malice 17  Tornado 18
//   Tiger 19  Psycho/Marauder 21  Delta ~21  Disk ~17  Medusa/Rhino 27  Panzy 31
// ---------------------------------------------------------------------------
ShipShapeInfo xtankBodyInfos[] =
{
   // -------------------------------------------------------------------------
   // XtankBody::Lightcycle  (max extent: 15)
   // Small pointed diamond -- the fastest, lightest vehicle.
   // -------------------------------------------------------------------------
   {
      4,   { 0,-15,   7,7,   0,14,   -7,7 },        // outer hull
      0,   { },                                     // inner hull
      4,   { 0,-15,   7,7,   0,14,   -7,7 },        // corners
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
   // Regular hexagon; wide and squat.  Inner forward chevron added for
   // clear directionality (nose = top, +Y).
   // -------------------------------------------------------------------------
   {
      6,   { 8,-13,   17,0,   8,13,   -8,13,   -17,0,   -8,-13 },

      1,
      {
         { 3, { -6,-3,   0,10,   6,-3 } },  // forward chevron pointing to y=+13 nose
      },

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
   // Rectangular body (width < height).  Inset rectangle kept; forward
   // chevron added so the nose (y=+18) is unmistakeable.
   // -------------------------------------------------------------------------
   {
      4,   { 13,-18,   13,18,   -13,18,   -13,-18 },

      2,
      {
         { 5, { -10,-14,   10,-14,   10,14,   -10,14,   -10,-14 } },  // inset rect
         { 3, { -7,2,   0,13,   7,2 } },                              // forward chevron
      },

      4,   { 13,-18,   13,18,   -13,18,   -13,-18 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Marauder  (max extent: 21)
   // Slightly tapered rectangle (wider at back than front).  Forward chevron
   // added so the narrower nose end (y=+21) reads clearly as the front.
   // -------------------------------------------------------------------------
   {
      4,   { 15,-21,   11,21,   -11,21,   -15,-21 },

      1,
      {
         { 3, { -7,2,   0,17,   7,2 } },  // forward chevron pointing toward y=+21 nose
      },

      4,   { 15,-21,   11,21,   -11,21,   -15,-21 },
      0,   { },
      NO_FLAMES,
   },

   // -------------------------------------------------------------------------
   // XtankBody::Tiger  (max extent: 19)
   // Wide rectangle -- one of the larger tanks.  Forward chevron added to the
   // inset box detail so the nose end (y=+19) is clear.
   // -------------------------------------------------------------------------
   {
      4,   { 17,-19,   17,19,   -17,19,   -17,-19 },

      2,
      {
         { 5, { -13,-15,   13,-15,   13,15,   -13,15,   -13,-15 } },  // inset rect
         { 3, { -8,2,   0,14,   8,2 } },                              // forward chevron
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
   // vehicles (Hexo, Spider, Malice) at ~17 units.  Inner ring kept; a forward
   // arrow added to indicate the nose direction (y=+17).
   // -------------------------------------------------------------------------
   {
      8,   { 0,-17,   12,-12,   17,0,   12,12,   0,17,   -12,12,   -17,0,   -12,-12 },

      2,
      {
         { 9, { 0,-12,   8,-8,   12,0,   8,8,   0,12,   -8,8,   -12,0,   -8,-8,   0,-12 } },  // inner ring
         { 3, { -4,2,   0,11,   4,2 } },                                                      // forward arrow
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
   // The largest body; a big square tank with two L-shaped interior details
   // and a nose chevron pointing toward y=+31.
   // -------------------------------------------------------------------------
   {
      4,   { -31,-31,   31,-31,   31,31,   -31,31 },

      3,
      {
         { 10, { -11,20,  -20,20,  -20,11,  -11,11,  -11,0,  -20,0,  -20,-9,  -11,-9,  -11,-20,  -20,-20 } },  // left L
         { 10, {  11,20,   20,20,   20,11,   11,11,   11,0,   20,0,   20,-9,   11,-9,   11,-20,   20,-20  } },  // right L
         {  3, { -10,10,   0,26,   10,10 } },                                                                   // forward chevron at nose
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
F32 xtankBodyCollisionRadius[] =
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
// Per-body nose and rear Y extents (body space, +Y = forward).
//
// noseY: Y coordinate of the foremost hull vertex.
// rearY: Y coordinate of the rearmost hull vertex (negative for most bodies).
// Used by the overlay renderer to position bling and exhaust indicators.
// ---------------------------------------------------------------------------
F32 xtankBodyNoseY[] =
{
   14.0f,  // Lightcycle  : nose vertex (0,14)
   11.0f,  // Trike       : nose vertex (0,11)
   17.0f,  // Hexo        : nose vertex (0,17)
   13.0f,  // Spider      : nose vertices (±8,13)
   21.0f,  // Psycho      : nose vertex (-7,21)
   18.0f,  // Tornado     : nose edge at y=18
   21.0f,  // Marauder    : nose vertices (±11,21)
   19.0f,  // Tiger       : nose edge at y=19
   27.0f,  // Rhino       : nose vertex (6,27)
   27.0f,  // Medusa      : nose vertex (0,27)
   20.0f,  // Delta       : nose vertex (0,20)
   17.0f,  // Disk        : nose vertex (0,17)
   17.0f,  // Malice      : nose vertex (0,17)
   31.0f,  // Panzy       : nose edge at y=31
}; // xtankBodyNoseY[]

F32 xtankBodyRearY[] =
{
  -15.0f,  // Lightcycle  : rear vertex (0,-15)
  -11.0f,  // Trike       : rear edge at y=-11
  -11.0f,  // Hexo        : rear vertices (±7,-11)
  -13.0f,  // Spider      : rear vertices (±8,-13)
  -19.0f,  // Psycho      : rear vertex (16,-19)
  -18.0f,  // Tornado     : rear edge at y=-18
  -21.0f,  // Marauder    : rear vertices (±15,-21)
  -19.0f,  // Tiger       : rear edge at y=-19
  -27.0f,  // Rhino       : rear vertex (6,-27)
  -17.0f,  // Medusa      : rear edge at y=-17
  -18.0f,  // Delta       : rear vertex (0,-18)
  -17.0f,  // Disk        : rear vertex (0,-17)
  -16.0f,  // Malice      : rear vertex (7,-16)
  -31.0f,  // Panzy       : rear edge at y=-31
}; // xtankBodyRearY[]


// ---------------------------------------------------------------------------
// Turret mount positions for each xtank vehicle body.
//
// Coordinates are in body space using the same scale as the hull vertices
// above (+Y = nose direction).  Positions are chosen to sit roughly in the
// centre of each hull so the barrel is visually unobstructed.
// ---------------------------------------------------------------------------
XtankBodyTurrets xtankTurretInfos[] =
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


// Original xtank
XtankBodyInfo2 xTankBodyStats[] =
{
   /* type     size weight wghtlim space  drag hndl trts cost */
   {"Lightcycle", 2,   200, 800,     600,  .10f, 8, 0,  3000 },
   {"Trike",      2,   400, 1600,   1200,  .15f, 6, 0,  4000 },
   {"Hexo",       3,  1500, 5000,   4000,  .25f, 6, 1,  4000 },
   {"Spider",     3,  2500, 8000,   3000,  .40f, 7, 1,  5000 },
   {"Psycho",     4,  5000, 18000,  8000,  .60f, 4, 1,  5000 },
   {"Tornado",    4,  7000, 22000, 12000,  .80f, 4, 1,  7000 },
   {"Marauder",   5,  9000, 28000, 18000, 1.00f, 3, 2, 10000 },
   {"Tiger",      6, 11000, 35000, 22000, 1.50f, 5, 1, 12000 },
   {"Rhino",      7, 15000, 55000, 35000, 2.00f, 3, 2, 10000 },
   {"Medusa",     7, 14000, 40000, 25000, 1.20f, 4, 3, 15000 },
   {"Delta",      6, 10000, 20000, 18000,  .15f, 6, 2, 14000 },
   {"Disk",       7, 15000, 35000, 25000,  .15f, 6, 2, 15000 },
   {"Malice",     5,  4000, 20000, 15000,  .40f, 7, 1, 17000 },
   {"Panzy",      8, 22000, 80000, 50000, 3.00f, 3, 4, 25000 },
};

// ---------------------------------------------------------------------------
// Xtank engine types: multipliers applied on top of the per-body base physics.
// ---------------------------------------------------------------------------


XtankEngineInfo xtankEngineInfos[] =
{
   //      name        speedMult  accelMult  power weight space  fuel fcap   cost
   { "Small Electric",   0.80f,     0.85f,    50,   100,   20,    5,    50,  1500  },
   { "Medium Electric",  1.00f,     1.00f,   100,   150,   30,    5,   100,  2200  },
   { "Large Electric",   1.25f,     1.20f,   200,   200,   40,    5,   200,  3000  },
   { "Super Electric",   1.35f,     1.25f,   300,   250,   50,    5,   300,  6000  },
   { "Small Combustion", 1.00f,     1.05f,   300,   400,  200,    8,    50,  2000  },
   { "Medium Combustion",1.10f,     1.10f,   400,   500,  300,    8,   100,  2500  },
   { "Large Combustion", 1.25f,     1.20f,   500,   600,  400,    8,   200,  3000  },
   { "Super Combustion", 1.40f,     1.25f,   600,  1000,  600,    8,   300,  4000  },
   { "Small Turbine",    1.30f,     1.15f,   600,  1000,  800,   10,   350,  4000  },
   { "Medium Turbine",   1.40f,     1.20f,   700,  1200, 1000,   10,   450,  5000  },
   { "Large Turbine",    1.55f,     1.25f,   800,  1500, 1500,   10,   550,  7000  },
   { "Turbojet Turbine", 1.75f,     1.30f,  1000,  2000, 2000,   10,   750, 10000  },
   { "Fuel Cell",        1.50f,     1.10f,  1500,  1000,  400,   20,   400, 45000  },
   { "Fission",          1.70f,     1.05f,  2000,  3000, 3500,   15,   800, 20000  },
   { "Breeder Fission",  1.90f,     1.00f,  2500,  3500, 4000,   15,  1050, 25000  },
   { "Fusion",           2.50f,     0.95f, 10000, 15000, 18000,   5,  1300, 100000 },
};


// ---------------------------------------------------------------------------
// Xtank tread types: multipliers applied on top of the per-body base physics.
// ---------------------------------------------------------------------------


// XtankTreadInfo xtankTreadInfos[] =
// {
//    //    name           turnMult  frictionMult  cost
//    { "Rubber Tread",   1.15f,    0.85f, 0 },
//    { "Metal Tread",    1.00f,    1.00f, 0 },
//    { "Heavy Tread",    0.85f,    1.20f, 0 },
// };


XtankTreadInfo xtankTreadInfos[] =
{
   //    name          friction  cost
   { "Smooth",       0.70f,  100 },
   { "Normal",       0.80f,  200 },
   { "Chained",      0.90f,  400 },
   { "Spiked",       1.00f, 1000 },
   { "Hover",        0.20f,  500 },
};



// int power = engine_stat[d->engine].power;
// float drag = body_stat[d->body].drag;
// float friction = tread_stat[d->treads].friction;

// d->max_speed = pow(power / drag, 0.3333) / friction;
// if (d->treads == HOVER_TREAD)
//    d->max_speed /= 2;

// d->engine_acc = 16.0 * power / d->weight;
// d->tread_acc = friction * MAX_ACCEL; /* weight drops out because
//              traction is proportional to
//              weight */
// d->acc = MIN(d->tread_acc, d->engine_acc);

//
// Ordered to match XtankWeapon enum values.
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


// Full 25-weapon xtank catalog with native stats + BF integration.
// Native xtank fields from weapon-defs.h (damage, reload, speed, etc.)
// BF integration: bfWeapon and style map to Bitfighter projectile system.
//
// Field order matches XtankWeaponInfo struct:
//   name, damage, max_ammo, reload_time, ammo_speed, weight, space, mount_space,
//   frames, heat, ammo_cost, cost, refill_time, safety, height, mount,
//   other_flgs, creat_flgs, disp_flgs, move_flgs, hit_flgs, bfWeapon, style
XtankWeaponInfo xtankWeaponInfos[] =      // spd (17) x frames (22) /192 * 256
{
   /*                             reload*/
   /* name               dam ammo  time  spd  wgt   spc  mspc  fr  ht  a$   cost  refl safety hgt  mount o_flgs  creat_flgs      disp_flgs    move_flgs   hit_flgs       bfWeapon         style */
   { "None",  1, 300, 133,   17,   20,  200,  200, 22,  0,  1,   1000,   1,   0, NORM,  M_ALL,  1,      NORM,           F_BL,         NORM,       NORM,    WeaponPhaser,    ProjectileStyleXtankBlue   },
   { "Light Machine Gun",  1, 300, 133,   17,   20,  200,  200, 22,  0,  1,   1000,   1,   0, NORM,  M_ALL,  1,      NORM,           F_BL,         NORM,       NORM,    WeaponPhaser,    ProjectileStyleXtankBlue   },
   { "Machine Gun",        2, 250, 200,   17,   50,  225,  225, 22,  1,  2,   2200,   1,   0, NORM,  M_ALL,  1,      NORM,           F_BL,         NORM,       NORM,    WeaponPhaser,    ProjectileStyleXtankBlue   },
   { "Heavy Machine Gun",  3, 200, 200,   17,  100,  250,  250, 22,  2,  3,   3000,   1,   0, NORM,  M_ALL,  2,      NORM,           F_BL,         NORM,       NORM,    WeaponPhaser,    ProjectileStyleXtankBlue   },
   { "Light Autocannon",   3, 250, 200,   22,  200,  300,  300, 19,  3,  4,   3000,   1,   3, NORM,  M_ALL,  2,      NORM,           F_OR,         NORM,       NORM,    WeaponPhaser,    ProjectileStyleXtankOrange },
   { "Autocannon",         4, 225, 200,   22,  300,  350,  350, 19,  4,  5,   6000,   1,   3, NORM,  M_ALL,  3,      NORM,           F_OR,         NORM,       NORM,    WeaponPhaser,    ProjectileStyleXtankOrange },
   { "Heavy Autocannon",   5, 200, 200,   22,  500,  400,  400, 19,  5,  6,  10000,   1,   3, NORM,  M_ALL,  4,      NORM,           F_OR,         NORM,       NORM,    WeaponPhaser,    ProjectileStyleXtankOrange },
   { "Light Rkt Launcher", 6, 150, 533,   40,  600,  800,  800, 15,  4,  8,   7000,   2,   3, NORM,  M_ALL,  5,      NORM,           F_YE,         NORM,       NORM,    WeaponBounce,    ProjectileStyleXtankYellow },
   { "Rkt Launcher",       7, 125, 533,   40,  900, 1200, 1200, 15,  6, 10,  10000,   2,   3, NORM,  M_ALL,  6,      NORM,           F_YE,         NORM,       NORM,    WeaponBounce,    ProjectileStyleXtankYellow },
   { "Heavy Rkt Launcher", 8, 100, 533,   40,  900, 1600, 1600, 15,  8, 12,  15000,   2,   3, NORM,  M_ALL,  7,      NORM,           F_YE,         NORM,       NORM,    WeaponBounce,    ProjectileStyleXtankYellow },
   { "Acid Sprayer",       4, 100, 200,   10,  600,  700,  700, 17,  0, 10,  10000,   1,   0, NORM,  M_ALL,  9,      NORM,           F_GR,         NORM,       NORM,    WeaponBounce,    ProjectileStyleXtankGreen  },
   { "Flame Thrower",      3, 300, 67,    12,  700,  500,  500, 17,  1,  2,   4000,   1,   0, NORM,  M_ALL,  9,      NORM,           F_GR,         NORM,       NORM,    WeaponTriple,    ProjectileStyleXtankGreen  },
   { "Heat Seeker",        8, 15,  1000,  25, 1000, 1800, 1800, 51,  9, 50,  20000,  12,   3, HIGH,  M_ALL, 10,    F_NREL,        F_TRL | F_VI,      NORM,       NORM,    WeaponSeeker,    ProjectileStyleXtankViolet },
   { "Pocket Rocket",      5, 24,  133,   40, 1200, 1900, 1900, 30,  9, 10,  17000,   2,   3, NORM,  M_ALL, 10,      NORM,          F_TRL,         NORM,       NORM,    WeaponSeeker,    ProjectileStyleXtankYellow },
   { "Unguided Missile",  10, 30,  533,   35, 1000, 1800, 1800, 57, 12, 25,  18000,   2,   3, NORM,  M_ALL,  0,      NORM,          F_TRL,         NORM,       NORM,    WeaponBounce,    ProjectileStyleXtankViolet },
   { "TeleGuided",        64, 1,   67,    15, 1500, 2000, 2000,400, 30,500,  30000, 200,   3, HIGH, M_ALL,F_CHO,  F_NREL, F_TELE | F_TRL | F_RE | F_TAC, F_KEYB,    NORM,    WeaponSeeker,    ProjectileStyleXtankViolet },
   { "TOW Missile",       64, 2,   67,    15, 1500, 2000, 2000,500, 30,100,  30000, 100,   3, HIGH, M_ALL,F_CHO,   F_NREL,    F_TRL | F_RE | F_TAC,  F_KEYB,       NORM,    WeaponSeeker,    ProjectileStyleXtankViolet },
   { "Land Torpedo",      20, 2,   67,    10, 1500, 2000, 2000,500, 30,100,  30000, 100,   3, LOW,  M_SIDES,F_CHO,   F_NREL,           F_RE,       F_KEYB,       NORM,    WeaponBounce,    ProjectileStyleXtankYellow },
   { "Blast Cannon",       1, 20,  533,   25,  300,  350,  350, 20,  6, 20, 100000,   2,   3, NORM,  M_ALL,  0,      NORM,           F_RE,         NORM,       NORM,    WeaponBurst,     ProjectileStyleXtankOrange },
   { "Pulse Laser",        3, 500, 200,   60,  300,  250,  250, 20,  3,  3,  30000,   1,   0, NORM,  M_ALL,  0,    F_NREL,       F_BEAM | F_NOPT,    NORM,       NORM,    WeaponPhaser,    ProjectileStyleXtankLaser  },
   { "Mine Layer",         6, 50,  133,   10, 1000, 1000, 1000, 70,  2, 10,   8000,   4,   0, LOW,   M_BACK,  0,      NORM, F_ROT | F_NOHD | F_NOPT,  F_MINE,    F_HOVER,   WeaponMine,      ProjectileStyleNotAProjectile },
   { "Oil Slick",          0, 50,  333,   10,  300,  500,  500, 70,  0, 10,   2000,   1,   0, LOW,   M_BACK,  0,     F_CR3,    F_NOHD | F_NOPT,     F_MINE,     F_SLICK,   WeaponMine,      ProjectileStyleNotAProjectile },
   { "Heavy Mortar",     170, 10,  1333,  50, 1400, 2000, 2000, 70, 30,500,   9000,  30,   0, FLY,  M_TURRET,0, F_MAP | F_NREL, F_NOHD | F_TAC | F_TRL, F_DET,       AREA,    WeaponBurst,     ProjectileStyleXtankOrange },
   { "Tactical Nuke",    170, 10,  1333,  10, 1600, 2200, 2200, 30,  6,500,  80000,  30,   0, NORM, M_BACK, 0,      NORM,         F_NOHD,   F_MINE | F_DET, AREA | F_NOHIT, WeaponBurst,     ProjectileStyleXtankOrange },
   { "Anti-Radiation",    64, 2,   12800, 25, 2000, 3400, 3400,100, 24,1000,100000, 192,   0, FLY,  M_LR,F_CHO, F_MAP | F_NREL,F_TRL | F_NOHD | F_TAC,   NORM,       NORM,    WeaponSeeker,    ProjectileStyleXtankViolet },
   { "Disc Shooter",       0, BIG, 67,    BIG,   0,    0,     0,   0,BIG,  0,      0,   0,   0,    0, M_ALL,  0,             0,          0,        0,          0,    WeaponPhaser,    ProjectileStyleXtankBlue   },
}; // xtankWeaponInfos[]



const char *getMountLabel(XtankMountLocation mount)
{
   switch(mount)
   {
      case XtankMountLocation::TURRET1: return "Turret 1";
      case XtankMountLocation::TURRET2: return "Turret 2";
      case XtankMountLocation::TURRET3: return "Turret 3";
      case XtankMountLocation::TURRET4: return "Turret 4";
      case XtankMountLocation::FRONT:   return "Front";
      case XtankMountLocation::BACK:    return "Back";
      case XtankMountLocation::LEFT:    return "Left";
      case XtankMountLocation::RIGHT:   return "Right";
      default:                          return "None";
   }
}

// ---------------------------------------------------------------------------
// Default weapon loadout for each xtank vehicle body.
//
// The first xtankTurretInfos[bodyIdx].count entries are valid; extras are
// XtankWeapon::NONE.
// ---------------------------------------------------------------------------

XtankBodyDefaultWeapons xtankDefaultWeapons[] =
{
   { { { XtankWeapon::MACHINE_GUN,        XtankMountLocation::TURRET1 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Lightcycle
   { { { XtankWeapon::LIGHT_MACHINE_GUN,  XtankMountLocation::TURRET1 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Trike
   { { { XtankWeapon::ACID_SPRAYER,       XtankMountLocation::TURRET1 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Hexo
   { { { XtankWeapon::FLAME_THROWER,      XtankMountLocation::TURRET1 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Spider
   { { { XtankWeapon::PULSE_LASER,        XtankMountLocation::TURRET1 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Psycho
   { { { XtankWeapon::AUTOCANNON,         XtankMountLocation::TURRET1 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Tornado
   { { { XtankWeapon::LIGHT_RKT_LAUNCHER, XtankMountLocation::TURRET1 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Marauder
   { { { XtankWeapon::MACHINE_GUN,        XtankMountLocation::TURRET1 }, { XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET2 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Tiger
   { { { XtankWeapon::BLAST_CANNON,       XtankMountLocation::TURRET1 }, { XtankWeapon::BLAST_CANNON, XtankMountLocation::TURRET2 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Rhino
   { { { XtankWeapon::HEAT_SEEKER,        XtankMountLocation::TURRET1 }, { XtankWeapon::HEAT_SEEKER, XtankMountLocation::TURRET2 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Medusa
   { { { XtankWeapon::PULSE_LASER,        XtankMountLocation::TURRET1 }, { XtankWeapon::PULSE_LASER, XtankMountLocation::TURRET2 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Delta
   { { { XtankWeapon::FLAME_THROWER,      XtankMountLocation::TURRET1 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Disk
   { { { XtankWeapon::LIGHT_RKT_LAUNCHER, XtankMountLocation::TURRET1 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Malice
   { { { XtankWeapon::HEAVY_AUTOCANNON,   XtankMountLocation::TURRET1 }, { XtankWeapon::HEAVY_AUTOCANNON, XtankMountLocation::TURRET2 }, { XtankWeapon::HEAVY_AUTOCANNON, XtankMountLocation::TURRET3 }, { XtankWeapon::HEAVY_AUTOCANNON, XtankMountLocation::TURRET4 }, { XtankWeapon::NONE, XtankMountLocation::NONE }, { XtankWeapon::NONE, XtankMountLocation::NONE } } },  // Panzy
};


// ---------------------------------------------------------------------------
// Heatsink info
// ---------------------------------------------------------------------------
HeatSinkStat heatSinkStat = { 500, 1000, 500 }; // weight, space, cost

SuspensionStat suspensionStat[] =
{
   // name     hndl cost
   { "Light",  -1,  100 },
   { "Normal",  0,  200 },
   { "Heavy",   1,  400 },
   { "Active",  2, 1000 },
};


BumperStat bumperStat[] =
{
   // name    elast   cost
   {"None",   0.00f,    0 },
   {"Normal", 0.07f,  200 },
   {"Rubber", 0.15f,  400 },
   {"Retro",  0.25f, 1000 }
};

// ---------------------------------------------------------------------------
// XtankDesign implementation
// ---------------------------------------------------------------------------

XtankSpecialInfo xtankSpecialInfos[XtankSpecialCount] =
{
   // name            description                         weight space  cost
   { "Console",       "Chat commands",                       50,    1,   250 },
   { "Mapper",        "Show full map",                      100,    2,   500 },
   { "Radar",         "Detect hidden enemies",              200,    3,  1000 },
   { "Repair",        "Auto-repair hull (+0.5%/s)",         400,    5, 30000 },
   { "Ram Plate",     "Bonus collision damage",             800,    4,  2000 },
   { "HUD",           "Heads-up display upgrades",           10,    1,     1 },
   { "Stealth",       "Reduce radar cross-section",         300,    3, 20000 },
   { "Navigation",    "Route-planning aid",                  10,    1,    20 },
   { "New Radar",     "Enhanced radar suite",               300,    4,  3000 },
   { "Tac-Link",      "Share allied telemetry",             100,    2,  1000 },
   { "Camo",          "Visual camouflage",                  200,    3,  2000 },
   { "RDF",           "Radio direction-finding",             50,    2,  1000 },
};


// Constructor
VehicleDesign::VehicleDesign()    
{
   init();
}


// Deserializing constructor
VehicleDesign::VehicleDesign(const Vector<U8> &design)
{
   init();
   unpack(design);
}


// Fills design with the serialized bytes representing this VehicleDesign. 
Vector<U8> VehicleDesign::pack() const
{
   Vector<U8> design(35);

   design.push_back(U8(body));
   design.push_back(U8(engine));
   design.push_back(U8(tread));
   design.push_back(U8(bumper));
   design.push_back(U8(suspension));
   design.push_back(U8(heatSinks));
   design.push_back(U8(armor));
   

   for(S32 i = 0; i < VehicleSidesCount; i++)
   {
      S32 value = armorSides[i];
      U8 hi = U8(value >> 8);
      U8 lo = U8(value & 0xFF);

      design.push_back(hi);
      design.push_back(lo);
   }

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      design.push_back(U8(weapons[i]));
      design.push_back(U8(weaponMounts[i]));
   }

   U8 b3 = U8(specials >> 24);
   U8 b2 = U8((specials >> 16) & 0xFF);
   U8 b1 = U8((specials >> 8) & 0xFF);
   U8 b0 = U8(specials & 0xFF);

   design.push_back(b3);
   design.push_back(b2);
   design.push_back(b1);
   design.push_back(b0);

   return design;
}


void VehicleDesign::unpack(const Vector<U8> design)
{
   S32 index = 0;

   body = (XtankBody)design[index++];
   engine = (XtankEngine)design[index++];
   tread = (XtankTread)design[index++];
   bumper = (XtankBumper)design[index++];
   suspension = (XtankSuspension)design[index++];
   heatSinks = design[index++];
   armor = (XtankArmor)design[index++];
   
   // Each side can have up to MAX_ARMOR_PER_SIDE armor, currently 999, so we need to U8s to store each side's armor value.
   for(S32 i = 0; i < VehicleSidesCount; i++)
   {
      U8 hi = design[index++];
      U8 lo = design[index++];

      U16 combined = (U16(hi) << 8) | U16(lo);

      armorSides[i] = (S32)combined;
   }
   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      weapons[i] = (XtankWeapon)design[index++];
      weaponMounts[i] = (XtankMountLocation)design[index++];
   }

   U8 b3 = design[index++];
   U8 b2 = design[index++];
   U8 b1 = design[index++];
   U8 b0 = design[index++];

   // Specials is a bitmap of up to 32 special features, so we need 4 U8s to store it.
   specials = (U32(b3) << 24) | (U32(b2) << 16) | (U32(b1) << 8) | (U32(b0));

   TNLAssert(design.size() == index, "Got wrong number of bytes!");
}


void VehicleDesign::init()
{
   reset();
   for(S32 i = 0; i < WEAPON_SLOTS; i++)
      preferredMounts[i] = XtankMountLocation::TURRET1;
}


// Transforms this design into a relatively useless standard starting point
void VehicleDesign::reset()
{
   body = XtankBody::DEFAULT;
   engine = XtankEngine::DEFAULT;
   tread = XtankTread::DEFAULT;
   heatSinks = XtankHeatSinkDefault;
   armor = XtankArmor::DEFAULT;
   suspension = XtankSuspension::DEFAULT;
   bumper = XtankBumper::DEFAULT;
   specials = 0;     // This is a bitmap, so 0 means "no specials"

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      weapons[i] = XtankWeapon::NONE;
      weaponMounts[i] = XtankMountLocation::NONE;
   }
   // Xtank-style default armor: all sides start at 0 and are set explicitly.
   for(S32 i = 0; i < VehicleSidesCount; i++)
      armorSides[i] = 0;
}


bool VehicleDesign::operator == (const DesignTracker &other) const
{
   const VehicleDesign *o = dynamic_cast<const VehicleDesign *>(&other);
   TNLAssert(o, "Comparing VehicleDesign with non-VehicleDesign!");

   if(!o)
      return false;


   if(engine != o->engine || tread != o->tread || armor != o->armor ||
      suspension != o->suspension || bumper != o->bumper || heatSinks != o->heatSinks ||
      specials != o->specials)
      return false;

   for(S32 i = 0; i < VehicleSidesCount; i++)
      if(armorSides[i] != o->armorSides[i])
         return false;

   // Unordered pairwise comparison: same set of (weapon, mount) pairs regardless of slot order.
   using WMPair = std::pair<XtankWeapon, XtankMountLocation>;
   WMPair mine[WEAPON_SLOTS], theirs[WEAPON_SLOTS];
   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      mine[i] = { weapons[i],       weaponMounts[i] };
      theirs[i] = { o->weapons[i], o->weaponMounts[i] };
   }
   std::sort(mine, mine + WEAPON_SLOTS);
   std::sort(theirs, theirs + WEAPON_SLOTS);
   for(S32 i = 0; i < WEAPON_SLOTS; i++)
      if(mine[i] != theirs[i])
         return false;

   return true;
}


// Name of currently selected armor
const char *VehicleDesign::getArmorName() const
{
   return xtankArmorInfos[(S32)armor].name;
}


// Size of currently selected body
S32 VehicleDesign::getBodySize() const
{
   return xTankBodyStats[(S32)body].size;
}


bool VehicleDesign::isXtankVehicle() const
{
   return body != XtankBody::BITFIGHTER_SHIP && body != XtankBody::NONE;
}


void VehicleDesign::reduceArmor(VehicleSides side, S32 amount)
{
   S32 sideIndex = (S32)side;
   if(armorSides[sideIndex] - amount >= 0)
      armorSides[sideIndex] -= amount;
   else
      armorSides[sideIndex] = 0;
}


void VehicleDesign::increaseArmor(VehicleSides side, S32 amount)
{
   S32 sideIndex = (S32)side;
   if(armorSides[sideIndex] + amount <= MAX_ARMOR_PER_SIDE)
      armorSides[sideIndex] += amount;
   else
      armorSides[sideIndex] = MAX_ARMOR_PER_SIDE;
}


void VehicleDesign::nextArmor()
{
   S32 nextArmor = (S32)armor + 1;
   if(nextArmor == (S32)XtankArmorCount)
      nextArmor = 0;

   armor = (XtankArmor)nextArmor;
}


void VehicleDesign::previousArmor()
{
   S32 prevArmor = (S32)armor - 1;
   if(prevArmor < 0)
      prevArmor = (S32)XtankArmorCount - 1;

   armor = (XtankArmor)prevArmor;
}


void VehicleDesign::nextMount(S32 slot)
{
   XtankWeapon w = weapons[slot];

   XtankMountLocation mount = weaponMounts[slot];
   do {
      mount = nextEnum(mount);
   } while(!isMountCompatible(body, w, mount));

   weaponMounts[(S32)slot] = mount;
   preferredMounts[(S32)slot] = mount;
}


void VehicleDesign::previousMount(S32 slot)
{
   XtankWeapon w = weapons[slot];

   XtankMountLocation mount = weaponMounts[slot];
   do {
      mount = prevEnum(mount);
   } while(!isMountCompatible(body, w, mount));

   weaponMounts[(S32)slot] = mount;
   preferredMounts[(S32)slot] = mount;
}


// This converts a mount location into a flag, which we can then use to compare with the bitmap
// of legal mountpoints for a particular weapon.  Not all weapons can go in all locations.
static S32 getXtankMountBit(XtankMountLocation mount)
{
   switch(mount)
   {
      case XtankMountLocation::TURRET1:
      case XtankMountLocation::TURRET2:
      case XtankMountLocation::TURRET3:
      case XtankMountLocation::TURRET4: return M_TURRET;
      case XtankMountLocation::FRONT:   return M_FRONT;
      case XtankMountLocation::BACK:    return M_BACK;
      case XtankMountLocation::LEFT:    return M_LEFT;
      case XtankMountLocation::RIGHT:   return M_RIGHT;
      default:            return 0;
   }
}


// Does this body support the specified mountpoint?
static bool bodySupportsMount(XtankBody body, XtankMountLocation mount)
{
   // Unknown body
   if(body == XtankBody::BITFIGHTER_SHIP || body == XtankBody::NONE)
      return false;

   // Turret mounted weapon -- legal if vehicle has enough turrets
   if(mount >= XtankMountLocation::TURRET1 && mount <= XtankMountLocation::TURRET4)
   {
      S32 turretRequested = (S32)mount - (S32)XtankMountLocation::TURRET1;
      return turretRequested < xtankTurretInfos[(S32)body].count;
   }

   // Otherwise it's ok
   return true;
}


// Is the weapon legally allowed at this mountpoint?
static bool weaponAllowedAtMount(XtankWeapon weapon, XtankMountLocation mount)
{
   return (xtankWeaponInfos[(S32)weapon].legalMounts & getXtankMountBit(mount)) != 0;
}


bool VehicleDesign::isMountCompatible(XtankBody body, XtankWeapon weapon, XtankMountLocation mount)
{
   // Does this body have the specified mountpoint?
   if(!bodySupportsMount(body, mount))
      return false;

   // Check if the weapon can be legally mounted to the requested location
   return weaponAllowedAtMount(weapon, mount);
}


XtankMountLocation VehicleDesign::firstValidMount(XtankBody body, XtankWeapon weapon, XtankMountLocation preferred)
{
   if(isMountCompatible(body, weapon, preferred))
      return preferred;

   for(S32 m = 0; m < (S32)XtankMountLocation::COUNT; m++)
   {
      XtankMountLocation mount = (XtankMountLocation)m;
      if(isMountCompatible(body, weapon, mount))
         return mount;
   }

   return XtankMountLocation::NONE;
}


void VehicleDesign::writeToStream(BitStream *stream)
{
   stream->writeEnum(body, XtankBody::COUNT);
   stream->writeEnum(engine, XtankEngine::COUNT);
   stream->writeEnum(tread, XtankTread::COUNT);
   stream->writeEnum(armor, XtankArmor::COUNT);
   stream->writeEnum(suspension, XtankSuspension::COUNT);
   stream->writeEnum(bumper, XtankBumper::COUNT);

   stream->writeRangedU32(heatSinks, 0, MAX_HEAT_SINKS);
   stream->writeRangedU32(specials, 0, MAX_SPECIALS);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      stream->writeEnum(weapons[i], XtankWeapon::COUNT);
      stream->writeEnum(weaponMounts[i], XtankMountLocation::COUNT);
   }
   // Xtank-style default armor: all sides start at 0 and are set explicitly.
   for(S32 i = 0; i < VehicleSidesCount; i++)
      stream->writeRangedU32(armorSides[i], 0, MAX_ARMOR_PER_SIDE);
}


void VehicleDesign::readFromStream(BitStream *stream)
{
   body = stream->readEnum(XtankBody::COUNT);
   engine = stream->readEnum(XtankEngine::COUNT);
   tread = stream->readEnum(XtankTread::COUNT);
   armor = stream->readEnum(XtankArmor::COUNT);
   suspension = stream->readEnum(XtankSuspension::COUNT);
   bumper = stream->readEnum(XtankBumper::COUNT);

   heatSinks = stream->readRangedU32(0, MAX_HEAT_SINKS);
   specials = stream->readRangedU32(0, MAX_SPECIALS);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      weapons[i] = stream->readEnum(XtankWeapon::COUNT);
      weaponMounts[i] = stream->readEnum(XtankMountLocation::COUNT);
   }
   // Xtank-style default armor: all sides start at 0 and are set explicitly.
   for(S32 i = 0; i < VehicleSidesCount; i++)
      armorSides[i] = stream->readRangedU32(0, MAX_ARMOR_PER_SIDE);
}


// Slot is ignored except when assigning weapons
void VehicleDesign::selected(Phase phase, S32 index, S32 slot)
{
   switch(phase)
   {
      case Phase::BODY:
         body = (XtankBody)index;
         break;
      case Phase::ENGINE:
         engine = (XtankEngine)index;
         break;
      case Phase::TREADS:
         tread = (XtankTread)index;
         break;
      case Phase::ARMOR:
         armor = (XtankArmor)index;
         break;
      case Phase::ARMOR_SIDES:
         // Do nothing here
         break;
      case Phase::SUSPENSION:
         suspension = (XtankSuspension)index;
         break;
      case Phase::BUMPERS:
         bumper = (XtankBumper)index;
         break;
      case Phase::SPECIALS:
         //specials = index;
         break;
      case Phase::HEATSINK:
         heatSinks = index;
         break;
      case Phase::WEAPONS:
         if(index == 0)
            weapons[slot] = XtankWeapon::NONE;
         else
         {
            weapons[slot] = (XtankWeapon)(index - 1);    // Compensate for None at top of list

            // Need a mountpoint?
            if(weaponMounts[slot] == XtankMountLocation::NONE)
               weaponMounts[slot] = firstValidMount(body, weapons[slot], preferredMounts[slot]);
         }

         break;
      default:
         TNLAssert(false, "Unhandled vehicle designer phase!");
   }
}


void VehicleDesign::setWeapon(S32 slot, XtankWeapon weapon, XtankMountLocation mountPoint)
{
   weapons[slot] = weapon;
   weaponMounts[slot] = mountPoint;
}


S32 VehicleDesign::slotsInUse() const
{
   S32 slots = 0;
   for(S32 i = 0; i < WEAPON_SLOTS; i++)
      if(weapons[i] != XtankWeapon::NONE)
         slots += 1;

   return slots;
}


// Returns the fire-delay multiplier for a given heat-sink count.
// 1 sink → 1.00x, each additional sink reduces delay by 8%.
// 6 sinks → 0.60x (40% faster cycling).
// Needs to be verified against original design!!

// Heat sink effect in original game:
// Heat sinks reduce your vehicle’s heat, which indirectly lets you shoot more before hitting the heat cap.
//
// In update.c:
//
// every 5 frames, the vehicle’s heat is reduced by:
// v->vdesc->heat_sinks + 1 (default case)
// So each heat sink gives you + 1 heat dissipation per 5 frames, on top of a base 1.
S32 VehicleDesign::heatDissipation() const
{
   return heatSinks * 3;
}


S32 VehicleDesign::getSpecialsSpace() const
{
   S32 total = 0;
   for(S32 i = 0; i < (S32)XtankSpecialCount; i++)
      if(hasSpecial(i))
         total += xtankSpecialInfos[i].space;

   return total;
}


S32 VehicleDesign::getSpecialsWeight() const
{
   S32 total = 0;
   for(S32 i = 0; i < (S32)XtankSpecialCount; i++)
      if(hasSpecial(i))
         total += xtankSpecialInfos[i].weight;

   return total;
}


S32 VehicleDesign::getSpecialsCost() const
{
   S32 total = 0;
   for(S32 i = 0; i < (S32)XtankSpecialCount; i++)
      if(hasSpecial(i))
         total += xtankSpecialInfos[i].cost;

   return total;
}


// Deserialize from a string, perhaps from a settings file.
void VehicleDesign::set(const string &loadoutStr)
{
   TNLAssert(false, "Needs to be implemented!");
}


string VehicleDesign::getValidMountList(XtankWeapon weapon) const
{
   if(weapon == XtankWeapon::NONE)
      return "--";

   string mounts = "";

   for(S32 i = 0; i < (S32)XtankMountLocation::LAST_TURRET; i++)
   {
      XtankMountLocation loc = (XtankMountLocation)i;
      if(isMountCompatible(body, weapon, loc))
      {
         mounts = "Turret";
         break;
      }
   }

   for(S32 i = (S32)XtankMountLocation::LAST_TURRET + 1; i < XtankMountLocationCount; i++)
   {
      XtankMountLocation loc = (XtankMountLocation)i;
      if(isMountCompatible(body, weapon, loc))
      {
         mounts += mounts.empty() ? "" : "; ";
         mounts += getMountLabel(loc);
      }
   }

   return mounts;
}


// Minimal validity test; basically seeing if it has been set or not.
bool VehicleDesign::isValid() const
{
   return true;
}


// All designs currently valid
bool VehicleDesign::isValidForLevel(bool engineerAllowed) const
{
   return isValid();
}


void VehicleDesign::saveDesignStats(Statistics &statistics) const
{
   // TODO: Implement something here
}




} /* namespace Zap */