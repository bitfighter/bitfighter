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

#include "LuaBase.h"          // Parent class
#include "GameTypesEnum.h"
#include "EventManager.h"
#include "LuaWrapper.h"

#include "Point.h"

#include "tnl.h"
#include "tnlVector.h"

#include <deque>
#include <string>

using namespace std;
using namespace TNL;


#define method(class, name)  { #name, &class::name }
#define ARRAYDEF(...) __VA_ARGS__                  // Wrap inline array definitions so they don't confuse the preprocessor   


namespace Zap
{

class BfObject;
class LuaPlayerInfo;
class Ship;
class MenuItem;


class LuaObject : public LuaBase
{
   typedef LuaBase Parent;

public:
   static bool shouldLuaGarbageCollectThisObject();     
};


extern void printFunctions(const ArgMap &argMap, const std::map<ClassName, unsigned int> &nodeMap, 
                              const std::vector<Node> &nodeList, const std::string &prefix, unsigned int nodeIndex);



////////////////////////////////////////
////////////////////////////////////////

typedef struct { LuaObject *objectPtr; } UserData;

////////////////////////////////////////
////////////////////////////////////////


#define LUA_HELPER_FUNCTIONS_KEY      "lua_helper_functions"
#define ROBOT_HELPER_FUNCTIONS_KEY    "robot_helper_functions"
#define LEVELGEN_HELPER_FUNCTIONS_KEY "levelgen_helper_functions"

class LuaScriptRunner : public LuaBase
{

typedef LuaBase Parent;

private:
   static deque<string> mCachedScripts;

   static string mScriptingDir;
   static bool mScriptingDirSet;

   void setLuaArgs(const Vector<string> &args);
   void setModulePath();

   bool configureNewLuaInstance();              // Prepare a new Lua environment for use

   bool loadCompileSaveHelper(const string &scriptName, const char *registryKey);
   bool loadCompileSaveScript(const char *filename, const char *registryKey);
   bool loadCompileScript(const char *filename);

   void registerLooseFunctions(lua_State *L);   // Register some functions not associated with a particular class
   void setEnums(lua_State *L);                 // Set a whole slew of enum values that we want the scripts to have access to


protected:
   static lua_State *L;          // Main Lua state variable
   string mScriptName;           // Fully qualified script name, with path and everything
   Vector<string> mScriptArgs;   // List of arguments passed to the script

   string mScriptId;             // Unique id for this script
   const char *mErrorMsgPrefix;

   virtual bool loadScript();
   bool startLua();
   bool mSubscriptions[EventManager::EventTypes];  // Keep track of which events we're subscribed to for rapid unsubscription upon death or destruction

   // This method should be abstract, but luaW requires us to be able to instantiate this class
   virtual bool prepareEnvironment();

   static void printStackTrace(lua_State *L);


   static int luaPanicked(lua_State *L);

   virtual void registerClasses();
   
   void setEnvironment();

   static void deleteScript(const char *name);     // Remove saved script from the Lua registry

   virtual void tickTimer(U32 deltaT);             // Advance script timers

public:
   LuaScriptRunner();               // Constructor
   virtual ~LuaScriptRunner();      // Destructor

   static void clearScriptCache();

   static void setScriptingDir(const string &scriptingDir);

   static lua_State *getL();
   static void shutdown();

   bool runMain();                                    // Run a script's main() function
   bool runMain(const Vector<string> &args);          // Run a script's main() function, putting args into Lua's arg table

   bool retrieveFunction(const char *functionName);   // Put specified function on top of the stack, if it's defined

   // Event subscriptions
   int subscribe(lua_State *L);
   int unsubscribe(lua_State *L);

   const char *getScriptId();
   static void loadFunction(lua_State *L, const char *scriptId, const char *functionName);
   bool loadAndRunGlobalFunction(lua_State *L, const char *key);

   void logError(const char *format, ...);


   // What follows is a number of static functions, which will be registered directly with our Lua instance as functions
   // that are not related to any particular object, but are just available locally.
   static S32 logprint(lua_State *L);
   static S32 printToConsole(lua_State *L);
   static S32 getMachineTime(lua_State *L);
   static S32 getRandomNumber(lua_State *L);
   static S32 findFile(lua_State *L);

   static const LuaFunctionProfile functionArgs[];
};


////////////////////////////////////////
////////////////////////////////////////
//
// Some ugly macro defs that will make our Lua classes sleek and beautiful
//
////////////////////////////////////////
////////////////////////////////////////
//
// See discussion of this code here:
// http://stackoverflow.com/questions/11413663/reducing-code-repetition-in-c
//
// Start with a definition like the following:
/*
 #define LUA_METHODS(CLASS, METHOD) \
    METHOD(CLASS, addDest,    ARRAYDEF({{ PT,  END }}), 1 ) \
    METHOD(CLASS, delDest,    ARRAYDEF({{ INT, END }}), 1 ) \
    METHOD(CLASS, clearDests, ARRAYDEF({{      END }}), 1 ) \
*/

#define LUA_METHOD_ITEM(class_, name, b, c) \
{ #name, luaW_doMethod<class_, &class_::name > },

#define GENERATE_LUA_METHODS_TABLE(class_, table_) \
const luaL_reg class_::luaMethods[] =              \
{                                                  \
   table_(class_, LUA_METHOD_ITEM)                 \
   { NULL, NULL }                                  \
}

// Generates something like the following:
// const luaL_reg Teleporter::luaMethods[] =
// {
//       { "addDest",    luaW_doMethod<Teleporter, &Teleporter::addDest >    }
//       { "delDest",    luaW_doMethod<Teleporter, &Teleporter::delDest >    }
//       { "clearDests", luaW_doMethod<Teleporter, &Teleporter::clearDests > }
//       { NULL, NULL }
// };

////////////////////////////////////////

#define LUA_FUNARGS_ITEM(class_, name, profiles, profileCount) \
{ #name, profiles, profileCount },

#define GENERATE_LUA_FUNARGS_TABLE(class_, table_)  \
const LuaFunctionProfile class_::functionArgs[] =   \
{                                                   \
   table_(class_, LUA_FUNARGS_ITEM)                 \
   { NULL, { }, 0 }                                 \
}

// Generates something like the following:
// const LuaFunctionProfile Teleporter::functionArgs[] =
// {
//    { "addDest",    {{ PT,  END }}, 1 }
//    { "delDest",    {{ INT, END }}, 1 }
//    { "clearDests", {{      END }}, 1 }
//    { NULL, { }, 0 }
// };

////////////////////////////////////////
////////////////////////////////////////


};

#endif
