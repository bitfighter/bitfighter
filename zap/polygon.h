//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _POLYGON_H_
#define _POLYGON_H_

#include "BfObject.h"     // For CentroidObject def

namespace Zap
{


// Provide editor related methods to the polygon class
class PolygonObject : public CentroidObject
{
   typedef CentroidObject Parent;

private:
   virtual Point getDockLabelPos();

   void prepareForDock(ClientGame *game, const Point &point, S32 teamIndex);
   virtual void renderDock();
   void highlightDockItem(); 

   // Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.
   // This makes placement seem more natural.
   Point getInitialPlacementOffset(F32 gridSize);

protected:
    void renderPolyHighlight();

public:
   PolygonObject();              // Constructor
   virtual ~PolygonObject();     // Destructor

   void newObjectFromDock(F32 gridSize);

   // Item is being actively dragged
   virtual void onGeomChanged();   // Tell the geometry that things have changed

   virtual void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);

   /////
   // This class serves only to provide an implementation of the abstract methods in LuaItem
   // that are common to the polygon classes
   S32 getRad(lua_State *L);        // Radius of item (returns number)
   S32 getVel(lua_State *L);        // Velocity of item (returns point)
};

};

#endif   // _POLYGON_H_
