//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ship.h"

#include "projectile.h"
#include "gameType.h"
#include "Zone.h"
#include "Colors.h"
#include "teleporter.h"
#include "speedZone.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#endif

#include "gameObjectRender.h"

#include "stringUtils.h"   // For itos
#include "MathUtils.h"     // For radiansToDegrees
#include "GeomUtils.h"


#ifdef TNL_OS_WIN32
#  include <windows.h>     // For ARRAYSIZE
#endif

#define hypot _hypot    // Kill some warnings


namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(Ship);

#ifdef _MSC_VER
#  pragma warning(disable:4355)
#endif


// Constructor
// Note that on client, we use all default values set in declaration; on the server, these values will be provided
// Most of these values are set in the initial packet set from the server (see packUpdate() below)
// Also, the following is also run by robot's constructor
Ship::Ship(ClientInfo *clientInfo, S32 team, const Point &pos, bool isRobot) : MoveObject(pos, (F32)CollisionRadius), mSpawnPoint(pos)
{
   initialize(clientInfo, team, pos, isRobot);
}


// Combined Lua / C++ default constructor -- this is used by Lua and by TNL, so we need to programatically separate the two
Ship::Ship(lua_State *L) : MoveObject(Point(0,0), (F32)CollisionRadius)
{
   if(L)
   {
      luaL_error(L, "Currently cannot instantiate a Ship object from Lua.");
      return;
   }

   initialize(NULL, TEAM_NEUTRAL, Point(0,0), false);
}


// Destructor
Ship::~Ship()
{
   dismountAll();

   // It may be that mClientInfo has already been assigned a new ship; we only want to NULL it out
   // in the event it hasn't.  At the end of a game, mClientInfo may disappear before the ship does.
   if(mClientInfo && mClientInfo->getShip() == this)
      mClientInfo->setShip(NULL);   // Don't leave a dangling pointer

   LUAW_DESTRUCTOR_CLEANUP;
}


void Ship::initialize(ClientInfo *clientInfo, S32 team, const Point &pos, bool isRobot)
{
   static const U32 ModuleSecondaryTimerDelay = 500;
   static const U32 SpyBugPlacementTimerDelay = 800;
   static const U32 IdleRechargeCycleTimerDelay = 2000;

   mObjectTypeNumber = PlayerShipTypeNumber;
   mFireTimer = 0;
   mFastRecharging = false;
   mLastProcessStateAngle = 0;

   mEngineeredTeleporter = NULL;

   // Set up module secondary delay timer
   for(S32 i = 0; i < ModuleCount; i++)
      mModuleSecondaryTimer[i].setPeriod(ModuleSecondaryTimerDelay);

   mSpyBugPlacementTimer.setPeriod(SpyBugPlacementTimerDelay);
   mSensorEquipZoomTimer.setPeriod(SensorZoomTime);
   mFastRechargeTimer.reset(IdleRechargeCycleTimerDelay, IdleRechargeCycleTimerDelay);
   mSendSpawnEffectTimer.setPeriod(300);

   mNetFlags.set(Ghostable);

#ifndef ZAP_DEDICATED
   for(U32 i = 0; i < TrailCount; i++)
      mLastTrailPoint[i] = -1;   // Or something... doesn't really matter what
#endif

   mClientInfo = clientInfo;     // Will be NULL if being created by TNL

   if(mClientInfo)
      mClientInfo->setShip(this);

   setTeam(team);
   mass = 1.0;            // Ship's mass, not used

   // Name will be unique across all clients, but client and server may disagree on this name if the server has modified it to make it unique

   mIsRobot = isRobot;

   if(!isRobot)               // Robots will run this during their own initialization; no need to run it twice!
      initialize(pos);
   else
      hasExploded = false;    // Client needs this false for unpackUpdate

   mZones1IsCurrent = true;

#ifndef ZAP_DEDICATED
   mSparkElapsed = 0;
   mShapeType = ShipShape::Normal;
#endif

   mLoadout.setLoadout(DefaultLoadout);

   // Added to keep the old loadout and keep the currently selected weapon. (Ship delete/new on player's respawn)
   if(clientInfo && clientInfo->getOldLoadout().getModule(0) != ModuleNone)
      mLoadout = clientInfo->getOldLoadout();

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


Ship *Ship::clone() const
{
   return new Ship(*this);
}


// Compare a client copy of a ship to the server copy and see if they are "equal"
bool Ship::isServerCopyOf(const Ship &clientShip) const
{
    if(mLoadout != clientShip.mLoadout)
       return false;

    if(mHealth != clientShip.mHealth || mEnergy != clientShip.mEnergy || getTeam() != clientShip.getTeam())
       return false;

    // Server sends renderPos/vel, client stores those in actualPos/vel
    if(getRenderPos() != clientShip.getActualPos() || getRenderVel() != clientShip.getRenderVel())
       return false;

    if(!mCurrentMove.isEqualMove(&clientShip.mCurrentMove))
       return false;

    if(mMountedItems.size() != clientShip.mMountedItems.size())
       return false;

    for(S32 i = 0; i < mMountedItems.size(); i++)
       if(mMountedItems[i]->getObjectTypeNumber() != clientShip.mMountedItems[i]->getObjectTypeNumber())
          return false;

    return true;
}


// Initialize some things that both ships and bots care about... this will get run during the ship's constructor
// and also after a bot respawns and needs to reset itself
void Ship::initialize(const Point &pos)
{
   // Does this ever evaluate to true?
   if(getGame())
      mSendSpawnEffectTimer.reset();

   setPosVelAng(pos, Point(0,0), 0);

   updateExtentInDatabase();

   mHealth = 1.0;       // Start at full health
   hasExploded = false; // Haven't exploded yet!

#ifndef ZAP_DEDICATED
   for(S32 i = 0; i < TrailCount; i++)          // Clear any vehicle trails
      mTrail[i].reset();
#endif

   mEnergy = (S32) ((F32) EnergyMax * .80);     // Start off with 80% energy

   mCooldownNeeded = false;

   // Start spawn shield timer
   mSpawnShield.reset(SpawnShieldTime);
}


bool Ship::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc != 3)
      return false;

   Point pos;
   pos.read(argv + 1);
   pos *= game->getLegacyGridSize();
   for(U32 i = 0; i < MoveStateCount; i++)
   {
      setPos(i, pos);
      setAngle(i, 0);
   }

   return true;
}


string Ship::toLevelCode() const
{
   return string(getClassName()) + " " + itos(getTeam()) + " " + geomToLevelCode();
}


ClientInfo *Ship::getClientInfo() const
{
   return mClientInfo;
}


bool Ship::canAddToEditor()          { return false;  }      // No ships in the editor
const char *Ship::getOnScreenName()  { return "Ship"; }


void Ship::setEngineeredTeleporter(Teleporter *teleporter)
{
   mEngineeredTeleporter = teleporter;
}


Teleporter *Ship::getEngineeredTeleporter()
{
   return mEngineeredTeleporter;
}


void Ship::onGhostRemove()
{
   Parent::onGhostRemove();
   mLoadout.deactivateAllModules();
   updateModuleSounds();
}


F32 Ship::getHealth() const
{
   return mHealth;
}


S32 Ship::getEnergy() const
{
   return mEnergy;
}


void Ship::setActualPos(const Point &p, bool warp)
{
   Parent::setActualPos(p);
   Parent::setRenderPos(p);

   if(warp)
      setMaskBits(PositionMask | WarpPositionMask | TeleportMask);
   else
      setMaskBits(PositionMask);
}


// Process a move.  This will advance the position of the ship, as well as adjust its velocity and angle.
F32 Ship::processMove(U32 stateIndex)
{
   static const F32 NORMAL_ACCEL_FACT = 1.0f;
   static const F32 ARMOR_ACCEL_FACT  = 0.35f;

   static const F32 NORMAL_SPEED_FACT = 1.0f;
   static const F32 ARMOR_SPEED_FACT  = 1.0f;

   mLastProcessStateAngle = getAngle(stateIndex);
   setAngle(stateIndex, mCurrentMove.angle);

   // Nothing to do when ship is not moving, Continue to check for SpeedZones
   if(mCurrentMove.x == 0 && mCurrentMove.y == 0 && getVel(stateIndex) == Point(0,0))
   {
      if(!checkForSpeedzones(stateIndex))
         return 0;
   }

   F32 maxVel = (mLoadout.isModulePrimaryActive(ModuleBoost) ? BoostMaxVelocity : MaxVelocity) *
                (hasModule(ModuleArmor) ? ARMOR_SPEED_FACT : NORMAL_SPEED_FACT);

   F32 time = mCurrentMove.time * 0.001f;

   static Point requestVel, accel;     // Reusable containers

   // This is what the client requested -- basically requestVel.len() will range from 0 to 1; any higher will be clipped
   requestVel.set(mCurrentMove.x, mCurrentMove.y);
   requestVel *= maxVel;

   static const S32 MAX_CONTROLLABLE_SPEED = 1000;    

   // If you are going too fast (i.e. > MAX_CONTROLLABLE_SPEED), you cannot move, and will automatically 
   // "hit the brakes" by requesting a speed of 0
   if(getVel(stateIndex).lenSquared() > sq(MAX_CONTROLLABLE_SPEED))
      requestVel.set(0,0);


   // Limit requestVel to maxVel (but can be lower)
   if(requestVel.lenSquared() > sq(maxVel))
      requestVel.normalize(maxVel);

   // a  = requested vel - current vel
   accel = requestVel    - getVel(stateIndex);

   // Increase acceleration when turbo-boost is active, reduce it when armor is present
   F32 maxAccel = Acceleration * time *                                                // Standard accel, modified by:
                  (mLoadout.isModulePrimaryActive(ModuleBoost) ? BoostAccelFact : 1) * // Boost
                  (hasModule(ModuleArmor) ? ARMOR_ACCEL_FACT : NORMAL_ACCEL_FACT) *    // Armor
                  getGame()->getShipAccelModificationFactor(this);                     // Slip zones

   // If you are requesting a lower accel than the max, you get it instantly... else you are limited to max
   if(accel.lenSquared() <= sq(maxAccel))
      setVel(stateIndex, requestVel);
   else
   {
      accel.normalize(maxAccel);
      setVel(stateIndex, getVel(stateIndex) + accel);
   }

   return move(time, stateIndex, false);
}


// Returns the zone in question if this ship is in any zone.
// If ship is in multiple zones, an aribtrary one will be returned, and the level designer will be flogged.
BfObject *Ship::isInAnyZone() const
{
   findObjectsUnderShip((TestFunc)isZoneType);  // Fills fillVector
   return doIsInZone(fillVector);
}


// Returns the zone in question if this ship is in a zone of type zoneType.
// If ship is in multiple zones of type zoneTypeNumber, an aribtrary one will be returned, and the level designer will be flogged.
BfObject *Ship::isInZone(U8 zoneTypeNumber) const
{
   findObjectsUnderShip(zoneTypeNumber);        // Fills fillVector
   return doIsInZone(fillVector);
}


