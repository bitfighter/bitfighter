//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo relased for Torque Network Library by GarageGames.com
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

#include "moveObject.h"
#include "gameType.h"
//#include "gameItems.h"
#include "goalZone.h"
#include "GeomUtils.h"
#include "ship.h"
#include "SoundSystem.h"
#include "speedZone.h"
#include "game.h"

#ifndef ZAP_DEDICATED
#include "ClientGame.h"
#include "sparkManager.h"
#include "UI.h" // for extern void glColor
#endif

#include <math.h>

namespace Zap
{

MoveObject::MoveObject(const Point &pos, F32 radius, F32 mass) : Parent(pos, radius, mass)    // Constructor
{
   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].pos = pos;
      mMoveState[i].angle = 0;
   }

   mInterpolating = false;
   mHitLimit = 16;
}


void MoveObject::idle(GameObject::IdleCallPath path)
{
   mHitLimit = 16;      // Reset hit limit
}


void MoveObject::onAddedToGame(Game *game)
{
   Parent::onAddedToGame(game);

#ifndef ZAP_DEDICATED
   if(isGhost())     // Client only
      this->setControllingClient(dynamic_cast<ClientGame *>(game)->getConnectionToServer());
#endif
}

static const float MoveObjectCollisionElasticity = 1.7f;


// Update object's extents in the database
void MoveObject::updateExtent()
{
   Rect r(mMoveState[ActualState].pos, mMoveState[RenderState].pos);
   r.expand(Point(mRadius + 10, mRadius + 10));
   setExtent(r);
}

// Ship movement system
// Identify the several cases in which a ship may be moving:
// if this is a client:
//   Ship controlled by this client.  Pos may have been set to something else by server, leaving renderPos elsewhere
//     all movement updates affect pos

// collision process for ships:
//

//
// ship *theShip;
// F32 time;
// while(time > 0)
// {
//    ObjHit = findFirstCollision(theShip);
//    advanceToCollision();
//    if(velocitiesColliding)
//    {
//       doCollisionResponse();
//    }
//    else
//    {
//       computeMinimumSeperationTime(ObjHit);
//       displaceObject(ObjHit, seperationTime);
//    }
// }
//
// displaceObject(Object, time)
// {
//    while(time > 0)
//    {
//       ObjHit = findFirstCollision();
//       advanceToCollision();
//       if(velocitiesColliding)
//       {
//          doCollisionResponse();
//          return;
//       }
//       else
//       {
//          computeMinimumSeperationTime(ObjHit);
//          displaceObject(ObjHit, seperationTime);
//       }
//    }
// }


// See http://flipcode.com/archives/Theory_Practice-Issue_01_Collision_Detection.shtml --> Example 1  May or may not be relevant
F32 MoveObject::computeMinSeperationTime(U32 stateIndex, MoveObject *contactShip, Point intendedPos)
{
   F32 myRadius;
   F32 contactShipRadius;

   Point myPos;
   Point contactShipPos;

   getCollisionCircle(stateIndex, myPos, myRadius);   // getCollisionCircle sets myPos and myRadius

   contactShip->getCollisionCircle(stateIndex, contactShipPos, contactShipRadius);

   // Find out if either of the colliding objects uses collisionPolys or not
   //Vector<Point> dummy;
   //F32 fixfact = (getCollisionPoly(dummy) || contactShip->getCollisionPoly(dummy)) ? 0 : 1;

   Point v = contactShip->mMoveState[stateIndex].vel;
   Point posDelta = contactShipPos - intendedPos;

   F32 R = myRadius + contactShipRadius;

   F32 a = v.dot(v);
   F32 b = 2 * v.dot(posDelta);
   F32 c = posDelta.dot(posDelta) - R * R;

   F32 t;

   bool result = FindLowestRootInInterval(a, b, c, 100000, t);

   return result ? t : -1;
}

const F32 moveTimeEpsilon = 0.000001f;
const F32 velocityEpsilon = 0.00001f;

