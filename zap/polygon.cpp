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

namespace Zap
{

extern S32 gMaxPolygonPoints;

void Polyline::packUpdate(GhostConnection *connection, BitStream *stream)
{
   // - 1 because writeEnum ranges from 0 to n-1; mPolyBounds.size() ranges from 1 to n
   stream->writeEnum(mPolyBounds.size() - 1, gMaxPolygonPoints);  
   for(S32 i = 0; i < mPolyBounds.size(); i++)
   {
      stream->write(mPolyBounds[i].x);
      stream->write(mPolyBounds[i].y);
   }
}


// Static version
void Polyline::readPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize, bool allowFirstAndLastPointToBeEqual, Vector<Point> &bounds)
{
   Point p, lastP;
   
   for(S32 i = firstCoord; i < argc; i += 2)
   {
      // Put a cap on the number of vertices in a polygon
      if(bounds.size() >= gMaxPolygonPoints)      // || argc == i + 1 might be needed...
         break;

      p.set( (F32) atof(argv[i]) * gridSize, (F32) atof(argv[i+1]) * gridSize );

   if(i == firstCoord || p != lastP)
         bounds.push_back(p);

      lastP.set(p);
   }

   // Check if last point was same as first; if so, scrap it
   if(!allowFirstAndLastPointToBeEqual && bounds.first() == bounds.last())
      bounds.erase(bounds.size() - 1);
}


// Read a series of points from a command line, and add them to a Vector of points
void Polyline::processPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize, bool allowFirstAndLastPointToBeEqual)
{
   readPolyBounds(argc, argv, firstCoord, gridSize, allowFirstAndLastPointToBeEqual, mPolyBounds);
}


void Polyline::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   U32 size = stream->readEnum(gMaxPolygonPoints) + 1;

   for(U32 i = 0; i < size; i++)
   {
      Point p;
      stream->read(&p.x);
      stream->read(&p.y);
      mPolyBounds.push_back(p);
   }
}


Rect Polyline::computePolyExtents()
{
   Rect extent(mPolyBounds[0], mPolyBounds[0]);

   for(S32 i = 1; i < mPolyBounds.size(); i++)
      extent.unionPoint(mPolyBounds[i]);

   return extent;
}


string Polyline::boundsToString(F32 gridSize)
{
   string bounds = "";
   S32 size = mPolyBounds.size();

   Point p;
   for(S32 i = 0; i < size; i++)
   {
      p = mPolyBounds[i] / gridSize;
      bounds += p.toString() + (i < size - 1 ? " " : "");
   }

   return bounds;
}


/////
// Point management methods

S32 EditorPolyline::getVertCount() 
{ 
   return mPolyBounds.size(); 
}


void EditorPolyline::clearVerts() 
{ 
   mPolyBounds.clear(); 
   mVertSelected.clear(); 
   onPointsChanged();
}


void EditorPolyline::addVert(const Point &point) 
{ 
   mPolyBounds.push_back(point); 
   mVertSelected.push_back(false); 
   onPointsChanged();
}


void EditorPolyline::addVertFront(Point vert) 
{ 
   mPolyBounds.push_front(vert); 
   mVertSelected.insert(mVertSelected.begin(), false); 
   onPointsChanged();
}


void EditorPolyline::deleteVert(S32 vertIndex) 
{ 
   mPolyBounds.erase(vertIndex); 
   mVertSelected.erase(mVertSelected.begin() + vertIndex); 
   onPointsChanged();
}


void EditorPolyline::insertVert(Point vertex, S32 vertIndex) 
{ 
   mPolyBounds.insert(vertIndex); 
   mPolyBounds[vertIndex] = vertex; 
                                                  
   mVertSelected.insert(mVertSelected.begin() + vertIndex, 1, false); 
   onPointsChanged();
}


Point EditorPolyline::getVert(S32 index) 
{ 
   return mPolyBounds[index]; 
}


void EditorPolyline::setVert(const Point &point, S32 index) 
{ 
   mPolyBounds[index] = point; 
   onPointsChanged();
}


