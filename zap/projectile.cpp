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

#include "gameWeapons.h"
#include "projectile.h"
#include "ship.h"
#include "sparkManager.h"
#include "sfx.h"
#include "gameObject.h"
#include "gameObjectRender.h"
#include "../glut/glutInclude.h"

namespace Zap
{

static Vector<GameObject*> fillVector;


TNL_IMPLEMENT_NETOBJECT(Projectile);

// Constructor
Projectile::Projectile(WeaponType type, Point p, Point v, U32 time, GameObject *shooter)
{
   mObjectTypeMask = BulletType;

   mNetFlags.set(Ghostable);
   pos = p;
   velocity = v;
   mTimeRemaining = time;
   collided = false;
   alive = true;
   mShooter = shooter;
   if(shooter)
   {
      setOwner(shooter->getOwner());
      mTeam = shooter->getTeam();
   }
   mType = gWeapons[type].projectileType;
   mWeaponType = type;
}


U32 Projectile::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
   {
      ((GameConnection *) connection)->writeCompressedPoint(pos, stream);
      writeCompressedVelocity(velocity, CompressedVelocityMax, stream);

      stream->writeEnum(mType, ProjectileTypeCount);

      S32 index = -1;
      if(mShooter.isValid())
         index = connection->getGhostIndex(mShooter);
      if(stream->writeFlag(index != -1))
         stream->writeInt(index, GhostConnection::GhostIdBitSize);
   }
   stream->writeFlag(collided);
   stream->writeFlag(alive);

   return 0;
}


void Projectile::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;

   if(stream->readFlag())     // Initial chunk of data, sent once for this object
   {
      ((GameConnection *) connection)->readCompressedPoint(pos, stream);
      readCompressedVelocity(velocity, CompressedVelocityMax, stream);

      mType = (ProjectileType) stream->readEnum(ProjectileTypeCount);

      Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
      if(ship && ship->hasModule(ModuleSensor))
         mObjectTypeMask |= CommandMapVisType;     // Bullets visible on commander's map if you have sensor

      if(stream->readFlag())
         mShooter = dynamic_cast<Ship *>(connection->resolveGhost(stream->readInt(GhostConnection::GhostIdBitSize)));
      pos += velocity * -0.020f; // What's this about?? -CE
      Rect newExtent(pos,pos);
      setExtent(newExtent);
      initial = true;
      SFXObject::play(gProjInfo[mType].projectileSound, pos, velocity);
   }
   bool preCollided = collided;
   collided = stream->readFlag();
   alive = stream->readFlag();

   if(!preCollided && collided)
      explode(NULL, pos);

   if(!collided && initial)
   {
      mCurrentMove.time = U32(connection->getOneWayTime());
     //idle(GameObject::ClientIdleMainRemote);   // not CE
   }
}

void Projectile::handleCollision(GameObject *hitObject, Point collisionPoint)
{
   collided = true;

   if(!isGhost())
   {
      DamageInfo theInfo;
      theInfo.collisionPoint = collisionPoint;
      theInfo.damageAmount = gWeapons[mWeaponType].damageAmount;
      theInfo.damageType = DamageTypePoint;
      theInfo.damagingObject = this;
      theInfo.impulseVector = velocity;

      hitObject->damageObject(&theInfo);
   }

   mTimeRemaining = 0;
   explode(hitObject, collisionPoint);
}