// Private helper for isInZone() and isInAnyZone() -- these fill fillVector, and we operate on it below
BfObject *Ship::doIsInZone(const Vector<DatabaseObject *> &objects) const
{
   if(objects.size() == 0)  // Ship isn't in extent of any objectType objects, can bail here
      return NULL;

   // Extents overlap...  now check for actual overlap

   for(S32 i = 0; i < objects.size(); i++)
   {
      BfObject *zone = static_cast<BfObject *>(objects[i]);

      // Get points that define the zone boundaries
      const Vector<Point> *polyPoints = zone->getCollisionPoly();

      if( polyPoints->size() != 0 && polygonContainsPoint(polyPoints->address(), polyPoints->size(), getActualPos()) )
         return zone;
   }
   return NULL;
}


// Returns the object in question if this ship is on an object of type objectType
DatabaseObject *Ship::isOnObject(U8 objectType, U32 stateIndex)
{
   findObjectsUnderShip(objectType);

   if(fillVector.size() == 0)  // Ship isn't in extent of any objectType objects, can bail here
      return NULL;

   // Return first actually overlapping object on our candidate list
   for(S32 i = 0; i < fillVector.size(); i++)
      if(isOnObject(static_cast<BfObject *>(fillVector[i]), ActualState))
         return fillVector[i];

   return NULL;
}


// Given an object, see if the ship is sitting on it (useful for figuring out if ship is on top of a regenerated repair item, z.B.)
bool Ship::isOnObject(BfObject *object, U32 stateIndex)
{
   Point center;
   float radius;
   static Vector<Point> polyPoints;
   polyPoints.clear();
   Rect rect;

   // Ships don't have collisionPolys, so this first check is utterly unneeded unless we change that
   /*if(getCollisionPoly(polyPoints))
      return object->collisionPolyPointIntersect(polyPoints);
   else */
   if(getCollisionCircle(ActualState, center, radius))
      return object->collisionPolyPointIntersect(center, radius);
   else
      return false;
}


F32 Ship::getSensorZoomFraction() const
{
   return 1 - mSensorEquipZoomTimer.getFraction();
}


// Returns vector based on direction ship is facing
Point Ship::getAimVector() const
{
   return Point(cos(getActualAngle()), sin(getActualAngle()));
}


void Ship::selectNextWeapon()
{
   setActiveWeapon(mLoadout.getActiveWeaponIndex() + 1);
}


void Ship::selectPrevWeapon()
{
   setActiveWeapon(mLoadout.getActiveWeaponIndex() - 1);
}


// I *think* this runs only on the server, and from tests
void Ship::selectWeapon(S32 weaponIdx)
{
   while(weaponIdx < 0)
      weaponIdx += ShipWeaponCount;

   setActiveWeapon(weaponIdx % ShipWeaponCount);      // Advance index to selected weapon
}


void Ship::processWeaponFire()
{
   // Can only fire when mFireTimer <= 0
   if(mFireTimer > 0)
      mFireTimer -= S32(mCurrentMove.time);

   if(!mCurrentMove.fire && mFireTimer < 0)
      mFireTimer = 0;

   mWeaponFireDecloakTimer.update(mCurrentMove.time);

   WeaponType curWeapon = mLoadout.getActiveWeapon();

   GameType *gameType = getGame()->getGameType();

   //             player is firing            player's ship is still largely functional
   if(gameType && mCurrentMove.fire && (!getClientInfo() || !getClientInfo()->isShipSystemsDisabled()))
   {
      // In a while loop, to catch up the firing rate for low Frame Per Second
      while(mFireTimer <= 0 && mEnergy >= WeaponInfo::getWeaponInfo(curWeapon).minEnergy)
      {
         mEnergy -= WeaponInfo::getWeaponInfo(curWeapon).drainEnergy;      // Drain energy
#ifdef SHOW_SERVER_SITUATION
         // Make a noise when the client thinks we've shot -- ideally, there should be one boop per shot, delayed by about half
         // of whatever /lag is set to.
         if(isClient())
            UserInterface::playBoop();
#endif
         mWeaponFireDecloakTimer.reset(WeaponFireDecloakTime);          // Uncloak ship

         if(getClientInfo())
            getClientInfo()->getStatistics()->countShot(curWeapon);

         if(isServer())  
         {
            Point dir = getAimVector();

            // TODO: To fix skip fire effect on jittery server, need to replace the 0 with... something...
            GameWeapon::createWeaponProjectiles(curWeapon, dir, getActualPos(), getActualVel(), 0, CollisionRadius - 2, this);
         }

         mFireTimer += S32(WeaponInfo::getWeaponInfo(curWeapon).fireDelay);

         // If we've fired, Spawn Shield turns off
         if(mSpawnShield.getCurrent() != 0)
         {
            setMaskBits(SpawnShieldMask);
            mSpawnShield.clear();
         }
      }
   }
}

// Compute the delta between our current render position and the server position after 
// client-side prediction has been run
void Ship::controlMoveReplayComplete()
{
   Point delta = getActualPos() - getRenderPos();
   F32 deltaLenSq = delta.lenSquared();

   // If the delta is either very small, or greater than the max interpolation threshold, 
   // just warp to the new position
   if(deltaLenSq <= sq(0.5) || deltaLenSq > sq(MaxControlObjectInterpDistance))
   {
#ifndef ZAP_DEDICATED
      // If it's a large delta, get rid of the movement trails
      if(deltaLenSq > sq(MaxControlObjectInterpDistance))
         for(S32 i = 0; i < TrailCount; i++)
            mTrail[i].reset();
#endif

      copyMoveState(ActualState, RenderState);
      mInterpolating = false;
   }
   else
      mInterpolating = true;
}


void Ship::idle(IdleCallPath path)
{
   // Don't idle exploded ships
   if(hasExploded)
      return;

   if(path == ServerProcessingUpdatesFromClient && getClientInfo())
      getClientInfo()->getStatistics()->mPlayTime += mCurrentMove.time;

   Parent::idle(path);

   if(path == ServerIdleMainLoop && controllingClientIsValid())
   {
      // If this is a controlled object in the server's main
      // idle loop, process the render state forward -- this
      // is what projectiles will collide against.  This allows
      // clients to properly lead other clients, instead of
      // piecewise stepping only when packets arrive from the client.
      processMove(RenderState);

      if(getActualVel().lenSquared() != 0 || getActualPos() != getRenderPos())
         setMaskBits(PositionMask);

      mSendSpawnEffectTimer.update(mCurrentMove.time);
   }
   else   // <=== do we really want this loop running if    path == ServerIdleMainLoop && NOT controllingClientIsValid() ??
   {
      //TNLAssert(path != ServerIdleMainLoop, "Does this ever happen?  If so, document and remove assert...");
      // Happens during testing...

      // If we're the client and are out-of-touch with the server, don't move the ship... 
      // moving won't actually hurt, but this seems somehow better
      if((path == ClientIdlingLocalShip || path == ClientIdlingNotLocalShip) && 
               getActualVel().lenSquared() != 0 && 
               getControllingClient() && getControllingClient()->lostContact())
         return;

      // Apply impulse vector and reset it
      setActualVel(getActualVel() + mImpulseVector);
      mImpulseVector.set(0,0);

      // For all other cases, advance the actual state of the object with the current move.
      // Dist is the distance the ship moved this tick.
      F32 dist = processMove(ActualState);

      if(path == ServerProcessingUpdatesFromClient || path == ClientIdlingLocalShip)
         getClientInfo()->getStatistics()->accumulateDistance(dist);

      if(path == ServerProcessingUpdatesFromClient ||
         path == ClientIdlingLocalShip             ||
         path == ClientReplayingPendingMoves)
      {
         // For different optimizer settings and different platforms the floating point calculations may come out slightly
         // differently in the lowest mantissa bits.  So normalize after each update the position and velocity, so that
         // the control state update will not differ from client to server.
         static const F32 ShipVarNormalizeMultiplier = 128;
         static const F32 ShipVarNormalizeFraction = 0.0078125; // 1/ShipVarNormalizeMultiplier

         static Point p;
         
         p = getActualPos();
         p.scaleFloorDiv(ShipVarNormalizeMultiplier, ShipVarNormalizeFraction);
         Parent::setActualPos(p);

         p = getActualVel();
         p.scaleFloorDiv(ShipVarNormalizeMultiplier, ShipVarNormalizeFraction);
         Parent::setActualVel(p);
      }

      if(path == ServerIdleMainLoop || path == ServerProcessingUpdatesFromClient)
      {
         // Update the render state on the server to match the actual updated state, and mark the object as having changed 
         // Position state.  An optimization here would check the before and after positions so as to not update unmoving ships.
         if(getRenderAngle() != getActualAngle() || getRenderPos() != getActualPos() || getRenderVel() != getActualVel())
            setMaskBits(PositionMask);

         copyMoveState(ActualState, RenderState);
      }
      else if(path == ClientIdlingLocalShip || path == ClientIdlingNotLocalShip)
      {
         // On the client, update the interpolation of this object unless we are replaying control moves
         mInterpolating = (getActualVel().lenSquared() < MoveObject::InterpMaxVelocity*MoveObject::InterpMaxVelocity);
         updateInterpolation();
      }

      // Don't want the replay to make timer count down much faster while having high ping
      if(path != ClientReplayingPendingMoves) 
      {
         mSensorEquipZoomTimer.update(mCurrentMove.time);
         mCloakTimer.update(mCurrentMove.time);

         // Update spawn shield unless we move the ship - then it turns off .. server only
         if(mSpawnShield.getCurrent() != 0)
         {
            if(path == ServerProcessingUpdatesFromClient && (mCurrentMove.x != 0 || mCurrentMove.y != 0))
            {
               mSpawnShield.clear();
               setMaskBits(SpawnShieldMask);  // Tell clients spawn shield turned off due to moving
            }
            else
               mSpawnShield.update(mCurrentMove.time);
         }
      }
   }

   if(path == ServerIdleMainLoop)
      checkForZones();        // See if ship entered or left any zones

   // Update the object in the game's extents database
   updateExtentInDatabase();

   // If this is a move executing on the server and it's different from the last move,
   // then mark the move to be updated to the ghosts
   if(path == ServerProcessingUpdatesFromClient && !mCurrentMove.isEqualMove(&mPrevMove))
      setMaskBits(MoveMask);

   mPrevMove = mCurrentMove;

   mRepairTargets.clear();

   // Find any repair targets for rendering repair rays -- on other paths, this will be done in processModules
   if(path == ClientIdlingNotLocalShip)
      if(mLoadout.isModulePrimaryActive(ModuleRepair))
         findRepairTargets();       

   // Process weapons and modules on controlled objects; handles all the energy reductions as well
   if(path == ServerProcessingUpdatesFromClient || path == ClientIdlingLocalShip || path == ClientReplayingPendingMoves)
   {
      mFastRechargeTimer.update(mCurrentMove.time);
      mFastRecharging = mFastRechargeTimer.getCurrent() == 0;

      processWeaponFire();
      processModules();
      rechargeEnergy();
   }

   if(path == ServerProcessingUpdatesFromClient)
      repairTargets();
      

   if(path == ClientIdlingLocalShip || path == ClientIdlingNotLocalShip)
   {
      mWarpInTimer.update(mCurrentMove.time);

      // Emit some particles, trail sections and update the turbo noise
      emitMovementSparks();
      updateTrails();
      updateModuleSounds();                       
   }
}


// Get list of zones ship is currently in
Vector<DatabaseObject *> *Ship::getCurrZoneList()
{
   return mZones1IsCurrent ? &mZones1 : &mZones2;
}


