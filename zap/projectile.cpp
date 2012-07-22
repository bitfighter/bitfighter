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

#include "projectile.h"
#include "gameWeapons.h"
#include "SoundSystem.h"
#include "BfObject.h"
#include "gameObjectRender.h"
#include "gameConnection.h"
#include "stringUtils.h"
#include "ClientInfo.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "sparkManager.h"
#  include "UI.h"
#  include "OpenglUtils.h"
#endif


namespace Zap
{


TNL_IMPLEMENT_NETOBJECT(Projectile);

// Constructor
Projectile::Projectile(WeaponType type, Point pos, Point vel, BfObject *shooter)
{
   mObjectTypeNumber = BulletTypeNumber;
   setNewGeometry(geomPoint);

   mNetFlags.set(Ghostable);
   setPos(pos);
   mVelocity = vel;

   mTimeRemaining = GameWeapon::weaponInfo[type].projLiveTime;
   collided = false;
   hitShip = false;
   alive = true;
   hasBounced = false;
   mShooter = shooter;

   setOwner(NULL);

   // Copy some attributes from the shooter
   if(shooter)
   {
      if(isShipType(shooter->getObjectTypeNumber()))
      {
         Ship *ship = static_cast<Ship *>(shooter);
         setOwner(ship->getClientInfo());
      }

      setTeam(shooter->getTeam());
      mKillString = shooter->getKillString();
   }

   mType = GameWeapon::weaponInfo[type].projectileType;
   mWeaponType = type;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
Projectile::~Projectile()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


U32 Projectile::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & PositionMask))
   {
      ((GameConnection *) connection)->writeCompressedPoint(getPos(), stream);
      writeCompressedVelocity(mVelocity, COMPRESSED_VELOCITY_MAX, stream);
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
      static Point pos;    // Reusable container
      ((GameConnection *) connection)->readCompressedPoint(pos, stream);
      setPos(pos);

      readCompressedVelocity(mVelocity, COMPRESSED_VELOCITY_MAX, stream);
   }

   if(stream->readFlag())         // Initial chunk of data, sent once for this object
   {

      mType = (ProjectileType) stream->readEnum(ProjectileTypeCount);

      TNLAssert(connection, "Defunct connection to server in projectile.cpp!");

      if(stream->readFlag())
         mShooter = dynamic_cast<Ship *>(connection->resolveGhost(stream->readInt(GhostConnection::GhostIdBitSize)));

      setExtent(Rect(getPos(), 0));
      initial = true;
      SoundSystem::playSoundEffect(GameWeapon::projectileInfo[mType].projectileSound, getPos(), mVelocity);
   }
   bool preCollided = collided;
   collided = stream->readFlag();
   
   if(collided)
      hitShip = stream->readFlag();

   alive = stream->readFlag();

   if(!preCollided && collided)     // Projectile has "become" collided
      explode(NULL, getPos());

   if(!collided && initial)
      mCurrentMove.time = U32(connection->getOneWayTime());
}


void Projectile::handleCollision(BfObject *hitObject, Point collisionPoint)
{
   collided = true;
   Ship *s = dynamic_cast<Ship *>(hitObject);
   hitShip = (s != NULL);

   if(!isGhost())    // If we're on the server, that is
   {
      DamageInfo theInfo;

      theInfo.collisionPoint = collisionPoint;
      theInfo.damageAmount = GameWeapon::weaponInfo[mWeaponType].damageAmount;
      theInfo.damageType = DamageTypePoint;
      theInfo.damagingObject = this;
      theInfo.impulseVector = mVelocity;
      theInfo.damageSelfMultiplier = GameWeapon::weaponInfo[mWeaponType].damageSelfMultiplier;

      hitObject->damageObject(&theInfo);

      Ship *shooter = dynamic_cast<Ship *>(mShooter.getPointer());

      if(hitShip && shooter && shooter->getClientInfo())
         shooter->getClientInfo()->getStatistics()->countHit(mWeaponType);
   }

   mTimeRemaining = 0;
   explode(hitObject, collisionPoint);
}