void Projectile::idle(GameObject::IdleCallPath path)
{
   U32 deltaT = mCurrentMove.time;

   if(!collided && alive)
   {
   // Calculate where projectile will be at the end of the current interval
   Point endPos = pos + velocity * (F32)deltaT * 0.001;

      // Check for collision along projected route of movement
      static Vector<GameObject *> disableVector;

      Rect queryRect(pos, endPos);     // Bounding box of our travels

      disableVector.clear();

      U32 aliveTime = getGame()->getCurrentTime() - getCreationTime();  // Age of object, in ms

      // Don't collide with shooter during first 500ms of life
      if(mShooter.isValid() && aliveTime < 500)
      {
         disableVector.push_back(mShooter);
         mShooter->disableCollision();
      }

      GameObject *hitObject;
      float collisionTime;
      Point surfNormal;

      // Do the search
      for(;;)
      {
         hitObject = findObjectLOS(MoveableType | BarrierType | EngineeredType | ForceFieldType,
                                   MoveObject::RenderState, pos, endPos, collisionTime, surfNormal);
         if(!hitObject || hitObject->collide(this))
            break;

         // Disable collisions with things that don't want to be
         // collided with (i.e. whose collide methods return false)
         disableVector.push_back(hitObject);
         hitObject->disableCollision();
      }

      // Re-enable collison flag for ship and items in our path that don't want to be collided with
      // Note that if we hit an object that does want to be collided with, it won't be in disableVector
      // and thus collisions will not have been disabled, and thus don't need to be re-enabled.

      for(S32 i = 0; i < disableVector.size(); i++)
         disableVector[i]->enableCollision();

      if(hitObject)  // Hit something...  should we bounce?
      {
         bool bounce = false;
         U32 typeMask = hitObject->getObjectTypeMask();

         if(mType == ProjectileBounce && (typeMask & BarrierType))
            bounce = true;
         else if(typeMask & (ShipType | RobotType))
         {
            Ship *s = dynamic_cast<Ship *>(hitObject);
            if(s->isModuleActive(ModuleShield))
               bounce = true;
         }

         if(bounce)
         {
            // We hit something that we should bounce from, so bounce!
            velocity -= surfNormal * surfNormal.dot(velocity) * 2;
            Point collisionPoint = pos + (endPos - pos) * collisionTime;
            pos = collisionPoint + surfNormal;

            SFXObject::play(SFXBounceShield, collisionPoint, surfNormal * surfNormal.dot(velocity) * 2);
         }
         else
         {
            // Not bouncing, so advance to location of collision
            Point collisionPoint = pos + (endPos - pos) * collisionTime;
            handleCollision(hitObject, collisionPoint);     // What we hit, and where we hit it
         }
      }
      else        // Hit nothing, advance projectile to endPos
         pos = endPos;


      Rect newExtent(pos,pos);
      setExtent(newExtent);
   }

   // Kill old projectiles
   if(alive && path == GameObject::ServerIdleMainLoop)
   {
      if(mTimeRemaining <= deltaT)
      {
         deleteObject(500);
         mTimeRemaining = 0;
         alive = false;
         setMaskBits(ExplodedMask);
      }
      else
         mTimeRemaining -= deltaT;     // Decrement time left to live
   }
}

// Gets run when projectile suffers damage, like from a burst going off
void Projectile::damageObject(DamageInfo *info)
{
   mTimeRemaining = 0;     // This will kill projectile --> remove this to have projectiles unaffected
}


void Projectile::explode(GameObject *hitObject, Point thePos)
{
   // Do some particle spew...
   if(isGhost())
   {
      FXManager::emitExplosion(thePos, 0.3, gProjInfo[mType].sparkColors, NumSparkColors);

      Ship *s = dynamic_cast<Ship *>(hitObject);
      if(s && s->isModuleActive(ModuleShield))
         SFXObject::play(SFXBounceShield, thePos, velocity);
      else
         SFXObject::play(gProjInfo[mType].impactSound, thePos, velocity);
   }
}

void Projectile::render()
{
   if(collided || !alive)
      return;

   renderProjectile(pos, mType, getGame()->getCurrentTime() - getCreationTime());
}


//-----------------------------------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(Mine);

// Constructor (planter defaults to null)
Mine::Mine(Point pos, Ship *planter) : GrenadeProjectile(pos, Point())
{
   mObjectTypeMask |= MineType;
   mWeaponType = WeaponMine;

   if(planter)
   {
      setOwner(planter->getOwner());
      mTeam = planter->getTeam();
   }
   else
      mTeam = -2;    // Hostile to all, as mines generally are!

   mArmed = false;
}


void Mine::idle(IdleCallPath path)
{
   // Skip the grenade timing goofiness...
   Item::idle(path);

   if(exploded || path != GameObject::ServerIdleMainLoop)
      return;

   // And check for enemies in the area...
   Point pos = getActualPos();
   Rect queryRect(pos, pos);
   queryRect.expand(Point(SensorRadius, SensorRadius));

   fillVector.clear();
   findObjects(MotionTriggerTypes | MineType, fillVector, queryRect);

   // Found something!
   bool foundItem = false;
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      F32 radius;
      Point ipos;
      if(fillVector[i]->getCollisionCircle(MoveObject::RenderState, ipos, radius))
      {
         if((ipos - pos).len() < (radius + SensorRadius))
         {
            bool isMine = fillVector[i]->getObjectTypeMask() & MineType;
            if(!isMine)
            {
               foundItem = true;
               break;
            }
            else if(mArmed && fillVector[i] != this)
            {
               foundItem = true;
               break;
            }
         }
      }
   }
   if(foundItem)
   {     // braces needed
      if(mArmed)
         explode(getActualPos(), WeaponMine);
   }
   else
   {
      if(!mArmed)
      {
         setMaskBits(ArmedMask);
         mArmed = true;
      }
   }
}

