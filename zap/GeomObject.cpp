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

#include "GeomObject.h"
#include "Geometry.h"

using namespace TNL;

namespace Zap
{


// Constructor
GeomObject::GeomObject()
{
   // Do nothing
}


//// Copy constructor
//GeomObject::GeomObject(const GeomObject &g) 
//{
//   mGeometry = g.mGeometry;
//}


// Destructor
GeomObject::~GeomObject()
{
   // Do nothing
};


// mGeometry will be deleted in destructor; radius default to 0
void GeomObject::setNewGeometry(GeomType geomType, F32 radius)
{
   TNLAssert(!mGeometry.getGeometry(), "This object already has a geometry!");

   switch(geomType)
   {
      case geomPoint:
         mGeometry.setGeometry(new PointGeometry(radius));
         return;

      case geomSimpleLine:
         mGeometry.setGeometry(new SimpleLineGeometry());
         return;

      case geomPolyLine:
         mGeometry.setGeometry(new PolylineGeometry());
         return;

      case geomPolygon:
         mGeometry.setGeometry(new PolygonGeometry());
         return;

      default:
         TNLAssert(false, "Unknown geometry!");
         break;
   }
}


// Basic definitions
GeomType GeomObject::getGeomType() const        {   return mGeometry.getGeometry()->getGeomType();   }
Point    GeomObject::getVert(S32 index) const   {   return mGeometry.getVert(index);  }

bool GeomObject::deleteVert(S32 vertIndex)               
{   
   if(mGeometry.getGeometry()->deleteVert(vertIndex))
   {
      onPointsChanged();  
      return true;
   }

   return false;
}


bool GeomObject::insertVert(Point vertex, S32 vertIndex) 
{   
   if(mGeometry.getGeometry()->insertVert(vertex, vertIndex))
   {
      onPointsChanged();
      return true;
   }

   return false;
}


void GeomObject::setVert(const Point &pos, S32 index)    { mGeometry.getGeometry()->setVert(pos, index); }
                                                                                           
bool GeomObject::anyVertsSelected()          {   return mGeometry.getGeometry()->anyVertsSelected();        }
S32 GeomObject::getVertCount() const         {   return mGeometry.getGeometry()->getVertCount();            }
S32 GeomObject::getMinVertCount() const      {   return mGeometry.getGeometry()->getMinVertCount();         }

void GeomObject::clearVerts()                {   mGeometry.getGeometry()->clearVerts(); onPointsChanged();  }                        


bool GeomObject::addVertFront(Point vert)
{
   if(mGeometry.getGeometry()->addVertFront(vert))
   {
      onPointsChanged();
      return true;
   }

   return false;
}


bool GeomObject::addVert(const Point &point, bool ignoreMaxPointsLimit) 
{
   if(mGeometry.getGeometry()->addVert(point, ignoreMaxPointsLimit))
   {
      onPointsChanged();
      return true;
   }

   return false;
}


// Vertex selection -- only needed in editor
void GeomObject::selectVert(S32 vertIndex)   {   mGeometry.getGeometry()->selectVert(vertIndex);            }
void GeomObject::aselectVert(S32 vertIndex)  {   mGeometry.getGeometry()->aselectVert(vertIndex);           }
void GeomObject::unselectVert(S32 vertIndex) {   mGeometry.getGeometry()->unselectVert(vertIndex);          }
void GeomObject::unselectVerts()             {   mGeometry.getGeometry()->unselectVerts();                  }
     
bool GeomObject::vertSelected(S32 vertIndex) {   return mGeometry.getGeometry()->vertSelected(vertIndex);   }

// Geometric calculations
Point GeomObject::getCentroid()   const {   return mGeometry.getGeometry()->getCentroid();     }
F32   GeomObject::getLabelAngle() const {   return mGeometry.getGeometry()->getLabelAngle();   }
      

// Geometry operations
const Vector<Point> *GeomObject::getOutline() const       {   return mGeometry.getOutline();    }
const Vector<Point> *GeomObject::getFill() const          {   return mGeometry.getFill();       }

void GeomObject::reverseWinding() { mGeometry.reverseWinding(); }


// Geometric manipulations
void GeomObject::rotateAboutPoint(const Point &center, F32 angle)  {  mGeometry.getGeometry()->rotateAboutPoint(center, angle);   }
void GeomObject::flip(F32 center, bool isHoriz)                    {  mGeometry.getGeometry()->flip(center, isHoriz);             }
void GeomObject::scale(const Point &center, F32 scale)             {  mGeometry.getGeometry()->scale(center, scale);              }

// Move object to location, specifying (optional) vertex to be positioned at pos
void GeomObject::moveTo(const Point &pos, S32 snapVertex)          {  mGeometry.getGeometry()->moveTo(pos, snapVertex);           }
void GeomObject::offset(const Point &offset)                       {  mGeometry.getGeometry()->offset(offset);                    }

// Geom in-out
void GeomObject::packGeom(GhostConnection *connection, BitStream *stream)    {   mGeometry.getGeometry()->packGeom(connection, stream);     }
void GeomObject::unpackGeom(GhostConnection *connection, BitStream *stream)  {   mGeometry.getGeometry()->unpackGeom(connection, stream); onPointsChanged();  }
void GeomObject::setGeom(const Vector<Point> &points)                        {   mGeometry.getGeometry()->setGeom(points); }

void GeomObject::readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize) 
{  
   mGeometry.getGeometry()->readGeom(argc, argv, firstCoord, gridSize); 
   onPointsChanged();
} 

