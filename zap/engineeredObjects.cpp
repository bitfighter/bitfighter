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

#include "engineeredObjects.h"
#include "ship.h"
#include "../glut/glutInclude.h"
#include "projectile.h"
#include "gameType.h"
#include "gameWeapons.h"
#include "sfx.h"
#include "gameObjectRender.h"

namespace Zap
{


bool PolygonsIntersect(const Vector<Point> &p1, Point rp1, Point rp2)
{
   Point cp1 = p1[p1.size() - 1];
   for(S32 j = 0; j < p1.size(); j++)
   {
      Point cp2 = p1[j];
      if(segmentsIntersect(rp1, rp2, cp1, cp2))
         return true;
   }
   rp1 = rp2;
   //  line inside the other polygon?
   return PolygonContains2(p1.address(), p1.size(), rp1);
}
bool PolygonsIntersect(const Vector<Point> &p1, const Vector<Point> &p2)
{
   Point rp1 = p1[p1.size() - 1];
   for(S32 i = 0; i < p1.size(); i++)
   {
      Point rp2 = p1[i];

      Point cp1 = p2[p2.size() - 1];
      for(S32 j = 0; j < p2.size(); j++)
      {
         Point cp2 = p2[j];
         if(segmentsIntersect(rp1, rp2, cp1, cp2))
            return true;
/*
         Point ce = cp2 - cp1;
         Point n(-ce.y, ce.x);

         F32 distToZero = n.dot(cp1);

         F32 d1 = n.dot(rp1);
         F32 d2 = n.dot(rp2);

         bool d1in = d1 >= distToZero;
         bool d2in = d2 >= distToZero;

         if(!d1in && !d2in) // both points are outside this edge of the poly, so...
            break;
         else if((d1in && !d2in) || (d2in && !d1in))
         {
            // find the clip intersection point:
            F32 t = (distToZero - d1) / (d2 - d1);
            Point clipPoint = rp1 + (rp2 - rp1) * t;

            if(d1in)
               rp2 = clipPoint;
            else
               rp1 = clipPoint;
         }
         else if(j == p2.size() - 1)
            return true;

         // if both are in, go to the next edge.
*/
         cp1 = cp2;
      }
      rp1 = rp2;
   }
   //  all points of polygon is inside the other polygon?
   return PolygonContains2(p1.address(), p1.size(), p2[0]) || PolygonContains2(p2.address(), p2.size(), p1[0]);
}



static Vector<DatabaseObject *> fillVector;

// Returns true if deploy point is valid, false otherwise.  deployPosition and deployNormal are populated if successful.
bool EngineerModuleDeployer::findDeployPoint(Ship *ship, Point &deployPosition, Point &deployNormal)
{
   // Ship must be within Ship::MaxEngineerDistance of a wall, pointing at where the object should be placed
   Point startPoint = ship->getActualPos();
   Point endPoint = startPoint + ship->getAimVector() * Ship::MaxEngineerDistance;     

   F32 collisionTime;

   GameObject *hitObject = ship->findObjectLOS(BarrierType, MoveObject::ActualState, startPoint, endPoint, 
                                               collisionTime, deployNormal);

   if(!hitObject)    // No appropriate walls found, can't deploy, sorry!
      return false;


   if(deployNormal.dot(ship->getAimVector()) * 2 > 0)
      deployNormal = -deployNormal;      // This is to fix deploy at wrong side of barrier.


   // Set deploy point, and move one unit away from the wall (this is a tiny amount, keeps linework from overlapping with wall)
   deployPosition.set(startPoint + (endPoint - startPoint) * collisionTime + deployNormal);

   return true;
}


// Check for sufficient energy and resources; return empty string if everything is ok
string EngineerModuleDeployer::checkResourcesAndEnergy(Ship *ship)
{
   if(!ship->isCarryingItem(ResourceItemType)) 
      return "!!! Need resource item to use Engineer module";

   if(ship->getEnergy() < ship->getGame()->getModuleInfo(ModuleEngineer)->getPerUseCost())
      return "!!! Not enough energy to engineer an object";

   return "";
}


// Returns "" if location is OK, otherwise returns an error message
// Runs on client and server
bool EngineerModuleDeployer::canCreateObjectAtLocation(Ship *ship, U32 objectType)
{
   string msg;

   mErrorMessage = checkResourcesAndEnergy(ship);
   if(mErrorMessage != "")
      return false;

   if(!findDeployPoint(ship, mDeployPosition, mDeployNormal))
   {
      mErrorMessage = "!!! Could not find a suitable wall for mounting the item";
      return false;
   }

   Vector<Point> bounds;

   // Seems inefficient to construct these just for the purpose of bounds checking...
   switch(objectType)
   {
      case EngineeredTurret:
         Turret::getGeom(mDeployPosition, mDeployNormal, bounds);   
         break;
      case EngineeredForceField:
         ForceFieldProjector::getGeom(mDeployPosition, mDeployNormal, bounds);
         break;
      default:    // will never happen
         TNLAssert(false, "Bad objectType");
         return false;
   }

   if(!EngineeredObject::checkDeploymentPosition(bounds, ship->getGridDatabase()))
   {
      mErrorMessage = "!!! Cannot deploy item at this location";
      return false;
   }

   // If this is a turret, then this location is good; we're done
   if(objectType == EngineeredTurret)
      return true;


   // Forcefields only from here on down; we've got miles to go before we sleep

   // We need to ensure forcefield doesn't cross another; doing so can create an impossible situation
   // Forcefield starts at the end of the projector.  Need to know where that is.
   Point forceFieldStart = ForceFieldProjector::getForceFieldStartPoint(mDeployPosition, mDeployNormal, 0);

   // Now we can find the end point...
   Point endPoint;
   DatabaseObject *collObj;
   ForceField::findForceFieldEnd(ship->getGridDatabase(), forceFieldStart, mDeployNormal, 1.0, endPoint, &collObj);

   // Create a temp forcefield to use for collision testing with other forcefields; this one will be deleted below
   //ForceField *newForceField = new ForceField(-1, forceFieldStart, endPoint);

   // Now we can do some actual checking.  We'll do this in two passes, one against existing ffs, another against disabled
   // projectors that might be enabled in the future.
   Vector<DatabaseObject *> fillVector;
   bool collision = false;

   // First test existing forcefields -- these will be faster than checking inactive projectors, which we'll do later
   //Rect queryRect = (forceFieldStart, ForceField::MAX_FORCEFIELD_LENGTH * 2);
   Rect queryRect(forceFieldStart, endPoint);
   queryRect.expand(Point(5,5));
   ship->getGridDatabase()->findObjects(ForceFieldProjectorType, fillVector, queryRect);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      ForceFieldProjector *ffp = dynamic_cast<ForceFieldProjector *>(fillVector[i]);
      if(ffp)
      {
         Vector<Point> thisGeom;
         Point start,end;
         ffp->getCollisionPoly(thisGeom);
         if(PolygonsIntersect(thisGeom, start, end))
         {
            collision = true;
            break;
         }
      }
   }