// Apply mMoveState info to an object to compute it's new position.  Used for ships et. al.
// isBeingDisplaced is true when the object is being pushed by something else, which will only happen in a collision
// Remember: stateIndex will be one of 0-ActualState, 1-RenderState, or 2-LastProcessState
void MoveObject::move(F32 moveTime, U32 stateIndex, bool isBeingDisplaced, Vector<SafePtr<MoveObject> > displacerList)
{
   U32 tryCount = 0;
   Vector<SafePtr<GameObject> > disabledList;

   while(moveTime > moveTimeEpsilon && tryCount < 8)     // moveTimeEpsilon is a very short, but non-zero, bit of time
   {
      tryCount++;

      // Ignore tiny movements unless we're processing a collision
      if(!isBeingDisplaced && mMoveState[stateIndex].vel.len() < velocityEpsilon)
         break;

      F32 collisionTime = moveTime;
      Point collisionPoint;

      GameObject *objectHit = findFirstCollision(stateIndex, collisionTime, collisionPoint);
      if(!objectHit)    // No collision (or if isBeingDisplaced is true, we haven't been pushed into another object)
      {
         mMoveState[stateIndex].pos += mMoveState[stateIndex].vel * moveTime;    // Move to desired destination
         break;
      }

      // Collision!  Advance to the point of collision
      mMoveState[stateIndex].pos += mMoveState[stateIndex].vel * collisionTime;

      if(objectHit->isMoveObject())     // Collided with a MoveObject
      {
         MoveObject *moveObjectThatWasHit = (MoveObject *) objectHit;  

         Point velDelta = moveObjectThatWasHit->mMoveState[stateIndex].vel - mMoveState[stateIndex].vel;
         Point posDelta = moveObjectThatWasHit->mMoveState[stateIndex].pos - mMoveState[stateIndex].pos;

         // Prevent infinite loops with a series of objects trying to displace each other forever
         if(isBeingDisplaced)
         {
            bool hit = false;
            for(S32 i = 0; i < displacerList.size(); i++)
               if(moveObjectThatWasHit == displacerList[i])
                 hit = true;
            if(hit) break;
         }
 
         if(posDelta.dot(velDelta) < 0)   // moveObjectThatWasHit is closing faster than we are ???
         {
            computeCollisionResponseMoveObject(stateIndex, moveObjectThatWasHit);
            if(isBeingDisplaced)
               break;
         }
         else                            // We're moving faster than the object we hit (I think)
         {
            Point intendedPos = mMoveState[stateIndex].pos + mMoveState[stateIndex].vel * moveTime;

            F32 displaceEpsilon = 0.002f;
            F32 t = computeMinSeperationTime(stateIndex, moveObjectThatWasHit, intendedPos);
            if(t <= 0)
               break;   // Some kind of math error, couldn't find result: stop simulating this ship

            // Note that we could end up with an infinite feedback loop here, if, for some reason, two objects keep trying to displace
            // one another, as this will just recurse deeper and deeper.

            displacerList.push_back(this);

            // Only try a limited number of times to avoid dragging the game under the dark waves of infinity
            if(mHitLimit > 0) 
            {
               // Move the displaced object a tiny bit, true -> isBeingDisplaced
               moveObjectThatWasHit->move(t + displaceEpsilon, stateIndex, true, displacerList); 
               mHitLimit--;
            }
         }
      }
      else if(isCollideableType(objectHit->getObjectTypeNumber()))
      {
         computeCollisionResponseBarrier(stateIndex, collisionPoint);
         //moveTime = 0;
      }
      else if(objectHit->getObjectTypeNumber() == SpeedZoneTypeNumber)
      {
         SpeedZone *speedZone = dynamic_cast<SpeedZone *>(objectHit);
         if(speedZone)
         {
            speedZone->collided(this, stateIndex);
         }
         disabledList.push_back(objectHit);
         objectHit->disableCollision();
         tryCount--;   // SpeedZone don't count as tryCount
      }
      moveTime -= collisionTime;
   }
   for(S32 i = 0; i < disabledList.size(); i++)   // enable any disabled collision
      if(disabledList[i].isValid())
         disabledList[i]->enableCollision();

   if(tryCount == 8)
      mMoveState[stateIndex].vel.set(0,0); // prevents some overload by not trying to move anymore
}


bool MoveObject::collide(GameObject *otherObject)
{
   return true;
}

