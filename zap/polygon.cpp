//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "polygon.h"

#include "Colors.h"
#include "gameObjectRender.h"

namespace Zap
{


// Constructor
PolygonObject::PolygonObject()
{  
   setNewGeometry(geomPolygon);
}


// Destructor
PolygonObject::~PolygonObject()
{
   // Do nothing
}


// Tell the geometry that things have changed
void PolygonObject::onGeomChanged() 
{ 
   Parent::onGeomChanged();
}  


void PolygonObject::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
#ifndef ZAP_DEDICATED
   if(isSelected() || isLitUp())
      renderPolyHighlight();

   renderPolyLineVertices(this, snappingToWallCornersEnabled, currentScale);
#endif
}


void PolygonObject::renderDock()
{
   renderEditor(1, false);
}


void PolygonObject::highlightDockItem() 
{   
   renderPolyHighlight(); 
}


extern F32 gLineWidth3;

void PolygonObject::renderPolyHighlight()
{
#ifndef ZAP_DEDICATED
   renderPolygonOutline(getOutline(), isSelected() ? &Colors::EDITOR_SELECT_COLOR : &Colors::EDITOR_HIGHLIGHT_COLOR, 1, gLineWidth3);
#endif
}


Point PolygonObject::getDockLabelPos()
{
   static const Point labelOffset(0, -2);

   return getCentroid() + labelOffset;
}


void PolygonObject::prepareForDock(ClientGame *game, const Point &point, S32 teamIndex)
{
#ifndef ZAP_DEDICATED
   F32 h = 16;    // Entire height
   F32 w = 20;    // Half the width

   clearVerts();
   addVert(point + Point(-w, 0)); 
   addVert(point + Point( w, 0)); 
   addVert(point + Point( w, h)); 
   addVert(point + Point(-w, h)); 

   Parent::prepareForDock(game, point, teamIndex);
#endif
}


static const F32 INITIAL_HEIGHT = 0.9f;
static const F32 INITIAL_WIDTH = 0.3f;

// Called when we create a brand new object and insert it in the editor, like when we drag a new item from the dock
void PolygonObject::newObjectFromDock(F32 gridSize)
{
#ifndef ZAP_DEDICATED
   F32 w = INITIAL_HEIGHT * gridSize / 2;
   F32 h = INITIAL_WIDTH * gridSize / 2;

   setVert(Point(-w, -h), 0);
   setVert(Point(-w,  h), 1);
   setVert(Point( w,  h), 2);
   setVert(Point( w, -h), 3);

   Parent::newObjectFromDock(gridSize);
#endif
}


// Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement seem more natural.
Point PolygonObject::getInitialPlacementOffset(U32 gridSize) const
{ 
   return Point(INITIAL_HEIGHT * gridSize / 2, INITIAL_WIDTH * gridSize / 2); 
}


S32 PolygonObject::getRad(lua_State *L)
{
   return returnInt(L, 0);
}


S32 PolygonObject::getVel(lua_State *L)
{
   return returnPoint(L, Point(0,0));
}


};

