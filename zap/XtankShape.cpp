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

const char *xtankBodyNames[XtankBodyCount] =
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
// Array layout mirrors the order of XtankBodyType enum values.
//
// All shapes use the same coordinate scale to preserve relative sizes.
// Raw extents (max coord from origin):
//   Trike 11  Lightcycle 15  Hexo/Spider/Malice 17  Tornado 18
//   Tiger 19  Psycho/Marauder 21  Delta ~21  Disk ~17  Medusa/Rhino 27  Panzy 31
// ---------------------------------------------------------------------------
ShipShapeInfo xtankBodyInfos[XtankBodyCount] =
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


// ---------------------------------------------------------------------------
// Per-body tank driving physics parameters.
//
// Ordered to match XtankBodyType enum values.
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
TankPhysicsInfo xtankPhysicsInfos[] =
{
   // maxSpd  maxRevSpd  accel   friction  turnRate  armor
   {  600,    340,      3500,    180,      2.6f,    1.10f },      // Lightcycle
   {  560,    320,      3200,    200,      2.3f,    1.05f },      // Trike
   {  520,    290,      3000,    230,      2.0f,    0.95f },      // Hexo
   {  490,    275,      2800,    240,      2.2f,    0.90f },      // Spider
   {  545,    310,      3250,    210,      2.4f,    1.00f },      // Psycho
   {  510,    285,      2950,    235,      2.1f,    0.90f },      // Tornado
   {  475,    265,      2650,    260,      1.9f,    0.85f },      // Marauder
   {  450,    250,      2500,    280,      1.6f,    0.80f },      // Tiger
   {  400,    220,      2100,    310,      1.2f,    0.60f },      // Rhino
   {  420,    235,      2250,    295,      1.5f,    0.70f },      // Medusa
   {  490,    275,      2750,    250,      1.9f,    0.85f },      // Delta
   {  515,    290,      3000,    190,      3.0f,    0.95f },      // Disk
   {  430,    240,      2300,    290,      1.4f,    0.75f },      // Malice
   {  370,    205,      1900,    340,      1.0f,    0.50f },      // Panzy
};

XtankBodyInfo body_stat[] =
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

// ---------------------------------------------------------------------------
// Xtank tread types: multipliers applied on top of the per-body base physics.
// ---------------------------------------------------------------------------

const char *xtankTreadNames[] =
    {
        "Rubber Trd",
        "Metal Trd",
        "Heavy Trd",
};

XtankTreadInfo xtankTreadInfos[] =
{
   //    name           turnMult  frictionMult  cost
   { "Rubber Tread",   1.15f,    0.85f, 0 },
   { "Metal Tread",    1.00f,    1.00f, 0 },
   { "Heavy Tread",    0.85f,    1.20f, 0 },
};