GameObject *MoveObject::findFirstCollision(U32 stateIndex, F32 &collisionTime, Point &collisionPoint)
{
   // Check for collisions against other objects
   Point delta = mMoveState[stateIndex].vel * collisionTime;

   Rect queryRect(mMoveState[stateIndex].pos, mMoveState[stateIndex].pos + delta);
   queryRect.expand(Point(mRadius, mRadius));

   fillVector.clear();

   // Free CPU for asteroids
   if (dynamic_cast<Asteroid *>(this))
      findObjects((TestFunc)isAsteroidCollideableType, fillVector, queryRect);
   else
      findObjects((TestFunc)isAnyObjectType, fillVector, queryRect);

   F32 collisionFraction;

   GameObject *collisionObject = NULL;
   Vector<Point> poly;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      GameObject *foundObject = dynamic_cast<GameObject *>(fillVector[i]);

      if(!foundObject->isCollisionEnabled())
         continue;

      poly.clear();

      if(foundObject->getCollisionPoly(poly))
      {
         Point cp;

         if(PolygonSweptCircleIntersect(&poly[0], poly.size(), mMoveState[stateIndex].pos,
                                        delta, mRadius, cp, collisionFraction))
         {
            if(cp != mMoveState[stateIndex].pos)   // avoid getting stuck inside polygon wall
            {
               bool collide1 = collide(foundObject);
               bool collide2 = foundObject->collide(this);

               if(!(collide1 && collide2))
                  continue;

               collisionPoint = cp;
               delta *= collisionFraction;
               collisionTime *= collisionFraction;
               collisionObject = foundObject;

               if(!collisionTime)
                  break;
            }
         }
      }
      else if(foundObject->isMoveObject())
      {
         MoveObject *otherShip = (MoveObject *) foundObject;

         F32 myRadius;
         F32 otherRadius;
         Point myPos;
         Point shipPos;

         getCollisionCircle(stateIndex, myPos, myRadius);
         otherShip->getCollisionCircle(stateIndex, shipPos, otherRadius);

         Point v = mMoveState[stateIndex].vel;
         Point p = myPos - shipPos;

         if(v.dot(p) < 0)
         {
            F32 R = myRadius + otherRadius;
            if(p.len() <= R)
            {
               bool collide1 = collide(otherShip);
               bool collide2 = otherShip->collide(this);

               if(!(collide1 && collide2))
                  continue;

               collisionTime = 0;
               collisionObject = foundObject;
               delta.set(0,0);
            }
            else
            {
               F32 a = v.dot(v);
               F32 b = 2 * p.dot(v);
               F32 c = p.dot(p) - R * R;
               F32 t;
               if(FindLowestRootInInterval(a, b, c, collisionTime, t))
               {
                  bool collide1 = collide(otherShip);
                  bool collide2 = otherShip->collide(this);

                  if(!(collide1 && collide2))
                     continue;

                  collisionTime = t;
                  collisionObject = foundObject;
                  delta = mMoveState[stateIndex].vel * collisionTime;
               }
            }
         }
      }
   }
   return collisionObject;
}


// Collided with BarrierType, EngineeredType, or ForceFieldType.  What's the response?
void MoveObject::computeCollisionResponseBarrier(U32 stateIndex, Point &collisionPoint)
{
   // Reflect the velocity along the collision point
   Point normal = mMoveState[stateIndex].pos - collisionPoint;
   normal.normalize();

   mMoveState[stateIndex].vel -= normal * MoveObjectCollisionElasticity * normal.dot(mMoveState[stateIndex].vel);

#ifndef ZAP_DEDICATED
   // Emit some bump particles on client
   if(isGhost())     // i.e. on client side
   {
      F32 scale = normal.dot(mMoveState[stateIndex].vel) * 0.01f;
      if(scale > 0.5f)
      {
         // Make a noise...
         SoundSystem::playSoundEffect(SFXBounceWall, collisionPoint, Point(), getMin(1.0f, scale - 0.25f));

         Color bumpC(scale/3, scale/3, scale);

         for(S32 i=0; i<4*pow((F32)scale, 0.5f); i++)
         {
            Point chaos(TNL::Random::readF(), TNL::Random::readF());
            chaos *= scale + 1;

            if(TNL::Random::readF() > 0.5)
               FXManager::emitSpark(collisionPoint, normal * chaos.len() + Point(normal.y, -normal.x)*scale*5  + chaos + mMoveState[stateIndex].vel*0.05f, bumpC);

            if(TNL::Random::readF() > 0.5)
               FXManager::emitSpark(collisionPoint, normal * chaos.len() + Point(normal.y, -normal.x)*scale*-5 + chaos + mMoveState[stateIndex].vel*0.05f, bumpC);
         }
      }
   }
#endif
}


