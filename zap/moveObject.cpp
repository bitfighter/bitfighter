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
#include "goalZone.h"
#include "GeomUtils.h"
#include "ship.h"
#include "SoundSystem.h"
#include "speedZone.h"
#include "game.h"
#include "gameConnection.h"
#include "stringUtils.h"

#include "LuaScriptRunner.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "sparkManager.h"
#  include "UIEditorMenus.h"
#  include "OpenglUtils.h"
#endif

#include "LuaWrapper.h"

#include <math.h>

namespace Zap
{

Point MoveStates::getPos(S32 state) const             
{
   TNLAssert(state != ActualState, "Do NOT use getPos with the ActualState!");
   return mMoveState[state].pos; 
}


void MoveStates::setPos(S32 state, const Point &pos) 
{ 
   TNLAssert(state != ActualState, "Do NOT use setPos with the ActualState!");
   mMoveState[state].pos = pos; 
}

Point MoveStates::getVel(S32 state) const             { return mMoveState[state].vel; }
void  MoveStates::setVel(S32 state, const Point &vel) { mMoveState[state].vel = vel; }

F32   MoveStates::getAngle(S32 state) const      { return mMoveState[state].angle; }
void  MoveStates::setAngle(S32 state, F32 angle) { mMoveState[state].angle = angle; }


////////////////////////////////////////
////////////////////////////////////////

// Constructor
MoveObject::MoveObject(const Point &pos, F32 radius, F32 mass) : Parent(radius)    
{
   setPosVelAng(pos, Point(0,0), 0);

   mMass = mass;
   mInterpolating = false;
   mHitLimit = 16;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
MoveObject::~MoveObject()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


bool MoveObject::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 2)
      return false;
   else if(!Parent::processArguments(argc, argv, game))
      return false;

   setInitialPosVelAng(getPos(), Point(0,0), 0);
      
   updateExtentInDatabase();

   return true;
}


string MoveObject::toString(F32 gridSize) const
{
   return string(appendId(getClassName())) + " " + geomToString(gridSize);
}


void MoveObject::idle(BfObject::IdleCallPath path)
{
   mHitLimit = 16;      // Reset hit limit
}


void MoveObject::onAddedToGame(Game *game)
{
   Parent::onAddedToGame(game);
   linkToIdleList(&game->idlingObjects);

#ifndef ZAP_DEDICATED
   if(isGhost())     // Client only
      setControllingClient(static_cast<ClientGame *>(game)->getConnectionToServer());
#endif
}

static const float MoveObjectCollisionElasticity = 1.7f;


Rect MoveObject::calcExtents()
{
   const F32 buffer = 10.0f;

   Rect r(getActualPos(), getRenderPos());
   r.expand(Point(mRadius + buffer, mRadius + buffer));

   return r;
}


bool MoveObject::isMoveObject()
{
   return true;
}


Point MoveObject::getPos() const { return getActualPos(); }
Point MoveObject::getVel() const { return getActualVel(); }

Point MoveObject::getActualPos() const { return getPos(ActualState); }
Point MoveObject::getRenderPos() const { return getPos(RenderState); }
Point MoveObject::getActualVel() const { return getVel(ActualState); }
Point MoveObject::getRenderVel() const { return getVel(RenderState); }

F32 MoveObject::getActualAngle()           const { return getAngle(ActualState);      }
F32 MoveObject::getRenderAngle()           const { return getAngle(RenderState);      }
F32 MoveObject::getLastProcessStateAngle() const { return getAngle(LastProcessState); }


void MoveObject::setActualPos(const Point &pos) { setPos(ActualState, pos); }
void MoveObject::setActualVel(const Point &vel) { setVel(ActualState, vel); }
void MoveObject::setActualAngle(F32 angle)      { setAngle(ActualState, angle); }

void MoveObject::setRenderPos(const Point &pos) { setPos(RenderState, pos); }
void MoveObject::setRenderVel(const Point &vel) { setVel(RenderState, vel); }
void MoveObject::setRenderAngle(F32 angle)      { setAngle(RenderState, angle); }


void MoveObject::copyMoveState(S32 from, S32 to)
{
   setPos  (to, getPos(from));
   setVel  (to, getVel(from));
   setAngle(to, getAngle(from));
}


///// The following 6 functions should be the ONLY ones to directly access mMoveStates members
Point MoveObject::getPos(S32 stateIndex) const
{
   if(stateIndex == ActualState)
      return Parent::getPos();
   return mMoveStates.getPos(stateIndex);
}


void MoveObject::setPos(S32 stateIndex, const Point &pos)
{
   if(stateIndex == ActualState)
      Parent::setPos(pos);
   else
      mMoveStates.setPos(stateIndex, pos);
}


// mMoveStates access ok here...
Point MoveObject::getVel  (S32 stateIndex) const { return mMoveStates.getVel  (stateIndex); }
F32   MoveObject::getAngle(S32 stateIndex) const { return mMoveStates.getAngle(stateIndex); }

void MoveObject::setVel  (S32 stateIndex, const Point &vel) { mMoveStates.setVel  (stateIndex, vel);   }
void MoveObject::setAngle(S32 stateIndex, F32 angle)        { mMoveStates.setAngle(stateIndex, angle); }


// For Geometry, should set both actual and render position
void MoveObject::setPos(const Point &pos)
{
   setActualPos(pos);
   setRenderPos(pos);
   Parent::setVert(pos, 0);      // Kind of hacky... need to get this point into the geom object, need to avoid stack overflow TODO: Can get rid of this?
   updateExtentInDatabase();
}


// This is overridden by Asteroids and Circles
void MoveObject::setInitialPosVelAng(const Point &pos, const Point &vel, F32 ang)
{
   setPosVelAng(pos, vel, ang);
}


void MoveObject::setPosVelAng(const Point &pos, const Point &vel, F32 ang)
{
   for(U32 i = 0; i < MoveStateCount; i++)
   {
      setPos(i, pos);
      setVel(i, vel);
      setAngle(i, ang);
   }
}


F32 MoveObject::getMass()
{
   return mMass;
}


