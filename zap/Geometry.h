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


#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_

#include "Point.h"
#include "Rect.h"
#include "tnlTypes.h"
#include "tnlGhostConnection.h"

#include "boost/smart_ptr/shared_ptr.hpp"

using namespace std;
using namespace TNL;

namespace Zap
{

 enum GeomType {           
   geomPoint,           
   geomSimpleLine,      
   geomLine,            // TODO: change to geomPolyline
   geomPoly,            // TODO: Change to geomPolygon
   geomNone,            
};

class Geometry
{
protected:
   bool mTriangluationDisabled;     // Allow optimization of adding points for polygons that will never be displayed

public:

   Geometry() { mTriangluationDisabled = false; }     // Constructor

   virtual GeomType getGeomType() = 0;
   virtual Point getVert(S32 index) = 0;
   virtual void setVert(const Point &pos, S32 index) = 0;
   virtual S32 getVertCount() = 0;
   virtual void clearVerts() = 0;
   virtual bool addVert(const Point &point) = 0;
   virtual bool addVertFront(Point vert) = 0;
   virtual bool deleteVert(S32 vertIndex) = 0;
   virtual bool insertVert(Point vertex, S32 vertIndex) = 0;

   virtual bool anyVertsSelected() = 0;

   virtual void selectVert(S32 vertIndex) = 0;
   virtual void aselectVert(S32 vertIndex) = 0;    // Select an additional vertex (remember command line ArcInfo?)
   virtual void unselectVert(S32 vertIndex) = 0;
   virtual void unselectVerts() = 0;
   virtual bool vertSelected(S32 vertIndex) = 0;

   virtual Vector<Point> *getOutline() = 0;
   virtual Vector<Point> *getFill() = 0;
   virtual Point getCentroid() = 0;
   virtual F32 getLabelAngle() = 0;

   virtual void packGeom(GhostConnection *connection, BitStream *stream) = 0;
   virtual void unpackGeom(GhostConnection *connection, BitStream *stream) = 0; 

   virtual string geomToString(F32 gridSize) = 0;
   virtual void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize) = 0;

   virtual boost::shared_ptr<Geometry> copyGeometry() = 0;
   void newGeomCopy();

   virtual Rect getExtents() = 0;

   virtual void onPointsChanged() = 0;

   void disableTriangluation() { mTriangluationDisabled = true; }
};


////////////////////////////////////////
////////////////////////////////////////

class PointGeometry : public Geometry
{
private:
   Point mPos;
   bool mPosIsSelected;

public:
   PointGeometry() { mPosIsSelected = false; }     // Constructor
   ~PointGeometry() {  /* TNLAssert(false, "deleting!");*/ }      // Destructor

   GeomType getGeomType() { return geomPoint; }
   virtual Point getVert(S32 index) { return mPos; }
   void setVert(const Point &pos, S32 index) { mPos = pos; }

   S32 getVertCount() { return 1; }

   void clearVerts() { /* Do nothing */ }
   bool addVert(const Point &point) { return false; }
   bool addVertFront(Point vert) { return false; }
   bool deleteVert(S32 vertIndex) { return false; }
   bool insertVert(Point vertex, S32 vertIndex) { return false; }

   bool anyVertsSelected() { return mPosIsSelected; }
   void selectVert(S32 vertIndex) { mPosIsSelected = true; }
   void aselectVert(S32 vertIndex) { mPosIsSelected = true; }
   void unselectVert(S32 vertIndex) { mPosIsSelected = false; }
   void unselectVerts() { mPosIsSelected = false; } 
   bool vertSelected(S32 vertIndex) { return mPosIsSelected; }

   Vector<Point> *getOutline() { return NULL; }
   Vector<Point> *getFill() { return NULL; }
   Point getCentroid() { return mPos; }
   F32 getLabelAngle() { return 0; }

   void packGeom(GhostConnection *connection, BitStream *stream);
   void unpackGeom(GhostConnection *connection, BitStream *stream); 

   string geomToString(F32 gridSize);
   void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   boost::shared_ptr<Geometry> copyGeometry();

   Rect getExtents();

   void onPointsChanged() { /* Do nothing */ }
};


////////////////////////////////////////
////////////////////////////////////////