   if(!collision)
   {
   fillVector.clear();
   ship->getGridDatabase()->findObjects(ForceFieldType, fillVector, queryRect);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      ForceField *ff = dynamic_cast<ForceField *>(fillVector[i]);
      if(ff)// && ff->intersects(newForceField)) // intersect slow
      {
         Point start, end;
         ff->getForceFieldStartAndEndPoints(start, end);
         if(segmentsIntersect(forceFieldStart, endPoint, start, end))
         {
            collision = true;
            break;
         }
      }
   }
   }

   if(!collision)
   {
      // First we must find any possibly intersecting forcefield projector and if it's inactive, create a temp forcefield
      // Projectors up to two forcefield lengths away must be considered because the end of one could intersect the end of the other
      fillVector.clear();
      queryRect.expand(Point(ForceField::MAX_FORCEFIELD_LENGTH, ForceField::MAX_FORCEFIELD_LENGTH));
      ship->getGridDatabase()->findObjects(ForceFieldProjectorType, fillVector, queryRect);

      for(S32 i = 0; i < fillVector.size(); i++)
      {
         ForceFieldProjector *proj = dynamic_cast<ForceFieldProjector *>(fillVector[i]);
         if(proj)// && !proj->isEnabled())      // Enabled projectors handled in forcefield search above
         {
            Point start, end;
            proj->getForceFieldStartAndEndPoints(start, end);

            //ForceField forceField = ForceField(0, start, end);

            //if(forceField.intersects(newForceField))

            if(segmentsIntersect(forceFieldStart, endPoint, start, end))
            {
               collision = true;
               break;
            }
         }
      }
   }

   //delete newForceField;

   if(collision)
   {
      mErrorMessage = "!!! Cannot deply forcefield where it could cross another.";
      return false;
   }

   return true;     // We've run the gammut -- this location is OK
}