bool Mine::collide(GameObject *otherObj)
{
   if(otherObj->getObjectTypeMask() & (BulletType))
      explode(getActualPos(), WeaponMine);
   return false;
}

void Mine::damageObject(DamageInfo *info)
{
   if(info->damageAmount > 0.f && !exploded)
      explode(getActualPos(), WeaponMine);
}

// Only runs on server side
U32 Mine::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 ret = Parent::packUpdate(connection, updateMask, stream);
   if(stream->writeFlag(updateMask & InitialMask))
   {
      stream->write(mTeam);

      // RDW It isn't legal to bind a reference to NULL, this shouldn't compile.
      // Well, we have to send something here, otherwise mines will crash the game (pack and unpack need to be symmetrical)
      // perhaps this is why laying mines was causing problems?
      // Anyway, how does this modification work?  Better?
      // stream->writeStringTableEntry(getOwner() ? getOwner()->getClientName() : NULL);  // <--- Original
      StringTableEntryRef noOwner = "";
      stream->writeStringTableEntry(getOwner() ? getOwner()->getClientName() : noOwner);
   }
   stream->writeFlag(mArmed);
   return ret;
}

void Mine::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())     // Initial data
   {
      initial = true;
      stream->read(&mTeam);
      stream->readStringTableEntry(&mSetBy);
   }
   bool wasArmed = mArmed;
   mArmed = stream->readFlag();

   if(initial && !mArmed)
      SFXObject::play(SFXMineDeploy, getActualPos(), Point());
   else if(!initial && !wasArmed && mArmed)
      SFXObject::play(SFXMineArm, getActualPos(), Point());
}

void Mine::renderItem(Point pos)
{
   if(exploded)
      return;

   Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());

   if(!ship)
      return;

   // Can see mine if laid by teammate in team game || sensor is active || 
   //  you laid it yourself
   bool visible = ( (ship->getTeam() == getTeam()) && getGame()->getGameType()->isTeamGame() ) || ship->isModuleActive(ModuleSensor) || 
      (getGame()->getGameType()->mLocalClient && getGame()->getGameType()->mLocalClient->name == mSetBy);

   renderMine(pos, mArmed, visible);
}

//-----------------------------------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(GrenadeProjectile);

GrenadeProjectile::GrenadeProjectile(Point pos, Point vel, U32 liveTime, GameObject *shooter)
 : Item(pos, true, 7.f, 1.f)
{
   mObjectTypeMask = MoveableType | BulletType;

   mNetFlags.set(Ghostable);

   mMoveState[ActualState].pos = pos;
   mMoveState[ActualState].vel = vel;
   setMaskBits(PositionMask);
   mWeaponType = WeaponBurst;

   updateExtent();

   ttl = liveTime;
   exploded = false;
   if(shooter)
   {
      setOwner(shooter->getOwner());
      mTeam = shooter->getTeam();
   }
}

void GrenadeProjectile::idle(IdleCallPath path)
{
   Parent::idle(path);

   // Do some drag...
   mMoveState[ActualState].vel -= mMoveState[ActualState].vel * (F32(mCurrentMove.time) / 1000.f);

   if(!exploded)
   {
      if(getActualVel().len() < 4.0)
        explode(getActualPos(), WeaponBurst);
   }

   if(isGhost()) return;

   // Update TTL
   S32 deltaT = mCurrentMove.time;
   if(path == GameObject::ClientIdleMainRemote)
      ttl += deltaT;
   else if(!exploded)
   {
      if(ttl <= deltaT)
        explode(getActualPos(), WeaponBurst);
      else
         ttl -= deltaT;
   }

}

