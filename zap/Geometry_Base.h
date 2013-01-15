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


#ifndef _GEOMETRY_BASE_H_
#define _GEOMETRY_BASE_H_

#include "Rect.h"

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

   virtual string geomToLevelCode(F32 gridSize) const;
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