void MoveObject::setMass(F32 mass)
{
   mMass = mass;
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

   Point v = contactShip->getVel(stateIndex);
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
   const U32 TRY_COUNT_MAX = 8;
   Vector<SafePtr<BfObject> > disabledList;
   F32 moveTimeStart = moveTime;

   while(moveTime > moveTimeEpsilon && tryCount < TRY_COUNT_MAX)     // moveTimeEpsilon is a very short, but non-zero, bit of time
   {
      tryCount++;

      // Ignore tiny movements unless we're processing a collision
      if(!isBeingDisplaced && getVel(stateIndex).len() < velocityEpsilon)
         break;

      F32 collisionTime = moveTime;
      Point collisionPoint;

      BfObject *objectHit = findFirstCollision(stateIndex, collisionTime, collisionPoint);
      if(!objectHit)    // No collision (or if isBeingDisplaced is true, we haven't been pushed into another object)
      {
         Point newPos = getPos(stateIndex) + getVel(stateIndex) * moveTime;   // Move to desired destination
         setPos(stateIndex, newPos);
         break;
      }

      // Collision!  Advance to the point of collision
      Point newPos = getPos(stateIndex) + getVel(stateIndex) * collisionTime;    // x = x + vt
      setPos(stateIndex, newPos);                                                // setPos(x)

      // Collided is a sort of collision pre-handler; it will return true if the collision was dealt with, false if not
      if(collided(objectHit, stateIndex) || objectHit->collided(this, stateIndex))
      {
         disabledList.push_back(objectHit);
         objectHit->disableCollision();
         tryCount--;   // Don't count as tryCount
      }
      else if(objectHit->isMoveObject())     // Collided with a MoveObject
      {
         TNLAssert(dynamic_cast<MoveObject *>(objectHit), "Not a MoveObject");
         MoveObject *moveObjectThatWasHit = static_cast<MoveObject *>(objectHit);

         Point velDelta = moveObjectThatWasHit->getVel(stateIndex) - getVel(stateIndex);
         Point posDelta = moveObjectThatWasHit->getPos(stateIndex) - getPos(stateIndex);

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
            Point intendedPos = getPos(stateIndex) + getVel(stateIndex) * moveTime;    // x = x + vt

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
         computeCollisionResponseBarrier(stateIndex, collisionPoint);

      moveTime -= collisionTime;
   }

   for(S32 i = 0; i < disabledList.size(); i++)   // enable any disabled collision
      if(disabledList[i].isValid())
         disabledList[i]->enableCollision();

   if(tryCount == TRY_COUNT_MAX && moveTime > moveTimeStart * 0.98f)
      setVel(stateIndex, Point(0,0));  // prevents some overload by not trying to move anymore
}


bool MoveObject::collide(BfObject *otherObject)
{
   return true;
}


TestFunc MoveObject::collideTypes()
{
   return (TestFunc) isAnyObjectType;
}


static S32 QSORT_CALLBACK sortBarriersFirst(DatabaseObject **a, DatabaseObject **b)
{
   return ((*b)->getObjectTypeNumber() == BarrierTypeNumber ? 1 : 0) - ((*a)->getObjectTypeNumber() == BarrierTypeNumber ? 1 : 0);
}


BfObject *MoveObject::findFirstCollision(U32 stateIndex, F32 &collisionTime, Point &collisionPoint)
{
   // Check for collisions against other objects
   Point delta = getVel(stateIndex) * collisionTime;

   Rect queryRect(getPos(stateIndex), getPos(stateIndex) + delta);
   queryRect.expand(Point(mRadius, mRadius));

   fillVector.clear();

   findObjects(collideTypes(), fillVector, queryRect);   // Free CPU for finding only the ones we care about

   fillVector.sort(sortBarriersFirst);  // Sort to do Barriers::Collide first, to prevent picking up flag (FlagItem::Collide) through Barriers, especially when client does /maxfps 10

   F32 collisionFraction;

   BfObject *collisionObject = NULL;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      BfObject *foundObject = static_cast<BfObject *>(fillVector[i]);

      if(!foundObject->isCollisionEnabled())
         continue;

      const Vector<Point> *poly = foundObject->getCollisionPoly();

      if(poly)
      {
         if(poly->size() > 4)   // TODO: Delete
            int x = 0;
         Point cp;

         if(PolygonSweptCircleIntersect(&poly->first(), poly->size(), getPos(stateIndex),
                                        delta, mRadius, cp, collisionFraction))
         {
            if(cp != getPos(stateIndex))   // Avoid getting stuck inside polygon wall
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
      else
      {
         F32   myRadius, otherRadius;
         Point myPos,    shipPos;

         getCollisionCircle(stateIndex, myPos, myRadius);
         if(foundObject->getCollisionCircle(stateIndex, shipPos, otherRadius))
         {

            Point v = getVel(stateIndex);
            Point p = myPos - shipPos;

            if(v.dot(p) < 0)
            {
               F32 R = myRadius + otherRadius;
               if(p.len() <= R)
               {
                  bool collide1 = collide(foundObject);
                  bool collide2 = foundObject->collide(this);

                  if(!(collide1 && collide2))
                     continue;

                  collisionTime = 0;
                  collisionObject = foundObject;
                  delta.set(0,0);

                  p.normalize(myRadius);  // we need this calculation, just to properly show bounce sparks at right position
                  collisionPoint = myPos - p;
               }
               else
               {
                  F32 a = v.dot(v);
                  F32 b = 2 * p.dot(v);
                  F32 c = p.dot(p) - R * R;
                  F32 t;
                  if(FindLowestRootInInterval(a, b, c, collisionTime, t))
                  {
                     bool collide1 = collide(foundObject);
                     bool collide2 = foundObject->collide(this);

                     // If A and B collide, both A and B's collide functions must return true to proceed
                     if(!collide1 || !collide2)
                        continue;

                     collisionTime = t;
                     collisionObject = foundObject;
                     delta = getVel(stateIndex) * collisionTime;

                     p.normalize(otherRadius);  // we need this calculation, just to properly show bounce sparks at right position
                     collisionPoint = shipPos + p;
                  }
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
   Point normal = getPos(stateIndex) - collisionPoint;
   normal.normalize();

   Point newVel = getVel(stateIndex) - normal * MoveObjectCollisionElasticity * normal.dot(getVel(stateIndex));
   setVel(stateIndex, newVel);

#ifndef ZAP_DEDICATED
   // Emit some bump particles on client
   if(isGhost())     // i.e. on client side
   {
      F32 scale = normal.dot(getVel(stateIndex)) * 0.01f;
      if(scale > 0.5f)
      {
         // Make a noise...
         SoundSystem::playSoundEffect(SFXBounceWall, collisionPoint, Point(), getMin(1.0f, scale - 0.25f));

         Color bumpC(scale/3, scale/3, scale);

         for(S32 i = 0; i < 4 * pow((F32)scale, 0.5f); i++)
         {
            Point chaos(TNL::Random::readF(), TNL::Random::readF());
            chaos *= scale + 1;

            if(TNL::Random::readF() > 0.5)
               static_cast<ClientGame *>(getGame())->emitSpark(collisionPoint, 
                                                               normal * chaos.len() + Point(normal.y, -normal.x) * scale * 5  + chaos + 
                                                                         getVel(stateIndex) * 0.05f, bumpC);

            if(TNL::Random::readF() > 0.5)
               static_cast<ClientGame *>(getGame())->emitSpark(collisionPoint, 
                                                               normal * chaos.len() + Point(normal.y, -normal.x) * scale * -5 + chaos + 
                                                                         getVel(stateIndex) * 0.05f, bumpC);
         }
      }
   }
#endif
}


// Runs on both client and server side...
void MoveObject::computeCollisionResponseMoveObject(U32 stateIndex, MoveObject *moveObjectThatWasHit)
{
   // collisionVector is simply a line from the center of moveObjectThatWasHit to the center of this
   Point collisionVector = moveObjectThatWasHit->getPos(stateIndex) - getPos(stateIndex);

   collisionVector.normalize();

   bool moveObjectThatWasHitWasMovingTooSlow = (moveObjectThatWasHit->getVel(stateIndex).lenSquared() < 0.001f);

   // Initial velocities projected onto collisionVector
   F32 v1i = getVel(stateIndex).dot(collisionVector);
   F32 v2i = moveObjectThatWasHit->getVel(stateIndex).dot(collisionVector);

   F32 v1f, v2f;     // Final velocities

   F32 e = 0.9f;     // Elasticity, I think

   v1f = (e * moveObjectThatWasHit->mMass * (v2i - v1i) + mMass * v1i + moveObjectThatWasHit->mMass * v2i) / (mMass + moveObjectThatWasHit->mMass);
   v2f = (e *                       mMass * (v1i - v2i) + mMass * v1i + moveObjectThatWasHit->mMass * v2i) / (mMass + moveObjectThatWasHit->mMass);

   Point newVel;

   newVel = moveObjectThatWasHit->getVel(stateIndex) + collisionVector * (v2f - v2i);
   moveObjectThatWasHit->setVel(stateIndex, newVel);

   newVel = getVel(stateIndex) + collisionVector * (v1f - v1i);
   setVel(stateIndex, newVel);

   if(!isGhost())    // Server only
   {
      // Check for asteroids hitting a ship
      Ship *ship = NULL;
      Asteroid *asteroid = NULL;

      if(getObjectTypeNumber() == AsteroidTypeNumber)
         asteroid = static_cast<Asteroid*>(this);

      if(isShipType(moveObjectThatWasHit->getObjectTypeNumber()))
         ship = static_cast<Ship*>(moveObjectThatWasHit);

      // Since asteroids and ships are both MoveObjects, we'll also check to see if ship hit an asteroid
      if(!ship)
      {
         if(moveObjectThatWasHit->getObjectTypeNumber() == AsteroidTypeNumber)
            asteroid = static_cast<Asteroid*>(moveObjectThatWasHit);
         if(isShipType(getObjectTypeNumber()))
            ship = static_cast<Ship*>(this);
      }

      if(ship && asteroid)      // Collided!  Do some damage!  Bring it on!
      {
         DamageInfo theInfo;
         theInfo.collisionPoint = getActualPos();
         theInfo.damageAmount = 1.0f;     // Kill ship
         theInfo.damageType = DamageTypePoint;
         theInfo.damagingObject = asteroid;
         theInfo.impulseVector = getActualVel();

         ship->damageObject(&theInfo);
      }
   }
#ifndef ZAP_DEDICATED
   else     // Client only
   {
      //logprintf("Collision sound! %d", stateIndex); // <== why don't we see renderstate here more often?
      playCollisionSound(stateIndex, moveObjectThatWasHit, v1i);    

      MoveItem *item = dynamic_cast<MoveItem *>(moveObjectThatWasHit);
      GameType *gameType = getGame()->getGameType();

      if(item && gameType && moveObjectThatWasHitWasMovingTooSlow)  // only if not moving fast, to prevent some overload
         gameType->c2sResendItemStatus(item->getItemId());
   }
#endif
}


// Sometimes stateIndex will in fact be ActualState, which frankly makes no sense, but there you have it
void MoveObject::playCollisionSound(U32 stateIndex, MoveObject *moveObjectThatWasHit, F32 velocity)
{
   if(velocity > 0.25)    // Make sound if the objects are moving fast enough
      SoundSystem::playSoundEffect(SFXBounceObject, moveObjectThatWasHit->getPos(stateIndex));
}


void MoveObject::updateInterpolation()
{
   U32 deltaT = mCurrentMove.time;
   {
      setRenderAngle(getActualAngle());

      if(mInterpolating)
      {
         // first step is to constrain the render velocity to
         // the vector of difference between the current position and
         // the actual position.
         // we can also clamp to zero, the actual velocity, or the
         // render velocity, depending on which one is best.

         Point deltaP = getActualPos() - getRenderPos();
         F32 distance = deltaP.len();

         if(distance == 0)
            goto interpDone;

         deltaP.normalize();
         F32 rvel = deltaP.dot(getRenderVel());
         F32 avel = deltaP.dot(getActualVel());

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
         float currentActualVelocity = getActualVel().len();
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
         setRenderVel(deltaP * rvel);
         setRenderPos(getRenderPos() + getRenderVel() * time);
      }
      else
      {
   interpDone:
         mInterpolating = false;
         copyMoveState(ActualState, RenderState);
      }
   }
}

bool MoveObject::getCollisionCircle(U32 stateIndex, Point &point, F32 &radius) const
{  
   point = getPos(stateIndex);
   radius = mRadius;
   return true;
}


void MoveObject::onGeomChanged()
{
   // This is here, to make sure pressing TAB in editor will show correct location for MoveItems
   setActualPos(getVert(0));
   setRenderPos(getVert(0));
   
   Parent::onGeomChanged();
}


void MoveObject::computeImpulseDirection(DamageInfo *damageInfo)
{
   // Compute impulse direction
   Point dv = damageInfo->impulseVector - getActualVel();
   Point iv = getActualPos() - damageInfo->collisionPoint;
   iv.normalize();
   setActualVel(getActualVel() + iv * dv.dot(iv) * 0.3f / mMass);
}


/////
// Lua interface
/**
 *   @luaclass MoveObject
 *   @brief    Parent class of most things that move (except bullets)
 */

//               Fn name Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getVel, ARRAYDEF({{     END }}), 1 ) \
   METHOD(CLASS, setVel, ARRAYDEF({{ PT, END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(MoveObject, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(MoveObject, LUA_METHODS);

#undef LUA_METHODS


const char *MoveObject::luaClassName = "MoveObject";     // <== Can we rename this to MoveItem without too much confusion?
REGISTER_LUA_SUBCLASS(MoveObject, Item);


/**
 *   @luafunc MoveObject::getVel()
 *   @brief   Returns the items's velocity.
 *   @descr   Points are used to represent velocity; the x and y components represent the speed in the x and y directions respectively.
 *   @return  \e point representing the item's velocity.
 */
S32 MoveObject::getVel(lua_State *L) { return returnPoint(L, getActualVel()); }


/**
 * @luafunc MoveObject::setVel(vel)
 * @brief   Sets the item's velocity.
 * @descr   As with other functions that take a point as an input, you can also specify the x and y components as numeric arguments.
 * @param   vel - A point representing item's velocity.
 */
S32 MoveObject::setVel(lua_State *L)
{
   checkArgList(L, functionArgs, "MoveObject", "setVel");
   setActualVel(getPointOrXY(L, 1));

   return 0;
}

////////////////////////////////////////
////////////////////////////////////////

// Constructor
MoveItem::MoveItem(const Point &pos, bool collideable, float radius, float mass) : MoveObject(pos, radius, mass)
{
   mIsCollideable = collideable;
   mInitial = false;

   updateTimer = 0;
}


// Destructor
MoveItem::~MoveItem()
{
   // Do nothing
}


void MoveItem::setCollideable(bool isCollideable)
{
   mIsCollideable = isCollideable;
}

// Rendering - client only, in-game
void MoveItem::render()                                     { renderItem(getRenderPos());                  }

// Override the following to actually draw our items
void MoveItem::renderItem(const Point &pos)                 { TNLAssert(false, "Unimplemented function!"); }
void MoveItem::renderItemAlpha(const Point &pos, F32 alpha) { TNLAssert(false, "Unimplemented function!"); }


void MoveItem::setActualPos(const Point &pos)
{
   if(pos != getActualPos())
   {
      setPos(ActualState, pos);
      setMaskBits(WarpPositionMask | PositionMask);
   }
}


void MoveItem::setActualVel(const Point &vel)
{
   Parent::setActualVel(vel);
   setMaskBits(PositionMask);
}


void MoveItem::idle(BfObject::IdleCallPath path)
{
   if(!isInDatabase())
      return;

   Parent::idle(path);


   float time = mCurrentMove.time * 0.001f;
   move(time, ActualState, false);
   if(path == BfObject::ServerIdleMainLoop)
   {
      // Only update if it's actually moving...
      if(getActualVel().lenSquared() != 0)
      {
         // Update less often on slow moving item, more often on fast moving item, and update when we change velocity.
         // Update at most every 5 seconds.
         updateTimer -= (getActualVel().len() + 20) * time;
         if(updateTimer < 0 || getActualVel().distSquared(prevMoveVelocity) > 100)
         {
            setMaskBits(PositionMask);
            updateTimer = 100;
            prevMoveVelocity = getActualVel();
         }
      }
      else if(prevMoveVelocity.lenSquared() != 0)
      {
         setMaskBits(PositionMask);  // Tell client that this item is no longer moving
         prevMoveVelocity.set(0,0);
      }

      copyMoveState(ActualState, RenderState);
   }
   else
      updateInterpolation();


   //updateExtentInDatabase();
}


void MoveItem::setPositionMask() { setMaskBits(PositionMask); }      // Could be moved to MoveObject


static const S32 VEL_POINT_SEND_BITS = 511;     // 511 = 2^9 - 1, the biggest int we can pack into 9 bits.

U32 MoveItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = 0;
   if(stream->writeFlag(updateMask & InitialMask))
   {
      // Send id in inital packet
      stream->writeRangedU32(getItemId(), 0, U16_MAX);
   }

   if(stream->writeFlag(updateMask & PositionMask))
   {
      ((GameConnection *) connection)->writeCompressedPoint(getActualPos(), stream);
      writeCompressedVelocity(getActualVel(), VEL_POINT_SEND_BITS, stream);
      stream->writeFlag(updateMask & WarpPositionMask);
   }

   return retMask;
}


void MoveItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool interpolate = false;
   bool positionChanged = false;

   mInitial = stream->readFlag();

   if(mInitial)     // InitialMask
      setItemId(stream->readRangedU32(0, U16_MAX));

   if(stream->readFlag())     // PositionMask
   {
      Point pt;

      ((GameConnection *) connection)->readCompressedPoint(pt, stream);
      setActualPos(pt);

      readCompressedVelocity(pt, VEL_POINT_SEND_BITS, stream);   
      setActualVel(pt);

      positionChanged = true;
      interpolate = !stream->readFlag();     // WarpPositionMask
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
         // Not interpolating here... just warp the object to its reported location
         mInterpolating = false;

         setRenderPos(getActualPos());
         setRenderVel(getActualVel());
         setRenderAngle(getActualAngle());
      }
   }
}


bool MoveItem::collide(BfObject *otherObject)
{
   return mIsCollideable && Parent::collide(otherObject);
}


////////////////////////////////////////
////////////////////////////////////////
// Class of things that can be mounted on ships, such as Flags and ResourceItems

// Constructor
MountableItem::MountableItem(const Point &pos, bool collideable, float radius, float mass) : Parent(pos, collideable, radius, mass)
{
   mIsMounted = false;
   mDroppedTimer.setPeriod(500);    // 500ms --> Time until we can pick the item up after it's been dropped

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
MountableItem::~MountableItem()
{
   if(mMount.isValid())
      mMount->removeMountedItem(this);    // Remove mounted item from our mount's list of mounted things (mostly for server side)
   LUAW_DESTRUCTOR_CLEANUP;
}
 

void MountableItem::idle(BfObject::IdleCallPath path)
{
   if(!isInDatabase())
      return;

   // Unmounted items idle normally.  Mounted ones are handled specially below.
   if(!mIsMounted)   // Unmounted item 
      Parent::idle(path);

   else              // Mounted item
   {
      // TODO: Seems like we shouldn't need to be doing this check... if mount dies, it should run dismount.  So it seems to me.
      if(mMount.isNull() || mMount->hasExploded)   // Mount has been killed... dismount!
      {
         if(!isGhost())    // Server only
            dismount();
      }
      // TODO: Tie item's pos to mount on client so we don't need to send position all the time.   019
      else     // Mount is still ok -- update item's position to match that of mount  
      {
         //setActualPos(mMount->getActualPos());  // This calls setMaskBits on something that does nothing visible, it only waste network bandwidth
         MoveObject::setActualPos(mMount->getActualPos());  // Bypass MoveItem::setActualPos's setMaskBits, Needed, or else it breaks Robots finding flags on ship.
         setRenderPos(mMount->getRenderPos());
      }

      updateExtentInDatabase();
   }

   // Runs on client and server, but only has meaning on server
   mDroppedTimer.update(mCurrentMove.time);
}


// Client only, in-game
void MountableItem::render()
{
   // If the item is mounted, renderItem will be called from the ship it is mounted to
   if(mIsMounted)
      return;

   Parent::render();
}


U32 MountableItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   if(stream->writeFlag(updateMask & MountMask) && stream->writeFlag(mIsMounted))      // mIsMounted gets written iff MountMask is set  
   {
      S32 index = connection->getGhostIndex(mMount);     // Index of ship with item mounted

      if(stream->writeFlag(index != -1))                 // True if some ship has item, false if nothing is mounted
         stream->writeInt(index, GhostConnection::GhostIdBitSize);
      else
         retMask |= MountMask;
   }

   return retMask;
}


void MountableItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())     // MountMask
   {
      bool isMounted = stream->readFlag();
      if(isMounted)
      {
         Ship *ship = NULL;
         
         if(stream->readFlag())
            ship = static_cast<Ship *>(connection->resolveGhost(stream->readInt(GhostConnection::GhostIdBitSize)));

         mountToShip(ship);
      }
      else
         dismount(false);

      mIsMounted = isMounted;
   }
}


bool MountableItem::collide(BfObject *otherObject)
{
   // Mounted items do not collide
   return !mIsMounted && Parent::collide(otherObject);
}


// Runs on both client and server, comes from collision() on the server and the colliding client, and from
// unpackUpdate() in the case of all clients
//
// theShip could be NULL here, and this could still be legit (e.g. flag is in scope, and ship is out of scope)
void MountableItem::mountToShip(Ship *ship)     
{
   TNLAssert(isGhost() || isInDatabase(), "Error, mount item not in database.");

   if(mMount.isValid() && mMount == ship)    // Already mounted on ship!  Nothing to do!
      return;

   if(!ship)
      return;

   if(mMount.isValid())                      // Mounted on something else; dismount!
      dismount(false);

   ship->addMountedItem(this);
   mMount = ship;

   mIsMounted = true;
   setMaskBits(MountMask);

    if(!isGhost()) // i.e. if this is the server
    {
       TNLAssert(getGame(), "NULL game!");
       getGame()->getGameType()->onFlagMounted(ship->getTeam());
    }
}


// Client & server; Note we come through here on initial unpack for mountItem, for better or worse.  When
// we do, mMount is NULL.
void MountableItem::dismount(bool mountWasKilled)
{
   Ship *ship = mMount;

   if(mMount.isValid())                   // Mount could be null if mount is out of scope, but is dropping an always-in-scope item
   {
      mMount->removeMountedItem(this);    // Remove mounted item from our mount's list of mounted things
      setPos(mMount->getActualPos());     // Update the position.  If mMount is invalid, we'll just have to wait for a message from the server to set the pos.
   }

   mMount = NULL;
   mIsMounted = false;
   setMaskBits(MountMask | PositionMask | WarpPositionMask);    // Tell packUpdate() to send item location


   if(!getGame())    // Can happen on game startup
      return;

   if(getGame()->isServer())
   {
      GameType *gt = getGame()->getGameType();
      if(gt)
         gt->itemDropped(ship, this);      // Server-only method
   }

   mDroppedTimer.reset();
}



void MountableItem::setMountedMask()  { setMaskBits(MountMask);    }

bool MountableItem::isMounted() { return mIsMounted; }
Ship *MountableItem::getMount() { return mMount;     }


bool MountableItem::isItemThatMakesYouVisibleWhileCloaked() { return true; }


/////
// Lua interface

/**
 *   @luaclass MountableItem
 *   @brief    Class of items that can be mounted on ships (such as \link Flag Flags\endlink and \link ResourceItem ResourceItems \endlink).
 */

//               Fn name       Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getShip,  ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, isOnShip, ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(MountableItem, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(MountableItem, LUA_METHODS);

#undef LUA_METHODS


const char *MountableItem::luaClassName = "MountableItem";
REGISTER_LUA_SUBCLASS(MountableItem, MoveObject);

/**
 *  @luafunc ship MountableItem::getShip()
 *  @brief   Gets ship that item is mounted to.  If item is not mounted, returns nil.
 *  @return  \e ship - Ship item is mounted to, or nil if item is unmounted
 */
S32 MountableItem::getShip(lua_State *L) 
{ 
   if(mMount.isValid())
      return returnShip(L, mMount);
   else 
      return returnNil(L); 
}


/**
 *  @luafunc bool MountableItem::isOnShip()
 *  @brief   Returns true if the item is mounted on a %ship, false otherwise.
 *  @return  \e bool - True if item is mounted on a %ship, false otherwise.
 */
S32 MountableItem::isOnShip(lua_State *L)
{
   return returnBool(L, mIsMounted);
}

////////////////////////////////////////
////////////////////////////////////////
// VelocityItem -- class of items with more-or-less constant velocity; currently Asteroid and Circle are children classes

VelocityItem::VelocityItem(const Point &pos, F32 speed, F32 radius, F32 mass) : Parent(pos, true, radius, mass)
{
   mInherentSpeed = speed;

   // Give the objects some intial motion in a random direction
   setPosAng(pos, TNL::Random::readF() * FloatTau);
}


void VelocityItem::setPosAng(Point pos, F32 ang)
{
   Point vel = Point(mInherentSpeed * cos(ang), mInherentSpeed * sin(ang));
   setPosVelAng(pos, vel, ang);
}


// Called by ProcessArgs, after object has been constructed
void VelocityItem::setInitialPosVelAng(const Point &pos, const Point &vel, F32 ang)
{
   // Don't clobber velocity set in the constructor -- ignore passed vel and use what we've already got
   setPosVelAng(pos, getActualVel(), ang);
}


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Asteroid);

static const S32 ASTEROID_INITIAL_SIZELEFT            = 3;
static const F32 ASTEROID_MASS_LAST_SIZE              = 1;
static const F32 ASTEROID_RADIUS_MULTIPLYER_LAST_SIZE = 89 * 0.2f;
static const F32 ASTEROID_SPEED                       = 250;

// Combined Lua / C++ default constructor
Asteroid::Asteroid(lua_State *L) : Parent(Point(0,0), ASTEROID_SPEED, getAsteroidRadius(ASTEROID_INITIAL_SIZELEFT), getAsteroidMass(ASTEROID_INITIAL_SIZELEFT))
{
   mSizeLeft = ASTEROID_INITIAL_SIZELEFT;  // higher = bigger

   mNetFlags.set(Ghostable);
   mObjectTypeNumber = AsteroidTypeNumber;
   hasExploded = false;
   mDesign = TNL::Random::readI(0, ASTEROID_DESIGNS - 1);

   mKillString = "crashed into an asteroid";

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
Asteroid::~Asteroid()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


Asteroid *Asteroid::clone() const
{
   return new Asteroid(*this);
}


U32 Asteroid::getDesignCount()
{
   return ASTEROID_DESIGNS;
}


F32 Asteroid::getAsteroidRadius(S32 size_left)
{
   return ASTEROID_RADIUS_MULTIPLYER_LAST_SIZE / 2 * F32(1 << size_left);  // doubles for each size left
}


F32 Asteroid::getAsteroidMass(S32 size_left)
{
   return ASTEROID_MASS_LAST_SIZE / 2 * F32(1 << size_left);  // doubles for each size left
}


void Asteroid::renderItem(const Point &pos)
{
   if(!hasExploded)
      renderAsteroid(pos, mDesign, mRadius / 89.f);
}


void Asteroid::renderDock()
{
   renderAsteroid(getActualPos(), 2, .1f);
}


const char *Asteroid::getOnScreenName()     { return "Asteroid";  }
const char *Asteroid::getPrettyNamePlural() { return "Asteroids"; }
const char *Asteroid::getOnDockName()       { return "Ast.";      }
const char *Asteroid::getEditorHelpString() { return "Shootable asteroid object.  Just like the arcade game."; }


F32 Asteroid::getEditorRadius(F32 currentScale)
{
   return mRadius * currentScale;
}


const Vector<Point> *Asteroid::getCollisionPoly() const
{
   //for(S32 i = 0; i < ASTEROID_POINTS; i++)
   //{
   //   Point p = Point(mMoveState[MoveObject::ActualState].pos.x + (F32) AsteroidCoords[mDesign][i][0] * asteroidRenderSize[mSizeIndex],
   //                   mMoveState[MoveObject::ActualState].pos.y + (F32) AsteroidCoords[mDesign][i][1] * asteroidRenderSize[mSizeIndex] );

   //   polyPoints.push_back(p);
   //}

   return NULL;  // No Collision Poly, that may help reduce lag with client and server  <== Why?
}


#define ABS(x) (((x) > 0) ? (x) : -(x))


void Asteroid::damageObject(DamageInfo *theInfo)
{
   if(hasExploded)   // Avoid index out of range error
      return; 

   // Compute impulse direction
   mSizeLeft--;
   
   if(mSizeLeft <= 0)    // Kill small items
   {
      hasExploded = true;
      deleteObject(500);
      setMaskBits(ExplodedMask);    // Fix asteroids delay destroy after hit again...
      return;
   }

   setMaskBits(ItemChangedMask);    // So our clients will get new size
   setRadius(getAsteroidRadius(mSizeLeft));
   setMass(getAsteroidMass(mSizeLeft));

   F32 ang = TNL::Random::readF() * FloatTau;      // Sync
   setPosAng(getActualPos(), ang);

   Asteroid *newItem = new Asteroid();
   newItem->mSizeLeft = mSizeLeft;
   newItem->setRadius(getAsteroidRadius(mSizeLeft));
   newItem->setMass(getAsteroidMass(mSizeLeft));

   F32 ang2;
   do
      ang2 = TNL::Random::readF() * Float2Pi;      // Sync
   while(ABS(ang2 - ang) < .0436 );    // That's 20 degrees in radians, folks!

   newItem->setPosAng(getActualPos(), ang2);

   newItem->addToGame(getGame(), getGame()->getGameObjDatabase());    // And add it to the list of game objects
}


static U8 ASTEROID_SIZELEFT_BIT_COUNT = 3;
static S32 ASTEROID_SIZELEFT_MAX = 5;   // For editor attribute. real limit based on bit count is (1 << ASTEROID_SIZELEFT_BIT_COUNT) - 1; // = 7

U32 Asteroid::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   if(stream->writeFlag(updateMask & ItemChangedMask))
   {
      stream->writeInt(mSizeLeft, ASTEROID_SIZELEFT_BIT_COUNT);
      stream->writeEnum(mDesign, ASTEROID_DESIGNS);
   }

   stream->writeFlag(hasExploded);

   return retMask;
}


void Asteroid::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
   {
      mSizeLeft = stream->readInt(ASTEROID_SIZELEFT_BIT_COUNT);
      setRadius(getAsteroidRadius(mSizeLeft));
      setMass(getAsteroidMass(mSizeLeft));
      mDesign = stream->readEnum(ASTEROID_DESIGNS);

      if(!mInitial)
         SoundSystem::playSoundEffect(SFXAsteroidExplode, getRenderPos());
   }

   bool explode = (stream->readFlag());     // Exploding!  Take cover!!

   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();
      onItemExploded(getRenderPos());
   }
}


bool Asteroid::collide(BfObject *otherObject)
{
   if(hasExploded)
      return false;

   if(isGhost())   // Client only, to try to prevent asteroids desync...
   {
      if(isShipType(otherObject->getObjectTypeNumber()))
      {
         // Client does not know if we actually get destroyed from asteroids
         // prevents bouncing off asteroids, then LAG puts back to position.
         if(!static_cast<Ship *>(otherObject)->isModulePrimaryActive(ModuleShield))
            return false;
      }
   }

   // Asteroids don't collide with one another!
   if(otherObject->getObjectTypeNumber() == AsteroidTypeNumber)
      return false;

   return true;
}


TestFunc Asteroid::collideTypes()
{
   return (TestFunc) isAsteroidCollideableType;
}


// Client only
void Asteroid::onItemExploded(Point pos)
{
   SoundSystem::playSoundEffect(SFXAsteroidExplode, pos);
   // FXManager::emitBurst(pos, Point(.1, .1), Colors::white, Colors::white, 10);
}


bool Asteroid::processArguments(S32 argc2, const char **argv2, Game *game)
{
   S32 argc = 0;
   const char *argv[8];                // 8 is ok for now..

   for(S32 i = 0; i < argc2; i++)      // The idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char firstChar = argv2[i][0];    // First character of arg

      if((firstChar >= 'a' && firstChar <= 'z') || (firstChar >= 'A' && firstChar <= 'Z'))
      {
         if(!strnicmp(argv2[i], "Size=", 5))
            mSizeLeft = atoi(&argv2[i][5]);
      }
      else
      {
         if(argc < 8)
         {  
            argv[argc] = argv2[i];
            argc++;
         }
      }
   }
   setRadius(getAsteroidRadius(mSizeLeft));
   setMass(getAsteroidMass(mSizeLeft));

   return Parent::processArguments(argc, argv, game);
}


string Asteroid::toString(F32 gridSize) const
{
   if(mSizeLeft != ASTEROID_INITIAL_SIZELEFT)
      return Parent::toString(gridSize) + " Size=" + itos(mSizeLeft);
   else
      return Parent::toString(gridSize);
}


#ifndef ZAP_DEDICATED

EditorAttributeMenuUI *Asteroid::mAttributeMenuUI = NULL;

EditorAttributeMenuUI *Asteroid::getAttributeMenu()
{
   // Lazily initialize this -- if we're in the game, we'll never need this to be instantiated
   if(!mAttributeMenuUI)
   {
      ClientGame *clientGame = static_cast<ClientGame *>(getGame());

      mAttributeMenuUI = new EditorAttributeMenuUI(clientGame);

      mAttributeMenuUI->addMenuItem(new CounterMenuItem("Size:", mSizeLeft, 1, 1, ASTEROID_SIZELEFT_MAX, "", "", ""));

      // Add our standard save and exit option to the menu
      mAttributeMenuUI->addSaveAndQuitMenuItem();
   }

   return mAttributeMenuUI;
}


// Get the menu looking like what we want
void Asteroid::startEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   attributeMenu->getMenuItem(0)->setIntValue(mSizeLeft);
}


// Retrieve the values we need from the menu
void Asteroid::doneEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   mSizeLeft = attributeMenu->getMenuItem(0)->getIntValue();
   setRadius(getAsteroidRadius(mSizeLeft));
   setMass(getAsteroidMass(mSizeLeft));
}


// Render some attributes when item is selected but not being edited
string Asteroid::getAttributeString()
{
   return "Size: " + itos(mSizeLeft);      
}

#endif


/////
// Lua interface

/**
 *   @luaclass Asteroid
 *   @brief    Just like the arcade game!  Yo!
 */

//               Fn name       Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getSizeIndex, ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getSizeCount, ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(Asteroid, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(Asteroid, LUA_METHODS);

#undef LUA_METHODS


const char *Asteroid::luaClassName = "Asteroid";
REGISTER_LUA_SUBCLASS(Asteroid, MoveObject);


/**
 *  @luafunc Asteroid::getSizeIndex()
 *  @brief   Get %s asteroids current size index.
 *  @descr   Index 1 represents the %asteroid's initial size.  After it has been broken once, it's size index will be 2, and so on.
 *           This method will always return an integer between 1 and the value returned by the %getSizeCount() method (inclusive).
 *  @return  \e int - Index corresponding to the %asteroid's current size.
 */
S32 Asteroid::getSizeIndex(lua_State *L) { return returnInt(L, ASTEROID_INITIAL_SIZELEFT - mSizeLeft + 1); }

/**
 *  @luafunc Asteroid::getSizeCount()
 *  @brief   Returns size index of smallest asteroid.
 *  @descr   Remember, bigger indices mean smaller asteroids.
 *  @return  \e int - Index of the %asteroid's smallest size.
 */
S32 Asteroid::getSizeCount(lua_State *L) { return returnInt(L, ASTEROID_INITIAL_SIZELEFT + 1); }


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Circle);