// Get list of zones ship was in last tick
Vector<DatabaseObject *> *Ship::getPrevZoneList()
{
   return mZones1IsCurrent ? &mZones2 : &mZones1;
}


bool Ship::checkForSpeedzones(U32 stateIndex)
{
   SpeedZone *speedZone = static_cast<SpeedZone *>(isOnObject(SpeedZoneTypeNumber, stateIndex));

   if(speedZone && speedZone->collide(this))
      return speedZone->collided(this, stateIndex);
   return false;
}

// See if ship entered or left any zones
// Server only
void Ship::checkForZones()
{
   Vector<DatabaseObject *> *currZoneList = getCurrZoneList();
   Vector<DatabaseObject *> *prevZoneList = getPrevZoneList();

   getZonesShipIsIn(currZoneList);     // Fill currZoneList with a list of all zones ship is currently in

   // Now compare currZoneList with prevZoneList to figure out if ship entered or exited any zones
   for(S32 i = 0; i < currZoneList->size(); i++)
      if(!prevZoneList->contains(currZoneList->get(i)))
         EventManager::get()->fireEvent(EventManager::ShipEnteredZoneEvent, this, static_cast<Zone *>(currZoneList->get(i)));

   for(S32 i = 0; i < prevZoneList->size(); i++)
      if(!currZoneList->contains(prevZoneList->get(i)))
         EventManager::get()->fireEvent(EventManager::ShipLeftZoneEvent, this, static_cast<Zone *>(prevZoneList->get(i)));
}


// Fill zoneList with a list of all zones that the ship is currently in
// Server only
void Ship::getZonesShipIsIn(Vector<DatabaseObject *> *zoneList)
{
   // Use this boolean as a cheap way of making the current zone list be the previous out without copying
   mZones1IsCurrent = !mZones1IsCurrent;     

   zoneList->clear();

   Rect rect(getActualPos(), getActualPos());      // Center of ship

   fillVector.clear();                             
   findObjects((TestFunc)isZoneType, fillVector, rect);  // Find all zones the ship might be in

   // Extents overlap...  now check for actual overlap
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      // Get points that define the zone boundaries
      const Vector<Point> *polyPoints = fillVector[i]->getCollisionPoly();

      if(polygonContainsPoint(polyPoints->address(), polyPoints->size(), getActualPos()))
         zoneList->push_back(fillVector[i]);
   }
}


static Vector<DatabaseObject *> foundObjects;      // Reusable container

void Ship::findRepairTargets()
{
   // We use the render position in findRepairTargets so that
   // ships that are moving can repair each other (server) and
   // so that ships don't render funny repair lines to interpolating
   // ships (client)

   Point pos = getRenderPos();
   Rect r(pos, (RepairRadius + CollisionRadius));
   
   foundObjects.clear();
   findObjects((TestFunc)isWithHealthType, foundObjects, r);   // All isWithHealthType objects are items

   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      BfObject *item = static_cast<BfObject*>(foundObjects[i]);

      // Don't repair dead or fully healed objects...
      if(item->isDestroyed() || item->getHealth() >= 1)
         continue;

      // ...or ones not on our team or neutral
      if(item->getTeam() != TEAM_NEUTRAL && item->getTeam() != getTeam())
         continue;

      // ...or same team in non-team game, except self
      if(!getGame()->getGameType()->isTeamGame() && item->getTeam() == getTeam() && item != this)
         continue;

      // Find the radius of the repairable.  Handle teleporter special case
      F32 itemRadius = 0;
      if(item->getObjectTypeNumber() == TeleporterTypeNumber)
         itemRadius = Teleporter::TELEPORTER_RADIUS;
      else
      {
         TNLAssert(dynamic_cast<Item*>(item), "Expected to find an item!");
         itemRadius = static_cast<Item*>(item)->getRadius();
      }

      // Only repair items within a circle around the ship since we did an object search with a rectangle
      if((item->getPos() - pos).lenSquared() > sq(RepairRadius + CollisionRadius + itemRadius))
         continue;

      // In case of CoreItem, don't repair if no repair locations are returned
      if(item->getRepairLocations(pos).size() == 0)
         continue;

      mRepairTargets.push_back(item);
   }
}


// Repairs ALL repair targets found above; server only
void Ship::repairTargets()
{
   if(mRepairTargets.size() == 0)
      return;

   F32 totalRepair = RepairHundredthsPerSecond * 0.01f * mCurrentMove.time * 0.001f;

   // totalRepair /= mRepairTargets.size();      // Divide repair amongst repair targets... makes repair too weak

   DamageInfo di;
   di.damageAmount = -totalRepair;
   di.damagingObject = this;
   di.damageType = DamageTypePoint;

   for(S32 i = 0; i < mRepairTargets.size(); i++)
      mRepairTargets[i]->damageObject(&di);
}


void Ship::processModules()
{
   // Update some timers
   for(S32 i = 0; i < ModuleCount; i++)
      mModuleSecondaryTimer[i].update(mCurrentMove.time);

   mSpyBugPlacementTimer.update(mCurrentMove.time);

   // Save the previous module primary/secondary component states; reset them - to be set later
   bool wasModulePrimaryActive[ModuleCount];
   bool wasModuleSecondaryActive[ModuleCount];

   for(S32 i = 0; i < ModuleCount; i++)
   {
      wasModulePrimaryActive[i]   = mLoadout.isModulePrimaryActive(ShipModule(i));
      wasModuleSecondaryActive[i] = mLoadout.isModuleSecondaryActive(ShipModule(i));
   }

   mLoadout.deactivateAllModules();

   // Go through our loaded modules and see if they are currently turned on
   // Are these checked on the server side?
   for(S32 i = 0; i < ShipModuleCount; i++)   
   {
      // If you have passive module, it's always active, no restrictions, but is off for energy consumption purposes
      if(ModuleInfo::getModuleInfo(mLoadout.getModule(i))->getPrimaryUseType() == ModulePrimaryUsePassive)
         mLoadout.setModuleIndexPrimary(i, true);         // needs to be true to allow stats counting

      // Set loaded module states to 'on' if detected as so, unless modules are disabled or we need to cooldown
      if(!mCooldownNeeded && (!getClientInfo() || (getClientInfo() && !getClientInfo()->isShipSystemsDisabled())))
      {
         if(mCurrentMove.modulePrimary[i])
            mLoadout.setModuleIndexPrimary(i, true);

         if(mCurrentMove.moduleSecondary[i])
            mLoadout.setModuleIndexSecondary(i, true);
      }
   }

   // No Turbo or Pulse if we're not moving
   if(mLoadout.isModulePrimaryActive(ModuleBoost) && mCurrentMove.x == 0 && mCurrentMove.y == 0)
   {
      mLoadout.setModulePrimary(ModuleBoost, false);
      mLoadout.setModuleSecondary(ModuleBoost, false);
   }

   if(mLoadout.isModulePrimaryActive(ModuleRepair))
   {
      findRepairTargets();
      // If there are no repair targets, turn off repair
      if(mRepairTargets.size() == 0)
         mLoadout.setModulePrimary(ModuleRepair, false);
   }

   // No cloak with nearby sensored people
   if(mLoadout.isModulePrimaryActive(ModuleCloak))
   {
      if(mWeaponFireDecloakTimer.getCurrent() != 0)
         mLoadout.setModulePrimary(ModuleCloak, false);
      //else
      //{
      //   Rect cloakCheck(getActualPos(), getActualPos());
      //   cloakCheck.expand(Point(CloakCheckRadius, CloakCheckRadius));

      //   fillVector.clear();
      //   findObjects(ShipType | RobotType, fillVector, cloakCheck);

      //   if(fillVector.size() > 0)
      //   {
      //      for(S32 i=0; i<fillVector.size(); i++)
      //      {
      //         Ship *s = dynamic_cast<Ship *>(fillVector[i]);

      //         if(!s) continue;

      //         if(s->getTeam() != getTeam() && s->isModuleActive(ModuleSensor))
      //         {
      //            mModuleActive[ModuleCloak] = false;
      //            break;
      //         }
      //      }
      //   }
      //}
   }

   U32 timeInMilliSeconds = mCurrentMove.time;

   // Modules with active primary components
   S32 primaryActivationCount = 0;

   // Update things based on available energy...
   for(S32 i = 0; i < ModuleCount; i++)
   {
      if(mLoadout.isModulePrimaryActive(ShipModule(i)))
      {
         const ModuleInfo *moduleInfo = ModuleInfo::getModuleInfo((ShipModule) i);
         S32 energyUsed = moduleInfo->getPrimaryEnergyDrain() * timeInMilliSeconds;
         mEnergy -= energyUsed;

         // Exclude passive modules
         if(energyUsed != 0)
            primaryActivationCount += 1;

         if(getClientInfo())
            getClientInfo()->getStatistics()->addModuleUsed(ShipModule(i), mCurrentMove.time);


         // Sensor module needs to place a spybug
         if(i == ModuleSensor &&  
               mSpyBugPlacementTimer.getCurrent() == 0 &&        // Prevent placement too fast
               mEnergy > moduleInfo->getPrimaryPerUseCost())     // Have enough energy
         {

            if(isClient())
            {
               mEnergy -= moduleInfo->getPrimaryPerUseCost();
               mSpyBugPlacementTimer.reset();
            }
            else
               deploySpybug();

         }
      }

      // Fire the module secondary component if it is active and the delay timer has run out
      if(mLoadout.isModuleSecondaryActive(ShipModule(i)) && mModuleSecondaryTimer[i].getCurrent() == 0)
      {
         S32 energyCost = gModuleInfo[i].getSecondaryPerUseCost();
         // If we have enough energy, fire the module
         if(mEnergy >= energyCost)
         {
            // Reduce energy
            mEnergy -= energyCost;

            // Pulse uses up all energy and applies an impulse vector
            if(i == ModuleBoost)
            {
               // The impulse should be in the same direction you're already going
               mImpulseVector = getActualVel();

               // Change to Pulse speed based on current energy
               mImpulseVector.normalize((((F32)mEnergy/(F32)EnergyMax) * (PulseMaxVelocity - PulseMinVelocity)) + PulseMinVelocity);

               mEnergy = 0;
            }
         }
      }
   }

   // Only toggle cooldown if no primary components are active
   if(primaryActivationCount == 0)
      mCooldownNeeded = mEnergy <= EnergyCooldownThreshold;

   // Offset recharge bonus when using modules in a friendly zone
   //if (primaryActivationCount > 0)
   //{
   //   // This assumes the neutral and friendly bonuses are equal
   //   BfObject *object = isInZone(LoadoutZoneTypeNumber);
   //   S32 currentZoneTeam = object ? object->getTeam() : NO_TEAM;
   //   if (currentZoneTeam == TEAM_NEUTRAL || currentZoneTeam == getTeam())
   //      mEnergy -= EnergyRechargeRateInFriendlyLoadoutZoneModifier * timeInMilliSeconds;            
   //}

   // Reduce total energy consumption when more than one module is used
   if(primaryActivationCount > 1)
      mEnergy += EnergyRechargeRate * timeInMilliSeconds;

   // Do logic triggered when module primary component state changes
   for(S32 i = 0; i < ModuleCount;i++)
   {
      if(mLoadout.isModulePrimaryActive(ShipModule(i)) != wasModulePrimaryActive[i])
      {
         if(i == ModuleCloak)
            mCloakTimer.reset(CloakFadeTime - mCloakTimer.getCurrent(), CloakFadeTime);

         setMaskBits(ModulePrimaryMask);
      }
   }

   // Do logic triggered when module secondary component state changes
   for(U32 i = 0; i < ModuleCount;i++)
   {
      if(mLoadout.isModuleSecondaryActive(ShipModule(i)) != wasModuleSecondaryActive[i])
      {
         // If current state is active, reset the delay timer if it has run out
         if(mLoadout.isModuleSecondaryActive(ShipModule(i)))
            if(mModuleSecondaryTimer[i].getCurrent() == 0)
               mModuleSecondaryTimer[i].reset();

         setMaskBits(ModuleSecondaryMask);
      }
   }
}


