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

#include "gameObject.h"
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
#endif

#include "projectile.h"

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
         x == MineTypeNumber || x == SpyBugTypeNumber || x == BulletTypeNumber;
}

bool isGrenadeType(U8 x)
{
   return
         x == MineTypeNumber || x == SpyBugTypeNumber;
}

bool isWithHealthType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == TurretTypeNumber     || x == ForceFieldProjectorTypeNumber ||
         x == CoreTypeNumber;
}

bool isForceFieldDeactivatingType(U8 x)
{
   return
         x == MineTypeNumber         || x == SpyBugTypeNumber ||
         x == FlagTypeNumber         || x == SoccerBallItemTypeNumber ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber || x == AsteroidTypeNumber ||
         x == EnergyItemTypeNumber   || x == RepairItemTypeNumber ||
         x == PlayerShipTypeNumber   || x == RobotShipTypeNumber;
}

bool isDamageableType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == BulletTypeNumber || x == MineTypeNumber || x == SpyBugTypeNumber ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber || x == AsteroidTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == FlagTypeNumber || x == SoccerBallItemTypeNumber || x == CircleTypeNumber ||
         x == CoreTypeNumber;
}


bool isMotionTriggerType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber || x == AsteroidTypeNumber ||
         x == MineTypeNumber;
}


bool isTurretTargetType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber || x == ResourceItemTypeNumber ||
         x == TestItemTypeNumber || x == SoccerBallItemTypeNumber;
}


bool isCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldTypeNumber ||
         x == ForceFieldProjectorTypeNumber || x == CoreTypeNumber;
}


bool isForceFieldCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldProjectorTypeNumber;
}


bool isWallType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
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
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == SpyBugTypeNumber || x == MineTypeNumber || x == BulletTypeNumber ||
         x == FlagTypeNumber || x == SoccerBallItemTypeNumber ||
         x == AsteroidTypeNumber || x == TestItemTypeNumber || x == ResourceItemTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == BarrierTypeNumber || x == PolyWallTypeNumber || x == ForceFieldTypeNumber || x == CircleTypeNumber || x == CoreTypeNumber;
}

bool isAsteroidCollideableType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == TestItemTypeNumber || x == ResourceItemTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == BarrierTypeNumber || x == PolyWallTypeNumber || x == ForceFieldTypeNumber || x == CoreTypeNumber;
}

bool isFlagCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == ForceFieldTypeNumber;
}

bool isFlagOrShipCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber || ForceFieldTypeNumber ||
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber;
}

bool isVisibleOnCmdrsMapType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == FlagTypeNumber || x == SoccerBallItemTypeNumber ||
         x == GoalZoneTypeNumber || x == NexusTypeNumber || x == LoadoutZoneTypeNumber || x == SlipZoneTypeNumber ||
         x == SpeedZoneTypeNumber || x == TeleportTypeNumber ||
         x == LineTypeNumber || x == TextItemTypeNumber ||
         x == AsteroidTypeNumber || x == TestItemTypeNumber || x == ResourceItemTypeNumber ||
         x == EnergyItemTypeNumber || x == RepairItemTypeNumber || x == CoreTypeNumber;
}

bool isVisibleOnCmdrsMapWithSensorType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber ||
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == TurretTypeNumber || x == ForceFieldTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == FlagTypeNumber || x == SoccerBallItemTypeNumber ||
         x == GoalZoneTypeNumber || x == NexusTypeNumber || x == LoadoutZoneTypeNumber || x == SlipZoneTypeNumber ||
         x == SpeedZoneTypeNumber || x == TeleportTypeNumber ||
         x == LineTypeNumber || x == TextItemTypeNumber ||
         x == AsteroidTypeNumber || x == TestItemTypeNumber || x == ResourceItemTypeNumber ||
         x == EnergyItemTypeNumber || x == RepairItemTypeNumber ||
         x == CoreTypeNumber ||
         x == BulletTypeNumber || x == MineTypeNumber;  // Weapons visible on commander's map for sensor
}


//bool isMoveItemType(U8 x)
//{
//   return x == 
//}
//
//bool isMoveObjectType(U8 x)
//{
//   return isMoveItem(x) || isShipType(x); 
//}

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
GeometryContainer::GeometryContainer()
{
   mGeometry = NULL;
}


