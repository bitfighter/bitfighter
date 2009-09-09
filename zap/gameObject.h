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

#include "../tnl/tnlTypes.h"
#include "../tnl/tnlNetObject.h"

#include "gameConnection.h"
#include "game.h"
#include "move.h"
#include "point.h"    

#include "lua.h"  // For push prototype

namespace Zap
{

class GridDatabase;

enum GameObjectType
{
   UnknownType         = BIT(0),    // First bit, BIT(0) == 1, NOT 0!  (Well, yes, 0!, but not 0.  C'mon... get a life!)
   ShipType            = BIT(1),
   BarrierType         = BIT(2),
   MoveableType        = BIT(3),
   ItemType            = BIT(4),    // Not made available to Lua... could we get rid of this altogether?
   ResourceItemType    = BIT(5),
         // slot available
   ForceFieldType      = BIT(7),
   LoadoutZoneType     = BIT(8),
   TestItemType        = BIT(9),
   FlagType            = BIT(10),
   TurretTargetType    = BIT(11),
   SlipZoneType        = BIT(12),

   BulletType          = BIT(13),      // All projectiles except grenades?
   MineType            = BIT(14),
   SpyBugType          = BIT(15),
         // slot available
   NexusType           = BIT(17),
   BotNavMeshZoneType  = BIT(18),
   RobotType           = BIT(19),
   TeleportType        = BIT(20),   
   GoalZoneType        = BIT(21),
   AsteroidType        = BIT(22),      // Only needed for Lua...
   RepairItemType      = BIT(23),      // Only needed for Lua...
   SoccerBallItemType  = BIT(24),      // Only needed for Lua...
   NexusFlagType       = BIT(25),      // Only needed for Lua...
   TurretType          = BIT(26),      // Formerly EngineeredType
   ForceFieldProjectorType = BIT(27),  // Formerly EngineeredType

   DeletedType       = BIT(30),
   CommandMapVisType = BIT(31),     // These are objects that can be seen on the commander's map 

   // Derived types:
   EngineeredType     = TurretType | ForceFieldProjectorType,
   DamagableTypes     = ShipType | RobotType | MoveableType | BulletType | ItemType | ResourceItemType | EngineeredType | MineType | AsteroidType,
   MotionTriggerTypes = ShipType | RobotType | ResourceItemType | TestItemType | AsteroidType,
   AllObjectTypes     = 0xFFFFFFFF,
};

const S32 gSpyBugRange = 300;     // How far can a spy bug see?

class GameObject;
class Game;
class GameConnection;

enum DamageType
{
   DamageTypePoint,
   DamageTypeArea,
};

struct DamageInfo
{
   Point collisionPoint;
   Point impulseVector;
   float damageAmount;
   DamageType damageType;       // see enum above!
   GameObject *damagingObject;  // see class below!
};

class GameObject : public NetObject
{
   friend class GridDatabase;

private:
   typedef NetObject Parent;
   Game *mGame;
   U32 mLastQueryId;
   U32 mCreationTime;
   SafePtr<GameConnection> mControllingClient;
   SafePtr<GameConnection> mOwner;
   U32 mDisableCollisionCount;      // No collisions with this object if true
   bool mInDatabase;

   F32 mRadius;
   F32 mMass;

   Rect extent;

protected:
   U32 mObjectTypeMask;
   Move mLastMove;      // The move for the previous update
   Move mCurrentMove;   // The move for the current update
   S32 mTeam;
   StringTableEntry mKillString;     // Alternate descr of what shot projectile (e.g. "Red turret"), used when shooter is not a ship or robot

public:
   GameObject();                          // Constructor
   ~GameObject() { removeFromGame(); }    // Destructor

   void addToGame(Game *theGame);
   virtual void onAddedToGame(Game *theGame);
   void removeFromGame();

   Game *getGame() { return mGame; }

   void deleteObject(U32 deleteTimeInterval = 0);
   U32 getCreationTime() { return mCreationTime; }
   bool isInDatabase() { return mInDatabase; }
   void setExtent(Rect &extentRect);
   StringTableEntry getKillString() { return mKillString; }
   Rect getExtent() { return extent; }
   S32 getTeam() { return mTeam; }
   void processPolyBounds(S32 argc, const char **argv, S32 firstCoord, Vector<Point> &polyBounds);    // Convert line of a level file into a Vector of Points
   void findObjects(U32 typeMask, Vector<GameObject *> &fillVector, const Rect &extents);

   GameObject *findObjectLOS(U32 typeMask, U32 stateIndex, Point rayStart, Point rayEnd, float &collisionTime, Point &collisionNormal);

   bool isControlled() { return mControllingClient.isValid(); }

   void setOwner(GameConnection *c);

   SafePtr<GameConnection> getControllingClient() { return mControllingClient; }
   void setControllingClient(GameConnection *c) { mControllingClient = c; }

   GameConnection *getOwner();

   U32 getObjectTypeMask() { return mObjectTypeMask; }
   void setObjectTypeMask(U32 objectTypeMask) { mObjectTypeMask = objectTypeMask; }

   F32 getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips);

   virtual S32 getRenderSortValue() { return 0; }
   virtual void render();

   /// render is called twice for every object that is in the
   /// render list.  By default GameObject will call the render()
   /// method one time (when layerIndex == 0).
   virtual void render(S32 layerIndex);

   virtual bool getCollisionPoly(Vector<Point> &polyPoints);
   virtual bool getCollisionCircle(U32 stateIndex, Point &point, float &radius);
   Rect getBounds(U32 stateIndex);

   const Move &getCurrentMove() { return mCurrentMove; }
   const Move &getLastMove() { return mLastMove; }
   void setCurrentMove(const Move &theMove) { mCurrentMove = theMove; }
   void setLastMove(const Move &theMove) { mLastMove = theMove; }

   enum IdleCallPath {
      ServerIdleMainLoop,              // Idle called from top-level idle loop on server
      ServerIdleControlFromClient,
      ClientIdleMainRemote,            // Idle run on the client 
      ClientIdleControlMain,
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

   virtual Point getRenderPos();
   virtual Point getActualPos();
   virtual Point getRenderVel() { return Point(); }
   virtual Point getActualVel() { return Point(); }

   virtual void setActualPos(Point p);

   virtual bool collide(GameObject *hitObject) { return false; }

   void radiusDamage(Point pos, U32 innerRad, U32 outerRad, U32 typemask, DamageInfo &info, F32 force = 2000.f);
   virtual void damageObject(DamageInfo *damageInfo);

   bool onGhostAdd(GhostConnection *theConnection);
   void disableCollision() { mDisableCollisionCount++; }
   void enableCollision() { mDisableCollisionCount--; }
   void addToDatabase();
   void removeFromDatabase();
   bool isCollisionEnabled() { return mDisableCollisionCount == 0; }

   bool collisionPolyPointIntersect(Point point);
   bool collisionPolyPointIntersect(Vector<Point> points);
   bool collisionPolyPointIntersect(Point center, F32 radius);

   virtual bool processArguments(S32 argc, const char**argv);
   void setScopeAlways();

   virtual void push(lua_State *L) { TNLAssert(false, "Unimplemented push function!"); }    // Lua-aware classes will implement this
};

};

#endif
