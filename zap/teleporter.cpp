//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "teleporter.h"

#include "Colors.h"
#include "gameObjectRender.h"

#include "stringUtils.h"
#include "MathUtils.h"           // For sq
#include "GeomUtils.h"
#include "tnlRandom.h"
#include "game.h"
#include "ship.h"
#include "SoundSystemEnums.h"
#include "ClientInfo.h"
#include "gameConnection.h"

#ifndef ZAP_DEDICATED
#   include "ClientGame.h"
#endif

using namespace TNL;

namespace Zap
{

using namespace LuaArgs;

S32 DestManager::getDestCount() const
{
   return mDests.size();
}


Point DestManager::getDest(S32 index) const
{
   // If we have no desitnations, return the orgin for a kind of bizarre loopback effect
   if(mDests.size() == 0)
      return mOwner->getPos();

   return mDests[index];
}


S32 DestManager::getRandomDest() const
{
   if(mDests.size() == 0)
      return 0;
   else
      return (S32)TNL::Random::readI(0, mDests.size() - 1);
}


void DestManager::addDest(const Point &dest)
{
   TNLAssert(mOwner, "We need an owner here!");
   mDests.push_back(dest);
   if(mDests.size() == 1)      // Just added the first dest
   {
      mOwner->setVert(dest, 1);
      mOwner->updateExtentInDatabase();
   }
   // If we're the server, update the clients 
   if(mOwner->getGame() && mOwner->getGame()->isServer())
      mOwner->s2cAddDestination(dest);
}


void DestManager::setDest(S32 index, const Point &dest)
{
   TNLAssert(index < mDests.size(), "Invalid index!");
   mDests[index] = dest;
}


void DestManager::delDest(S32 index)
{
   if(mDests.size() == 1)
      clear();
   else
      mDests.erase(index);

   // If we're the server, update the clients -- this will likely be used rarely; not worth a new RPC, I think
   if(mOwner->getGame() && mOwner->getGame()->isServer())
   {
      mOwner->s2cClearDestinations();
      for(S32 i = 0; i < mDests.size(); i++)
         mOwner->s2cAddDestination(mDests[i]);
   }
}


void DestManager::clear()
{
   mDests.clear();
   mOwner->setVert(mOwner->getPos(), 1);       // Set destination to same as origin
   mOwner->updateExtentInDatabase();

   // If we're the server, update the clients 
   if(mOwner->getGame() && mOwner->getGame()->isServer())
      mOwner->s2cClearDestinations();
}


void DestManager::resize(S32 count)
{
   mDests.resize(count);
}


// Read a single dest
void DestManager::read(S32 index, BitStream *stream)
{
   mDests[index].read(stream);
}


// Read a whole list of dests
void DestManager::read(BitStream *stream)
{
   Types::read(*stream, &mDests);
}


const Vector<Point> *DestManager::getDestList() const
{
   return &mDests;
}


// Only done at creation time
void DestManager::setOwner(Teleporter *owner)
{
   mOwner = owner;
}

////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Teleporter);

static Vector<DatabaseObject *> foundObjects;      // Reusable container

const F32 Teleporter::DamageReductionFactor = 0.5f;

// Combined default C++/Lua constructor
Teleporter::Teleporter(lua_State *L)
{
   initialize(Point(0,0), Point(0,0), NULL);
   
   if(L)
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { PT, END }, { LINE, END } }, 3 };
      
      S32 profile = checkArgList(L, constructorArgList, "Teleporter", "constructor");

      if(profile > 0)
         doSetGeom(L);
   }
}


// Constructor used by engineer
Teleporter::Teleporter(const Point &pos, const Point &dest, Ship *engineeringShip) : Engineerable()
{
   initialize(pos, dest, engineeringShip);
}


// Destructor
Teleporter::~Teleporter()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


// Note that the teleporter may already have destinations when we get here... just something to keep in mind
void Teleporter::initialize(const Point &pos, const Point &dest, Ship *engineeringShip)
{
   mObjectTypeNumber = TeleporterTypeNumber;
   mNetFlags.set(Ghostable);

   mTime = 0;
   mTeleporterCooldown = TeleporterCooldown;    // Teleporters can have non-standard cooldown periods, but start with default
   setTeam(TEAM_NEUTRAL);

   setVert(pos, 0);
   setVert(dest, 1);

   mHasExploded = false;
   mFinalExplosionTriggered = false;
   mHealth = 1.0f;

   mLastDest = 0;
   mDestManager.setOwner(this);

   mEngineeringShip = engineeringShip;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


Teleporter *Teleporter::clone() const
{
   return new Teleporter(*this);
}


void Teleporter::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();    // Always in scope!
}