// Copy constructor
GeometryContainer::GeometryContainer(const GeometryContainer &container)
{
   const Geometry *old = container.mGeometry;

   switch(container.mGeometry->getGeomType())
   {
      case geomPoint:
         mGeometry = new PointGeometry(*static_cast<const PointGeometry *>(old));
         break;
      case geomSimpleLine:
         mGeometry = new SimpleLineGeometry(*static_cast<const SimpleLineGeometry *>(old));
         break;
      case geomPolyLine:
         mGeometry = new PolylineGeometry(*static_cast<const PolylineGeometry *>(old));
         break;
      case geomPolygon:
         mGeometry = new PolygonGeometry(*static_cast<const PolygonGeometry *>(old));
         break;
      default:
         TNLAssert(false, "Invalid value!");
         break;
   }
}


// Destructor
GeometryContainer::~GeometryContainer()
{
   delete mGeometry;
}


Geometry *GeometryContainer::getGeometry() const
{
   return mGeometry;
}


const Geometry *GeometryContainer::getConstGeometry() const
{
   return mGeometry;
}


void GeometryContainer::setGeometry(Geometry *geometry)
{
   mGeometry = geometry;
}


const Vector<Point> *GeometryContainer::getOutline() const
{
   return mGeometry->getOutline();
}


const Vector<Point> *GeometryContainer::getFill() const    
{
   return mGeometry->getFill();
}


Point GeometryContainer::getVert(S32 index) const   
{   
   return mGeometry->getVert(index);  
}


string GeometryContainer::geomToString(F32 gridSize) const 
{  
   return mGeometry->geomToString(gridSize);  
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
GeomObject::GeomObject()
{
   // Do nothing
}


// Destructor
GeomObject::~GeomObject()
{
   // Do nothing
};


   // mGeometry will be deleted in destructor
void GeomObject::setNewGeometry(GeomType geomType)
{
   TNLAssert(!mGeometry.getGeometry(), "This object already has a geometry!");

   switch(geomType)
   {
      case geomPoint:
         mGeometry.setGeometry(new PointGeometry());
         return;

      case geomSimpleLine:
         mGeometry.setGeometry(new SimpleLineGeometry());
         return;

      case geomPolyLine:
         mGeometry.setGeometry(new PolylineGeometry());
         return;

      case geomPolygon:
         mGeometry.setGeometry(new PolygonGeometry());
         return;

      default:
         TNLAssert(false, "Unknown geometry!");
         break;
   }
}


// Basic definitions
GeomType GeomObject::getGeomType()              {   return mGeometry.getGeometry()->getGeomType();   }
Point   GeomObject::getVert(S32 index) const   {   return mGeometry.getVert(index);  }

bool GeomObject::deleteVert(S32 vertIndex)               
{   
   if(mGeometry.getGeometry()->deleteVert(vertIndex))
   {
      onPointsChanged();  
      return true;
   }

   return false;
}


bool GeomObject::insertVert(Point vertex, S32 vertIndex) 
{   
   if(mGeometry.getGeometry()->insertVert(vertex, vertIndex))
   {
      onPointsChanged();
      return true;
   }

   return false;
}


void GeomObject::setVert(const Point &pos, S32 index)    { mGeometry.getGeometry()->setVert(pos, index); }
                                                                                           
bool GeomObject::anyVertsSelected()          {   return mGeometry.getGeometry()->anyVertsSelected();        }
S32 GeomObject::getVertCount() const         {   return mGeometry.getGeometry()->getVertCount();            }
S32 GeomObject::getMinVertCount() const      {   return mGeometry.getGeometry()->getMinVertCount();         }

void GeomObject::clearVerts()                {   mGeometry.getGeometry()->clearVerts(); onPointsChanged();  }                        


bool GeomObject::addVertFront(Point vert)
{
   if(mGeometry.getGeometry()->addVertFront(vert))
   {
      onPointsChanged();
      return true;
   }

   return false;
}


bool GeomObject::addVert(const Point &point, bool ignoreMaxPointsLimit) 
{
   if(mGeometry.getGeometry()->addVert(point, ignoreMaxPointsLimit))
   {
      onPointsChanged();
      return true;
   }

   return false;
}


// Vertex selection -- only needed in editor
void GeomObject::selectVert(S32 vertIndex)   {   mGeometry.getGeometry()->selectVert(vertIndex);            }
void GeomObject::aselectVert(S32 vertIndex)  {   mGeometry.getGeometry()->aselectVert(vertIndex);           }
void GeomObject::unselectVert(S32 vertIndex) {   mGeometry.getGeometry()->unselectVert(vertIndex);          }
void GeomObject::unselectVerts()             {   mGeometry.getGeometry()->unselectVerts();                  }
     
bool GeomObject::vertSelected(S32 vertIndex) {   return mGeometry.getGeometry()->vertSelected(vertIndex);   }

// Geometric calculations
Point GeomObject::getCentroid()     {   return mGeometry.getGeometry()->getCentroid();     }
F32  GeomObject::getLabelAngle()   {   return mGeometry.getGeometry()->getLabelAngle();   }
      

// Geometry operations
const Vector<Point> *GeomObject::getOutline() const       {   return mGeometry.getOutline();    }
const Vector<Point> *GeomObject::getFill() const          {   return mGeometry.getFill();       }

Rect GeomObject::calcExtents()                            {   return mGeometry.getGeometry()->calcExtents();   }


// Geometric manipulations
void GeomObject::rotateAboutPoint(const Point &center, F32 angle)  {  mGeometry.getGeometry()->rotateAboutPoint(center, angle);   }
void GeomObject::flip(F32 center, bool isHoriz)                    {  mGeometry.getGeometry()->flip(center, isHoriz);             }
void GeomObject::scale(const Point &center, F32 scale)             {  mGeometry.getGeometry()->scale(center, scale);              }

// Move object to location, specifying (optional) vertex to be positioned at pos
void GeomObject::moveTo(const Point &pos, S32 snapVertex)          {  mGeometry.getGeometry()->moveTo(pos, snapVertex);           }
void GeomObject::offset(const Point &offset)                       {  mGeometry.getGeometry()->offset(offset);                    }

// Geom in-out
void GeomObject::packGeom(GhostConnection *connection, BitStream *stream)    {   mGeometry.getGeometry()->packGeom(connection, stream);     }
void GeomObject::unpackGeom(GhostConnection *connection, BitStream *stream)  {   mGeometry.getGeometry()->unpackGeom(connection, stream); onPointsChanged();  }

void GeomObject::readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize) 
{  
   mGeometry.getGeometry()->readGeom(argc, argv, firstCoord, gridSize); 
   onPointsChanged();
} 

