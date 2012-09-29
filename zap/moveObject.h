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

#include "item.h"          // Parent class
#include "LuaWrapper.h"

namespace Zap
{

enum MoveStateNames {
   ActualState = 0,
   RenderState,
   LastProcessState,
   MoveStateCount,
};


class MoveStates
{
private:
   struct MoveState  // need public, not protected, for SpeedZone handling...  TODO: fix this flaw
   {
      Point pos;        // Actual position of the ship/object
      float angle;      // Actual angle of the ship/object
      Point vel;        // Actual velocity of the ship/object
   };

   MoveState mMoveState[MoveStateCount];     // MoveStateCount = 3, as per enum above

public:
   Point getPos(S32 state) const;
   void setPos(S32 state, const Point &pos);

   Point getVel(S32 state) const;
   void setVel(S32 state, const Point &vel);

   F32 getAngle(S32 state) const;
   void setAngle(S32 state, F32 angle);
};


////////////////////////////////////////
////////////////////////////////////////

class MoveObject : public Item
{
   typedef Item Parent;

private:
   S32 mHitLimit;             // Internal counter for processing collisions
   MoveStates mMoveStates;

protected:
   enum {
      InterpMaxVelocity = 900, // velocity to use to interpolate to proper position
      InterpAcceleration = 1800,
   };

   bool mInterpolating;
   F32 mMass;

public:
   MoveObject(const Point &pos = Point(0,0), float radius = 1, float mass = 1);     // Constructor
   ~MoveObject();                                                                   // Destructor

   void onAddedToGame(Game *game);
   void idle(BfObject::IdleCallPath path);    // Called from child object idle methods
   void updateInterpolation();
   virtual Rect calcExtents();

   bool isMoveObject();

   Point getRenderPos() const;
   Point getActualPos() const;
   Point getRenderVel() const;      // Distance/sec
   Point getActualVel() const;      // Distance/sec

   F32 getRenderAngle() const;
   F32 getActualAngle() const;
   F32 getLastProcessStateAngle() const;

   // Because MoveObjects have multiple positions (actual, render), we need to implement the following
   // functions differently than most objects do
   Point getPos() const;      // Maps to getActualPos
   Point getVel() const;      // Maps to getActualVel

   Point getPos(S32 stateIndex) const;
   Point getVel(S32 stateIndex) const;
   F32 getAngle(S32 stateIndex) const;

   void setPos(S32 stateIndex, const Point &pos);
   void setVel(S32 stateIndex, const Point &vel);     // Distance/sec
   void setAngle(S32 stateIndex, F32 angle);

   void copyMoveState(S32 from, S32 to);

   virtual void setActualPos(const Point &pos);
   virtual void setActualVel(const Point &vel);

   void setRenderPos(const Point &pos);
   void setRenderVel(const Point &vel);

   void setRenderAngle(F32 angle);
   void setActualAngle(F32 angle);

   void setPos(const Point &pos);

   void setPosVelAng(const Point &pos, const Point &vel, F32 ang);

   F32 getMass();
   void setMass(F32 mass);

   virtual void playCollisionSound(U32 stateIndex, MoveObject *moveObjectThatWasHit, F32 velocity);

   void move(F32 time, U32 stateIndex, bool displacing = false, Vector<SafePtr<MoveObject> > = Vector<SafePtr<MoveObject> >());
   virtual bool collide(BfObject *otherObject);

   // CollideTypes is used to improve speed on findFirstCollision
   virtual TestFunc collideTypes();

   BfObject *findFirstCollision(U32 stateIndex, F32 &collisionTime, Point &collisionPoint);
   void computeCollisionResponseMoveObject(U32 stateIndex, MoveObject *objHit);
   void computeCollisionResponseBarrier(U32 stateIndex, Point &collisionPoint);
   F32 computeMinSeperationTime(U32 stateIndex, MoveObject *contactObject, Point intendedPos);

   void computeImpulseDirection(DamageInfo *damageInfo);

   virtual bool getCollisionCircle(U32 stateIndex, Point &point, F32 &radius) const;

   virtual void onGeomChanged();

   ///// Lua interface
   LUAW_DECLARE_CLASS(MoveObject);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   // Get/set object's velocity vector
   virtual S32 getVel(lua_State *L);
   virtual S32 setVel(lua_State *L);
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

