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
#include "tnlVector.h"
#include "gameObject.h" // for BfObject

namespace TNL
{
   class GhostConnection; // speeds up compiler, or use #include "tnlGhostConnection.h"
   class BitStream;
}

namespace boost{ template<class T> class shared_ptr; }      // or use #include "boost/smart_ptr/shared_ptr.hpp"

using namespace std;
using namespace TNL;


namespace Zap
{

// Geometry is in Geometry_Base.h
// The split .h files was due to gameObject trying to include us, and we need BfObject

////////////////////////////////////////
////////////////////////////////////////

class PointGeometry : public Geometry
{
private:
   bool mPosIsSelected;
   Point mPoint;

   virtual Point getVert(S32 index) const;
   virtual void setVert(const Point &pos, S32 index);

public:
   PointGeometry();                             // Constructor
   ~PointGeometry();                            // Destructor

   GeomType getGeomType();

   S32 getVertCount() const;
   S32 getMinVertCount() const;

   void clearVerts();
   bool addVert(const Point &point, bool ignoreMaxPointsLimit = false);
   bool addVertFront(Point vert);
   bool deleteVert(S32 vertIndex);
   bool insertVert(Point vertex, S32 vertIndex);

   bool anyVertsSelected();
   void selectVert(S32 vertIndex);
   void aselectVert(S32 vertIndex);
   void unselectVert(S32 vertIndex);
   void unselectVerts();
   bool vertSelected(S32 vertIndex);

   const Vector<Point> *getOutline() const;
   const Vector<Point> *getFill() const;
   Point getCentroid();
   F32 getLabelAngle();

   void packGeom(GhostConnection *connection, BitStream *stream);
   void unpackGeom(GhostConnection *connection, BitStream *stream);
   void setGeom(const Vector<Point> &points);

   string geomToString(F32 gridSize) const;
   void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   virtual Rect calcExtents();
};


////////////////////////////////////////
////////////////////////////////////////

class SimpleLineGeometry : public Geometry
{
private:
   Point mFromPos, mToPos;
   bool mFromSelected, mToSelected;

public:
   SimpleLineGeometry();           // Constructor
   virtual ~SimpleLineGeometry();  // Destructor

   GeomType getGeomType();

   Point getVert(S32 index) const;
   void setVert(const Point &pos, S32 index);

   S32 getVertCount() const;
   S32 getMinVertCount() const;

   void clearVerts();
   bool addVert(const Point &point, bool ignoreMaxPointsLimit = false);
   bool addVertFront(Point vert);
   bool deleteVert(S32 vertIndex);
   bool insertVert(Point vertex, S32 vertIndex);

   bool anyVertsSelected();
   void selectVert(S32 vertIndex);
   void aselectVert(S32 vertIndex);
   void unselectVert(S32 vertIndex);
   void unselectVerts();
   bool vertSelected(S32 vertIndex);

   const Vector<Point> *getOutline() const;
   const Vector<Point> *getFill() const;
   Point getCentroid();
   F32 getLabelAngle();

   void packGeom(GhostConnection *connection, BitStream *stream);
   void unpackGeom(GhostConnection *connection, BitStream *stream); 
   void setGeom(const Vector<Point> &points);

   string geomToString(F32 gridSize) const;
   void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   virtual Rect calcExtents();
};


////////////////////////////////////////
////////////////////////////////////////

class PolylineGeometry : public Geometry
{
protected:
   Vector<Point> mPolyBounds;
      
   bool mAnyVertsSelected;
   vector<bool> mVertSelected; 

   void checkIfAnyVertsSelected();     

public:
   PolylineGeometry();    // Constructor
   ~PolylineGeometry();   // Destructor

   virtual GeomType getGeomType();

   Point getVert(S32 index) const;
   virtual void setVert(const Point &pos, S32 index);

   S32 getVertCount() const;
   S32 getMinVertCount() const;

   void clearVerts();
   bool addVert(const Point &point, bool ignoreMaxPointsLimit = false);
   bool addVertFront(Point vert);
   bool deleteVert(S32 vertIndex);
   bool insertVert(Point vertex, S32 vertIndex);

   bool anyVertsSelected();
   void selectVert(S32 vertIndex);
   void aselectVert(S32 vertIndex);
   void unselectVert(S32 vertIndex);
   void unselectVerts();
   bool vertSelected(S32 vertIndex);

   const Vector<Point> *getOutline() const;
   virtual const Vector<Point> *getFill() const;
   virtual Point getCentroid();
   virtual F32 getLabelAngle();

   void packGeom(GhostConnection *connection, BitStream *stream);
   void unpackGeom(GhostConnection *connection, BitStream *stream); 
   void setGeom(const Vector<Point> &points);

   string geomToString(F32 gridSize) const;
   virtual void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   virtual Rect calcExtents();
};


////////////////////////////////////////
////////////////////////////////////////

class PolygonGeometry : public PolylineGeometry
{
   typedef PolylineGeometry Parent;

private:
   Vector<Point> mPolyFill;
   F32 mLabelAngle;
   Point mCentroid;
   bool mTriangluationDisabled;     // Allow optimization of adding points for polygons that will never be displayed

public:
   PolygonGeometry();      // Constructor

   GeomType getGeomType();

   const Vector<Point> *getFill() const;
   Point getCentroid();
   F32 getLabelAngle();

   void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   virtual void onPointsChanged();

   void disableTriangulation();

   S32 getMinVertCount() const;
};


};

#endif
