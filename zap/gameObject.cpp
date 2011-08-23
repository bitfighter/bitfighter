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
#include "game.h"

#ifndef ZAP_DEDICATED
#include "ClientGame.h"
#endif

#include "projectile.h"

#include <math.h>

#include "luaObject.h"     // For LuaObject def and returnInt method
#include "lua.h"           // For push prototype

#include "tnlBitStream.h"

using namespace TNL;

namespace Zap
{

// Derived Object Type conditional methods
bool isEngineeredType(U8 x)
{
   return
         x == TurretTypeNumber || x == ForceFieldProjectorTypeNumber;
}

bool isShipType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber;
}

bool isProjectileType(U8 x)
{
   return
         x == MineTypeNumber || x == SpyBugTypeNumber || x == BulletTypeNumber;
}

bool isGrenadeType(U8 x)
{
   return
         x == MineTypeNumber || x == SpyBugTypeNumber;
}

bool isWithHealthType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldProjectorTypeNumber;
}

bool isForceFieldDeactivatingType(U8 x)
{
   return
         x == MineTypeNumber || x == SpyBugTypeNumber ||
         x == FlagTypeNumber || x == SoccerBallItemTypeNumber ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber || x == AsteroidTypeNumber ||
         x == EnergyItemTypeNumber || x == RepairItemTypeNumber ||
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber;
}

bool isDamageableType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == BulletTypeNumber || x == MineTypeNumber || x == SpyBugTypeNumber ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber || x == AsteroidTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == FlagTypeNumber || x == SoccerBallItemTypeNumber || x == CircleTypeNumber || x == ReactorTypeNumber;
}


bool isMotionTriggerType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber || x == AsteroidTypeNumber ||
         x == MineTypeNumber;
}


bool isTurretTargetType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber || x == ResourceItemTypeNumber ||
         x == TestItemTypeNumber || x == SoccerBallItemTypeNumber;
}


bool isCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldTypeNumber ||
         x == ForceFieldProjectorTypeNumber;
}


bool isForceFieldCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldProjectorTypeNumber;
}


bool isWallType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == WallItemTypeNumber || x == WallEdgeTypeNumber || x == WallSegmentTypeNumber;
}


bool isLineItemType(U8 x)
{
   return
         x == BarrierTypeNumber || x == WallItemTypeNumber || x == LineTypeNumber;
}


bool isWeaponCollideableType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == SpyBugTypeNumber || x == MineTypeNumber || x == BulletTypeNumber ||
         x == FlagTypeNumber || x == SoccerBallItemTypeNumber ||
         x == AsteroidTypeNumber || x == TestItemTypeNumber || x == ResourceItemTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == BarrierTypeNumber || x == PolyWallTypeNumber || x == ForceFieldTypeNumber || x == CircleTypeNumber || x == ReactorTypeNumber;
}

bool isAsteroidCollideableType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == TestItemTypeNumber || x == ResourceItemTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == BarrierTypeNumber || x == PolyWallTypeNumber || x == ForceFieldTypeNumber || x == ReactorTypeNumber;
}

bool isFlagCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == ForceFieldTypeNumber;
}

bool isVisibleOnCmdrsMapType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == FlagTypeNumber || x == SoccerBallItemTypeNumber ||
         x == GoalZoneTypeNumber || x == NexusTypeNumber || x == LoadoutZoneTypeNumber || x == SlipZoneTypeNumber ||
         x == SpeedZoneTypeNumber || x == TeleportTypeNumber ||
         x == LineTypeNumber || x == TextItemTypeNumber ||
         x == AsteroidTypeNumber || x == TestItemTypeNumber || x == ResourceItemTypeNumber ||
         x == EnergyItemTypeNumber || x == RepairItemTypeNumber || x == ReactorTypeNumber;
}

bool isVisibleOnCmdrsMapWithSensorType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == FlagTypeNumber || x == SoccerBallItemTypeNumber ||
         x == GoalZoneTypeNumber || x == NexusTypeNumber || x == LoadoutZoneTypeNumber || x == SlipZoneTypeNumber ||
         x == SpeedZoneTypeNumber || x == TeleportTypeNumber ||
         x == LineTypeNumber || x == TextItemTypeNumber ||
         x == AsteroidTypeNumber || x == TestItemTypeNumber || x == ResourceItemTypeNumber ||
         x == EnergyItemTypeNumber || x == RepairItemTypeNumber ||
         x == ReactorTypeNumber ||
         x == BulletTypeNumber || x == MineTypeNumber;  // Weapons visible on commander's map for sensor
}