bool Teleporter::processArguments(S32 argc2, const char **argv2, Game *game)
{
   S32 argc = 0;
   const char *argv[8];

   for(S32 i = 0; i < argc2; i++)      // The idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char firstChar = argv2[i][0];    // First character of arg

      if((firstChar >= 'a' && firstChar <= 'z') || (firstChar >= 'A' && firstChar <= 'Z'))  // starts with a letter
      {
         if(!strnicmp(argv2[i], "Delay=", 6))
            mTeleporterCooldown = MAX(100, U32(atof(&argv2[i][6]) * 1000));
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

   if(argc != 4)
      return false;

   Point pos, dest;

   pos.read(argv);
   dest.read(argv + 2);

   pos  *= game->getLegacyGridSize();
   dest *= game->getLegacyGridSize();

   setVert(pos,  0);
   setVert(dest, 1);

   // See if we already have any teleports with this pos... if so, this is a "multi-dest" teleporter.
   // Note that editor handles multi-dest teleporters as separate single dest items, so this only runs on server!
   if(game->isServer())    
   {
      foundObjects.clear();
      game->getGameObjDatabase()->findObjects(TeleporterTypeNumber, foundObjects, Rect(pos, 1));

      for(S32 i = 0; i < foundObjects.size(); i++)
      {
         Teleporter *tel = static_cast<Teleporter *>(foundObjects[i]);
         if(tel->getOrigin().distSquared(pos) < 1)     // i.e These are really close!  Must be the same!
         {
            tel->addDest(dest);

            // See http://www.parashift.com/c++-faq-lite/delete-this.html for thoughts on delete this here
            delete this;    // Since this is really part of a different teleporter, delete this one 
            return true;    // There will only be one!
         }
      }

      // New teleporter origin
      addDest(dest);
      computeExtent(); // for ServerGame extent
   }
#ifndef ZAP_DEDICATED
   else     // Is client
   {
      addDest(dest);
      setExtent(calcExtents()); // for editor
   }
#endif

   return true;
}


TNL_IMPLEMENT_NETOBJECT_RPC(Teleporter, s2cAddDestination, (Point dest), (dest),
   NetClassGroupGameMask, RPCGuaranteed, RPCToGhost, 0)
{
   addDest(dest);
}


TNL_IMPLEMENT_NETOBJECT_RPC(Teleporter, s2cClearDestinations, (), (),
   NetClassGroupGameMask, RPCGuaranteed, RPCToGhost, 0)
{
   clearDests();
}


string Teleporter::toLevelCode() const
{
   string out = string(appendId(getClassName())) + " " + geomToLevelCode();

   if(mTeleporterCooldown != TeleporterCooldown)
      out += " Delay=" + ftos(mTeleporterCooldown / 1000.f, 3);
   return out;
}


Rect Teleporter::calcExtents()
{
   Rect rect(getVert(0), getVert(1));
   rect.expand(Point(Teleporter::TELEPORTER_RADIUS, Teleporter::TELEPORTER_RADIUS));
   return rect;
}


bool Teleporter::checkDeploymentPosition(const Point &position, const GridDatabase *gb, const Ship *ship)
{
   Rect queryRect(position, TELEPORTER_RADIUS);
   Point outPoint;  // only used as a return value in polygonCircleIntersect

   foundObjects.clear();
   gb->findObjects((TestFunc) isCollideableType, foundObjects, queryRect);

   Point foundObjectCenter;
   F32 foundObjectRadius;

   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      BfObject *bfObject = static_cast<BfObject *>(foundObjects[i]);

      // Skip if found objects are same team as the ship that is deploying the teleporter
      if(bfObject->getTeam() == ship->getTeam())
         continue;

      // Now calculate bounds
      const Vector<Point> *foundObjectBounds = bfObject->getCollisionPoly();
      if(foundObjectBounds)
      {
         // If they intersect, then bad deployment position
         if(foundObjectBounds->size() != 0)
            if(polygonCircleIntersect(foundObjectBounds->address(), foundObjectBounds->size(), position, sq(TELEPORTER_RADIUS), outPoint))
               return false;
      }
      // Try the collision circle if no poly bounds were found
      else
      {
         if(bfObject->getCollisionCircle(RenderState, foundObjectCenter, foundObjectRadius))
            if(circleCircleIntersect(foundObjectCenter, foundObjectRadius, position, (F32)TELEPORTER_RADIUS))
               return false;
      }

   }

   return true;
}