// Runs on server
// Only run after canCreateObjectAtLocation, which checks for errors and sets mDeployPosition
bool EngineerModuleDeployer::deployEngineeredItem(GameConnection *connection, U32 objectType)
{
   // Do some basic crash-proofing sanity checks first
   Ship *ship = dynamic_cast<Ship *>(connection->getControlObject());
   if(!ship)
      return false;

   EngineeredObject *deployedObject = NULL;

   switch(objectType)
   {
      case EngineeredTurret:
         deployedObject = new Turret(ship->getTeam(), mDeployPosition, mDeployNormal);    // Calc'ed in canCreateObjectAtLocation
         break;
      case EngineeredForceField:
         deployedObject = new ForceFieldProjector(ship->getTeam(), mDeployPosition, mDeployNormal);
         break;
      default:
         return false;
   }

   deployedObject->setOwner(connection);
   deployedObject->computeExtent();

   if(!deployedObject)              // Something went wrong
   {
      connection->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "Error deploying object.");
      delete deployedObject;
      return false;
   }

   ship->engineerBuildObject();     // Deducts energy

   deployedObject->addToGame(gServerGame);
   deployedObject->onEnabled();

   Item *resource = ship->unmountItem(ResourceItemType);

   deployedObject->setResource(resource);

   return true;
}


// Constructor
EngineeredObject::EngineeredObject(S32 team, Point anchorPoint, Point anchorNormal)
{
   mHealth = 1.0f;
   mTeam = team;
   mOriginalTeam = mTeam;
   mAnchorPoint = anchorPoint;
   mAnchorNormal = anchorNormal;
   mIsDestroyed = false;
   mHealRate = 0;

   //setObjectMask();  // --> Moved to child classes for the moment, because there was a problem with inheritence.
}


bool EngineeredObject::processArguments(S32 argc, const char **argv)
{
   if(argc < 3)
      return false;

   mTeam = atoi(argv[0]);
   mOriginalTeam = mTeam;
   if(mTeam == -1)      // Neutral object starts with no health, can be repaired and claimed by anyone
      mHealth = 0;
   
   Point pos;
   pos.read(argv + 1);
   pos *= getGame()->getGridSize();

   if(argc >= 4)
   {
      mHealRate = atoi(argv[3]);
      mHealTimer.setPeriod(mHealRate * 1000);
   }

   // Find the mount point:
   Point normal, anchor;

   if(!findAnchorPointAndNormal(getGridDatabase(), pos, MAX_SNAP_DISTANCE, true, anchor, normal))
      return false;      // Found no mount point

   mAnchorPoint.set(anchor + normal);
   mAnchorNormal.set(normal);
   computeExtent();

   if(mHealth != 0)
      onEnabled();

   return true;
}


// This is used for both positioning items in-game and for snapping them to walls in the editor --> static method
DatabaseObject *EngineeredObject::findAnchorPointAndNormal(GridDatabase *db, const Point &pos, F32 snapDist, 
                                                           bool format, Point &anchor, Point &normal)
{
   F32 minDist = F32_MAX;
   DatabaseObject *closestWall = NULL;

   // Start with a sweep of the area
   for(F32 theta = 0; theta < Float2Pi; theta += FloatPi * 0.125)    // Reducing to 0.0125 seems to have no effect
   {
      Point dir(cos(theta), sin(theta));
      dir *= snapDist;

      F32 t;
      Point n;

      // Look for walls
      DatabaseObject *wall = db->findObjectLOS(BarrierType, MoveObject::ActualState, format, pos, pos + dir, t, n);

      if(wall != NULL)     // Found one!
      {
         if(t < minDist)
         {
            anchor.set(pos + dir * t);
            normal.set(n);
            minDist = t;
            closestWall = wall;
         }
      }
   }

   return closestWall;
}


void EngineeredObject::setResource(Item *resource)
{
   TNLAssert(resource->isMounted() == false, "Doh!");
   mResource = resource;
   mResource->removeFromDatabase();
}


static const F32 disabledLevel = 0.25;

