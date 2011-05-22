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

#include "gameObject.h"
#include "moveObject.h"    // For ActualState definition
#include "gameType.h"
#include "ship.h"
#include "GeomUtils.h"

#include "projectile.h"

#include "../glut/glutInclude.h"

#include <math.h>


using namespace TNL;

namespace Zap
{

////////////////////////////////////////
////////////////////////////////////////

GridDatabase *BfObject::getGridDatabase() 
{ 
   return mGame ? mGame->getGridDatabase() : NULL; 
}


bool BfObject::getCollisionPoly(Vector<Point> &polyPoints)
{
   return false;
}


bool BfObject::getCollisionCircle(U32 stateIndex, Point &point, float &radius)
{
   return false;
}


void BfObject::render()
{
   // Do nothing
}


void BfObject::render(S32 layerIndex)
{
   if(layerIndex == 1)
      render();
}

////////////////////////////////////////
////////////////////////////////////////
// Constructor
GameObject::GameObject()
{
   mGame = NULL;
   mTeam = -1;
   /*mLastQueryId = 0;*/
   mObjectTypeMask = UnknownType;
   mObjectTypeNumber = UnknownTypeNumber;
   mDisableCollisionCount = 0;
   mCreationTime = 0;
}


void GameObject::setOwner(GameConnection *connection)
{
   mOwner = connection;
}


GameConnection *GameObject::getOwner()
{
   return mOwner;
}

void GameObject::deleteObject(U32 deleteTimeInterval)
{
   mObjectTypeMask = DeletedType;
   // mObjectTypeNumber = ??? ;
   if(!mGame)
      delete this;
   else
      mGame->addToDeleteList(this, deleteTimeInterval);
}


Point GameObject::getRenderPos()
{
   return getExtent().getCenter();
}


Point GameObject::getActualPos()
{
   return getExtent().getCenter();
}


void GameObject::setScopeAlways()
{
   getGame()->setScopeAlwaysObject(this);
}


void GameObject::setActualPos(Point p)
{
   // Do nothing
}


F32 GameObject::getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips)
{
   GameObject *so = (GameObject *) scopeObject;

   Point center = so->getExtent().getCenter();

   Point nearest;
   const Rect &extent = getExtent();

   if(center.x < extent.min.x)
      nearest.x = extent.min.x;
   else if(center.x > extent.max.x)
      nearest.x = extent.max.x;
   else
      nearest.x = center.x;

   if(center.y < extent.min.y)
      nearest.y = extent.min.y;
   else if(center.y > extent.max.y)
      nearest.y = extent.max.y;
   else
      nearest.y = center.y;

   Point deltap = nearest - center;

   F32 distance = (nearest - center).len();

   Point deltav = getActualVel() - so->getActualVel();

   F32 add = 0;

   // initial scoping factor is distance based.
   F32 distFactor = (500 - distance) / 500;

   // give some extra love to things that are moving towards the scope object
   if(deltav.dot(deltap) < 0)
      add = 0.7;

   // and a little more love if this object has not yet been scoped.
   if(updateMask == 0xFFFFFFFF)
      add += 0.5;
   return distFactor + add + updateSkips * 0.5f;
}

void GameObject::damageObject(DamageInfo *theInfo)
{

}

static Vector<DatabaseObject *> fillVector;

// Returns number of ships hit
S32 GameObject::radiusDamage(Point pos, S32 innerRad, S32 outerRad, U32 typemask, DamageInfo &info, F32 force)
{
   // Check for players within range.  If so, blast them to little tiny bits!
   // Those within innerRad get full force of the damage.  Those within outerRad get damage prop. to distance
   Rect queryRect(pos, pos);
   queryRect.expand(Point(outerRad, outerRad));

   fillVector.clear();
   findObjects(typemask, fillVector, queryRect);

   // Ghosts can't do damage
   if(isGhost())
      info.damageAmount = 0;


   S32 shipsHit = 0;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      GameObject *foundObject = dynamic_cast<GameObject *>(fillVector[i]);
      // Check the actual distance against our outer radius.  Recall that we got a list of potential
      // collision objects based on a square area, but actual collisions will be based on true distance
      Point objPos = foundObject->getActualPos();
      Point delta = objPos - pos;

      if(delta.len() > outerRad)
         continue;

      // Can one damage another?
      if(getGame()->getGameType())
         if(!getGame()->getGameType()->objectCanDamageObject(info.damagingObject, foundObject))
            continue;

      //// Check if damager is an area weapon, and damagee is a projectile... if so, kill it
      //if(Projectile *proj = dynamic_cast<Projectile*>(foundObject))
      //{
      //   proj->explode(proj, proj->getActualPos());
      //}

      // Do an LOS check...
      F32 t;
      Point n;

      if(findObjectLOS(BarrierType, MoveObject::ActualState, pos, objPos, t, n))
         continue;

      // Figure the impulse and damage
      DamageInfo localInfo = info;

      // Figure collision forces...
      localInfo.impulseVector  = delta;
      localInfo.impulseVector.normalize();

      localInfo.collisionPoint  = objPos;
      localInfo.collisionPoint -= info.impulseVector;

      // Reuse t from above to represent interpolation based on distance
      F32 dist = delta.len();
      if(dist < innerRad)           // Inner radius gets full force of blast
         t = 1.f;
      else                          // But if we're further away, force is attenuated
         t = 1.f - (dist - innerRad) / (outerRad - innerRad);

      // Attenuate impulseVector and damageAmount
      localInfo.impulseVector  *= force * t;
      localInfo.damageAmount   *= t;

      // Adjust for self-damage
      GameConnection *damagerOwner = info.damagingObject->getOwner();
      GameConnection *victimOwner = foundObject->getOwner();

      if(victimOwner && damagerOwner == victimOwner)
         localInfo.damageAmount *= localInfo.damageSelfMultiplier;


      if(foundObject->getObjectTypeMask() & (ShipType | RobotType))
         shipsHit++;

      foundObject->damageObject(&localInfo); 
   }

   return shipsHit;
}

