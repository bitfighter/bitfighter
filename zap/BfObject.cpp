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

#include "BfObject.h"
#include "moveObject.h"          // For ActualState definition
#include "gameType.h"
#include "ship.h"
#include "GeomUtils.h"
#include "game.h"
#include "gameConnection.h"
#include "ClientInfo.h"

#include "gameObjectRender.h"    // For drawSquare

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "OpenglUtils.h"
#endif

#include "projectile.h"
#include "PickupItem.h"          // For getItem function below
#include "soccerGame.h"          // For getItem function below

#include "luaObject.h"           // For LuaObject def and returnInt method
#include "lua.h"                 // For push prototype

#include "tnlBitStream.h"

#include <math.h>

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
         x == MineTypeNumber || x == SpyBugTypeNumber || x == BulletTypeNumber ||
         x == BurstTypeNumber || x == HeatSeekerTypeNumber;
}

bool isGrenadeType(U8 x)
{
   return
         x == MineTypeNumber || x == SpyBugTypeNumber || x == BurstTypeNumber;
}

// If we add something here that is not an Item, need to check where this is used to make sure everything is ok
bool isWithHealthType(U8 x)      
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber           ||
         x == TurretTypeNumber     || x == ForceFieldProjectorTypeNumber ||
         x == CoreTypeNumber;
}

bool isForceFieldDeactivatingType(U8 x)
{
   return
         x == MineTypeNumber         || x == SpyBugTypeNumber         ||
         x == FlagTypeNumber         || x == SoccerBallItemTypeNumber ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber       ||
         x == EnergyItemTypeNumber   || x == RepairItemTypeNumber     ||
         x == PlayerShipTypeNumber   || x == RobotShipTypeNumber      || 
         x == AsteroidTypeNumber;
}

bool isDamageableType(U8 x)
{
   return
         x == PlayerShipTypeNumber   || x == RobotShipTypeNumber           || x == BurstTypeNumber    ||
         x == BulletTypeNumber       || x == MineTypeNumber                || x == SpyBugTypeNumber   ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber            || x == AsteroidTypeNumber ||
         x == TurretTypeNumber       || x == ForceFieldProjectorTypeNumber || x == CoreTypeNumber     ||
         x == FlagTypeNumber         || x == SoccerBallItemTypeNumber      || x == CircleTypeNumber   ||
         x == TeleporterTypeNumber   || x == HeatSeekerTypeNumber          || x == WormTypeNumber;
}


bool isMotionTriggerType(U8 x)
{
   return
         x == PlayerShipTypeNumber   || x == RobotShipTypeNumber ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber  || 
         x == AsteroidTypeNumber     || x == MineTypeNumber;
}


bool isTurretTargetType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber       || x == ResourceItemTypeNumber ||
         x == TestItemTypeNumber   || x == SoccerBallItemTypeNumber;
}


bool isCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber   ||
         x == TurretTypeNumber  || x == ForceFieldTypeNumber ||
         x == CoreTypeNumber    || x == ForceFieldProjectorTypeNumber ||
         x == WormTypeNumber;
}


bool isForceFieldCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == TurretTypeNumber  || x == ForceFieldProjectorTypeNumber;
}


bool isWallType(U8 x)
{
   return
         x == BarrierTypeNumber  || x == PolyWallTypeNumber ||
         x == WallItemTypeNumber || x == WallEdgeTypeNumber || x == WallSegmentTypeNumber;
}


bool isWallItemType(U8 x)
{
   return x == WallItemTypeNumber;
}


bool isLineItemType(U8 x)
{
   return
         x == BarrierTypeNumber || x == WallItemTypeNumber || x == LineTypeNumber;
}


bool isWeaponCollideableType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber      || x == BurstTypeNumber               ||
         x == SpyBugTypeNumber     || x == MineTypeNumber           || x == BulletTypeNumber              ||
         x == FlagTypeNumber       || x == SoccerBallItemTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == AsteroidTypeNumber   || x == TestItemTypeNumber       || x == ResourceItemTypeNumber        ||
         x == TurretTypeNumber     || x == CircleTypeNumber         || x == CoreTypeNumber                ||
         x == BarrierTypeNumber    || x == PolyWallTypeNumber       || x == ForceFieldTypeNumber          ||
         x == TeleporterTypeNumber || x == HeatSeekerTypeNumber     || x == WormTypeNumber;
}

