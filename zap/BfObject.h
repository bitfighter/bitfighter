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

#ifndef _BFOBJECT_H_
#define _BFOBJECT_H_

#include "gridDB.h"           // Base class
#include "tnlNetObject.h"
#include "move.h"
#include "LuaWrapper.h"
#include "LuaScriptRunner.h"  // Base class

//struct lua_State;  // or #include "lua.h"

#ifdef TNL_OS_WIN32 
#  pragma warning( disable : 4250)
#endif

namespace TNL{ class BitStream; }


namespace Zap
{

class GridDatabase;
class Game;
class ClientInfo;

/**
 * @luaenum ObjType(2, 1)
 * The ObjType enum can be used to represent different kinds of objects.
 */
// We're limited to 255 values in this table, as type number is commonly passed as a U8.
// We don't need to make all these values available to scripts; set the 2nd value to false for
// those values that we don't want to share.
//
//                                         Make available      
//                   Enum                   to scripts?        Available to scripts as:  
#define TYPE_NUMBER_TABLE \
   TYPE_NUMBER( BarrierTypeNumber,             true,              "BarrierType"             ) \
   TYPE_NUMBER( PlayerShipTypeNumber,          true,              "ShipType"                ) \
   TYPE_NUMBER( LineTypeNumber,                true,              "LineType"                ) \
   TYPE_NUMBER( ResourceItemTypeNumber,        true,              "ResourceItemType"        ) \
   TYPE_NUMBER( TextItemTypeNumber,            true,              "TextItemType"            ) \
   TYPE_NUMBER( LoadoutZoneTypeNumber,         true,              "LoadoutZoneType"         ) \
   TYPE_NUMBER( TestItemTypeNumber,            true,              "TestItemType"            ) \
   TYPE_NUMBER( FlagTypeNumber,                true,              "FlagType"                ) \
   TYPE_NUMBER( BulletTypeNumber,              true,              "BulletType"              ) \
   TYPE_NUMBER( BurstTypeNumber,               true,              "BurstType"               ) \
   TYPE_NUMBER( MineTypeNumber,                true,              "MineType"                ) \
   TYPE_NUMBER( NexusTypeNumber,               true,              "NexusType"               ) \
   TYPE_NUMBER( BotNavMeshZoneTypeNumber,      true,              "BotNavMeshZoneType"      ) \
   TYPE_NUMBER( RobotShipTypeNumber,           true,              "RobotType"               ) \
   TYPE_NUMBER( TeleporterTypeNumber,          true,              "TeleporterType"          ) \
   TYPE_NUMBER( GoalZoneTypeNumber,            true,              "GoalZoneType"            ) \
   TYPE_NUMBER( AsteroidTypeNumber,            true,              "AsteroidType"            ) \
   TYPE_NUMBER( RepairItemTypeNumber,          true,              "RepairItemType"          ) \
   TYPE_NUMBER( EnergyItemTypeNumber,          true,              "EnergyItemType"          ) \
   TYPE_NUMBER( SoccerBallItemTypeNumber,      true,              "SoccerBallItemType"      ) \
   TYPE_NUMBER( WormTypeNumber,                true,              "WormType"                ) \
   TYPE_NUMBER( TurretTypeNumber,              true,              "TurretType"              ) \
   TYPE_NUMBER( ForceFieldTypeNumber,          true,              "ForceFieldType"          ) \
   TYPE_NUMBER( ForceFieldProjectorTypeNumber, true,              "ForceFieldProjectorType" ) \
   TYPE_NUMBER( SpeedZoneTypeNumber,           true,              "SpeedZoneType"           ) \
   TYPE_NUMBER( PolyWallTypeNumber,            true,              "PolyWallType"            ) \
   TYPE_NUMBER( ShipSpawnTypeNumber,           true,              "ShipSpawnType"           ) \
   TYPE_NUMBER( FlagSpawnTypeNumber,           true,              "FlagSpawnType"           ) \
   TYPE_NUMBER( AsteroidSpawnTypeNumber,       true,              "AsteroidSpawnType"       ) \
   TYPE_NUMBER( CircleSpawnTypeNumber,         true,              "CircleSpawnType"         ) \
   TYPE_NUMBER( WallItemTypeNumber,            true,              "WallItemType"            ) \
   TYPE_NUMBER( WallEdgeTypeNumber,            true,              "WallEdgeType"            ) \
   TYPE_NUMBER( WallSegmentTypeNumber,         true,              "WallSegmentType"         ) \
   TYPE_NUMBER( SlipZoneTypeNumber,            true,              "SlipZoneType"            ) \
   TYPE_NUMBER( SpyBugTypeNumber,              true,              "SpyBugType"              ) \
   TYPE_NUMBER( CoreTypeNumber,                true,              "CoreType"                ) \
   TYPE_NUMBER( ZoneTypeNumber,                true,              "ZoneType"                ) \
   TYPE_NUMBER( CircleTypeNumber,              true,              "CircleType"              ) \
   TYPE_NUMBER( SeekerTypeNumber,              true,              "SeekerType"              ) \
   TYPE_NUMBER( DeletedTypeNumber,             false,             "DeletedType"             ) \
   TYPE_NUMBER( UnknownTypeNumber,             false,             "UnknownType"             ) \


// Define an enum from the first values in TYPE_NUMBER_TABLE
enum TypeNumber {
#define TYPE_NUMBER(a, b, c) a,
    TYPE_NUMBER_TABLE
#undef TYPE_NUMBER
    TypesNumbers
};


// Derived Types are determined by function
bool isEngineeredType(U8 x);
bool isShipType(U8 x);
bool isProjectileType(U8 x);
bool isGrenadeType(U8 x);
bool isWithHealthType(U8 x);
bool isForceFieldDeactivatingType(U8 x);
bool isDamageableType(U8 x);
bool isMotionTriggerType(U8 x);
bool isTurretTargetType(U8 x);
bool isCollideableType(U8 x);                  // Move objects bounce off of these
bool isForceFieldCollideableType(U8 x);
bool isWallType(U8 x);
bool isWallItemType(U8 x);
bool isLineItemType(U8 x);
bool isWeaponCollideableType(U8 x);
bool isAsteroidCollideableType(U8 x);
bool isFlagCollideableType(U8 x);
bool isFlagOrShipCollideableType(U8 x);
bool isVisibleOnCmdrsMapType(U8 x);
bool isVisibleOnCmdrsMapWithSensorType(U8 x);
bool isZoneType(U8 x);
bool isSeekerTarget(U8 x);

bool isAnyObjectType(U8 x);
// END GAME OBJECT TYPES

typedef bool (*TestFunc)(U8);

const S32 gSpyBugRange = 300;                // How far can a spy bug see?

class Game;
class GameConnection;
class Color;


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
   BfObject *damagingObject;  // see class below!

