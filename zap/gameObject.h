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

#include "gridDB.h"        // For DatabaseObject
#include "game.h"

#include "luaObject.h"     // For LuaObject def and returnInt method
#include "lua.h"           // For push prototype

#ifdef TNL_OS_WIN32 
#  pragma warning( disable : 4250)
#endif

namespace Zap
{

class GridDatabase;

enum GameObjectType
{
   UnknownType         = BIT(0),    // First bit, BIT(0) == 1, NOT 0!  (Well, yes, 0!, but not 0.  C'mon... get a life!)
   ShipType            = BIT(1),
   BarrierType         = BIT(2),    // Used in both editor and game
   MoveableType        = BIT(3),
   ItemType            = BIT(4),    // Not made available to Lua... could we get rid of this altogether?  Or make it a aggregate of other masks?
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
   BotNavMeshZoneType  = BIT(17),
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
   CommandMapVisType = BIT(31),     // These are objects that can be seen on the commander's map

   // Derived types:
   EngineeredType     = TurretType | ForceFieldProjectorType,
   DamagableTypes     = ShipType | RobotType | MoveableType | BulletType | ItemType | ResourceItemType | EngineeredType | MineType | AsteroidType,
   MotionTriggerTypes = ShipType | RobotType | ResourceItemType | TestItemType | AsteroidType,
   CollideableType    = BarrierType | TurretType | ForceFieldProjectorType,
   AllObjectTypes     = 0xFFFFFFFF,
  
   //////////
   // Types used exclusively in the editor -- will reuse some values from above
   EditorWallSegmentType = BIT(3),
   SpawnType = BIT(13),
   FlagSpawnType = BIT(11),
   PolyWallType = BIT(25)
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
class XObject : public DatabaseObject
{
protected:
   Game *mGame;
   S32 mTeam;

public:
   virtual ~XObject() { };     // Provide virtual destructor

   S32 getTeam() { return mTeam; }
   void setTeam(S32 team) { mTeam = team; }    


   Game *getGame() { return mGame; }
   void setGame(Game *game) { mGame = game; }

   // DatabaseObject methods
   virtual GridDatabase *getGridDatabase();     // BotNavMeshZones have their own GridDatabase
   virtual bool getCollisionPoly(Vector<Point> &polyPoints);
   virtual bool getCollisionCircle(U32 stateIndex, Point &point, float &radius);

   virtual bool processArguments(S32 argc, const char**argv) { return true; }

   // Render is called twice for every object that is in the
   // render list.  By default GameObject will call the render()
   // method one time (when layerIndex == 0).
   virtual void render(S32 layerIndex);
   virtual void render();
};

////////////////////////////////////////
////////////////////////////////////////

class GameObject : public virtual XObject, public NetObject
{
   typedef NetObject Parent;

private:
   U32 mCreationTime;
   SafePtr<GameConnection> mControllingClient;     // Only has meaning on the server, will be null on the client
   SafePtr<GameConnection> mOwner;
   U32 mDisableCollisionCount;                     // No collisions when > 0, use of counter allows "nested" collision disabling

protected:
   Move mLastMove;      // The move for the previous update
   Move mCurrentMove;   // The move for the current update
   StringTableEntry mKillString;     // Alternate descr of what shot projectile (e.g. "Red turret"), used when shooter is not a ship or robot

public:
   GameObject();                             // Constructor
   ~GameObject() { removeFromGame(); }       // Destructor

   virtual void addToGame(Game *game);       // BotNavMeshZone has its own addToGame
   virtual void onAddedToGame(Game *game);
   void removeFromGame();
   

   void deleteObject(U32 deleteTimeInterval = 0);
   U32 getCreationTime() { return mCreationTime; }
   
   
   StringTableEntry getKillString() { return mKillString; }

   F32 getRating() { return 0; }    // TODO: Fix this
   S32 getScore() { return 0; }     // TODO: Fix this

   void findObjects(U32 typeMask, Vector<DatabaseObject *> &fillVector, const Rect &extents);

   GameObject *findObjectLOS(U32 typeMask, U32 stateIndex, Point rayStart, Point rayEnd, float &collisionTime, Point &collisionNormal);

   bool isControlled() { return mControllingClient.isValid(); }

   void setOwner(GameConnection *c);

   SafePtr<GameConnection> getControllingClient() { return mControllingClient; }
   void setControllingClient(GameConnection *c) { mControllingClient = c; }         // This only gets run on the server

   GameConnection *getOwner();

   F32 getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips);

   virtual S32 getRenderSortValue() { return 2; }

   Rect getBounds(U32 stateIndex);

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

   S32 getTeamIndx(lua_State *L) { return LuaObject::returnInt(L, mTeam + 1); }             // Return item team to Lua
   virtual void push(lua_State *L) { TNLAssert(false, "Unimplemented push function!"); }    // Lua-aware classes will implement this
};


};

#endif

