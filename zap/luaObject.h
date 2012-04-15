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

#ifndef _LUAOBJECT_H_
#define _LUAOBJECT_H_


#include "GameTypesEnum.h"
#include "EventManager.h"

#include "lua.h"
#include "../lua/include/lunar.h"

#include "Point.h"

#include "tnl.h"

#include <string>

using namespace std;
using namespace TNL;


#define method(class, name) {#name, &class::name}

namespace Zap
{

class GameObject;
class LuaPlayerInfo;
class Ship;
class MenuItem;


class LuaObject
{
protected:
   static MenuItem *pushMenuItem (lua_State *L, MenuItem *menuItem);

   // This doesn't really need to be virtual, but something here does, to allow dynamic_casting to occur... I picked
   // this one pretty much arbitrarily...  it won't be overridden.
   virtual void getStringVectorFromTable(lua_State *L, S32 index, const char *methodName, Vector<string> &strings);
   static bool getMenuItemVectorFromTable(lua_State *L, S32 index, const char *methodName, Vector<MenuItem *> &menuItems);

   static Point getPointOrXY(lua_State *L, S32 index, const char *functionName);
   static Point getVec(lua_State *L, S32 index, const char *methodName);

   static void setfield (lua_State *L, const char *key, F32 value);

public:
   LuaObject();            // Constructor
   virtual ~LuaObject();   // Destructor

   // All of these return<T> functions work in the same way.  Include at the end of a child class method.
   // Usage: return returnInt(L, int);

   template<class T> S32 returnVal(lua_State *L, T value, bool letLuaDelete = true);

   // The basics:
   static S32 returnInt(lua_State *L, S32 num);
   static S32 returnVec(lua_State *L, F32 x, F32 y);
   static S32 returnFloat(lua_State *L, F32 num);
   static S32 returnString(lua_State *L, const char *str);
   static S32 returnBool(lua_State *L, bool boolean);
   static S32 returnNil(lua_State *L);

   static void checkArgCount(lua_State *L, S32 argsWanted, const char *methodName);

   // More complex objects:
   static S32 returnPoint(lua_State *L, const Point &point);
   static S32 returnMenuItem(lua_State *L, MenuItem *menuItem);
   static S32 returnShip(lua_State *L, Ship *ship);                // Handles null references properly

   static S32 returnPlayerInfo(lua_State *L, Ship *ship);
   static S32 returnPlayerInfo(lua_State *L, LuaPlayerInfo *playerInfo);


   static void clearStack(lua_State *L);
   static F32 getFloat(lua_State *L, S32 index, const char *methodName);

   static bool getBool(lua_State *L, S32 index, const char *methodName);
   static bool getBool(lua_State *L, S32 index, const char *methodName, bool defaultVal);

   static lua_Integer getInt(lua_State *L, S32 index, const char *methodName);
   static lua_Integer getInt(lua_State *L, S32 index, const char *methodName, S32 defaultVal);
   static lua_Integer getInt(lua_State *L, S32 index, const char *methodName, S32 minVal, S32 maxVal);

   static const char *getString(lua_State *L, S32 index, const char *methodName);
   static const char *getString(lua_State *L, S32 index, const char *methodName, const char *defaultVal);


   static bool dumpStack(lua_State* L);

   static bool shouldLuaGarbageCollectThisObject();     
};


////////////////////////////////////////
////////////////////////////////////////

typedef struct { LuaObject *objectPtr; } UserData;

////////////////////////////////////////
////////////////////////////////////////


class LuaScriptRunner
{
public:
   enum ScriptType {
      ROBOT,
      LEVELGEN,
      PLUGIN
   };

private:
   static string mScriptingDir;
   static bool mScriptingDirSet;

   void setLuaArgs(const Vector<string> &args);
   bool loadHelper(const string &script);    // Tries to load specified file with luaL_loadfile()
   void setModulePath();

   bool configureNewLuaInstance();           // Prepare a new Lua environment for use
   bool loadCompileSaveScript(const string &scriptName, const char *registryKey);

   void setEnums(lua_State *L);  // Set a whole slew of enum values that we want the scripts to have access to

   string mScriptId;             // Unique id for this script
   static U32 mNextScriptId;

protected:
   static lua_State *L;          // Main Lua state variable
   string mScriptName;           // Fully qualified script name, with path and everything
   Vector<string> mScriptArgs;   // List of arguments passed to the script

   virtual bool loadScript();
   bool startLua(ScriptType scriptType);
   bool mSubscriptions[EventManager::EventTypes];  // Keep track of which events we're subscribed to for rapid unsubscription upon death or destruction

   virtual void logError(const char *format, ...) = 0;

   static int luaPanicked(lua_State *L);

   virtual void registerClasses();
   virtual void prepareEnvironment() = 0;
   
   void setEnvironment();

   void deleteScript(const char *name);      // Remove saved script from the Lua registry

   virtual void tickTimer(U32 deltaT);       // Advance script timers

public:
   LuaScriptRunner();               // Constructor
   virtual ~LuaScriptRunner();      // Destructor

   void setScriptingDir(const string &scriptingDir);

   static lua_State *getL();
   static void shutdown();

   bool runMain();                                    // Run a script's main() function
   bool runMain(const Vector<string> &args);          // Run a script's main() function, putting args into Lua's arg table

   bool retrieveFunction(const char *functionName);   // Put specified function on top of the stack, if it's defined

   // Event handling
   int subscribe(lua_State *L);
   int unsubscribe(lua_State *L);

   const char *getScriptId();
   static void loadFunction(lua_State *L, const char *scriptId, const char *functionName);
};


////////////////////////////////////////
////////////////////////////////////////


class LuaItem : public LuaObject
{
public:
   // "= 0" ==> make these methods "pure virtual" functions, and must be implemented in child classes!
   virtual S32 getLoc(lua_State *L) = 0;       // Center of item (returns point)
   virtual S32 getRad(lua_State *L) = 0;       // Radius of item (returns number)
   virtual S32 getVel(lua_State *L) = 0;       // Speed of item (returns point)
   virtual S32 getTeamIndx(lua_State *L) = 0;  // Team of item (returns team index)
   virtual GameObject *getGameObject() = 0;    // Return the underlying GameObject

   virtual void push(lua_State *L);            // Push item onto stack
   virtual S32 getClassID(lua_State *L);       // Object's class

   static LuaItem *getItem(lua_State *L, S32 index, U32 type, const char *functionName);
};

};

#endif