U32 Teleporter::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitMask))
   {
      stream->writeFlag(mEngineered);

      if(stream->writeFlag(mTeleporterCooldown != TeleporterCooldown))  // Most teleporter will have default timing
         stream->writeInt(mTeleporterCooldown, 32);

      if(mTeleporterCooldown != 0 && stream->writeFlag(mTeleportCooldown.getCurrent() != 0))
         stream->writeInt(mTeleportCooldown.getCurrent(), 32);    // A player might join while this teleporter is in the middle of cooldown  // TODO: Make this a rangedInt
   }

   else if(stream->writeFlag(updateMask & TeleportMask))          // This gets triggered if a ship passes through
   {
      TNLAssert(U32(mLastDest) < U32(getDestCount()), "packUpdate out of range teleporter number");
      stream->write(mLastDest);                                   // Where ship is going
   }

   if(stream->writeFlag(updateMask & (InitMask | GeomMask)))
   {
      getOrigin().write(stream);     // Location of intake
      
      S32 dests = getDestCount();

      stream->writeInt(dests, 16);

      for(S32 i = 0; i < dests; i++)
         getDest(i).write(stream);
   }
   
   // If we're not destroyed and health has changed
   if(!stream->writeFlag(mHasExploded))
   {
      if(stream->writeFlag(updateMask & HealthMask))
         stream->writeFloat(mHealth, 6);
   }

   return 0;
}


void Teleporter::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())                 // InitMask
   {
      mEngineered = stream->readFlag();

      if(stream->readFlag())
         mTeleporterCooldown = stream->readInt(32);

      if(mTeleporterCooldown != 0 && stream->readFlag())
         mTeleportCooldown.reset(stream->readInt(32));
   }
   else  if(stream->readFlag())                 // TeleportMask
   {
      S32 dest;
      stream->read(&dest);

#ifndef ZAP_DEDICATED
      TNLAssert(U32(dest) < U32(getDestCount()), "unpackUpdate out of range teleporter number");
      if(U32(dest) < U32(getDestCount()))
      {
         TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");
         static_cast<ClientGame *>(getGame())->emitTeleportInEffect(mDestManager.getDest(dest), 0);

         getGame()->playSoundEffect(SFXTeleportIn, mDestManager.getDest(dest));
      }

      getGame()->playSoundEffect(SFXTeleportOut, getOrigin());
#endif
      mTeleportCooldown.reset(mTeleporterCooldown);
   }

   if(stream->readFlag())                 // InitMask || GeomMask
   {
      U32 count;
      Point pos;

      pos.read(stream);
      setVert(pos, 0);                    // Location of intake
      setVert(pos, 1);                    // Simulate a point geometry -- will be changed later when we add our first dest

      count = stream->readInt(16);
      mDestManager.resize(count);         // Prepare the list for multiple additions

      for(U32 i = 0; i < count; i++)
         mDestManager.read(i, stream);
      
      computeExtent();
      generateOutlinePoints();

   }

   if(stream->readFlag())     // mHasExploded
   {
      if(!mHasExploded)       // Protect against double explosions
      {
         mHasExploded = true;
         disableCollision();
         mExplosionTimer.reset(TeleporterExplosionTime);
         getGame()->playSoundEffect(SFXTeleportExploding, getOrigin());
         mFinalExplosionTriggered = false;
      }
   }

   else if(stream->readFlag())      // HealthMask -- only written when hasExploded is false
      mHealth = stream->readFloat(6);
}


F32 Teleporter::getHealth() const
{
   return mHealth;
}


void Teleporter::damageObject(DamageInfo *theInfo)
{
   // Only engineered teleports can be damaged
   if(!mEngineered)
      return;

   if(mHasExploded)
      return;

   // Do the damage
   if(theInfo->damageAmount > 0)
      mHealth -= theInfo->damageAmount * DamageReductionFactor;
   else  // For repair
      mHealth -= theInfo->damageAmount;

   // Check bounds
   if(mHealth < 0)
      mHealth = 0;
   else if(mHealth > 1)
      mHealth = 1;

   setMaskBits(HealthMask);

   // Destroyed!
   if(mHealth <= 0)
      onDestroyed();
}


