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
// See XtankShape.h for the XtankBody enum that indexes this array.
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

} /* namespace Zap */