U32  GrenadeProjectile::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 ret = Parent::packUpdate(connection, updateMask, stream);
   stream->writeFlag(exploded);
   stream->writeFlag(updateMask & InitialMask);
   return ret;
}

void GrenadeProjectile::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
   if(ship && ship->hasModule(ModuleSensor))
      mObjectTypeMask |= CommandMapVisType;     // Bursts visible on commander's map if you have sensor

   if(stream->readFlag())
   {
      explode(getActualPos(), WeaponBurst);
   }

   if(stream->readFlag())
   {
      SFXObject::play(SFXGrenadeProjectile, getActualPos(), getActualVel());
   }
}

void GrenadeProjectile::damageObject(DamageInfo *theInfo)
{
   // If we're being damaged by another grenade, explode...
   if(theInfo->damageType == DamageTypeArea)
   {
      explode(getActualPos(), WeaponBurst);
      return;
   }

   // Bounce off of stuff.
   Point dv = theInfo->impulseVector - mMoveState[ActualState].vel;
   Point iv = mMoveState[ActualState].pos - theInfo->collisionPoint;
   iv.normalize();
   mMoveState[ActualState].vel += iv * dv.dot(iv) * 0.3;

   setMaskBits(PositionMask);
}

// Also used for mines and spybugs  --> not sure if we really need to pass weaponType
void GrenadeProjectile::explode(Point pos, WeaponType weaponType)
{
   if(exploded) return;

   if(isGhost())
   {
      // Make us go boom!
      Color b(1,1,1);

      //FXManager::emitExplosion(getRenderPos(), 0.5, gProjInfo[ProjectilePhaser].sparkColors, NumSparkColors);      // Original, nancy explosion
      FXManager::emitBlast(pos, OuterBlastRadius);          // New, manly explosion

      SFXObject::play(SFXMineExplode, getActualPos(), Point());
   }

   disableCollision();

   if(!isGhost())
   {
      setMaskBits(PositionMask);
      deleteObject(100);

      DamageInfo info;
      info.collisionPoint = pos;
      info.damagingObject = this;
      info.damageAmount   = gWeapons[weaponType].damageAmount;
      info.damageType     = DamageTypeArea;

      radiusDamage(pos, InnerBlastRadius, OuterBlastRadius, DamagableTypes, info);
   }

   exploded = true;
}

void GrenadeProjectile::renderItem(Point pos)
{
   if(exploded)
      return;

   // Add some sparks... this needs work, as it is rather dooky  Looks too much like a comet!
   //S32 num = Random::readI(1, 10);
   //for(S32 i = 0; i < num; i++)
   //{
   //   Point sparkVel = mMoveState[RenderState].vel * Point(Random::readF() * -.5 + .55, Random::readF() * -.5 + .55);
   //   //sparkVel.normalize(Random::readF());
   //   FXManager::emitSpark(pos, sparkVel, Color(Random::readF() *.5 +.5, Random::readF() *.5, 0), Random::readF() * 2, FXManager::SparkTypePoint);
   //}

   ShipWeaponInfo *wi = gWeapons + WeaponBurst;
   F32 initTTL = (F32) wi->projLiveTime;
   renderGrenade( pos, (initTTL - (F32) (getGame()->getCurrentTime() - getCreationTime())) / initTTL );
}



////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////




TNL_IMPLEMENT_NETOBJECT(HeatSeeker);


HeatSeeker::HeatSeeker(Point pos, Point vel, U32 liveTime, GameObject *shooter)

{
   mObjectTypeMask = MoveableType | HeatSeekerType; // BulletType;
   mWeaponType = WeaponHeatSeeker;
   mNetFlags.set(Ghostable);

   mMoveState[ActualState].pos = pos;
   mMoveState[ActualState].vel = vel;
   setMaskBits(PositionMask);

   updateExtent();

   mTimeRemaining = liveTime;
   collided = false;
   alive = true;
   mHasTarget = false;

   mShooter = shooter;

   if(shooter)
   {
      setOwner(shooter->getOwner());
      mTeam = shooter->getTeam();
   }
}




extern bool pointInTriangle(Point p, Point a, Point b, Point c);
extern bool PolygonContains2(const Point *inVertices, int inNumVertices, const Point &inPoint);
#define SIGN(x)   ((x) < 0 ? -1 : (x) == 0 ? 0 : 1)
#define ABS(x) (((x) > 0) ? (x) : -(x))
#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