bool Teleporter::isDestroyed()
{
   return mHasExploded;
}


void Teleporter::onDestroyed()
{
   mHasExploded = true;

   releaseResource(getOrigin(), getGame()->getGameObjDatabase());

   deleteObject(TeleporterExplosionTime + 500);  // Guarantee our explosion effect will complete
   setMaskBits(DestroyedMask);

   if(mEngineeringShip && mEngineeringShip->getEngineeredTeleporter() == this && mEngineeringShip->getClientInfo())
   {
      mEngineeringShip->getClientInfo()->sTeleporterCleanup();

      if(!mEngineeringShip->getClientInfo()->isRobot())     // If they're not a bot, tell client to hide engineer menu
      {
         static const StringTableEntry Your_Teleporter_Got_Destroyed("Your teleporter got destroyed");
         mEngineeringShip->getClientInfo()->getConnection()->s2cDisplayErrorMessage(Your_Teleporter_Got_Destroyed);
         mEngineeringShip->getClientInfo()->getConnection()->s2cEngineerResponseEvent(EngineeredTeleporterExit);
      }
   }
}

void Teleporter::doTeleport()
{
   static const F32 TRIGGER_RADIUS  = F32(TELEPORTER_RADIUS - Ship::CollisionRadius);
   static const F32 TELEPORT_RADIUS = F32(TELEPORTER_RADIUS + Ship::CollisionRadius);

   if(getDestCount() == 0)      // Ignore 0-dest teleporters -- where would you go??
      return;

   Rect queryRect(getOrigin(), TRIGGER_RADIUS);     

   foundObjects.clear();
   findObjects((TestFunc)isShipType, foundObjects, queryRect);

   S32 dest = mDestManager.getRandomDest();

   Point teleportCenter = getOrigin();

   bool isTriggered = false; // First, check if triggered, can come from idle timer.
   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      if((teleportCenter - static_cast<Ship *>(foundObjects[i])->getRenderPos()).lenSquared() < sq(TRIGGER_RADIUS))
      {
         isTriggered = true;
         break;
      }
   }

   if(!isTriggered)
      return;

   setMaskBits(TeleportMask);
   mTeleportCooldown.reset(mTeleporterCooldown);      // Teleport needs to wait a bit before being usable again

   // We've triggered the teleporter.  Relocate any ships within range.  Any ship touching the teleport will be warped.
   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      Ship *ship = static_cast<Ship *>(foundObjects[i]);
      if((teleportCenter - ship->getRenderPos()).lenSquared() < sq(TELEPORT_RADIUS))
      {
         mLastDest = dest;    // Save the destination

         Point newPos = ship->getActualPos() - teleportCenter + mDestManager.getDest(dest);
         ship->setActualPos(newPos, true);

         if(ship->getClientInfo() && ship->getClientInfo()->getStatistics())
            ship->getClientInfo()->getStatistics()->mTeleport++;

         // See if we've teleported onto a zone of some sort
         BfObject *zone = ship->isInAnyZone();
         if(zone)
            zone->collide(ship);
      }
   }

   Vector<DatabaseObject *> foundTeleporters; // Must be kept local, non static, because of possible recursive.
   queryRect.set(getOrigin(), TRIGGER_RADIUS);
   findObjects(TeleporterTypeNumber, foundTeleporters, queryRect);
   for(S32 i = 0; i < foundTeleporters.size(); i++)
      if(static_cast<Teleporter *>(foundTeleporters[i])->mTeleportCooldown.getCurrent() == 0)
         static_cast<Teleporter *>(foundTeleporters[i])->doTeleport();
}