bool EngineeredObject::isEnabled()
{
   return mHealth >= disabledLevel;
}


void EngineeredObject::damageObject(DamageInfo *di)
{
   F32 prevHealth = mHealth;

   if(di->damageAmount > 0)
      mHealth -= di->damageAmount * .25f; // ???
   else
      mHealth -= di->damageAmount;

   if(mHealth < 0)
      mHealth = 0;

   mHealTimer.reset();     // Restart healing timer...

   // No additional damage, nothing more to do (i.e. was already at 0)
   if(prevHealth == mHealth)
      return;

   setMaskBits(HealthMask);

   // Check if turret just died
   if(prevHealth >= disabledLevel && mHealth < disabledLevel)        // Turret just died
   {
      // Revert team to neutral if this was a repaired turret
      if(mTeam != mOriginalTeam)
      {
         mTeam = mOriginalTeam;
         setMaskBits(TeamMask);
      }
      onDisabled();

      // Handle scoring
      if( isTurret() && di->damagingObject && di->damagingObject->getOwner() && di->damagingObject->getOwner()->getControlObject() )
      {
         Ship *s = dynamic_cast<Ship *>(di->damagingObject->getOwner()->getControlObject());
         if(s)
         {
            GameType *gt = getGame()->getGameType();

            if(gt->isTeamGame() && s->getTeam() == getTeam())
               gt->updateScore(s, GameType::KillOwnTurret);
            else
               gt->updateScore(s, GameType::KillEnemyTurret);
         }
      }
   }
   else if(prevHealth < disabledLevel && mHealth >= disabledLevel)   // Turret was just repaired or healed
   {
      if(mTeam == -1)                                 // Neutral objects...
      {
         if(di->damagingObject)
         {
            mTeam = di->damagingObject->getTeam();    // ...join the team of their repairer
            setMaskBits(TeamMask);                    // Broadcast new team status
         }
      }
      onEnabled();
   }

   if(mHealth == 0 && mResource.isValid())
   {
      mIsDestroyed = true;
      onDestroyed();

      mResource->addToDatabase();
      mResource->setActualPos(mAnchorPoint + mAnchorNormal * mResource->getRadius());

      deleteObject(500);
   }
}


void EngineeredObject::computeExtent()
{
   Vector<Point> v;
   getCollisionPoly(v);
   Rect r(v);

   setExtent(r);
}


void EngineeredObject::explode()
{
   const S32 EXPLOSION_COLOR_COUNT = 12;

   static Color ExplosionColors[EXPLOSION_COLOR_COUNT] = {
   Color(1, 0, 0),
   Color(0.9, 0.5, 0),
   Color(1, 1, 1),
   Color(1, 1, 0),
   Color(1, 0, 0),
   Color(0.8, 1.0, 0),
   Color(1, 0.5, 0),
   Color(1, 1, 1),
   Color(1, 0, 0),
   Color(0.9, 0.5, 0),
   Color(1, 1, 1),
   Color(1, 1, 0),
   };

   SFXObject::play(SFXShipExplode, getActualPos(), Point());

   F32 a = TNL::Random::readF() * 0.4 + 0.5;
   F32 b = TNL::Random::readF() * 0.2 + 0.9;
   F32 c = TNL::Random::readF() * 0.15 + 0.125;
   F32 d = TNL::Random::readF() * 0.2 + 0.9;

   FXManager::emitExplosion(getActualPos(), 0.65, ExplosionColors, EXPLOSION_COLOR_COUNT);
   FXManager::emitBurst(getActualPos(), Point(a,c) * 0.6, Color(1,1,0.25), Color(1,0,0));
   FXManager::emitBurst(getActualPos(), Point(b,d) * 0.6, Color(1,1,0), Color(0,1,1));

   disableCollision();
}


// Make sure position looks good when player deploys item with Engineer module -- make sure we're not deploying on top of
// a wall or another engineered item
// static method
bool EngineeredObject::checkDeploymentPosition(const Vector<Point> &thisBounds, GridDatabase *gb)
{
   Vector<DatabaseObject *> foundObjects;
   Rect queryRect(thisBounds);
   gb->findObjects(BarrierType | EngineeredType, foundObjects, queryRect);

   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      Vector<Point> foundObjectBounds;
      dynamic_cast<GameObject *>(foundObjects[i])->getCollisionPoly(foundObjectBounds);

      if(PolygonsIntersect(thisBounds, foundObjectBounds))     // Do they intersect?
         return false;     // Bad location
   }
   return true;            // Good location
}


