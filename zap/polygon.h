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
#include "EditorObject.h"     // For EditorPolygon parentage

namespace Zap
{


// Provide editor related methods to the polygon class
class EditorPolygon : public EditorObject, public GameObject, public LuaItem
{
   virtual void renderItemText(const char *text, S32 offset, F32 currentScale);
   virtual void labelDockItem();

   void addToDock(Game *game, const Point &point);
   virtual void renderDock();
   void highlightDockItem(); 

   // Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement seem more natural.
   Point getInitialPlacementOffset(F32 gridSize);

protected:
      void renderPolyHighlight();

public:
   EditorPolygon() { mGeometry = boost::shared_ptr<Geometry>(new PolygonGeometry); };                             // Constructor
   //EditorPolygon(const EditorPolygon &ep) : EditorObject(ep) { mGeometry = boost::shared_ptr<Geometry>(new PolygonGeometry); };      // Copy constructor

   void newObjectFromDock(F32 gridSize);

   // Item is being actively dragged
   virtual void onItemDragging() { onGeomChanged(); }    // maybe not for polywalls??
   virtual void onGeomChanged() { onPointsChanged(); }   // Tell the geometry that things have changed

   virtual void renderEditor(F32 currentScale);

   /////
   // Former LuaPolygon methods
   // This class serves only to provide an implementation of the abstract methods in LuaItem that are common to the polygon classes
public:
   S32 getLoc(lua_State *L) { return LuaObject::returnPoint(L, getCentroid()); }     // Center of item (returns point)
   S32 getRad(lua_State *L) { return LuaObject::returnInt(L, 0); }                   // Radius of item (returns number)
   S32 getVel(lua_State *L) { return LuaObject::returnPoint(L, Point(0,0)); }        // Velocity of item (returns point)
   S32 getTeamIndx(lua_State *L) { return LuaObject::returnInt(L, getTeam() + 1); }  // Team of item (in bots, teams start with 1)

};

};

#endif   // _POLYGON_H_
