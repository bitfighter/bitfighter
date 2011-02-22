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

#ifndef _POLYGON_H_
#define _POLYGON_H_

#include "SweptEllipsoid.h"      // For polygon triangulation
#include "point.h"

namespace Zap
{

extern S32 gMaxPolygonPoints;

class Polyline
{
public:
   Vector<Point> mPolyBounds;


protected:
   void packUpdate(GhostConnection *connection, BitStream *stream)
   {
      // - 1 because writeEnum ranges from 0 to n-1; mPolyBounds.size() ranges from 1 to n
      stream->writeEnum(mPolyBounds.size() - 1, gMaxPolygonPoints);  
      for(S32 i = 0; i < mPolyBounds.size(); i++)
      {
         stream->write(mPolyBounds[i].x);
         stream->write(mPolyBounds[i].y);
      }
   }


   // Read a series of points from a command line, and add them to a Vector of points
   void processPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize, bool allowFirstAndLastPointToBeEqual)
   {
      Point p, lastP;
      
      for(S32 i = firstCoord; i < argc; i += 2)
      {
         // Put a cap on the number of vertices in a polygon
         if(mPolyBounds.size() >= gMaxPolygonPoints)      // || argc == i + 1 might be needed...
            break;

         p.set( (F32) atof(argv[i]) * gridSize, (F32) atof(argv[i+1]) * gridSize );

         if(i == firstCoord || p != lastP)
            mPolyBounds.push_back(p);
         
         lastP.set(p);
      }

      // Check if last point was same as first; if so, scrap it
      if(!allowFirstAndLastPointToBeEqual && mPolyBounds.first() == mPolyBounds.last())
         mPolyBounds.erase(mPolyBounds.size() - 1);
   }


   virtual void processPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize)
   {
      processPolyBounds(argc, argv, firstCoord, gridSize, true);
   }


   U32 unpackUpdate(GhostConnection *connection, BitStream *stream)
   {
      U32 size = stream->readEnum(gMaxPolygonPoints) + 1;

      for(U32 i = 0; i < size; i++)
      {
         Point p;
         stream->read(&p.x);
         stream->read(&p.y);
         mPolyBounds.push_back(p);
      }

      return size;
   }


   Rect computePolyExtents()
   {
      Rect extent(mPolyBounds[0], mPolyBounds[0]);

      for(S32 i = 1; i < mPolyBounds.size(); i++)
         extent.unionPoint(mPolyBounds[i]);

      return extent;
   }
};


////////////////////////////////////////
////////////////////////////////////////

class Polygon : public Polyline
{
protected:
   void processPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize)
   {
      Parent::processPolyBounds(argc, argv, firstCoord, gridSize, false);

      mCentroid = findCentroid(mPolyBounds);
   }


public:
   typedef Polyline Parent;

   Point mCentroid;

   Vector<Point> mPolyFill;      // Triangles used for rendering polygon fill

   F32 mLabelAngle;

   U32 unpackUpdate(GhostConnection *connection, BitStream *stream)
   {
      U32 size = Parent::unpackUpdate(connection, stream);

      if(size)
      {
         Triangulate::Process(mPolyBounds, mPolyFill);
         mCentroid = findCentroid(mPolyBounds);
         mLabelAngle = angleOfLongestSide(mPolyBounds);
      }

      return size;
   }
};


////////////////////////////////////////
////////////////////////////////////////

// This class serves only to provide an implementation of the abstract methods in LuaItem that are common to the polygon classes
class LuaPolygonalGameObject : public GameObject, public Polygon, public LuaItem
{
public:
   S32 getLoc(lua_State *L) { return LuaObject::returnPoint(L, mCentroid); }         // Center of item (returns point)
   S32 getRad(lua_State *L) { return LuaObject::returnInt(L, 0); }                   // Radius of item (returns number)
   S32 getVel(lua_State *L) { return LuaObject::returnPoint(L, Point(0,0)); }        // Velocity of item (returns point)
   S32 getTeamIndx(lua_State *L) { return LuaObject::returnInt(L, getTeam() + 1); }  // Team of item (in bots, teams start with 1)
};


};

#endif   // _POLYGON_H_