// Runs on both client and server side...
void MoveObject::computeCollisionResponseMoveObject(U32 stateIndex, MoveObject *moveObjectThatWasHit)
{
   // collisionVector is simply a line from the center of moveObjectThatWasHit to the center of this
   Point collisionVector = moveObjectThatWasHit->mMoveState[stateIndex].pos -mMoveState[stateIndex].pos;

   collisionVector.normalize();
   // F32 m1 = getMass();             <-- May be useful in future
   // F32 m2 = moveObjectThatWasHit->getMass();

   bool moveObjectThatWasHitWasNotMoving = (moveObjectThatWasHit->mMoveState[stateIndex].vel.lenSquared() == 0.0f);
      
   // Initial velocities projected onto collisionVector
   F32 v1i = mMoveState[stateIndex].vel.dot(collisionVector);
   F32 v2i = moveObjectThatWasHit->mMoveState[stateIndex].vel.dot(collisionVector);

   F32 v1f, v2f;     // Final velocities

   F32 e = 0.9f;     // Elasticity, I think

   // Could incorporate m1 & m2 here in future
   v2f = ( e * (v1i - v2i) + v1i + v2i) / 2;
   v1f = ( v1i + v2i - v2f);

   mMoveState[stateIndex].vel += collisionVector * (v1f - v1i);
   moveObjectThatWasHit->mMoveState[stateIndex].vel += collisionVector * (v2f - v2i);

   if(!isGhost())    // Server only
   {
      // Check for asteroids hitting a ship
      Ship *ship = dynamic_cast<Ship *>(moveObjectThatWasHit);
      Asteroid *asteroid = dynamic_cast<Asteroid *>(this);
 
      if(!ship)
      {
         // Since asteroids and ships are both MoveObjects, we'll also check to see if ship hit an asteroid
         ship = dynamic_cast<Ship *>(this);
         asteroid = dynamic_cast<Asteroid *>(moveObjectThatWasHit);
      }

      if(ship && asteroid)      // Collided!  Do some damage!  Bring it on!
      {
         DamageInfo theInfo;
         theInfo.collisionPoint = mMoveState[ActualState].pos;
         theInfo.damageAmount = 1.0f;     // Kill ship
         theInfo.damageType = DamageTypePoint;
         theInfo.damagingObject = asteroid;
         theInfo.impulseVector = mMoveState[ActualState].vel;

         ship->damageObject(&theInfo);
      }
   }
#ifndef ZAP_DEDICATED
   else     // Client only
   {
      playCollisionSound(stateIndex, moveObjectThatWasHit, v1i);

      MoveItem *item = dynamic_cast<MoveItem *>(moveObjectThatWasHit);
      GameType *gameType = gClientGame->getGameType();

      if(item && gameType)
         gameType->c2sResendItemStatus(item->getItemId());
   }
#endif
}


void MoveObject::playCollisionSound(U32 stateIndex, MoveObject *moveObjectThatWasHit, F32 velocity)
{
   if(velocity > 0.25)    // Make sound if the objects are moving fast enough
      SoundSystem::playSoundEffect(SFXBounceObject, moveObjectThatWasHit->mMoveState[stateIndex].pos, Point());
}


void MoveObject::updateInterpolation()
{
   U32 deltaT = mCurrentMove.time;
   {
      mMoveState[RenderState].angle = mMoveState[ActualState].angle;

      if(mInterpolating)
      {
         // first step is to constrain the render velocity to
         // the vector of difference between the current position and
         // the actual position.
         // we can also clamp to zero, the actual velocity, or the
         // render velocity, depending on which one is best.

         Point deltaP = mMoveState[ActualState].pos - mMoveState[RenderState].pos;
         F32 distance = deltaP.len();

         if(!distance)
            goto interpDone;

         deltaP.normalize();
         F32 rvel = deltaP.dot(mMoveState[RenderState].vel);
         F32 avel = deltaP.dot(mMoveState[ActualState].vel);

         if(rvel < avel)
            rvel = avel;
         if(rvel < 0)
            rvel = 0;

         bool hit = true;
         float time = deltaT * 0.001f;
         if(rvel * time > distance)
            goto interpDone;

         float requestVel = distance / time;
         float interpMaxVel = InterpMaxVelocity;
         float currentActualVelocity = mMoveState[ActualState].vel.len();
         if(interpMaxVel < currentActualVelocity)
            interpMaxVel = currentActualVelocity;
         if(requestVel > interpMaxVel)
         {
            hit = false;
            requestVel = interpMaxVel;
         }
         F32 a = (requestVel - rvel) / time;
         if(a > InterpAcceleration)
         {
            a = InterpAcceleration;
            hit = false;
         }

         if(hit)
            goto interpDone;

         rvel += a * time;
         mMoveState[RenderState].vel = deltaP * rvel;
         mMoveState[RenderState].pos += mMoveState[RenderState].vel * time;
      }
      else
      {
   interpDone:
         mInterpolating = false;
         mMoveState[RenderState] = mMoveState[ActualState];
      }
   }
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
MoveItem::MoveItem(Point p, bool collideable, float radius, float mass) : MoveObject(p, radius, mass)
{
   mIsMounted = false;
   mIsCollideable = collideable;
   mInitial = false;

   updateTimer = 0;
}


// Server only
bool MoveItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 2)
      return false;
   else if(!Parent::processArguments(argc, argv, game))
      return false;

   for(U32 i = 0; i < MoveStateCount; i++)
      mMoveState[i].pos = getVert(0);

   updateExtent();

   return true;
}