void Projectile::idle(BfObject::IdleCallPath path)
{
   U32 deltaT = mCurrentMove.time;

   if(!collided && alive)
   {
      U32 aliveTime = getGame()->getCurrentTime() - getCreationTime();  // Age of object, in ms
      F32 timeLeft = (F32)deltaT;
      S32 loopcount = 32;

      Point startPos, collisionPoint;

      while(timeLeft > 0.01f && loopcount != 0)    // This loop is to prevent slow bounce on low frame rate / high time left
      {
         loopcount--;
         
         startPos = getPos();

         // Calculate where projectile will be at the end of the current interval
         Point endPos = startPos + (mVelocity * .001f) * timeLeft;    // mVelocity in units/sec, timeLeft in ms

         // Check for collision along projected route of movement
         static Vector<BfObject *> disabledList;

         Rect queryRect(startPos, endPos);     // Bounding box of our travels

         disabledList.clear();


         // Don't collide with shooter during first 500ms of life
         if(mShooter.isValid() && aliveTime < 500 && !hasBounced)
         {
            disabledList.push_back(mShooter);
            mShooter->disableCollision();
         }

         BfObject *hitObject;

         F32 collisionTime;
         Point surfNormal;

         // Do the search
         while(true)  
         {
            hitObject = findObjectLOS((TestFunc)isWeaponCollideableType, RenderState, startPos, endPos, collisionTime, surfNormal);

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

            if(mType == ProjectileBounce && isWallType(hitObject->getObjectTypeNumber()))
               bounce = true;
            else if(isShipType(hitObject->getObjectTypeNumber()))
            {
               Ship *ship = static_cast<Ship *>(hitObject);
               if(ship->isModulePrimaryActive(ModuleShield))
                  bounce = true;
            }

            if(bounce)
            {
               hasBounced = true;
               // We hit something that we should bounce from, so bounce!
               F32 float1 = surfNormal.dot(mVelocity) * 2;
               mVelocity -= surfNormal * float1;

               if(float1 > 0)
                  surfNormal = -surfNormal;      // This is to fix going through polygon barriers

               startPos = getPos();
               collisionPoint = startPos + (endPos - startPos) * collisionTime;

               setPos(collisionPoint + surfNormal);
               timeLeft = timeLeft * (1 - collisionTime);

               MoveObject *obj = dynamic_cast<MoveObject *>(hitObject);
               if(obj)
               {
                  startPos = getPos();

                  setMaskBits(PositionMask);  // Bouncing off a moving objects can easily get desync.
                  float1 = startPos.distanceTo(obj->getRenderPos());
                  if(float1 < obj->getRadius())
                  {
                     float1 = obj->getRadius() * 1.01f / float1;
                     setVert(startPos * float1 + obj->getRenderPos() * (1 - float1), 0);  // to fix bouncy stuck inside shielded ship
                  }
               }

               if(isGhost())
                  SoundSystem::playSoundEffect(SFXBounceShield, collisionPoint, surfNormal * surfNormal.dot(mVelocity) * 2);
            }
            else
            {
               // Not bouncing, so advance to location of collision
               startPos = getPos();
               collisionPoint = startPos + (endPos - startPos) * collisionTime;
               handleCollision(hitObject, collisionPoint);     // What we hit, where we hit it
               timeLeft = 0;
            }

         }
         else        // Hit nothing, advance projectile to endPos
         {
            timeLeft = 0;

            setPos(endPos);
         }
      }
   }
         

   // Kill old projectiles
   if(alive && path == BfObject::ServerIdleMainLoop)
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


void Projectile::explode(BfObject *hitObject, Point pos)
{
#ifndef ZAP_DEDICATED
   // Do some particle spew...
   if(isGhost())
   {
      TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");
      static_cast<ClientGame *>(getGame())->emitExplosion(pos, 0.3f, GameWeapon::projectileInfo[mType].sparkColors, NumSparkColors);

      SFXProfiles sound;

      bool isShip = hitObject && isShipType(hitObject->getObjectTypeNumber());

      Ship *ship = NULL;
      if(isShip)
         ship = static_cast<Ship *>(hitObject);

      if(ship && ship->isModulePrimaryActive(ModuleShield))
         sound = SFXBounceShield;
      else if((hitShip || ship))                           // We hit a ship with shields down
         sound = SFXShipHit;
      else                                                   // We hit something else
         sound = GameWeapon::projectileInfo[mType].impactSound;

      SoundSystem::playSoundEffect(sound, pos, mVelocity);   // Play the sound
   }
#endif
}


Point Projectile::getRenderVel() const
{
   return mVelocity;
}


Point Projectile::getActualVel() const
{
   return mVelocity;
}


// TODO: Get rid of this! (currently won't render without it)
void Projectile::render()
{
   renderItem(getPos());
}


void Projectile::renderItem(const Point &pos)
{
   if(collided || !alive)
      return;

   renderProjectile(pos, mType, getGame()->getCurrentTime() - getCreationTime());
}


//// Lua methods

//               Fn name    Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getRad,    ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getWeapon, ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getVel,    ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(Projectile, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(Projectile, LUA_METHODS);

#undef LUA_METHODS


const char *Projectile::luaClassName = "Projectile";
REGISTER_LUA_SUBCLASS(Projectile, BfObject);


S32 Projectile::getRad(lua_State *L)    { return returnInt(L, 10);               } // TODO: Wrong!!  Radius of item (returns number)
S32 Projectile::getVel(lua_State *L)    { return returnPoint(L, getActualVel()); }
S32 Projectile::getWeapon(lua_State *L) { return returnInt(L, mWeaponType);      }


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(BurstProjectile);

// Constructor
BurstProjectile::BurstProjectile(Point pos, Point vel, BfObject *shooter): MoveItem(pos, true, mRadius, mMass)
{
   mObjectTypeNumber = BurstTypeNumber;

   mNetFlags.set(Ghostable);

   setActualPos(pos);
   setActualVel(vel);
   setMaskBits(PositionMask);
   mWeaponType = WeaponBurst;

   updateExtentInDatabase();

   mTimeRemaining = GameWeapon::weaponInfo[WeaponBurst].projLiveTime;
   exploded = false;

   if(shooter)
   {
      setOwner(shooter->getOwner());
      setTeam(shooter->getTeam());
   }
   else
      setOwner(NULL);

   mRadius = 7;
   mMass = 1;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
BurstProjectile::~BurstProjectile()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


// Runs on client and server
void BurstProjectile::idle(IdleCallPath path)
{
   bool collisionDisabled = false;
   GameConnection *gc = NULL;

#ifndef ZAP_DEDICATED
   if(isGhost())   // Fix effect of ship getting ahead of burst on laggy client  
   {
      U32 aliveTime = getGame()->getCurrentTime() - getCreationTime();  // Age of object, in ms

      ClientGame *clientGame = static_cast<ClientGame *>(getGame());
      gc = clientGame->getConnectionToServer();

      collisionDisabled = aliveTime < 250 && gc && gc->getControlObject();

      if(collisionDisabled) 
         gc->getControlObject()->disableCollision();
   }
#endif

   Parent::idle(path);

   if(collisionDisabled) 
      gc->getControlObject()->enableCollision();

   // Do some drag...  no, not that kind of drag!
   setActualVel(getActualVel() - getActualVel() * (((F32)mCurrentMove.time) / 1000.f));

   if(isGhost())       // Here on down is server only
      return;

   if(!exploded)
      if(getActualVel().len() < 4.0)
         explode(getActualPos(), WeaponBurst);

   // Update TTL
   S32 deltaT = mCurrentMove.time;
   if(path == BfObject::ClientIdleMainRemote)
      mTimeRemaining += deltaT;
   else if(!exploded)
   {
      if(mTimeRemaining <= deltaT)
        explode(getActualPos(), WeaponBurst);
      else
         mTimeRemaining -= deltaT;
   }
}


U32 BurstProjectile::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 ret = Parent::packUpdate(connection, updateMask, stream);

   stream->writeFlag(exploded);
   stream->writeFlag((updateMask & InitialMask) && (getGame()->getCurrentTime() - getCreationTime() < 500));
   return ret;
}


void BurstProjectile::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   TNLAssert(connection, "Invalid connection to server in BurstProjectile//projectile.cpp");

   if(stream->readFlag())
      explode(getActualPos(), WeaponBurst);

   if(stream->readFlag())
      SoundSystem::playSoundEffect(SFXBurstProjectile, getActualPos(), getActualVel());
}