bool isAnyObjectType(U8 x)
{
   return true;
}

////////////////////////////////////////
////////////////////////////////////////

// BfObject - the declerations are in BfObject.h

// Constructor
BfObject::BfObject()
{
   mGame = NULL;
   mObjectTypeNumber = UnknownTypeNumber;
}


void BfObject::addToGame(Game *game, GridDatabase *database)
{   
   TNLAssert(mGame == NULL, "Error: Object already in a game in GameObject::addToGame.");
   TNLAssert(game != NULL,  "Error: theGame is NULL in GameObject::addToGame.");

   mGame = game;
   addToDatabase(database);
}


void BfObject::removeFromGame()
{
   removeFromDatabase();
   mGame = NULL;
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

void BfObject::readThisTeam(BitStream *stream)
{
   mTeam = stream->readInt(4) - 2;
}
void BfObject::writeThisTeam(BitStream *stream)
{
   stream->writeInt(mTeam + 2, 4);
}


////////////////////////////////////////
////////////////////////////////////////
// Constructor
GameObject::GameObject() : BfObject()
{
   mGame = NULL;
   mTeam = -1;
   /*mLastQueryId = 0;*/
   mDisableCollisionCount = 0;
   mCreationTime = 0;
}

void GameObject::setControllingClient(GameConnection *c)         // This only gets run on the server
{
	mControllingClient = c;
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
   mObjectTypeNumber = DeletedTypeNumber;

   if(!mGame)                    // Not in a game
      delete this;
   else
      mGame->addToDeleteList(this, deleteTimeInterval);
}


Point GameObject::getRenderPos() const
{
   return getExtent().getCenter();
}


Point GameObject::getActualPos() const
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
   GameObject *so = dynamic_cast<GameObject *>(scopeObject);
   F32 add = 0;
   if(so) // GameType is not GameObject, and GameType don't have position
   {
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


      // initial scoping factor is distance based.
      add += (500 - distance) / 500;

      // give some extra love to things that are moving towards the scope object
      if(deltav.dot(deltap) < 0)
         add += 0.7f;
   }

   // and a little more love if this object has not yet been scoped.
   if(updateMask == 0xFFFFFFFF)
      add += 0.5;
   return add + updateSkips * 0.5f;
}

void GameObject::damageObject(DamageInfo *theInfo)
{

}


// Returns number of ships hit
S32 GameObject::radiusDamage(Point pos, S32 innerRad, S32 outerRad, TestFunc objectTypeTest, DamageInfo &info, F32 force)
{
   // Check for players within range.  If so, blast them to little tiny bits!
   // Those within innerRad get full force of the damage.  Those within outerRad get damage prop. to distance
   Rect queryRect(pos, pos);
   queryRect.expand(Point(outerRad, outerRad));

   fillVector.clear();
   findObjects(objectTypeTest, fillVector, queryRect);

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

      if(findObjectLOS((TestFunc)isWallType, MoveObject::ActualState, pos, objPos, t, n))
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


      if(isShipType(foundObject->getObjectTypeNumber()))
         shipsHit++;

      foundObject->damageObject(&localInfo); 
   }

   return shipsHit;
}


void GameObject::findObjects(TestFunc objectTypeTest, Vector<DatabaseObject *> &fillVector, const Rect &ext)
{
   GridDatabase *gridDB = getDatabase();
   if(!gridDB)
      return;

   gridDB->findObjects(objectTypeTest, fillVector, ext);
}


void GameObject::findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect &ext)
{
   GridDatabase *gridDB = getDatabase();
   if(!gridDB)
      return;

   gridDB->findObjects(typeNumber, fillVector, ext);
}


GameObject *GameObject::findObjectLOS(U8 typeNumber, U32 stateIndex, Point rayStart, Point rayEnd,
                                      float &collisionTime, Point &collisionNormal)
{
   GridDatabase *gridDB = getDatabase();

   if(!gridDB)
      return NULL;

   return dynamic_cast<GameObject *>(
         gridDB->findObjectLOS(typeNumber, stateIndex, rayStart, rayEnd, collisionTime, collisionNormal)
         );
}


