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

#ifndef _GEOMOBJECT_H_
#define _GEOMOBJECT_H_

#include "Geometry_Base.h"    // For Geometry class def
#include "Point.h"

namespace Zap
{

class GeometryContainer
{
private:
   Geometry *mGeometry;

public:
   GeometryContainer();                                     // Constructor
   GeometryContainer(const GeometryContainer &container);   // Copy constructor
   virtual ~GeometryContainer();                            // Destructor

   Geometry *getGeometry() const;
   const Geometry *getConstGeometry() const;
   void setGeometry(Geometry *geometry);
   void reverseWinding();

   const Vector<Point> *getOutline() const;
   const Vector<Point> *getFill() const;
   
   Point getVert(S32 index) const;
   string geomToLevelCode(F32 gridSize) const; 
};


////////////////////////////////////////
////////////////////////////////////////

class GeomObject 
{
private:
   GeometryContainer mGeometry;

public:
   GeomObject();                          // Constructor
   virtual ~GeomObject();                 // Destructor

   void setNewGeometry(GeomType geomType, F32 radius = 0);

   GeomType getGeomType() const;

   virtual Point getVert(S32 index) const;               // Overridden by MoveObject
   virtual void setVert(const Point &pos, S32 index);    // Overridden by MoveObject

   S32 getMinVertCount() const;       // Minimum  vertices geometry needs to be viable
   S32 getVertCount() const;          // Actual number of vertices in the geometry

   bool anyVertsSelected();
   void selectVert(S32 vertIndex);
   void aselectVert(S32 vertIndex);   // Select another vertex (remember cmdline ArcInfo?)
   void unselectVert(S32 vertIndex);

   void clearVerts();
   bool addVert(const Point &point, bool ignoreMaxPointsLimit = false);
   bool addVertFront(Point vert);
   bool deleteVert(S32 vertIndex);
   bool insertVert(Point vertex, S32 vertIndex);
   void unselectVerts();
   bool vertSelected(S32 vertIndex);

   // Transforming the geometry
   void rotateAboutPoint(const Point &center, F32 angle);
   void flip(F32 center, bool isHoriz);                   // Do a horizontal or vertical flip about line at center
   void scale(const Point &center, F32 scale);
   void moveTo(const Point &pos, S32 snapVertex = 0);     // Move object to location, specifying (optional) vertex to be positioned at pos
   void offset(const Point &offset);                      // Offset object by a certain amount

   // Getting parts of the geometry
   Point getCentroid() const;
   F32 getLabelAngle() const;

   const Vector<Point> *getOutline() const;
   const Vector<Point> *getFill()    const;

   void reverseWinding();     

   virtual Rect calcExtents();

   void disableTriangulation();

   // Sending/receiving
   void packGeom(GhostConnection *connection, BitStream *stream);
   void unpackGeom(GhostConnection *connection, BitStream *stream);

   // Saving/loading
   string geomToLevelCode(F32 gridSize) const;
   void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

   virtual void onPointsChanged();
   virtual void onGeomChanging();      // Item geom is interactively changing
   virtual void onGeomChanged();       // Item changed geometry (or moved), do any internal updating that might be required

   virtual void setExtent(const Rect &extentRect) = 0;

   virtual Point getPos() const;
   virtual Point getRenderPos() const;

   virtual void setPos(const Point &pos);

   virtual void setGeom(const Vector<Point> &points);
};

}


#endif
