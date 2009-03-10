//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#include "gameItems.h"
#include "item.h"
#include "ship.h"
#include "gameObjectRender.h"
#include "../glut/glutInclude.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(RepairItem);

// Constructor
RepairItem::RepairItem(Point p) : PickupItem(p, 20)
{
   mNetFlags.set(Ghostable);
   mRepopDelay = 20000;       // Default to 20 seconds
}


void RepairItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 2)
      return;
   else
      Parent::processArguments(argc, argv);

   if(argc == 3)
   {
      S32 repopDelay = atoi(argv[2]) * 1000;    // 3rd param is time for this to regenerate in seconds
      if(repopDelay > 0)
         mRepopDelay = repopDelay;
   }
}


bool RepairItem::pickup(Ship *theShip)
{
   if(theShip->getHealth() >= 1)
      return false;

   DamageInfo di;
   di.damageAmount = -0.5f;      // Negative damage = repair!
   di.damageType = DamageTypePoint;
   di.damagingObject = this;

   theShip->damageObject(&di);
   return true;
}

void RepairItem::onClientPickup()
{
   SFXObject::play(SFXShipHeal, getRenderPos(), getRenderVel());
}


U32 RepairItem::getRepopDelay()
{
   return mRepopDelay;     // 20 seconds until the health item reappears
}


void RepairItem::renderItem(Point pos)
{
   if(!isVisible())
      return;
   renderRepairItem(pos, false);
}



TNL_IMPLEMENT_NETOBJECT(Asteroid);

// Constructor
Asteroid::Asteroid() : Item(Point(0,0), true, AsteroidRadius, 4)
{
   mNetFlags.set(Ghostable);
   mObjectTypeMask |= AsteroidType;
   mSizeIndex = 0;     // Higher = smaller
   hasExploded = false;
   mDesign = TNL::Random::readI(0, AsteroidDesigns - 1);
}


void Asteroid::renderItem(Point pos)
{
   if(!hasExploded)
      renderAsteroid(pos, mDesign, mRenderSize[mSizeIndex]);
}


bool Asteroid::getCollisionCircle(U32 state, Point center, F32 radius)
{
   center = mMoveState[state].pos;
   radius = AsteroidRadius * mRenderSize[mSizeIndex];
   return true;
}


// This would be better, but causes crashes :-(
bool Asteroid::getCollisionPoly(U32 state, Vector<Point> &polyPoints)
{
   //for(S32 i = 0; i < AsteroidPoints; i++)
   //{
   //   Point p = Point(mMoveState[state].pos.x + (F32) AsteroidCoords[mDesign][i][0] * mRenderSize[mSizeIndex],
   //                   mMoveState[state].pos.y + (F32) AsteroidCoords[mDesign][i][1] * mRenderSize[mSizeIndex] );

   //   polyPoints.push_back(p);
   //}

   return false;
}


void Asteroid::damageObject(DamageInfo *theInfo)
{
   // Compute impulse direction
   mSizeIndex++;
   if(mRenderSize[mSizeIndex] == -1)    // Kill small items
   {
      hasExploded = true;
      deleteObject(500);
      return;
   }

   setMaskBits(ItemChangedMask);    // So our clients will get new size
   setRadius(AsteroidRadius * mRenderSize[mSizeIndex]);

   Point dv = theInfo->impulseVector - mMoveState[ActualState].vel;
   Point iv = mMoveState[ActualState].pos - theInfo->collisionPoint;
   iv.normalize();
   mMoveState[ActualState].vel += iv * dv.dot(iv) * 0.3;

   Asteroid *newItem = dynamic_cast<Asteroid *>(TNL::Object::create("Asteroid"));
   newItem->setRadius(AsteroidRadius * mRenderSize[mSizeIndex]);

   for(U32 i = 0; i < MoveStateCount; i++)
   {
      newItem->mMoveState[i].pos = mMoveState[i].pos;
      newItem->mMoveState[i].angle = mMoveState[i].angle + FloatHalfPi;
      newItem->mMoveState[i].vel.x = mMoveState[i].vel.x * TNL::Random::readF();
      newItem->mMoveState[i].vel.y = mMoveState[i].vel.y * TNL::Random::readF();
      newItem->mMoveState[i].vel.normalize(mMoveState[i].vel.len());
   }

   newItem->mSizeIndex = mSizeIndex;
   newItem->addToGame(gServerGame);                    // And add it to the list of game objects
}

   
U32 Asteroid::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);

   if(stream->writeFlag(updateMask & ItemChangedMask))
      stream->writeEnum(mSizeIndex, mSizeIndexLength);

   stream->writeFlag(hasExploded);
   
   return retMask;
}


