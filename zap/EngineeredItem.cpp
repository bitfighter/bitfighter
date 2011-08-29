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

#include "EngineeredItem.h"
#include "ship.h"
#include "projectile.h"
#include "gameType.h"
#include "gameWeapons.h"
#include "SoundSystem.h"
#include "gameObjectRender.h"
#include "GeomUtils.h"
#include "BotNavMeshZone.h"
#include "Colors.h"
#include "game.h"
#include "stringUtils.h"
#include "gameConnection.h"

#include <math.h>

namespace Zap
{

static bool forceFieldEdgesIntersectPoints(const Vector<Point> &points, const Vector<Point> forceField)
{
   return polygonIntersectsSegment(points, forceField[0], forceField[1]) || polygonIntersectsSegment(points, forceField[2], forceField[3]);
}



// Returns true if deploy point is valid, false otherwise.  deployPosition and deployNormal are populated if successful.
bool EngineerModuleDeployer::findDeployPoint(Ship *ship, Point &deployPosition, Point &deployNormal)
{
   // Ship must be within Ship::MaxEngineerDistance of a wall, pointing at where the object should be placed
   Point startPoint = ship->getActualPos();
   Point endPoint = startPoint + ship->getAimVector() * Ship::MaxEngineerDistance;     

   F32 collisionTime;

   GameObject *hitObject = ship->findObjectLOS((TestFunc)isWallType, MoveObject::ActualState, startPoint, endPoint,
                                               collisionTime, deployNormal);

   if(!hitObject)    // No appropriate walls found, can't deploy, sorry!
      return false;


   if(deployNormal.dot(ship->getAimVector()) > 0)
      deployNormal = -deployNormal;      // This is to fix deploy at wrong side of barrier.


   // Set deploy point, and move one unit away from the wall (this is a tiny amount, keeps linework from overlapping with wall)
   deployPosition.set(startPoint + (endPoint - startPoint) * collisionTime + deployNormal);

   return true;
}


// Check for sufficient energy and resources; return empty string if everything is ok
string EngineerModuleDeployer::checkResourcesAndEnergy(Ship *ship)
{
   if(!ship->isCarryingItem(ResourceItemTypeNumber))
      return "!!! Need resource item to use Engineer module";

   if(ship->getEnergy() < ship->getGame()->getModuleInfo(ModuleEngineer)->getPerUseCost())
      return "!!! Not enough energy to engineer an object";

   return "";
}


// Returns "" if location is OK, otherwise returns an error message
// Runs on client and server
bool EngineerModuleDeployer::canCreateObjectAtLocation(GridDatabase *database, Ship *ship, U32 objectType)
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

   if(!EngineeredItem::checkDeploymentPosition(bounds, database))
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

   // Now we can find the point where the forcefield would end if this were a valid position
   Point forceFieldEnd;
   DatabaseObject *collObj;
   ForceField::findForceFieldEnd(database, forceFieldStart, mDeployNormal, forceFieldEnd, &collObj);

   bool collision = false;

   // Check for collisions with existing projectors
   Rect queryRect(forceFieldStart, forceFieldEnd);
   queryRect.expand(Point(5,5));    // Just a touch bigger than the bare minimum

   Vector<Point> candidateForceFieldGeom;
   ForceField::getGeom(forceFieldStart, forceFieldEnd, candidateForceFieldGeom);

   fillVector.clear();
   database->findObjects(ForceFieldProjectorTypeNumber, fillVector, queryRect);