// Runs on server only, at the request of c2sDeploySpybug
void Ship::deploySpybug()
{
   const ModuleInfo *moduleInfo = ModuleInfo::getModuleInfo(ModuleSensor);     // Spybug is attached to this module

   S32 deploymentEnergy = moduleInfo->getPrimaryPerUseCost();

   // Double check the requirements... we don't want no monkey business
   if(mEnergy < deploymentEnergy || mSpyBugPlacementTimer.getCurrent() > 0)
   {
      // Problem! -- send message to client to recredit their energy.  This is a very rare circumstance.
      GameConnection *cc = getControllingClient();

      if(cc)
         cc->s2cCreditEnergy(deploymentEnergy);

      return;
   }

   mEnergy -= deploymentEnergy;                             
   mSpyBugPlacementTimer.reset();

   Point direction = getAimVector();
   GameWeapon::createWeaponProjectiles(WeaponSpyBug, direction, getActualPos(),
                                       getActualVel(), 0, CollisionRadius - 2, this);

   if(getClientInfo())
      getClientInfo()->getStatistics()->countShot(WeaponSpyBug);
}


// Energy can be negative!
void Ship::creditEnergy(S32 deltaEnergy)
{
   mEnergy = MAX(0, MIN(EnergyMax, mEnergy + deltaEnergy));
}


// Runs on client and server
void Ship::rechargeEnergy()
{
   U32 timeInMilliSeconds = mCurrentMove.time;

   // Energy will not recharge if spawn shield is up
   if(mSpawnShield.getCurrent() != 0)     
      mFastRechargeTimer.reset();    // Fast recharge timer doesn't really get going until after spawn shield is down
   else
   {
      // Base recharge rate
      mEnergy += EnergyRechargeRate * timeInMilliSeconds;

      //// Apply energy recharge modifier for the zone the player is in
      //BfObject *object = isInZone(LoadoutZoneTypeNumber);
      //S32 currentLoadoutZoneTeam = object ? object->getTeam() : NO_TEAM;

      //if(currentLoadoutZoneTeam == TEAM_HOSTILE)
      //   mEnergy += EnergyRechargeRateInHostileLoadoutZoneModifier * timeInMilliSeconds;

      //else if(currentLoadoutZoneTeam == TEAM_NEUTRAL)
      //   mEnergy += EnergyRechargeRateInNeutralLoadoutZoneModifier * timeInMilliSeconds;

      //else if(currentLoadoutZoneTeam == getTeam())
      //   mEnergy += EnergyRechargeRateInFriendlyLoadoutZoneModifier * timeInMilliSeconds;

      //else if(currentLoadoutZoneTeam != NO_TEAM)
      //   mEnergy += EnergyRechargeRateInEnemyLoadoutZoneModifier * timeInMilliSeconds;

      // Recharge energy very fast if we're completely idle for a given amount of time
      if(mCurrentMove.x != 0 || mCurrentMove.y != 0 || mCurrentMove.fire || mCurrentMove.isAnyModActive())
      {
         resetFastRecharge();
      }

      if(mFastRecharging)
         mEnergy += EnergyRechargeRateIdleRechargeCycle * timeInMilliSeconds;
   }

   // Movement penalty
   //if (mCurrentMove.x != 0 || mCurrentMove.y != 0)
   //   mEnergy += EnergyRechargeRateMovementModifier * timeInMilliSeconds;

   // Handle energy falling below 0
   if(mEnergy <= 0)
   {
      mEnergy = 0;
      mCooldownNeeded = true;
   }

   // Cap energy at max
   else if(mEnergy >= EnergyMax)
      mEnergy = EnergyMax;
}


void Ship::resetFastRecharge()
{
   mFastRechargeTimer.reset();
   mFastRecharging = false;
}


void Ship::damageObject(DamageInfo *theInfo)
{
   TNLAssert(hasExploded || mHealth > 0, "One must be true!  If this never fires, remove mHealth == 0 from if below.");  // Added 22-Sep-2013 by Watusimoto
   if(mHealth == 0 || hasExploded)  // Stop multi-kill problem;  might stop robots from becoming invincible
      return; 

   bool hasArmor = hasModule(ModuleArmor);

   // Deal with grenades and other explody things, even if they cause no damage
   if(theInfo->damageType == DamageTypeArea)
   {
      static const F32 ARMOR_IMPULSE_ABSORBTION_FACTOR = 0.25f;

      // Armor pads impulses.  Here comes the tank!
      if(hasArmor)
         mImpulseVector += (theInfo->impulseVector * ARMOR_IMPULSE_ABSORBTION_FACTOR);
      else
         mImpulseVector += theInfo->impulseVector;
   }

   if(theInfo->damageAmount == 0)
      return;

   F32 damageAmount = theInfo->damageAmount;

   if(theInfo->damageAmount > 0)
   {
      if(!getGame()->objectCanDamageObject(theInfo->damagingObject, this))
         return;

      // Factor in shields
      if(mLoadout.isModulePrimaryActive(ModuleShield)) // && mEnergy >= EnergyShieldHitDrain)     // Commented code will cause
      {                                                                                           // shields to drain when they
         //mEnergy -= EnergyShieldHitDrain;                                                       // have been hit.
         return;
      }

      // No damage done if spawn shield is active
      if(mSpawnShield.getCurrent() != 0)
         return;

      // Having armor reduces damage
      if(hasArmor)
      {
         static const F32 ArmorDamageReductionFactor = 0.4f;   // Having armor reduces damage

         damageAmount *= ArmorDamageReductionFactor;           // Any other damage, including asteroids
      }
   }

   ClientInfo *damagerOwner = theInfo->damagingObject ? theInfo->damagingObject->getOwner() : NULL;
   ClientInfo *victimOwner = this->getOwner();

   bool damageWasSelfInflicted = victimOwner && damagerOwner == victimOwner;

   // Healing things do negative damage, thus adding to health
   mHealth -= damageAmount * (damageWasSelfInflicted ? theInfo->damageSelfMultiplier : 1);
   setMaskBits(HealthMask);

   if(mHealth <= 0)
   {
      mHealth = 0;
      killAndScore(theInfo);
   }
   else if(mHealth > 1)
      mHealth = 1;


   // Do some stats related work
   if(getClientInfo() && theInfo->damagingObject) // getClientInfo() could be NULL <== could it?... damagingObject could be NULL in testing
   {
      Projectile *projectile = NULL;
      if(theInfo->damagingObject->getObjectTypeNumber() == BulletTypeNumber)
         projectile = static_cast<Projectile *>(theInfo->damagingObject);

      if(projectile)
         getClientInfo()->getStatistics()->countHitBy(projectile->mWeaponType);
 
      else if(mHealth == 0 && theInfo->damagingObject->getObjectTypeNumber() == AsteroidTypeNumber)
         getClientInfo()->getStatistics()->mCrashedIntoAsteroid++;
   }
}


// Returns true if ship represents local player -- client only
bool Ship::isLocalPlayerShip(Game *game) const
{
   return getClientInfo() == game->getLocalRemoteClientInfo();
}


// Runs when ship spawns -- runs on client and server
// Gets run on client every time ship spawns, gets run on server once per level
void Ship::onAddedToGame(Game *game)
{
   Parent::onAddedToGame(game);
#ifndef ZAP_DEDICATED
   if(isClient())       // Client
   {
      if(isLocalPlayerShip(game))
         static_cast<ClientGame *>(game)->undelaySpawn();    // Server tells us we're undelayed by spawning our ship
   }

   else                 // Server
#endif
   {
      mSendSpawnEffectTimer.reset();
      EventManager::get()->fireEvent(EventManager::ShipSpawnedEvent, this);
   }
}


void Ship::updateModuleSounds()
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = static_cast<ClientGame *>(getGame());
   clientGame->updateModuleSounds(getRenderPos(), getRenderVel(), mLoadout);
#endif
}


static U32 MaxFireDelay = 0;

// static method, only run during init on both client and server
void Ship::computeMaxFireDelay()
{
   for(S32 i = 0; i < WeaponCount; i++)
      if(WeaponInfo::getWeaponInfo(WeaponType(i)).fireDelay > MaxFireDelay)
         MaxFireDelay = WeaponInfo::getWeaponInfo(WeaponType(i)).fireDelay;
}


const U32 negativeFireDelay = 123;  // how far into negative we are allowed to send.
// MaxFireDelay + negativeFireDelay, 900 + 123 = 1023, so writeRangedU32 are sending full range of 10 bits of information.

// Only used on client for prediction in replay moves
void Ship::setState(ControlObjectData *state)
{
   mEnergy = state->mEnergy;
   mFireTimer = state->mFireTimer;
   mFastRechargeTimer.reset(state->mFastRechargeTimer, mFastRechargeTimer.getPeriod());
   mSpyBugPlacementTimer.reset(state->mSpyBugPlacementTimer, mSpyBugPlacementTimer.getPeriod());
   mCooldownNeeded = state->mCooldownNeeded;
   mFastRecharging = state->mFastRecharging;
}
void Ship::getState(ControlObjectData *state) const
{
   state->mEnergy = mEnergy;
   state->mFireTimer = mFireTimer;
   state->mFastRechargeTimer = mFastRechargeTimer.getCurrent();
   state->mSpyBugPlacementTimer = mSpyBugPlacementTimer.getCurrent();
   state->mCooldownNeeded = mCooldownNeeded;
   state->mFastRecharging = mFastRecharging;
}

void Ship::writeControlState(BitStream *stream)
{
   stream->write(getActualPos().x);
   stream->write(getActualPos().y);
   stream->write(getActualVel().x);
   stream->write(getActualVel().y);

   //stream->writeRangedU32(mEnergy, 0, EnergyMax);
   //stream->writeFlag(mFastRecharging);
   stream->writeFlag(mCooldownNeeded);

   stream->writeRangedU32(mLoadout.getActiveWeaponIndex(), 0, ShipWeaponCount);
}


void Ship::readControlState(BitStream *stream)
{
   F32 x, y;

   stream->read(&x);
   stream->read(&y);
   Parent::setActualPos(Point(x, y));

   stream->read(&x);
   stream->read(&y);
   Parent::setActualVel(Point(x, y));

   //int serverReportedEnergy = stream->readRangedU32(0, EnergyMax);
   //bool rrrmFastRecharging = stream->readFlag();

   mCooldownNeeded = stream->readFlag();

   setActiveWeapon(stream->readRangedU32(0, ShipWeaponCount));
}


// Only used by tests
void Ship::setMove(const Move &move)
{
   mCurrentMove = move;
}


