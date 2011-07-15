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

#include "Geometry.h"
#include "GeomUtils.h"              // For polygon triangulation

#include "Rect.h"
#include <math.h>

//#include "boost/smart_ptr/shared_ptr.hpp"


using namespace TNL;

namespace Zap
{

void Geometry::rotateAboutPoint(const Point &center, F32 angle)
{
   F32 sinTheta = sin(angle * Float2Pi / 360.0f);
   F32 cosTheta = cos(angle * Float2Pi / 360.0f);

   for(S32 j = 0; j < getVertCount(); j++)
   {
      Point v = getVert(j) - center;
      Point n(v.x * cosTheta + v.y * sinTheta, v.y * cosTheta - v.x * sinTheta);

      setVert(n + center, j);
   }
}


// Make object bigger or smaller
void Geometry::scale(const Point &center, F32 scale) 
{
   for(S32 j = 0; j < getVertCount(); j++)
      setVert((getVert(j) - center) * scale + center, j);
}


////////////////////////////////////////
////////////////////////////////////////

void PointGeometry::packGeom(GhostConnection *connection, BitStream *stream)
{
   mPos.write(stream);
}


void PointGeometry::unpackGeom(GhostConnection *connection, BitStream *stream)
{
   mPos.read(stream);
}


Rect PointGeometry::getExtents()
{
   return Rect(mPos, 1);
}


string PointGeometry::geomToString(F32 gridSize) const
{
   Point pos = mPos; 
   return (pos / gridSize).toString();
}


void PointGeometry::readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize)
{
   TNLAssert(false, "Haven't figured this one out yet!");
}


boost::shared_ptr<Geometry> PointGeometry::copyGeometry() const
{
    return boost::shared_ptr<Geometry>(new PointGeometry(*this));
}


void PointGeometry::flipHorizontal(F32 boundingBoxMinX, F32 boundingBoxMaxX)
{
   mPos.x = boundingBoxMinX + (boundingBoxMaxX - mPos.x);
}


void PointGeometry::flipVertical(F32 boundingBoxMinY, F32 boundingBoxMaxY)
{
   mPos.y = boundingBoxMinY + (boundingBoxMaxY - mPos.y);
}


////////////////////////////////////////
////////////////////////////////////////

static Vector<Point> outlinePoints(2);    // Reusable container

Vector<Point> *SimpleLineGeometry::getOutline()
{
   outlinePoints.resize(2);
   outlinePoints[0] = mFromPos;
   outlinePoints[1] = mToPos;

   return &outlinePoints;
}


void SimpleLineGeometry::packGeom(GhostConnection *connection, BitStream *stream)
{
   mFromPos.write(stream);
   mToPos.write(stream);
}


void SimpleLineGeometry::unpackGeom(GhostConnection *connection, BitStream *stream)
{
   mFromPos.read(stream);
   mToPos.read(stream);
}


Rect SimpleLineGeometry::getExtents()
{
   return Rect(mFromPos, mToPos);
}


string SimpleLineGeometry::geomToString(F32 gridSize) const
{
   Point fromPos = mFromPos;
   Point toPos = mToPos;

   return (fromPos / gridSize).toString() + " " + (toPos / gridSize).toString();
}


void SimpleLineGeometry::readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize)
{
   TNLAssert(false, "Haven't figured this one out yet!");
}


boost::shared_ptr<Geometry> SimpleLineGeometry::copyGeometry() const
{
    return boost::shared_ptr<Geometry>(new SimpleLineGeometry(*this));
}


void SimpleLineGeometry::flipHorizontal(F32 boundingBoxMinX, F32 boundingBoxMaxX)
{
   mFromPos.x = boundingBoxMinX + (boundingBoxMaxX - mFromPos.x);
   mToPos.x = boundingBoxMinX + (boundingBoxMaxX - mToPos.x);
}


void SimpleLineGeometry::flipVertical(F32 boundingBoxMinY, F32 boundingBoxMaxY)
{
   mFromPos.y = boundingBoxMinY + (boundingBoxMaxY - mFromPos.y);
   mToPos.y = boundingBoxMinY + (boundingBoxMaxY - mToPos.y);
}


////////////////////////////////////////
////////////////////////////////////////

extern S32 gMaxPolygonPoints;

Point PolylineGeometry::getVert(S32 index) 
{ 
   return mPolyBounds[index]; 
}


void PolylineGeometry::setVert(const Point &point, S32 index) 
{ 
   mPolyBounds[index] = point; 
}


S32 PolylineGeometry::getVertCount() 
{ 
   return mPolyBounds.size(); 
}


void PolylineGeometry::clearVerts() 
{ 
   mPolyBounds.clear(); 
   mVertSelected.clear(); 
   mAnyVertsSelected = false;
   onPointsChanged();
}


bool PolylineGeometry::addVert(const Point &point) 
{ 
   if(mPolyBounds.size() >= gMaxPolygonPoints)
      return false;

   mPolyBounds.push_back(point); 
   mVertSelected.push_back(false); 
   onPointsChanged();

   return true;
}


bool PolylineGeometry::addVertFront(Point vert) 
{ 
   if(mPolyBounds.size() >= gMaxPolygonPoints)
      return false;

   mPolyBounds.push_front(vert); 
   mVertSelected.insert(mVertSelected.begin(), false); 
   onPointsChanged();

   return true;
}