bool isAsteroidCollideableType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber           ||
         x == TestItemTypeNumber   || x == ResourceItemTypeNumber        ||
         x == TurretTypeNumber     || x == ForceFieldProjectorTypeNumber ||
         x == BarrierTypeNumber    || x == PolyWallTypeNumber            ||
         x == ForceFieldTypeNumber || x == CoreTypeNumber;
}

bool isFlagCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber   || x == ForceFieldTypeNumber || x == PolyWallTypeNumber;
}

bool isFlagOrShipCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber    || x == PolyWallTypeNumber    || ForceFieldTypeNumber ||
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber;
}

bool isVisibleOnCmdrsMapType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber      || x == CoreTypeNumber                ||
         x == BarrierTypeNumber    || x == PolyWallTypeNumber       || x == TextItemTypeNumber            ||
         x == TurretTypeNumber     || x == ForceFieldTypeNumber     || x == ForceFieldProjectorTypeNumber ||
         x == FlagTypeNumber       || x == SoccerBallItemTypeNumber || x == LineTypeNumber                ||
         x == GoalZoneTypeNumber   || x == NexusTypeNumber          || x == LoadoutZoneTypeNumber         || 
         x == SpeedZoneTypeNumber  || x == TeleporterTypeNumber     || x == SlipZoneTypeNumber            ||
         x == AsteroidTypeNumber   || x == TestItemTypeNumber       || x == ResourceItemTypeNumber        ||
         x == EnergyItemTypeNumber || x == RepairItemTypeNumber; 
}

bool isVisibleOnCmdrsMapWithSensorType(U8 x)     // Weapons visible on commander's map for sensor
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber      || x == ResourceItemTypeNumber        ||
         x == BarrierTypeNumber    || x == PolyWallTypeNumber       || x == LoadoutZoneTypeNumber         || 
         x == TurretTypeNumber     || x == ForceFieldTypeNumber     || x == ForceFieldProjectorTypeNumber ||
         x == FlagTypeNumber       || x == SoccerBallItemTypeNumber || x == SlipZoneTypeNumber            ||
         x == GoalZoneTypeNumber   || x == NexusTypeNumber          || x == CoreTypeNumber                ||
         x == SpeedZoneTypeNumber  || x == TeleporterTypeNumber     || x == BurstTypeNumber               ||
         x == LineTypeNumber       || x == TextItemTypeNumber       || x == RepairItemTypeNumber          ||
         x == AsteroidTypeNumber   || x == TestItemTypeNumber       || x == EnergyItemTypeNumber          ||
         x == BulletTypeNumber     || x == MineTypeNumber           || x == HeatSeekerTypeNumber;
}


bool isZoneType(U8 x)      // Zones a ship could be in
{
   return 
         x == NexusTypeNumber || x == GoalZoneTypeNumber || x == LoadoutZoneTypeNumber ||
         x == ZoneTypeNumber  || x == SlipZoneTypeNumber; 
}


bool isHeatSeekerTarget(U8 x)
{
   return isShipType(x);
}


bool isAnyObjectType(U8 x)
{
   return true;
}

////////////////////////////////////////
////////////////////////////////////////