   Vector<Point> ffpGeom;     // Geom of any projectors we find

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      ForceFieldProjector *ffp = dynamic_cast<ForceFieldProjector *>(fillVector[i]);
      if(ffp)
      {
         ffpGeom.clear();
         ffp->getCollisionPoly(ffpGeom);
         if(forceFieldEdgesIntersectPoints(ffpGeom, candidateForceFieldGeom))
         {
            collision = true;
            break;
         }
      }
   }
   
   if(!collision)
   {
      // Check for collision with forcefields that could be projected from those projectors.
      // Projectors up to two forcefield lengths away must be considered because the end of 
      // one could intersect the end of the other.
      fillVector.clear();
      queryRect.expand(Point(ForceField::MAX_FORCEFIELD_LENGTH, ForceField::MAX_FORCEFIELD_LENGTH));
      database->findObjects(ForceFieldProjectorTypeNumber, fillVector, queryRect);

      // Reusable containers for holding geom of any forcefields we might need to check for intersection with our candidate
      Point start, end;
      Vector<Point> ffGeom;

      for(S32 i = 0; i < fillVector.size(); i++)
      {
         ForceFieldProjector *proj = dynamic_cast<ForceFieldProjector *>(fillVector[i]);
         if(proj)
         {
            proj->getForceFieldStartAndEndPoints(start, end);

            ffGeom.clear();
            ForceField::getGeom(start, end, ffGeom);

            if(forceFieldEdgesIntersectPoints(candidateForceFieldGeom, ffGeom))
            {
               collision = true;
               break;
            }
         }
      }
   }


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

   EngineeredItem *deployedObject = NULL;

   switch(objectType)
   {
      case EngineeredTurret:
         deployedObject = new Turret(ship->getTeam(), mDeployPosition, mDeployNormal);    // Deploy pos/norm calc'ed in canCreateObjectAtLocation()
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

   deployedObject->addToGame(gServerGame, gServerGame->getGameObjDatabase());
   deployedObject->onEnabled();

   MoveItem *resource = ship->unmountItem(ResourceItemTypeNumber);

   deployedObject->setResource(resource);

   return true;
}


// Constructor
EngineeredItem::EngineeredItem(S32 team, Point anchorPoint, Point anchorNormal) : mAnchorNormal(anchorNormal)
{
   setVert(anchorPoint, 0);
   mHealth = 1.0f;
   mTeam = team;
   mOriginalTeam = mTeam;
   mIsDestroyed = false;
   mHealRate = 0;
   mMountSeg = NULL;
   mSnapped = false;
}


bool EngineeredItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 3)
      return false;

   mTeam = atoi(argv[0]);
   mOriginalTeam = mTeam;
   if(mTeam == -1)      // Neutral object starts with no health, can be repaired and claimed by anyone
      mHealth = 0;
   
   Point pos;
   pos.read(argv + 1);
   pos *= game->getGridSize();

   if(argc >= 4)
   {
      mHealRate = atoi(argv[3]);
      mHealTimer.setPeriod(mHealRate * 1000);
   }

   // Find the mount point:
   Point normal, anchor;

   // Anchor objects to the correct point
   if(!findAnchorPointAndNormal(game->getGameObjDatabase(), pos, (F32)MAX_SNAP_DISTANCE, true, anchor, normal))
   {
      setVert(pos, 0);      // Found no mount point, but for editor, needs to set the position
      mAnchorNormal.set(1,0);
   }
   else
   {
      setVert(anchor + normal, 0);
      mAnchorNormal.set(normal);
   }
   computeExtent();

   return true;
}


void EngineeredItem::onAddedToGame(Game *game)
{
   Parent::onAddedToGame(game);

   if(mHealth != 0)
      onEnabled();
}


string EngineeredItem::toString(F32 gridSize) const
{
   return string(Object::getClassName()) + " " + itos(mTeam) + " " + geomToString(gridSize) + " " + itos(mHealRate);
}


// This is used for both positioning items in-game and for snapping them to walls in the editor --> static method
// Polulates anchor and normal

DatabaseObject *EngineeredItem::findAnchorPointAndNormal(GridDatabase *wallEdgeDatabase, const Point &pos, F32 snapDist, 
                                                           bool format, Point &anchor, Point &normal)
{
   return findAnchorPointAndNormal(wallEdgeDatabase, pos, snapDist, format, (TestFunc)isWallType, anchor, normal);
}


DatabaseObject *EngineeredItem::findAnchorPointAndNormal(GridDatabase *wallEdgeDatabase, const Point &pos, F32 snapDist, 
                                                           bool format, TestFunc testFunc, Point &anchor, Point &normal)
{
   F32 minDist = F32_MAX;
   DatabaseObject *closestWall = NULL;

   Point n;    // Reused in loop below
   F32 t;

   // Start with a sweep of the area
   for(F32 theta = 0; theta < Float2Pi; theta += FloatPi * 0.125f)    // Reducing to 0.0125 seems to have no effect
   {
      Point dir(cos(theta), sin(theta));
      dir *= snapDist;
      Point mountPos = pos - dir * 0.001f;                           // Offsetting slightly prevents spazzy behavior in editor
      
      // Look for walls
      DatabaseObject *wall = wallEdgeDatabase->findObjectLOS(testFunc, MoveObject::ActualState, format, mountPos, mountPos + dir, t, n);

      if(wall != NULL)     // Found one!
      {
         if(t < minDist)
         {
            anchor.set(mountPos + dir * t);
            normal.set(n);
            minDist = t;
            closestWall = wall;
         }
      }
   }

   return closestWall;
}