string GeomObject::geomToString(F32 gridSize) const {  return mGeometry.geomToString(gridSize);  }


Rect GeomObject::calcExtents() {  return mGeometry.getGeometry()->calcExtents();  }


// Settings
void GeomObject::disableTriangulation() {   mGeometry.getGeometry()->disableTriangulation();   }


Point GeomObject::getPos()       const { return getVert(0); }
Point GeomObject::getRenderPos() const { return getPos();   }


void GeomObject::setPos(const Point &pos)
{
   setVert(pos, 0);  
   setExtent(calcExtents());
}


void GeomObject::onGeomChanging()
{
   if(getGeomType() == geomPolygon)
      onGeomChanged();               // Allows poly fill to get reshaped as vertices move

   onPointsChanged();
}


void GeomObject::onGeomChanged() {  /* Do nothing */ }


void GeomObject::onPointsChanged()                        
{   
   mGeometry.getGeometry()->onPointsChanged();
}

////////////////////////////////////////
////////////////////////////////////////

// Constructor
GeometryContainer::GeometryContainer()
{
   mGeometry = NULL;
}


// Copy constructor
GeometryContainer::GeometryContainer(const GeometryContainer &container)
{
   const Geometry *old = container.mGeometry;

   switch(container.mGeometry->getGeomType())
   {
      case geomPoint:
         mGeometry = new PointGeometry(*static_cast<const PointGeometry *>(old));
         break;
      case geomSimpleLine:
         mGeometry = new SimpleLineGeometry(*static_cast<const SimpleLineGeometry *>(old));
         break;
      case geomPolyLine:
         mGeometry = new PolylineGeometry(*static_cast<const PolylineGeometry *>(old));
         break;
      case geomPolygon:
         mGeometry = new PolygonGeometry(*static_cast<const PolygonGeometry *>(old));
         break;
      default:
         TNLAssert(false, "Invalid value!");
         break;
   }
}


// Destructor
GeometryContainer::~GeometryContainer()
{
   delete mGeometry;
}


Geometry *GeometryContainer::getGeometry() const
{
   return mGeometry;
}


const Geometry *GeometryContainer::getConstGeometry() const
{
   return mGeometry;
}


void GeometryContainer::setGeometry(Geometry *geometry)
{
   mGeometry = geometry;
}


void GeometryContainer::reverseWinding()    
{
   mGeometry->reverseWinding();
}


const Vector<Point> *GeometryContainer::getOutline() const
{
   return mGeometry->getOutline();
}


const Vector<Point> *GeometryContainer::getFill() const    
{
   return mGeometry->getFill();
}


Point GeometryContainer::getVert(S32 index) const   
{   
   return mGeometry->getVert(index);  
}


string GeometryContainer::geomToString(F32 gridSize) const 
{  
   return mGeometry->geomToString(gridSize);  
}


};