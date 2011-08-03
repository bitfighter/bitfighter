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

#include "Geometry_Base.h"      // For GeomType enum

#include "boost/smart_ptr/shared_ptr.hpp"

struct lua_State;  // or #include "lua.h"

#ifdef TNL_OS_WIN32 
#  pragma warning( disable : 4250)
#endif

namespace Zap
{

class GridDatabase;
class Game;

// Most of TypeNumber are used in LUA
const U8 UnknownTypeNumber = 0;
const U8 ShipTypeNumber = 1;
const U8 BarrierTypeNumber = 2;
// 3 = MovableType..
const U8 LineTypeNumber = 4;
const U8 ResourceItemTypeNumber = 5;
const U8 TextItemTypeNumber = 6;
const U8 ForceFieldTypeNumber = 7;
const U8 LoadoutZoneTypeNumber = 8;
const U8 TestItemTypeNumber = 9;
const U8 FlagTypeNumber = 10;
const U8 SpeedZoneTypeNumber = 11;
const U8 SlipZoneTypeNumber = 12;
const U8 BulletTypeNumber = 13;
const U8 MineTypeNumber = 14;
const U8 SpyBugTypeNumber = 15;
const U8 NexusTypeNumber = 16;
// bit 17?
const U8 RobotTypeNumber = 18;
const U8 TeleportTypeNumber = 19;
const U8 GoalZoneTypeNumber = 20;
const U8 AsteroidTypeNumber = 21;
const U8 RepairItemTypeNumber = 22;
const U8 EnergyItemTypeNumber = 23;
const U8 SoccerBallItemTypeNumber = 24;
const U8 WormTypeNumber = 25;
const U8 TurretTypeNumber = 26;
const U8 ForceFieldProjectorTypeNumber = 27;

const U8 BotNavMeshZoneTypeNumber = 64;  // seperate database
const U8 PolyWallTypeNumber = 65;

// These are probably used only in editor..
const U8 WallItemTypeNumber = 66;
const U8 WallEdgeTypeNumber = 67;
const U8 WallSegmentTypeNumber = 68;
const U8 ShipSpawnTypeNumber = 69;
const U8 FlagSpawnTypeNumber = 70;
const U8 AsteroidSpawnTypeNumber = 71;


// LuaRobot uses 
// LuaRobot::doFindItems uses TypeNumber, to speed up search, anything less then 32 will be type masked


enum GameObjectType
{
   UnknownType         = BIT(UnknownTypeNumber),    // First bit, BIT(0) == 1, NOT 0!  (Well, yes, 0!, but not 0.  C'mon... get a life!)
   ShipType            = BIT(ShipTypeNumber),
   BarrierType         = BIT(BarrierTypeNumber),    // Used in both editor and game
   MoveableType        = BIT(3),

   LineType            = BIT(LineTypeNumber),     
   ResourceItemType    = BIT(ResourceItemTypeNumber),
   TextItemType        = BIT(TextItemTypeNumber),    // Added during editor refactor, only used in editor
   ForceFieldType      = BIT(ForceFieldTypeNumber),
   LoadoutZoneType     = BIT(LoadoutZoneTypeNumber),
   TestItemType        = BIT(TestItemTypeNumber),
   FlagType            = BIT(FlagTypeNumber),
   SpeedZoneType       = BIT(SpeedZoneTypeNumber),      // Only needed for finding speed zones that we may have spawned on top of
   SlipZoneType        = BIT(SlipZoneTypeNumber),

   BulletType          = BIT(BulletTypeNumber),      // All projectiles except grenades?
   MineType            = BIT(MineTypeNumber),
   SpyBugType          = BIT(SpyBugTypeNumber),
   NexusType           = BIT(NexusTypeNumber),
   //                  = BIT(17),  // Was bot zone, now this bit might be used for editor
   RobotType           = BIT(RobotTypeNumber),
   TeleportType        = BIT(TeleportTypeNumber),
   GoalZoneType        = BIT(GoalZoneTypeNumber),

   AsteroidType        = BIT(AsteroidTypeNumber),      // Only needed for editor...
   RepairItemType      = BIT(RepairItemTypeNumber),
   EnergyItemType      = BIT(EnergyItemTypeNumber),
   SoccerBallItemType  = BIT(SoccerBallItemTypeNumber),      // Only needed for indicating what the ship is carrying and editor...
   WormType            = BIT(WormTypeNumber),

   TurretType          = BIT(TurretTypeNumber),      // Formerly EngineeredType
   ForceFieldProjectorType = BIT(ForceFieldProjectorTypeNumber),  // Formerly EngineeredType

   // _______________  = BIT(28),      // FREE BIT

   DeletedType         = BIT(30),
   CommandMapVisType   = BIT(31),        // These are objects that can be seen on the commander's map

   //////////
   // Types used exclusively in the editor -- will reuse some values from above
   PolyWallType = BIT(29),             // WormType
   //ShipSpawnType = BIT(13),            // BulletType
   //FlagSpawnType = BIT(1),            // ShipType
   //AsteroidSpawnType = BIT(17),        //
   //WallSegmentType = BIT(13),          // Used only in wallSegmentDatabase, so this is OK for the moment


   // Derived types:
   EngineeredType     = TurretType | ForceFieldProjectorType,
// MountableType      = TurretType | ForceFieldProjectorType,
   ItemType           = SoccerBallItemType | MineType | SpyBugType | AsteroidType | FlagType | ResourceItemType | 
                        TestItemType | EnergyItemType | RepairItemType,
   DamagableTypes     = ShipType | RobotType | MoveableType | BulletType | ItemType | ResourceItemType | 
                        EngineeredType | MineType | AsteroidType,
   MotionTriggerTypes = ShipType | RobotType | ResourceItemType | TestItemType | AsteroidType,
	TurretTargetType   = ShipType | RobotType | ResourceItemType | TestItemType | SoccerBallItemType,
   CollideableType    = BarrierType | TurretType | ForceFieldProjectorType,
   WallType           = BarrierType | PolyWallType,
   AllObjectTypes     = 0xFFFFFFFF
};






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
class BfObject : public DatabaseObject, public Geometry
{
protected:
   Game *mGame;
   S32 mTeam;

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

   virtual void setExtent() { setExtent(getExtents()); }                    // Set extents of object in database
   virtual void setExtent(const Rect &extent) { DatabaseObject::setExtent(extent); }   // Passthrough

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