   DamageInfo();  // Constructor
};


////////////////////////////////////////
////////////////////////////////////////

class EditorObject
{
private:
   bool mSelected;            // True if item is selected                                                                     
   bool mLitUp;               // True if user is hovering over the item and it's "lit up"                                     
   S32 mVertexLitUp;          // Only one vertex should be lit up at a given time -- could this be an attribute of the editor?
   S32 mUserDefinedItemId;    // Item's unique id... 0 if there is none

public:
   EditorObject();      // Constructor

   // Messages and such for the editor
   virtual const char *getOnScreenName();
   virtual const char *getPrettyNamePlural();
   virtual const char *getOnDockName();
   virtual const char *getEditorHelpString();
   virtual const char *getInstructionMsg();        // Message printed below item when it is selected
   virtual string getAttributeString();            // Used for displaying object attributes in lower-left of editor


   // Objects can be different sizes on the dock and in the editor.  We need to draw selection boxes in both locations,
   // and these functions specify how big those boxes should be.  Override if implementing a non-standard sized item.
   // (strictly speaking, only getEditorRadius needs to be public, but it make sense to keep these together organizationally.)
   virtual S32 getDockRadius();                    // Size of object on dock
   virtual F32 getEditorRadius(F32 currentScale);  // Size of object in editor


   //////
   // Things are happening in the editor; the object must respond!
   // Actually, onGeomChanged() and onAttrsChanged() might need to do something if changed by a script in-game

   virtual void onItemDragging();      // Item is being dragged around the screen

   virtual void onAttrsChanging();     // Attr is in the process of being changed (e.g. a char was typed for a textItem)
   virtual void onAttrsChanged();      // Attrs changed -- only used by TextItem

   // Track some items used in the editor
   void setSelected(bool selected);
   bool isSelected();

   bool isLitUp();
   void setLitUp(bool litUp);

   // Keep track which vertex, if any is lit up in the currently selected item
   bool isVertexLitUp(S32 vertexIndex);
   void setVertexLitUp(S32 vertexIndex);