void HeatSeeker::idle(GameObject::IdleCallPath path)  // Not done yet
{

   //Parent::idle(path);     // Runs Item's idle routine


   U32 deltaT = mCurrentMove.time;

   if(!collided && alive)
   {
      mHasTarget = false;

      // Steer heat seeker by modifying direction (but not magnitude) of velocity

      // Find target: hottest item within searchAngle of straight ahead
      F32 searchAngle = FloatPi * 0.25f;  // radians, of course
      F32 maxAdjustmentAngle = 1.0f;      // Fastest seekers can turn, in radians/sec

      Point velocity = mMoveState[ActualState].vel;
      Point pos = mMoveState[ActualState].pos;

      F32 speed = velocity.len();
      F32 angle = velocity.ATAN2();       // Angle projectile is travelling

      F32 distLeft = speed * mTimeRemaining * 0.001;

      Point vertex2( distLeft * cos(angle + searchAngle) + pos.x, distLeft * sin(angle + searchAngle) + pos.y );
      Point vertex3( distLeft * cos(angle - searchAngle) + pos.x, distLeft * sin(angle - searchAngle) + pos.y );

      Vector<Point> searchTriangle;
      searchTriangle.clear();    // Really needed?

      searchTriangle.push_back(pos);      // First vertex -- current location
      searchTriangle.push_back(vertex2);  // Next two represent furthest points we could hit
      searchTriangle.push_back(vertex3);  // within searchAngle of our current heading

      // Find all potential targets within searchTriangle

      fillVector.clear();           // This vector will hold any potential targets

      // Since we can only search for objects within a rectangular area, we'll create a bounding box
      // for our triangle, search against that, then winnow any matches with a more precise comparison

      Point minXY(min(pos.x, min(vertex2.x, vertex3.x)), min(pos.y, min(vertex2.y, vertex3.y)));
      Point maxXY(max(pos.x, max(vertex2.x, vertex3.x)), max(pos.y, max(vertex2.y, vertex3.y)));
      Rect boundingBox(minXY, maxXY);

      findObjects(TurretTargetType, fillVector, boundingBox);

      F32 collisionTime;
      Point normalPoint;

      GameObject *hottestObject = NULL;
      F32 howHotIsIt = -1;

      for(S32 i=0; i<fillVector.size(); i++)
      {
         GameObject *obj = fillVector[i];

         // First, make sure object is really in our searchTriangle
         if ( !PolygonContains2(searchTriangle.address(), searchTriangle.size(), obj->getActualPos()) )
         //if (!pointInTriangle(obj->getActualPos(), pos, vertex2, vertex3))
            continue;      // Obj not really in range

         // Now, make sure we have a clear line of site to potential target
         // Open question: Should forcefields block heat seekers as if they were walls?
         if(findObjectLOS(BarrierType|ForceFieldType, MoveObject::ActualState, pos,     \
                       obj->getActualPos(), collisionTime, normalPoint))
            continue;      // Way is blocked

         // Nothing blocking us, calculate heat intensity
         // Heat falls off proprotional to distance^2, so we will find the object
         // that minimizes heat / dist^2 within our searchTriangle

         F32 d = pos.distanceTo(obj->getActualPos());
         F32 heat = d;

         if (heat > howHotIsIt)
         {
            hottestObject = obj;
            howHotIsIt = heat;
         }

      }

      // Now that we know what the seeker wants to attack, turn towards the target
      if (hottestObject)
      {
         // Probably wrong, but if time increments are small enough it should work OK
         // If game stuttered, it could allow the seeker to turn very sharply

         mTarget = hottestObject->getActualPos();
         mHasTarget = true;

         F32 adjustmentAngle = min(ABS(pos.angleTo(hottestObject->getActualPos()) - angle), maxAdjustmentAngle *  (F32)deltaT * 0.001);

         F32 adjustedAngle = angle + adjustmentAngle * SIGN(pos.angleTo(hottestObject->getActualPos()) - angle);

         velocity.set(speed * cos(adjustedAngle), speed * sin(adjustedAngle));
         mMoveState[ActualState].vel = velocity;

      }


      // Calculate where projectile will be at the end of the current interval
      Point endPos = pos + velocity * (F32)deltaT * 0.001;

      // Check for collision along projected route of movement
      static Vector<GameObject *> disableVector;

      Rect queryRect(pos, endPos);

      disableVector.clear();

      // Don't collide with self in first 500ms of projectile's life
      U32 aliveTime = getGame()->getCurrentTime() - getCreationTime();
      if(mShooter.isValid() && aliveTime < 500)
      {
         disableVector.push_back(mShooter);
         mShooter->disableCollision();
      }

      // And don't hit self
      disableVector.push_back(this);
      this->disableCollision();


      GameObject *hitObject;
      Point surfNormal;

      // Do the search
      for(;;)
      {
         hitObject = findObjectLOS(MoveableType | BarrierType | EngineeredType | ForceFieldType, \
            MoveObject::RenderState, pos, endPos, collisionTime, surfNormal);

         if(!hitObject || hitObject->collide(this))  // If we hit nothing, or something that
            break;                                   // wants to be collided with, drop out of loop

         // We hit something that does not want to be collided with
         // (i.e. whose collide methods return false)
         // Disable collisions with this object and keep searching
         disableVector.push_back(hitObject);
         hitObject->disableCollision();

      }

      // Re-enable collison flag for ship and items in our path that don't want to be collided with
      // Note that if we hit an object that does want to be collided with, it won't be in disableVector
      // and thus collisions will not have been disabled, and thus don't need to be re-enabled.
      for(S32 i = 0; i < disableVector.size(); i++)
         disableVector[i]->enableCollision();

      if(hitObject)
      {
         Point collisionPoint = pos + (endPos - pos) * collisionTime;
         handleCollision(hitObject, collisionPoint);     // What we hit, and where we hit it
      }
      else
         mMoveState[ActualState].pos = endPos;

      mTarget = endPos;

   }

   // Kill old projectiles
   if(alive && path == GameObject::ServerIdleMainLoop)
   {
      if(mTimeRemaining <= deltaT)
      {
         deleteObject(500);
         mTimeRemaining = 0;
         alive = false;
         setMaskBits(ExplodedMask);
      }
      else
         mTimeRemaining -= deltaT;     // Decrement time left to live
   }


   if(path == GameObject::ServerIdleMainLoop)
   {
      setMaskBits(PositionMask);
      mMoveState[RenderState] = mMoveState[ActualState];
   }
   else
      updateInterpolation();

   updateExtent();
}