U32 EngineeredObject::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
   {
      stream->write(mAnchorPoint.x);
      stream->write(mAnchorPoint.y);
      stream->write(mAnchorNormal.x);
      stream->write(mAnchorNormal.y);
   }

   if(stream->writeFlag(updateMask & TeamMask))
      stream->write(mTeam);

   if(stream->writeFlag(updateMask & HealthMask))
   {
      stream->writeFloat(mHealth, 6);
      stream->writeFlag(mIsDestroyed);
   }
   return 0;
}


void EngineeredObject::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;
   if(stream->readFlag())
   {
      initial = true;
      stream->read(&mAnchorPoint.x);
      stream->read(&mAnchorPoint.y);
      stream->read(&mAnchorNormal.x);
      stream->read(&mAnchorNormal.y);
      computeExtent();
   }
   if(stream->readFlag())
      stream->read(&mTeam);

   if(stream->readFlag())
   {
      mHealth = stream->readFloat(6);
      bool wasDestroyed = mIsDestroyed;
      mIsDestroyed = stream->readFlag();

      if(mIsDestroyed && !wasDestroyed && !initial)
         explode();
   }
}


void EngineeredObject::healObject(S32 time)
{
   if(mHealRate == 0 || mTeam == -1)      // Neutral items don't heal!
      return;

   F32 prevHealth = mHealth;

   if(mHealTimer.update(time))
   {
      mHealth += .1;
      setMaskBits(HealthMask);

      if(mHealth >= 1)
         mHealth = 1;
      else
         mHealTimer.reset();

      if(prevHealth < disabledLevel && mHealth >= disabledLevel)
         onEnabled();
   }
}


TNL_IMPLEMENT_NETOBJECT(ForceFieldProjector);

// Constructor
ForceFieldProjector::ForceFieldProjector(S32 team, Point anchorPoint, Point anchorNormal) : EngineeredObject(team, anchorPoint, anchorNormal)
{
   mNetFlags.set(Ghostable);
   setObjectMask();
}

void ForceFieldProjector::onDisabled()
{
   if(mField.isValid())
      mField->deleteObject(0);
}


void ForceFieldProjector::idle(GameObject::IdleCallPath path)
{
   if(path != ServerIdleMainLoop)
      return;

   healObject(mCurrentMove.time);
}

static const S32 PROJECTOR_HALF_WIDTH = 12;  // Half the width of base of the projector, along the wall
static const S32 PROJECTOR_OFFSET = 15;      // Distance from wall to projector tip; thickness, if you will

// static method
void ForceFieldProjector::getGeom(const Point &anchor, const Point &normal, Vector<Point> &geom)      
{
   Point cross(normal.y, -normal.x);
   cross.normalize(PROJECTOR_HALF_WIDTH);

   geom.push_back(anchor + cross);
   geom.push_back(getForceFieldStartPoint(anchor, normal));
   geom.push_back(anchor - cross);
}


// Get the point where the forcefield actually starts, as it leaves the projector; i.e. the tip of the projector.  Static method.
Point ForceFieldProjector::getForceFieldStartPoint(const Point &anchor, const Point &normal, F32 scaleFact)
{
   return Point(anchor.x + normal.x * PROJECTOR_OFFSET * scaleFact, 
                anchor.y + normal.y * PROJECTOR_OFFSET * scaleFact);
}


void ForceFieldProjector::getForceFieldStartAndEndPoints(Point &start, Point &end)
{
   start = getForceFieldStartPoint(mAnchorPoint, mAnchorNormal);

   DatabaseObject *collObj;
   ForceField::findForceFieldEnd(getGridDatabase(), getForceFieldStartPoint(mAnchorPoint, mAnchorNormal), mAnchorNormal, 1.0, end, &collObj);
}


void ForceFieldProjector::onEnabled()
{
   Point start = getForceFieldStartPoint(mAnchorPoint, mAnchorNormal);
   Point end;
   DatabaseObject *collObj;
   
   ForceField::findForceFieldEnd(getGridDatabase(), start, mAnchorNormal, 1.0, end, &collObj);

   mField = new ForceField(mTeam, start, end);
   mField->addToGame(getGame());
}


