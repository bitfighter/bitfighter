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

#include "gameWeapons.h"
#include "projectile.h"
#include "ship.h"
#include "sparkManager.h"
#include "SoundSystem.h"
#include "gameObject.h"
#include "gameObjectRender.h"

#include "SDL/SDL_opengl.h"

namespace Zap
{

const char LuaProjectile::className[] = "ProjectileItem";      // Class name as it appears to Lua scripts

// Define the methods we will expose to Lua
Lunar<LuaProjectile>::RegType LuaProjectile::methods[] =
{
   // Standard gameItem methods
   method(LuaProjectile, getClassID),
   method(LuaProjectile, getLoc),
   method(LuaProjectile, getRad),
   method(LuaProjectile, getVel),
   method(LuaProjectile, getTeamIndx),

   method(LuaProjectile, getWeapon),

   {0,0}    // End method list
};

//===========================================

static Vector<DatabaseObject*> fillVector;

TNL_IMPLEMENT_NETOBJECT(Projectile);

// Constructor
Projectile::Projectile(WeaponType type, Point p, Point v, GameObject *shooter)
{
   mObjectTypeMask = BulletType;
   mObjectTypeNumber = BulletTypeNumber;

   mNetFlags.set(Ghostable);
   pos = p;
   velocity = v;

   mTimeRemaining = gWeapons[type].projLiveTime;
   collided = false;
   hitShip = false;
   alive = true;
   hasBounced = false;
   mShooter = shooter;

   // Copy some attributes from the shooter
   if(shooter)
   {
      setOwner(shooter->getOwner());
      mTeam = shooter->getTeam();
      mKillString = shooter->getKillString();
   }
   else
      setOwner(NULL);

   mType = gWeapons[type].projectileType;
   mWeaponType = type;
}


U32 Projectile::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & PositionMask))
   {
      ((GameConnection *) connection)->writeCompressedPoint(pos, stream);
      writeCompressedVelocity(velocity, CompressedVelocityMax, stream);
   }

   if(stream->writeFlag(updateMask & InitialMask))
   {
      stream->writeEnum(mType, ProjectileTypeCount);

      S32 index = -1;
      if(mShooter.isValid())
         index = connection->getGhostIndex(mShooter);
      if(stream->writeFlag(index != -1))
         stream->writeInt(index, GhostConnection::GhostIdBitSize);
   }

   stream->writeFlag(collided);
   if(collided)
      stream->writeFlag(hitShip);
   stream->writeFlag(alive);

   return 0;
}


void Projectile::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;
   if(stream->readFlag())  // Read position, for correcting bouncers, needs to be before inital for SoundSystem::playSoundEffect
   {
      ((GameConnection *) connection)->readCompressedPoint(pos, stream);
      readCompressedVelocity(velocity, CompressedVelocityMax, stream);
   }

   if(stream->readFlag())         // Initial chunk of data, sent once for this object
   {

      mType = (ProjectileType) stream->readEnum(ProjectileTypeCount);

      TNLAssert(gClientGame->getConnectionToServer(), "Defunct connection to server in projectile.cpp!");
      Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
      if(ship && ship->hasModule(ModuleSensor))
         mObjectTypeMask |= CommandMapVisType;     // Bullets visible on commander's map if you have sensor

      if(stream->readFlag())
         mShooter = dynamic_cast<Ship *>(connection->resolveGhost(stream->readInt(GhostConnection::GhostIdBitSize)));

      Rect newExtent(pos,pos);
      setExtent(newExtent);
      initial = true;
      SoundSystem::playSoundEffect(gProjInfo[mType].projectileSound, pos, velocity);
   }
   bool preCollided = collided;
   collided = stream->readFlag();
   
   if(collided)
      hitShip = stream->readFlag();

   alive = stream->readFlag();

   if(!preCollided && collided)     // Projectile has "become" collided
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
   Ship *s = dynamic_cast<Ship *>(hitObject);
   hitShip = (s != NULL);

   if(!isGhost())    // If we're on the server, that is
   {
      DamageInfo theInfo;
      theInfo.collisionPoint = collisionPoint;
      theInfo.damageAmount = gWeapons[mWeaponType].damageAmount;
      theInfo.damageType = DamageTypePoint;
      theInfo.damagingObject = this;
      theInfo.impulseVector = velocity;
      theInfo.damageSelfMultiplier = gWeapons[mWeaponType].damageSelfMultiplier;

      hitObject->damageObject(&theInfo);

      if(hitShip && mShooter.isValid() && mShooter->getControllingClient().isValid())
         mShooter->getControllingClient()->mStatistics.countHit(mWeaponType);
   }

   mTimeRemaining = 0;
   explode(hitObject, collisionPoint);
}