   // Manage user-assigned IDs -- intended for use by scripts to identify user-designated items
   S32 getUserDefinedItemId();
   void setUserDefinedItemId(S32 itemId);
};

////////////////////////////////////////
////////////////////////////////////////

class ClientGame;
class EditorAttributeMenuUI;
class WallSegment;
class ClientInfo;

class BfObject : public DatabaseObject, public NetObject, public EditorObject, public LuaObject
{
   typedef NetObject Parent;

public:
   enum IdleCallPath {
      ServerIdleMainLoop,              // On server, when called from top-level idle loop
      ServerIdleControlFromClient,     // On server, when processing moves from the client
      ClientIdleMainRemote,            // On client, when object is not our control object
      ClientIdleControlMain,           // On client, when object is our control object
      ClientIdleControlReplay,
   };

private:
   SafePtr<GameConnection> mControllingClient;     // Only has meaning on the server, will be null on the client
   SafePtr<ClientInfo> mOwner;
   U32 mDisableCollisionCount;                     // No collisions when > 0, use of counter allows "nested" collision disabling

   U32 mCreationTime;
   S32 mTeam;

   S32 mSerialNumber;         // Autoincremented serial number  
   U32 mUserAssignedId;       // Id assigned to some objects in the editor

protected:
   Move mLastMove;      // The move for the previous update
   Move mCurrentMove;   // The move for the current update
   StringTableEntry mKillString;     // Alternate descr of what shot projectile (e.g. "Red turret"), used when shooter is not a ship or robot
   Game *mGame;

public:
   BfObject();                // Constructor
   virtual ~BfObject();       // Destructor

   virtual void addToGame(Game *game, GridDatabase *database);       // BotNavMeshZone has its own addToGame
   virtual void onAddedToGame(Game *game);

   void markAsGhost();

   virtual bool isMoveObject();
   virtual Point getVel() const;

   U32 getCreationTime();
   void setCreationTime(U32 creationTime);

   void deleteObject(U32 deleteTimeInterval = 0);
   
   void setUserAssignedId(U32 id);
   U32 getUserAssignedId();

   StringTableEntry getKillString();

   enum MaskBits {
      GeomMask      = BIT(0),
      TeamMask      = BIT(1),
      FirstFreeMask = BIT(2)
   };

   BfObject *findObjectLOS(U8 typeNumber, U32 stateIndex, Point rayStart, Point rayEnd, float &collisionTime, Point &collisionNormal);
   BfObject *findObjectLOS(TestFunc, U32 stateIndex, Point rayStart, Point rayEnd, float &collisionTime, Point &collisionNormal);

   bool controllingClientIsValid();                   // Checks if controllingClient is valid
   SafePtr<GameConnection> getControllingClient();
   void setControllingClient(GameConnection *c);      // This only gets run on the server

   void setOwner(ClientInfo *clientInfo);
   ClientInfo *getOwner();

