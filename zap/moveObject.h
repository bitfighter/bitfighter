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

//#include "item.h"
#include "gameObject.h" 
#include "luaObject.h"     // For LuaItem class
#include "EditorObject.h"  // For EditorItem class
#include "Timer.h"


namespace Zap
{

////////////////////////////////////////
////////////////////////////////////////

// Parent class of EngineeredObject, PickupItem
class Item : public GameObject, public EditorItem, public LuaItem
{
   typedef GameObject Parent;

protected:
   F32 mRadius;
   F32 mMass;

   enum MaskBits {
      ItemChangedMask = Parent::FirstFreeMask << 0,
      ExplodedMask    = Parent::FirstFreeMask << 1,
      FirstFreeMask   = Parent::FirstFreeMask << 2
   };

public:
   Item(const Point &pos = Point(0,0), F32 radius = 1, F32 mass = 1);      // Constructor

   F32 getRadius() { return mRadius; }
   virtual void setRadius(F32 radius) { mRadius = radius; }

   F32 getMass() { return mMass; }
   void setMass(F32 mass) { mMass = mass; }

   virtual void renderItem(const Point &pos);      // Generic renderer -- will be overridden

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


class GoalZone;

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
      ZoneMask         = Parent::FirstFreeMask << 3,
      ItemChangedMask  = Parent::FirstFreeMask << 4,
      FirstFreeMask    = Parent::FirstFreeMask << 5
   };

   SafePtr<Ship> mMount;
   SafePtr<GoalZone> mZone;

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

   void setZone(GoalZone *theZone);
   GoalZone *getZone();
   bool isInZone() { return mZone.isValid(); }
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
   virtual S32 isInCaptureZone(lua_State *L) { return returnBool(L, mZone.isValid()); }    // Is flag in a team's capture zone?
   virtual S32 isOnShip(lua_State *L) { return returnBool(L, mIsMounted); }                // Is flag being carried by a ship?
   virtual S32 getCaptureZone(lua_State *L);
   virtual S32 getShip(lua_State *L);

};





};

#endif

