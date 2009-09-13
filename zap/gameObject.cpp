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

#include "projectile.h"


#include "../glut/glutInclude.h"


using namespace TNL;

namespace Zap
{

// Constructor
GameObject::GameObject()
{
   mGame = NULL;
   mTeam = -1;
   mLastQueryId = 0;
   mObjectTypeMask = UnknownType;
   mDisableCollisionCount = 0;
   mInDatabase = false;
   mCreationTime = 0;
}

void GameObject::setOwner(GameConnection *c)
{
   mOwner = c;
}

GameConnection *GameObject::getOwner()
{
   return mOwner;
}

void GameObject::deleteObject(U32 deleteTimeInterval)
{
   mObjectTypeMask = DeletedType;
   if(!mGame)
      delete this;
   else
      mGame->addToDeleteList(this, deleteTimeInterval);
}

Point GameObject::getRenderPos()
{
   return extent.getCenter();
}

Point GameObject::getActualPos()
{
   return extent.getCenter();
}

void GameObject::setScopeAlways()
{
   getGame()->setScopeAlwaysObject(this);
}

void GameObject::setActualPos(Point p)
{
}

F32 GameObject::getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips)
{
   GameObject *so = (GameObject *) scopeObject;

   Point center = so->extent.getCenter();

   Point nearest;
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

static Vector<GameObject*> fillVector;

void GameObject::radiusDamage(Point pos, U32 innerRad, U32 outerRad, U32 typemask, DamageInfo &info, F32 force)
{
   // Check for players within range.  If so, blast them to little tiny bits!
   // Those within innerRad get full force of the damage.  Those within outerRad get damage prop. to distance
   Rect queryRect(pos, pos);
   queryRect.expand(Point(outerRad, outerRad));

   fillVector.clear();
   findObjects(typemask, fillVector, queryRect);

   // Ghosts can't do damage.
   if(isGhost())
      info.damageAmount = 0;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      // Check the actual distance against our outer radius.  Recall that we got a list of potential
      // collision objects based on a square area, but actual collisions will be based on true distance
      Point objPos = fillVector[i]->getActualPos();
      Point delta = objPos - pos;

      if(delta.len() > outerRad)
         continue;

      // Can one damage another?
      if(getGame()->getGameType())
         if(!getGame()->getGameType()->objectCanDamageObject(info.damagingObject, fillVector[i]))
            continue;


      //// Check if damager is an area weapon, and damagee is a projectile... if so, kill it
      //if(Projectile *proj = dynamic_cast<Projectile*>(fillVector[i]))
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

      // Reuse t from above to represent interpolation based on distance.
      F32 dist = delta.len();
      if(dist < innerRad)           // Inner radius gets full force of blast
         t = 1.f;
      else                          // But if we're further away, force is attenuated
         t = 1.f - (dist - innerRad) / (outerRad - innerRad);

      // Attenuate impulseVector and damageAmount
      localInfo.impulseVector  *= force * t;
      localInfo.damageAmount   *= t;

      fillVector[i]->damageObject(&localInfo);
   }
}


// Update object's extents in the database
void GameObject::setExtent(Rect &extents)
{
   if(mGame && mInDatabase)
   {
      // Remove from the extents database for current extents...
      mGame->getGridDatabase()->removeFromExtents(this, extent);
      // ...and re-add for the new extent
      mGame->getGridDatabase()->addToExtents(this, extents);
   }
   extent = extents;
}


void GameObject::findObjects(U32 typeMask, Vector<GameObject *> &fillVector, const Rect &ext)
{
   if(!mGame)
      return;
   mGame->getGridDatabase()->findObjects(typeMask, fillVector, ext);
}


GameObject *GameObject::findObjectLOS(U32 typeMask, U32 stateIndex, Point rayStart, Point rayEnd, float &collisionTime, Point &collisionNormal)
{
   if(!mGame)
      return NULL;
   return mGame->getGridDatabase()->findObjectLOS(typeMask, stateIndex, rayStart, rayEnd, collisionTime, collisionNormal);
}


void GameObject::addToDatabase()
{
   if(!mInDatabase)
   {
      mInDatabase = true;
      mGame->getGridDatabase()->addToExtents(this, extent);
   }
}


void GameObject::removeFromDatabase()
{
   if(mInDatabase)
   {
      mInDatabase = false;
      mGame->getGridDatabase()->removeFromExtents(this, extent);
   }
}


void GameObject::addToGame(Game *theGame)
{
   TNLAssert(mGame == NULL, "Error: Object already in a game in GameObject::addToGame.");
   TNLAssert(theGame != NULL, "Error: theGame is NULL in GameObject::addToGame.");

   theGame->addToGameObjectList(this);
   mCreationTime = theGame->getCurrentTime();
   mGame = theGame;
   addToDatabase();
   onAddedToGame(theGame);
}


void GameObject::onAddedToGame(Game *)
{
   // Note --> will be overridden
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

bool GameObject::getCollisionPoly(Vector<Point> &polyPoints)
{
   return false;
}

bool GameObject::getCollisionCircle(U32 stateIndex, Point &point, float &radius)
{
   return false;
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


extern bool PolygonContains(const Point *inVertices, int inNumVertices, const Point &inPoint);

// Find if the specified point is in theObject's collisionPoly or collisonCircle
bool GameObject::collisionPolyPointIntersect(Point point)
{
   Point center;
   F32 radius;
   Vector<Point> polyPoints;

   polyPoints.clear();

   if(getCollisionPoly(polyPoints))
      return PolygonContains(polyPoints.address(), polyPoints.size(), point);
   else if(getCollisionCircle(MoveObject::ActualState, center, radius))
      return(center.distanceTo(point) <= radius);
   else
      return false;
}


extern bool PolygonsIntersect(Vector<Point> &p1, Vector<Point> &p2);
extern bool PolygonCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inCenter, Point::member_type inRadiusSq, Point &outPoint);


// Find if the specified polygon intersects theObject's collisionPoly or collisonCircle
bool GameObject::collisionPolyPointIntersect(Vector<Point> points)
{
   Point center;
   F32 radius;
   Vector<Point> polyPoints;

   polyPoints.clear();

   if(getCollisionPoly(polyPoints))
      return PolygonsIntersect(polyPoints, points);

   else if(getCollisionCircle(MoveObject::ActualState, center, radius))
   {
      Point pt;
      return PolygonCircleIntersect(&points[0], points.size(), center, radius * radius, pt);
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
      return PolygonCircleIntersect(&polyPoints[0], polyPoints.size(), center, radius * radius, pt);

   else if(getCollisionCircle(MoveObject::ActualState, c, r))
      return ( center.distSquared(c) < (radius + r) * (radius + r) );

   else
      return false;
}




void GameObject::render()
{
   // Do nothing
}


void GameObject::render(S32 layerIndex)
{
   if(layerIndex == 1)
      render();
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

// Gets overridden by child classes
bool GameObject::processArguments(S32 argc, const char**argv)
{
   return true;
}

bool GameObject::onGhostAdd(GhostConnection *theConnection)
{
   addToGame(gClientGame);
   return true;
}



};