// Server only
string MoveItem::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + geomToString(gridSize);
}


// Client only, in-game
void MoveItem::render()
{
   // If the item is mounted, renderItem will be called from the ship it is mounted to
   if(mIsMounted)
      return;

   renderItem(mMoveState[RenderState].pos);
}


// Runs on both client and server, comes from collision() on the server and the colliding client, and from
// unpackUpdate() in the case of all clients
//
// theShip could be NULL here, and this could still be legit (e.g. flag is in scope, and ship is out of scope)
void MoveItem::mountToShip(Ship *theShip)     
{
   TNLAssert(isGhost() || isInDatabase(), "Error, mount item not in database.");

   if(mMount.isValid() && mMount == theShip)    // Already mounted on ship!  Nothing to do!
      return;

   if(mMount.isValid())                         // Mounted on something else; dismount!
      dismount();

   mMount = theShip;
   if(theShip)
      theShip->mMountedItems.push_back(this);

   mIsMounted = true;
   setMaskBits(MountMask);
}


void MoveItem::onMountDestroyed()
{
   dismount();
}


// Runs on client & server, via different code paths
void MoveItem::onItemDropped()
{
   if(!getGame())    // Can happen on game startup
      return;

   GameType *gt = getGame()->getGameType();
   if(!gt || !mMount.isValid())
      return;

   if(!isGhost())    // Server only; on client calls onItemDropped from dismount
   {
      gt->itemDropped(mMount, this);
      dismount();
   }

   mDroppedTimer.reset(DROP_DELAY);
}


// Client & server, called via different paths
void MoveItem::dismount()
{
   if(mMount.isValid())      // Mount could be null if mount is out of scope, but is dropping an always-in-scope item
   {
      for(S32 i = 0; i < mMount->mMountedItems.size(); i++)
         if(mMount->mMountedItems[i].getPointer() == this)
         {
            mMount->mMountedItems.erase(i);     // Remove mounted item from our mount's list of mounted things
            break;
         }
   }

   if(isGhost())     // Client only; on server, we came from onItemDropped()
      onItemDropped();

   mMount = NULL;
   mIsMounted = false;
   setMaskBits(MountMask | PositionMask);    // Sending position fixes the super annoying "flag that can't be picked up" bug
}


void MoveItem::setActualPos(const Point &p)
{
   mMoveState[ActualState].pos = p;
   mMoveState[ActualState].vel.set(0,0);
   setMaskBits(WarpPositionMask | PositionMask);
}


void MoveItem::setActualVel(const Point &vel)
{
   mMoveState[ActualState].vel = vel;
   setMaskBits(WarpPositionMask | PositionMask);
}


Ship *MoveItem::getMount()
{
   return mMount;
}


void MoveItem::setZone(GoalZone *theZone)
{
   // If the item on which we're setting the zone is a flag (which, at this point, it always will be),
   // we want to make sure to update the zone itself.  This is mostly a convenience for robots searching
   // for objects that meet certain criteria, such as for zones that contain a flag.
   FlagItem *flag = dynamic_cast<FlagItem *>(this);

   if(flag)
   {
      GoalZone *z = ((theZone == NULL) ? flag->getZone() : theZone);

      if(z)
         z->mHasFlag = ((theZone == NULL) ? false : true );
   }

   // Now we can get around to setting the zone, like we came here to do
   mZone = theZone;
   setMaskBits(ZoneMask);
}