static const F32 CIRCLE_SPEED = 250;
static const F32 CIRCLE_MASS  = 4;

// Combined Lua / C++ default constructor
Circle::Circle(lua_State *L) : Parent(Point(0,0), CIRCLE_SPEED, (F32)CIRCLE_RADIUS, CIRCLE_MASS)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = CircleTypeNumber;
   hasExploded = false;

   mKillString = "crashed into an circle";
   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
Circle::~Circle()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


Circle *Circle::clone() const
{
   return new Circle(*this);
}


void Circle::idle(BfObject::IdleCallPath path)
{
   //if(path == BfObject::ServerIdleMainLoop)
   {
      // Find nearest ship
      fillVector.clear();
      findObjects((TestFunc)isShipType, fillVector, Rect(getActualPos(), 1200));

      F32 dist = F32_MAX;
      Ship *closest = NULL;

      for(S32 i = 0; i < fillVector.size(); i++)
      {
         Ship *ship = static_cast<Ship *>(fillVector[i]);
         F32 d = getActualPos().distSquared(ship->getActualPos());
         if(d < dist)
         {
            closest = ship;
            dist = d;
         }
      }

      if(!closest)
         return;

      Point v = getActualVel();
      v += closest->getActualPos() - getActualPos();

      v.normalize(CIRCLE_SPEED);

      setActualVel(v);
   }

   Parent::idle(path);
}


