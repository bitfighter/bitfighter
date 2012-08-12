//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "polygon.h"
#include "GeomUtils.h"           // For polygon triangulation
#include "gameObjectRender.h"
#include "Colors.h"

#ifndef ZAP_DEDICATED
#include "OpenglUtils.h"
#include "UI.h"
#endif

namespace Zap
{

// TODO: Put in editor ??
static const Color INSTRUCTION_TEXTCOLOR = Colors::white;
static const S32 INSTRUCTION_TEXTSIZE = 9;      
static const S32 INSTRUCTION_TEXTGAP = 3;

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


void PolygonObject::onItemDragging()
{
   onGeomChanged();
}


// Tell the geometry that things have changed
void PolygonObject::onGeomChanged() 
{ 
   onPointsChanged(); 
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


void PolygonObject::renderPolyHighlight()
{
#ifndef ZAP_DEDICATED
   glLineWidth(gLineWidth3);
   glColor(isSelected() ? *SELECT_COLOR : *HIGHLIGHT_COLOR);
   renderPolygonOutline(getOutline());
   glLineWidth(gDefaultLineWidth);
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
Point PolygonObject::getInitialPlacementOffset(F32 gridSize)
{ 
   return Point(INITIAL_HEIGHT * gridSize / 2, INITIAL_WIDTH * gridSize / 2); 
}


/////
// Former LuaPolygon methods
S32 PolygonObject::getLoc(lua_State *L)
{
   return LuaObject::returnPoint(L, getCentroid());      // Do we want this to return a series of points?
}


S32 PolygonObject::setLoc(lua_State *L)
{
   checkArgList(L, functionArgs, "BfObject", "setLoc");

   Point newPos = getPointOrXY(L, 1);
   offset(newPos - getCentroid());

   return 0;
}


S32 PolygonObject::getRad(lua_State *L)
{
   return LuaObject::returnInt(L, 0);
}


S32 PolygonObject::getVel(lua_State *L)
{
   return LuaObject::returnPoint(L, Point(0,0));
}


};