void BurstProjectile::damageObject(DamageInfo *theInfo)
{
   // If we're being damaged by another burst, explode...
   if(theInfo->damageType == DamageTypeArea)
   {
      explode(getActualPos(), WeaponBurst);
      return;
   }

   computeImpulseDirection(theInfo);

   setMaskBits(PositionMask);
}


// Also used for mines and spybugs  --> not sure if we really need to pass weaponType
void BurstProjectile::explode(Point pos, WeaponType weaponType)
{
   if(exploded) return;
   exploded = true;

#ifndef ZAP_DEDICATED
   if(isGhost())
   {
      TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");
      //static_cast<ClientGame *>(getGame())->emitExplosion(getRenderPos(), 0.5, GameWeapon::projectileInfo[ProjectilePhaser].sparkColors, NumSparkColors);      // Original, nancy explosion
      static_cast<ClientGame *>(getGame())->emitBlast(pos, OuterBlastRadius);          // New, manly explosion

      SoundSystem::playSoundEffect(SFXMineExplode, getActualPos());
   }
#endif

   disableCollision();

   if(!isGhost())
   {

      DamageInfo info;
      info.collisionPoint       = pos;
      info.damagingObject       = this;
      info.damageAmount         = GameWeapon::weaponInfo[weaponType].damageAmount;
      info.damageType           = DamageTypeArea;
      info.damageSelfMultiplier = GameWeapon::weaponInfo[weaponType].damageSelfMultiplier;

      S32 hits = radiusDamage(pos, InnerBlastRadius, OuterBlastRadius, (TestFunc)isDamageableType, info);

      if(getOwner())
         for(S32 i = 0; i < hits; i++)
            getOwner()->getStatistics()->countHit(mWeaponType);

      setMaskBits(ExplodedMask);
      deleteObject(100);
   }
}