//
U32  HeatSeeker::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 ret = Parent::packUpdate(connection, updateMask, stream);
   stream->writeFlag(exploded);
   stream->writeFlag(updateMask & InitialMask);
      //writeCompressedVelocity(velocity, CompressedVelocityMax, stream);

   return ret;
}

void HeatSeeker::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
   {
      explode(getActualPos(), WeaponBurst);
   }

   if(stream->readFlag())
   {
      SFXObject::play(SFXGrenadeProjectile, getActualPos(), getActualVel());
   }

        // readCompressedVelocity(velocity, CompressedVelocityMax, stream);

}


void HeatSeeker::handleCollision(GameObject *hitObject, Point collisionPoint)
{
   // Identical to Projectile...  create sub to handle both?
   collided = true;

   if(!isGhost())
   {
      DamageInfo theInfo;
      theInfo.collisionPoint = collisionPoint;
      theInfo.damageAmount = gWeapons[WeaponHeatSeeker].damageAmount;
      theInfo.damageType = DamageTypePoint;
      theInfo.damagingObject = this;
      theInfo.impulseVector = velocity;

      hitObject->damageObject(&theInfo);
   }

   mTimeRemaining = 0;
   explode(hitObject, collisionPoint);
}



// Still need to customize this a bit!
void HeatSeeker::explode(GameObject *hitObject, Point thePos)
{
   // Do some particle spew...
   if(isGhost())
   {
      FXManager::emitExplosion(thePos, 0.3, gProjInfo[ProjectileBounce].sparkColors, NumSparkColors);

      Ship *s = dynamic_cast<Ship *>(hitObject);
      if(s && s->isModuleActive(ModuleShield))
         SFXObject::play(SFXBounceShield, thePos, velocity);
      else
         SFXObject::play(gProjInfo[ProjectileBounce].impactSound, thePos, velocity);
   }
}

void HeatSeeker::renderItem(Point p)
{

  if(collided || !alive)
      return;

   renderProjectile(p, ProjectileBounce, getGame()->getCurrentTime() - getCreationTime());

   // Render target
   if (mHasTarget)
   {
      glColor3f(0.5,0.5,0.5);
      drawCircle(mTarget, 100);
   }
}