   Timer mDroppedTimer;                   // Make flags have a tiny bit of delay before they can be picked up again
   static const U32 DROP_DELAY = 500;     // Time until we can pick the item up after it's dropped (in ms)

public:
   MoveItem(Point p = Point(0,0), bool collideable = false, float radius = 1, float mass = 1);   // Constructor
   virtual ~MoveItem();                                                                          // Destructor

   void idle(BfObject::IdleCallPath path);

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString(F32 gridSize) const;

   virtual U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   virtual void unpackUpdate(GhostConnection *connection, BitStream *stream);

   virtual void setActualPos(const Point &pos);
   virtual void setActualVel(const Point &vel);

   virtual void mountToShip(Ship *theShip);
   void setMountedMask();
   void setPositionMask();

   bool isMounted();
   virtual bool isItemThatMakesYouVisibleWhileCloaked();      // NexusFlagItem overrides to false

   void setCollideable(bool isCollideable);

   Ship *getMount();
   void dismount();
   void render();

   virtual void renderItem(const Point &pos);             // Does actual rendering, allowing render() to be generic for all Items
   virtual void renderItemAlpha(const Point &pos, F32 alpha);  // Used for mounted items when cloaked

   virtual void onMountDestroyed();
   virtual void onItemDropped();

   bool collide(BfObject *otherObject);

   ///// LuaItem interface
   //LUAW_DECLARE_CLASS(MoveItem);

   //static const char *luaClassName;
   //static const luaL_reg luaMethods[];
   //static const LuaFunctionProfile functionArgs[];

   virtual S32 isOnShip(lua_State *L);                 // Is flag being carried by a ship?
   virtual S32 getShip(lua_State *L);
};


////////////////////////////////////////
////////////////////////////////////////

static const S32 ASTEROID_DESIGNS = 4;
static const S32 ASTEROID_POINTS = 12;

static const S8 AsteroidCoords[ASTEROID_DESIGNS][ASTEROID_POINTS][2] =   // <== Wow!  A 3D array!
{
  { { 80, -43}, { 47, -84 }, { 5, -58 }, { -41, -81 }, { -79, -21 }, { -79, -0 }, { -79, 10 }, { -79, 47 }, { -49, 78 }, { 43, 78 }, { 80, 40 }, { 46, -0 } },
  { { -41, -83 }, { 18, -83 }, { 81, -42 }, { 83, -42 }, { 7, -2 }, { 81, 38 }, { 41, 79 }, { 10, 56 }, { -48, 79 }, { -80, 15 }, { -80, -43 }, { -17, -43 } },
  { { -2, -56 }, { 40, -79 }, { 81, -39 }, { 34, -19 }, { 82, 22 }, { 32, 83 }, { -21, 59 }, { -40, 82 }, { -80, 42 }, { -57, 2 }, { -79, -38 }, { -31, -79 } },
  { { 42, -82 }, { 82, -25 }, { 82, 5 }, { 21, 80 }, { -19, 80 }, { -8, 5 }, { -48, 79 }, { -79, 16 }, { -39, -4 }, { -79, -21 }, { -19, -82 }, { -4, -82 } },
};


class Asteroid : public MoveItem
{

typedef MoveItem Parent;      // TODO: Should be PointObject???

private:
   S32 mSizeLeft;
   bool hasExploded;
   S32 mDesign;

public:
   Asteroid();       // Constructor  
   ~Asteroid();      // Destructor
   Asteroid *clone() const;

   static F32 getAsteroidRadius(S32 size_left);
   static F32 getAsteroidMass(S32 size_left);

   void renderItem(const Point &pos);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool collide(BfObject *otherObject);
   void setPosAng(Point pos, F32 ang);

   // Asteroid does not collide to another asteroid
   TestFunc collideTypes();

   void damageObject(DamageInfo *theInfo);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);
   void onItemExploded(Point pos);

   bool processArguments(S32 argc2, const char **argv2, Game *game);
   string toString(F32 gridSize) const;

#ifndef ZAP_DEDICATED
private:
   static EditorAttributeMenuUI *mAttributeMenuUI;
public:
   // These four methods are all that's needed to add an editable attribute to a class...
   EditorAttributeMenuUI *getAttributeMenu();
   void startEditingAttrs(EditorAttributeMenuUI *attributeMenu);    // Called when we start editing to get menus populated
   void doneEditingAttrs(EditorAttributeMenuUI *attributeMenu);     // Called when we're done to retrieve values set by the menu

