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

#include "gameObject.h" 


namespace Zap
{

class MoveObject : public GameObject
{
   typedef GameObject Parent;

private:
   S32 mHitLimit;    // Internal counter for processing collisions

protected:
   F32 mRadius;
   F32 mMass;

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

   MoveObject(Point pos = Point(0,0), float radius = 1, float mass = 1);

   void onAddedToGame(Game *game);
   void idle(GameObject::IdleCallPath path);    // Called from child object idle methods
   void updateInterpolation();
   void updateExtent();

   bool isMoveObject() { return true; }

   F32 getRadius() { return mRadius; }
   F32 getMass() { return mMass; }
   virtual void setRadius(F32 radius) { mRadius = radius; }
   void setMass(F32 mass) { mMass = mass; }

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
};


};

#endif