bool BurstProjectile::collide(BfObject *otherObj)
{
   return true;
}


void BurstProjectile::renderItem(const Point &pos)
{
   if(exploded)
      return;

   WeaponInfo *wi = GameWeapon::weaponInfo + WeaponBurst;
   F32 initTTL = (F32) wi->projLiveTime;
   renderGrenade( pos, (initTTL - (F32) (getGame()->getCurrentTime() - getCreationTime())) / initTTL);
}


/////
// Lua interface

//               Fn name    Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getWeapon, ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(BurstProjectile, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(BurstProjectile, LUA_METHODS);

#undef LUA_METHODS


const char *BurstProjectile::luaClassName = "Burst";
REGISTER_LUA_SUBCLASS(BurstProjectile, MoveItem);


S32 BurstProjectile::getWeapon(lua_State *L) { return returnInt(L, mWeaponType); }


////////////////////////////////////////
////////////////////////////////////////

static void drawLetter(char letter, const Point &pos, const Color &color, F32 alpha)
{
#ifndef ZAP_DEDICATED
   // Mark the item with a letter, unless we're showing the reference ship
   F32 vertOffset = 8;
   if (letter >= 'a' && letter <= 'z')    // Better position lowercase letters
      vertOffset = 10;

   glColor(color, alpha);
   F32 xpos = pos.x - UserInterface::getStringWidthf(15, "%c", letter) / 2;

   UserInterface::drawStringf(xpos, pos.y - vertOffset, 15, "%c", letter);
#endif
}


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Mine);

