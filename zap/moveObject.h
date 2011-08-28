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

#ifndef _MOVEOBJECT_H_
#define _MOVEOBJECT_H_

#include "Item.h"          // Parent class


namespace Zap
{

class MoveObject : public Item
{
   typedef Item Parent;

private:
   S32 mHitLimit;    // Internal counter for processing collisions

public:
   enum {
      ActualState = 0,
      RenderState,
      LastProcessState,
      MoveStateCount,
   };

protected:
   enum {
      InterpMaxVelocity = 900, // velocity to use to interpolate to proper position
      InterpAcceleration = 1800,
   };

   bool mInterpolating;

public:
   struct MoveState  // need public, not protected, for SpeedZone handling...  TODO: fix this flaw
   {
      Point pos;        // Actual position of the ship/object
      float angle;      // Actual angle of the ship/object
      Point vel;        // Actual velocity of the ship/object
   };
   MoveState mMoveState[MoveStateCount];     // MoveStateCount = 3, as per enum above

   MoveObject(const Point &pos = Point(0,0), float radius = 1, float mass = 1);     // Constructor

   void onAddedToGame(Game *game);
   void idle(GameObject::IdleCallPath path);    // Called from child object idle methods
   void updateInterpolation();
   void updateExtent();

   bool isMoveObject() { return true; }

   Point getRenderPos() const { return mMoveState[RenderState].pos; }
   Point getActualPos() const { return mMoveState[ActualState].pos; }
   Point getRenderVel() const { return mMoveState[RenderState].vel; }
   Point getActualVel() const { return mMoveState[ActualState].vel; }

   void setActualVel(Point vel) { mMoveState[ActualState].vel = vel; }

   virtual void playCollisionSound(U32 stateIndex, MoveObject *moveObjectThatWasHit, F32 velocity);

   void move(F32 time, U32 stateIndex, bool displacing = false, Vector<SafePtr<MoveObject> > = Vector<SafePtr<MoveObject> >());
   bool collide(GameObject *otherObject);

   // CollideTypes is used to improve speed on findFirstCollision
   virtual TestFunc collideTypes() { return (TestFunc)isAnyObjectType; }

   GameObject *findFirstCollision(U32 stateIndex, F32 &collisionTime, Point &collisionPoint);
   void computeCollisionResponseMoveObject(U32 stateIndex, MoveObject *objHit);
   void computeCollisionResponseBarrier(U32 stateIndex, Point &collisionPoint);
   F32 computeMinSeperationTime(U32 stateIndex, MoveObject *contactObject, Point intendedPos);

   virtual bool getCollisionCircle(U32 stateIndex, Point &point, F32 &radius) const
   {
      point = mMoveState[stateIndex].pos;
      radius = mRadius;
      return true;
   }

   // LuaItem interface
   virtual S32 getVel(lua_State *L) { return LuaObject::returnPoint(L, getActualVel()); }
};


class MoveItem : public MoveObject
{
   typedef MoveObject Parent;

private:
   F32 updateTimer;
   Point prevMoveVelocity;

protected:
   enum MaskBits {
      PositionMask     = Parent::FirstFreeMask << 0,     // <-- Indicates position has changed and needs to be updated
      WarpPositionMask = Parent::FirstFreeMask << 1,
      MountMask        = Parent::FirstFreeMask << 2,
      ItemChangedMask  = Parent::FirstFreeMask << 3,
      FirstFreeMask    = Parent::FirstFreeMask << 4
   };

   SafePtr<Ship> mMount;

   bool mIsMounted;
   bool mIsCollideable;
   bool mInitial;       // True on initial unpack, false thereafter

   U16 mItemId;         // Item ID, shared between client and server

   Timer mDroppedTimer;                   // Make flags have a tiny bit of delay before they can be picked up again
   static const U32 DROP_DELAY = 500;     // Time until we can pick the item up after it's dropped (in ms)

public:
   MoveItem(Point p = Point(0,0), bool collideable = false, float radius = 1, float mass = 1);   // Constructor

   void idle(GameObject::IdleCallPath path);

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString(F32 gridSize) const;

   virtual U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   virtual void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void setActualPos(const Point &p);
   void setActualVel(const Point &vel);

   U16 getItemId() { return mItemId; }

