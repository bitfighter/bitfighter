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


#ifndef _GAME_ITEMS_H_
#define _GAME_ITEMS_H_

#include "PickupItem.h"
#include "moveObject.h"

//#include "luaObject.h"        // For Lua interfaces
#include "gameObjectRender.h"


namespace Zap
{

class Ship;


class Reactor : public Item
{

typedef Item Parent;

private:
   bool hasExploded;
   U32 mHitPoints;

public:
   Reactor();     // Constructor  
   Reactor *clone() const;

   static const S32 REACTOR_RADIUS = 10;

   void renderItem(const Point &pos);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool getCollisionCircle(U32 state, Point &center, F32 &radius) const;
   bool getCollisionRect(U32 state, Rect &rect) const;
   bool collide(GameObject *otherObject);

   F32 getReactorRadius() const;

   void damageObject(DamageInfo *theInfo);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);
   void onItemExploded(Point pos);

   void setRadius(F32 radius);

   TNL_DECLARE_CLASS(Reactor);

   ///// Editor methods
   const char *getEditorHelpString() { return "Reactor.  Destroy to score."; }
   const char *getPrettyNamePlural() { return "Reactors"; }
   const char *getOnDockName() { return "Rctr"; }
   const char *getOnScreenName() { return "Reactor"; }

   F32 getEditorRadius(F32 currentScale);
   void renderDock();

   ///// Lua interface
public:
   Reactor(lua_State *L);    // Constructor

   static const char className[];

   static Lunar<Reactor>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, ReactorTypeNumber); }

   S32 getHitPoints(lua_State *L);   // Index of current asteroid size (0 = initial size, 1 = next smaller, 2 = ...) (returns int)
   void push(lua_State *L) {  Lunar<Reactor>::push(L, this); }
};



////////////////////////////////////////
////////////////////////////////////////

// Parent class for spawns that generate items
class AbstractSpawn : public EditorPointObject
{
   typedef EditorObject Parent;

protected:
   S32 mSpawnTime;
   Timer mTimer;

   void setRespawnTime(S32 time);

public:
   AbstractSpawn(const Point &pos = Point(), S32 time = 0); // Constructor

   
   virtual bool processArguments(S32 argc, const char **argv, Game *game);

   ///// Editor methods
   virtual const char *getEditorHelpString() = 0;
   virtual const char *getPrettyNamePlural() = 0;
   virtual const char *getOnDockName() = 0;
   virtual const char *getOnScreenName() = 0;

   virtual const char *getClassName() const = 0;

   virtual S32 getDefaultRespawnTime() = 0;

   virtual string toString(F32 gridSize) const;

   Point getPos() const { return getVert(0); }     // For readability 

   F32 getEditorRadius(F32 currentScale);

   bool updateTimer(U32 deltaT) { return mTimer.update(deltaT); }
   void resetTimer() { mTimer.reset(); }
   U32 getPeriod() { return mTimer.getPeriod(); }     // temp debugging

   virtual void renderEditor(F32 currentScale) = 0;
   virtual void renderDock() = 0;
};


class Spawn : public AbstractSpawn
{
public:
   static const S32 DEFAULT_RESPAWN_TIME = 30;    // in seconds

   Spawn(const Point &pos = Point(), S32 time = DEFAULT_RESPAWN_TIME);  // C++ constructor (no lua constructor)
   virtual ~Spawn();
   Spawn *clone() const;

   const char *getEditorHelpString() { return "Location where ships start.  At least one per team is required. [G]"; }
   const char *getPrettyNamePlural() { return "Spawn points"; }
   const char *getOnDockName() { return "Spawn"; }
   const char *getOnScreenName() { return "Spawn"; }

   const char *getClassName() const { return "Spawn"; }

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString(F32 gridSize) const;

   S32 getDefaultRespawnTime() { return 0; }    // Somewhat meaningless in this context

   void renderEditor(F32 currentScale);
   void renderDock();
};


////////////////////////////////////////
////////////////////////////////////////

class ItemSpawn : public AbstractSpawn
{
   typedef AbstractSpawn Parent;

   public:
      ItemSpawn(const Point &pos, S32 time);
      virtual void spawn(Game *game, const Point &pos) = 0;
};

////////////////////////////////////////
////////////////////////////////////////

class AsteroidSpawn : public ItemSpawn    
{
   typedef ItemSpawn Parent;

public:
   static const S32 DEFAULT_RESPAWN_TIME = 30;    // in seconds

   AsteroidSpawn(const Point &pos = Point(), S32 time = DEFAULT_RESPAWN_TIME);  // C++ constructor (no lua constructor)
   virtual ~AsteroidSpawn();
   AsteroidSpawn *clone() const;

   const char *getEditorHelpString() { return "Periodically spawns a new asteroid."; }
   const char *getPrettyNamePlural() { return "Asteroid spawn points"; }
   const char *getOnDockName() { return "ASP"; }
   const char *getOnScreenName() { return "AsteroidSpawn"; }

   const char *getClassName() const { return "AsteroidSpawn"; }