// Constructor (planter defaults to null)
Mine::Mine(Point pos, Ship *planter) : BurstProjectile(pos, Point())
{
   mObjectTypeNumber = MineTypeNumber;
   mWeaponType = WeaponMine;

   if(planter)
   {
      setOwner(planter->getOwner());
      setTeam(planter->getTeam());
   }
   else
   {
      setTeam(TEAM_HOSTILE);    // Hostile to all, as mines generally are!
      setOwner(NULL);
   }

   mArmed = false;
   mKillString = "mine";      // Triggers special message when player killed

   setExtent(Rect(pos, pos));

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
Mine::~Mine()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


Mine *Mine::clone() const
{
   return new Mine(*this);
}


// ProcessArguments() used is the one in item
string Mine::toString(F32 gridSize) const
{
   return string(Object::getClassName()) + " " + geomToString(gridSize);
}


void Mine::idle(IdleCallPath path)
{
   // Skip the grenade timing goofiness...
   MoveItem::idle(path);

   if(exploded || path != BfObject::ServerIdleMainLoop)
      return;

   // And check for enemies in the area...
   Point pos = getActualPos();
   Rect queryRect(pos, pos);
   queryRect.expand(Point(SensorRadius, SensorRadius));

   fillVector.clear();
   findObjects((TestFunc)isMotionTriggerType, fillVector, queryRect);

   // Found something!
   bool foundItem = false;
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      BfObject *foundObject = dynamic_cast<BfObject *>(fillVector[i]);

      F32 radius;
      Point ipos;
      if(foundObject->getCollisionCircle(RenderState, ipos, radius))
      {
         if((ipos - pos).len() < (radius + SensorRadius))
         {
            bool isMine = foundObject->getObjectTypeNumber() == MineTypeNumber;
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


bool Mine::collide(BfObject *otherObj)
{
   if(isGhost())
      return false;  // avoid client side explode, server side don't explode
   if(isProjectileType(otherObj->getObjectTypeNumber()))
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

   if(updateMask & InitialMask)
   {
      writeThisTeam(stream);
      stream->writeStringTableEntry(getOwner() ? getOwner()->getName() : "");
   }

   stream->writeFlag(mArmed);

   return ret;
}


void Mine::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;
   Parent::unpackUpdate(connection, stream);

   if(mInitial)     // Initial data
   {
      initial = true;
      readThisTeam(stream);
      stream->readStringTableEntry(&mSetBy);
   }
   bool wasArmed = mArmed;
   mArmed = stream->readFlag();

   if(initial && !mArmed)
      SoundSystem::playSoundEffect(SFXMineDeploy, getActualPos());
   else if(!initial && !wasArmed && mArmed)
      SoundSystem::playSoundEffect(SFXMineArm, getActualPos());
}


void Mine::renderItem(const Point &pos)
{
#ifndef ZAP_DEDICATED
   if(exploded)
      return;

   bool visible, armed;

   Game *game = getGame();
   ClientGame *clientGame = static_cast<ClientGame *>(game);

   if(clientGame && clientGame->getConnectionToServer())
   {
      Ship *ship = dynamic_cast<Ship *>(clientGame->getConnectionToServer()->getControlObject());

      if(!ship)
         return;

      armed = mArmed;

      GameType *gameType = clientGame->getGameType();

      GameConnection *localClient = clientGame->getConnectionToServer();

      // Can see mine if laid by teammate in team game || sensor is active ||
      // you laid it yourself
      visible = ( (ship->getTeam() == getTeam()) && gameType->isTeamGame() ) || ship->isModulePrimaryActive(ModuleSensor) ||
                  (localClient && localClient->getClientInfo()->getName() == mSetBy);
   }
   else     // Must be in editor?
   {
      armed = true;
      visible = true;
   }

   renderMine(pos, armed, visible);
#endif
}


void Mine::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   renderMine(getActualPos(), true, true);
}


void Mine::renderDock()
{
#ifndef ZAP_DEDICATED
   Point pos = getActualPos();

   glColor(.7);
   drawCircle(pos, 9);
   drawLetter('M', pos, Color(.7), 1);
#endif
}


const char *Mine::getOnScreenName()     { return "Mine";  }
const char *Mine::getOnDockName()       { return "Mine";  }
const char *Mine::getPrettyNamePlural() { return "Mines"; }
const char *Mine::getEditorHelpString() { return "Mines can be prepositioned, and are are \"hostile to all\". [M]"; }


bool Mine::hasTeam()      { return false; }
bool Mine::canBeHostile() { return false; }
bool Mine::canBeNeutral() { return false; }


/////
// Lua interface

const luaL_reg           Mine::luaMethods[]   = { { NULL, NULL } };
const LuaFunctionProfile Mine::functionArgs[] = { { NULL, { }, 0 } };


const char *Mine::luaClassName = "MineItem";
REGISTER_LUA_SUBCLASS(Mine, BurstProjectile);


//////////////////////////////////
//////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(SpyBug);

// Constructor
SpyBug::SpyBug(Point pos, Ship *planter) : BurstProjectile(pos, Point())
{
   mObjectTypeNumber = SpyBugTypeNumber;

   mWeaponType = WeaponSpyBug;
   mNetFlags.set(Ghostable);

   if(planter)
   {
      setTeam(planter->getTeam());
      setOwner(planter->getOwner());
   }
   else
   {
      setTeam(TEAM_NEUTRAL);
      setOwner(NULL);
   }

   setExtent(Rect(pos, pos));

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
SpyBug::~SpyBug()
{
   if(getGame() && getGame()->isServer() && getGame()->getGameType())
      getGame()->getGameType()->catalogSpybugs();

   LUAW_DESTRUCTOR_CLEANUP;
}


SpyBug *SpyBug::clone() const
{
   return new SpyBug(*this);
}


bool SpyBug::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 3)
      return false;

   setTeam(atoi(argv[0]));

   // Strips off first arg from argv, so the parent gets the straight coordinate pair it's expecting
   if(!Parent::processArguments(2, &argv[1], game))    
      return false;

   return true;
}


// ProcessArguments() used is the one in item
string SpyBug::toString(F32 gridSize) const
{
   return string(Object::getClassName()) + " " + itos(getTeam()) + " " + geomToString(gridSize);
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
   MoveItem::idle(path);

   if(exploded || path != BfObject::ServerIdleMainLoop)
      return;
}


bool SpyBug::collide(BfObject *otherObj)
{
   if(isGhost())
      return false;  // avoid client side explode, server side don't explode
   if(isProjectileType(otherObj->getObjectTypeNumber()))
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
      writeThisTeam(stream);
      static StringTableEntry NO_OWNER = StringTableEntry();

      if(getOwner() != NULL)
         stream->writeStringTableEntry(getOwner()->getName());
      else
         stream->writeStringTableEntry(NO_OWNER);
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
      readThisTeam(stream);
      stream->readStringTableEntry(&mSetBy);
   }
   if(initial)
      SoundSystem::playSoundEffect(SFXSpyBugDeploy, getActualPos());
}