bool ForceFieldProjector::getCollisionPoly(Vector<Point> &polyPoints)
{
   getGeom(mAnchorPoint, mAnchorNormal, polyPoints);
   return true;
}

void ForceFieldProjector::onAddedToGame(Game *theGame)
{
   getGame()->mObjectsLoaded++;
}


void ForceFieldProjector::render()
{
   renderForceFieldProjector(mAnchorPoint, mAnchorNormal, getGame()->getGameType()->getTeamColor(getTeam()), isEnabled());
}

// Lua methods

const char ForceFieldProjector::className[] = "ForceFieldProjector";      // Class name as it appears to Lua scripts

// Lua constructor
ForceFieldProjector::ForceFieldProjector(lua_State *L)
{
   // Do nothing
}


// Define the methods we will expose to Lua
Lunar<ForceFieldProjector>::RegType ForceFieldProjector::methods[] =
{
   // Standard gameItem methods
   method(ForceFieldProjector, getClassID),
   method(ForceFieldProjector, getLoc),
   method(ForceFieldProjector, getRad),
   method(ForceFieldProjector, getVel),
   method(ForceFieldProjector, getTeamIndx),

   method(ForceFieldProjector, getHealth),
   method(ForceFieldProjector, isActive),

   {0,0}    // End method list
};

////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(ForceField);

ForceField::ForceField(S32 team, Point start, Point end)
{
   mTeam = team;
   mStart = start;
   mEnd = end;

   Rect extent(mStart, mEnd);
   extent.expand(Point(5,5));
   setExtent(extent);

   mFieldUp = true;
   mObjectTypeMask = ForceFieldType | CommandMapVisType;
   mNetFlags.set(Ghostable);
}

bool ForceField::collide(GameObject *hitObject)
{
   if(!mFieldUp)
      return false;

   if( ! (hitObject->getObjectTypeMask() & (ShipType | RobotType)))
      return true;

   if(hitObject->getTeam() == mTeam)
   {
      if(!isGhost())
      {
         mFieldUp = false;
         mDownTimer.reset(FieldDownTime);
         setMaskBits(StatusMask);
      }
      return false;
   }
   return true;
}


// Returns true if two forcefields intersect
bool ForceField::intersects(ForceField *forceField)
{
   Vector<Point> thisGeom, thatGeom;

   getGeom(thisGeom);
   forceField->getGeom(thatGeom);

   return PolygonsIntersect(thisGeom, thatGeom);
}


void ForceField::idle(GameObject::IdleCallPath path)
{
   if(path != ServerIdleMainLoop)
      return;

   if(mDownTimer.update(mCurrentMove.time))
   {
      // do an LOS test to see if anything is in the field:
      F32 t;
      Point n;
      if(!findObjectLOS(ShipType | RobotType | ItemType, MoveObject::ActualState, mStart, mEnd, t, n))
      {
         mFieldUp = true;
         setMaskBits(StatusMask);
      }
      else
         mDownTimer.reset(10);
   }
}

U32 ForceField::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
   {
      stream->write(mStart.x);
      stream->write(mStart.y);
      stream->write(mEnd.x);
      stream->write(mEnd.y);
      stream->write(mTeam);
   }
   stream->writeFlag(mFieldUp);
   return 0;
}


void ForceField::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;
   if(stream->readFlag())
   {
      initial = true;
      stream->read(&mStart.x);
      stream->read(&mStart.y);
      stream->read(&mEnd.x);
      stream->read(&mEnd.y);
      stream->read(&mTeam);

      Rect extent(mStart, mEnd);
      extent.expand(Point(5,5));
      setExtent(extent);
   }
   bool wasUp = mFieldUp;
   mFieldUp = stream->readFlag();

   if(initial || (wasUp != mFieldUp))
      SFXObject::play(mFieldUp ? SFXForceFieldUp : SFXForceFieldDown, mStart, Point());
}


void ForceField::getGeom(const Point &start, const Point &end, Vector<Point> &geom, F32 scaleFact)
{
   static const F32 FORCEFIELD_HALF_WIDTH = 2.5;

   Point normal(end.y - start.y, start.x - end.x);
   normal.normalize(FORCEFIELD_HALF_WIDTH * scaleFact);    

   geom.push_back(start + normal);
   geom.push_back(end + normal);
   geom.push_back(end - normal);
   geom.push_back(start - normal);
}


