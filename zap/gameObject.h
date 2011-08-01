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

#ifndef _GAMEOBJECT_H_
#define _GAMEOBJECT_H_

#include "gameConnection.h"
#include "gridDB.h"        // For DatabaseObject
#include "tnlNetObject.h"

#include "Geometry.h"      // For GeomType enum

#include "boost/smart_ptr/shared_ptr.hpp"

struct lua_State;  // or #include "lua.h"

#ifdef TNL_OS_WIN32 
#  pragma warning( disable : 4250)
#endif

namespace Zap
{

class GridDatabase;
class Game;

enum GameObjectType
{
   UnknownType         = BIT(0),    // First bit, BIT(0) == 1, NOT 0!  (Well, yes, 0!, but not 0.  C'mon... get a life!)
   ShipType            = BIT(1),
   BarrierType         = BIT(2),    // Used in both editor and game
   MoveableType        = BIT(3),

   LineType            = BIT(4),     
   ResourceItemType    = BIT(5),
   TextItemType        = BIT(6),    // Added during editor refactor, only used in editor
   ForceFieldType      = BIT(7),
   LoadoutZoneType     = BIT(8),
   TestItemType        = BIT(9),
   FlagType            = BIT(10),
   TurretTargetType    = BIT(11),
   SlipZoneType        = BIT(12),

   BulletType          = BIT(13),      // All projectiles except grenades?
   MineType            = BIT(14),
   SpyBugType          = BIT(15),
   NexusType           = BIT(16),
   //BotNavMeshZoneType  = BIT(17),
   RobotType           = BIT(18),
   TeleportType        = BIT(19),
   GoalZoneType        = BIT(20),

   AsteroidType        = BIT(21),      // Only needed for Lua and editor...
   RepairItemType      = BIT(22),      // Only needed for Lua...
   EnergyItemType      = BIT(23),      // Only needed for Lua...
   SoccerBallItemType  = BIT(24),      // Only needed for Lua and indicating what the ship is carrying and editor...
   WormType            = BIT(25),

   TurretType          = BIT(26),      // Formerly EngineeredType
   ForceFieldProjectorType = BIT(27),  // Formerly EngineeredType
   SpeedZoneType       = BIT(28),      // Only needed for finding speed zones that we may have spawned on top of

   DeletedType       = BIT(30),
   CommandMapVisType = BIT(31),        // These are objects that can be seen on the commander's map

   //////////
   // Types used exclusively in the editor -- will reuse some values from above
   PolyWallType = BIT(25),             // WormType
   ShipSpawnType = BIT(13),            // BulletType
   FlagSpawnType = BIT(1),            // ShipType
   AsteroidSpawnType = BIT(17),        // BotZone
   WallSegmentType = BIT(13),          // Used only in wallSegmentDatabase, so this is OK for the moment


   // Derived types:
   EngineeredType     = TurretType | ForceFieldProjectorType,
   MountableType      = TurretType | ForceFieldProjectorType,
   ItemType           = SoccerBallItemType | MineType | SpyBugType | AsteroidType | FlagType | ResourceItemType | 
                        TestItemType | EnergyItemType | RepairItemType,
   DamagableTypes     = ShipType | RobotType | MoveableType | BulletType | ItemType | ResourceItemType | 
                        EngineeredType | MineType | AsteroidType,
   MotionTriggerTypes = ShipType | RobotType | ResourceItemType | TestItemType | AsteroidType,
   CollideableType    = BarrierType | TurretType | ForceFieldProjectorType,
   WallType           = BarrierType | PolyWallType,
   AllObjectTypes     = 0xFFFFFFFF
};

const U8 UnknownTypeNumber = 0;
const U8 BarrierTypeNumber = 1;
const U8 ShipTypeNumber = 2;
const U8 LineTypeNumber = 3;
const U8 ResourceItemTypeNumber = 4;
const U8 TextItemTypeNumber = 5;
const U8 LoadoutZoneTypeNumber = 6;
const U8 TestItemTypeNumber = 7;
const U8 FlagTypeNumber = 8;
const U8 BulletTypeNumber = 9;
const U8 MineTypeNumber = 10;
const U8 SpybugZoneTypeNumber = 11;
const U8 NexusTypeNumber = 12;
const U8 BotNavMeshZoneTypeNumber = 13;
const U8 RobotTypeNumber = 14;
const U8 TeleporterTypeNumber = 15;
const U8 GoalZoneTypeNumber = 16;
const U8 AsteroidTypeNumber = 17;
const U8 RepairItemTypeNumber = 18;
const U8 EnergyItemTypeNumber = 19;
const U8 SoccerBallItemTypeNumber = 20;
const U8 WormTypeNumber = 21;
const U8 TurretTypeNumber = 22;
const U8 TurretTargetTypeNumber = 23;
const U8 ForceFieldTypeNumber = 24;
const U8 ForceFieldProjectorTypeNumber = 25;
const U8 SpeedZoneTypeNumber = 26;
const U8 PolyWallTypeNumber = 27;
const U8 ShipSpawnTypeNumber = 28;
const U8 FlagSpawnTypeNumber = 29;
const U8 AsteroidSpawnTypeNumber = 30;
const U8 WallItemTypeNumber = 31;
const U8 WallEdgeTypeNumber = 32;
const U8 WallSegmentTypeNumber = 33;
const U8 GrenadeProjectileTypeNumber = 34;
const U8 SlipZoneTypeNumber = 35;
const U8 SpyBugTypeNumber = 36;



const S32 gSpyBugRange = 300;     // How far can a spy bug see?

class GameObject;
class Game;
class GameConnection;


////////////////////////////////////////
////////////////////////////////////////

enum DamageType
{
   DamageTypePoint,
   DamageTypeArea,
};

struct DamageInfo
{
   Point collisionPoint;
   Point impulseVector;
   F32 damageAmount;
   F32 damageSelfMultiplier;
   DamageType damageType;       // see enum above!
   GameObject *damagingObject;  // see class below!

