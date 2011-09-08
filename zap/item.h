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

#ifndef _ITEM_H_
#define _ITEM_H_

#include "gameObject.h"       // Parent class
#include "EditorObject.h"     // Parent class
#include "luaObject.h"        // Parent class

#include "Timer.h"

namespace Zap
{

// A note on terminology here: an "object" is any game object, whereas an "item" is a point object that the player will interact with
// Item is now parent class of MoveItem, EngineeredItem, PickupItem

class Item : public GameObject, public EditorItem, public LuaItem
{
   typedef GameObject Parent;

protected:
   F32 mRadius;
   F32 mMass;

   enum MaskBits {
      InitialMask = Parent::FirstFreeMask << 0,
      ItemChangedMask = Parent::FirstFreeMask << 1,
      ExplodedMask    = Parent::FirstFreeMask << 2,
      FirstFreeMask   = Parent::FirstFreeMask << 3
   };

public:
   Item(const Point &pos = Point(0,0), F32 radius = 1, F32 mass = 1);      // Constructor

   virtual bool processArguments(S32 argc, const char **argv, Game *game);

   virtual U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   virtual void unpackUpdate(GhostConnection *connection, BitStream *stream);

   F32 getRadius() { return mRadius; }
   virtual void setRadius(F32 radius) { mRadius = radius; }

   F32 getMass() { return mMass; }
   void setMass(F32 mass) { mMass = mass; }

   virtual void renderItem(const Point &pos);      // Generic renderer -- will be overridden
   virtual void render();

   // EditorItem interface
   virtual void renderEditor(F32 currentScale);
   virtual F32 getEditorRadius(F32 currentScale);
   virtual string toString(F32 gridSize) const;

   // LuaItem interface
   virtual S32 getLoc(lua_State *L) { return LuaObject::returnPoint(L, getActualPos()); }
   virtual S32 getRad(lua_State *L) { return LuaObject::returnFloat(L, getRadius()); }
   virtual S32 getVel(lua_State *L) { return LuaObject::returnPoint(L, Point(0,0)); }
   virtual S32 getTeamIndx(lua_State *L) { return TEAM_NEUTRAL + 1; }              // Can be overridden for team items
   virtual S32 isInCaptureZone(lua_State *L) { return returnBool(L, false); }      // Non-moving item is never in capture zone, even if it is!
   virtual S32 isOnShip(lua_State *L) { return returnBool(L, false); }             // Is item being carried by a ship? NO!
   virtual S32 getCaptureZone(lua_State *L) { return returnNil(L); }
   virtual S32 getShip(lua_State *L) { return returnNil(L); }
   virtual GameObject *getGameObject() { return this; }          // Return the underlying GameObject
};


////////////////////////////////////////
////////////////////////////////////////

class Reactor : public Item
{

typedef Item Parent;

private:
   bool hasExploded;
   U32 mHitPoints;

public:
   Reactor();     // Constructor  
   Reactor *clone() const;

   static const S32 REACTOR_RADIUS = 10;

   void renderItem(const Point &pos);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool getCollisionCircle(U32 state, Point &center, F32 &radius) const;
   bool getCollisionRect(U32 state, Rect &rect) const;
   bool collide(GameObject *otherObject);

   F32 getReactorRadius() const;

   void damageObject(DamageInfo *theInfo);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);
   void onItemExploded(Point pos);

   void setRadius(F32 radius);

   TNL_DECLARE_CLASS(Reactor);

   ///// Editor methods
   const char *getEditorHelpString() { return "Reactor.  Destroy to score."; }
   const char *getPrettyNamePlural() { return "Reactors"; }
   const char *getOnDockName() { return "Rctr"; }
   const char *getOnScreenName() { return "Reactor"; }

   F32 getEditorRadius(F32 currentScale);
   void renderDock();

   ///// Lua interface
public:
   Reactor(lua_State *L);    // Constructor

   static const char className[];

   static Lunar<Reactor>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, ReactorTypeNumber); }

   S32 getHitPoints(lua_State *L);   // Index of current asteroid size (0 = initial size, 1 = next smaller, 2 = ...) (returns int)
   void push(lua_State *L) {  Lunar<Reactor>::push(L, this); }
};



};

#endif