void Projectile::idle(GameObject::IdleCallPath path)
{
   U32 deltaT = mCurrentMove.time;

   if(!collided && alive)
   {
    U32 aliveTime = getGame()->getCurrentTime() - getCreationTime();  // Age of object, in ms
    F32 timeLeft = deltaT;
    S32 loopcount1 = 32;
    while(timeLeft > 0.001 && loopcount1 != 0)    // This loop is to prevent slow bounce on low frame rate / high time left.
    {
      loopcount1--;
      // Calculate where projectile will be at the end of the current interval
      Point endPos = pos + velocity * timeLeft * 0.001;

      // Check for collision along projected route of movement
      static Vector<GameObject *> disabledList;

      Rect queryRect(pos, endPos);     // Bounding box of our travels

      disabledList.clear();


      // Don't collide with shooter during first 500ms of life
      if(mShooter.isValid() && aliveTime < 500 && !hasBounced)
      {
         disabledList.push_back(mShooter);
         mShooter->disableCollision();
      }

      GameObject *hitObject;

      F32 collisionTime;
      Point surfNormal;

      // Do the search
      for(;;)
      {
         hitObject = findObjectLOS(MoveableType | BarrierType | EngineeredType | ForceFieldType,
                                   MoveObject::RenderState, pos, endPos, collisionTime, surfNormal);

         if((!hitObject || hitObject->collide(this)))
            break;

         // Disable collisions with things that don't want to be
         // collided with (i.e. whose collide methods return false)
         disabledList.push_back(hitObject);
         hitObject->disableCollision();
      }

      // Re-enable collison flag for ship and items in our path that don't want to be collided with
      // Note that if we hit an object that does want to be collided with, it won't be in disabledList
      // and thus collisions will not have been disabled, and thus don't need to be re-enabled.
      // Our collision detection is done, and hitObject contains the first thing that the projectile hit.

      for(S32 i = 0; i < disabledList.size(); i++)
         disabledList[i]->enableCollision();

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
            hasBounced = true;
            // We hit something that we should bounce from, so bounce!
            F32 float1 = surfNormal.dot(velocity) * 2;
            velocity -= surfNormal * float1;
            if(float1 > 0)
               surfNormal = -surfNormal;      // This is to fix going through polygon barriers

            Point collisionPoint = pos + (endPos - pos) * collisionTime;
            pos = collisionPoint + surfNormal;
            timeLeft = timeLeft * (0.99 - collisionTime);

            MoveObject *obj = dynamic_cast<MoveObject *>(hitObject);
            if(obj)
            {
               setMaskBits(PositionMask);  // Bouncing off a moving objects can easily get desync.
               float1 = pos.distanceTo(obj->getRenderPos());
               if(float1 < obj->getRadius())
               {
                  float1 = obj->getRadius() * 1.01 / float1;
                  pos = pos * float1 + obj->getRenderPos() * (1.0 - float1);  // to fix bouncy stuck inside shielded ship
               }
            }

            if(isGhost())
               SoundSystem::playSoundEffect(SFXBounceShield, collisionPoint, surfNormal * surfNormal.dot(velocity) * 2);
         }
         else
         {
            // Not bouncing, so advance to location of collision
            Point collisionPoint = pos + (endPos - pos) * collisionTime;
            handleCollision(hitObject, collisionPoint);     // What we hit, where we hit it
            timeLeft = 0;
         }

      }
      else        // Hit nothing, advance projectile to endPos
      {
         timeLeft = 0;
         //// Steer towards a nearby testitem
         //static Vector<DatabaseObject *> targetItems;
         //targetItems.clear();
         //Rect searchArea(pos, 2000);
         //S32 HeatSeekerTargetType = TestItemType | ResourceItemType;
         //findObjects(HeatSeekerTargetType, targetItems, searchArea);

         //F32 maxPull = 0;
         //Point dist;
         //Point maxDist;

         //for(S32 i = 0; i < targetItems.size(); i++)
         //{
         //   GameObject *target = dynamic_cast<GameObject *>(targetItems[i]);
         //   dist.set(pos - target->getActualPos());

         //   F32 pull = min(100.0f / dist.len(), 1.0);      // Pull == strength of attraction
         //   
         //   pull *= (target->getObjectTypeMask() & ResourceItemType) ? .5 : 1;

         //   if(pull > maxPull)
         //   {
         //      maxPull = pull;
         //      maxDist.set(dist);
         //   }
         //   
         //}

         //if(maxPull > 0)
         //{
         //   F32 speed = velocity.len();
         //   velocity += (velocity - maxDist) * maxPull;
         //   velocity.normalize(speed);
         //   endPos.set(pos + velocity * (F32)deltaT * 0.001);  // Apply the adjusted velocity right now!
         //}

         pos.set(endPos);     // Keep this line
      }

      Rect newExtent(pos,pos);
      setExtent(newExtent);
    }
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