   DamageInfo() { damageSelfMultiplier = 1; }      // Quickie constructor
};

////////////////////////////////////////
////////////////////////////////////////

// Interface class that feeds GameObject and EditorObject -- these things are common to in-game and editor instances of an object
class BfObject : public DatabaseObject
{
protected:
   Game *mGame;
   S32 mTeam;

   boost::shared_ptr<Geometry> mGeometry;

public:
   BfObject();                  // Constructor
   virtual ~BfObject() { };     // Provide virtual destructor

   S32 getTeam() { return mTeam; }
   void setTeam(S32 team) { mTeam = team; }    

   Game *getGame() const { return mGame; }

   virtual void addToGame(Game *game, GridDatabase *database);
   virtual void removeFromGame();
   void clearGame() { mGame = NULL; }

   // DatabaseObject methods
   virtual bool getCollisionPoly(Vector<Point> &polyPoints) const;
   virtual bool getCollisionCircle(U32 stateIndex, Point &point, float &radius) const;

   virtual bool processArguments(S32 argc, const char**argv, Game *game) { return true; }

   // Render is called twice for every object that is in the
   // render list.  By default GameObject will call the render()
   // method one time (when layerIndex == 0).
   virtual void render(S32 layerIndex);
   virtual void render();


   // Geometry methods
   virtual GeomType getGeomType() const { return mGeometry->getGeomType(); }
   virtual Point getVert(S32 index) const { return mGeometry->getVert(index); }
   virtual void setVert(const Point &point, S32 index) { mGeometry->setVert(point, index); }

   S32 getVertCount() const { return mGeometry->getVertCount(); }
   void clearVerts() { mGeometry->clearVerts(); }
   bool addVert(const Point &point)  { return mGeometry->addVert(point); }
   bool addVertFront(Point vert)  { return mGeometry->addVertFront(vert); }
   bool deleteVert(S32 vertIndex)  { return mGeometry->deleteVert(vertIndex); }
   bool insertVert(Point vertex, S32 vertIndex)  { return mGeometry->insertVert(vertex, vertIndex); }

   bool anyVertsSelected() { return mGeometry->anyVertsSelected(); }
   void selectVert(S32 vertIndex) { mGeometry->selectVert(vertIndex); }
   void aselectVert(S32 vertIndex) { mGeometry->aselectVert(vertIndex); }
   void unselectVert(S32 vertIndex) { mGeometry->unselectVert(vertIndex); }
   void unselectVerts() { mGeometry->unselectVerts(); }
   bool vertSelected(S32 vertIndex) { return mGeometry->vertSelected(vertIndex); }

   Vector<Point> *getOutline() const { return mGeometry->getOutline(); }
   Vector<Point> *getFill() const { return mGeometry->getFill(); }
   Point getCentroid() const { return mGeometry->getCentroid(); }
   F32 getLabelAngle() const { return mGeometry->getLabelAngle(); }

   void packGeom(GhostConnection *connection, BitStream *stream) { mGeometry->packGeom(connection, stream); }
   void unpackGeom(GhostConnection *connection, BitStream *stream) { mGeometry->unpackGeom(connection, stream); }

   string geomToString(F32 gridSize) const { return mGeometry->geomToString(gridSize); }
   void readGeom(S32 argc, const char **argv, S32 firstCoord, F32 gridSize) { mGeometry->readGeom(argc, argv, firstCoord, gridSize); }