   virtual void mountToShip(Ship *theShip);
   void setMountedMask() { setMaskBits(MountMask); }
   void setPositionMask() { setMaskBits(PositionMask); }

   bool isMounted() { return mIsMounted; }
   virtual bool isItemThatMakesYouVisibleWhileCloaked() { return true; }      // HuntersFlagItem overrides to false

   void setCollideable(bool isCollideable) { mIsCollideable = isCollideable; }

   Ship *getMount();
   void dismount();
   void render();

   virtual void renderItem(const Point &pos) = 0;      // Does actual rendering, allowing render() to be generic for all Items

   virtual void onMountDestroyed();
   virtual void onItemDropped();

   bool collide(GameObject *otherObject);

   GameObject *getGameObject() { return this; }

   // LuaItem interface
   virtual S32 isOnShip(lua_State *L) { return returnBool(L, mIsMounted); }                // Is flag being carried by a ship?
   virtual S32 getShip(lua_State *L);

};



////////////////////////////////////////
////////////////////////////////////////

static const S32 AsteroidDesigns = 4;
static const S32 AsteroidPoints = 12;

static const F32 asteroidRenderSize[] = { .8f, .4f, .2f, -1 };      // Must end in -1
static const S32 asteroidRenderSizes = sizeof(asteroidRenderSize) / sizeof(F32) - 1;

static const S32 mSizeIndexLength = sizeof(asteroidRenderSize) / sizeof(F32) - 1;

static const S8 AsteroidCoords[AsteroidDesigns][AsteroidPoints][2] =   // <== Wow!  A 3D array!
{
  { { 80, -43}, { 47, -84 }, { 5, -58 }, { -41, -81 }, { -79, -21 }, { -79, -0 }, { -79, 10 }, { -79, 47 }, { -49, 78 }, { 43, 78 }, { 80, 40 }, { 46, -0 } },
  { { -41, -83 }, { 18, -83 }, { 81, -42 }, { 83, -42 }, { 7, -2 }, { 81, 38 }, { 41, 79 }, { 10, 56 }, { -48, 79 }, { -80, 15 }, { -80, -43 }, { -17, -43 } },
  { { -2, -56 }, { 40, -79 }, { 81, -39 }, { 34, -19 }, { 82, 22 }, { 32, 83 }, { -21, 59 }, { -40, 82 }, { -80, 42 }, { -57, 2 }, { -79, -38 }, { -31, -79 } },
  { { 42, -82 }, { 82, -25 }, { 82, 5 }, { 21, 80 }, { -19, 80 }, { -8, 5 }, { -48, 79 }, { -79, 16 }, { -39, -4 }, { -79, -21 }, { -19, -82 }, { -4, -82 } },
};


class Asteroid : public MoveItem
{

typedef MoveItem Parent;      // TODO: Should be EditorItem???

private:
   S32 mSizeIndex;
   bool hasExploded;
   S32 mDesign;

public:
   Asteroid();     // Constructor  
   Asteroid *clone() const;

   static const S32 ASTEROID_RADIUS = 89;

   void renderItem(const Point &pos);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool getCollisionCircle(U32 state, Point &center, F32 &radius) const;
   bool collide(GameObject *otherObject);
   void setPosAng(Point pos, F32 ang);

   // Asteroid does not collide to another asteroid
   TestFunc collideTypes() { return (TestFunc)isAsteroidCollideableType; }

   void damageObject(DamageInfo *theInfo);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);
   void onItemExploded(Point pos);

   static U32 getDesignCount() { return AsteroidDesigns; }

   S32 getSizeIndex() { return mSizeIndex; }
   S32 getSizeCount() { return asteroidRenderSizes; }

   TNL_DECLARE_CLASS(Asteroid);

   ///// Editor methods
   const char *getEditorHelpString() { return "Shootable asteroid object.  Just like the arcade game."; }
   const char *getPrettyNamePlural() { return "Asteroids"; }
   const char *getOnDockName() { return "Ast."; }
   const char *getOnScreenName() { return "Asteroid"; }

      //virtual S32 getDockRadius() { return 11; }
   F32 getEditorRadius(F32 currentScale);
   void renderDock();

   ///// Lua interface

   public:
   Asteroid(lua_State *L);    // Constructor

   static const char className[];