// Constructor
DamageInfo::DamageInfo()
{
   damageSelfMultiplier = 1;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
EditorObject::EditorObject()
{
   mLitUp = false; 
   mSelected = false; 
}


void EditorObject::onItemDragging()  { /* Do nothing */ }
void EditorObject::onAttrsChanging() { /* Do nothing */ }
void EditorObject::onAttrsChanged()  { /* Do nothing */ }


const char *EditorObject::getEditorHelpString()
{
   TNLAssert(false, "getEditorHelpString method not implemented!");
   return "getEditorHelpString method not implemented!";  // better then a NULL crash in non-debug mode or continuing past the Assert
}


const char *EditorObject::getPrettyNamePlural()
{
   TNLAssert(false, "getPrettyNamePlural method not implemented!");
   return "getPrettyNamePlural method not implemented!";
}


const char *EditorObject::getOnDockName()
{
   TNLAssert(false, "getOnDockName method not implemented!");
   return "getOnDockName method not implemented!";
}


const char *EditorObject::getOnScreenName()
{
   TNLAssert(false, "getOnScreenName method not implemented!");
   return "getOnScreenName method not implemented!";
}


// Not all editor objects will implement this
const char *EditorObject::getInstructionMsg()
{
   return "";
}


string EditorObject::getAttributeString()
{
   return "";
}


S32 EditorObject::getDockRadius()
{
   return 10;
}



bool EditorObject::isSelected()
{
   return mSelected;
}


void EditorObject::setSelected(bool selected)
{
   mSelected = selected;
}


bool EditorObject::isLitUp() 
{ 
   return mLitUp; 
}


void EditorObject::setLitUp(bool litUp) 
{ 
   mLitUp = litUp; 

   if(!litUp) 
      setVertexLitUp(NONE); 
}


bool EditorObject::isVertexLitUp(S32 vertexIndex)
{
   return mVertexLitUp == vertexIndex;
}


void EditorObject::setVertexLitUp(S32 vertexIndex)
{
   mVertexLitUp = vertexIndex;
}


// Size of object in editor 
F32 EditorObject::getEditorRadius(F32 currentScale)
{
   return 10 * currentScale;   // 10 pixels is base size
}


// User assigned id, if any
S32 EditorObject::getUserDefinedItemId()
{
   return mUserDefinedItemId;
}


void EditorObject::setUserDefinedItemId(S32 itemId)
{
   mUserDefinedItemId = itemId;
}



////////////////////////////////////////
////////////////////////////////////////

// BfObject - the declarations are in GameObject.h

// Constructor
BfObject::BfObject()
{
   mGame = NULL;
   mObjectTypeNumber = UnknownTypeNumber;

   assignNewSerialNumber();

   mTeam = -1;
   mDisableCollisionCount = 0;
   mCreationTime = 0;

   mOwner = NULL;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
BfObject::~BfObject()
{
   removeFromGame();
   LUAW_DESTRUCTOR_CLEANUP;
}


// Serial numbers are used in a couple of ways: in the editor, they are used to identify same objects in different databases,
// for example to identify objects across undo/redo states.  They are also used by walls to help identify which segments belong
// to which wall, even as walls are being moved around, and wall edits are undone/redone.
void BfObject::assignNewSerialNumber()
{
   static S32 mNextSerialNumber = 0;

   mSerialNumber = mNextSerialNumber++;
}


S32 BfObject::getSerialNumber()
{
   return mSerialNumber;
}



S32 BfObject::getTeam() const
{
   return mTeam;
}


void BfObject::setTeam(S32 team)
{
   mTeam = team;
   setMaskBits(TeamMask);
}


const Color *BfObject::getColor() const
{ 
   return mGame->getTeamColor(mTeam);
}


Game *BfObject::getGame() const
{
   return mGame;
}


bool BfObject::hasTeam()
{
   return true;
}


bool BfObject::canBeNeutral()
{
   return true;
}


bool BfObject::canBeHostile()
{
   return true;
}


void BfObject::addToGame(Game *game, GridDatabase *database)
{   
   TNLAssert(mGame == NULL, "Error: Object already in a game in BfObject::addToGame.");
   TNLAssert(game != NULL,  "Error: theGame is NULL in BfObject::addToGame.");

   mGame = game;
   if(database)
      addToDatabase(database);

   setCreationTime(game->getCurrentTime());
   onAddedToGame(game);
}


void BfObject::removeFromGame()
{
   removeFromDatabase();
   mGame = NULL;
}


bool BfObject::processArguments(S32 argc, const char**argv, Game *game)
{
   return true;
}


void BfObject::onPointsChanged()                        
{   
   GeomObject::onPointsChanged();
   updateExtentInDatabase();      
}


void BfObject::updateExtentInDatabase()
{
   setExtent(calcExtents());    // Make sure the database extents are in sync with where the object actually is
}


void BfObject::unselect()
{
   setSelected(false);
   setLitUp(false);

   unselectVerts();
}


void BfObject::onGeomChanged()
{
   GeomObject::onGeomChanged();
   updateExtentInDatabase();
}



#ifndef ZAP_DEDICATED
void BfObject::prepareForDock(ClientGame *game, const Point &point, S32 teamIndex)
{
   mGame = game;

   unselectVerts();
   setTeam(teamIndex);
}

#endif


#ifndef ZAP_DEDICATED
// Render selected and highlighted vertices, called from renderEditor
void BfObject::renderAndLabelHighlightedVertices(F32 currentScale)
{
   F32 radius = getEditorRadius(currentScale);

   // Label and highlight any selected or lit up vertices.  This will also highlight point items.
   for(S32 i = 0; i < getVertCount(); i++)
      if(vertSelected(i) || isVertexLitUp(i) || ((isSelected() || isLitUp())  && getVertCount() == 1))
      {
         glColor((vertSelected(i) || (isSelected() && getGeomType() == geomPoint)) ? SELECT_COLOR : HIGHLIGHT_COLOR);

         Point center = getVert(i) + getEditorSelectionOffset(currentScale);

         drawSquare(center, radius / currentScale);
      }
}
#endif


Point BfObject::getDockLabelPos()
{
   static const Point labelOffset(0, 11);

   return getPos() + labelOffset;
}


void BfObject::highlightDockItem()
{
#ifndef ZAP_DEDICATED
   glColor(HIGHLIGHT_COLOR);
   drawSquare(getPos(), getDockRadius());
#endif
}


void BfObject::initializeEditor()
{
   unselectVerts();
}


string BfObject::toString(F32) const
{
   TNLAssert(false, "This object not be serialized");
   return "";
}


// Return a pointer to a new copy of the object.  This is more like a duplicate or twin of the object -- it has the same
// serial number, and is already assigned to a game.
// You will have to delete this copy when you are done with it!
BfObject *BfObject::copy()
{
   BfObject *newObject = clone();     
   newObject->initializeEditor();         // Marks all vertices as unselected

   return newObject;
}


// Return a pointer to a new copy of the object.  This copy will be completely new -- new serial number, mGame set to NULL, everything.
// You will have to delete this copy when you are done with it!
BfObject *BfObject::newCopy()
{
   BfObject *newObject = copy();
   newObject->mGame = NULL;

   newObject->assignNewSerialNumber();    // Give this object an identity of its own

   return newObject;
}


BfObject *BfObject::clone() const
{
   TNLAssert(false, "Clone method not implemented!");
   return NULL;
}


void BfObject::setSnapped(bool snapped)
{
   // Do nothing
}


// Called when item dragged from dock to editor -- overridden by several objects
void BfObject::newObjectFromDock(F32 gridSize) 
{  
   assignNewSerialNumber();

   updateExtentInDatabase();
   mGame = NULL;
}   


Point BfObject::getEditorSelectionOffset(F32 scale)
{
   return Point(0,0);     // No offset for most items
}


Point BfObject::getInitialPlacementOffset(F32 gridSize)
{
   return Point(0, 0);
}


void BfObject::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   TNLAssert(false, "renderEditor not implemented!");
}


void BfObject::renderDock()
{
   TNLAssert(false, "renderDock not implemented!");
}


// For editing attributes -- all implementation will need to be provided by the children
EditorAttributeMenuUI *BfObject::getAttributeMenu()                                      { return NULL; }
void                   BfObject::startEditingAttrs(EditorAttributeMenuUI *attributeMenu) { /* Do nothing */ }
void                   BfObject::doneEditingAttrs(EditorAttributeMenuUI *attributeMenu)  { /* Do nothing */ }


bool BfObject::controllingClientIsValid()
{
   return mControllingClient.isValid();
}


SafePtr<GameConnection> BfObject::getControllingClient()
{
   return mControllingClient;
}


void BfObject::setControllingClient(GameConnection *c)         // This only gets run on the server
{
   mControllingClient = c;
}


void BfObject::setOwner(ClientInfo *clientInfo)
{
   mOwner = clientInfo;
}


ClientInfo *BfObject::getOwner()
{
   return mOwner;
}


void BfObject::deleteObject(U32 deleteTimeInterval)
{
   mObjectTypeNumber = DeletedTypeNumber;

   if(!mGame)                    // Not in a game
      delete this;
   else
      mGame->addToDeleteList(this, deleteTimeInterval);
}


void BfObject::setUserAssignedId(U32 id)
{
   mUserAssignedId = id;
}


U32 BfObject::getUserAssignedId()
{
   return mUserAssignedId;
}


void BfObject::setScopeAlways()
{
   getGame()->setScopeAlwaysObject(this);
}


F32 BfObject::getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips)
{
   BfObject *so = dynamic_cast<BfObject *>(scopeObject);
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

      Point deltav = getVel() - so->getVel();


      // initial scoping factor is distance based.
      add += (500 - distance) / 500;

      // give some extra love to things that are moving towards the scope object
      if(deltav.dot(deltap) < 0)
         add += 0.7f;
   }

   // and a little more love if this object has not yet been scoped.
   if(updateMask == 0xFFFFFFFF)
      add += 2.5;
   return add + updateSkips * 0.2f;
}