// Transmit ship status from server to client
// Any changes here need to be reflected in Ship::unpackUpdate
U32 Ship::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   GameConnection *gameConnection = (GameConnection *) connection;

   if(isInitialUpdate())      // This stuff gets sent only once per ship
   {
      // We'll need the name (or some other identifier) to match the ship to its clientInfo on the client side
      stream->writeStringTableEntry(getClientInfo() ? getClientInfo()->getName() : StringTableEntry());

      // Now write all the mounts:
      for(S32 i = 0; i < mMountedItems.size(); i++)
      {
         if(mMountedItems[i].isValid())
         {
            S32 index = connection->getGhostIndex(mMountedItems[i]);
            if(index != -1)      // This will skip any items that haven't yet been created on the client
            {
               stream->writeFlag(true);
               stream->writeInt(index, GhostConnection::GhostIdBitSize);
            }
         }
      }
      stream->writeFlag(false);
   }  // End initial update

   if(stream->writeFlag(updateMask & ChangeTeamMask))    // A player with admin can change robots teams
      writeThisTeam(stream);

   if(stream->writeFlag(updateMask & LoadoutMask))       // Loadout configuration
   {
      for(S32 i = 0; i < ShipModuleCount; i++)
         stream->writeEnum(mLoadout.getModule(i), ModuleCount);

      for(S32 i = 0; i < ShipWeaponCount; i++)
         stream->writeEnum(mLoadout.getWeapon(i), WeaponCount);
   }

   if(!stream->writeFlag(hasExploded))
   {
      // Note that RespawnMask is only used by Robots -- can this be refactored out of Ship.cpp?
      if(stream->writeFlag(updateMask & (RespawnMask | SpawnShieldMask)))
      {
         stream->writeFlag((updateMask & RespawnMask) != 0 && mSendSpawnEffectTimer.getCurrent() > 0);  // If true, ship will appear to spawn on client
         U32 sendNumber = (mSpawnShield.getCurrent() + (SpawnShieldTime / 16 / 2)) * 16 / SpawnShieldTime; // rounding
         if(stream->writeFlag(sendNumber != 0))
            stream->writeInt(sendNumber - 1, 4); 
      }

      if(stream->writeFlag(updateMask & HealthMask))     // Health
         stream->writeFloat(mHealth, 6);
   }

   stream->writeFlag((updateMask & WarpPositionMask) && updateMask != 0xFFFFFFFF);

   // Don't show warp effect when all mask flags are set, as happens when ship comes into scope
   stream->writeFlag((updateMask & TeleportMask) && !(updateMask & InitialMask));

   // Send position if this is our intial update or this ship does not represent the client that owns this ship
   bool shouldWritePosition = (updateMask & InitialMask) || gameConnection->getControlObject() != this;

   if(!shouldWritePosition)
   {
      // The number of writeFlags here *must* match the same number in the else statement
      stream->writeFlag(false);
      stream->writeFlag(false);
      stream->writeFlag(false);
      stream->writeFlag(false);
   }
   else     // Write mCurrentMove data...
   {
      if(stream->writeFlag(updateMask & PositionMask))         // <=== ONE
      {
         // Send position and speed  ==> use renderPos because that is the server's best guess of where a client-controlled
         //                              ship is at any given moment, even if the server hasn't heard from the client for
         //                              dseveral frames due to network delays.
         gameConnection->writeCompressedPoint(getRenderPos(), stream);
         writeCompressedVelocity(getRenderVel(), BoostMaxVelocity + 1, stream);
      }
      if(stream->writeFlag(updateMask & MoveMask))             // <=== TWO
         mCurrentMove.pack(stream, NULL, false);               // Send current move

      // If a module primary component is detected as on, pack it
      if(stream->writeFlag(updateMask & ModulePrimaryMask))    // <=== THREE
         for(S32 i = 0; i < ModuleCount; i++)                  // Send info about which modules are active (primary)
            stream->writeFlag(mLoadout.isModulePrimaryActive(ShipModule(i)));

      // If a module secondary component is detected as on, pack it
      if(stream->writeFlag(updateMask & ModuleSecondaryMask))  // <=== FOUR
         for(S32 i = 0; i < ModuleCount; i++)                  // Send info about which modules are active (secondary)
            stream->writeFlag(mLoadout.isModuleSecondaryActive(ShipModule(i)));
   }
   return 0;
}


// Any changes here need to be reflected in Ship::packUpdate
void Ship::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
#ifndef ZAP_DEDICATED
   bool positionChanged = false;    // True when position changes a little -- ship position will be interpolated
   bool shipwarped = false;         // True when position changes a lot -- ship will be warped to new location

   bool wasInitialUpdate = false;
   bool playSpawnEffect  = false;

   TNLAssert(isClient(), "We are expecting a ClientGame here!");

   if(isInitialUpdate())
   {
      wasInitialUpdate = true;

      // During the initial update, we need to assign a ClientInfo object to this ship.  We'll 
      // identifyt the proper ClientInfo by the player's name.
      //
      // Read the name and use it to find the clientInfo that should be waiting for us... hopefully
      StringTableEntry playerName;
      stream->readStringTableEntry(&playerName);

      // ClientInfo can be NULL if the ship has been added via level file, for example
      ClientInfo *clientInfo = getGame()->findClientInfo(playerName);
      
      mClientInfo = clientInfo;
      if(mClientInfo)
         mClientInfo->setShip(this);

      // Read mounted items:
      while(stream->readFlag())
      {
         S32 index = stream->readInt(GhostConnection::GhostIdBitSize);
         MountableItem *item = static_cast<MountableItem *>(connection->resolveGhost(index));
         if(item)                      // Could be NULL if server hasn't yet sent mounted item to us
            item->mountToShip(this);
      }

   }  // initial update

   if(stream->readFlag())        // Team changed (ChangeTeamMask)
      readThisTeam(stream);

   if(stream->readFlag())        // New loadout configuration (LoadoutMask)
   {
      bool hadSensorThen = false;
      bool hasSensorNow = false;
      bool hasEngineerModule = false;

      for(S32 i = 0; i < ShipModuleCount; i++)
      {
         // Check old loadout for sensor
         if(mLoadout.getModule(i) == ModuleSensor)
            hadSensorThen = true;

         // Update loadout
         mLoadout.setModule(i, (ShipModule) stream->readEnum(ModuleCount));

         // Check new loadout for sensor
         if(mLoadout.getModule(i) == ModuleSensor)
            hasSensorNow = true;

         else if(mLoadout.getModule(i) == ModuleEngineer)
            hasEngineerModule = true;
      }

      // Set sensor zoom timer if sensor carrying status has switched
      if(hadSensorThen != hasSensorNow && !isInitialUpdate())  // ! isInitialUpdate(), don't do zoom out effect of ship spawn
         mSensorEquipZoomTimer.reset();

      for(S32 i = 0; i < ShipWeaponCount; i++)
         mLoadout.setWeapon(i, (WeaponType) stream->readEnum(WeaponCount));

      // Notify the user interface (via the ClientGame object) about some things that may have changed.
      // Note that during testing, we might not have a game object, so we'll need to check for NULL here.
      if(getGame())
      {
         if(!hasEngineerModule)           // Can't engineer without this module
         {
            if(isLocalPlayerShip(getGame()))   // If this ship is ours, quit engineer menu (does nothing if menu is not shown)
               getGame()->quitEngineerHelper();
         }

         // Alert the UI that a new loadout has arrived (ClientGame->GameUI->LoadoutIndicator)
         if(isLocalPlayerShip(getGame()))
            static_cast<ClientGame *>(getGame())->newLoadoutHasArrived(mLoadout);

         // Looks like the user has successfully updated their loadout... we want to show a congratulations message.  However,
         // we don't want to do this if it has changed because the level reset, nor if it changed due to a respawn.  If the
         // level has a loadout zone, it means that the loadout was not changed due to a spawn event.
         if(!wasInitialUpdate && getGame()->levelHasLoadoutZone())
            getGame()->addInlineHelpItem(LoadoutFinishedItem);
      }
   }

   if(stream->readFlag())  // hasExploded
   {
      mHealth = 0;
      if(!hasExploded)
      {
         hasExploded = true;
         disableCollision();

         if(!wasInitialUpdate)
            emitExplosion();     // Boom!
      }

      if(isLocalPlayerShip(getGame()))   // If this ship is ours, quit engineer menu
         getGame()->quitEngineerHelper();
   }
   else
   {
      if(stream->readFlag())     // Respawn
      {
         if(hasExploded)
            enableCollision();
         hasExploded = false;
         playSpawnEffect = stream->readFlag();    // Prevent spawn effect every time the robot goes into scope
         shipwarped = true;

         if(stream->readFlag())
            mSpawnShield.reset((stream->readInt(4) + 1) * SpawnShieldTime / 16);
         else
            mSpawnShield.reset(0);
      }

      if(stream->readFlag())     // Health
         mHealth = stream->readFloat(6);
   }

   if(stream->readFlag())        // Ship made a large change in position
      shipwarped = true;

   if(stream->readFlag())        // Ship just teleported
   {
      shipwarped = true;
      mWarpInTimer.reset(WarpFadeInTime);    // Make ship all spinny (sfx, spiral bg are done by the teleporter itself)
   }

   if(stream->readFlag())     // UpdateMask
   {
      Point p;
      ((GameConnection *) connection)->readCompressedPoint(p, stream);
      Parent::setActualPos(p);

      readCompressedVelocity(p, BoostMaxVelocity + 1, stream);
      Parent::setActualVel(p);
      positionChanged = true;
   }

   if(stream->readFlag())     // MoveMask
   {
      mCurrentMove.initialize();             // Reset mCurrentMove to its factory defaults
      mCurrentMove.unpack(stream, false);    // And populate it with data from stream
   }

   if(stream->readFlag())     // ModulePrimaryMask
   {
      bool wasPrimaryActive[ModuleCount];
      for(S32 i = 0; i < ModuleCount; i++)
      {
         wasPrimaryActive[i] = mLoadout.isModulePrimaryActive(ShipModule(i));
         mLoadout.setModulePrimary(ShipModule(i), stream->readFlag());

         // Module activity toggled
         if(wasPrimaryActive[i] != mLoadout.isModulePrimaryActive(ShipModule(i)))
         {
            if(i == ModuleCloak)
               mCloakTimer.reset(CloakFadeTime - mCloakTimer.getCurrent(), CloakFadeTime);
         }
      }
   }

   if(stream->readFlag())     // ModuleSecondaryMask
      for(S32 i = 0; i < ModuleCount; i++)
         mLoadout.setModuleSecondary(ShipModule(i), stream->readFlag());

   setActualAngle(mCurrentMove.angle);


   if(positionChanged && !isRobot() )
   {
      mCurrentMove.time = (U32) connection->getOneWayTime();
      processMove(ActualState);
   }

   if(shipwarped)
   {
      mInterpolating = false;
      copyMoveState(ActualState, RenderState);

      for(S32 i = 0; i < TrailCount; i++)
         mTrail[i].reset();
   }
   else
      mInterpolating = true;


   if(playSpawnEffect)
   {
      mWarpInTimer.reset(WarpFadeInTime);    // Make ship all spinny

      static_cast<ClientGame *>(getGame())->emitTeleportInEffect(getActualPos(), 1);

      getGame()->playSoundEffect(SFXTeleportIn, getActualPos());
   }

   if(positionChanged)
      updateExtentInDatabase();

