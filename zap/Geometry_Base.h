//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------


#ifndef _GEOMETRY_BASE_H_
#define _GEOMETRY_BASE_H_

#include "Rect.h"

namespace TNL
{
   class GhostConnection; 
   class BitStream;
}

namespace boost{ template<class T> class shared_ptr; }      // or use #include "boost/smart_ptr/shared_ptr.hpp"

using namespace std;
using namespace TNL;


namespace Zap
{

enum GeomType {           
   geomPoint,        // One point      
   geomSimpleLine,   // Two points   
   geomPolyLine,     // Many points
   geomPolygon,      // Many points, closed loop
   geomNone,         // Singularity   
};


// Geometry is essentially an interface class that all geometric objects implement
class Geometry
{
public:
   static const S32 MAX_POLY_POINTS = 64;       // Max number of points we can have in Walls, Nexuses, LoadoutZones, etc.

   Geometry();           // Constructor
   virtual ~Geometry();  // Destructor

   virtual GeomType getGeomType() const;

   virtual Point getVert(S32 index) const;
   virtual void setVert(const Point &pos, S32 index);
   virtual S32 getVertCount() const;
   virtual S32 getMinVertCount() const;      // Minimum number of vertices geometry needs to be viable
   virtual void clearVerts();
   virtual bool addVert(const Point &point, bool ignoreMaxPointsLimit = false);
   virtual bool addVertFront(Point vert);
   virtual bool deleteVert(S32 vertIndex);
   virtual bool insertVert(Point vertex, S32 vertIndex);
   virtual void reverseWinding();

   virtual bool anyVertsSelected();

   virtual void selectVert(S32 vertIndex);
   virtual void aselectVert(S32 vertIndex);   // Select another vertex (remember cmdline ArcInfo?)
   virtual void unselectVert(S32 vertIndex);
   virtual void unselectVerts();
   virtual bool vertSelected(S32 vertIndex);

   virtual const Vector<Point> *getOutline() const;
   virtual const Vector<Point> *getFill() const;
   virtual Point getCentroid() const;
   virtual F32 getLabelAngle() const;
                                                            
   virtual void packGeom(GhostConnection *connection, BitStream *stream);
   virtual void unpackGeom(GhostConnection *connection, BitStream *stream);
   virtual void setGeom(const Vector<Point> &points);

   virtual string geomToLevelCode() const;
   virtual void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   virtual void disableTriangulation();

   Geometry *newCopy();

   virtual Rect calcExtents();

   virtual void onPointsChanged();

   // These functions are declered in Geometry.cpp
   void rotateAboutPoint(const Point &center, F32 angle);
   void flip(F32 center, bool isHoriz);                   // Do a horizontal or vertical flip about line at center
   void scale(const Point &center, F32 scale);
   void moveTo(const Point &pos, S32 vertexIndexToBePositionedAtPos);
   void offset(const Point &offset);
};


};

#endif