   virtual void setExtent() { setExtent(mGeometry->getExtents()); }                    // Set extents of object in database
   virtual void setExtent(const Rect &extent) { DatabaseObject::setExtent(extent); }   // Passthrough
   void onPointsChanged() { mGeometry->onPointsChanged(); }

   void disableTriangluation() { mGeometry->disableTriangluation(); }
};


////////////////////////////////////////
////////////////////////////////////////

class GameObject : public virtual BfObject, public NetObject
{
   typedef NetObject Parent;

private:
   SafePtr<GameConnection> mControllingClient;     // Only has meaning on the server, will be null on the client
   SafePtr<GameConnection> mOwner;
   U32 mDisableCollisionCount;                     // No collisions when > 0, use of counter allows "nested" collision disabling

   U32 mCreationTime;

protected:
   Move mLastMove;      // The move for the previous update
   Move mCurrentMove;   // The move for the current update
   StringTableEntry mKillString;     // Alternate descr of what shot projectile (e.g. "Red turret"), used when shooter is not a ship or robot

public:
   GameObject();                             // Constructor
   ~GameObject() { removeFromGame(); }       // Destructor

   virtual void addToGame(Game *game, GridDatabase *database);       // BotNavMeshZone has its own addToGame
   virtual void onAddedToGame(Game *game);

   void markAsGhost() { mNetFlags = NetObject::IsGhost; }

   U32 getCreationTime() { return mCreationTime; }
   void setCreationTime(U32 creationTime) { mCreationTime = creationTime; }

   void deleteObject(U32 deleteTimeInterval = 0);
   
   StringTableEntry getKillString() { return mKillString; }

   F32 getRating() { return 0; }    // TODO: Fix this
   S32 getScore() { return 0; }     // TODO: Fix this

   void findObjects(U32 typeMask, Vector<DatabaseObject *> &fillVector, const Rect &extents, U8 typeNumber = U8_MAX);

   GameObject *findObjectLOS(U32 typeMask, U32 stateIndex, Point rayStart, Point rayEnd, float &collisionTime, Point &collisionNormal, U8 typeNumber = U8_MAX);

   bool isControlled() { return mControllingClient.isValid(); }

   void setOwner(GameConnection *c);

   SafePtr<GameConnection> getControllingClient() { return mControllingClient; }
   void setControllingClient(GameConnection *c) { mControllingClient = c; }         // This only gets run on the server

   GameConnection *getOwner();

   F32 getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips);

   virtual S32 getRenderSortValue() { return 2; }

   Rect getBounds(U32 stateIndex) const;

   const Move &getCurrentMove() { return mCurrentMove; }
   const Move &getLastMove() { return mLastMove; }
   void setCurrentMove(const Move &theMove) { mCurrentMove = theMove; }
   void setLastMove(const Move &theMove) { mLastMove = theMove; }

   enum IdleCallPath {
      ServerIdleMainLoop,              // Idle called from top-level idle loop on server
      ServerIdleControlFromClient,
      ClientIdleMainRemote,            // On client, when object is not our control object
      ClientIdleControlMain,           // On client, when object is our control object
      ClientIdleControlReplay,
   };

   virtual void idle(IdleCallPath path);

   virtual void writeControlState(BitStream *stream);
   virtual void readControlState(BitStream *stream);
   virtual F32 getHealth() { return 1; }
   virtual bool isDestroyed() { return false; }

   virtual void controlMoveReplayComplete();

   void writeCompressedVelocity(Point &vel, U32 max, BitStream *stream);
   void readCompressedVelocity(Point &vel, U32 max, BitStream *stream);

   virtual Point getActualPos();
   virtual Point getRenderPos();
   virtual Point getRenderVel() { return Point(); }
   virtual Point getActualVel() { return Point(); }

   virtual void setActualPos(Point p);

   virtual bool collide(GameObject *hitObject) { return false; }

   S32 radiusDamage(Point pos, S32 innerRad, S32 outerRad, U32 typemask, DamageInfo &info, F32 force = 2000);
   virtual void damageObject(DamageInfo *damageInfo);

   bool onGhostAdd(GhostConnection *theConnection);
   void disableCollision() { TNLAssert(mDisableCollisionCount < 10, "Too many disabled collisions"); mDisableCollisionCount++; }
   void enableCollision() { TNLAssert(mDisableCollisionCount != 0, "Trying to enable collision, already enabled"); mDisableCollisionCount--; }
   bool isCollisionEnabled() { return mDisableCollisionCount == 0; }

   bool collisionPolyPointIntersect(Point point);
   bool collisionPolyPointIntersect(Vector<Point> points);
   bool collisionPolyPointIntersect(Point center, F32 radius);

   void setScopeAlways();

   S32 getTeamIndx(lua_State *L);            // Return item team to Lua
   virtual void push(lua_State *L);    // Lua-aware classes will implement this
};


};

#endif