bool Teleporter::collide(BfObject *otherObject)
{
   U8 otherObjectType = otherObject->getObjectTypeNumber();

   if(isShipType(otherObjectType))     // i.e. ship or robot
   {
      static const F32 TRIGGER_RADIUS  = F32(TELEPORTER_RADIUS - Ship::CollisionRadius);

      if(isGhost())
         return false; // Server only

      if(mTeleportCooldown.getCurrent() > 0)    // Ignore teleports in cooldown mode
         return false;

      if(getDestCount() == 0)      // Ignore 0-dest teleporters -- where would you go??
         return false;

      if(mHasExploded)                          // Destroyed teleports don't work so well anymore...
         return false;

      // First see if we've triggered the teleport...
      Ship *ship = static_cast<Ship *>(otherObject);

      // Check if the center of the ship is closer than TRIGGER_RADIUS -- this is equivalent to testing if
      // the ship is entirely within the outer radius of the teleporter.  Therefore, ships can almost entirely 
      // overlap the teleporter before triggering the teleport.
      if((getOrigin() - ship->getActualPos()).lenSquared() > sq(TRIGGER_RADIUS))  
         return false;     // Too far -- teleport not activated!

      // Check for players within a square box around the teleporter.  Not all these ships will teleport; the actual determination is made
      // via a circle, and will be checked below.

      doTeleport();

      return true;
   }

   // Only engineered teleports have collision with projectiles
   if(isProjectileType(otherObjectType))
      return mEngineered;        

   return false;
}


bool Teleporter::getCollisionCircle(U32 state, Point &center, F32 &radius) const
{
   center = getOrigin();
   radius = TELEPORTER_RADIUS;
   return true;
}


const Vector<Point> *Teleporter::getCollisionPoly() const
{
   return NULL;
}


void Teleporter::generateOutlinePoints()
{
   static const S32 sides = 10;

   mOutlinePoints.resize(sides);

   F32 x = getOrigin().x;
   F32 y = getOrigin().y;

   for(S32 i = 0; i < sides; i++)    
      mOutlinePoints[i] = Point(TELEPORTER_RADIUS * cos(i * Float2Pi / sides + FloatHalfPi) + x, 
                                TELEPORTER_RADIUS * sin(i * Float2Pi / sides + FloatHalfPi) + y);
}


// Need a different outline here when in editor v when in game
const Vector<Point> *Teleporter::getOutline() const
{
   return &mOutlinePoints;
}


const Vector<Point> *Teleporter::getEditorHitPoly() const
{
   return Parent::getOutline();
}



void Teleporter::computeExtent()
{
   setExtent(Rect(getOrigin(), TELEPORTER_RADIUS));    // This Rect constructor takes a diameter, not a radius
}


inline Point Teleporter::getOrigin() const
{
   return getVert(0);
}


S32 Teleporter::getDestCount() const
{
   return mDestManager.getDestCount();
}


Point Teleporter::getDest(S32 index) const
{
   return mDestManager.getDest(index);
}


void Teleporter::addDest(const Point &dest)
{
   mDestManager.addDest(dest);
}


void Teleporter::delDest(S32 index)
{
   mDestManager.delDest(index);
}


void Teleporter::clearDests()
{
   mDestManager.clear();
}


void Teleporter::onConstructed()
{
   // Do nothing
}


bool Teleporter::hasAnyDests() const
{
   return getDestCount() > 0;
}


// Server only, also called from editor
void Teleporter::setEndpoint(const Point &point)
{
   mDestManager.addDest(point);
   setVert(point, 1);
}


const Vector<Point> *Teleporter::getDestList() const
{
   return mDestManager.getDestList();
}


void Teleporter::idle(IdleCallPath path)
{
   U32 deltaT = mCurrentMove.time;
   mTime += deltaT;

   // Client only
   if(path == ClientIdlingNotLocalShip)
   {
      // Update Explosion Timer
      if(mHasExploded)
         mExplosionTimer.update(deltaT);
   }

   if(mTeleportCooldown.update(deltaT) && path == ServerIdleMainLoop)
      doTeleport();
}


