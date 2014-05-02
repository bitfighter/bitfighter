//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------


#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_

#include "Geometry_Base.h"    // Parent class
#include "Point.h"
#include "tnlVector.h"

namespace TNL
{
   class GhostConnection; // speeds up compiler, or use #include "tnlGhostConnection.h"
   class BitStream;
}

namespace boost{ template<class T> class shared_ptr; }      // or use #include <boost/smart_ptr/shared_ptr.hpp>

using namespace std;
using namespace TNL;


namespace Zap
{

// Geometry is in Geometry_Base.h
// The split .h files was due to BfObject trying to include us, and we need BfObject

////////////////////////////////////////
////////////////////////////////////////

class PointGeometry : public Geometry
{
private:
   bool mPosIsSelected;
   Point mPoint;
   F32 mRadius;

   virtual Point getVert(S32 index) const;
   virtual void setVert(const Point &pos, S32 index);

public:
   explicit PointGeometry(F32 radius);          // Constructor
   virtual ~PointGeometry();                    // Destructor

   GeomType getGeomType() const;

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
   Point getCentroid() const;
   F32 getLabelAngle() const;

   void packGeom(GhostConnection *connection, BitStream *stream);
   void unpackGeom(GhostConnection *connection, BitStream *stream);
   void setGeom(const Vector<Point> &points);

   string geomToLevelCode() const;
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

   GeomType getGeomType() const;

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
   Point getCentroid() const;
   F32 getLabelAngle() const;

   void packGeom(GhostConnection *connection, BitStream *stream);
   void unpackGeom(GhostConnection *connection, BitStream *stream); 
   void setGeom(const Vector<Point> &points);

   string geomToLevelCode() const;
   void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   virtual Rect calcExtents();
};


////////////////////////////////////////
////////////////////////////////////////

class PolylineGeometry : public Geometry
{

typedef Geometry Parent;

private:
   Point mCentroid;

protected:
   Vector<Point> mPolyBounds;
      
   bool mAnyVertsSelected;
   Vector<bool> mVertSelected; 

   void checkIfAnyVertsSelected();     

public:
   PolylineGeometry();           // Constructor
   virtual ~PolylineGeometry();  // Destructor

   virtual GeomType getGeomType() const;

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
   virtual Point getCentroid() const;
   virtual F32 getLabelAngle() const;

   void packGeom(GhostConnection *connection, BitStream *stream);
   void unpackGeom(GhostConnection *connection, BitStream *stream); 
   void setGeom(const Vector<Point> &points);

   string geomToLevelCode() const;
   virtual void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);
   virtual void onPointsChanged();

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
   bool mTriangluationDisabled;     // Allow optimization of adding points for polygons that will never be displayed

public:
   PolygonGeometry();      // Constructor
   virtual ~PolygonGeometry();

   GeomType getGeomType() const;

   const Vector<Point> *getFill() const;
   Point getCentroid() const;
   F32 getLabelAngle() const;

   void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   virtual void onPointsChanged();

   void disableTriangulation();

   S32 getMinVertCount() const;
};


};

#endif