void EngineeredItem::setResource(MoveItem *resource)
{
   TNLAssert(resource->isMounted() == false, "Doh!");
   mResource = resource;
   mResource->removeFromDatabase();
}


static const F32 disabledLevel = 0.25;

bool EngineeredItem::isEnabled()
{
   return mHealth >= disabledLevel;
}


void EngineeredItem::damageObject(DamageInfo *di)
{
   F32 prevHealth = mHealth;

   if(di->damageAmount > 0)
      mHealth -= di->damageAmount * .25f; // ???
   else
      mHealth -= di->damageAmount;

   if(mHealth < 0)
      mHealth = 0;
   else if(mHealth > 1)
      mHealth = 1;

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

      mResource->addToDatabase(getGame()->getGameObjDatabase());
      mResource->setActualPos(getVert(0) + mAnchorNormal * mResource->getRadius());

      deleteObject(500);
   }
}


void EngineeredItem::computeExtent()
{
   Vector<Point> v;
   getCollisionPoly(v);

   setExtent(Rect(v));
}


void EngineeredItem::explode()
{
#ifndef ZAP_DEDICATED
   const S32 EXPLOSION_COLOR_COUNT = 12;

   static Color ExplosionColors[EXPLOSION_COLOR_COUNT] = {
      Colors::red,
      Color(0.9, 0.5, 0),
      Colors::white,
      Colors::yellow,
      Colors::red,
      Color(0.8, 1.0, 0),
      Colors::orange50,
      Colors::white,
      Colors::red,
      Color(0.9, 0.5, 0),
      Colors::white,
      Colors::yellow,
   };

   SoundSystem::playSoundEffect(SFXShipExplode, getActualPos(), Point());

   F32 a = TNL::Random::readF() * 0.4f + 0.5f;
   F32 b = TNL::Random::readF() * 0.2f + 0.9f;
   F32 c = TNL::Random::readF() * 0.15f + 0.125f;
   F32 d = TNL::Random::readF() * 0.2f + 0.9f;

   FXManager::emitExplosion(getActualPos(), 0.65f, ExplosionColors, EXPLOSION_COLOR_COUNT);
   FXManager::emitBurst(getActualPos(), Point(a,c) * 0.6f, Color(1,1,0.25), Colors::red);
   FXManager::emitBurst(getActualPos(), Point(b,d) * 0.6f, Colors::yellow, Colors::yellow);

   disableCollision();
#endif
}


// Make sure position looks good when player deploys item with Engineer module -- make sure we're not deploying on top of
// a wall or another engineered item
// static method
bool EngineeredItem::checkDeploymentPosition(const Vector<Point> &thisBounds, GridDatabase *gb)
{
   Vector<DatabaseObject *> foundObjects;
   Rect queryRect(thisBounds);
   gb->findObjects((TestFunc) isForceFieldCollideableType, foundObjects, queryRect);

   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      Vector<Point> foundObjectBounds;
      dynamic_cast<GameObject *>(foundObjects[i])->getCollisionPoly(foundObjectBounds);

      if(polygonsIntersect(thisBounds, foundObjectBounds))     // Do they intersect?
         return false;     // Bad location
   }
   return true;            // Good location
}


U32 EngineeredItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
   {
      stream->write(getVert(0).x);
      stream->write(getVert(0).y);
      stream->write(mAnchorNormal.x);
      stream->write(mAnchorNormal.y);
   }

   if(stream->writeFlag(updateMask & TeamMask))
      writeThisTeam(stream);

   if(stream->writeFlag(updateMask & HealthMask))
   {
      if(stream->writeFlag(isEnabled()))
         stream->writeFloat((mHealth - disabledLevel) / (1 - disabledLevel), 5);
      else
         stream->writeFloat(mHealth / disabledLevel, 5);

      stream->writeFlag(mIsDestroyed);
   }
   return 0;
}


void EngineeredItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;

   if(stream->readFlag())
   {
      Point pos;
      initial = true;
      stream->read(&pos.x);
      stream->read(&pos.y);
      stream->read(&mAnchorNormal.x);
      stream->read(&mAnchorNormal.y);
      setVert(pos, 0);
      computeExtent();
   }


   if(stream->readFlag())
      readThisTeam(stream);

   if(stream->readFlag())
   {
      if(stream->readFlag())
         mHealth = stream->readFloat(5) * (1 - disabledLevel) + disabledLevel; // enabled
      else
         mHealth = stream->readFloat(5) * (disabledLevel * 0.99f); // disabled, make sure (mHealth < disabledLevel)


      bool wasDestroyed = mIsDestroyed;
      mIsDestroyed = stream->readFlag();

      if(mIsDestroyed && !wasDestroyed && !initial)
         explode();
   }
}


void EngineeredItem::healObject(S32 time)
{
   if(mHealRate == 0 || mTeam == -1)      // Neutral items don't heal!
      return;

   F32 prevHealth = mHealth;

   if(mHealTimer.update(time))
   {
      mHealth += .1f;
      setMaskBits(HealthMask);

      if(mHealth >= 1)
         mHealth = 1;
      else
         mHealTimer.reset();

      if(prevHealth < disabledLevel && mHealth >= disabledLevel)
         onEnabled();
   }
}


// Find mount point or turret or forcefield closest to pos
Point EngineeredItem::mountToWall(const Point &pos, GridDatabase *wallEdgeDatabase, GridDatabase *wallSegmentDatabase)
{  
   Point anchor, nrml;

   DatabaseObject *mountEdge = NULL, *mountSeg = NULL;

   // First we snap to a wall edge -- this will ensure we don't end up attaching to an interior wall segment in the case of a wall intersection.
   // That will determine our location, but we also need to figure out which segment we're attaching to so that if that segment were to move,
   // we could update or item accordingly.  Unfortunately, there is no direct way to associate a WallEdge with a WallSegment, but we can do
   // it indirectly by snapping again, this time to a segment in our WallSegment database.  By using the snap point we found initially, that will
   // ensure the segment we find is associated with the edge found in the first pass.
   mountEdge = findAnchorPointAndNormal(wallEdgeDatabase, pos, 
                               (F32)EngineeredItem::MAX_SNAP_DISTANCE, false, (TestFunc)isWallType, anchor, nrml);

   if(mountEdge)
   {
      mountSeg = findAnchorPointAndNormal(wallSegmentDatabase, anchor,     // <== passing in anchor here (found above), not pos
                        (F32)EngineeredItem::MAX_SNAP_DISTANCE, false, (TestFunc)isWallType, anchor, nrml);
   }

   if(mountSeg)   // Found a segment we can mount to
   {
      setVert(anchor, 0);
      setAnchorNormal(nrml);
      setMountSegment(dynamic_cast<WallSegment *>(mountSeg));

      mSnapped = true;
      onGeomChanged();

      return anchor;
   }
   else           // No suitable segments found
   {
      mSnapped = false;
      onGeomChanged();

      return pos;
   }
}

////////////////////////////////////////
////////////////////////////////////////

// Returns true if we should use the in-game rendering, false if we should use iconified editor rendering
// needed? or delete?
static bool renderFull(F32 currentScale, bool snapped)
{
   return(snapped && currentScale > 70);
}


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(ForceFieldProjector);

// Constructor
ForceFieldProjector::ForceFieldProjector(S32 team, Point anchorPoint, Point anchorNormal) : Parent(team, anchorPoint, anchorNormal)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = ForceFieldProjectorTypeNumber;
   mRadius = 7;
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


Point ForceFieldProjector::getEditorSelectionOffset(F32 currentScale)
{
   return renderFull(currentScale, false) ? Point(0, .035 * 255) : Point(0,0);
}


// static method
void ForceFieldProjector::getGeom(const Point &anchor, const Point &normal, Vector<Point> &geom)      
{
   static const S32 PROJECTOR_HALF_WIDTH = 12;  // Half the width of base of the projector, along the wall

   Point cross(normal.y, -normal.x);
   cross.normalize((F32)PROJECTOR_HALF_WIDTH);

   geom.push_back(anchor + cross);
   geom.push_back(getForceFieldStartPoint(anchor, normal));
   geom.push_back(anchor - cross);
}