void Circle::renderItem(const Point &pos)
{
#ifndef ZAP_DEDICATED
   if(!hasExploded)
   {
      glColor(Colors::red);
      drawCircle(pos, (F32)CIRCLE_RADIUS);
   }
#endif
}


void Circle::renderDock()
{
   drawCircle(getActualPos(), 2);
}


const char *Circle::getOnScreenName()     { return "Circle";  }
const char *Circle::getPrettyNamePlural() { return "Circles"; }
const char *Circle::getOnDockName()       { return "Circ.";   }
const char *Circle::getEditorHelpString() { return "Shootable circle object.  Scary."; }


F32 Circle::getEditorRadius(F32 currentScale)
{
   return CIRCLE_RADIUS * currentScale;
}


const Vector<Point> *Circle::getCollisionPoly() const
{
   return NULL;
}


void Circle::damageObject(DamageInfo *theInfo)
{
   // Compute impulse direction
   hasExploded = true;
   deleteObject(500);
   setMaskBits(ExplodedMask);    // Fix asteroids delay destroy after hit again...
   return;
}


U32 Circle::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   stream->writeFlag(hasExploded);

   return retMask;
}


void Circle::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   bool explode = (stream->readFlag());     // Exploding!  Take cover!!

   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();
      onItemExploded(getRenderPos());
   }
}