void Projectile::explode(GameObject *hitObject, Point pos)
{
   // Do some particle spew...
   if(isGhost())
   {
      FXManager::emitExplosion(pos, 0.3, gProjInfo[mType].sparkColors, NumSparkColors);

      Ship *s = dynamic_cast<Ship *>(hitObject);

      SFXProfiles sound; 
      if(s && s->isModuleActive(ModuleShield))     // We hit a ship with shields up
         sound = SFXBounceShield;
      else if((hitShip || s))                      // We hit a ship with shields down
         sound = SFXShipHit;
      else                                         // We hit something else
         sound = gProjInfo[mType].impactSound;

      SoundSystem::playSoundEffect(sound, pos, velocity);       // Play the sound
   }
}


void Projectile::renderItem(Point pos)
{
   if(collided || !alive)
      return;

   renderProjectile(pos, mType, getGame()->getCurrentTime() - getCreationTime());
}


//// Lua methods

S32 Projectile::getLoc(lua_State *L) { return returnPoint(L, getActualPos()); }     // Center of item (returns point)
S32 Projectile::getRad(lua_State *L) { return returnInt(L, 10); }                   // TODO: Wrong!!  Radius of item (returns number)
S32 Projectile::getVel(lua_State *L) { return returnPoint(L, getActualVel()); }     // Speed of item (returns point)
S32 Projectile::getTeamIndx(lua_State *L) { return returnInt(L, mShooter->getTeam()); }   // Team of shooter

GameObject *Projectile::getGameObject() { return this; }                            // Return the underlying GameObject


//-----------------------------------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(GrenadeProjectile);

GrenadeProjectile::GrenadeProjectile(Point pos, Point vel, GameObject *shooter): Item(pos, true, mRadius, mMass)
{
   mObjectTypeMask = MoveableType | BulletType;
   mObjectTypeNumber = GrenadeProjectileTypeNumber;

   mNetFlags.set(Ghostable);

   mMoveState[ActualState].pos = pos;
   mMoveState[ActualState].vel = vel;
   setMaskBits(PositionMask);
   mWeaponType = WeaponBurst;

   updateExtent();

   mTimeRemaining = gWeapons[WeaponBurst].projLiveTime;
   exploded = false;

   if(shooter)
   {
      setOwner(shooter->getOwner());
      mTeam = shooter->getTeam();
   }
   else
      setOwner(NULL);

   mRadius = 7;
   mMass = 1;
}


// Runs on client and server
void GrenadeProjectile::idle(IdleCallPath path)
{

   U32 aliveTime = getGame()->getCurrentTime() - getCreationTime();  // Age of object, in ms

   // Fix effect of ship getting ahead of burst on laggy client      (gClientGame is never NULL)
   bool collisionDisabled = isGhost() && aliveTime < 250 &&
                            gClientGame->getConnectionToServer() && gClientGame->getConnectionToServer()->getControlObject();

   if(collisionDisabled) 
      gClientGame->getConnectionToServer()->getControlObject()->disableCollision();

   Parent::idle(path);

   if(collisionDisabled) 
      gClientGame->getConnectionToServer()->getControlObject()->enableCollision();

   // Do some drag...  no, not that kind of drag!
   mMoveState[ActualState].vel -= mMoveState[ActualState].vel * (((F32)mCurrentMove.time) / 1000.f);

   if(isGhost()) return;      // Here on down is server only

   if(!exploded)
      if(getActualVel().len() < 4.0)
         explode(getActualPos(), WeaponBurst);

   // Update TTL
   S32 deltaT = mCurrentMove.time;
   if(path == GameObject::ClientIdleMainRemote)
      mTimeRemaining += deltaT;
   else if(!exploded)
   {
      if(mTimeRemaining <= deltaT)
        explode(getActualPos(), WeaponBurst);
      else
         mTimeRemaining -= deltaT;
   }
}


U32 GrenadeProjectile::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 ret = Parent::packUpdate(connection, updateMask, stream);

   stream->writeFlag(exploded);
   stream->writeFlag(updateMask & InitialMask);
   return ret;
}