void HeatSeeker::explode(Point pos, WeaponType weaponType)
{
   if(exploded) return;

   if(isGhost())
   {
      // Make us go boom!
      Color b(1,1,1);

      FXManager::emitExplosion(getRenderPos(), 0.5, gProjInfo[ProjectilePhaser].sparkColors, NumSparkColors);
      SFXObject::play(SFXMineExplode, getActualPos(), Point());
   }

   disableCollision();

   if(!isGhost())
   {
      setMaskBits(PositionMask);
      deleteObject(100);

      DamageInfo info;
      info.collisionPoint = pos;
      info.damagingObject = this;
      info.damageAmount   = gWeapons[weaponType].damageAmount;
      info.damageType     = DamageTypeArea;

      //radiusDamage(pos, InnerBlastRadius, OuterBlastRadius, DamagableTypes, info);
   }
   exploded = true;
}


//-----------------------------------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(SpyBug);

// Constructor
SpyBug::SpyBug(Point pos, Ship *planter) : GrenadeProjectile(pos, Point())
{
   mObjectTypeMask |= SpyBugType;            // Flag this as a SpyBug
   //mObjectTypeMask &= ~CommandMapVisType;    // These items aren't shown on commander's map (well, sometimes they are, but through a special method)

   mWeaponType = WeaponSpyBug;
   mNetFlags.set(Ghostable);

   if(planter)
   {
      setOwner(planter->getOwner());
      mTeam = planter->getTeam();
   }
   else
      mTeam = -1;
}


bool SpyBug::processArguments(S32 argc, const char **argv)
{
   if(argc < 3)
      return false;

   mTeam = atoi(argv[0]);                        // Team first!
   
   if(!Parent::processArguments(2, &argv[1]))    // Strips off first arg from argv, so the parent gets the straight coordinate pair it's expecting
      return false;

   return true;
}

// Spy bugs are always in scope.  This only really matters on pre-positioned spy bugs...
void SpyBug::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();

   getGame()->mObjectsLoaded++;
}

void SpyBug::idle(IdleCallPath path)
{
   // Skip the grenade timing goofiness...
   Item::idle(path);

   if(exploded || path != GameObject::ServerIdleMainLoop)
      return;
}

bool SpyBug::collide(GameObject *otherObj)
{
   if(otherObj->getObjectTypeMask() & (BulletType))
      explode(getActualPos(), WeaponSpyBug);
   return false;
}

void SpyBug::damageObject(DamageInfo *info)
{
   if(info->damageAmount > 0.f && !exploded)    // Any damage will kill the SpyBug
      explode(getActualPos(), WeaponSpyBug);
}

U32 SpyBug::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 ret = Parent::packUpdate(connection, updateMask, stream);
   if(stream->writeFlag(updateMask & InitialMask))
   {
      stream->write(mTeam);
      //RDW I want to kill the compiler that allows binding NULL to a reference.
      //stream->writeStringTableEntry(getOwner() ? getOwner()->getClientName() : NULL);
      // Just don't kill the coder who keeps doing it! -CE
      // And remember, pack and unpack must match, so if'ing this out won't work unless we do the same on unpack.
      StringTableEntryRef noOwner = StringTableEntryRef();
      stream->writeStringTableEntry(getOwner() ? getOwner()->getClientName() : noOwner);
   }
   return ret;
}

void SpyBug::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
   {
      initial = true;
      stream->read(&mTeam);
      stream->readStringTableEntry(&mSetBy);
   }
   if(initial)
      SFXObject::play(SFXSpyBugDeploy, getActualPos(), Point());
}

void SpyBug::renderItem(Point pos)
{
   if(exploded)
      return;

   Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
   if(!ship)
      return;


   // Can see bug if laid by teammate in team game || sensor is active || 
   //       you laid it yourself || spyBug is neutral

   bool visible = ( (ship->getTeam() == getTeam()) && getGame()->getGameType()->isTeamGame() || ship->isModuleActive(ModuleSensor) || 
            (getGame()->getGameType()->mLocalClient && getGame()->getGameType()->mLocalClient->name == mSetBy) || getTeam() == -1);

   renderSpyBug(pos, visible);
}


};
