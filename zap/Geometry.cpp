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
#include "Geometry_Base.h"
#include "GeomUtils.h"              // For polygon triangulation

#include "Rect.h"
#include <math.h>

// #include "tnlGhostConnection.h"  // appears to not be needed in here, as we are only handling pointer of GhostConnection
#include "tnlBitStream.h"
#include "boost/smart_ptr/shared_ptr.hpp"

using namespace TNL;

namespace Zap
{

// Constructor
Geometry::Geometry()
{
   // Do nothing
}


// Destructor
Geometry::~Geometry()
{
   // Do nothing
}


GeomType Geometry::getGeomType()
{
   TNLAssert(false, "Not implemented");
   return geomNone;
}


Point Geometry::getVert(S32 index) const
{
   TNLAssert(false, "Not implemented");
   return Point();
}


void Geometry::setVert(const Point &pos, S32 index)
{
   TNLAssert(false, "Not implemented");
}


S32 Geometry::getVertCount()
{
   TNLAssert(false, "Not implemented");
   return 0;
}


S32 Geometry::getMinVertCount()
{
   TNLAssert(false, "Not implemented");
   return 0;
}


void Geometry::clearVerts()
{
   TNLAssert(false, "Not implemented");
}


bool Geometry::addVert(const Point &point, bool ignoreMaxPointsLimit)
{
   TNLAssert(false, "Not implemented");
   return false;
}


bool Geometry::addVertFront(Point vert)
{
   TNLAssert(false, "Not implemented");
   return false;
}


bool Geometry::deleteVert(S32 vertIndex)
{
   TNLAssert(false, "Not implemented");
   return false;
}


bool Geometry::insertVert(Point vertex, S32 vertIndex)
{
   TNLAssert(false, "Not implemented");
   return false;
}


bool Geometry::anyVertsSelected()
{
   TNLAssert(false, "Not implemented");
   return false;
}


void Geometry::selectVert(S32 vertIndex)
{
   TNLAssert(false, "Not implemented");
}


void Geometry::aselectVert(S32 vertIndex)
{
   TNLAssert(false, "Not implemented");
}


void Geometry::unselectVert(S32 vertIndex)
{
   TNLAssert(false, "Not implemented");
}


void Geometry::unselectVerts()
{
   TNLAssert(false, "Not implemented");
}


bool Geometry::vertSelected(S32 vertIndex)
{
   TNLAssert(false, "Not implemented");
   return false;
}


const Vector<Point> *Geometry::getOutline() const
{
   TNLAssert(false, "Not implemented");
   return NULL;
}


const Vector<Point> *Geometry::getFill() const
{
   TNLAssert(false, "Not implemented");
   return NULL;
}


Point Geometry::getCentroid()
{
   TNLAssert(false, "Not implemented");
   return Point();
}


F32 Geometry::getLabelAngle()
{
   TNLAssert(false, "Not implemented");
   return 0;
}


void Geometry::packGeom(GhostConnection *connection, BitStream *stream)
{
   TNLAssert(false, "Not implemented");
}


void Geometry::unpackGeom(GhostConnection *connection, BitStream *stream)
{
   TNLAssert(false, "Not implemented");
}


string Geometry::geomToString(F32 gridSize) const
{
   TNLAssert(false, "Not implemented");
   return string();
}


void Geometry::readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize)
{
   TNLAssert(false, "Not implemented");
}


Rect Geometry::calcExtents()
{
   TNLAssert(false, "Not implemented");
   return Rect();
}


void Geometry::onPointsChanged()
{
   TNLAssert(false, "Not implemented");
}


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