bool Circle::collide(BfObject *otherObject)
{
   return true;
}


// Client only
void Circle::onItemExploded(Point pos)
{
   SoundSystem::playSoundEffect(SFXAsteroidExplode, pos);
}


void Circle::playCollisionSound(U32 stateIndex, MoveObject *moveObjectThatWasHit, F32 velocity)
{
   // Do nothing
}


/////
// Lua interface
/**
  *  @luaclass Circle
  *  @brief Annoying circle that follows ships around.
  *  @descr This is not really a supported object.  Just something we were playing around with.  Do not use this!
  *  @geom  The geometry of %Circle is a single point.
  */
const luaL_reg           Circle::luaMethods[]   = { { NULL, NULL } };
const LuaFunctionProfile Circle::functionArgs[] = { { NULL, { }, 0 } };


const char *Circle::luaClassName = "Circle";
REGISTER_LUA_SUBCLASS(Circle, MoveObject);


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Worm);

Worm::Worm()
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = WormTypeNumber;
   hasExploded = false;

   mDirTimer.reset(100);

   mKillString = "killed by a worm";

   mHeadIndex = 0;
   mTailLength = 0;
}


bool Worm::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 2)
      return false;

   Point pos;
   pos.read(argv);
   pos *= game->getGridSize();

   setPos(pos);
   mPoints[mHeadIndex] = pos;  // needed to give Worm a proper position

   mHeadIndex = 0;
   mTailLength = 0;

   setExtent(Rect(pos, 1));

   return true;
}


