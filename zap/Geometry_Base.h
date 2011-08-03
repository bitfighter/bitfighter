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

namespace TNL{
   class GhostConnection; // speeds up compiler, or use #include "tnlGhostConnection.h"
   class BitStream;
}
namespace boost{ template<class T> class shared_ptr; }      // or use #include "boost/smart_ptr/shared_ptr.hpp"

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

   virtual GeomType getGeomType()                  {TNLAssert(false, "Not implemented"); return geomNone;}
   virtual Point getVert(S32 index) const          {TNLAssert(false, "Not implemented"); return Point();}
   virtual void setVert(const Point &pos, S32 index) {TNLAssert(false, "Not implemented");}
   virtual S32 getVertCount()                      {TNLAssert(false, "Not implemented"); return 0;}
   virtual void clearVerts()                       {TNLAssert(false, "Not implemented");}
   virtual bool addVert(const Point &point)        {TNLAssert(false, "Not implemented"); return false;}
   virtual bool addVertFront(Point vert)           {TNLAssert(false, "Not implemented"); return false;}
   virtual bool deleteVert(S32 vertIndex)          {TNLAssert(false, "Not implemented"); return false;}
   virtual bool insertVert(Point vertex, S32 vertIndex) {TNLAssert(false, "Not implemented"); return false;}

   virtual bool anyVertsSelected()                 {TNLAssert(false, "Not implemented"); return false;}

   virtual void selectVert(S32 vertIndex)          {TNLAssert(false, "Not implemented");}
   virtual void aselectVert(S32 vertIndex)         {TNLAssert(false, "Not implemented");}    // Select an additional vertex (remember command line ArcInfo?)
   virtual void unselectVert(S32 vertIndex)        {TNLAssert(false, "Not implemented");}
   virtual void unselectVerts()                    {TNLAssert(false, "Not implemented");}
   virtual bool vertSelected(S32 vertIndex)        {TNLAssert(false, "Not implemented"); return false;}

   virtual Vector<Point> *getOutline() const       {TNLAssert(false, "Not implemented"); return NULL;}
   virtual Vector<Point> *getFill() const          {TNLAssert(false, "Not implemented"); return NULL;}
   virtual Point getCentroid()                     {TNLAssert(false, "Not implemented"); return Point();}
   virtual F32 getLabelAngle()                     {TNLAssert(false, "Not implemented"); return 0;}
                                                            //
   virtual void packGeom(GhostConnection *connection, BitStream *stream) {TNLAssert(false, "Not implemented");}
   virtual void unpackGeom(GhostConnection *connection, BitStream *stream) {TNLAssert(false, "Not implemented");}

   virtual string geomToString(F32 gridSize) const                                    {TNLAssert(false, "Not implemented"); return string();}
   virtual void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize) {TNLAssert(false, "Not implemented");}

   //virtual boost::shared_ptr<Geometry> copyGeometry() const = 0;
   void newGeomCopy();

   virtual Rect getExtents()                       {TNLAssert(false, "Not implemented"); return Rect();}

   virtual void onPointsChanged()                  {TNLAssert(false, "Not implemented");}

   void disableTriangluation() { mTriangluationDisabled = true; }

   // These functions are declered in Geometry.cpp
   void rotateAboutPoint(const Point &center, F32 angle);
   void flipHorizontal(F32 centerX = 0);
   void flipVertical(F32 centerY = 0);
   void scale(const Point &center, F32 scale);


   virtual void onGeomChanged() { /* do nothing */ };      // Item changed geometry (or moved), do any internal updating that might be required

};
};

#endif