void GrenadeProjectile::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   TNLAssert(gClientGame->getConnectionToServer(), "Invalid connection to server in GrenadeProjectile//projectile.cpp");
   Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
   if(ship && ship->hasModule(ModuleSensor))
      mObjectTypeMask |= CommandMapVisType;     // Bursts visible on commander's map if you have sensor

   if(stream->readFlag())
   {
      explode(getActualPos(), WeaponBurst);
   }

   if(stream->readFlag())
   {
      SoundSystem::playSoundEffect(SFXGrenadeProjectile, getActualPos(), getActualVel());
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

   // Bounce off stuff
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

      SoundSystem::playSoundEffect(SFXMineExplode, getActualPos(), Point());
   }

   disableCollision();

   if(!isGhost())
   {
      setMaskBits(PositionMask);
      deleteObject(100);

      DamageInfo info;
      info.collisionPoint       = pos;
      info.damagingObject       = this;
      info.damageAmount         = gWeapons[weaponType].damageAmount;
      info.damageType           = DamageTypeArea;
      info.damageSelfMultiplier = gWeapons[weaponType].damageSelfMultiplier;

      S32 hits = radiusDamage(pos, InnerBlastRadius, OuterBlastRadius, DamagableTypes, info);

      if(getOwner())
         for(S32 i = 0; i < hits; i++)
            getOwner()->mStatistics.countHit(mWeaponType);
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

   WeaponInfo *wi = gWeapons + WeaponBurst;
   F32 initTTL = (F32) wi->projLiveTime;
   renderGrenade( pos, (initTTL - (F32) (getGame()->getCurrentTime() - getCreationTime())) / initTTL );
}


////////////////////////////////////////
////////////////////////////////////////

static void drawLetter(char letter, const Point &pos, const Color &color, F32 alpha)
{
   // Mark the item with a letter, unless we're showing the reference ship
   F32 vertOffset = 8;
   if (letter >= 'a' && letter <= 'z')    // Better position lowercase letters
      vertOffset = 10;

   glColor(color, alpha);
   F32 xpos = pos.x - UserInterface::getStringWidthf(15, "%c", letter) / 2;

   UserInterface::drawStringf(xpos, pos.y - vertOffset, 15, "%c", letter);
}


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Mine);

// Constructor (planter defaults to null)
Mine::Mine(Point pos, Ship *planter) : GrenadeProjectile(pos, Point())
{
   mObjectTypeMask = MoveableType | MineType;
   mObjectTypeNumber = MineTypeNumber;
   mWeaponType = WeaponMine;

   if(planter)
   {
      setOwner(planter->getOwner());
      mTeam = planter->getTeam();
   }
   else
   {
      mTeam = -2;    // Hostile to all, as mines generally are!
      setOwner(NULL);
   }

   mArmed = false;
   mKillString = "mine";      // Triggers special message when player killed
}


Mine *Mine::clone() const
{
   return new Mine(*this);
}