void GameObject::findObjects(U32 typeMask, Vector<DatabaseObject *> &fillVector, const Rect &ext)
{
   GridDatabase *gridDB = getGridDatabase();
   if(!gridDB)
      return;
   gridDB->findObjects(typeMask, fillVector, ext);
}


GameObject *GameObject::findObjectLOS(U32 typeMask, U32 stateIndex, Point rayStart, Point rayEnd, 
                                      float &collisionTime, Point &collisionNormal)
{
   GridDatabase *gridDB = getGridDatabase();
   if(!gridDB)
      return NULL;

   return dynamic_cast<GameObject *>(gridDB->
                        findObjectLOS(typeMask, stateIndex, rayStart, rayEnd, collisionTime, collisionNormal));
}


void GameObject::addToGame(Game *theGame)
{
   TNLAssert(mGame == NULL,   "Error: Object already in a game in GameObject::addToGame.");
   TNLAssert(theGame != NULL, "Error: theGame is NULL in GameObject::addToGame.");

   theGame->addToGameObjectList(this);
   mCreationTime = theGame->getCurrentTime();
   mGame = theGame;
   addToDatabase();
   onAddedToGame(theGame);
}


void GameObject::onAddedToGame(Game *)
{
   // Do nothing; overridden by child classes
}


void GameObject::removeFromGame()
{
   if(mGame)
   {
      removeFromDatabase();
      mGame->removeFromGameObjectList(this);
      mGame = NULL;
   }
}


Rect GameObject::getBounds(U32 stateIndex)
{
   Rect ret;
   Point p;
   float radius;
   Vector<Point> bounds;

   if(getCollisionPoly(bounds))
   {
      ret.min = ret.max = bounds[0];
      for(S32 i = 1; i < bounds.size(); i++)
         ret.unionPoint(bounds[i]);
   }
   else if(getCollisionCircle(stateIndex, p, radius))
   {
      ret.max = p + Point(radius, radius);
      ret.min = p - Point(radius, radius);
   }

   return ret;
}


// Find if the specified point is in theObject's collisionPoly or collisonCircle
bool GameObject::collisionPolyPointIntersect(Point point)
{
   Point center;
   F32 radius;
   Vector<Point> polyPoints;

   polyPoints.clear();

   if(getCollisionPoly(polyPoints))
      return PolygonContains2(polyPoints.address(), polyPoints.size(), point);
   else if(getCollisionCircle(MoveObject::ActualState, center, radius))
      return(center.distanceTo(point) <= radius);
   else
      return false;
}


// Find if the specified polygon intersects theObject's collisionPoly or collisonCircle
bool GameObject::collisionPolyPointIntersect(Vector<Point> points)
{
   Point center;
   F32 radius;
   Vector<Point> polyPoints;

   polyPoints.clear();

   if(getCollisionPoly(polyPoints))
      return polygonsIntersect(polyPoints, points);

   else if(getCollisionCircle(MoveObject::ActualState, center, radius))
   {
      Point pt;
      return polygonCircleIntersect(&points[0], points.size(), center, radius * radius, pt);
   }
   else
      return false;
}


// Find if the specified polygon intersects theObject's collisionPoly or collisonCircle
bool GameObject::collisionPolyPointIntersect(Point center, F32 radius)
{
   Point c;
   float r;
   Point pt;
   Vector<Point> polyPoints;

   polyPoints.clear();

   if(getCollisionPoly(polyPoints))
      return polygonCircleIntersect(&polyPoints[0], polyPoints.size(), center, radius * radius, pt);

   else if(getCollisionCircle(MoveObject::ActualState, c, r))
      return ( center.distSquared(c) < (radius + r) * (radius + r) );

   else
      return false;
}


void GameObject::idle(IdleCallPath path)
{
   // Do nothing
}


void GameObject::writeControlState(BitStream *)
{
   // Do nothing
}


void GameObject::readControlState(BitStream *)
{
   // Do nothing
}


void GameObject::controlMoveReplayComplete()
{
   // Do nothing
}


void GameObject::writeCompressedVelocity(Point &vel, U32 max, BitStream *stream)
{
   U32 len = U32(vel.len());
   if(stream->writeFlag(len == 0))
      return;

   if(stream->writeFlag(len > max))
   {
      stream->write(vel.x);
      stream->write(vel.y);
   }
   else
   {
      F32 theta = atan2(vel.y, vel.x);

      //RDW This needs to be writeSignedFloat.
      //Otherwise, it keeps dropping negative thetas.
      stream->writeSignedFloat(theta * FloatInverse2Pi, 10);
      stream->writeRangedU32(len, 0, max);
   }
}


void GameObject::readCompressedVelocity(Point &vel, U32 max, BitStream *stream)
{
   if(stream->readFlag())
   {
      vel.x = vel.y = 0;
      return;
   }
   else if(stream->readFlag())
   {
      stream->read(&vel.x);
      stream->read(&vel.y);
   }
   else
   {
      //RDW This needs to be readSignedFloat.
      //See above.
      F32 theta = stream->readSignedFloat(10) * Float2Pi;
      F32 magnitude = stream->readRangedU32(0, max);
      vel.set(cos(theta) * magnitude, sin(theta) * magnitude);
   }
}


bool GameObject::onGhostAdd(GhostConnection *theConnection)
{
   addToGame(gClientGame);
   return true;
}


};

