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

#ifndef _ENGINEEREDITEM_H_
#define _ENGINEEREDITEM_H_

#include "gameObject.h"
#include "item.h"
#include "moveObject.h"    // For MoveItem
#include "barrier.h"

namespace Zap
{

class EngineeredItem : public Item 
{
private:
   typedef Item Parent;

protected:
   F32 mHealth;
   SafePtr<MoveItem> mResource;
   Point mAnchorNormal;
   bool mIsDestroyed;
   S32 mOriginalTeam;

   bool mSnapped;       // Item is snapped to a wall

   S32 mHealRate;       // Rate at which items will heal themselves, defaults to 0
   Timer mHealTimer;    // Timer for tracking mHealRate

   WallSegment *mMountSeg;    // Segment we're mounted to in the editor (don't care in the game)

   enum MaskBits
   {
      InitialMask = Parent::FirstFreeMask << 0,
      HealthMask = Parent::FirstFreeMask << 1,
      TeamMask = Parent::FirstFreeMask << 2,
      FirstFreeMask = Parent::FirstFreeMask << 3,
   };

public:
   EngineeredItem(S32 team = -1, Point anchorPoint = Point(), Point anchorNormal = Point());
   virtual bool processArguments(S32 argc, const char **argv, Game *game);

   virtual void onAddedToGame(Game *theGame);

   static const S32 MAX_SNAP_DISTANCE = 100;    // Max distance to look for a mount point

   void setResource(MoveItem *resource);
   static bool checkDeploymentPosition(const Vector<Point> &thisBounds, GridDatabase *gb);
   void computeExtent();

   virtual void onDestroyed() { /* Do nothing */ }  
   virtual void onDisabled()  { /* Do nothing */ } 
   virtual void onEnabled()   { /* Do nothing */ }  
   virtual bool isTurret()    { return false; }

   bool isEnabled();    // True if still active, false otherwise

   void explode();
   bool isDestroyed() { return mIsDestroyed; }
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void damageObject(DamageInfo *damageInfo);
   bool collide(GameObject *hitObject) { return true; }
   F32 getHealth() { return mHealth; }
   void healObject(S32 time);
   Point mountToWall(const Point &pos, GridDatabase *wallEdgeDatabase, GridDatabase *wallSegmentDatabase);

   // Figure out where to put our turrets and forcefield projectors.  Will return NULL if no mount points found.
   static DatabaseObject *findAnchorPointAndNormal(GridDatabase *db, const Point &pos, F32 snapDist, bool format, 
                                                   Point &anchor, Point &normal);

   static DatabaseObject *findAnchorPointAndNormal(GridDatabase *db, const Point &pos, F32 snapDist, 
                                                   bool format, TestFunc testFunc, Point &anchor, Point &normal);

   void setAnchorNormal(const Point &nrml) { mAnchorNormal = nrml; }
   WallSegment *getMountSegment() { return mMountSeg; }
   void setMountSegment(WallSegment *mountSeg) { mMountSeg = mountSeg; }

   // These methods are overriden in ForceFieldProjector
   virtual WallSegment *getEndSegment() { return NULL; }  
   virtual void setEndSegment(WallSegment *endSegment) { /* Do nothing */ }

   //// Is item sufficiently snapped?  
   void setSnapped(bool snapped) { mSnapped = snapped; }


   /////
   // Editor stuff
   //Point getVert(S32 index) const { return getVert(0); }
   //void setVert(const Point &pos, S32 index) { mGeometry->setVert(pos, 0); }
   
   virtual string toString(F32 gridSize) const;

   /////
   // LuaItem interface
   // S32 getLoc(lua_State *L) { }   ==> Will be implemented by derived objects
   // S32 getRad(lua_State *L) { }   ==> Will be implemented by derived objects
   S32 getVel(lua_State *L) { return LuaObject::returnPoint(L, Point(0, 0)); }   

   // More Lua methods that are inherited by turrets and forcefield projectors
   S32 getTeamIndx(lua_State *L) { return returnInt(L, getTeam() + 1); }
   S32 getHealth(lua_State *L) { return returnFloat(L, mHealth); }
   S32 isActive(lua_State *L) { return returnInt(L, isEnabled()); }
   S32 getAngle(lua_State *L) {return returnFloat(L, mAnchorNormal.ATAN2());};

   GameObject *getGameObject() { return this; }

#ifdef ZAP_DEDICATED
   Point forceFieldEnd;  // would normally be in EditorObject
#endif
};


class ForceField : public GameObject
{
   typedef GameObject Parent;

private:
   Point mStart, mEnd;
   Timer mDownTimer;
   bool mFieldUp;

protected:
   enum MaskBits {
      StatusMask   = Parent::FirstFreeMask << 0,
      FirstFreeMask = Parent::FirstFreeMask << 1
   };

public:
   static const S32 FieldDownTime = 250;
   static const S32 MAX_FORCEFIELD_LENGTH = 2500;


   ForceField(S32 team = -1, Point start = Point(), Point end = Point());
   bool collide(GameObject *hitObject);
   bool intersects(ForceField *forceField);     // Return true if forcefields intersect
   void idle(GameObject::IdleCallPath path);


   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   bool getCollisionPoly(Vector<Point> &polyPoints) const;

   void getGeom(Vector<Point> &geom) const;
   static void getGeom(const Point &start, const Point &end, Vector<Point> &points, F32 scaleFact = 1);
   static bool findForceFieldEnd(GridDatabase *db, const Point &start, const Point &normal, 
                                 Point &end, DatabaseObject **collObj);

   void render();
   S32 getRenderSortValue() { return 0; }