void SpyBug::renderItem(const Point &pos)
{
#ifndef ZAP_DEDICATED
   if(exploded)
      return;

   bool visible;

   Game *game = getGame();
   ClientGame *clientGame = static_cast<ClientGame *>(game);

   if(clientGame->getConnectionToServer())
   {
      GameConnection *conn = clientGame->getConnectionToServer();   
      Ship *ship = dynamic_cast<Ship *>(conn->getControlObject());

      if(!ship)
         return;

      GameType *gameType = clientGame->getGameType();

      // Can see bug if laid by teammate in team game || sensor is active ||
      //       you laid it yourself                   || spyBug is neutral
      visible = ( ((ship->getTeam() == getTeam()) && gameType->isTeamGame())   || ship->isModulePrimaryActive(ModuleSensor) ||
                  (conn && conn->getClientInfo()->getName() == mSetBy) || getTeam() == TEAM_NEUTRAL);
   }
   else    
      visible = true;      // We get here in editor when in preview mode

   renderSpyBug(pos, getColor(), visible, true);
#endif
}


void SpyBug::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   renderSpyBug(getRenderPos(), getColor(), true, true);
}


void SpyBug::renderDock()
{
#ifndef ZAP_DEDICATED
   const F32 radius = 9;

   Point pos = getRenderPos();

   glColor(getColor());
   drawFilledCircle(pos, radius);

   glColor(Colors::gray70);
   drawCircle(pos, radius);
   drawLetter('S', pos, Color(getTeam() < 0 ? .5 : .7), 1);    // Use darker gray for neutral spybugs so S will show up clearer
#endif
}