// Get the point where the forcefield actually starts, as it leaves the projector; i.e. the tip of the projector.  Static method.
Point ForceFieldProjector::getForceFieldStartPoint(const Point &anchor, const Point &normal, F32 scaleFact)
{
   static const S32 PROJECTOR_OFFSET = 15;      // Distance from wall to projector tip; thickness, if you will

   return Point(anchor.x + normal.x * PROJECTOR_OFFSET * scaleFact, 
                anchor.y + normal.y * PROJECTOR_OFFSET * scaleFact);
}


void ForceFieldProjector::getForceFieldStartAndEndPoints(Point &start, Point &end)
{
   start = getForceFieldStartPoint(getVert(0), mAnchorNormal);

   DatabaseObject *collObj;
   ForceField::findForceFieldEnd(getDatabase(), getForceFieldStartPoint(getVert(0), mAnchorNormal), mAnchorNormal, end, &collObj);
}


// Forcefield projector has been turned on some how; either at the beginning of a level, or via repairing, or deploying. 
// Runs on both client and server
void ForceFieldProjector::onEnabled()
{
   if(!isGhost())
   {
      Point start = getForceFieldStartPoint(getVert(0), mAnchorNormal);
      Point end;
      DatabaseObject *collObj;
   
      ForceField::findForceFieldEnd(getDatabase(), start, mAnchorNormal, end, &collObj);

      mField = new ForceField(mTeam, start, end);
      mField->addToGame(getGame(), getGame()->getGameObjDatabase());
   }
}


bool ForceFieldProjector::getCollisionPoly(Vector<Point> &polyPoints) const
{
   getGeom(getVert(0), mAnchorNormal, polyPoints);
   return true;
}


Vector<Point> ForceFieldProjector::getBufferForBotZone()
{
   Vector<Point> inputPoints, bufferedPoints;
   getCollisionPoly(inputPoints);

   if(isWoundClockwise(inputPoints))
      inputPoints.reverse();

   offsetPolygon(inputPoints, bufferedPoints, (F32)BotNavMeshZone::BufferRadius);

   return bufferedPoints;
}


void ForceFieldProjector::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);
}


void ForceFieldProjector::render()
{
   renderForceFieldProjector(getVert(0), mAnchorNormal, getGame()->getTeamColor(getTeam()), isEnabled());
}


void ForceFieldProjector::renderDock()
{
   renderSquareItem(getVert(0), getGame()->getTeamColor(mTeam), 1, &Colors::white, '>');
}


void ForceFieldProjector::renderEditor(F32 currentScale)
{
#ifndef ZAP_DEDICATED
   F32 scaleFact = 1;
   const Color *color = getGame()->getTeamColor(mTeam);

   if(mSnapped)
   {
      renderForceFieldProjector(getVert(0), mAnchorNormal, color, true);
      renderForceField(ForceFieldProjector::getForceFieldStartPoint(getVert(0), mAnchorNormal, scaleFact), 
                       forceFieldEnd, color, true, scaleFact);
   }
   else
      renderDock();
#endif
}


class EditorUserInterface;

// Determine on which segment forcefield lands -- only used in the editor, wraps ForceField::findForceFieldEnd()
void ForceFieldProjector::findForceFieldEnd()
{
   // Load the corner points of a maximum-length forcefield into geom
   
   DatabaseObject *collObj;

   F32 scale = 1;
   
   Point start = getForceFieldStartPoint(getVert(0), mAnchorNormal);

   // Pass in database containing WallSegments
   if(ForceField::findForceFieldEnd(getGame()->getWallSegmentManager()->getGridDatabase(), start, mAnchorNormal, forceFieldEnd, &collObj))
      setEndSegment(dynamic_cast<WallSegment *>(collObj));
   else
      setEndSegment(NULL);

   Vector<Point> geom;
   ForceField::getGeom(start, forceFieldEnd, geom, scale);    
   setExtent(Rect(geom));
}


void ForceFieldProjector::onGeomChanged()
{
   if(mSnapped)
      findForceFieldEnd();
}


// Lua methods

const char ForceFieldProjector::className[] = "ForceFieldProjector";      // Class name as it appears to Lua scripts

// Lua constructor
ForceFieldProjector::ForceFieldProjector(lua_State *L)
{
   // Do nothing
}