const char *Worm::getOnScreenName()
{
   return "Worm";
}

string Worm::toString(F32 gridSize) const
{
   return string(appendId(getClassName())) + " " + geomToString(gridSize);
}

void Worm::render()
{
#ifndef ZAP_DEDICATED

   if(!hasExploded)
   {
      if(mTailLength <= 1)
      {
         renderWorm(mPoints[mHeadIndex]);
         return;
      }
      F32 p[maxTailLength * 2];
      S32 i = mHeadIndex;
      for(S32 count = 0; count <= mTailLength; count++)
      {
         p[count * 2] = mPoints[i].x;
         p[count * 2 + 1] = mPoints[i].y;
         i--;
         if(i < 0)
            i = maxTailLength - 1;
      }
      glColor(Colors::white);

      static const F32 WormColors[maxTailLength * 4] = {
         1,   1,   1,  1,
      .90f,.80f,.66f,  1,
      .80f,.60f,.33f,  1,
      .70f,.40f,   0,  1,
      .80f,.60f,.33f,  1,
      .90f,.80f,.66f,  1,
         1,   1,   1,  1,
      .90f,.80f,.66f,  1,
      .80f,.60f,.33f,  1,
      .70f,.40f,   0,  1,
      .80f,.60f,.33f,  1,
      .90f,.80f,.66f,  1,
         1,   1,   1,  1,
      .90f,.80f,.66f,  1,
      .80f,.60f,.33f,  1,
      .70f,.40f,   0,  1,
      .80f,.60f,.33f,  1,
      .90f,.80f,.66f,  1,
         1,   1,   1,  1,
      .90f,.80f,.66f,  1,
      .80f,.60f,.33f,  1,
      .70f,.40f,   0,  1,
      .80f,.60f,.33f,  1,
      .90f,.80f,.66f,  1,
         1,   1,   1,  1,
      .90f,.80f,.66f,  1,
      .80f,.60f,.33f,  1,
      .70f,.40f,   0,  1,
      };
      renderColorVertexArray(p, WormColors, mTailLength + 1, GL_LINE_STRIP);
   }
#endif
}


