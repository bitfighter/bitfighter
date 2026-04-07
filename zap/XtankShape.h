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
//   - ship.h / ship.cpp  (mXtankBodyIndex field + cycleXtankBody() + tank physics)
//   - UIGame.cpp         (Ctrl+Alt+Shift+X hotkey handler)
//   - move.h / move.cpp  (Move::bodyIndex field)
//------------------------------------------------------------------------------

#ifndef _XTANK_SHAPE_H_
#define _XTANK_SHAPE_H_

#include "ShipShape.h"   // for ShipShapeInfo

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
};

// One entry per XtankBody::Type value.
extern TankPhysicsInfo xtankPhysicsInfos[XtankBody::Count];

} /* namespace Zap */
#endif /* _XTANK_SHAPE_H_ */