class SimpleLineGeometry : public Geometry
{
private:
   Point mFromPos, mToPos;
   bool mFromSelected, mToSelected;

public:
   SimpleLineGeometry() { mFromSelected = false; mToSelected = false; }    // Constructor
   ~SimpleLineGeometry() { 
      //TNLAssert(false, "deleting!");
   }
   GeomType getGeomType() { return geomSimpleLine; }
   Point getVert(S32 index) { return (index == 1) ? mToPos : mFromPos; }
   void setVert(const Point &pos, S32 index) { if(index == 1) mToPos = pos; else mFromPos = pos; }

   S32 getVertCount() { return 2; }

   void clearVerts() { /* Do nothing */ }
   bool addVert(const Point &point)  { return false; }
   bool addVertFront(Point vert)  { return false; }
   bool deleteVert(S32 vertIndex)  {  return false; }
   bool insertVert(Point vertex, S32 vertIndex)  {  return false; }

   bool anyVertsSelected() { return mFromSelected | mToSelected; }
   void selectVert(S32 vertIndex) { unselectVerts(); if(vertIndex == 1) mToSelected = true; else mFromSelected = true; }
   void aselectVert(S32 vertIndex) { if(vertIndex == 1) mToSelected = true; else mFromSelected = true; }
   void unselectVert(S32 vertIndex) { if(vertIndex == 1) mToSelected = false; else mFromSelected = false; }
   void unselectVerts() { mFromSelected = false; mToSelected = false; } 
   bool vertSelected(S32 vertIndex) { return (vertIndex == 1) ? mToSelected : mFromSelected; }

   Vector<Point> *getOutline();
   Vector<Point> *getFill() { return NULL; }
   Point getCentroid() { return (mFromPos + mToPos) / 2; }        // Returns midpoint of line
   F32 getLabelAngle() { return mFromPos.angleTo(mToPos); }

   void packGeom(GhostConnection *connection, BitStream *stream);
   void unpackGeom(GhostConnection *connection, BitStream *stream); 

   string geomToString(F32 gridSize);
   void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   boost::shared_ptr<Geometry> copyGeometry();

   Rect getExtents();

   void onPointsChanged() { /* Do nothing */ }
};


////////////////////////////////////////
////////////////////////////////////////

class PolylineGeometry : public Geometry
{
protected:
   Vector<Point> mPolyBounds;
   Point mCentroid;
      
   bool mAnyVertsSelected;
   vector<bool> mVertSelected; 

   void checkIfAnyVertsSelected();     

public:
   PolylineGeometry() { mAnyVertsSelected = false; }              // Constructor
   ~PolylineGeometry() { /*TNLAssert(false, "deleting!");*/ }     // Destructor

   GeomType getGeomType() { return geomLine; }

   Point getVert(S32 index);
   virtual void setVert(const Point &pos, S32 index);

   S32 getVertCount();

   void clearVerts();
   bool addVert(const Point &point);
   bool addVertFront(Point vert);
   bool deleteVert(S32 vertIndex);
   bool insertVert(Point vertex, S32 vertIndex);

   bool anyVertsSelected();
   void selectVert(S32 vertIndex);
   void aselectVert(S32 vertIndex);
   void unselectVert(S32 vertIndex);
   void unselectVerts();
   bool vertSelected(S32 vertIndex);

   Vector<Point> *getOutline() { return &mPolyBounds; }
   virtual Vector<Point> *getFill() { return NULL; }
   Point getCentroid() { TNLAssert(!mTriangluationDisabled, "Triangluation disabled!"); return mCentroid; }   
   virtual F32 getLabelAngle() { return 0; }

   void packGeom(GhostConnection *connection, BitStream *stream);
   void unpackGeom(GhostConnection *connection, BitStream *stream); 

   string geomToString(F32 gridSize);
   virtual void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   virtual boost::shared_ptr<Geometry> copyGeometry();

   Rect getExtents();

   virtual void onPointsChanged();
};


////////////////////////////////////////
////////////////////////////////////////

class PolygonGeometry : public PolylineGeometry
{
private:
   Vector<Point> mPolyFill;
   F32 mLabelAngle;

public:
   PolygonGeometry() : PolylineGeometry() { mLabelAngle = 0; }      // Constructor

   GeomType getGeomType() { return geomPoly; }

   Vector<Point> *getFill() { TNLAssert(!mTriangluationDisabled, "Triangluation disabled!"); return &mPolyFill; }
   F32 getLabelAngle()      { TNLAssert(!mTriangluationDisabled, "Triangluation disabled!"); return mLabelAngle; }

   void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   virtual void onPointsChanged();

   boost::shared_ptr<Geometry> copyGeometry();
};


};

#endif