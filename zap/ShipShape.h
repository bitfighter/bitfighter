//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SHIP_SHAPE_H_
#define _SHIP_SHAPE_H_

#include "tnlTypes.h"
#include "Color.h"

using namespace TNL;

namespace Zap
{


struct ShipFlameLayer
{
   Color color;
   F32 points[6];             // 3 points for a triangle
   F32 multiplier;            // Thrust multiplier
};


struct ShipFlame
{
   S32 layerCount;
   ShipFlameLayer layers[3];  // 2 or 3 triangles
};


struct ShipInnerHull
{
   S32 pointCount;
   F32 points[32];
};


struct ShipShapeInfo
{
   S32 outerHullPointCount;
   F32 outerHullPoints[64];

   S32 innerHullPieceCount;
   ShipInnerHull innerHullPieces[4];

   S32 cornerCount;
   F32 cornerPoints[16];

   S32 flamePortPointCount;
   F32 flamePortPoints[16];

   // Flames
   S32 forwardFlameCount;
   ShipFlame forwardFlames[2];

   S32 reverseFlameCount;
   ShipFlame reverseFlames[2];

   S32 portFlameCount;
   ShipFlame portFlames[2];

   S32 starboardFlameCount;
   ShipFlame starboardFlames[2];
};


class ShipShape
{
public:

   enum ShipShapeType
   {
      Normal,          // Our beloved triangle
      BirdOfPrey,      // tlhIngan toQDuj!
      Cube,            // You will be assimilated
      ShapeTypeCount
   };

   static ShipShapeInfo shipShapeInfos[ShapeTypeCount];
};




} /* namespace Zap */
#endif /* SHIPSHAPE_H_ */