void BfObject::damageObject(DamageInfo *theInfo)
{
   // Do nothing
}


bool BfObject::collide(BfObject *hitObject)
{
   return false;
}


Vector<Point> BfObject::getRepairLocations(const Point &repairOrigin)
{
   Vector<Point> repairLocations;
   repairLocations.push_back(getPos());

   return repairLocations;
}


// This method returns true if the specified object collides with the given ray designated by
// rayStart and rayEnd
bool BfObject::objectIntersectsSegment(BfObject *object, const Point &rayStart, const Point &rayEnd,
      F32 &fillCollisionTime)
{
   F32 collisionTime = 1.f;

   Point targetLocation;
   F32 targetRadius;
   static Vector<Point> fillPolygon;
   fillPolygon.clear();

   // If our target has a collision circle
   if(object->getCollisionCircle(ActualState, targetLocation, targetRadius))
   {
      if(circleIntersectsSegment(targetLocation, targetRadius, rayStart, rayEnd, fillCollisionTime))
      {
         // If we're super close, we've hit!
         if(fillCollisionTime < collisionTime)
            return true;
      }
   }
   // Our target has a collision polygon
   else if(object->getCollisionPoly(fillPolygon))
   {
      if(fillPolygon.size() == 0)
        return false;

      Point normal;
      if(polygonIntersectsSegmentDetailed(&fillPolygon[0], fillPolygon.size(), true, rayStart, rayEnd, fillCollisionTime, normal))
      {
         if(fillCollisionTime < collisionTime)
            return true;
      }
   }

   return false;
}