   S32 getDefaultRespawnTime() { return DEFAULT_RESPAWN_TIME; }

   void spawn(Game *game, const Point &pos);
   void renderEditor(F32 currentScale);
   void renderDock();
};


////////////////////////////////////////
////////////////////////////////////////

class CircleSpawn : public ItemSpawn    
{
   typedef ItemSpawn Parent;

public:
   static const S32 DEFAULT_RESPAWN_TIME = 20;    // in seconds

   CircleSpawn(const Point &pos = Point(), S32 time = DEFAULT_RESPAWN_TIME);  // C++ constructor (no lua constructor)
   CircleSpawn *clone() const;

   const char *getEditorHelpString() { return "Periodically spawns a new circle."; }
   const char *getPrettyNamePlural() { return "Circle spawn points"; }
   const char *getOnDockName() { return "CSP"; }
   const char *getOnScreenName() { return "CircleSpawn"; }

   const char *getClassName() const { return "CircleSpawn"; }

   S32 getDefaultRespawnTime() { return DEFAULT_RESPAWN_TIME; }

   void spawn(Game *game, const Point &pos);
   void renderEditor(F32 currentScale);
   void renderDock();
};

////////////////////////////////////////
////////////////////////////////////////

class FlagSpawn : public AbstractSpawn
{
public:
   static const S32 DEFAULT_RESPAWN_TIME = 30;    // in seconds

   FlagSpawn(const Point &pos = Point(), S32 time = DEFAULT_RESPAWN_TIME);  // C++ constructor (no lua constructor)
   virtual ~FlagSpawn();
   FlagSpawn *clone() const;

   bool updateTimer(S32 deltaT) { return mTimer.update(deltaT); }
   void resetTimer() { mTimer.reset(); }

   const char *getEditorHelpString() { return "Location where flags (or balls in Soccer) spawn after capture."; }
   const char *getPrettyNamePlural() { return "Flag spawn points"; }
   const char *getOnDockName() { return "FlagSpawn"; }
   const char *getOnScreenName() { return "FlagSpawn"; }

   const char *getClassName() const { return "FlagSpawn"; }

   S32 getDefaultRespawnTime() { return DEFAULT_RESPAWN_TIME; }

   void spawn(Game *game, const Point &pos);
   void renderEditor(F32 currentScale);
   void renderDock();

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString(F32 gridSize) const;
};


////////////////////////////////////////
////////////////////////////////////////

class Worm : public MoveItem      // But not an editor object!!  -- should be a Projectile?
{
typedef MoveItem Parent;

public:
   static const S32 WORM_RADIUS = 5;

private:
   bool hasExploded;
   F32 mNextAng;
   Timer mDirTimer;

public:
   Worm();     // Constructor  

   void renderItem(const Point &pos);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool getCollisionCircle(U32 state, Point &center, F32 &radius) const;
   bool collide(GameObject *otherObject);
   void setPosAng(Point pos, F32 ang);
   void setNextAng(F32 nextAng) { mNextAng = nextAng; }

   void damageObject(DamageInfo *theInfo);
   void idle(GameObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(Worm);

};


////////////////////////////////////////
////////////////////////////////////////

class TestItem : public MoveItem
{
   typedef MoveItem Parent;

public:
   TestItem();     // Constructor
   TestItem *clone() const;

   static const S32 TEST_ITEM_RADIUS = 60;

   void renderItem(const Point &pos);
   void damageObject(DamageInfo *theInfo);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;

   TNL_DECLARE_CLASS(TestItem);

   ///// Editor methods
   const char *getEditorHelpString() { return "Bouncy object that floats around and gets in the way."; }
   const char *getPrettyNamePlural() { return "TestItems"; }
   const char *getOnDockName() { return "Test"; }
   const char *getOnScreenName() { return "TestItem"; }

   F32 getEditorRadius(F32 currentScale);
   void renderDock();

   ///// Lua Interface

   TestItem(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<TestItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, TestItemTypeNumber); }
   void push(lua_State *L) {  Lunar<TestItem>::push(L, this); }
};


////////////////////////////////////////
////////////////////////////////////////

class ResourceItem : public MoveItem
{
   typedef MoveItem Parent;         // TODO: Should be EditorItem???

public:
   ResourceItem();      // Constructor
   ResourceItem *clone() const;

   static const S32 RESOURCE_ITEM_RADIUS = 20;

   void renderItem(const Point &pos);
   bool collide(GameObject *hitObject);
   void damageObject(DamageInfo *theInfo);
   void onItemDropped();

   TNL_DECLARE_CLASS(ResourceItem);

   ///// Editor methods
   const char *getEditorHelpString() { return "Small bouncy object; capture one to activate Engineer module"; }
   const char *getPrettyNamePlural() { return "ResourceItems"; }
   const char *getOnDockName() { return "Res."; }
   const char *getOnScreenName() { return "ResourceItem"; }

   void renderDock();

   ///// Lua Interface

   ResourceItem(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<ResourceItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, ResourceItemTypeNumber); }
   void push(lua_State *L) {  Lunar<ResourceItem>::push(L, this); }

};

};

#endif