Worm *Worm::clone() const
{
   return new Worm(*this);
}


F32 Worm::getRadius()
{
   return WORM_RADIUS;
}



void Worm::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   render();
}


void Worm::renderDock()
{
   render();
}


F32 Worm::getEditorRadius(F32 currentScale)
{
   return 10;     // Or something... who knows??
}


bool Worm::getCollisionCircle(U32 state, Point &center, F32 &radius) const
{
   //center = mMoveState[state].pos;
   //radius = F32(WORM_RADIUS);
   return false;
}


const Vector<Point> *Worm::getCollisionPoly() const
{
   if(mPolyPoints.size() > 0)
      return &mPolyPoints;
   else
      return NULL;
}

bool Worm::collide(BfObject *otherObject)
{
   // Worms don't collide with one another!  (look at isCollideableTypeWorm for what Worm collide to)
   return true;
}


void Worm::setPosAng(Point pos, F32 ang)
{
   mHeadIndex++;
   if(mHeadIndex >= maxTailLength)
      mHeadIndex = 0;

   mPoints[mHeadIndex] = pos;

   if(mTailLength < maxTailLength - 1)
   {
      mTailLength++;
      setMaskBits(TailPointPartsMask << mHeadIndex | ExplodeOrTailLengthMask);
   }
   else
      setMaskBits(TailPointPartsMask << mHeadIndex);

   setExtent(*getCollisionPoly());
}


void Worm::damageObject(DamageInfo *theInfo)
{
   mTailLength -= S32(theInfo->damageAmount * 8.f + .9f);
   if(mTailLength >= maxTailLength - 1)
      mTailLength = maxTailLength - 1;


   if(mTailLength < 2)
   {
      hasExploded = true;
      deleteObject(500);
   }
   setMaskBits(ExplodeOrTailLengthMask);
}


static bool isCollideableTypeWorm(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber   ||
         x == TurretTypeNumber  || x == ForceFieldTypeNumber ||
         x == ForceFieldProjectorTypeNumber;
}

void Worm::onAddedToGame(Game *game)
{
   Parent::onAddedToGame(game);
   linkToIdleList(&game->idlingObjects);
}

void Worm::idle(BfObject::IdleCallPath path)
{
   if(!isInDatabase())
      return;

   if(path == ServerIdleMainLoop && mDirTimer.update(mCurrentMove.time))
   {
      Point p, pos1;
      F32 collisionTime;
      mAngle = (mAngle + (TNL::Random::readF() - 0.5f) * 173); // * FloatPi * 4.f;
      U32 retryCount = 0;
      do
      {
         if(retryCount)
            mAngle += FloatPi * 2 / 5;
         retryCount++;

         if(mAngle < -FloatPi * 4.f)
            mAngle += FloatPi * 8.f;
         if(mAngle > FloatPi * 4.f)
            mAngle -= FloatPi * 8.f;

         p.setPolar(TNL::Random::readF() * 32 + 40, mAngle);

         Rect queryRect(mPoints[mHeadIndex], mPoints[mHeadIndex] + p);
         static const F32 radius = 2;
         queryRect.expand(Point(radius, radius));
         fillVector.clear();
         findObjects((TestFunc) isCollideableTypeWorm, fillVector, queryRect);
         
         collisionTime = 1;

         pos1 = mPoints[mHeadIndex] - p * 0.01f;

         for(S32 i = 0; i < fillVector.size(); i++)
         {
            BfObject *foundObject = static_cast<BfObject *>(fillVector[i]);

            if(!foundObject->isCollisionEnabled())
               continue;

            const Vector<Point> *poly = foundObject->getCollisionPoly();
            if(poly)
            {
               Point cp;
               F32 collisionFraction;
               if(PolygonSweptCircleIntersect(&poly->first(), poly->size(), pos1, p, radius, cp, collisionFraction))
               {
                  if(cp != pos1 && collisionTime > collisionFraction)   // Avoid getting stuck inside polygon wall
                  {
                     collisionTime = collisionFraction;
                     if(collisionTime <= 0)
                     {
                        collisionTime = 0;
                        break;
                     }
                  }
               }
            }
         }
      } while(retryCount < 5 && collisionTime < 0.25f);

      setPosAng(p * collisionTime + pos1, mAngle);
      mDirTimer.reset(TNL::Random::readI(300, 400));
   }

   Parent::idle(path);

   // Recompute mPolyPoints
   S32 i = mHeadIndex;
   mPolyPoints.clear();

   for(S32 count = 0; count < mTailLength; count++)
   {
      mPolyPoints.push_back(mPoints[i]);
      i--;
      if(i < 0)
         i = maxTailLength - 1;
   }
   for(S32 count = 0; count < mTailLength; count++)
   {
      mPolyPoints.push_back(mPoints[i]);
      i++;
      if(i >= maxTailLength)
         i = 0;
   }
}