void EditorPolyline::onPointsChanged()
{
   // Do nothing
}

////////////////////////////////////////
////////////////////////////////////////

void EditorPolygon::processPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize)
{
   Parent::processPolyBounds(argc, argv, firstCoord, gridSize, false);
   onPointsChanged();
}


// If polygon changes shape (really only happens in the editor), we need to update some things
void EditorPolygon::onPointsChanged() 
{ 
   mCentroid = findCentroid(mPolyBounds); 
   Triangulate::Process(mPolyBounds, mPolyFill);      // Fills mPolyFill from data in mPolyBounds
   mLabelAngle = angleOfLongestSide(mPolyBounds);
}

void EditorPolygon::packUpdate(GhostConnection *connection, BitStream *stream)
{
   Polyline::packUpdate(connection, stream);
}

void EditorPolygon::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Polyline::unpackUpdate(connection, stream);
   onPointsChanged();
}

////////////////////////////////////////
////////////////////////////////////////

// TODO: Put in editor ??
static const Color INSTRUCTION_TEXTCOLOR(1,1,1);
static const S32 INSTRUCTION_TEXTSIZE = 9;      
static const S32 INSTRUCTION_TEXTGAP = 3;
static const Color ACTIVE_SPECIAL_ATTRIBUTE_COLOR = Color(.6, .6, .6);    
static const Color INACTIVE_SPECIAL_ATTRIBUTE_COLOR = Color(.6, .6, .6);      // already in editor, called inactiveSpecialAttributeColor

// Offset: negative below the item, positive above
void EditorPolygon::renderItemText(const char *text, S32 offset, F32 currentScale)
{
   glColor(INSTRUCTION_TEXTCOLOR);
   S32 off = (INSTRUCTION_TEXTSIZE + INSTRUCTION_TEXTGAP) * offset - 10 - ((offset > 0) ? 5 : 0);
   Point pos = convertLevelToCanvasCoord(getVert(0));
   UserInterface::drawCenteredString(pos.x, pos.y - off, INSTRUCTION_TEXTSIZE, text);
}


void EditorPolygon::renderDock()
{
   renderEditor(1);
}


void EditorPolygon::highlightDockItem() 
{   
   renderPolyHighlight(); 
}  


// TODO: merge with versions in editor
const Color HIGHLIGHT_COLOR = white;
const Color SELECT_COLOR = yellow;


void EditorPolygon::renderPolyHighlight()
{
   glLineWidth(gLineWidth3);
   glColor(mSelected ? SELECT_COLOR : HIGHLIGHT_COLOR);
   renderPolygonOutline(mPolyBounds);
   glLineWidth(gDefaultLineWidth);
}


void EditorPolygon::labelDockItem()
{
   renderDockItemLabel(mCentroid, getOnDockName(), -2);
}


void EditorPolygon::addToDock(Game *game, const Point &point)
{
   F32 h = 16;    // Entire height
   F32 w = 20;    // Half the width

   clearVerts();
   addVert(point + Point(-w, 0)); 
   addVert(point + Point( w, 0)); 
   addVert(point + Point( w, h)); 
   addVert(point + Point(-w, h)); 

   Parent::addToDock(game, point);
}


static const F32 INITIAL_HEIGHT = 0.9;
static const F32 INITIAL_WIDTH = 0.3;

void EditorPolygon::initializeEditor(F32 gridSize) 
{
   Parent::initializeEditor(gridSize);

   F32 w = INITIAL_HEIGHT * gridSize / 2;
   F32 h = INITIAL_WIDTH * gridSize / 2;

   setVert(Point(-w, -h), 0);
   setVert(Point(-w,  h), 1);
   setVert(Point( w,  h), 2);
   setVert(Point( w, -h), 3);
}


// Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement seem more natural.
Point EditorPolygon::getInitialPlacementOffset(F32 gridSize)
{ 
   return Point(INITIAL_HEIGHT * gridSize / 2, INITIAL_WIDTH * gridSize / 2); 
}


};