GoalZone *MoveItem::getZone()
{
	return mZone;
}


void MoveItem::idle(GameObject::IdleCallPath path)
{
   if(!isInDatabase())
      return;

   Parent::idle(path);

   if(mIsMounted)    // Item is mounted on something else
   {
      if(mMount.isNull() || mMount->hasExploded)
      {
         if(!isGhost())    // Server only
            dismount();
      }
      else
      {
         mMoveState[RenderState].pos = mMount->getRenderPos();
         mMoveState[ActualState].pos = mMount->getActualPos();
      }
   }
   else              // Not mounted
   {
      float time = mCurrentMove.time * 0.001f;
      move(time, ActualState, false);
      if(path == GameObject::ServerIdleMainLoop)
      {
         // Only update if it's actually moving...
         if(mMoveState[ActualState].vel.lenSquared() != 0)
         {
            // Update less often on slow moving item, more often on fast moving item, and update when we change velocity.
            // Update at most every 5 seconds.
            updateTimer -= (mMoveState[ActualState].vel.len() + 20) * time;
            if(updateTimer < 0 || mMoveState[ActualState].vel.distSquared(prevMoveVelocity) > 100)
            {
               setMaskBits(PositionMask);
               updateTimer = 100;
               prevMoveVelocity = mMoveState[ActualState].vel;
            }
         }
         else if(prevMoveVelocity.lenSquared() != 0)
         {
            setMaskBits(PositionMask);  // update to client that this item is no longer moving.
            prevMoveVelocity.set(0,0);
         }

         mMoveState[RenderState] = mMoveState[ActualState];

      }
      else
         updateInterpolation();
   }
   updateExtent();

   // Server only...
   U32 deltaT = mCurrentMove.time;
   mDroppedTimer.update(deltaT);
}


static const S32 VEL_POINT_SEND_BITS = 511;     // 511 = 2^9 - 1, the biggest int we can pack into 9 bits.

U32 MoveItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = 0;
   if(stream->writeFlag(updateMask & InitialMask))
   {
      // Send id in inital packet
      stream->writeRangedU32(mItemId, 0, U16_MAX);
   }
   if(stream->writeFlag(updateMask & PositionMask))
   {
      ((GameConnection *) connection)->writeCompressedPoint(mMoveState[ActualState].pos, stream);
      writeCompressedVelocity(mMoveState[ActualState].vel, VEL_POINT_SEND_BITS, stream);      
      stream->writeFlag(updateMask & WarpPositionMask);
   }
   if(stream->writeFlag(updateMask & MountMask) && stream->writeFlag(mIsMounted))      // mIsMounted gets written iff MountMask is set  
   {
      S32 index = connection->getGhostIndex(mMount);     // Index of ship with item mounted

      if(stream->writeFlag(index != -1))                 // True if some ship has item, false if nothing is mounted
         stream->writeInt(index, GhostConnection::GhostIdBitSize);
      else
         retMask |= MountMask;
   }
   if(stream->writeFlag(updateMask & ZoneMask))
   {
      if(mZone.isValid())
      {
         S32 index = connection->getGhostIndex(mZone);
         if(stream->writeFlag(index != -1))
            stream->writeInt(index, GhostConnection::GhostIdBitSize);
         else
            retMask |= ZoneMask;
      }
      else
         stream->writeFlag(false);
   }
   return retMask;
}


void MoveItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool interpolate = false;
   bool positionChanged = false;

   mInitial = stream->readFlag();

   if(mInitial)     // InitialMask
   {
      mItemId = stream->readRangedU32(0, U16_MAX);
   }

   if(stream->readFlag())     // PositionMask
   {
      ((GameConnection *) connection)->readCompressedPoint(mMoveState[ActualState].pos, stream);
      readCompressedVelocity(mMoveState[ActualState].vel, VEL_POINT_SEND_BITS, stream);   
      positionChanged = true;
      interpolate = !stream->readFlag();
   }

   if(stream->readFlag())     // MountMask
   {
      bool isMounted = stream->readFlag();
      if(isMounted)
      {
         Ship *theShip = NULL;
         
         if(stream->readFlag())
            theShip = dynamic_cast<Ship *>(connection->resolveGhost(stream->readInt(GhostConnection::GhostIdBitSize)));

         mountToShip(theShip);
      }
      else
         dismount();
   }

   if(stream->readFlag())     // ZoneMask
   {
      bool hasZone = stream->readFlag();
      if(hasZone)
         mZone = (GoalZone *) connection->resolveGhost(stream->readInt(GhostConnection::GhostIdBitSize));
      else
         mZone = NULL;
   }

   if(positionChanged)
   {
      if(interpolate)
      {
         mInterpolating = true;
         move(connection->getOneWayTime() * 0.001f, ActualState, false);
      }
      else
      {
         mInterpolating = false;
         mMoveState[RenderState] = mMoveState[ActualState];
      }
   }
}