   static Lunar<Asteroid>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, AsteroidTypeNumber); }

   S32 getSize(lua_State *L);        // Index of current asteroid size (0 = initial size, 1 = next smaller, 2 = ...) (returns int)
   S32 getSizeCount(lua_State *L);   // Number of indexes of size we can have (returns int)
   void push(lua_State *L) {  Lunar<Asteroid>::push(L, this); }
};


////////////////////////////////////////
////////////////////////////////////////

class Circle : public MoveItem
{

typedef MoveItem Parent;

private:
   bool hasExploded;

public:
   Circle();     // Constructor  
   Circle *clone() const;

   static const S32 CIRCLE_RADIUS = 10;

   void renderItem(const Point &pos);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool collide(GameObject *otherObject);
   void setPosAng(Point pos, F32 ang);

   void idle(GameObject::IdleCallPath path);

   void damageObject(DamageInfo *theInfo);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);
   void onItemExploded(Point pos);

   void playCollisionSound(U32 stateIndex, MoveObject *moveObjectThatWasHit, F32 velocity) { /* Do nothing */ }

   static U32 getDesignCount() { return AsteroidDesigns; }

   TNL_DECLARE_CLASS(Circle);

   ///// Editor methods
   const char *getEditorHelpString() { return "Shootable circle object.  Scary."; }
   const char *getPrettyNamePlural() { return "Circles"; }
   const char *getOnDockName() { return "Circ."; }
   const char *getOnScreenName() { return "Circle"; }

   F32 getEditorRadius(F32 currentScale);
   void renderDock();

   ///// Lua interface

   public:
   Circle(lua_State *L);    // Lua constructor

   static const char className[];

   static Lunar<Circle>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, CircleTypeNumber); }

   void push(lua_State *L) {  Lunar<Circle>::push(L, this); }
};


////////////////////////////////////////
////////////////////////////////////////

class Worm : public MoveItem      // But not an editor object!!  -- should be a Projectile?
{
typedef MoveItem Parent;

public:
   static const S32 WORM_RADIUS = 5;

private:
   bool hasExploded;
   F32 mNextAng;
   Timer mDirTimer;

public:
   Worm();     // Constructor  

   void renderItem(const Point &pos);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool getCollisionCircle(U32 state, Point &center, F32 &radius) const;
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

class TestItem : public MoveItem
{
   typedef MoveItem Parent;

public:
   TestItem();     // Constructor
   TestItem *clone() const;

   static const S32 TEST_ITEM_RADIUS = 60;

   void renderItem(const Point &pos);
   void damageObject(DamageInfo *theInfo);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;

   TNL_DECLARE_CLASS(TestItem);

   ///// Editor methods
   const char *getEditorHelpString() { return "Bouncy object that floats around and gets in the way."; }
   const char *getPrettyNamePlural() { return "TestItems"; }
   const char *getOnDockName() { return "Test"; }
   const char *getOnScreenName() { return "TestItem"; }

   F32 getEditorRadius(F32 currentScale);
   void renderDock();

   ///// Lua Interface

   TestItem(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<TestItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, TestItemTypeNumber); }
   void push(lua_State *L) {  Lunar<TestItem>::push(L, this); }
};


////////////////////////////////////////
////////////////////////////////////////

class ResourceItem : public MoveItem
{
   typedef MoveItem Parent; 

public:
   ResourceItem();      // Constructor
   ResourceItem *clone() const;

   static const S32 RESOURCE_ITEM_RADIUS = 20;

   void renderItem(const Point &pos);
   bool collide(GameObject *hitObject);
   void damageObject(DamageInfo *theInfo);
   void onItemDropped();

   TNL_DECLARE_CLASS(ResourceItem);

   ///// Editor methods
   const char *getEditorHelpString() { return "Small bouncy object; capture one to activate Engineer module"; }
   const char *getPrettyNamePlural() { return "ResourceItems"; }
   const char *getOnDockName() { return "Res."; }
   const char *getOnScreenName() { return "ResourceItem"; }

   void renderDock();

   ///// Lua Interface

   ResourceItem(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<ResourceItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, ResourceItemTypeNumber); }
   void push(lua_State *L) {  Lunar<ResourceItem>::push(L, this); }

};



};

#endif