// Non-static version
void ForceField::getGeom(Vector<Point> &geom)
{
   getGeom(mStart, mEnd, geom);
}


bool ForceField::findForceFieldEnd(GridDatabase *db, const Point &start, const Point &normal, F32 scaleFact, 
                                   Point &end, DatabaseObject **collObj)
{
   F32 time;
   Point n;

   end.set(start.x + normal.x * MAX_FORCEFIELD_LENGTH * scaleFact, 
           start.y + normal.y * MAX_FORCEFIELD_LENGTH * scaleFact);

   *collObj = db->findObjectLOS(BarrierType, MoveObject::ActualState, start, end, time, n);

   if(*collObj)
   {
      end.set(start + (end - start) * time); 
      return true;
   }

   return false;
}


bool ForceField::getCollisionPoly(Vector<Point> &points)
{
   getGeom(points);
   return true;
}


void ForceField::render()
{
   Color c = getGame()->getGameType()->getTeamColor(mTeam);
   renderForceField(mStart, mEnd, c, mFieldUp);
}


TNL_IMPLEMENT_NETOBJECT(Turret);

// Constructor
Turret::Turret(S32 team, Point anchorPoint, Point anchorNormal) : EngineeredObject(team, anchorPoint, anchorNormal)
{
   mNetFlags.set(Ghostable);
   setObjectMask();
}


// static method
void Turret::getGeom(const Point &anchor, const Point &normal, Vector<Point> &polyPoints)
{
   Point cross(normal.y, -normal.x);
   polyPoints.push_back(anchor + cross * 25);
   polyPoints.push_back(anchor + cross * 10 + Point(normal) * 45);
   polyPoints.push_back(anchor - cross * 10 + Point(normal) * 45);
   polyPoints.push_back(anchor - cross * 25);
}


bool Turret::getCollisionPoly(Vector<Point> &polyPoints)
{
   getGeom(mAnchorPoint, mAnchorNormal, polyPoints);
   return true;
}


void Turret::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);
   mCurrentAngle = atan2(mAnchorNormal.y, mAnchorNormal.x);
   getGame()->mObjectsLoaded++;     // N.B.: For some reason this has no effect on the client
}


void Turret::render()
{
   Color c;

   if(gClientGame->getGameType())
      c = gClientGame->getGameType()->getTeamColor(mTeam);
   else
      c = Color(1,1,1);

   renderTurret(c, mAnchorPoint, mAnchorNormal, isEnabled(), mHealth, mCurrentAngle);
}


U32 Turret::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 ret = Parent::packUpdate(connection, updateMask, stream);
   if(stream->writeFlag(updateMask & AimMask))
      stream->write(mCurrentAngle);

   return ret;
}


void Turret::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);
   if(stream->readFlag())
      stream->read(&mCurrentAngle);
}


extern ServerGame *gServerGame;
extern bool FindLowestRootInInterval(F32 inA, F32 inB, F32 inC, F32 inUpperBound, F32 &outX);