// ProcessArguments() used is the one in item
string Mine::toString()
{
   return string(Object::getClassName()) + " " + (getActualPos() / getGame()->getGridSize()).toString();
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
      GameObject *foundObject = dynamic_cast<GameObject *>(fillVector[i]);

      F32 radius;
      Point ipos;
      if(foundObject->getCollisionCircle(MoveObject::RenderState, ipos, radius))
      {
         if((ipos - pos).len() < (radius + SensorRadius))
         {
            bool isMine = foundObject->getObjectTypeMask() & MineType;
            if(!isMine)
            {
               foundItem = true;
               break;
            }
            else if(mArmed && foundObject != this)
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
   if(otherObj->getObjectTypeMask() & (BulletType | SpyBugType | MineType))
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
      SoundSystem::playSoundEffect(SFXMineDeploy, getActualPos(), Point());
   else if(!initial && !wasArmed && mArmed)
      SoundSystem::playSoundEffect(SFXMineArm, getActualPos(), Point());
}


void Mine::renderItem(Point pos)
{
   if(exploded)
      return;

   TNLAssert(gClientGame->getConnectionToServer(), "Invalid connection to server in Mine//projectile.cpp");
   Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());

   if(!ship)
      return;

   // Can see mine if laid by teammate in team game || sensor is active ||
   //  you laid it yourself
   bool visible = ( (ship->getTeam() == getTeam()) && getGame()->getGameType()->isTeamGame() ) || ship->isModuleActive(ModuleSensor) ||
      (getGame()->getGameType()->mLocalClient && getGame()->getGameType()->mLocalClient->name == mSetBy);

   renderMine(pos, mArmed, visible);
}


void Mine::renderEditor(F32 currentScale)
{
   renderMine(getVert(0), true, true);
}


void Mine::renderDock()
{
   glColor3f(.7, .7, .7);
   drawCircle(getVert(0), 9);
   drawLetter('M', getVert(0), Color(.7), 1);
}


// Lua methods
const char Mine::className[] = "MineItem";      // Class name as it appears to Lua scripts

// Define the methods we will expose to Lua
Lunar<Mine>::RegType Mine::methods[] =
{
   // Standard gameItem methods
   method(Mine, getClassID),
   method(Mine, getLoc),
   method(Mine, getRad),
   method(Mine, getVel),
   method(Mine, getTeamIndx),

   method(Mine, getWeapon),

   {0,0}    // End method list
};

//////////////////////////////////
//////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(SpyBug);

// Constructor
SpyBug::SpyBug(Point pos, Ship *planter) : GrenadeProjectile(pos, Point())
{
   mObjectTypeMask = MoveableType | SpyBugType;
   mObjectTypeNumber = SpyBugTypeNumber;
   //mObjectTypeMask &= ~CommandMapVisType;    // These items aren't shown on commander's map (well, sometimes they are, but through a special method)

   mWeaponType = WeaponSpyBug;
   mNetFlags.set(Ghostable);

   if(planter)
   {
      setOwner(planter->getOwner());
      mTeam = planter->getTeam();
   }
   else
   {
      mTeam = -1;
      setOwner(NULL);
   }
}


// Destructor
SpyBug::~SpyBug()
{
   if(gServerGame && gServerGame->getGameType())
      gServerGame->getGameType()->catalogSpybugs();
}


SpyBug *SpyBug::clone() const
{
   return new SpyBug(*this);
}


bool SpyBug::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 3)
      return false;

   mTeam = atoi(argv[0]);                        // Team first!

   // Strips off first arg from argv, so the parent gets the straight coordinate pair it's expecting
   if(!Parent::processArguments(2, &argv[1], game))    
      return false;

   return true;
}


// ProcessArguments() used is the one in item
string SpyBug::toString()
{
   return string(Object::getClassName()) + " " + (getActualPos() / getGame()->getGridSize()).toString() + " " + itos(mTeam);
}


// Spy bugs are always in scope.  This only really matters on pre-positioned spy bugs...
void SpyBug::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();

   GameType *gt = getGame()->getGameType();
   if(gt && !isGhost())  // non-Ghost / ServerGame only, currently useless for client
      gt->addSpyBug(this);
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
   if(otherObj->getObjectTypeMask() & (BulletType | SpyBugType | MineType))
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
      StringTableEntryRef noOwner = StringTableEntryRef("");
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
      SoundSystem::playSoundEffect(SFXSpyBugDeploy, getActualPos(), Point());
}


void SpyBug::renderItem(Point pos)
{
   if(exploded)
      return;

   TNLAssert(gClientGame->getConnectionToServer(), "Invalid connection to server in SpyBug//projectile.cpp");
   Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
   if(!ship)
      return;

   // Can see bug if laid by teammate in team game || sensor is active ||
   //       you laid it yourself || spyBug is neutral
   bool visible = ( ((ship->getTeam() == getTeam()) && getGame()->getGameType()->isTeamGame()) || ship->isModuleActive(ModuleSensor) ||
            (getGame()->getGameType()->mLocalClient && getGame()->getGameType()->mLocalClient->name == mSetBy) || getTeam() == -1);

   renderSpyBug(pos, visible);
}


void SpyBug::renderEditor(F32 currentScale)
{
   renderSpyBug(getVert(0), true);
}


void SpyBug::renderDock()
{
   glColor3f(.7, .7, .7);
   drawCircle(getVert(0), 9);
   drawLetter('S', getVert(0), Color(.7), 1);
}



// Can the player see the spybug?
bool SpyBug::isVisibleToPlayer(S32 playerTeam, StringTableEntry playerName, bool isTeamGame)
{
   // On our team (in a team game) || was set by us (in any game) || is neutral (in any game)
   return ((getTeam() == playerTeam) && isTeamGame) || playerName == mSetBy || mTeam == -1;
}



// Lua methods
const char SpyBug::className[] = "SpyBugItem";      // Class name as it appears to Lua scripts

// Define the methods we will expose to Lua
Lunar<SpyBug>::RegType SpyBug::methods[] =
{
   // Standard gameItem methods
   method(SpyBug, getClassID),
   method(SpyBug, getLoc),
   method(SpyBug, getRad),
   method(SpyBug, getVel),
   method(SpyBug, getTeamIndx),

   method(SpyBug, getWeapon),

   {0,0}    // End method list
};



};