U32 Worm::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   stream->writeInt(mHeadIndex, 5);

   // Most of time, we only update the Point at mHeadIndex, which can save 28 bits.
   if(stream->writeFlag(U32(TailPointPartsMask << mHeadIndex) == (updateMask & TailPointPartsFullMask)))
      mPoints[mHeadIndex].write(stream);
   else
      for(S32 i = 0; i < maxTailLength; i++)
      {
         if(stream->writeFlag(TailPointPartsMask << i))
         {
            mPoints[i].write(stream);
         }
      }

   if(stream->writeFlag(updateMask & ExplodeOrTailLengthMask))
   {
      if(!stream->writeFlag(hasExploded))
         stream->writeInt(mTailLength, 5);
   }

   return retMask;
}


void Worm::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
#ifndef ZAP_DEDICATED
   Parent::unpackUpdate(connection, stream);

   mHeadIndex = stream->readInt(5);

   if(stream->readFlag())
      mPoints[mHeadIndex].read(stream);
   else
      for(S32 i=0; i < maxTailLength; i++)
      {
         if(stream->readFlag())
         {
            mPoints[i].read(stream);
         }
      }

   if(stream->readFlag())
   {
      bool explode = (stream->readFlag());     // Exploding!  Take cover!!
      if(!explode)
         mTailLength = stream->readInt(5);

      if(explode && !hasExploded)
      {
         hasExploded = true;
         disableCollision();
         ClientGame *game = static_cast<ClientGame *>(getGame());

         static const S32 WormExplodeColorsTotal = 2;
         static const Color WormExplodeColors[WormExplodeColorsTotal] = {Colors::orange50, Colors::orange67};
         game->emitExplosion(mPoints[mHeadIndex], 0.4f, WormExplodeColors, WormExplodeColorsTotal);
      }
   }

   setExtent(*getCollisionPoly());
#endif
}


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(TestItem);

static const F32 TEST_ITEM_MASS = 4;

// Combined Lua / C++ default constructor
TestItem::TestItem(lua_State *L) : Parent(Point(0, 0), true, (F32)TEST_ITEM_RADIUS, TEST_ITEM_MASS)
{
   if(L && checkArgList(L, functionArgs, "TestItem", "constructor") == 1)
      setPos(getPointOrXY(L, 1));
   
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = TestItemTypeNumber;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
TestItem::~TestItem() 
{
   LUAW_DESTRUCTOR_CLEANUP;
}


TestItem *TestItem::clone() const
{
   return new TestItem(*this);
}


void TestItem::idle(BfObject::IdleCallPath path)
{
   //if(path == ServerIdleMainLoop && (abs(getPos().x) > 1000 || abs(getPos().y > 1000)))
   //   deleteObject(100);

   Parent::idle(path);
}


void TestItem::renderItem(const Point &pos)
{
   renderTestItem(pos);
}


void TestItem::renderDock()
{
   renderTestItem(getActualPos(), 8);
}


const char *TestItem::getOnScreenName()      {  return "TestItem";   }
const char *TestItem::getPrettyNamePlural()  {  return "TestItems";  }
const char *TestItem::getOnDockName()        {  return "Test";       }
const char *TestItem::getEditorHelpString()  {  return "Bouncy object that floats around and gets in the way."; }


F32 TestItem::getEditorRadius(F32 currentScale)
{
   return getRadius() * currentScale;
}


// Appears to be server only??
void TestItem::damageObject(DamageInfo *theInfo)
{
   computeImpulseDirection(theInfo);
}


const Vector<Point> *TestItem::getCollisionPoly() const
{
   //for(S32 i = 0; i < 8; i++)    // 8 so that first point gets repeated!  Needed?  Maybe not
   //{
   //   Point p = Point(60 * cos(i * Float2Pi / 7 + FloatHalfPi) + getActualPos().x, 60 * sin(i * Float2Pi / 7 + FloatHalfPi) + getActualPos().y);
   //   polyPoints.push_back(p);
   //}

   // Override parent so getCollisionCircle is used instead
   return NULL;
}

/////
// Lua interface

/**
 *  @luafunc  TestItem::TestItem()
 *  @luafunc  TestItem::TestItem(geom)
 *  @luaclass TestItem
 *  @brief    Large bouncy ball type item.
 */
const luaL_reg           TestItem::luaMethods[]   = { { NULL, NULL } };

#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, constructor, ARRAYDEF({{ END }, { GEOM, END }}), 2 ) \
   
GENERATE_LUA_FUNARGS_TABLE(TestItem, LUA_METHODS);

const char *TestItem::luaClassName = "TestItem";
REGISTER_LUA_SUBCLASS(TestItem, MoveObject);


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(ResourceItem);

static const F32 RESOURCE_ITEM_MASS = 1;

// Combined Lua / C++ default constructor
ResourceItem::ResourceItem(lua_State *L) : Parent(Point(0,0), true, (F32)RESOURCE_ITEM_RADIUS, RESOURCE_ITEM_MASS)
{
   if(L && checkArgList(L, functionArgs, "ResourceItem", "constructor") == 1)
      setPos(getPointOrXY(L, 1));
   
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = ResourceItemTypeNumber;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
ResourceItem::~ResourceItem() 
{
   LUAW_DESTRUCTOR_CLEANUP;
}


ResourceItem *ResourceItem::clone() const
{
   return new ResourceItem(*this);
}


void ResourceItem::renderItem(const Point &pos)
{
   renderResourceItem(pos);
}


void ResourceItem::renderItemAlpha(const Point &pos, F32 alpha)
{
   renderResourceItem(pos, alpha);
}


void ResourceItem::renderDock()
{
   renderResourceItem(getActualPos(), .4f, 0, 1);
}


const char *ResourceItem::getOnScreenName()     { return "ResourceItem"; }
const char *ResourceItem::getPrettyNamePlural() { return "Resource Items"; }
const char *ResourceItem::getOnDockName()       { return "Res."; }
const char *ResourceItem::getEditorHelpString() { return "Small bouncy object; capture one to activate Engineer module"; }


bool ResourceItem::collide(BfObject *hitObject)
{
   if(mIsMounted)
      return false;

   if( ! (isShipType(hitObject->getObjectTypeNumber())) )
      return true;

   // Ignore collisions that occur to recently dropped items.  Make sure item is ready to be picked up! 
   if(mDroppedTimer.getCurrent())    
      return false;

   if(!isShipType(hitObject->getObjectTypeNumber()))
      return false;

   Ship *ship = static_cast<Ship *>(hitObject);

   if(ship->hasExploded)
      return false;

   if(ship->hasModule(ModuleEngineer) && !ship->isCarryingItem(ResourceItemTypeNumber))
   {
      if(!isGhost())
         mountToShip(ship);
      return false;
   }
   return true;
}


void ResourceItem::damageObject(DamageInfo *theInfo)
{
   computeImpulseDirection(theInfo);
}


void ResourceItem::dismount(bool mountWasKilled)
{
   Ship *ship = mMount;
   Parent::dismount(mountWasKilled);

   if(!isGhost() && ship)   // Server only, to prevent desync
      setActualVel(ship->getActualVel() * 1.5);
}


/////
// Lua interface

/**
 *  @luafunc  ResourceItem::ResourceItem()
 *  @luafunc  ResourceItem::ResourceItem(geom)
 *  @luaclass ResourceItem
 *  @brief    Small bouncy ball type item.  In levels where Engineer module is allowed, ResourceItems can be collected and transformed
 *            into other items.
 */
const luaL_reg           ResourceItem::luaMethods[]   = { { NULL, NULL } };

#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, constructor, ARRAYDEF({{ END }, { GEOM, END }}), 2 ) \
   
GENERATE_LUA_FUNARGS_TABLE(ResourceItem, LUA_METHODS);

const char *ResourceItem::luaClassName = "ResourceItem";
REGISTER_LUA_SUBCLASS(ResourceItem, MountableItem);


};