void Teleporter::render()
{
#ifndef ZAP_DEDICATED
   // Render at a different radius depending on if a ship has just gone into the teleport
   // and we are waiting for the teleport timeout to expire
   F32 radiusFraction;
   if(!mHasExploded)
   {
      U32 cooldown = mTeleportCooldown.getCurrent();

      if(cooldown == 0)
         radiusFraction = 1;

      else if(cooldown > TeleporterExpandTime - TeleporterCooldown + mTeleporterCooldown)
         radiusFraction = F32(cooldown - TeleporterExpandTime + TeleporterCooldown - mTeleporterCooldown) / 
                          F32(TeleporterCooldown - TeleporterExpandTime);

      else if(mTeleporterCooldown < TeleporterExpandTime)
         radiusFraction = F32(mTeleporterCooldown - cooldown + TeleporterExpandTime - TeleporterCooldown) / 
                          F32(mTeleporterCooldown + TeleporterExpandTime - TeleporterCooldown);

      else if(cooldown < TeleporterExpandTime)
         radiusFraction = F32(TeleporterExpandTime - cooldown) / 
                          F32(TeleporterExpandTime);
      else
         radiusFraction = 0;
   }
   else
   {
      // If the teleport has been destroyed, adjust the radius larger/smaller for a neat effect
      U32 halfPeriod = mExplosionTimer.getPeriod() / 2;

      if(mExplosionTimer.getCurrent() > halfPeriod)
         radiusFraction = 2.f - F32(mExplosionTimer.getCurrent() - halfPeriod) / 
                                F32(halfPeriod);

      else
         radiusFraction = 2 * F32(mExplosionTimer.getCurrent()) / 
                              F32(halfPeriod);

      // Add ending explosion
      if(mExplosionTimer.getCurrent() == 0 && !mFinalExplosionTriggered)
         doExplosion();
   }

   if(radiusFraction != 0)
   {
      U32 trackerCount = 100;    // Trackers are the swirling bits in a teleporter.  100 gives the "classic" appearance.
      if(mHealth < 1.f)
         trackerCount = U32(mHealth * 75.f) + 25;

      F32 zoomFraction = getGame()->getCommanderZoomFraction();
      U32 renderStyle = mEngineered ? 2 : 0;
      renderTeleporter(getOrigin(), renderStyle, true, mTime, zoomFraction, radiusFraction, 
                       (F32)TELEPORTER_RADIUS, 1.0, mDestManager.getDestList(), trackerCount);
   }

   if(mEngineered)
   {
      // Render the exit of engineered teleports with an outline.  If teleporter has exploded, implode the exit.
      // The implosion calculations were an attempt to avoid using another timer, but perhaps that would be clearer than this mess
      const F32 IMPLOSION_FACTOR = 0.2f;     // Smaller numbers = faster implosion

      F32 implosionOffset  = mExplosionTimer.getPeriod() * (1 - IMPLOSION_FACTOR);
      F32 implosionTime    = mExplosionTimer.getPeriod() * IMPLOSION_FACTOR;

      F32 sizeFraction = mHasExploded ? F32(mExplosionTimer.getCurrent() - implosionOffset) / implosionTime : 1;

      if(sizeFraction > 0)
         for(S32 i = getDestCount() - 1; i >= 0; i--)
            renderTeleporterOutline(mDestManager.getDest(i), (F32)TELEPORTER_RADIUS * sizeFraction, Colors::richGreen);
   }
#endif
}


#ifndef ZAP_DEDICATED
void Teleporter::doExplosion()
{
   mFinalExplosionTriggered = true;
   const S32 EXPLOSION_COLOR_COUNT = 12;

   static Color ExplosionColors[EXPLOSION_COLOR_COUNT] = {
         Colors::green,
         Color(0, 1, 0.5),
         Colors::white,
         Colors::yellow,
         Colors::green,
         Color(0, 0.8, 1.0),
         Color(0, 1, 0.5),
         Colors::white,
         Colors::green,
         Color(0, 1, 0.5),
         Colors::white,
         Colors::yellow,
   };

   getGame()->playSoundEffect(SFXShipExplode, getPos());

   F32 a = TNL::Random::readF() * 0.4f  + 0.5f;
   F32 b = TNL::Random::readF() * 0.2f  + 0.9f;
   F32 c = TNL::Random::readF() * 0.15f + 0.125f;
   F32 d = TNL::Random::readF() * 0.2f  + 0.9f;

   ClientGame *game = static_cast<ClientGame *>(getGame());

   Point pos = getPos();

   game->emitExplosion(pos, 0.65f, ExplosionColors, EXPLOSION_COLOR_COUNT);
   game->emitBurst(pos, Point(a,c) * 0.6f, Colors::yellow, Colors::green);
   game->emitBurst(pos, Point(b,d) * 0.6f, Colors::yellow, Colors::green);
}
#endif


void Teleporter::renderEditorItem()
{
#ifndef ZAP_DEDICATED
   renderTeleporterEditorObject(getOrigin(), TELEPORTER_RADIUS, getEditorRenderColor());
#endif
}