ForceFieldProjector *ForceFieldProjector::clone() const
{
   return new ForceFieldProjector(*this);
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

   // EngineeredItem methods
   method(ForceFieldProjector, getHealth),
   method(ForceFieldProjector, isActive),
   method(ForceFieldProjector, getAngle),

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
   mObjectTypeNumber = ForceFieldTypeNumber;
   mNetFlags.set(Ghostable);
}

bool ForceField::collide(GameObject *hitObject)
{
   if(!mFieldUp)
      return false;

   if( ! (isShipType(hitObject->getObjectTypeNumber()) ) )
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

   return polygonsIntersect(thisGeom, thatGeom);
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
      if(!findObjectLOS((TestFunc)isForceFieldDeactivatingType, MoveObject::ActualState, mStart, mEnd, t, n))
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
      writeThisTeam(stream);
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
      readThisTeam(stream);

      Rect extent(mStart, mEnd);
      extent.expand(Point(5,5));
      setExtent(extent);
   }
   bool wasUp = mFieldUp;
   mFieldUp = stream->readFlag();

   if(initial || (wasUp != mFieldUp))
      SoundSystem::playSoundEffect(mFieldUp ? SFXForceFieldUp : SFXForceFieldDown, mStart, Point());
}


// static
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
void ForceField::getGeom(Vector<Point> &geom) const
{
   getGeom(mStart, mEnd, geom);
}


// Pass in a database containing walls or wallsegments
bool ForceField::findForceFieldEnd(GridDatabase *db, const Point &start, const Point &normal,  
                                   Point &end, DatabaseObject **collObj)
{
   F32 time;
   Point n;

   end.set(start.x + normal.x * MAX_FORCEFIELD_LENGTH, start.y + normal.y * MAX_FORCEFIELD_LENGTH);

   *collObj = db->findObjectLOS((TestFunc)isWallType, MoveObject::ActualState, start, end, time, n);

   if(*collObj)
   {
      end.set(start + (end - start) * time); 
      return true;
   }

   return false;
}


bool ForceField::getCollisionPoly(Vector<Point> &points) const
{
   getGeom(points);
   return true;
}


void ForceField::render()
{
   Color c = getGame()->getTeamColor(mTeam);
   renderForceField(mStart, mEnd, c, mFieldUp);
}


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Turret);

// Constructor
Turret::Turret(S32 team, Point anchorPoint, Point anchorNormal) : EngineeredItem(team, anchorPoint, anchorNormal)
{
   mObjectTypeNumber = TurretTypeNumber;

   mWeaponFireType = WeaponTurret;
   mNetFlags.set(Ghostable);

   onGeomChanged();
}


Turret *Turret::clone() const
{

   return new Turret(*this);
}


Vector<Point> Turret::getBufferForBotZone()
{
   Vector<Point> inputPoints, bufferedPoints;
   getCollisionPoly(inputPoints);

   if (isWoundClockwise(inputPoints))
      inputPoints.reverse();

   offsetPolygon(inputPoints, bufferedPoints, (F32)BotNavMeshZone::BufferRadius);

   return bufferedPoints;
}


bool Turret::processArguments(S32 argc2, const char **argv2, Game *game)
{
   S32 argc1 = 0;
   const char *argv1[32];
   for(S32 i = 0; i < argc2; i++)
   {
      char firstChar = argv2[i][0];
      if((firstChar >= 'a' && firstChar <= 'z') || (firstChar >= 'A' && firstChar <= 'Z'))  // starts with a letter
      {
         if(!strncmp(argv2[i], "W=", 2))  // W= is in 015a
         {
            S32 w=0;
            while(w < WeaponCount && stricmp(gWeapons[w].name.getString(), &argv2[i][2]))
               w++;
            if(w < WeaponCount)
               mWeaponFireType = w;
            break;
         }
      }
      else
      {
         if(argc1 < 32)
         {
            argv1[argc1] = argv2[i];
            argc1++;
         }
      }
      
   }

   bool returnBool = EngineeredItem::processArguments(argc1, argv1, game);
   mCurrentAngle = mAnchorNormal.ATAN2();
   return returnBool;
}


string Turret::toString(F32 gridSize) const
{
   string out = EngineeredItem::toString(gridSize);
   if(mWeaponFireType != WeaponTurret)
      out = out + " W=" + gWeapons[mWeaponFireType].name.getString();
   return out;
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


bool Turret::getCollisionPoly(Vector<Point> &polyPoints) const
{
   getGeom(getVert(0), mAnchorNormal, polyPoints);
   return true;
}


void Turret::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);
   mCurrentAngle = mAnchorNormal.ATAN2();
}