#endif
}  // unpackUpdate


F32 Ship::getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips)
{
   F32 value = Parent::getUpdatePriority(scopeObject, updateMask, updateSkips);

   if(getControllingClient())
      value += 2.3f;
   else
      value -= 2.3f;

   return value;
}


void Ship::updateInterpolation()
{
   Parent::updateInterpolation();

   // Update position of any mounted items
   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid())
         mMountedItems[i]->setRenderPos(getRenderPos());
}



bool Ship::hasModule(ShipModule mod)
{
   return mLoadout.hasModule(mod);
}


bool Ship::isDestroyed()
{
   return hasExploded;
}


bool Ship::isVisible(bool viewerHasSensor) 
{
   if(viewerHasSensor || !mLoadout.isModulePrimaryActive(ModuleCloak))
      return true;

   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid() && mMountedItems[i]->isItemThatMakesYouVisibleWhileCloaked())
         return true;

   return false;
}


// Returns index of first flag mounted on ship, or NO_FLAG if there aren't any
S32 Ship::getFlagIndex()
{
   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid() && (mMountedItems[i]->getObjectTypeNumber() == FlagTypeNumber))
         return i;
   return GameType::NO_FLAG;
}


S32 Ship::getFlagCount()
{
   S32 count = 0;
   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid() && (mMountedItems[i]->getObjectTypeNumber() == FlagTypeNumber))
      {
         FlagItem *flag = static_cast<FlagItem *>(mMountedItems[i].getPointer());
         count += flag->getFlagCount();      // Nexus flag have multiple flags as one item
      }
   return count;
}


bool Ship::isCarryingItem(U8 objectType) const
{
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
      if(mMountedItems[i].isValid() && mMountedItems[i]->getObjectTypeNumber() == objectType)
         return true;

   return false;
}


// Dismounts first object found of specified type, and returns the object.  If no objects of specified type found, will return NULL.
MountableItem *Ship::dismountFirst(U8 objectType)
{
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
      if(mMountedItems[i]->getObjectTypeNumber() == objectType)
      {
         MountableItem *item = mMountedItems[i];
         item->dismount(DISMOUNT_NORMAL);
         return item;
      }

   return NULL;
}


// Dismount all objects of any type -- runs on client and server.  Only runs when carrier was killed.
void Ship::dismountAll()
{
   // Count down here because as items are dismounted, they will be removed from the mMountedItems vector
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)       
      if(mMountedItems[i].isValid())               // Can be NULL when quitting the server
         mMountedItems[i]->dismount(DISMOUNT_MOUNT_WAS_KILLED);
}


// Dismount all objects of specified type.  Currently only used when loadout no longer includes engineer and ship drops all ResourceItems.
void Ship::dismountAll(U8 objectType)
{
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
      if(mMountedItems[i]->getObjectTypeNumber() == objectType)
         mMountedItems[i]->dismount(DISMOUNT_NORMAL);
}


bool Ship::isLoadoutSameAsCurrent(const LoadoutTracker &loadout)
{
   return loadout == mLoadout;      // Yay for operator overloading!
}


// This actualizes the requested loadout... when, for example the user enters a loadout zone
// To set the "on-deck" loadout, use GameType->setClientShipLoadout()
// Returns true if loadout has changed
// silent param only true when a ship is spawning and there is a loadout zone in the level; we need to update
// the ship's loadout to be what they had before they died, but we don't want to send a message to the client
// Server only?
bool Ship::setLoadout(const LoadoutTracker &loadout, bool silent)
{
   // Check to see if the new configuration is the same as the old.  If so, we have nothing to do.
   if(loadout == mLoadout)      // Don't bother if ship config hasn't changed
      return false;

   if(getClientInfo())
      getClientInfo()->getStatistics()->mChangedLoadout++;

   mLoadout = loadout;

   setMaskBits(LoadoutMask);

   // Set our current weapon to the first one, for consistency
   selectWeapon(0);

   if(!hasModule(ModuleEngineer))         // We don't have engineer, so drop any resources we may be carrying
   {
      dismountAll(ResourceItemTypeNumber);

      if(getClientInfo())
      {
         destroyPartiallyDeployedTeleporter();
         getClientInfo()->sTeleporterCleanup();
      }
   }

   if(!silent) 
   {
      // Notifiy user
      GameConnection *cc = getControllingClient();

      if(cc)
      {
         static StringTableEntry msg("Ship loadout configuration updated.");
         cc->s2cDisplayMessage(GameConnection::ColorInfo, SFXUIBoop, msg);
      }
   }

   return true;
}


// Runs on client and server
void Ship::setActiveWeapon(U32 weaponIndex)
{
   mLoadout.setActiveWeapon(weaponIndex);

#ifndef ZAP_DEDICATED
   // Notify the UI that the weapon has changed (mGame might be NULL when testing)
   if(mGame && !mGame->isServer())    
      static_cast<ClientGame *>(mGame)->setActiveWeapon(weaponIndex);
#endif
}


// Used by tests
WeaponType Ship::getActiveWeapon() const
{
   return mLoadout.getActiveWeapon();
}


// Used by tests
string Ship::getLoadoutString() const
{
   return mLoadout.toString();
}


bool Ship::isModulePrimaryActive(ShipModule mod)
{
   return mLoadout.isModulePrimaryActive(mod);
}


ShipModule Ship::getModule(U32 modIndex)
{
   return mLoadout.getModule(modIndex);
}


void Ship::killAndScore(DamageInfo *theInfo)
{
   if(isClient())     // Server only, please...
      return;

   GameType *gt = getGame()->getGameType();
   if(gt)
      gt->controlObjectForClientKilled(getClientInfo(), this, theInfo->damagingObject);

   kill();
}


// Ship was killed
void Ship::kill()
{
   if(isServer())    // Server only block
   {
      if(getOwner())
         getOwner()->saveActiveLoadout(mLoadout);      // Save current loadout in getOwner()->mActiveLoadout

      // Fire some events, starting with ShipKilledEvent
      EventManager::get()->fireEvent(EventManager::ShipKilledEvent, this);

      // Fire the ShipLeftZoneEvent for every zone the ship is in
      Vector<DatabaseObject *> *zoneList = &foundObjects;   // Reuse our reusable container

      getZonesShipIsIn(zoneList);
   
      for(S32 i = 0; i < zoneList->size(); i++)
         EventManager::get()->fireEvent(EventManager::ShipLeftZoneEvent, this, static_cast<Zone *>(zoneList->get(i)));
   }

   // Client and server
   deleteObject(KillDeleteDelay);
   hasExploded = true;
   setMaskBits(ExplodedMask);
   disableCollision();

   // Jettison any mounted items
   dismountAll();

   // Handle if in the middle of building a teleport
   if(isServer())   // Server only
   {
      destroyPartiallyDeployedTeleporter();
      if(getClientInfo())
         getClientInfo()->sTeleporterCleanup();
   }
}


// Server only -- ship has been killed, or player changed loadout in middle of engineering
void Ship::destroyPartiallyDeployedTeleporter()
{
   if(mEngineeredTeleporter)
   {
      Teleporter *t = mEngineeredTeleporter;
      mEngineeredTeleporter = NULL;          // Set to NULL first to avoid "Your teleporter got destroyed" message
      getGame()->teleporterDestroyed(t);
   }
}


void Ship::setChangeTeamMask()
{
   setMaskBits(ChangeTeamMask);  
}


// Client only
void Ship::emitExplosion()
{
#ifndef ZAP_DEDICATED
   TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");
   ClientGame *game = static_cast<ClientGame *>(getGame());

   game->emitShipExplosion(getRenderPos());
#endif
}


// Client only
void Ship::emitMovementSparks()
{
#ifndef ZAP_DEDICATED
   static const F32 TOO_SLOW_FOR_SPARKS = 0.1f;
   if(hasExploded || getActualVel().lenSquared() < sq(TOO_SLOW_FOR_SPARKS))
      return;

   ShipShapeInfo *shipShapeInfo = &ShipShape::shipShapeInfos[mShapeType];
   S32 cornerCount = shipShapeInfo->cornerCount;

   Vector<Point> corners;
   corners.resize(cornerCount);

   Vector<Point> shipDirs;
   shipDirs.resize(cornerCount);

   for(S32 i = 0; i < cornerCount; i++)
      corners[i].set(shipShapeInfo->cornerPoints[i*2], shipShapeInfo->cornerPoints[i*2 + 1]);

   F32 th = FloatHalfPi - getRenderAngle();

   F32 sinTh = sin(th);
   F32 cosTh = cos(th);
   F32 warpInScale = (WarpFadeInTime - mWarpInTimer.getCurrent()) / F32(WarpFadeInTime);

   for(S32 i = 0; i < cornerCount; i++)
   {
      shipDirs[i].x = corners[i].x * cosTh + corners[i].y * sinTh;
      shipDirs[i].y = corners[i].y * cosTh - corners[i].x * sinTh;
      shipDirs[i] *= warpInScale;
   }

   Point leftVec ( getActualVel().y, -getActualVel().x);
   Point rightVec(-getActualVel().y,  getActualVel().x);

   leftVec.normalize();
   rightVec.normalize();

   S32 bestId = 0, leftId, rightId;
   F32 bestDot = leftVec.dot(shipDirs[0]);

   // Find the left-wards match
   for(S32 i = 1; i < cornerCount; i++)
   {
      F32 d = leftVec.dot(shipDirs[i]);
      if(d >= bestDot)
      {
         bestDot = d;
         bestId = i;
      }
   }

   leftId = bestId;
   Point leftPt = getRenderPos() + shipDirs[bestId];

   // Find the right-wards match
   bestId = 0;
   bestDot = rightVec.dot(shipDirs[0]);

   for(S32 i = 1; i < cornerCount; i++)
   {
      F32 d = rightVec.dot(shipDirs[i]);
      if(d >= bestDot)
      {
         bestDot = d;
         bestId = i;
      }
   }

   rightId = bestId;
   Point rightPt = getRenderPos() + shipDirs[bestId];

   bool boostActive = mLoadout.isModulePrimaryActive(ModuleBoost);
   bool cloakActive = mLoadout.isModulePrimaryActive(ModuleCloak);

   // Select profile
   UI::TrailProfile profile;

   if(cloakActive)
      profile = UI::CloakedShipProfile;
   else if(boostActive)
      profile = UI::TurboShipProfile;
   else
      profile = UI::ShipProfile;

   // Stitch things up if we must...
   if(leftId == mLastTrailPoint[0] && rightId == mLastTrailPoint[1])
   {
      mTrail[0].update(leftPt,  profile);
      mTrail[1].update(rightPt, profile);
      mLastTrailPoint[0] = leftId;
      mLastTrailPoint[1] = rightId;
   }
   else if(leftId == mLastTrailPoint[1] && rightId == mLastTrailPoint[0])
   {
      mTrail[1].update(leftPt,  profile);
      mTrail[0].update(rightPt, profile);
      mLastTrailPoint[1] = leftId;
      mLastTrailPoint[0] = rightId;
   }
   else
   {
      mTrail[0].update(leftPt,  profile);
      mTrail[1].update(rightPt, profile);
      mLastTrailPoint[0] = leftId;
      mLastTrailPoint[1] = rightId;
   }

   if(mLoadout.isModulePrimaryActive(ModuleCloak))
      return;

   // Finally, do some particles
   Point velDir(mCurrentMove.x, mCurrentMove.y);
   F32 len = velDir.len();

   if(len > 0)
   {
      if(len > 1)
         velDir *= 1 / len;

      static Point shipDirs[4];
      shipDirs[0].set(cos(getRenderAngle()), sin(getRenderAngle()));
      shipDirs[1].set(-shipDirs[0]);
      shipDirs[2].set( shipDirs[0].y, -shipDirs[0].x);
      shipDirs[3].set(-shipDirs[0].y,  shipDirs[0].x);

      for(U32 i = 0; i < 4; i++)
      {
         F32 th = shipDirs[i].dot(velDir);

          if(th > 0.1)
          {
             // shoot some sparks...
             if(th >= 0.2*velDir.len())
             {
                Point chaos(TNL::Random::readF(),TNL::Random::readF());
                chaos *= 5;

                // interp give us some nice enginey colors...
                Color dim(Colors::red);
                Color light(1, 1, boostActive ? 1.f : 0.f);
                Color thrust;

                F32 t = TNL::Random::readF();
                thrust.interp(t, dim, light);

                TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");

                static_cast<ClientGame *>(getGame())->emitSpark(getRenderPos() - shipDirs[i] * 13,
                                          -shipDirs[i] * 100 + chaos, thrust, TNL::Random::readI(0, 1500), UI::SparkTypePoint);
             }
          }
      }
   }
#endif
}