Color Teleporter::getEditorRenderColor()
{
   return Colors::green;
}


void Teleporter::newObjectFromDock(F32 gridSize)
{
   addDest(Point(0,0));      // We need to have one for rendering preview... actual location will be updated in onGeomChanged()
   Parent::newObjectFromDock(gridSize);
}


void Teleporter::onAttrsChanging()
{
   /* Do nothing */
}


void Teleporter::onGeomChanging()
{
   onGeomChanged();
}


void Teleporter::onGeomChanged()
{
   Parent::onGeomChanged();

   // Update the dest manager.  We need this for rendering in preview mode.
   mDestManager.setDest(0, getVert(1));
}   


const char *Teleporter::getOnScreenName()     { return "Teleporter";    }
const char *Teleporter::getPrettyNamePlural() { return "Teleporters"; }
const char *Teleporter::getOnDockName()       { return "Teleporter";    }
const char *Teleporter::getEditorHelpString() { return "Teleports ships from one place to another. [T]"; }


bool Teleporter::hasTeam()      { return false; }
bool Teleporter::canBeHostile() { return false; }
bool Teleporter::canBeNeutral() { return false; }


//// Lua methods

/**
 * @luafunc Teleporter::Teleporter()
 * @luafunc Teleporter::Teleporter(geom)
 * @luaclass Teleporter
 *
 * @brief Instantly transports ships from here to there.
 *
 * @descr A Teleporter represents the basic teleporter object. Every teleporter
 * has an intake location and one or more destinations. When a ship enters the
 * teleporter, a destination will be chosen randomly if there is more than one. 
 */
//               Fn name                       Param profiles         Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, addDest,       ARRAYDEF({ { PT,   END }                }), 1 ) \
   METHOD(CLASS, delDest,       ARRAYDEF({ { INT,  END }                }), 1 ) \
   METHOD(CLASS, clearDests,    ARRAYDEF({ {       END }                }), 1 ) \
   METHOD(CLASS, getDest,       ARRAYDEF({ { INT,  END }                }), 1 ) \
   METHOD(CLASS, getDestCount,  ARRAYDEF({ {       END }                }), 1 ) \
   METHOD(CLASS, setGeom,       ARRAYDEF({ { PT,   END }, { LINE, END } }), 2 ) \
   METHOD(CLASS, getEngineered, ARRAYDEF({ {       END }                }), 1 ) \
   METHOD(CLASS, setEngineered, ARRAYDEF({ { BOOL, END }                }), 1 ) \

