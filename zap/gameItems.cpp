
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


bool RepairItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 2)
      return false;
   else if(!Parent::processArguments(argc, argv))
      return false;

   if(argc == 3)
   {
      S32 repopDelay = atoi(argv[2]) * 1000;    // 3rd param is time for this to regenerate in seconds
      if(repopDelay > 0)
         mRepopDelay = repopDelay;
   }

   return true;
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


const char RepairItem::className[] = "RepairItem";      // Class name as it appears to Lua scripts

// Lua constructor
RepairItem::RepairItem(lua_State *L)
{
   // Do nothing, for now...  should take params from stack and create RepairItem object
}


// Define the methods we will expose to Lua
Lunar<RepairItem>::RegType RepairItem::methods[] =
{
   // Standard gameItem methods
   method(RepairItem, getClassID),
   method(RepairItem, getLoc),
   method(RepairItem, getRad),
   method(RepairItem, getVel),

   // Class specific methods
   method(RepairItem, isVis),
   {0,0}    // End method list
};

S32 RepairItem::isVis(lua_State *L) { return returnBool(L, isVisible()); }        // Is RepairItem visible? (returns boolean)
S32 RepairItem::getLoc(lua_State *L) { return returnPoint(L, getActualPos()); }   // Center of RepairItem (returns point)
S32 RepairItem::getRad(lua_State *L) { return returnFloat(L, getRadius()); }      // Radius of RepairItem (returns number)
S32 RepairItem::getVel(lua_State *L) { return returnPoint(L, getActualVel()); }   // Speed of RepairItem (returns point)



TNL_IMPLEMENT_NETOBJECT(Asteroid);
class LuaAsteroid;

// Constructor
Asteroid::Asteroid() : Item(Point(0,0), true, AsteroidRadius, 4)
{
   mNetFlags.set(Ghostable);
   mObjectTypeMask |= AsteroidType;
   mSizeIndex = 0;     // Higher = smaller
   hasExploded = false;
   mDesign = TNL::Random::readI(0, AsteroidDesigns - 1);

   // Give the asteroids some intial motion in a random direction
   F32 ang = TNL::Random::readF() * Float2Pi;
   F32 vel = 50;

   for(U32 i = 0; i < MoveStateCount; i++)
   {
      mMoveState[i].vel.x = vel * cos(ang);
      mMoveState[i].vel.y = vel * sin(ang);
   }
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
   mMoveState[ActualState].vel += iv * dv.dot(iv) * 0.5;

   Asteroid *newItem = dynamic_cast<Asteroid *>(TNL::Object::create("Asteroid"));
   newItem->setRadius(AsteroidRadius * mRenderSize[mSizeIndex]);
   F32 ang = 0;
   while(abs(ang) < .5)
      ang = TNL::Random::readF() * FloatPi - FloatHalfPi;

   for(U32 i = 0; i < MoveStateCount; i++)
   {
      newItem->mMoveState[i].pos = mMoveState[i].pos;
      newItem->mMoveState[i].angle = mMoveState[i].angle;
      newItem->mMoveState[i].vel.x = mMoveState[i].vel.len() * cos(ang);
      newItem->mMoveState[i].vel.y = mMoveState[i].vel.len() * sin(ang);
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
         SFXObject::play(SFXShipExplode, mMoveState[RenderState].pos, Point());
   }

   bool explode = (stream->readFlag());     // Exploding!  Take cover!!

   if(explode && !hasExploded)
   {
      hasExploded = true;
      disableCollision();
      emitAsteroidExplosion(mMoveState[RenderState].pos);
   }
}


bool Asteroid::collide(GameObject *otherObject)
{
   // Asteroids don't collide with one another!
   return dynamic_cast<Asteroid *>(otherObject) ? false : true;
}


void Asteroid::emitAsteroidExplosion(Point pos)
{
   SFXObject::play(SFXShipExplode, pos, Point());
   // FXManager::emitBurst(pos, Point(.1, .1), Color(1,1,1), Color(1,1,1), 10);
}



const char Asteroid::className[] = "Asteroid";      // Class name as it appears to Lua scripts

// Lua constructor
Asteroid::Asteroid(lua_State *L)
{
   // Do we want to construct these from Lua?  If so, do that here!
}


// Define the methods we will expose to Lua
Lunar<Asteroid>::RegType Asteroid::methods[] =
{
   // Standard gameItem methods
   method(Asteroid, getClassID),
   method(Asteroid, getLoc),
   method(Asteroid, getRad),
   method(Asteroid, getVel),

   // Class specific methods
   method(Asteroid, getSize),
   method(Asteroid, getSizeCount),

   {0,0}    // End method list
};


S32 Asteroid::getSize(lua_State *L) { return returnInt(L, getSizeIndex()); }         // Index of current asteroid size (0 = initial size, 1 = next smaller, 2 = ...) (returns int)
S32 Asteroid::getSizeCount(lua_State *L) { return returnInt(L, getSizeCount()); }    // Number of indexes of size we can have (returns int)
S32 Asteroid::getLoc(lua_State *L) { return returnPoint(L, getActualPos()); }        // Center of asteroid (returns point)
S32 Asteroid::getRad(lua_State *L) { return returnFloat(L, getRadius()); }           // Radius of asteroid (returns number)
S32 Asteroid::getVel(lua_State *L) { return returnPoint(L, getActualVel()); }        // Speed of asteroid (returns point)



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


const char TestItem::className[] = "TestItem";      // Class name as it appears to Lua scripts

// Lua constructor
TestItem::TestItem(lua_State *L)
{
   // Do nothing, for now...  should take params from stack and create testItem object
}


// Define the methods we will expose to Lua
Lunar<TestItem>::RegType TestItem::methods[] =
{
   // Standard gameItem methods
   method(TestItem, getClassID),
   method(TestItem, getLoc),
   method(TestItem, getRad),
   method(TestItem, getVel),

   {0,0}    // End method list
};

S32 TestItem::getLoc(lua_State *L) { return returnPoint(L, getActualPos()); }   // Center of testItem (returns point)
S32 TestItem::getRad(lua_State *L) { return returnFloat(L, getRadius()); }      // Radius of testItem (returns number)
S32 TestItem::getVel(lua_State *L) { return returnPoint(L, getActualVel()); }   // Speed of testItem (returns point)



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

   if( ! (hitObject->getObjectTypeMask() & (ShipType | RobotType)))
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


const char ResourceItem::className[] = "ResourceItem";      // Class name as it appears to Lua scripts

// Lua constructor
ResourceItem::ResourceItem(lua_State *L)
{
   // Do nothing, for now...  should take params from stack and create testItem object
}


// Define the methods we will expose to Lua
Lunar<ResourceItem>::RegType ResourceItem::methods[] =
{
   // Standard gameItem methods
   method(ResourceItem, getClassID),
   method(ResourceItem, getLoc),
   method(ResourceItem, getRad),
   method(ResourceItem, getVel),

   {0,0}    // End method list
};

S32 ResourceItem::getLoc(lua_State *L) { return returnPoint(L, getActualPos()); }   // Center of testItem (returns point)
S32 ResourceItem::getRad(lua_State *L) { return returnFloat(L, getRadius()); }      // Radius of testItem (returns number)
S32 ResourceItem::getVel(lua_State *L) { return returnPoint(L, getActualVel()); }   // Speed of testItem (returns point)

};