bool MoveItem::collide(GameObject *otherObject)
{
   return mIsCollideable && !mIsMounted;
}


S32 MoveItem::getCaptureZone(lua_State *L) 
{ 
   if(mZone.isValid()) 
   {
      mZone->push(L); 
      return 1;
   } 
   else 
      return returnNil(L); 
}


S32 MoveItem::getShip(lua_State *L) 
{ 
   if(mMount.isValid()) 
   {
      mMount->push(L); 
      return 1;
   } 
   else 
      return returnNil(L); 
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
AsteroidSpawn::AsteroidSpawn(const Point &pos, S32 time) : Parent(pos, time)
{
   mObjectTypeNumber = AsteroidSpawnTypeNumber;
}


AsteroidSpawn::~AsteroidSpawn()
{
   // Do nothing
}


AsteroidSpawn *AsteroidSpawn::clone() const
{
   return new AsteroidSpawn(*this);
}


void AsteroidSpawn::spawn(Game *game, const Point &pos)
{
   Asteroid *asteroid = dynamic_cast<Asteroid *>(TNL::Object::create("Asteroid"));   // Create a new asteroid

   F32 ang = TNL::Random::readF() * Float2Pi;

   asteroid->setPosAng(pos, ang);

   asteroid->addToGame(game, game->getGameObjDatabase());              // And add it to the list of game objects
}


static void renderAsteroidSpawn(const Point &pos)
{
#ifndef ZAP_DEDICATED
   F32 scale = 0.8f;
   static const Point p(0,0);

   glPushMatrix();
      glTranslatef(pos.x, pos.y, 0);
      glScalef(scale, scale, 1);
      renderAsteroid(p, 2, .1f);

      glColor(Colors::white);
      drawCircle(p, 13);
   glPopMatrix();  
#endif
}


void AsteroidSpawn::renderEditor(F32 currentScale)
{
#ifndef ZAP_DEDICATED
   Point pos = getVert(0);

   glPushMatrix();
      glTranslate(pos);
      glScale(1/currentScale);    // Make item draw at constant size, regardless of zoom
      renderAsteroidSpawn(Point(0,0));
   glPopMatrix();   
#endif
}


void AsteroidSpawn::renderDock()
{
   renderAsteroidSpawn(getVert(0));
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
CircleSpawn::CircleSpawn(const Point &pos, S32 time) : Parent(pos, time)
{
   mObjectTypeNumber = CircleSpawnTypeNumber;
}


CircleSpawn *CircleSpawn::clone() const
{
   return new CircleSpawn(*this);
}


void CircleSpawn::spawn(Game *game, const Point &pos)
{
   for(S32 i = 0; i < 10; i++)
   {
      Circle *circle = dynamic_cast<Circle *>(TNL::Object::create("Circle"));   // Create a new Circle
      F32 ang = TNL::Random::readF() * Float2Pi;

      circle->setPosAng(pos, ang);

      circle->addToGame(game, game->getGameObjDatabase());              // And add it to the list of game objects
   }
}


static void renderCircleSpawn(const Point &pos)
{
#ifndef ZAP_DEDICATED
   F32 scale = 0.8f;
   static const Point p(0,0);

   glPushMatrix();
      glTranslatef(pos.x, pos.y, 0);
      glScalef(scale, scale, 1);
      drawCircle(p, 8);

      glColor(Colors::white);
      drawCircle(p, 13);
   glPopMatrix();  
#endif
}


void CircleSpawn::renderEditor(F32 currentScale)
{
#ifndef ZAP_DEDICATED
   Point pos = getVert(0);

   glPushMatrix();
      glTranslate(pos);
      glScale(1/currentScale);    // Make item draw at constant size, regardless of zoom
      renderCircleSpawn(Point(0,0));
   glPopMatrix();   
#endif
}


void CircleSpawn::renderDock()
{
   renderCircleSpawn(getVert(0));
}



};