// Returns number of ships hit
S32 BfObject::radiusDamage(Point pos, S32 innerRad, S32 outerRad, TestFunc objectTypeTest, DamageInfo &info, F32 force)
{
   GameType *gameType = getGame()->getGameType();

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
      BfObject *foundObject = static_cast<BfObject *>(fillVector[i]);

      // No object damages itself
      if(foundObject == info.damagingObject)
         continue;

      // Check the actual distance against our outer radius.  Recall that we got a list of potential
      // collision objects based on a square area, but actual collisions will be based on true distance
      Point objPos = foundObject->getPos();
      Point delta = objPos - pos;

      if(delta.len() > outerRad)
         continue;

      // Can one damage another?
      if(gameType && !gameType->objectCanDamageObject(info.damagingObject, foundObject))
         continue;

      // Do an LOS check...
      F32 t;
      Point n;

      if(findObjectLOS((TestFunc)isWallType, ActualState, pos, objPos, t, n))
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
      ClientInfo *damagerOwner = info.damagingObject->getOwner();
      ClientInfo *victimOwner = foundObject->getOwner();

      if(victimOwner && damagerOwner == victimOwner)
         localInfo.damageAmount *= localInfo.damageSelfMultiplier;

      if(isShipType(foundObject->getObjectTypeNumber()))
         shipsHit++;

      foundObject->damageObject(&localInfo); 
   }

   return shipsHit;
}


void BfObject::findObjects(TestFunc objectTypeTest, Vector<DatabaseObject *> &fillVector, const Rect &ext)
{
   GridDatabase *gridDB = getDatabase();

   if(gridDB)
      gridDB->findObjects(objectTypeTest, fillVector, ext);
}


void BfObject::findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect &ext)
{
   GridDatabase *gridDB = getDatabase();

   if(gridDB)
      gridDB->findObjects(typeNumber, fillVector, ext);
}


BfObject *BfObject::findObjectLOS(U8 typeNumber, U32 stateIndex, Point rayStart, Point rayEnd,
                                      float &collisionTime, Point &collisionNormal)
{
   GridDatabase *gridDB = getDatabase();

   if(gridDB)
     return static_cast<BfObject *>(
         gridDB->findObjectLOS(typeNumber, stateIndex, rayStart, rayEnd, collisionTime, collisionNormal)
         );

   return NULL;
}