void Asteroid::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
   {
      mSizeIndex = stream->readEnum(mSizeIndexLength);
      setRadius(AsteroidRadius * mRenderSize[mSizeIndex]);
      mDesign = TNL::Random::readI(0, AsteroidDesigns - 1);     // No need to sync between client and server or between clients

      if(!mInitial)
         SFXObject::play(SFXShipExplode, mMoveState[ActualState].pos, Point());
   }

   bool explode = (stream->readFlag());     // Exploding!  Take cover!!
   
   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();
      emitAsteroidExplosion(mMoveState[ActualState].pos);
   }
}


bool Asteroid::collide(GameObject *otherObject)
{
   // Asteroids don't collide with one another!
   if(dynamic_cast<Asteroid *>(otherObject))
      return false;

   return true;
}


void Asteroid::emitAsteroidExplosion(Point pos)
{
   SFXObject::play(SFXShipExplode, pos, Point());

   //F32 a = TNL::Random::readF() * 0.4 + 0.5;
   //F32 b = TNL::Random::readF() * 0.2 + 0.9;

   //F32 c = TNL::Random::readF() * 0.15 + 0.125;
   //F32 d = TNL::Random::readF() * 0.2 + 0.9;

   //FXManager::emitExplosion(mMoveState[ActualState].pos, 0.9, ShipExplosionColors, NumShipExplosionColors);
   FXManager::emitBurst(pos, Point(.5, .5), Color(1,1,1), Color(1,1, 1));
   //FXManager::emitBurst(pos, Point(b,d), Color(1,1,0), Color(0,0.75,0));
}


TNL_IMPLEMENT_NETOBJECT(TestItem);

// Constructor
TestItem::TestItem() : Item(Point(0,0), true, 60, 4)
{
   mNetFlags.set(Ghostable);
   mObjectTypeMask |= TestItemType | TurretTargetType;
}

void TestItem::renderItem(Point pos)
{
   renderTestItem(pos);
}

void TestItem::damageObject(DamageInfo *theInfo)
{
   // Compute impulse direction
   Point dv = theInfo->impulseVector - mMoveState[ActualState].vel;
   Point iv = mMoveState[ActualState].pos - theInfo->collisionPoint;
   iv.normalize();
   mMoveState[ActualState].vel += iv * dv.dot(iv) * 0.3;
}

bool TestItem::getCollisionPoly(U32 state, Vector<Point> &polyPoints)
{
   //for(S32 i = 0; i < 8; i++)    // 8 so that first point gets repeated!  Needed?  Maybe not
   //{
   //   Point p = Point(60 * cos(i * Float2Pi / 7 + FloatHalfPi) + mMoveState[ActualState].pos.x, 60 * sin(i * Float2Pi / 7 + FloatHalfPi) + mMoveState[ActualState].pos.y);
   //   polyPoints.push_back(p);
   //}

   return false;
}


TNL_IMPLEMENT_NETOBJECT(ResourceItem);

// Constructor
ResourceItem::ResourceItem() : Item(Point(0,0), true, 20, 1)     
{
   mNetFlags.set(Ghostable);
   mObjectTypeMask |= ResourceItemType | TurretTargetType;
}

void ResourceItem::renderItem(Point pos)
{
   renderResourceItem(pos);
}

bool ResourceItem::collide(GameObject *hitObject)
{
   if(mIsMounted)
      return false;

   if(!(hitObject->getObjectTypeMask() & ShipType))
      return true;

   Ship *ship = dynamic_cast<Ship *>(hitObject);
   if(ship->hasExploded)
      return false;

   if(ship->hasModule(ModuleEngineer) && !ship->carryingResource())
   {
      if(!isGhost())
         mountToShip(ship);
      return false;
   }
   return true;
}

void ResourceItem::damageObject(DamageInfo *theInfo)
{
   // Compute impulse direction
   Point dv = theInfo->impulseVector - mMoveState[ActualState].vel;
   Point iv = mMoveState[ActualState].pos - theInfo->collisionPoint;
   iv.normalize();
   mMoveState[ActualState].vel += iv * dv.dot(iv) * 0.3;
}


};

