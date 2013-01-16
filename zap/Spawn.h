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


#ifndef _SPAWN_H_
#define _SPAWN_H_

#include "EditorObject.h"     // For PointObject def
#include "Timer.h"
#include "LuaWrapper.h"


namespace Zap
{

class EditorAttributeMenuUI;     // Needed in case class def hasn't been included in dedicated build

// Parent class for spawns that generate items
class AbstractSpawn : public PointObject
{
   typedef PointObject Parent;

private:
   static EditorAttributeMenuUI *mAttributeMenuUI;

protected:
   S32 mSpawnTime;
   Timer mTimer;

   void setRespawnTime(F32 time);

public:
   AbstractSpawn(const Point &pos = Point(), S32 time = 0); // Constructor
   AbstractSpawn(const AbstractSpawn &copy);                // Copy constructor
   virtual ~AbstractSpawn();                                // Destructor
   
   virtual bool processArguments(S32 argc, const char **argv, Game *game);

#ifndef ZAP_DEDICATED
   // These four methods are all that's needed to add an editable attribute to a class...
   EditorAttributeMenuUI *getAttributeMenu();
   void startEditingAttrs(EditorAttributeMenuUI *attributeMenu);    // Called when we start editing to get menus populated
   void doneEditingAttrs(EditorAttributeMenuUI *attributeMenu);     // Called when we're done to retrieve values set by the menu

   virtual string getAttributeString();
#endif

   virtual const char *getClassName() const = 0;

   virtual S32 getDefaultRespawnTime() = 0;

   virtual string toLevelCode(F32 gridSize) const;

   F32 getRadius();
   F32 getEditorRadius(F32 currentScale);

   bool updateTimer(U32 deltaT);
   void resetTimer();
   U32 getPeriod();     // temp debugging

   virtual void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled) = 0;
   virtual void renderDock() = 0;
};


////////////////////////////////////////
////////////////////////////////////////

class Spawn : public AbstractSpawn
{
   typedef AbstractSpawn Parent;

private:
   void initialize();

public:
   Spawn(const Point &pos = Point(0,0));  // C++ constructor
   Spawn(lua_State *L);                   // Lua constructor
   virtual ~Spawn();                      // Destructor

   Spawn *clone() const;

   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   const char *getClassName() const;

   string toLevelCode(F32 gridSize) const;
   bool processArguments(S32 argc, const char **argv, Game *game);

   S32 getDefaultRespawnTime();    // Somewhat meaningless in this context

   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   void renderDock();

   TNL_DECLARE_CLASS(Spawn);

   ///// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(Spawn);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


////////////////////////////////////////
////////////////////////////////////////

// Class of spawns that spawn items, rather than places ships might appear
class ItemSpawn : public AbstractSpawn
{
   typedef AbstractSpawn Parent;

public:
   ItemSpawn(const Point &pos, S32 time);    // C++ constructor
   virtual ~ItemSpawn();                     // Destructor

   virtual void spawn();                     // All ItemSpawns will use this to spawn things
   void onAddedToGame(Game *game);
   void idle(IdleCallPath path);

   // These methods exist solely to make ItemSpawn instantiable so it can be instantiated by Lua... even though it never will
   virtual const char *getClassName() const;
   virtual S32 getDefaultRespawnTime();
   virtual void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   virtual void renderDock();


   ///// Lua interface
   LUAW_DECLARE_ABSTRACT_CLASS(ItemSpawn);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_getSpawnTime(lua_State *L);
   S32 lua_setSpawnTime(lua_State *L);
   S32 lua_spawnNow(lua_State *L);
};


////////////////////////////////////////
////////////////////////////////////////

class AsteroidSpawn : public ItemSpawn    
{
   typedef ItemSpawn Parent;

private:
   void initialize();

public:
   static const S32 DEFAULT_RESPAWN_TIME = 30;    // in seconds

   AsteroidSpawn(const Point &pos = Point(), S32 time = DEFAULT_RESPAWN_TIME);  // C++ constructor
   AsteroidSpawn(lua_State *L);                                                 // Lua constructor
   virtual ~AsteroidSpawn();

   AsteroidSpawn *clone() const;

   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   const char *getClassName() const;

   S32 getDefaultRespawnTime();

   void spawn();
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   void renderDock();

   TNL_DECLARE_CLASS(AsteroidSpawn);


   ///// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(AsteroidSpawn);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


////////////////////////////////////////
////////////////////////////////////////

class CircleSpawn : public ItemSpawn    
{
   typedef ItemSpawn Parent;

private:
   void initialize();

public:
   static const S32 DEFAULT_RESPAWN_TIME = 20;    // in seconds

   CircleSpawn(const Point &pos = Point(), S32 time = DEFAULT_RESPAWN_TIME);  // C++ constructor
   CircleSpawn(lua_State *L);                                                 // Lua constructor
   ~CircleSpawn();                                                            // Destructor

   CircleSpawn *clone() const;

   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   const char *getClassName() const;

   S32 getDefaultRespawnTime();

   void spawn();
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   void renderDock();

   TNL_DECLARE_CLASS(CircleSpawn);


   ///// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(CircleSpawn);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


////////////////////////////////////////
////////////////////////////////////////

static const S32 TeamNotSpecified = -99999;

class FlagSpawn : public ItemSpawn
{
   typedef ItemSpawn Parent;

private:
   void initialize();

public:
   TNL_DECLARE_CLASS(FlagSpawn);

   static const S32 DEFAULT_RESPAWN_TIME = 30;    // in seconds

   FlagSpawn(const Point &pos = Point(), S32 time = DEFAULT_RESPAWN_TIME, S32 team = TeamNotSpecified);  // C++ constructor
   FlagSpawn(lua_State *L);                                                                              // Lua constructor
   virtual ~FlagSpawn();                                                                                 // Destructor

   FlagSpawn *clone() const;

   bool updateTimer(S32 deltaT);
   void resetTimer();

   void spawn();

   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   const char *getClassName() const;

   S32 getDefaultRespawnTime();

   //void spawn(Game *game);
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   void renderDock();

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toLevelCode(F32 gridSize) const;

   ///// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(FlagSpawn);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


};    // namespace


#endif