GameObject *GameObject::findObjectLOS(TestFunc objectTypeTest, U32 stateIndex, Point rayStart, Point rayEnd,
                                      float &collisionTime, Point &collisionNormal)
{
   GridDatabase *gridDB = getDatabase();

   if(!gridDB)
      return NULL;

   return dynamic_cast<GameObject *>(
         gridDB->findObjectLOS(objectTypeTest, stateIndex, rayStart, rayEnd, collisionTime, collisionNormal)
         );
}


void GameObject::addToGame(Game *game, GridDatabase *database)
{
   BfObject::addToGame(game, database);
   // constists of:
   //    mGame = game;
   //    addToDatabase();

   setCreationTime(game->getCurrentTime());
   onAddedToGame(game);
}


void GameObject::onAddedToGame(Game *)
{
   getGame()->mObjectsLoaded++;
}


Rect GameObject::getBounds(U32 stateIndex) const
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
   else if(getCollisionRect(stateIndex, ret))
   {
      /* Nothing to do -- ret is populated by getCollisionRect() */
   }

   return ret;
}


// Find if the specified point is in theObject's collisionPoly or collisonCircle
bool GameObject::collisionPolyPointIntersect(Point point)
{
   Point center;
   F32 radius;
   Rect rect;

   Vector<Point> polyPoints;

   polyPoints.clear();

   if(getCollisionPoly(polyPoints))
      return PolygonContains2(polyPoints.address(), polyPoints.size(), point);

   else if(getCollisionCircle(MoveObject::ActualState, center, radius))
      return(center.distanceTo(point) <= radius);

   else if(getCollisionRect(MoveObject::ActualState, rect))
      return rect.contains(point);

   else
      return false;
}


// Find if the specified polygon intersects theObject's collisionPoly or collisonCircle
bool GameObject::collisionPolyPointIntersect(Vector<Point> points)
{
   Point center;
   Rect rect;
   F32 radius;
   Vector<Point> polyPoints;

   polyPoints.clear();

   if(getCollisionPoly(polyPoints))
      return polygonsIntersect(polyPoints, points);

   else if(getCollisionCircle(MoveObject::ActualState, center, radius))
   {
      Point unusedPt;
      return polygonCircleIntersect(&points[0], points.size(), center, radius * radius, unusedPt);
   }
   else if(getCollisionRect(MoveObject::ActualState, rect))
      return rect.intersects(points);

   else
      return false;
}


// Find if the specified polygon intersects theObject's collisionPoly or collisonCircle
bool GameObject::collisionPolyPointIntersect(Point center, F32 radius)
{
   Point c, pt;
   float r;
   Rect rect;
   static Vector<Point> polyPoints;

   polyPoints.clear();

   if(getCollisionPoly(polyPoints))
      return polygonCircleIntersect(&polyPoints[0], polyPoints.size(), center, radius * radius, pt);

   else if(getCollisionCircle(MoveObject::ActualState, c, r))
      return ( center.distSquared(c) < (radius + r) * (radius + r) );

   else if(getCollisionRect(MoveObject::ActualState, rect))
      return rect.intersects(center, radius);

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
      F32 magnitude = (F32)stream->readRangedU32(0, max);
      vel.set(cos(theta) * magnitude, sin(theta) * magnitude);
   }
}


void GameObject::onGhostAddBeforeUpdate(GhostConnection *theConnection)
{
   // Some unpackUpdate need getGame() available.
   GameConnection *gc = (GameConnection *)(theConnection);  // GhostConnection is always GameConnection
   TNLAssert(theConnection && gc->mClientGame, "Should only be client here!");
#ifndef ZAP_DEDICATED
   mGame = gc->mClientGame;
#endif
}

bool GameObject::onGhostAdd(GhostConnection *theConnection)
{
   GameConnection *gc = (GameConnection *)(theConnection);  // GhostConnection is always GameConnection
   TNLAssert(theConnection && gc->mClientGame, "Should only be client here!");
#ifndef ZAP_DEDICATED

#ifdef TNL_ENABLE_ASSERTS
   mGame = NULL;  // prevent false asserts
#endif

   // for performance, add to GridDatabase after update, to avoid slowdown from adding to database with zero points or (0,0) then moving
   addToGame(gc->mClientGame, gc->mClientGame->getGameObjDatabase());
#endif
   return true;
}


S32 GameObject::getTeamIndx(lua_State *L)  // Return item team to Lua
{
   return LuaObject::returnInt(L, mTeam + 1);
}


void GameObject::push(lua_State *L)       // Lua-aware classes will implement this
{
   TNLAssert(false, "Unimplemented push function!");
}

};