BfObject *BfObject::findObjectLOS(TestFunc objectTypeTest, U32 stateIndex, Point rayStart, Point rayEnd,
                                      float &collisionTime, Point &collisionNormal)
{
   GridDatabase *gridDB = getDatabase();

   if(gridDB)
     return static_cast<BfObject *>(
         gridDB->findObjectLOS(objectTypeTest, stateIndex, rayStart, rayEnd, collisionTime, collisionNormal)
         );

   return NULL;
}


void BfObject::onAddedToGame(Game *)
{
   getGame()->mObjectsLoaded++;
}


void BfObject::markAsGhost()
{
   mNetFlags = NetObject::IsGhost;
}


bool BfObject::isMoveObject()
{
   return false;
}


Point BfObject::getVel() const
{
   return Point(0,0);
}


U32 BfObject::getCreationTime()
{
   return mCreationTime;
}


void BfObject::setCreationTime(U32 creationTime)
{
   mCreationTime = creationTime;
}


StringTableEntry BfObject::getKillString()
{
   return mKillString;
}


S32 BfObject::getRenderSortValue()
{
   return 2;
}


const Move &BfObject::getCurrentMove()
{
   return mCurrentMove;
}


const Move &BfObject::getLastMove()
{
   return mLastMove;
}


void BfObject::setCurrentMove(const Move &theMove)
{
   mCurrentMove = theMove;
}