void Geometry::flip(F32 center, bool isHoriz)
{
   for(S32 i = 0; i < getVertCount(); i++)
   {
      Point p = getVert(i);
      
      if(isHoriz)
         p.x = center * 2 - p.x;
      else
         p.y = center * 2 - p.y;

      setVert(p, i);
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


// Constructor
PointGeometry::PointGeometry()
{
   mPosIsSelected = false;
}


// Destructor
PointGeometry::~PointGeometry()
{
   /* TNLAssert(false, "deleting!");*/
}


GeomType PointGeometry::getGeomType()
{
   return geomPoint;
}


Point PointGeometry::getVert(S32 index) const
{
   return getActualPos();
}


S32 PointGeometry::getVertCount()
{
   return 1;
}


S32 PointGeometry::getMinVertCount()
{
   return 1;
}


void PointGeometry::clearVerts()
{
   // Do nothing
}


bool PointGeometry::addVert(const Point &point, bool ignoreMaxPointsLimit)
{
   return false;
}


bool PointGeometry::addVertFront(Point vert)
{
   return false;
}


bool PointGeometry::deleteVert(S32 vertIndex)
{
   return false;
}


bool PointGeometry::insertVert(Point vertex, S32 vertIndex)
{
   return false;
}


bool PointGeometry::anyVertsSelected()
{
   return mPosIsSelected;
}


void PointGeometry::selectVert(S32 vertIndex)
{
   mPosIsSelected = true;
}


void PointGeometry::aselectVert(S32 vertIndex)
{
   mPosIsSelected = true;
}


void PointGeometry::unselectVert(S32 vertIndex)
{
   mPosIsSelected = false;
}


void PointGeometry::unselectVerts()
{
   mPosIsSelected = false;
}


bool PointGeometry::vertSelected(S32 vertIndex)
{
   return mPosIsSelected;
}


const Vector<Point> *PointGeometry::getOutline() const
{
   TNLAssert(false, "Points do not have outline!");
   return NULL;
}


const Vector<Point> *PointGeometry::getFill() const
{
   TNLAssert(false, "Points do not have fill!");
   return NULL;
}


Point PointGeometry::getCentroid()
{
   return getActualPos();
}


F32 PointGeometry::getLabelAngle()
{
   return 0;
}


void PointGeometry::onPointsChanged()
{
   // Do nothing
}


void PointGeometry::setVert(const Point &pos, S32 index)
{
	setActualPos(pos);
}


void PointGeometry::packGeom(GhostConnection *connection, BitStream *stream)
{
   getActualPos().write(stream);
}


void PointGeometry::unpackGeom(GhostConnection *connection, BitStream *stream)
{
   Point p;
   p.read(stream);
   setActualPos(p);
}


Rect PointGeometry::calcExtents()
{
   return Rect(getActualPos(), 1);
}


string PointGeometry::geomToString(F32 gridSize) const
{
   Point pos = getActualPos(); 
   return (pos / gridSize).toString();
}


void PointGeometry::readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize)
{
   TNLAssert(false, "Haven't figured this one out yet!");
}


////////////////////////////////////////
////////////////////////////////////////

static Vector<Point> outlinePoints(2);    // Reusable container

// Constructor
SimpleLineGeometry::SimpleLineGeometry()
{
   mFromSelected = false;
   mToSelected = false;
}


// Destructor
SimpleLineGeometry::~SimpleLineGeometry()
{
   //TNLAssert(false, "deleting!");
}


GeomType SimpleLineGeometry::getGeomType()
{
   return geomSimpleLine;
}


Point SimpleLineGeometry::getVert(S32 index) const
{
   return (index == 1) ? mToPos : mFromPos;
}


void SimpleLineGeometry::setVert(const Point &pos, S32 index)
{
   if (index == 1)
      mToPos = pos;
   else
      mFromPos = pos;
}


S32 SimpleLineGeometry::getVertCount()
{
   return 2;
}


S32 SimpleLineGeometry::getMinVertCount()
{
   return 2;
}


void SimpleLineGeometry::clearVerts()
{
   // Do nothing
}


bool SimpleLineGeometry::addVert(const Point &point, bool ignoreMaxPointsLimit)
{
   return false;
}


bool SimpleLineGeometry::addVertFront(Point vert)
{
   return false;
}


bool SimpleLineGeometry::deleteVert(S32 vertIndex)
{
   return false;
}


bool SimpleLineGeometry::insertVert(Point vertex, S32 vertIndex)
{
   return false;
}


bool SimpleLineGeometry::anyVertsSelected()
{
   return mFromSelected | mToSelected;
}


void SimpleLineGeometry::selectVert(S32 vertIndex)
{
   unselectVerts();
   if (vertIndex == 1)
      mToSelected = true;
   else
      mFromSelected = true;
}


void SimpleLineGeometry::aselectVert(S32 vertIndex)
{
   if (vertIndex == 1)
      mToSelected = true;
   else
      mFromSelected = true;
}


void SimpleLineGeometry::unselectVert(S32 vertIndex)
{
   if (vertIndex == 1)
      mToSelected = false;
   else
      mFromSelected = false;
}


void SimpleLineGeometry::unselectVerts()
{
   mFromSelected = false;
   mToSelected = false;
}


bool SimpleLineGeometry::vertSelected(S32 vertIndex)
{
   return (vertIndex == 1) ? mToSelected : mFromSelected;
}


const Vector<Point> *SimpleLineGeometry::getFill() const
{
   TNLAssert(false, "SimpleLines do not have fill!");
   return NULL;
}


Point SimpleLineGeometry::getCentroid()
{
   return (mFromPos + mToPos) / 2; // Returns midpoint of line
}


F32 SimpleLineGeometry::getLabelAngle()
{
   return mFromPos.angleTo(mToPos);
}


void SimpleLineGeometry::onPointsChanged()
{
   // Do nothing
}


const Vector<Point> *SimpleLineGeometry::getOutline() const
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


Rect SimpleLineGeometry::calcExtents()
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


////////////////////////////////////////
////////////////////////////////////////

extern S32 gMaxPolygonPoints;

// Constructor
PolylineGeometry::PolylineGeometry()
{
   mAnyVertsSelected = false;
}


// Destructor
PolylineGeometry::~PolylineGeometry()
{
   /*TNLAssert(false, "deleting!");*/
}


GeomType PolylineGeometry::getGeomType()
{
   return geomPolyLine;
}


Point PolylineGeometry::getVert(S32 index)  const
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


S32 PolylineGeometry::getMinVertCount()
{
   return 2;
}


void PolylineGeometry::clearVerts() 
{ 
   mPolyBounds.clear(); 
   mVertSelected.clear(); 
   mAnyVertsSelected = false;
   onPointsChanged();
}


bool PolylineGeometry::addVert(const Point &point, bool ignoreMaxPointsLimit) 
{ 
   if(mPolyBounds.size() >= gMaxPolygonPoints && !ignoreMaxPointsLimit)
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
   updateExtentInDatabase();
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


const Vector<Point> *PolylineGeometry::getOutline() const
{
   return (Vector<Point> *) &mPolyBounds;
}


const Vector<Point> *PolylineGeometry::getFill() const
{
   TNLAssert(false, "Polylines don't have fill!");
   return NULL;
}


Point PolylineGeometry::getCentroid()
{
   TNLAssert(false, "Polylines don't have Centroid");
   return Point();
}


F32 PolylineGeometry::getLabelAngle()
{
   return 0;
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


Rect PolylineGeometry::calcExtents()
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
    readPolyBounds(argc, argv, firstCoord, gridSize, true, mPolyBounds);      // Fills mPolyBounds
    mVertSelected.resize(mPolyBounds.size());
    onPointsChanged();
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
PolygonGeometry::PolygonGeometry() : PolylineGeometry()
{
   mLabelAngle = 0;
   mTriangluationDisabled = false;
}


GeomType PolygonGeometry::getGeomType()
{
   return geomPolygon;
}


const Vector<Point> *PolygonGeometry::getFill() const
{
   TNLAssert(!mTriangluationDisabled, "Triangluation disabled!");
   return &mPolyFill;
}

Point PolygonGeometry::getCentroid()
{
   TNLAssert(!mTriangluationDisabled, "Triangluation disabled!");
   return mCentroid;
}

F32 PolygonGeometry::getLabelAngle()
{
   TNLAssert(!mTriangluationDisabled, "Triangluation disabled!");
   return mLabelAngle;
}


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

   Parent::onPointsChanged();
}

void PolygonGeometry::disableTriangluation()
{
   mTriangluationDisabled = true;
}


S32 PolygonGeometry::getMinVertCount()
{
   return 3;
}

};