const char *SpyBug::getOnScreenName()     { return "Spy Bug";  }
const char *SpyBug::getOnDockName()       { return "Bug";      }
const char *SpyBug::getPrettyNamePlural() { return "Spy Bugs"; }
const char *SpyBug::getEditorHelpString() { return "Remote monitoring device that shows enemy ships on the commander's map. [Ctrl-B]"; }


bool SpyBug::hasTeam()      { return true;  }
bool SpyBug::canBeHostile() { return false; }
bool SpyBug::canBeNeutral() { return true;  }


// Can the player see the spybug?
bool SpyBug::isVisibleToPlayer(S32 playerTeam, StringTableEntry playerName, bool isTeamGame)
{
   // On our team (in a team game) || was set by us (in any game) || is neutral (in any game)
   return ((getTeam() == playerTeam) && isTeamGame) || playerName == mSetBy || getTeam() == TEAM_NEUTRAL;
}


/////
// Lua interface

const luaL_reg           SpyBug::luaMethods[]   = { { NULL, NULL } };
const LuaFunctionProfile SpyBug::functionArgs[] = { { NULL, { }, 0 } };


const char *SpyBug::luaClassName = "SpyBugItem";
REGISTER_LUA_SUBCLASS(SpyBug, BurstProjectile);


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(HeatSeekerProjectile);

// Constructor
HeatSeekerProjectile::HeatSeekerProjectile(Point pos, Point vel, BfObject *shooter): MoveItem(pos, true, mRadius, mMass)
{
   mObjectTypeNumber = HeatSeekerTypeNumber;

   mNetFlags.set(Ghostable);

   setActualPos(pos);
   setActualVel(vel);
   setMaskBits(PositionMask);
   mWeaponType = WeaponHeatSeeker;

   updateExtentInDatabase();

   mTimeRemaining = GameWeapon::weaponInfo[WeaponHeatSeeker].projLiveTime;
   exploded = false;

   if(shooter)
   {
      setOwner(shooter->getOwner());
      setTeam(shooter->getTeam());
      mShooter = shooter;
   }
   else
      setOwner(NULL);

   mAcquiredTarget = NULL;

   mRadius = 7;
   mMass = 1;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
HeatSeekerProjectile::~HeatSeekerProjectile()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


F32 HeatSeekerProjectile::AccelerationConstant = 1.02;
U32 HeatSeekerProjectile::TargetAcquisitionRadius = 800;

// Runs on client and server
void HeatSeekerProjectile::idle(IdleCallPath path)
{
   Parent::idle(path);

   if(path != BfObject::ServerIdleMainLoop)      // Server only from now on
      return;

   // Update time-to-live server-side
   S32 deltaT = mCurrentMove.time;
   if(!exploded)
   {
      if(mTimeRemaining <= deltaT)
         handleCollision(NULL, getActualPos());
      else
         mTimeRemaining -= deltaT;
   }

   // Do we already have a target?
   if(mAcquiredTarget)
   {
      // First, remove target if it is too far away.  Next tick will search for a new one
      Point delta = mAcquiredTarget->getPos() - getActualPos();
      if(delta.lenSquared() > TargetAcquisitionRadius * TargetAcquisitionRadius)
         mAcquiredTarget = NULL;

      // Else accelerate!
      else
      {
         // Create an velocity vector towards the target.  Average the old velocity vector and
         // a vector towards the target with the same magnitude.  Then add acceleration
         // TODO:  put a maximum on the speed?
         // TODO:  put a limit on angle changed per tick?  i.e. don't just split the angle in half
         //        with the averaging of the velocities
         Point targetVector = delta;
         targetVector.normalize(getActualVel().len());
         Point newVelocity = (targetVector + getActualVel()) / 2;

         newVelocity *= AccelerationConstant;
//         logprintf("speed: %f", newVelocity.len());
         setActualVel(newVelocity);

         // If we're right on top of the target, collide!
         // FIXME:  need a better way to tell when we hit the target from this class
         //         This fails miserably when the heat-seeker is going too fast
         F32 hitDistance = mRadius + Ship::CollisionRadius + 2;
         if(delta.lenSquared() < hitDistance * hitDistance)
            handleCollision(mAcquiredTarget, getActualPos());
      }
   }

   // No target acquired yet; time to search
   else
   {
      Rect queryRect(getActualPos(), TargetAcquisitionRadius);
      fillVector.clear();
      findObjects(isShipType, fillVector, queryRect);

      for(S32 i = 0; i < fillVector.size(); i++)
      {
         BfObject *foundObject = static_cast<BfObject *>(fillVector[i]);

         // TODO: determine nearest target?

         // Don't target self
         if(mShooter == foundObject)
            continue;

         Point delta = foundObject->getPos() - getActualPos();

         // Only acquire an object within a circle radius instead of query rect
         if(delta.lenSquared() > TargetAcquisitionRadius * TargetAcquisitionRadius)
            continue;

         // We found a target!
         mAcquiredTarget = foundObject;
         break;
      }
   }

   // XXX:  really old code from watusimoto on first run with heat-seeker.  still useful??
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
   //   BfObject *target = dynamic_cast<BfObject *>(targetItems[i]);
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
}


U32 HeatSeekerProjectile::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 ret = Parent::packUpdate(connection, updateMask, stream);

   stream->writeFlag(exploded);
   stream->writeFlag((updateMask & InitialMask) && (getGame()->getCurrentTime() - getCreationTime() < 500));
   return ret;
}