void BfObject::setLastMove(const Move &theMove)
{
   mLastMove = theMove;
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


void BfObject::disableCollision()
{
   TNLAssert(mDisableCollisionCount < 10, "Too many disabled collisions");
   mDisableCollisionCount++;
}


void BfObject::enableCollision()
{
   TNLAssert(mDisableCollisionCount != 0, "Trying to enable collision, already enabled");
   mDisableCollisionCount--;
}


bool BfObject::isCollisionEnabled()
{
   return mDisableCollisionCount == 0;
}

//
//// Find if the specified point is in theObject's collisionPoly or collisonCircle
//bool BfObject::collisionPolyPointIntersect(Point point)
//{
//   Point center;
//   F32 radius;
//   Rect rect;
//
//   Vector<Point> polyPoints;
//
//   polyPoints.clear();
//
//   if(getCollisionPoly(polyPoints))
//      return PolygonContains2(polyPoints.address(), polyPoints.size(), point);
//
//   else if(getCollisionCircle(MoveObject::ActualState, center, radius))
//      return(center.distanceTo(point) <= radius);
//
//   else
//      return false;
//}
//
//
//// Find if the specified polygon intersects theObject's collisionPoly or collisonCircle
//bool BfObject::collisionPolyPointIntersect(Vector<Point> points)
//{
//   Point center;
//   Rect rect;
//   F32 radius;
//   Vector<Point> polyPoints;
//
//   polyPoints.clear();
//
//   if(getCollisionPoly(polyPoints))
//      return polygonsIntersect(polyPoints, points);
//
//   else if(getCollisionCircle(MoveObject::ActualState, center, radius))
//   {
//      Point unusedPt;
//      return polygonCircleIntersect(&points[0], points.size(), center, radius * radius, unusedPt);
//   }
//
//   else
//      return false;
//}
//

// Find if the specified polygon intersects theObject's collisionPoly or collisonCircle
bool BfObject::collisionPolyPointIntersect(Point center, F32 radius)
{
   Point c, pt;
   float r;
   Rect rect;
   static Vector<Point> polyPoints;

   polyPoints.clear();

   if(getCollisionPoly(polyPoints))
      return polygonCircleIntersect(&polyPoints[0], polyPoints.size(), center, radius * radius, pt);

   else if(getCollisionCircle(ActualState, c, r))
      return ( center.distSquared(c) < (radius + r) * (radius + r) );

   else
      return false;
}


F32 BfObject::getHealth()
{
   return 1;
}


bool BfObject::isDestroyed()
{
   return false;
}


void BfObject::idle(IdleCallPath path)
{
   // Do nothing
}


void BfObject::writeControlState(BitStream *)
{
   // Do nothing
}


void BfObject::readControlState(BitStream *)
{
   // Do nothing
}


void BfObject::controlMoveReplayComplete()
{
   // Do nothing
}


void BfObject::writeCompressedVelocity(const Point &vel, U32 max, BitStream *stream)
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


void BfObject::readCompressedVelocity(Point &vel, U32 max, BitStream *stream)
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


void BfObject::onGhostAddBeforeUpdate(GhostConnection *theConnection)
{
#ifndef ZAP_DEDICATED
   // Some unpackUpdate need getGame() available.
   GameConnection *gc = (GameConnection *)(theConnection);  // GhostConnection is always GameConnection
   TNLAssert(theConnection && gc->getClientGame(), "Should only be client here!");
   mGame = gc->getClientGame();
#endif
}

bool BfObject::onGhostAdd(GhostConnection *theConnection)
{
#ifndef ZAP_DEDICATED
   GameConnection *gc = (GameConnection *)(theConnection);  // GhostConnection is always GameConnection
   TNLAssert(theConnection && gc->getClientGame(), "Should only be client here!");

#ifdef TNL_ENABLE_ASSERTS
   mGame = NULL;  // prevent false asserts
#endif

   // for performance, add to GridDatabase after update, to avoid slowdown from adding to database with zero points or (0,0) then moving
   addToGame(gc->getClientGame(), gc->getClientGame()->getGameObjDatabase());
#endif
   return true;
}


void BfObject::readThisTeam(BitStream *stream)
{
   mTeam = stream->readInt(4) - 2;
}


void BfObject::writeThisTeam(BitStream *stream)
{
   stream->writeInt(mTeam + 2, 4);
}


/////
// Lua interface

//               Fn name         Param profiles     Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getClassID,     ARRAYDEF({{            END }}), 1 ) \
   METHOD(CLASS, getLoc,         ARRAYDEF({{            END }}), 1 ) \
   METHOD(CLASS, setLoc,         ARRAYDEF({{ PT,        END }}), 1 ) \
   METHOD(CLASS, getTeamIndx,    ARRAYDEF({{            END }}), 1 ) \
   METHOD(CLASS, setTeam,        ARRAYDEF({{ TEAM_INDX, END }}), 1 ) \
   METHOD(CLASS, addToGame,      ARRAYDEF({{            END }}), 1 ) \
   METHOD(CLASS, removeFromGame, ARRAYDEF({{            END }}), 1 ) \
   METHOD(CLASS, setGeom,        ARRAYDEF({{ PTS,       END }}), 1 ) \
   METHOD(CLASS, getGeom,        ARRAYDEF({{            END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(BfObject, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(BfObject, LUA_METHODS);

#undef LUA_METHODS


const char *BfObject::luaClassName = "BfItem";
REGISTER_LUA_CLASS(BfObject);


S32 BfObject::getClassID(lua_State *L)  { return returnInt  (L, mObjectTypeNumber); }
S32 BfObject::getLoc(lua_State *L)      { return returnPoint(L, getPos());          }
S32 BfObject::getTeamIndx(lua_State *L) { return returnInt  (L, mTeam + 1);         }  // + 1 because Lua indices start at 1


S32 BfObject::setTeam(lua_State *L) 
{ 
   checkArgList(L, functionArgs, "BfObject", "setTeam");

   setTeam(getInt(L, 1) - 1);    // - 1 because Lua indices start at 1  
   
   return 0;            
}  


S32 BfObject::setLoc(lua_State *L)
{
   checkArgList(L, functionArgs, "BfObject", "setLoc");
   setPos(getPointOrXY(L, 1));
   return returnNil(L);
}


S32 BfObject::addToGame(lua_State *L)
{
   addToGame(gServerGame, gServerGame->getGameObjDatabase());
   return returnNil(L);
}


S32 BfObject::removeFromGame(lua_State *L)
{
   deleteObject();
   return returnNil(L);
}


S32 BfObject::setGeom(lua_State *L)
{
   checkArgList(L, functionArgs, "BfObject", "setGeom");

   Vector<Point> points = getPointsOrXYs(L, 1);

   GeomObject::setGeom(points);
   updateExtentInDatabase();

   setMaskBits(GeomMask);

   return 0;
}


S32 BfObject::getGeom(lua_State *L)
{
   return 0;      // TODO: return geometry!
}

};