// Client only
void Ship::updateTrails()
{
#ifndef ZAP_DEDICATED
   for(U32 i = 0; i < TrailCount; i++)
      mTrail[i].idle(mCurrentMove.time); 
#endif
}


void Ship::renderLayer(S32 layerIndex)
{
   TNLAssert(getGame()->getGameType(), "gameType should always be valid here");

#ifndef ZAP_DEDICATED
   if(layerIndex == 0)  // Only render on layers -1 and 1
      return;

   if(hasExploded)      // Don't render an exploded ship!
      return;

   ClientGame *clientGame = static_cast<ClientGame *>(getGame());
   GameConnection *conn = clientGame->getConnectionToServer();

   ClientInfo *clientInfo = getClientInfo();    // Could be NULL

   // This is the local player's ship -- could be NULL
   Ship *localShip = dynamic_cast<Ship *>(conn->getControlObject());

   const bool isLocalShip = !(conn && conn->getControlObject() != this);    // i.e. the ship belongs to the player viewing the rendering
   const bool isAuthenticated = clientInfo ? clientInfo->isAuthenticated() : false;

   const bool boostActive  = mLoadout.isModulePrimaryActive(ModuleBoost);
   const bool shieldActive = mLoadout.isModulePrimaryActive(ModuleShield);
   const bool repairActive = mLoadout.isModulePrimaryActive(ModuleRepair) && mHealth < 1;
   const bool sensorActive = doesShipActivateSensor(localShip);
   const bool hasArmor     = hasModule(ModuleArmor);

   const Point vel(mCurrentMove.x, mCurrentMove.y);

   // If the local player is cloaked, and is close enough to this ship, it will activate a sensor module, 
   // and we'll need to draw it.  Here, we determine if that has happened.
   
   const bool isBusy              = clientInfo ? clientInfo->isBusy() : false;
   const bool engineeringTeleport = clientInfo ? clientInfo->isEngineeringTeleporter() : false;
   const bool showCoordinates     = clientGame->isShowingDebugShipCoords();

   // Caclulate rotAmount to add the spinny effect you see when a ship spawns or comes through a teleport
   F32 warpInScale = (WarpFadeInTime - mWarpInTimer.getCurrent()) / F32(WarpFadeInTime);

   const string shipName = clientInfo ? clientInfo->getName().getString() : "";
   const U32 killStreak  = clientInfo ? clientInfo->getKillStreak() : 0;
   const U32 gamesPlayed  = clientInfo ? clientInfo->getGamesPlayed() : 0;

   const Color *color = getGame()->getGameType()->getTeamColor(this);
   F32 alpha = getShipVisibility(localShip);

   F32 angle = getRenderAngle();
   F32 deltaAngle = getAngleDiff(mLastProcessStateAngle, angle);     // Change in angle since we were last here

   renderShip(layerIndex, getRenderPos(), getActualPos(), vel, angle, deltaAngle,
              mShapeType, color, alpha, clientGame->getCurrentTime(), shipName, warpInScale, 
              isLocalShip, isBusy, isAuthenticated, showCoordinates, mHealth, mRadius, getTeam(), 
              boostActive, shieldActive, repairActive, sensorActive, hasArmor, engineeringTeleport, killStreak, 
              gamesPlayed);

   if(mSpawnShield.getCurrent() != 0)  // Add spawn shield -- has a period of being on solidly, then blinks yellow 
      renderSpawnShield(getRenderPos(), mSpawnShield.getCurrent(), clientGame->getCurrentTime());

   if(mLoadout.isModulePrimaryActive(ModuleRepair) && alpha != 0)     // Don't bother when completely transparent
      renderShipRepairRays(getRenderPos(), this, mRepairTargets, alpha);

   // Render mounted items
   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i].isValid())
         mMountedItems[i]->renderItemAlpha(getRenderPos(), alpha);
#endif
}


// Determine if the specified ship will activate our sensor.  Ship can be NULL.
bool Ship::doesShipActivateSensor(const Ship *ship)
{
   if(!ship)
      return false;

   // If ship is cloaking, and we have sensor...
   if(ship->mLoadout.isModulePrimaryActive(ModuleCloak) && hasModule(ModuleSensor))
   {
      // ...then check the distance
      F32 distanceSquared = (ship->getActualPos() - getActualPos()).lenSquared();

      return (distanceSquared < sq(ModuleInfo::SensorCloakOuterDetectionDistance));
   }

   return false;
}


// Set to true to allow players to see their cloaked teammates, 
// false if cloaked teammates should be invisible
static const bool SHOW_CLOAKED_TEAMMATES = true;    

// Determine ship's visibility, 0 = invisible, 1 = normal visibility.  
// Value will be used as alpha when rendering ship.
F32 Ship::getShipVisibility(const Ship *localShip)
{
   bool isLocalShip = (localShip == this);

   const bool cloakActive  = mLoadout.isModulePrimaryActive(ModuleCloak);
   const F32 cloakFraction =  mCloakTimer.getFraction();

   // If ship is using cloak module, we'll reduce its visibility
   F32 alpha = cloakActive ? cloakFraction : 1 - cloakFraction;

   S32 localTeam =  localShip ? localShip->getTeam() : NO_TEAM;
   bool teamGame = mGame->getGameType()->isTeamGame();
   bool isFriendly = (getTeam() == localTeam) && teamGame;

   // Don't completely hide local player or ships on same team (in a team game)
   if(isLocalShip || (SHOW_CLOAKED_TEAMMATES && isFriendly))
      return max(alpha, 0.25f);     // Make sure we have at least .25 alpha

   // Apply rules to cloaked players not on your team

   // If we have sensor equipped and this non-local ship is cloaked
   if(localShip && localShip->mLoadout.isModulePrimaryActive(ModuleSensor) && alpha < 0.5)
   {
      // Do a distance check - cloaked ships are detected at a reduced distance
      F32 distanceSquared = (localShip->getActualPos() - getActualPos()).lenSquared();

      // Ship is within outer detection radius
      if(distanceSquared < sq(ModuleInfo::SensorCloakOuterDetectionDistance))
      {
         // Inside inner radius? De-cloak a maximum of 0.5
         if(distanceSquared < sq(ModuleInfo::SensorCloakInnerDetectionDistance))
            return 0.5;

         // Otherwise de-cloak proportionally to the distance between inner and outer detection radii
         F32 ratio = (sq(ModuleInfo::SensorCloakOuterDetectionDistance) - distanceSquared) /
                     (sq(ModuleInfo::SensorCloakOuterDetectionDistance) - sq(ModuleInfo::SensorCloakInnerDetectionDistance));

         return sq(ratio) * 0.5f;  // Non-linear
      }
   }

   return alpha;
}


S32 Ship::getMountedItemCount() const
{
   return mMountedItems.size();
}


MountableItem *Ship::getMountedItem(S32 index) const
{
   if(index < 0 || index >= mMountedItems.size())
      return NULL;

   return mMountedItems[index];    
}


void Ship::addMountedItem(MountableItem *item)
{
   TNLAssert(item->getMount() == this, "Mounting to wrong ship!  Maybe try item.mountToShip(&ship);");
   mMountedItems.push_back(item);
}


// Supposes mountedItems are not repeated, and list is unordered
void Ship::removeMountedItem(MountableItem *item)
{
   for(S32 i = 0; i < mMountedItems.size(); i++)
      if(mMountedItems[i] == item)
      {
         mMountedItems.erase_fast(i);
         return;
      }
}


bool Ship::isRobot()
{
   return mIsRobot;
}


//// Lua methods

//               Fn name           Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, isAlive,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getPlayerInfo,   ARRAYDEF({{ END }}), 1 ) \
                                                           \
   METHOD(CLASS, isModActive,     ARRAYDEF({{ MOD_ENUM, END }}), 1 ) \
   METHOD(CLASS, getEnergy,       ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, setEnergy,       ARRAYDEF({{ NUM, END }}), 1 ) \
   METHOD(CLASS, getHealth,       ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, setHealth,       ARRAYDEF({{ NUM, END }}), 1 ) \
   METHOD(CLASS, hasFlag,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getFlagCount,    ARRAYDEF({{ END }}), 1 ) \
                                                           \
   METHOD(CLASS, getAngle,        ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getActiveWeapon, ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getMountedItems, ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getLoadout,      ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, setLoadout,      ARRAYDEF({{ TABLE, END }, { INT, INT, INT, INT, INT, END }}), 2 ) \
   METHOD(CLASS, setLoadoutNow,   ARRAYDEF({{ TABLE, END }, { INT, INT, INT, INT, INT, END }}), 2 ) \