   string getAttributeString();
#endif


   static U32 getDesignCount();

   TNL_DECLARE_CLASS(Asteroid);


   ///// Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   //virtual S32 getDockRadius();
   F32 getEditorRadius(F32 currentScale);
   void renderDock();

   ///// Lua interface
   LUAW_DECLARE_CLASS(Asteroid);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 getSize(lua_State *L);        // Index of current asteroid size (0 = initial size, 1 = next smaller, 2 = ...) (returns int)
   S32 getSizeCount(lua_State *L);   // Number of indexes of size we can have (returns int)
};


////////////////////////////////////////
////////////////////////////////////////

class Circle : public MoveItem
{

typedef MoveItem Parent;

private:
   bool hasExploded;

public:
   Circle();      // Constructor  
   ~Circle();     // Destructor
   Circle *clone() const;

   static const S32 CIRCLE_RADIUS = 10;

   void renderItem(const Point &pos);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool collide(BfObject *otherObject);
   void setPosAng(Point pos, F32 ang);

   void idle(BfObject::IdleCallPath path);

   void damageObject(DamageInfo *theInfo);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);
   void onItemExploded(Point pos);

   void playCollisionSound(U32 stateIndex, MoveObject *moveObjectThatWasHit, F32 velocity);

   static U32 getDesignCount();

   TNL_DECLARE_CLASS(Circle);

   ///// Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   F32 getEditorRadius(F32 currentScale);
   void renderDock();

   ///// Lua interface
   LUAW_DECLARE_CLASS(Circle);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


////////////////////////////////////////
////////////////////////////////////////

class Worm : public PointObject
{
typedef BfObject Parent;

public:
   static const S32 WORM_RADIUS = 5;

private:
   bool hasExploded;
   F32 mAngle;
   Timer mDirTimer;

   static const S32 maxTailLength = 28;

protected:
   enum MaskBits {
      ExplodeOrTailLengthMask = Parent::FirstFreeMask << 0,
      TailPointPartsMask = Parent::FirstFreeMask << 1,  // there are multiple tail parts
      TailPointPartsFullMask = ((1 << maxTailLength) - 1) * TailPointPartsMask,
      FirstFreeMask = TailPointPartsMask << maxTailLength,
   };

   Point mPoints[maxTailLength];
   S32 mHeadIndex;
   S32 mTailLength;


public:
   Worm();     // Constructor  
   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString(F32 gridSize) const;
   const char *getOnScreenName();
   Worm *clone() const;

   void render();
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   void renderDock();

   F32 getEditorRadius(F32 currentScale);

   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool getCollisionCircle(U32 state, Point &center, F32 &radius) const;
   bool collide(BfObject *otherObject);
   void setPosAng(Point pos, F32 ang);

   void damageObject(DamageInfo *theInfo);
   void idle(BfObject::IdleCallPath path);
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
   TestItem();                // Constructor
   ~TestItem();               // Destructor
   TestItem *clone() const;

   // Test methods
   void idle(BfObject::IdleCallPath path);

   static const S32 TEST_ITEM_RADIUS = 60;

   void renderItem(const Point &pos);
   void damageObject(DamageInfo *theInfo);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;


   TNL_DECLARE_CLASS(TestItem);

   ///// Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   F32 getEditorRadius(F32 currentScale);
   void renderDock();

   ///// Lua interface
   LUAW_DECLARE_CLASS(TestItem);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


////////////////////////////////////////
////////////////////////////////////////

class ResourceItem : public MoveItem
{
   typedef MoveItem Parent; 

public:
   ResourceItem();      // Constructor
   ~ResourceItem();     // Destructor

   ResourceItem *clone() const;

   static const S32 RESOURCE_ITEM_RADIUS = 20;

   void renderItem(const Point &pos);
   void renderItemAlpha(const Point &pos, F32 alpha);
   bool collide(BfObject *hitObject);
   void damageObject(DamageInfo *theInfo);
   void onItemDropped();

   TNL_DECLARE_CLASS(ResourceItem);

   ///// Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   void renderDock();

   ///// Lua Interface
   LUAW_DECLARE_CLASS(ResourceItem);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};



};

#endif