void Turret::render()
{
   Color c = getGame()->getTeamColor(mTeam);

   renderTurret(c, getVert(0), mAnchorNormal, isEnabled(), mHealth, mCurrentAngle);
}


void Turret::renderDock()
{
   renderSquareItem(getVert(0), getGame()->getTeamColor(mTeam), 1, &Colors::white, 'T');
}


void Turret::renderEditor(F32 currentScale)
{
   if(mSnapped)
      render();
   else
      renderDock();
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
   Point aimPos = getVert(0) + mAnchorNormal * TURRET_OFFSET;
   Point cross(mAnchorNormal.y, -mAnchorNormal.x);

   Rect queryRect(aimPos, aimPos);
   queryRect.unionPoint(aimPos + cross * TurretPerceptionDistance);
   queryRect.unionPoint(aimPos - cross * TurretPerceptionDistance);
   queryRect.unionPoint(aimPos + mAnchorNormal * TurretPerceptionDistance);
   fillVector.clear();
   findObjects((TestFunc)isTurretTargetType, fillVector, queryRect);    // Get all potential targets

   GameObject *bestTarget = NULL;
   F32 bestRange = F32_MAX;
   Point bestDelta;

   Point delta;

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      if(isShipType(fillVector[i]->getObjectTypeNumber()))
      {
         Ship *potential = dynamic_cast<Ship *>(fillVector[i]);

         // Is it dead or cloaked?  Carrying objects makes ship visible, except in nexus game
         TNLAssert(gServerGame->getGameType(), "Bad GameType!");
         if(!potential->isVisible() || potential->hasExploded)
            continue;
      }

      // Don't target mounted items (like resourceItems and flagItems)
      MoveItem *item = dynamic_cast<MoveItem *>(fillVector[i]);
      if(item && item->isMounted())
         continue;

      GameObject *potential = dynamic_cast<GameObject *>(fillVector[i]);
      if(potential->getTeam() == mTeam)      // Is target on our team?
         continue;                           // ...if so, skip it!

      // Calculate where we have to shoot to hit this...
      Point Vs = potential->getActualVel();
      F32 S = (F32)gWeapons[mWeaponFireType].projVelocity;
      Point d = potential->getRenderPos() - aimPos;

// This could possibly be combined with LuaRobot's getFiringSolution, as it's essentially the same thing
      F32 t;      // t is set in next statement
      if(!FindLowestRootInInterval(Vs.dot(Vs) - S * S, 2 * Vs.dot(d), d.dot(d), gWeapons[mWeaponFireType].projLiveTime * 0.001f, t))
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
      if(findObjectLOS((TestFunc)isWallType, MoveObject::ActualState, aimPos, potential->getActualPos(), t, n))
         continue;

      // See if we're gonna clobber our own stuff...
      disableCollision();
      Point delta2 = delta;
      delta2.normalize(gWeapons[mWeaponFireType].projLiveTime * (F32)gWeapons[mWeaponFireType].projVelocity / 1000.f);
      GameObject *hitObject = findObjectLOS((TestFunc) isWithHealthType, 0, aimPos, aimPos + delta2, t, n);
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
         string killer = string("got blasted by ") + getGame()->getTeamName(mTeam).getString() + " turret";
         mKillString = killer.c_str();

         createWeaponProjectiles(WeaponType(mWeaponFireType), bestDelta, aimPos, velocity, mWeaponFireType == WeaponBurst ? 45.f : 35.f, this);
         mFireTimer.reset(gWeapons[mWeaponFireType].fireDelay);
      }
   }
}


// For turrets, apparent selection center is not the same as the item's actual location
Point Turret::getEditorSelectionOffset(F32 currentScale)
{
   return renderFull(currentScale, false) ? Point(0, .075 * 255) : Point(0,0);
}


void Turret::onGeomChanged() 
{ 
   mCurrentAngle = mAnchorNormal.ATAN2();       // Keep turret pointed away from the wall... looks better like that!
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

   // EngineeredItem methods
   method(Turret, getHealth),
   method(Turret, isActive),
   method(Turret, getAngle),

   // Turret Methods
   method(Turret, getAngleAim),

   {0,0}    // End method list
};

};

