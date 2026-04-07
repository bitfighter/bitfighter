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
//   - ship.h / ship.cpp  (mXtankBodyIndex field + cycleXtankBody())
//   - UIGame.cpp         (Ctrl+Alt+Shift+X hotkey handler)
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

} /* namespace Zap */
#endif /* _XTANK_SHAPE_H_ */