   void getForceFieldStartAndEndPoints(Point &start, Point &end) {start = mStart; end = mEnd;}


   TNL_DECLARE_CLASS(ForceField);
};


class ForceFieldProjector : public EngineeredItem
{
private:
   typedef EngineeredItem Parent;
   SafePtr<ForceField> mField;
   WallSegment *mForceFieldEndSegment;

public:
   static const S32 defaultRespawnTime = 0;

   ForceFieldProjector(S32 team = -1, Point anchorPoint = Point(), Point anchorNormal = Point());  // Constructor
   ForceFieldProjector *clone() const;
   
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   static void getGeom(const Point &anchor, const Point &normal, Vector<Point> &geom);
   static Point getForceFieldStartPoint(const Point &anchor, const Point &normal, F32 scaleFact = 1);

   Vector<Point> getBufferForBotZone();

   // Get info about the forcfield that might be projected from this projector
   void getForceFieldStartAndEndPoints(Point &start, Point &end);

   WallSegment *getEndSegment() { return mForceFieldEndSegment; }  
   void setEndSegment(WallSegment *endSegment) { mForceFieldEndSegment = endSegment; } 

   void onAddedToGame(Game *theGame);
   void idle(GameObject::IdleCallPath path);

   void render();
   void onEnabled();
   void onDisabled();

   TNL_DECLARE_CLASS(ForceFieldProjector);

   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString() { return "Creates a force field that lets only team members pass. [F]"; }  
   const char *getPrettyNamePlural() { return "Force field projectors"; }
   const char *getOnDockName() { return "ForceFld"; }
   const char *getOnScreenName() { return "ForceFld"; }
   bool hasTeam() { return true; }
   bool canBeHostile() { return true; }
   bool canBeNeutral() { return true; }

   Point getEditorSelectionOffset(F32 currentScale);

   void renderDock();
   void renderEditor(F32 currentScale);

   void onItemDragging() { onGeomChanged(); }   // Item is being actively dragged

   void onGeomChanged();
   void findForceFieldEnd();                      // Find end of forcefield in editor

   ///// Lua Interface

   ForceFieldProjector(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<ForceFieldProjector>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, ForceFieldProjectorTypeNumber); }
   void push(lua_State *L) {  Lunar<ForceFieldProjector>::push(L, this); }

   // LuaItem methods
   //S32 getRad(lua_State *L) { return returnInt(L, radius); }
   S32 getLoc(lua_State *L) { return LuaObject::returnPoint(L, getVert(0) + mAnchorNormal * getRadius() ); }
};


////////////////////////////////////////
////////////////////////////////////////

class Turret : public EngineeredItem
{
   typedef EngineeredItem Parent;

private:
   Timer mFireTimer;
   F32 mCurrentAngle;

public:
   Turret(S32 team = -1, Point anchorPoint = Point(), Point anchorNormal = Point(1, 0));     // Constructor
   Turret *clone() const;

   S32 mWeaponFireType;
   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString(F32 gridSize) const;

   static const S32 defaultRespawnTime = 0;

   static const S32 TURRET_OFFSET = 15;   // Distance of the turret's render location from it's attachment location
                                          // Also serves as radius of circle of turret's body, where the turret starts

   enum {
      TurretPerceptionDistance = 800,     // Area to search for potential targets...
      TurretTurnRate = 4,                 // How fast can turrets turn to aim?
      // Turret projectile characteristics (including bullet range) set in gameWeapons.cpp

      AimMask = Parent::FirstFreeMask,
   };


   static void getGeom(const Point &anchor, const Point &normal, Vector<Point> &polyPoints);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;

   Vector<Point> getBufferForBotZone();

   void render();
   void idle(IdleCallPath path);
   void onAddedToGame(Game *theGame);
   bool isTurret() { return true; }

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(Turret);

   /////
   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString() { return "Creates shooting turret.  Can be on a team, neutral, or \"hostile to all\". [Y]"; }  
   const char *getPrettyNamePlural() { return "Turrets"; }
   const char *getOnDockName() { return "Turret"; }
   const char *getOnScreenName() { return "Turret"; }
   bool hasTeam() { return true; }
   bool canBeHostile() { return true; }
   bool canBeNeutral() { return true; }

   Point getEditorSelectionOffset(F32 currentScale);
   void onGeomChanged();
   void onItemDragging() { onGeomChanged(); }

   void renderDock();
   void renderEditor(F32 currentScale);

   ///// 
   //Lua Interface

   Turret(lua_State *L);             //  Lua constructor
   static const char className[];
   static Lunar<Turret>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, TurretTypeNumber); }
   void push(lua_State *L) { Lunar<Turret>::push(L, this); }

   // LuaItem methods
   S32 getRad(lua_State *L) { return returnInt(L, TURRET_OFFSET); }
   S32 getLoc(lua_State *L) { return LuaObject::returnPoint(L, getVert(0) + mAnchorNormal * (TURRET_OFFSET)); }
   S32 getAngleAim(lua_State *L) {return returnFloat(L, mCurrentAngle);};

};


////////////////////////////////////////
////////////////////////////////////////


class EngineerModuleDeployer
{
private:
   Point mDeployPosition, mDeployNormal;
   string mErrorMessage;

public:
   bool canCreateObjectAtLocation(GridDatabase *database, Ship *ship, U32 objectType);    // Check potential deployment position
   bool deployEngineeredItem(GameConnection *connection, U32 objectType);  // Deploy!
   string getErrorMessage() { return mErrorMessage; }

   static bool findDeployPoint(Ship *ship, Point &deployPosition, Point &deployNormal);
   static string checkResourcesAndEnergy(Ship *ship);
};


};
#endif

