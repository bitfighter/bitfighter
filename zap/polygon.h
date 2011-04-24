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

#include "gameObject.h"

namespace Zap
{

extern S32 gMaxPolygonPoints;

class Polyline
{
public:
   Vector<Point> mPolyBounds;

protected:
   void packUpdate(GhostConnection *connection, BitStream *stream);

   // Read a series of points from a command line, and add them to a Vector of points
   void processPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize, bool allowFirstAndLastPointToBeEqual);

   virtual void processPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize)
   {
      processPolyBounds(argc, argv, firstCoord, gridSize, true);
   }

   U32 unpackUpdate(GhostConnection *connection, BitStream *stream);
   Rect computePolyExtents();
};


////////////////////////////////////////
////////////////////////////////////////

class Polygon : public Polyline
{
protected:
   Vector<Point> mPolyFill;      // Triangles used for rendering polygon fill
   Point mCentroid;
   void processPolyBounds(S32 argc, const char **argv, S32 firstCoord, F32 gridSize);

public:
   typedef Polyline Parent;
   F32 mLabelAngle;

   Point getCentroid() { return mCentroid; }

   Vector<Point> *getPolyFillPoints() { return &mPolyFill; }

   U32 unpackUpdate(GhostConnection *connection, BitStream *stream);
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