bool PolylineGeometry::deleteVert(S32 vertIndex) 
{ 
   TNLAssert(vertIndex < S32(mVertSelected.size()), "Index out of bounds!");

   if(vertIndex >= mPolyBounds.size())
      return false;

   mPolyBounds.erase(vertIndex); 
   mVertSelected.erase(mVertSelected.begin() + vertIndex); 
   checkIfAnyVertsSelected();
   onPointsChanged();

   return true;
}


bool PolylineGeometry::insertVert(Point vertex, S32 vertIndex) 
{ 
   if(mPolyBounds.size() >= gMaxPolygonPoints)
      return false;

   mPolyBounds.insert(vertIndex); 
   mPolyBounds[vertIndex] = vertex; 
                                                  
   mVertSelected.insert(mVertSelected.begin() + vertIndex, 1, false); 
   onPointsChanged();

   return true;
}


bool PolylineGeometry::anyVertsSelected()
{
   return mAnyVertsSelected;
}


void PolylineGeometry::selectVert(S32 vertIndex)
{
   unselectVerts();
   aselectVert(vertIndex);
}


void PolylineGeometry::aselectVert(S32 vertIndex)
{
   TNLAssert(vertIndex < S32(mVertSelected.size()), "Index out of bounds!");
   mVertSelected[vertIndex] = true;
   mAnyVertsSelected = true;
}


void PolylineGeometry::unselectVert(S32 vertIndex)
{
   TNLAssert(vertIndex < S32(mVertSelected.size()), "Index out of bounds!");
   mVertSelected[vertIndex] = false;
   checkIfAnyVertsSelected();
}


void PolylineGeometry::unselectVerts()
{
   for(U32 i = 0; i < mVertSelected.size(); i++)
      mVertSelected[i] = false;

   mAnyVertsSelected = false;
}


bool PolylineGeometry::vertSelected(S32 vertIndex)
{
   TNLAssert(vertIndex < S32(mVertSelected.size()), "Index out of bounds!");
   return mVertSelected[vertIndex];
}


void PolylineGeometry::onPointsChanged()
{
   // Do nothing
}


void PolylineGeometry::checkIfAnyVertsSelected()
{
   mAnyVertsSelected = false;

   for(U32 i = 0; i < mVertSelected.size(); i++)
      if(mVertSelected[i])
      {
         mAnyVertsSelected = true;
         break;
      }
}


void PolylineGeometry::packGeom(GhostConnection *connection, BitStream *stream)
{
   // - 1 because writeEnum ranges from 0 to n-1; mPolyBounds.size() ranges from 1 to n
   stream->writeEnum(mPolyBounds.size() - 1, gMaxPolygonPoints);  
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      mPolyBounds[i].write(stream);
}


void PolylineGeometry::unpackGeom(GhostConnection *connection, BitStream *stream)
{
   U32 size = stream->readEnum(gMaxPolygonPoints) + 1;

   mPolyBounds.resize(size);
   
   for(U32 i = 0; i < size; i++)
      mPolyBounds[i].read(stream);

   onPointsChanged();
}


Rect PolylineGeometry::getExtents()
{
   return Rect(mPolyBounds);
}


string PolylineGeometry::geomToString(F32 gridSize) const
{
   string bounds = "";
   S32 size = mPolyBounds.size();

   Point p;
   for(S32 i = 0; i < size; i++)
   {
      p = mPolyBounds[i];
      p /= gridSize;
      bounds += p.toString() + (i < size - 1 ? " " : "");
   }

   return bounds;
}


// Fills bounds with points from argv starting at firstCoord
static void readPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize, 
                           bool allowFirstAndLastPointToBeEqual, Vector<Point> &bounds)
{
   Point p, lastP;
   
   bounds.clear();

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


void PolylineGeometry::readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize)
{
    readPolyBounds(argc, argv, firstCoord, gridSize, true, mPolyBounds);
    mVertSelected.resize(mPolyBounds.size());
    onPointsChanged();
}


boost::shared_ptr<Geometry> PolylineGeometry::copyGeometry() const
{
    return boost::shared_ptr<Geometry>(new PolylineGeometry(*this));
}


void PolylineGeometry::flipHorizontal(F32 boundingBoxMinX, F32 boundingBoxMaxX)
{
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      mPolyBounds[i].x = boundingBoxMinX + (boundingBoxMaxX - mPolyBounds[i].x);
}


void PolylineGeometry::flipVertical(F32 boundingBoxMinY, F32 boundingBoxMaxY)
{
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      mPolyBounds[i].y = boundingBoxMinY + (boundingBoxMaxY - mPolyBounds[i].y);
}


////////////////////////////////////////
////////////////////////////////////////

void PolygonGeometry::readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize)
{
   readPolyBounds(argc, argv, firstCoord, gridSize, false, mPolyBounds);
   mVertSelected.resize(mPolyBounds.size());
   onPointsChanged();
}


void PolygonGeometry::onPointsChanged()
{
   if(mTriangluationDisabled)
      return;

   mCentroid = findCentroid(mPolyBounds); 
   Triangulate::Process(mPolyBounds, mPolyFill);        // Resizes and fills mPolyFill from data in mPolyBounds
   mLabelAngle = angleOfLongestSide(mPolyBounds);
}


boost::shared_ptr<Geometry> PolygonGeometry::copyGeometry() const
{
   PolygonGeometry *newGeom = new PolygonGeometry(*this);
   return boost::shared_ptr<Geometry>(newGeom);
}



};