   F32 getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips);

   void findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect &extents);
   void findObjects(TestFunc, Vector<DatabaseObject *> &fillVector, const Rect &extents);

   virtual S32 getRenderSortValue();

   // Move related      
   const Move &getCurrentMove();
   const Move &getLastMove();
   void setCurrentMove(const Move &theMove);
   void setLastMove(const Move &theMove);

   // Render is called twice for every object that is in the
   // render list.  By default BfObject will call the render()
   // method one time (when layerIndex == 0).
   // TODO: Would be better to render once and use different z-order to create layers?
   virtual void render(S32 layerIndex);
   virtual void render();

   virtual void idle(IdleCallPath path);              

   virtual void writeControlState(BitStream *stream); 
   virtual void readControlState(BitStream *stream);  
   virtual F32 getHealth();                           
   virtual bool isDestroyed();                        

   virtual void controlMoveReplayComplete();          

   // These are only here because Projectiles are not MoveObjects -- if they were, this could go there
   void writeCompressedVelocity(const Point &vel, U32 max, BitStream *stream);
   void readCompressedVelocity(Point &vel, U32 max, BitStream *stream);

   virtual bool collide(BfObject *hitObject);
   virtual bool collided(BfObject *otherObject, U32 stateIndex);

   // Gets location(s) where repair rays should be rendered while object is being repaired
   virtual Vector<Point> getRepairLocations(const Point &repairOrigin);    
   bool objectIntersectsSegment(BfObject *object, const Point &rayStart, const Point &rayEnd, F32 &fillCollisionTime);
   S32 radiusDamage(Point pos, S32 innerRad, S32 outerRad, TestFunc objectTypeTest, DamageInfo &info, F32 force = 2000);
   virtual void damageObject(DamageInfo *damageInfo);

   void onGhostAddBeforeUpdate(GhostConnection *theConnection);
   bool onGhostAdd(GhostConnection *theConnection);

   void disableCollision();        
   void enableCollision();         
   bool isCollisionEnabled(); 

   //bool collisionPolyPointIntersect(Point point);
   //bool collisionPolyPointIntersect(Vector<Point> points);
   bool collisionPolyPointIntersect(Point center, F32 radius);

   void setScopeAlways();           

   void readThisTeam(BitStream *stream);     // xxx editor?
   void writeThisTeam(BitStream *stream);    // xxx editor?

   // Team related
   S32 getTeam() const;
   void setTeam(S32 team);
   const Color *getColor() const;      // Get object's team's color

   // These methods used to be in EditorObject, but we'll need to know about them as we add
   // the ability to manipulate objects more using Lua
   virtual bool canBeHostile();  
   virtual bool canBeNeutral();  
   virtual bool hasTeam();       

   BfObject *copy();       // Makes a duplicate of the item (see method for explanation)
   BfObject *newCopy();    // Creates a brand new object based on the current one (see method for explanation)
   virtual BfObject *clone() const;


   Game *getGame() const;

   // Manage serial numbers -- every object gets a unique number to help identify it
   void assignNewSerialNumber();
   S32 getSerialNumber();

   virtual void removeFromGame();

   virtual bool processArguments(S32 argc, const char**argv, Game *game);
   virtual string toString(F32 gridSize) const;    // Generates levelcode line for object      --> TODO: Rename to toLevelCode()?

   void onPointsChanged();
   void updateExtentInDatabase();
   virtual void onGeomChanged();    // Item changed geometry (or moved), do any internal updating that might be required
   virtual void onItemDragging();   // Item is being dragged around in the editor; make any updates necessary

   void unselect();

   // Account for the fact that the apparent selection center and actual object center are not quite aligned
   virtual Point getEditorSelectionOffset(F32 currentScale);  

   // Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement seem more natural.
   virtual Point getInitialPlacementOffset(F32 gridSize);

#ifndef ZAP_DEDICATED
   void renderAndLabelHighlightedVertices(F32 currentScale);      // Render selected and highlighted vertices, called from renderEditor
#endif
   virtual void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);


   void setSnapped(bool snapped);                  // Overridden in EngineeredItem 

   ///// Dock related
#ifndef ZAP_DEDICATED
   virtual void prepareForDock(ClientGame *game, const Point &point, S32 teamIndex);
#endif
   virtual void newObjectFromDock(F32 gridSize);   // Called when item dragged from dock to editor -- overridden by several objects

   ///// Dock item rendering methods
   virtual void renderDock();   
   virtual Point getDockLabelPos();
   virtual void highlightDockItem();

   virtual void initializeEditor();

   // For editing attributes:
   virtual EditorAttributeMenuUI *getAttributeMenu();                      // Override in child if it has an attribute menu
   virtual void startEditingAttrs(EditorAttributeMenuUI *attributeMenu);   // Called when we start editing to get menus populated
   virtual void doneEditingAttrs(EditorAttributeMenuUI *attributeMenu);    // Called when we're done to retrieve values set by the menu

   ///// Lua interface
   // Top level Lua methods
   LUAW_DECLARE_CLASS(BfObject);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 getClassID(lua_State *L);

   // Get/set object's position
   virtual S32 getLoc(lua_State *L);
   virtual S32 setLoc(lua_State *L);

   virtual S32 getTeamIndx(lua_State *L);   
   virtual S32 setTeam(lua_State *L);   

   virtual S32 addToGame(lua_State *L);
   virtual S32 removeFromGame(lua_State *L);

   virtual S32 setGeom(lua_State *L);
   virtual S32 getGeom(lua_State *L);

   S32 clone(lua_State *L);
};


////////////////////////////////////////
////////////////////////////////////////
// A trivial extension of the above, to provide special geometry methods for 2D objects
class CentroidObject : public BfObject
{
public:
   // Provide special location handlers
   virtual S32 getLoc(lua_State *L);
   virtual S32 setLoc(lua_State *L);
};



};

#endif

