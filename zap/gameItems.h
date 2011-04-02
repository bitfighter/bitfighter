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


#ifndef _GAME_ITEMS_H_
#define _GAME_ITEMS_H_

#include "item.h"
#include "luaObject.h"        // For Lua interfaces
#include "gameObjectRender.h"
#include "../glut/glutInclude.h"



namespace Zap
{
class Ship;

class RepairItem : public PickupItem
{
protected:
   typedef PickupItem Parent;

public:
   static const S32 defaultRespawnTime = 20;    // In seconds

   RepairItem(Point p = Point()) : PickupItem(p, 20, defaultRespawnTime * 1000) { /* do nothing */ };   // Constructor
   bool pickup(Ship *theShip);
   void onClientPickup();
   void renderItem(Point pos);

   TNL_DECLARE_CLASS(RepairItem);

   ///// Lua Interface

   RepairItem(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<RepairItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, RepairItemType); }

   S32 isVis(lua_State *L); // Is RepairItem visible? (returns boolean)
   void push(lua_State *L) {  Lunar<RepairItem>::push(L, this); }
};


////////////////////////////////////////
////////////////////////////////////////

class EnergyItem : public PickupItem
{
private:
   typedef PickupItem Parent;

public:
   static const S32 defaultRespawnTime = 20;    // In seconds

   EnergyItem(Point p = Point()) : PickupItem(p, 20, defaultRespawnTime * 1000) { /* do nothing */ };   // Constructor
   bool pickup(Ship *theShip);
   void onClientPickup();
   void renderItem(Point pos);

   TNL_DECLARE_CLASS(EnergyItem);

   ///// Lua Interface

   EnergyItem(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<EnergyItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, EnergyItemType); }

   S32 isVis(lua_State *L); // Is EnergyItem visible? (returns boolean)
   void push(lua_State *L) {  Lunar<EnergyItem>::push(L, this); }
};


////////////////////////////////////////
////////////////////////////////////////

static const S32 AsteroidDesigns = 4;
static const S32 AsteroidPoints = 12;

static const F32 asteroidRenderSize[] = { .8, .4, .2, -1 };      // Must end in -1
static const S32 asteroidRenderSizes = sizeof(asteroidRenderSize) / sizeof(F32) - 1;

static const S32 mSizeIndexLength = sizeof(asteroidRenderSize) / sizeof(F32) - 1;

static const S8 AsteroidCoords[AsteroidDesigns][AsteroidPoints][2] =   // <== Wow!  A 3D array!
{
  { { 80, -43}, { 47, -84 }, { 5, -58 }, { -41, -81 }, { -79, -21 }, { -79, -0 }, { -79, 10 }, { -79, 47 }, { -49, 78 }, { 43, 78 }, { 80, 40 }, { 46, -0 } },
  { { -41, -83 }, { 18, -83 }, { 81, -42 }, { 83, -42 }, { 7, -2 }, { 81, 38 }, { 41, 79 }, { 10, 56 }, { -48, 79 }, { -80, 15 }, { -80, -43 }, { -17, -43 } },
  { { -2, -56 }, { 40, -79 }, { 81, -39 }, { 34, -19 }, { 82, 22 }, { 32, 83 }, { -21, 59 }, { -40, 82 }, { -80, 42 }, { -57, 2 }, { -79, -38 }, { -31, -79 } },
  { { 42, -82 }, { 82, -25 }, { 82, 5 }, { 21, 80 }, { -19, 80 }, { -8, 5 }, { -48, 79 }, { -79, 16 }, { -39, -4 }, { -79, -21 }, { -19, -82 }, { -4, -82 } },
};


class Asteroid : public Item
{

typedef Item Parent;

private:
   S32 mSizeIndex;
   bool hasExploded;
   S32 mDesign;

public:
   Asteroid();     // Constructor  

   static const S32 ASTEROID_RADIUS = 89;

   void renderItem(Point pos);
   bool getCollisionPoly(Vector<Point> &polyPoints);
   bool getCollisionCircle(U32 state, Point &center, F32 &radius);
   bool collide(GameObject *otherObject);
   void setPosAng(Point pos, F32 ang);

   void damageObject(DamageInfo *theInfo);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);
   void emitAsteroidExplosion(Point pos);

   static U32 getDesignCount() { return AsteroidDesigns; }

   S32 getSizeIndex() { return mSizeIndex; }
   S32 getSizeCount() { return asteroidRenderSizes; }

   TNL_DECLARE_CLASS(Asteroid);

   ///// Lua interface

   public:
   Asteroid(lua_State *L);    // Constructor

   static const char className[];

   static Lunar<Asteroid>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, AsteroidType); }

   S32 getSize(lua_State *L);        // Index of current asteroid size (0 = initial size, 1 = next smaller, 2 = ...) (returns int)
   S32 getSizeCount(lua_State *L);   // Number of indexes of size we can have (returns int)
   void push(lua_State *L) {  Lunar<Asteroid>::push(L, this); }
};


////////////////////////////////////////
////////////////////////////////////////

class AsteroidSpawn     // Essentially the same as a FlagSpawn... merge classes?
{
private:
   Point mPos;

public:
   static const S32 defaultRespawnTime = 30;    // in seconds

   AsteroidSpawn(Point pos, S32 delay);  // C++ constructor (no lua constructor)
   Point getPos() { return mPos; }
   Timer timer;
};


////////////////////////////////////////
////////////////////////////////////////

class Worm : public Item
{
typedef Item Parent;

public:
   static const S32 WORM_RADIUS = 5;

private:
   bool hasExploded;
   F32 mNextAng;
   Timer mDirTimer;

public:
   Worm();     // Constructor  

   void renderItem(Point pos);
   bool getCollisionPoly(Vector<Point> &polyPoints);
   bool getCollisionCircle(U32 state, Point &center, F32 &radius);
   bool collide(GameObject *otherObject);
   void setPosAng(Point pos, F32 ang);
   void setNextAng(F32 nextAng) { mNextAng = nextAng; }

   void damageObject(DamageInfo *theInfo);
   void idle(GameObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(Worm);
};


////////////////////////////////////////
////////////////////////////////////////

class TestItem : public Item
{
   typedef Item Parent;

public:
   TestItem();     // Constructor

   static const S32 TEST_ITEM_RADIUS = 60;

   void renderItem(Point pos);
   void damageObject(DamageInfo *theInfo);
   bool getCollisionPoly(Vector<Point> &polyPoints);

   TNL_DECLARE_CLASS(TestItem);

   ///// Lua Interface

   TestItem(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<TestItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, TestItemType); }
   void push(lua_State *L) {  Lunar<TestItem>::push(L, this); }
};


////////////////////////////////////////
////////////////////////////////////////

class ResourceItem : public Item
{
   typedef Item Parent;

public:
   ResourceItem();      // Constructor

   static const S32 RESOURCE_ITEM_RADIUS = 20;

   void renderItem(Point pos);
   bool collide(GameObject *hitObject);
   void damageObject(DamageInfo *theInfo);
   void onItemDropped();

   TNL_DECLARE_CLASS(ResourceItem);

   ///// Lua Interface

   ResourceItem(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<ResourceItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, ResourceItemType); }
   void push(lua_State *L) {  Lunar<ResourceItem>::push(L, this); }

};

};

#endif