GENERATE_LUA_METHODS_TABLE(Teleporter, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(Teleporter, LUA_METHODS);

#undef LUA_METHODS


const char *Teleporter::luaClassName = "Teleporter";
REGISTER_LUA_SUBCLASS(Teleporter, BfObject);

/** 
 * @luafunc Teleporter::addDest(dest)
 *
 * @brief Adds a destination to the teleporter.
 *
 * @param dest A point or coordinate pair representing the location of the
 * destination.
 *
 * Example:
 * @code 
 *   t = Teleporter.new()
 *   t:addDest(100,150)
 *   levelgen:addItem(t)  -- or plugin:addItem(t) in a plugin
 * @endcode
 */
S32 Teleporter::lua_addDest(lua_State *L)
{
   checkArgList(L, functionArgs, "Teleporter", "addDest");

   Point point = getPointOrXY(L, 1);
   addDest(point);

   return 0;
}


/**
 * @luafunc Teleporter::delDest(int index)
 * 
 * @brief Removes a destination from the teleporter.
 * 
 * @param index The index of the destination to delete. If you specify an
 * invalid index, will generate an error.
 * 
 * @note Remember that in Lua, indices start with 1!
 */
S32 Teleporter::lua_delDest(lua_State *L)
{
   checkArgList(L, functionArgs, "Teleporter", "delDest");

   S32 index = getInt(L, 1, "Teleporter:delDest()", 1, getDestCount());

   index--;    // Adjust for Lua's 1-based index

   delDest(index);

   return 0;
}


/**
 * @luafunc Teleporter::clearDests()
 *
 * @brief Removes all destinations from the teleporter.
 */
S32 Teleporter::lua_clearDests(lua_State *L)
{
   checkArgList(L, functionArgs, "Teleporter", "clearDests");

   clearDests();
   return 0;
}


/**
 * @luafunc point Teleporter::getDest(int index)
 *
 * @brief Returns the specified destination.
 *
 * @param index Index of the dest to return. Will generate an error if index is
 * invalid.
 *
 * @return A point object representing the requested destination.
 */
S32 Teleporter::lua_getDest(lua_State *L)
{
   checkArgList(L, functionArgs, "Teleporter", "getDest");
   S32 index = getInt(L, 1) - 1;    // - 1 corrects for Lua indices starting at 1

   if(index < 0 || index >= getDestCount())
      throw LuaException("Index out of range (requested " + itos(index) + ")");

   return returnPoint(L, mDestManager.getDest(index));
}


/**
 * @luafunc int Teleporter::getDestCount()
 *
 * @brief Returns the number of destinations this teleporter has.
 *
 * @return The number of destinations this teleporter has.
 */
S32 Teleporter::lua_getDestCount(lua_State *L)
{
   return returnInt(L, getDestCount());
}


/**
 * @luafunc bool Teleporter::getEngineered()
 *
 * @return `true` if the item can be destroyed, `false` otherwise.
 */
S32 Teleporter::lua_getEngineered(lua_State *L)
{
   return returnBool(L, mEngineered);
}


/**
 * @luafunc Teleporter::setEngineered(bool engineered)
 *
 * @brief Sets whether the item can be destroyed when its health reaches zero.
 *
 * @param engineered `true` to make the item destructible, `false` to make it permanent.
 */
S32 Teleporter::lua_setEngineered(lua_State *L)
{
   checkArgList(L, functionArgs, "Teleporter", "setEngineered");

   mEngineered = getBool(L, 1);
   setMaskBits(InitMask);

   return returnBool(L, mEngineered);
}


// Overrides

/**
 * @luafunc Teleporter::setGeom(geom geometry)
 * @brief Sets teleporter geometry; differs from standard conventions.
 * @descr In this case, geometry represents both Teleporter's location and those
 * of all destinations.  The first point specified will be used to set the
 * location. If two or more points are supplied, all existing destinations will be deleted, 
 * and the remaining points will be used to define new destinations.
 *
 * Note that in the editor, teleporters can only have a single destination.
 * Since scripts can add or modify editor items, when the script has finished
 * running, all affected teleporters will be converted into a series of single
 * destination items, all having the same location but with different
 * destinations.  (In a game, multi-destination teleporters will remain as created.)
 *
 * If the teleporter has no destinations, it will not be added to the editor.
 *
 * @param geometry New geometry for Teleporter.
 */
S32 Teleporter::lua_setGeom(lua_State *L)
{
   checkArgList(L, functionArgs, "Teleporter", "setGeom");

   doSetGeom(L);

   return 0;
}


// Helper for Lua constructor and setGeom(L) methods
void Teleporter::doSetGeom(lua_State *L)
{
   doSetGeom(getPointsOrXYs(L, 1));
}


// If points only contains a single point, only the origin will change; destinations will
// remain unaltered.
void Teleporter::doSetGeom(const Vector<Point> &points)
{
   if(points.size() == 0)     // No points, no action
      return;

   setVert(points[0], 0);     // First point is the origin

   if(points.size() >= 2)     // Subsequent points are destinations
   {
      clearDests();

      // Notify destination manager about the new destinations
      for(S32 i = 1; i < points.size(); i++)
         addDest(points[i]);

      TNLAssert(getDest(0) == points[1], "Should have been set by addDest()!");
   }
   else
      setVert(points[0], 1);  // Set default dest -- maybe not important to set this

   computeExtent();
   setMaskBits(GeomMask);
}


/**
 * @luafunc Geom Teleporter::getGeom()
 * 
 * @brief Gets teleporter geometry; differs from standard conventions.
 * 
 * @descr In this case, geometry represents both Teleporter's location and those
 * of all destinations. The first point in the Geom will be the teleporter's
 * intake location. Each destination will be represented by an additional point.
 * In the editor, all teleporters are simple lines, and will return geometries
 * with two points -- an origin and a destination.
 * 
 * @param Geom geometry: New geometry for Teleporter.
 */
S32 Teleporter::lua_getGeom(lua_State *L)
{
   Vector<Point> points;

   points.push_back(getPos());

   for(S32 i = 0; i < getDestCount(); i++)
      points.push_back(mDestManager.getDest(i));

   return returnPoints(L, &points);
}

   
};