// Choose target, aim, and, if possible, fire
void Turret::idle(IdleCallPath path)
{
   if(path != ServerIdleMainLoop)
      return;

   // Server only!

   healObject(mCurrentMove.time);

   if(!isEnabled())
      return;

   mFireTimer.update(mCurrentMove.time);

   // Choose best target:
   Point aimPos = mAnchorPoint + mAnchorNormal * TURRET_OFFSET;
   Point cross(mAnchorNormal.y, -mAnchorNormal.x);

   Rect queryRect(aimPos, aimPos);
   queryRect.unionPoint(aimPos + cross * TurretPerceptionDistance);
   queryRect.unionPoint(aimPos - cross * TurretPerceptionDistance);
   queryRect.unionPoint(aimPos + mAnchorNormal * TurretPerceptionDistance);
   fillVector.clear();
   findObjects(TurretTargetType, fillVector, queryRect);    // Get all potential targets

   GameObject *bestTarget = NULL;
   F32 bestRange = F32_MAX;
   Point bestDelta;

   Point delta;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      if(fillVector[i]->getObjectTypeMask() & ( ShipType | RobotType))
      {
         Ship *potential = (Ship*)fillVector[i];

         // Is it dead or cloaked?  Carrying objects makes ship visible, except in nexus game
         TNLAssert(gServerGame->getGameType(), "Bad GameType!");
         if(!potential->isVisible() || potential->hasExploded)
            continue;
      }

      // Don't target mounted items (like resourceItems and flagItems)
      Item *item = dynamic_cast<Item *>(fillVector[i]);
      if(item && item->isMounted())
         continue;

      GameObject *potential = dynamic_cast<GameObject *>(fillVector[i]);
      if(potential->getTeam() == mTeam)      // Is target on our team?
         continue;                           // ...if so, skip it!

      // Calculate where we have to shoot to hit this...
      Point Vs = potential->getActualVel();
      F32 S = gWeapons[WeaponTurret].projVelocity;
      Point d = potential->getRenderPos() - aimPos;

// This could possibly be combined with LuaRobot's getFiringSolution, as it's essentially the same thing
      F32 t;      // t is set in next statement
      if(!FindLowestRootInInterval(Vs.dot(Vs) - S * S, 2 * Vs.dot(d), d.dot(d), gWeapons[WeaponTurret].projLiveTime * 0.001f, t))
         continue;

      Point leadPos = potential->getRenderPos() + Vs * t;

      // Calculate distance
      delta = (leadPos - aimPos);

      Point angleCheck = delta;
      angleCheck.normalize();

      // Check that we're facing it...
      if(angleCheck.dot(mAnchorNormal) <= -0.1f)
         continue;

      // See if we can see it...
      Point n;
      if(findObjectLOS(BarrierType, MoveObject::ActualState, aimPos, potential->getActualPos(), t, n))
         continue;

      // See if we're gonna clobber our own stuff...
      disableCollision();
      Point delta2 = delta;
      delta2.normalize(gWeapons[WeaponTurret].projLiveTime * gWeapons[WeaponTurret].projVelocity / 1000);
      GameObject *hitObject = findObjectLOS(ShipType | RobotType | BarrierType | EngineeredType, 0, aimPos, aimPos + delta2, t, n);
      enableCollision();

      if(hitObject && hitObject->getTeam() == mTeam)
         continue;

      F32 dist = delta.len();

      if(dist < bestRange)
      {
         bestDelta  = delta;
         bestRange  = dist;
         bestTarget = potential;
      }
   }

   if(!bestTarget)      // No target, nothing to do
      return;
 
   // Aim towards the best target.  Note that if the turret is at one extreme of its range, and the target is at the other,
   // then the turret will rotate the wrong-way around to aim at the target.  If we were to detect that condition here, and
   // constrain our turret to turning the correct direction, that would be great!!
   F32 destAngle = bestDelta.ATAN2();

   F32 angleDelta = destAngle - mCurrentAngle;

   if(angleDelta > FloatPi)
      angleDelta -= Float2Pi;
   else if(angleDelta < -FloatPi)
      angleDelta += Float2Pi;

   F32 maxTurn = TurretTurnRate * mCurrentMove.time * 0.001f;
   if(angleDelta != 0)
      setMaskBits(AimMask);

   if(angleDelta > maxTurn)
      mCurrentAngle += maxTurn;
   else if(angleDelta < -maxTurn)
      mCurrentAngle -= maxTurn;
   else
   {
      mCurrentAngle = destAngle;

      if(mFireTimer.getCurrent() == 0)
      {
         bestDelta.normalize();
         Point velocity;

         // String handling in C++ is such a mess!!!
         string killer = string("got blasted by ") + getGame()->getGameType()->getTeamName(mTeam).getString() + " turret";
         mKillString = killer.c_str();

         createWeaponProjectiles(WeaponTurret, bestDelta, aimPos, velocity, 35.0f, this);
         mFireTimer.reset(gWeapons[WeaponTurret].fireDelay);
      }
   }
}


const char Turret::className[] = "Turret";      // Class name as it appears to Lua scripts

// Lua constructor
Turret::Turret(lua_State *L)
{
   // Do nothing
}


// Define the methods we will expose to Lua
Lunar<Turret>::RegType Turret::methods[] =
{
   // Standard gameItem methods
   method(Turret, getClassID),
   method(Turret, getLoc),
   method(Turret, getRad),
   method(Turret, getVel),
   method(Turret, getTeamIndx),

   method(Turret, getHealth),
   method(Turret, isActive),

   {0,0}    // End method list
};

};