void HeatSeekerProjectile::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   TNLAssert(connection, "Invalid connection to server in BurstProjectile//projectile.cpp");

   if(stream->readFlag())
   {
      disableCollision();
      doExplosion(getActualPos());
   }

   if(stream->readFlag())
      SoundSystem::playSoundEffect(SFXBurstProjectile, getActualPos(), getActualVel());
}


void HeatSeekerProjectile::damageObject(DamageInfo *theInfo)
{
   // If we're being damaged by a burst, explode...
   if(theInfo->damageType == DamageTypeArea)
   {
      handleCollision(theInfo->damagingObject, getActualPos());
      return;
   }

   computeImpulseDirection(theInfo);

   setMaskBits(PositionMask);
}


void HeatSeekerProjectile::doExplosion(Point pos)
{
#ifndef ZAP_DEDICATED
   if(isGhost())
   {
      TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");
      static_cast<ClientGame *>(getGame())->emitBlast(pos, 100);          // New, manly explosion

      SoundSystem::playSoundEffect(SFXMineExplode, getActualPos());
   }
#endif
}


// Server-side only
void HeatSeekerProjectile::handleCollision(BfObject *hitObject, Point collisionPoint)
{
   // Damage the object we hit
   if(hitObject)
   {
      DamageInfo theInfo;

      theInfo.collisionPoint = collisionPoint;
      theInfo.damageAmount = GameWeapon::weaponInfo[mWeaponType].damageAmount;
      theInfo.damageType = DamageTypePoint;
      theInfo.damagingObject = this;
//      theInfo.impulseVector = mVelocity;
      theInfo.damageSelfMultiplier = GameWeapon::weaponInfo[mWeaponType].damageSelfMultiplier;

      hitObject->damageObject(&theInfo);

      if(getOwner())
         getOwner()->getStatistics()->countHit(mWeaponType);
   }

   mTimeRemaining = 0;
   exploded = true;
   setMaskBits(ExplodedMask);

   disableCollision();
   deleteObject(100);
}


bool HeatSeekerProjectile::collide(BfObject *otherObj)
{
   return true;
}


void HeatSeekerProjectile::renderItem(const Point &pos)
{
   if(exploded)
      return;

   F32 startLiveTime = (F32) GameWeapon::weaponInfo[mWeaponType].projLiveTime;
   renderHeatSeeker(pos, (startLiveTime - F32(getGame()->getCurrentTime() - getCreationTime())) / startLiveTime);
}


/////
// Lua interface

//               Fn name    Param profiles  Profile count
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getWeapon, ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(HeatSeekerProjectile, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(HeatSeekerProjectile, LUA_METHODS);

#undef LUA_METHODS


const char *HeatSeekerProjectile::luaClassName = "HeatSeeker";
REGISTER_LUA_SUBCLASS(HeatSeekerProjectile, MoveItem);

S32 HeatSeekerProjectile::getWeapon(lua_State *L) { return returnInt(L, mWeaponType); }



};