string GeomObject::geomToString(F32 gridSize) const {  return mGeometry.geomToString(gridSize);  }

// Settings
void GeomObject::disableTriangulation() {   mGeometry.getGeometry()->disableTriangulation();   }


void GeomObject::onGeomChanging()
{
   if(getGeomType() == geomPolygon)
      onGeomChanged();               // Allows poly fill to get reshaped as vertices move

   onPointsChanged();
}


void GeomObject::onGeomChanged() {  /* Do nothing */ }


void GeomObject::onPointsChanged()                        
{   
   mGeometry.getGeometry()->onPointsChanged();
}


////////////////////////////////////////
////////////////////////////////////////

// BfObject - the declerations are in BfObject.h

// Constructor
BfObject::BfObject()
{
   mGame = NULL;
   mObjectTypeNumber = UnknownTypeNumber;

   mLitUp = false; 
   mSelected = false; 
   assignNewSerialNumber();

   mTeam = -1;
   mDisableCollisionCount = 0;
   mCreationTime = 0;

   mOwner = NULL;
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


// User assigned id, if any
S32 BfObject::getUserDefinedItemId()
{
   return mUserDefinedItemId;
}


void BfObject::setUserDefinedItemId(S32 itemId)
{
   mUserDefinedItemId = itemId;
}


S32 BfObject::getTeam()
{
   return mTeam;
}


void BfObject::setTeam(S32 team)
{
   mTeam = team;
}


const Color *BfObject::getColor() 
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


void BfObject::clearGame()
{
   mGame = NULL;
}


bool BfObject::processArguments(S32 argc, const char**argv, Game *game)
{
   return true;
}


Point BfObject::getPos() const
{
   return getVert(0);
}


void BfObject::setPos(const Point &pos)
{
   setVert(pos, 0);  
   setExtent(Rect(pos, 10));     // Why 10?  Just a random small number?  We use 0 and 1 elsewhere 
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


bool BfObject::isSelected()
{
   return mSelected;
}


void BfObject::setSelected(bool selected)
{
   mSelected = selected;
}


bool BfObject::isLitUp() 
{ 
   return mLitUp; 
}


void BfObject::setLitUp(bool litUp) 
{ 
   mLitUp = litUp; 

   if(!litUp) 
      setVertexLitUp(NONE); 
}


bool BfObject::isVertexLitUp(S32 vertexIndex)
{
   return mVertexLitUp == vertexIndex;
}


void BfObject::setVertexLitUp(S32 vertexIndex)
{
   mVertexLitUp = vertexIndex;
}


void BfObject::onGeomChanged()
{
   GeomObject::onGeomChanged();
   updateExtentInDatabase();
}


void BfObject::onItemDragging()  { /* Do nothing */ }
void BfObject::onAttrsChanging() { /* Do nothing */ }
void BfObject::onAttrsChanged()  { /* Do nothing */ }


const char *BfObject::getEditorHelpString()
{
   TNLAssert(false, "getEditorHelpString method not implemented!");
   return "getEditorHelpString method not implemented!";  // better then a NULL crash in non-debug mode or continuing past the Assert
}


const char *BfObject::getPrettyNamePlural()
{
   TNLAssert(false, "getPrettyNamePlural method not implemented!");
   return "getPrettyNamePlural method not implemented!";
}


const char *BfObject::getOnDockName()
{
   TNLAssert(false, "getOnDockName method not implemented!");
   return "getOnDockName method not implemented!";
}


const char *BfObject::getOnScreenName()
{
   TNLAssert(false, "getOnScreenName method not implemented!");
   return "getOnScreenName method not implemented!";
}


// Not all editor objects will implement this
const char *BfObject::getInstructionMsg()
{
   return "";
}


#ifndef ZAP_DEDICATED
void BfObject::prepareForDock(ClientGame *game, const Point &point, S32 teamIndex)
{
   mGame = game;

   unselectVerts();
   setTeam(teamIndex);
}

#endif


extern void glColor(const Color &c, float alpha = 1.0);

#ifndef ZAP_DEDICATED
// Render selected and highlighted vertices, called from renderEditor
void BfObject::renderAndLabelHighlightedVertices(F32 currentScale)
{
   F32 radius = getEditorRadius(currentScale);

   // Label and highlight any selected or lit up vertices.  This will also highlight point items.
   for(S32 i = 0; i < getVertCount(); i++)
      if(vertSelected(i) || isVertexLitUp(i) || ((mSelected || mLitUp)  && getVertCount() == 1))
      {
         glColor((vertSelected(i) || (mSelected && getGeomType() == geomPoint)) ? SELECT_COLOR : HIGHLIGHT_COLOR);

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


// Size of object in editor 
F32 BfObject::getEditorRadius(F32 currentScale)
{
   return 10 * currentScale;   // 10 pixels is base size
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
   clearGame();
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


EditorObjectDatabase *BfObject::getEditorObjectDatabase()
{
   TNLAssert(dynamic_cast<EditorObjectDatabase *>(getDatabase()), "This should be a EditorObjectDatabase!");
   return static_cast<EditorObjectDatabase *>(getDatabase());
}


string BfObject::getAttributeString()
{
   return "";
}


S32 BfObject::getDockRadius()
{
   return 10;
}


// For editing attributes -- all implementation will need to be provided by the children
EditorAttributeMenuUI *BfObject::getAttributeMenu()                                      { return NULL; }
void                   BfObject::startEditingAttrs(EditorAttributeMenuUI *attributeMenu) { /* Do nothing */ }
void                   BfObject::doneEditingAttrs(EditorAttributeMenuUI *attributeMenu)  { /* Do nothing */ }


////////////////////////////////////////
////////////////////////////////////////

// Destructor
BfObject::~BfObject()
{
   removeFromGame();
}


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


// Returns number of ships hit
S32 BfObject::radiusDamage(Point pos, S32 innerRad, S32 outerRad, TestFunc objectTypeTest, DamageInfo &info, F32 force)
{
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
      BfObject *foundObject = dynamic_cast<BfObject *>(fillVector[i]);
      // Check the actual distance against our outer radius.  Recall that we got a list of potential
      // collision objects based on a square area, but actual collisions will be based on true distance
      Point objPos = foundObject->getPos();
      Point delta = objPos - pos;

      if(delta.len() > outerRad)
         continue;

      // Can one damage another?
      if(getGame()->getGameType())
         if(!getGame()->getGameType()->objectCanDamageObject(info.damagingObject, foundObject))
            continue;

      //// Check if damager is an area weapon, and damagee is a projectile... if so, kill it
      //if(Projectile *proj = dynamic_cast<Projectile*>(foundObject))
      //{
      //   proj->explode(proj, proj->getActualPos());
      //}

      // Do an LOS check...
      F32 t;
      Point n;

      if(findObjectLOS((TestFunc)isWallType, MoveObject::ActualState, pos, objPos, t, n))
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
   if(!gridDB)
      return;

   gridDB->findObjects(objectTypeTest, fillVector, ext);
}


void BfObject::findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect &ext)
{
   GridDatabase *gridDB = getDatabase();
   if(!gridDB)
      return;

   gridDB->findObjects(typeNumber, fillVector, ext);
}


BfObject *BfObject::findObjectLOS(U8 typeNumber, U32 stateIndex, Point rayStart, Point rayEnd,
                                      float &collisionTime, Point &collisionNormal)
{
   GridDatabase *gridDB = getDatabase();

   if(!gridDB)
      return NULL;

   return dynamic_cast<BfObject *>(
         gridDB->findObjectLOS(typeNumber, stateIndex, rayStart, rayEnd, collisionTime, collisionNormal)
         );
}


BfObject *BfObject::findObjectLOS(TestFunc objectTypeTest, U32 stateIndex, Point rayStart, Point rayEnd,
                                      float &collisionTime, Point &collisionNormal)
{
   GridDatabase *gridDB = getDatabase();

   if(!gridDB)
      return NULL;

   return dynamic_cast<BfObject *>(
         gridDB->findObjectLOS(objectTypeTest, stateIndex, rayStart, rayEnd, collisionTime, collisionNormal)
         );
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


Point BfObject::getVel()
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


F32 BfObject::getRating()
{
   return 0; // TODO: Fix this
}


S32 BfObject::getScore()
{
   return 0; // TODO: Fix this
}


S32 BfObject::getRenderSortValue()
{
   return 2;
}


Rect BfObject::getBounds(U32 stateIndex) const
{
   Rect ret;
   Point p;
   float radius;
   Vector<Point> bounds;

   if(getCollisionPoly(bounds))
   {
      ret.min = ret.max = bounds[0];
      for(S32 i = 1; i < bounds.size(); i++)
         ret.unionPoint(bounds[i]);
   }
   else if(getCollisionCircle(stateIndex, p, radius))
   {
      ret.max = p + Point(radius, radius);
      ret.min = p - Point(radius, radius);
   }

   return ret;
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


// Find if the specified point is in theObject's collisionPoly or collisonCircle
bool BfObject::collisionPolyPointIntersect(Point point)
{
   Point center;
   F32 radius;
   Rect rect;

   Vector<Point> polyPoints;

   polyPoints.clear();

   if(getCollisionPoly(polyPoints))
      return PolygonContains2(polyPoints.address(), polyPoints.size(), point);

   else if(getCollisionCircle(MoveObject::ActualState, center, radius))
      return(center.distanceTo(point) <= radius);

   else
      return false;
}


// Find if the specified polygon intersects theObject's collisionPoly or collisonCircle
bool BfObject::collisionPolyPointIntersect(Vector<Point> points)
{
   Point center;
   Rect rect;
   F32 radius;
   Vector<Point> polyPoints;

   polyPoints.clear();

   if(getCollisionPoly(polyPoints))
      return polygonsIntersect(polyPoints, points);

   else if(getCollisionCircle(MoveObject::ActualState, center, radius))
   {
      Point unusedPt;
      return polygonCircleIntersect(&points[0], points.size(), center, radius * radius, unusedPt);
   }

   else
      return false;
}


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

   else if(getCollisionCircle(MoveObject::ActualState, c, r))
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


void BfObject::writeCompressedVelocity(Point &vel, U32 max, BitStream *stream)
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


S32 BfObject::getTeamIndx(lua_State *L)  // Return item team to Lua
{
   return LuaObject::returnInt(L, mTeam + 1);
}


void BfObject::push(lua_State *L)       // Lua-aware classes will implement this
{
   TNLAssert(false, "Unimplemented push function!");
}


void BfObject::readThisTeam(BitStream *stream)
{
   mTeam = stream->readInt(4) - 2;
}


void BfObject::writeThisTeam(BitStream *stream)
{
   stream->writeInt(mTeam + 2, 4);
}


};

