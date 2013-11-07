//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "PointObject.h"

namespace Zap
{


// Constructor
PointObject::PointObject(F32 radius)
{
   mRadius = radius;
   setNewGeometry(geomPoint, radius);
}


// Destructor
PointObject::~PointObject()
{
   // Do nothing
}


void PointObject::prepareForDock(ClientGame *game, const Point &point, S32 teamIndex)
{
#ifndef ZAP_DEDICATED
   setPos(point);
   Parent::prepareForDock(game, point, teamIndex);
#endif
}


F32 PointObject::getRadius() { return mRadius; }


};