GENERATE_LUA_METHODS_TABLE(Ship, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(Ship, LUA_METHODS);

#undef LUA_METHODS


const char *Ship::luaClassName = "Ship";
REGISTER_LUA_SUBCLASS(Ship, MoveObject);

// Note: All of these methods will return nil if the ship in question has been deleted.

/**
 * @luafunc bool Ship::isAlive()
 *
 * @brief Check if this Ship is alive.
 *
 * @return `true` if the Ship is present in the game world and still alive,
 * `false` otherwise.
 */
S32 Ship::lua_isAlive(lua_State *L)    { return returnBool(L, !isDestroyed()); }

/**
 * @luafunc bool Ship::hasFlag()
 *
 * @brief Check if this Ship is carrying a flag.
 *
 * @return `true` if the Ship is carrying at least one flag, `false` otherwise.
 */
S32 Ship::lua_hasFlag(lua_State *L)    { return returnBool (L, getFlagCount() > 0); }

/**
 * @luafunc int Ship::getFlagCount()
 *
 * @brief Get the number of flags carried by this Ship.
 *
 * @return The number of flags carried by this Ship.
 */
S32 Ship::lua_getFlagCount(lua_State *L) { return returnInt(L, getFlagCount()); }


/**
 * @luafunc LuaPlayerInfo Ship::getPlayerInfo()
 *
 * @brief Get the LuaPlayerInfo for this Ship.
 *
 * @return The LuaPlayerInfo for this Ship.
 */
S32 Ship::lua_getPlayerInfo(lua_State *L) { return returnPlayerInfo(L, this); }

/**
 * @luafunc num Ship::getAngle()
 *
 * @brief Get the angle of Ship.
 *
 * @return The Ship's angle in radians.
 */
S32 Ship::lua_getAngle(lua_State *L)        { return returnFloat(L, getCurrentMove().angle);     }  // Get angle ship is pointing at


/**
 * @luafunc Weapon Ship::getActiveWeapon()
 *
 * @brief Checks if the given module is active.
 *
 * @descr This will return an item of the \ref WeaponEnum enum, e.g.
 * `Weapon.Phaser`.
 *
 * @return int The \ref WeaponEnum that is currently active on this ship.
 */
S32 Ship::lua_getActiveWeapon(lua_State *L)
{
   return returnWeaponType(L, mLoadout.getActiveWeapon());
}


/**
 * @luafunc bool Ship::isModActive(Module module)
 *
 * @brief Checks if the given module is active.
 *
 * @descr This method takes a \ref ModuleEnum item as a parameter, e.g.
 * `Module.Shield`
 *
 * @param module The \ref ModuleEnum to check.
 *
 * @return `true` if the given \ref ModuleEnum is in active use, false
 * otherwise.
 */
S32 Ship::lua_isModActive(lua_State *L) {
   checkArgList(L, functionArgs, luaClassName, "isModActive");

   ShipModule module = getShipModule(L, 1);

   return returnBool(L, mLoadout.isModulePrimaryActive(module) || mLoadout.isModuleSecondaryActive(module));
}


/**
 * @luafunc num Ship::getEnergy()
 * 
 * @brief Gets the enegy of this ship.
 * 
 * @descr Energy is specified as a number between 0 and 1 where 0 means no
 * energy and 1 means full energy.
 * 
 * @return Returns a value between 0 and 1 indicating the energy of the item.
 */
S32 Ship::lua_getEnergy(lua_State *L)
{
   // Return ship's energy as a fraction between 0 and 1
   return returnFloat(L, (F32) mEnergy / (F32) EnergyMax);
}


/**
 * @luafunc Ship::setEnergy(num energy)
 * 
 * @brief Set the current energy of this ship.
 * 
 * @descr Energy is specified as a number between 0 and 1 where 0 means no
 * energy and 1 means full energy.  Values outside this range will be clamped to
 * the valid range.
 * 
 * @param energy A value between 0 and 1.
 */
S32 Ship::lua_setEnergy(lua_State *L)
{
   checkArgList(L, functionArgs, "Ship", "setEnergy");

   F32 param = getFloat(L, 1);
   S32 newEnergy = S32(CLAMP(param, 0.0f, 1.0f) * (F32) EnergyMax);

   // Determine our credit to be sent to the client, and send it
   S32 credit = CLAMP(newEnergy - mEnergy, -Ship::EnergyMax, Ship::EnergyMax);

   GameConnection *cc = getControllingClient();
   if(cc)
      cc->s2cCreditEnergy(credit);

   // Now set the server-side energy to the new one
   mEnergy = newEnergy;

   return 0;
}


/**
 * @luafunc num Ship::getHealth()
 * 
 * @brief Returns the health of this ship.
 * 
 * @descr Health is specified as a number between 0 and 1 where 0 is completely
 * dead and 1 is full health.
 * 
 * @return Returns a value between 0 and 1 indicating the health of the item.
 */
S32 Ship::lua_getHealth(lua_State *L)
{
   // Return ship's health as a fraction between 0 and 1
   return returnFloat(L, getHealth());
}


/**
 * @luafunc Ship::setHealth(num health)
 * 
 * @brief Set the current health of this ship.
 * 
 * @descr Health is specified as a number between 0 and 1 where 0 is completely
 * dead and 1 is full health. Values outside this range will be clamped to the
 * valid range.
 * 
 * @param health A value between 0 and 1.
 * 
 * @note A setting of 0 will kill the ship instantly.
 */
S32 Ship::lua_setHealth(lua_State *L)
{
   checkArgList(L, functionArgs, "Ship", "setHealth");

   F32 param = getFloat(L, 1);

   mHealth = CLAMP(param, 0.0f, 1.0f);

   // Transmit with next packet
   setMaskBits(HealthMask);

   if(mHealth <= 0)
   {
      DamageInfo di;
      di.damagingObject = NULL;
      killAndScore(&di);
   }

   return 0;
}


/**
 * @luafunc table Ship::getMountedItems()
 * 
 * @brief Get all Items carried by this Ship.
 * 
 * @return A table of all Items mounted on ship (e.g. ResourceItems and Flags)
 */
S32 Ship::lua_getMountedItems(lua_State *L)
{
   bool hasArgs = lua_isnumber(L, 1);
   Vector<BfObject *> tempVector;

   // Loop through all the mounted items
   for(S32 i = 0; i < mMountedItems.size(); i++)
   {
      // Add every item to the list if no arguments were specified
      if(!hasArgs)
         tempVector.push_back(dynamic_cast<BfObject *>(mMountedItems[i].getPointer()));

      // Else, compare against argument type and add to the list if matched
      else
      {
         S32 index = 1;
         while(lua_isnumber(L, index))
         {
            U8 objectType = (U8) lua_tointeger(L, index);

            if(mMountedItems[i]->getObjectTypeNumber() == objectType)
            {
               tempVector.push_back(dynamic_cast<BfObject *>(mMountedItems[i].getPointer()));
               break;
            }

            index++;
         }
      }
   }

   clearStack(L);

   lua_createtable(L, mMountedItems.size(), 0);    // Create a table, with enough slots pre-allocated for our data

   // Now push all found items back to LUA
   S32 pushed = 0;      // Count of items actually pushed onto the stack

   for(S32 i = 0; i < tempVector.size(); i++)
   {
      tempVector[i]->push(L);
      pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   return 1;
}


/**
 * @luafunc table Ship::getLoadout()
 * 
 * @brief Get the current loadout
 * 
 * @descr This method will return a table with the loadout in the following
 * order:
 * 
 * `Module 1, Module 2, Weapon 1, Weapon 2, Weapon 3`
 * 
 * @return A table with the current loadout, as described above.
 */
S32 Ship::lua_getLoadout(lua_State *L)
{
   // Create our loadout table
   lua_createtable(L, ShipModuleCount + ShipWeaponCount, 0);

   // Add current modules and weapons to the table
   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
   {
      // Modules
      if(i < ShipModuleCount)
         lua_pushinteger(L, (S32)mLoadout.getModule(i));

      // Weapons are offset by total module type count
      else
         lua_pushinteger(L, (S32)mLoadout.getWeapon(i - ShipModuleCount) + ModuleCount);

      // Lua uses 1-based arrays
      lua_rawseti(L, 1, i + 1);
   }

   // Return our table
   return 1;
}


LoadoutTracker Ship::checkAndBuildLoadout(lua_State *L, S32 profile)
{
   S32 expectedSize = ShipModuleCount + ShipWeaponCount;

   Vector<S32> loadoutValues(expectedSize);
   LoadoutTracker loadout;

   // A table
   if(profile == 0)
   {
      lua_pushnil(L);                             // table, nil
      while(lua_next(L, 1) != 0)                  // table, key, value
      {
         loadoutValues.push_back(getInt(L, -1));
         lua_pop(L, 1);                           // table, key
      }
   }
   // 5 parameters all integers, the argument list check guarantees 5 params here
   else
   {
      for(S32 i = 0; i < expectedSize; i++)
         loadoutValues.push_back(getInt(L, i + 1));
   }

   // Make sure we have the appropriate number of loadout values
   if(loadoutValues.size() != expectedSize)
      throw LuaException("The loadout given must contain " + itos(expectedSize) + " elements");


   // Now we verify and build up our loadout
   S32 moduleCount = 0;
   S32 weaponCount = 0;
   for(S32 i = 0; i < expectedSize; i++)
   {
      S32 value = loadoutValues[i];

      // Test if a weapon - the integer will be greater than ModuleCount
      if(value >= (S32)ModuleCount)
      {
         if(weaponCount >= ShipWeaponCount)
            throw LuaException("Too many weapons!  You must provide exactly " + itos(ShipWeaponCount) + " weapons.");

         loadout.setWeapon(weaponCount, WeaponType(value - ModuleCount));
         weaponCount++;
      }
      else
      {
         if(moduleCount >= ShipWeaponCount)
            throw LuaException("Too many modules!  You must provide exactly " + itos(ShipModuleCount) + " modules.");

         loadout.setModule(moduleCount, ShipModule(value));
         moduleCount++;
      }
   }

   // If we made it here without throwing an exception, then we have a loadout
   // with proper number of weapons/modules!
   return loadout;
}


/**
 * @luafunc Ship::setLoadout(Weapon w1, Weapon w2, Module m1, Module m2, Module m3)
 * @brief Convenience alias for setLoadout(table)
 *
 * @param w1 The new \ref WeaponEnum for slot 1.
 * @param w2 The new \ref WeaponEnum for slot 2.
 * @param m1 The new \ref ModuleEnum for slot 1.
 * @param m2 The new \ref ModuleEnum for slot 2.
 * @param m3 The new \ref ModuleEnum for slot 3.
 *
 * @luafunc Ship::setLoadout(table loadout)
 *
 * @brief Sets the requested loadout for the ship.
 *
 * @descr When setting the loadout, normal rules apply for updating the
 * loadout, e.g. moving over a loadout zone.
 *
 * This method will take a table with 5 entries in any order comprised of
 * 2 modules and 3 weapons.
 *
 * @note This method will also take 5 parameters as a new loadout, instead
 * of a table. See setLoadout(Weapon, Weapon, Module, Module, Module)
 *
 * @param loadout The new loadout to request.
 *
 * @see setLoadoutNow()
 */
S32 Ship::lua_setLoadout(lua_State *L)
{
   S32 profile = checkArgList(L, functionArgs, luaClassName, "setLoadout");

   LoadoutTracker loadout = checkAndBuildLoadout(L, profile);

   getOwner()->requestLoadout(loadout);

   return 0;
}


/**
 * @luafunc Ship::setLoadoutNow(table loadout)
 *
 * @brief Immediately sets the loadout for the ship.
 *
 * @descr This method does not require that you follow normal loadout-switching
 * rules.
 *
 * The parameters for this method follow the same rules as Ship::setLoadout().
 *
 * @param loadout The new loadout to set.
 *
 * @see setLoadout(loadout)
 */
S32 Ship::lua_setLoadoutNow(lua_State *L)
{
   S32 profile = checkArgList(L, functionArgs, luaClassName, "setLoadoutNow");

   LoadoutTracker loadout = checkAndBuildLoadout(L, profile);

   if(getClientInfo()->isLoadoutValid(loadout, getGame()->getGameType()->isEngineerEnabled()))
   {
      // Set requested loadout so we don't revert if going to a loadout zone
      // (this may set the loadout now if the ship is in a loadout zone)
      getClientInfo()->requestLoadout(loadout);

      // Set current loadout
      setLoadout(loadout);
   }
   else
      throw LuaException("The loadout given is invalid");

   return 0;
}


};