XtankTreadInfo xtankTreadInfos2[] =
{
//    name           friction  friction  cost
   { "Smooth",       0.70f, 0.70f, 100  },
   { "Normal",       0.80f, 0.80f, 200  },
   { "Chained",      0.90f, 0.90f, 400  },
   { "Spiked",       1.00f, 1.00f, 1000 },
   { "Hover",        0.20f, 0.20f, 500  },
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

const char *xtankWeaponNames[] =
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

XtankWeaponInfo xtankWeaponInfos[] =
{
   // Xtank weapon parameters are derived from the original xtank weapon-defs.h.
   // Speed conversion: xtank px/frame × 20 fps = BF units/sec.
   // Lifetime conversion: xtank frames × 50 ms/frame = BF ms.
   // Reload conversion: xtank reload_frames × 50 ms/frame = BF ms.
   //
   // Colour mapping from xtank display flags:
   //   F_BL=blue, F_OR=orange, F_YE=yellow, F_GR=green, F_VI=violet, F_BEAM=laser
   //
   //   name       fireDelay energyDrain     bfWeapon      projVelocity  projLiveTime       style
   { "Machine Gun",   150,       300,        WeaponPhaser,  340,          1100,         ProjectileStyleXtankBlue   },  // MG:   spd=17,fr=22,tm=3
   { "Laser",         150,       8000,       WeaponRailgun, 1200,         1000,         ProjectileStyleXtankLaser  },  // LASER:spd=60,fr=20,tm=3
   { "Missile",       750,       8000,       WeaponSeeker,  500,          2550,         ProjectileStyleXtankViolet },  // SEEKER:spd=25,fr=51,tm=15
   { "Grenade",       150,       6000,       WeaponBurst,   440,           950,         ProjectileStyleXtankOrange },  // CANNON:spd=22,fr=19,tm=3
   { "Rocket",        400,       7000,       WeaponBurst,   800,           750,         ProjectileStyleXtankYellow },  // ROCKET:spd=40,fr=15,tm=8
   { "Acid",          150,        800,       WeaponBounce,  200,           850,         ProjectileStyleXtankGreen  },  // ACID:  spd=10,fr=17,tm=3
   { "Tracer",        100,        400,       WeaponTurret,  340,          1100,         ProjectileStyleXtankBlue   },  // LMG:   spd=17,fr=22,tm=2
   { "Bomb",          400,       10000,      WeaponBurst,   500,          1000,         ProjectileStyleXtankYellow },  // HROCKET:spd=40,rng~500,tm=8
   { "Fire",           50,       1500,       WeaponTriple,  240,           850,         ProjectileStyleXtankGreen  },  // FLAME: spd=12,fr=17,tm=1
}; // xtankWeaponInfos[]


XtankWeaponInfo2 xtankWeaponInfos2[] =
{
   /* name                 dam rng ammo tm  spd  wgt   spc  mspc  fr  ht  a$   cost  refl safety hgt */
   /* mount o_flgs  creat_flgs      disp_flgs    move_flgs   hit_flgs cr_f di_f upd_f hit_f */
   { "Light Machine Gun",  1, 360, 300, 2, 17,   20,  200,  200, 22,  0,  1,   1000,   1,   0, NORM,  M_ALL,  1,      NORM,           F_BL,         NORM,       NORM,   }, // LMG
   { "Machine Gun",        2, 360, 250, 3, 17,   50,  225,  225, 22,  1,  2,   2200,   1,   0, NORM,  M_ALL,  1,      NORM,           F_BL,         NORM,       NORM,   }, // MG
   { "Heavy Machine Gun",  3, 360, 200, 3, 17,  100,  250,  250, 22,  2,  3,   3000,   1,   0, NORM,  M_ALL,  2,      NORM,           F_BL,         NORM,       NORM,   }, // HMG
   { "Light Autocannon",   3, 400, 250, 3, 22,  200,  300,  300, 19,  3,  4,   3000,   1,   3, NORM,  M_ALL,  2,      NORM,           F_OR,         NORM,       NORM,   }, // LCANNON
   { "Autocannon",         4, 400, 225, 3, 22,  300,  350,  350, 19,  4,  5,   6000,   1,   3, NORM,  M_ALL,  3,      NORM,           F_OR,         NORM,       NORM,   }, // CANNON
   { "Heavy Autocannon",   5, 400, 200, 3, 22,  500,  400,  400, 19,  5,  6,  10000,   1,   3, NORM,  M_ALL,  4,      NORM,           F_OR,         NORM,       NORM,   }, // HCANNON
   { "Light Rkt Launcher", 6, 600, 150, 8, 40,  600,  800,  800, 15,  4,  8,   7000,   2,   3, NORM,  M_ALL,  5,      NORM,           F_YE,         NORM,       NORM,   }, // LROCKET
   { "Rkt Launcher",       7, 600, 125, 8, 40,  900, 1200, 1200, 15,  6, 10,  10000,   2,   3, NORM,  M_ALL,  6,      NORM,           F_YE,         NORM,       NORM,   }, // ROCKET
   { "Heavy Rkt Launcher", 8, 600, 100, 8, 40,  900, 1600, 1600, 15,  8, 12,  15000,   2,   3, NORM,  M_ALL,  7,      NORM,           F_YE,         NORM,       NORM,   }, // HROCKET
   { "Acid Sprayer",       4, 160, 100, 3, 10,  600,  700,  700, 17,  0, 10,  10000,   1,   0, NORM,  M_ALL,  9,      NORM,           F_GR,         NORM,       NORM,   }, // ACID
   { "Flame Thrower",      3, 200, 300, 1, 12,  700,  500,  500, 17,  1,  2,   4000,   1,   0, NORM,  M_ALL,  9,      NORM,           F_GR,         NORM,       NORM,   }, // FLAME
   { "Heat Seeker",        8, 1250, 15,15, 25, 1000, 1800, 1800, 51,  9, 50,  20000,  12,   3, HIGH,  M_ALL, 10,    F_NREL,        F_TRL | F_VI,      NORM,       NORM,  }, // SEEKER
   { "Pocket Rocket",      5, 1160, 24, 2, 40, 1200, 1900, 1900, 30,  9, 10,  17000,   2,   3, NORM,  M_ALL, 10,      NORM,          F_TRL,         NORM,       NORM,    }, // PROCKET
   { "Unguided Missle",   10, 1960, 30, 8, 35, 1000, 1800, 1800, 57, 12, 25,  18000,   2,   3, NORM,  M_ALL,  0,      NORM,          F_TRL,         NORM,       NORM,    }, // UMISSLE
   { "TeleGuided",        64, 6000, 1,  1, 15, 1500, 2000, 2000,400, 30,500,  30000, 200,   3, HIGH, M_ALL,F_CHO,  F_NREL, F_TELE | F_TRL | F_RE | F_TAC, F_KEYB,    NORM,               }, // TELE
   { "TOW Missile",       64, 7500, 2,  1, 15, 1500, 2000, 2000,500, 30,100,  30000, 100,   3, HIGH, M_ALL,F_CHO,   F_NREL,    F_TRL | F_RE | F_TAC,  F_KEYB,       NORM,              }, // TOW
   { "Land Torpedo",      20, 5000, 2,  1, 10, 1500, 2000, 2000,500, 30,100,  30000, 100,   3, LOW,  M_SIDES,F_CHO,   F_NREL,           F_RE,       F_KEYB,       NORM,            }, // LTORP
   { "Blast Cannon",       1, 500,  20, 8, 25,  300,  350,  350, 20,  6, 20, 100000,   2,   3, NORM,  M_ALL,  0,      NORM,           F_RE,         NORM,       NORM,           }, // BLAST
   { "Pulse Laser",        3,1200, 500, 3, 60,  300,  250,  250, 20,  3,  3,  30000,   1,   0, NORM,  M_ALL,  0,    F_NREL,       F_BEAM | F_NOPT,    NORM,       NORM,              }, // LASER
   { "Mine Layer",         6,  50,  50, 2, 10, 1000, 1000, 1000, 70,  2, 10,   8000,   4,   0, LOW,   M_BACK,  0,      NORM, F_ROT | F_NOHD | F_NOPT,  F_MINE,    F_HOVER,             }, // MINE
   { "Oil Slick",          0,  50,  50, 5, 10,  300,  500,  500, 70,  0, 10,   2000,   1,   0, LOW,   M_BACK,  0,     F_CR3,    F_NOHD | F_NOPT,     F_MINE,     F_SLICK,             }, // SLICK
   { "Heavy Mortar",     170, 7500,  10,20, 50, 1400, 2000, 2000, 70, 30,500,  9000,  30,   0, FLY,  M_TURRET,0, F_MAP | F_NREL, F_NOHD | F_TAC | F_TRL, F_DET,       AREA,   }, // MORTAR
   { "Tactical Nuke",    170,  50,  10,20, 10, 1600, 2200, 2200, 30,  6,500,  80000,  30,   0, NORM, M_BACK, 0,      NORM,         F_NOHD,   F_MINE | F_DET, AREA | F_NOHIT,              }, // NUKE
   { "Anti-Radiation",    64, 2500, 2, 192,25, 2000, 3400, 3400,100, 24,1000,100000, 192,   0, FLY,  M_LR,F_CHO, F_MAP | F_NREL,F_TRL | F_NOHD | F_TAC,   NORM,       NORM,     }, // HARM
   { "Disc Shooter",       0,  BIG, 1, BIG, 0,   0,     0,    0,BIG,  0,   0,     0,    0,  0, 0,  M_ALL,  0,             0,          0,        0,          0,              }, // DISC
};


// ---------------------------------------------------------------------------
// Default weapon loadout for each xtank vehicle body.
//
// The first xtankTurretInfos[bodyIdx].count entries are valid; extras are
// XtankWeaponNone.
// ---------------------------------------------------------------------------

#define MG XtankWeapon::MachineGun
#define NONE XtankWeaponNone

XtankBodyDefaultWeapons xtankDefaultWeapons[] =
{
   { { MG, NONE, NONE, NONE } },                                      // Lightcycle  – 1 turret
   { { XtankWeapon::Tracer,     NONE, NONE, NONE } },                 // Trike       – 1 turret
   { { XtankWeapon::Acid,       NONE, NONE, NONE } },                 // Hexo        – 1 turret
   { { XtankWeapon::Fire,       NONE, NONE, NONE } },                 // Spider      – 1 turret
   { { XtankWeapon::Laser,      NONE, NONE, NONE } },                 // Psycho      – 1 turret
   { { XtankWeapon::Grenade,    NONE, NONE, NONE } },                 // Tornado     – 1 turret
   { { XtankWeapon::Rocket,     NONE, NONE, NONE } },                 // Marauder    – 1 turret
   { { XtankWeapon::MachineGun, MG, NONE, NONE } },                   // Tiger       – 2 turrets
   { { XtankWeapon::Bomb,       XtankWeapon::Bomb, NONE, NONE } },    // Rhino       – 2 turrets
   { { XtankWeapon::Missile,    XtankWeapon::Missile, NONE, NONE } }, // Medusa      – 2 turrets
   { { XtankWeapon::Laser,      XtankWeapon::Laser, NONE, NONE } },   // Delta       – 2 turrets
   { { XtankWeapon::Fire,       NONE, NONE, NONE } },                 // Disk        – 1 turret
   { { XtankWeapon::Rocket,     NONE, NONE, NONE } },                 // Malice      – 1 turret
   { { MG,                      MG, MG, MG } },                       // Panzy       – 4 turrets
};

#undef MG
#undef NONE


// ---------------------------------------------------------------------------
// Heatsink info
// ---------------------------------------------------------------------------
HeatSinkStat heatSinkStat = {500, 1000, 500}; // weight, space, cost

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

    XtankDesign::XtankDesign()
{
   bodyIndex = (S8)XtankBodyNone;
   for(S32 i = 0; i < 4; i++)
      weapons[i] = XtankWeaponNone;
   engineType    = XtankEngineDefault;
   treadType     = XtankTreadDefault;
   heatSinkCount = (S8)XtankHeatSinkDefault;
}


void XtankDesign::initForBody(S32 bodyIdx)
{
   bodyIndex = (S8)bodyIdx;
   if(bodyIdx >= 0 && bodyIdx < XtankBodyCount)
   {
      for(S32 i = 0; i < 4; i++)
         weapons[i] = xtankDefaultWeapons[bodyIdx].weapons[i];
   }
   else
   {
      for(S32 i = 0; i < 4; i++)
         weapons[i] = XtankWeaponNone;
   }
   engineType    = XtankEngineDefault;
   treadType     = XtankTreadDefault;
   heatSinkCount = (S8)XtankHeatSinkDefault;
}

} /* namespace Zap */
