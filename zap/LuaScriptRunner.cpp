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

#include "LuaScriptRunner.h"   // Header

#include "luaGameInfo.h"
#include "robot.h"             // For subscribing
#include "playerInfo.h"        // For playerInfo def
#include "UIMenuItems.h"       // For MenuItem def
#include "config.h"            
#include "ClientInfo.h"
#include "Console.h"           // For gConsole

#include "luaLevelGenerator.h" // For managing event subscriptions

#include "stringUtils.h"       // For joindir  
#include "lua.h"

#include "tnlLog.h"            // For logprintf

#include <iostream>            // For enum code
#include <sstream>             // For enum code


namespace Zap
{

// This method exists only for lunar support
bool LuaObject::shouldLuaGarbageCollectThisObject() { return true; }


////////////////////////////////////////
////////////////////////////////////////

// Declare and Initialize statics:
lua_State *LuaScriptRunner::L = NULL;
bool  LuaScriptRunner::mScriptingDirSet = false;
string LuaScriptRunner::mScriptingDir;

deque<string> LuaScriptRunner::mCachedScripts;

void LuaScriptRunner::clearScriptCache()
{
	while(mCachedScripts.size() != 0)
	{
		deleteScript(mCachedScripts.front().c_str());
		mCachedScripts.pop_front();
	}
}


// Constructor
LuaScriptRunner::LuaScriptRunner()
{
   static U32 mNextScriptId = 0;

   // Initialize all subscriptions to unsubscribed -- bits will automatically subscribe to onTick later
   for(S32 i = 0; i < EventManager::EventTypes; i++)
      mSubscriptions[i] = false;

   mScriptId = "script" + itos(mNextScriptId++);
}


// Destructor
LuaScriptRunner::~LuaScriptRunner()
{
   // Make sure we're unsubscribed to all those events we subscribed to.  Don't want to
   // send an event to a dead bot, after all...
   for(S32 i = 0; i < EventManager::EventTypes; i++)
      if(mSubscriptions[i])
         EventManager::get()->unsubscribeImmediate(getScriptId(), (EventManager::EventType)i);

   // And delete the script's environment table from the Lua instance
   deleteScript(getScriptId());
}

 const char *LuaScriptRunner::getErrorMessagePrefix() { return "SCRIPT"; }


// Static method setting static vars
void LuaScriptRunner::setScriptingDir(const string &scriptingDir)
{
   mScriptingDir = scriptingDir;
   mScriptingDirSet = true;
}


lua_State *LuaScriptRunner::getL()
{
   TNLAssert(L, "L not yet instantiated!");
   return L;
}


void LuaScriptRunner::shutdown()
{
   if(L)
   {
      lua_close(L);
      L = NULL;
   }
}


const char *LuaScriptRunner::getScriptId()
{
   return mScriptId.c_str();
}


// Sets the environment for the function on the top of the stack to that associated with name
// Starts with a function on the stack
void LuaScriptRunner::setEnvironment()
{                                    
   // Grab the script's environment table from the registry, place it on the stack
   lua_getfield(L, LUA_REGISTRYINDEX, getScriptId());    // Push REGISTRY[scriptId] onto stack           -- function, table
   lua_setfenv(L, -2);                                   // Set that table to be the env for function    -- function
}


// Retrieve the environment from the registry, and put the requested function from that environment onto the stack
void LuaScriptRunner::loadFunction(lua_State *L, const char *scriptId, const char *functionName)
{
   try
   {
      lua_getfield(L, LUA_REGISTRYINDEX, scriptId);   // Push REGISTRY[scriptId] onto the stack                -- table
      lua_getfield(L, -1, functionName);              // And get the requested function from the environment   -- table, function

      lua_remove(L, -2);                              // Remove table                                          -- function
   }

   catch(LuaException &e)
   {
      // TODO: Should be logError()!
      logprintf(LogConsumer::LogError, "Error accessing %s function: %s.  Aborting script.", functionName, e.what());
      LuaObject::clearStack(L);
   }
}


bool LuaScriptRunner::loadAndRunGlobalFunction(lua_State *L, const char *key)
{
   lua_getfield(L, LUA_REGISTRYINDEX, key);     // Get function out of the registry      -- functionName()
   setEnvironment();                            // Set the environment for the code
   S32 err = lua_pcall(L, 0, 0, 0);             // Run it                                 -- <<empty stack>>

   if(err != 0)
   {
      logError("Failed to load startup functions %s: %s", key, lua_tostring(L, -1));

      lua_pop(L, -1);             // Remove error message from stack
      TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");

      return false;
   }

   return true;
}



// Loads script from file into a Lua chunk, then runs it.  This has the effect of loading all our functions into the local environment,
// defining any globals, and executing any "loose" code not defined in a function.
bool LuaScriptRunner::loadScript()
{
   static const S32 MAX_CACHE_SIZE = 2;      // For now -- can be bigger when we know this works

#ifdef ZAP_DEDICATED
   bool cacheScripts = true;
#else
   bool cacheScripts = gServerGame && !gServerGame->isTestServer();
#endif

   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");

   if(!cacheScripts)
      loadCompileScript(mScriptName.c_str());
   else  
   {
      bool found = false;

      // Check if script is in our cache
      S32 cacheSize = (S32)mCachedScripts.size();

      for(S32 i = 0; i < cacheSize; i++)
         if(mCachedScripts[i] == mScriptName)
         {
            found = true;
            break;
         }

      if(!found)     // Script is not (yet) cached
      {
         if(cacheSize > MAX_CACHE_SIZE)
         {
            // Remove oldest script from the cache
            deleteScript(mCachedScripts.front().c_str());
            mCachedScripts.pop_front();
         }

         // Load new script into cache using full name as registry key
         loadCompileSaveScript(mScriptName.c_str(), mScriptName.c_str());
         mCachedScripts.push_back(mScriptName);
      }

      lua_getfield(L, LUA_REGISTRYINDEX, mScriptName.c_str());    // Load script from cache
   }

   if(lua_gettop(L) == 0)     // Script compile error?
   {
      logError("Error compiling script -- aborting.");
         return false;
   }

   // So, however we got here, the script we want to run is now sitting on top of the stack
   TNLAssert((lua_gettop(L) == 1 && lua_isfunction(L, 1)) || LuaObject::dumpStack(L), "Expected a single function on the stack!");

   setEnvironment();

   S32 error = lua_pcall(L, 0, 0, 0);     // Passing 0 args, expecting none back

   if(!error)
   {
      TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");
      return true;
   }

   logError("%s -- Aborting.", lua_tostring(L, -1));

   lua_pop(L, -1);       // Remove error message from stack

   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");
   return false;
}


// Don't forget to update the eventManager after running a robot's main function!
// return false if failed
bool LuaScriptRunner::runMain()
{
   return runMain(mScriptArgs);
}


bool LuaScriptRunner::runMain(const Vector<string> &args)
{
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");
   try 
   {
      // Retrieve the bot's getName function, and put it on top of the stack
      bool ok = retrieveFunction("main");     

      if(!ok)
      {      
         const char *msg = "Function main() could not be found.  This _might_ be OK, but probably isn't.  If it is intentional, please add an empty function called main() to avoid this message.";
         logError(msg);
         throw LuaException(msg);
      }  

      setLuaArgs(args);

      S32 error = lua_pcall(L, 0, 0, 0);     // Passing no args, expecting none back

      if(error != 0)
         throw LuaException(lua_tostring(L, -1));

      TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");
   }

   catch(LuaException &e)
   {
      logError("Error encountered while attempting to run script's main() function: %s.  Aborting script.", e.what());
      LuaObject::clearStack(L);
      return false;
   }

   return true;
}


// Get a function from the currently running script, and place it on top of the stack.  Returns true if it works, false
// if the specified function could not be found.  If this fails will remove the non-function from the stack.
bool LuaScriptRunner::retrieveFunction(const char *functionName)
{
   lua_getfield(L, LUA_REGISTRYINDEX, getScriptId());    // Put script's environment table onto stack  -- table
   lua_pushstring(L, functionName);                      //                                            -- table, functionName
   lua_gettable(L, -2);                                  // Push requested function onto the stack     -- table, fn
   lua_remove(L, lua_gettop(L) - 1);                     // Remove environment table from stack        -- fn

   // Check if the top stack item is indeed a function (as we would expect)
   if(lua_isfunction(L, -1))
      return true;      // If so, return true

   // else
   lua_pop(L, 1);
   return false;
}


bool LuaScriptRunner::startLua()
{
   // Start Lua and get everything configured if we haven't already done so
   if(!L)
   {
      TNLAssert(mScriptingDirSet, "Must set scripting folder before starting Lua interpreter!");

      // Prepare the Lua global environment
      L = lua_open();     // Create a new Lua interpreter; will be shutdown in the destructor

      if(!L)
      {  
         // Failure here is likely to be something systemic, something bad.  Like smallpox.
         logprintf(LogConsumer::LogError, "%s %s", "STARTUP", "Could not create Lua interpreter.  Aborting script.");
         return false;
      }

      if(!configureNewLuaInstance())
      {
         logErrorHandler("Could not configure Lua interpreter.  I cannot run any scripts until the problem is resolved.", "STARTUP");
         lua_close(L);
         L = NULL;
         return false;
      }
   }

   return true;
}


// Prepare a new Lua environment ("L") for use -- now only run once since we only
// have one L which all scripts share
bool LuaScriptRunner::configureNewLuaInstance()
{
   lua_atpanic(L, luaPanicked);  // Register our panic function 

#ifdef USE_PROFILER
   init_profiler(L);
   `
#endif

   luaL_openlibs(L);    // Load the standard libraries
   luaopen_vec(L);      // For vector math (lua-vec)

   setModulePath();

   /*luaL_dostring(L, "local env = setmetatable({}, {__index=function(t,k) if k=='_G' then return nil else return _G[k] end})");*/

   // Define a function for copying the global environment to create a private environment for our scripts to run in
   // TODO: This needs to be a deep copy
   luaL_dostring(L, " function table.copy(t)"
                    "    local u = { }"
                    "    for k, v in pairs(t) do u[k] = v end"
                    "    return setmetatable(u, getmetatable(t))"
                    " end");

   // Load our helper functions and store copies of the compiled code in the registry where we can use them for starting new scripts
   return(loadCompileSaveHelper("lua_helper_functions.lua",      LUA_HELPER_FUNCTIONS_KEY)   &&
          loadCompileSaveHelper("robot_helper_functions.lua",    ROBOT_HELPER_FUNCTIONS_KEY) &&
          loadCompileSaveHelper("levelgen_helper_functions.lua", LEVELGEN_HELPER_FUNCTIONS_KEY));
}


bool LuaScriptRunner::loadCompileSaveHelper(const string &scriptName, const char *registryKey)
{
   return loadCompileSaveScript(joindir(mScriptingDir, scriptName).c_str(), registryKey);
}


// Load script from specified file, compile it, and store it in the registry
bool LuaScriptRunner::loadCompileSaveScript(const char *filename, const char *registryKey)
{
   if(!loadCompileScript(filename))
      return false;

   lua_setfield(L, LUA_REGISTRYINDEX, registryKey);      // Save compiled code in registry

   return true;
}


// Load script and place on top of the stack
bool LuaScriptRunner::loadCompileScript(const char *filename)
{
   S32 err = luaL_loadfile(L, filename);     

   if(err == 0)
      return true;

   logprintf(LogConsumer::LogError, "Error loading script %s: %s.", filename, lua_tostring(L, -1));
   lua_pop(L, 1);    // Remove error message from stack
   return false;
}


// Delete script's environment from the registry -- actually set the registry entry to nil so the table can be collected
void LuaScriptRunner::deleteScript(const char *name)
{
   // If a script is not found, or there is some other problem with the bot (or levelgen), we might get here before our L has been
   // set up.  If L hasn't been defined, there's no point in mucking with the registry, right?
   if(L)    
   {
      lua_pushnil(L);                                       //                             -- nil
      lua_setfield(L, LUA_REGISTRYINDEX, name);             // REGISTRY[scriptId] = nil    -- <<empty stack>>
   }
}


bool LuaScriptRunner::prepareEnvironment()              
{
   if(!L)
   {
      logprintf(LogConsumer::LogError, "%s %s.", getErrorMessagePrefix(), "Lua interpreter doesn't exist.  Aborting environment setup");
      return false;
   }

   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");

   // Register all our classes in the global namespace... they will be copied below when we copy the environment

   registerClasses();            // Register classes -- needs to be differentiated by script type
   registerLooseFunctions(L);    // Register some functions not associated with a particular class

   // Set scads of global vars in the Lua instance that mimic the use of the enums we use everywhere.
   // These will be copied into the script's environment when we run createEnvironment.
   setEnums(L);    

   luaL_dostring(L, "e = table.copy(_G)");               // Copy global environment to create a local scripting environment
   lua_getglobal(L, "e");                                //                                        -- environment e   
   lua_setfield(L, LUA_REGISTRYINDEX, getScriptId());    // Store copied table in the registry     -- <<empty stack>> 

   return true;
}


void LuaScriptRunner::logError(const char *format, ...) 
{ 
   va_list args;
   va_start(args, format);
   char buffer[2048];

   vsnprintf(buffer, sizeof(buffer), format, args);
   va_end(args);

   logErrorHandler(buffer, getErrorMessagePrefix());
}


void LuaScriptRunner::logErrorHandler(const char *msg, const char *prefix) 
{ 
   // Log the error to the logging system and also to the game console
   logprintf(LogConsumer::LogError, "%s %s", prefix, msg);

   printStackTrace(L);

   LuaObject::clearStack(L);
}


static string getStackTraceLine(lua_State *L, S32 level)
{
	lua_Debug ar;
	memset(&ar, 0, sizeof(ar));

	if(!lua_getstack(L, level + 1, &ar) || !lua_getinfo(L, "Snl", &ar))
		return "";

	char str[512];
	dSprintf(str, sizeof(str), "%s(%s:%i)", ar.name, ar.source, ar.currentline);

   return str;    // Implicitly converted to string to avoid passing pointer to deleted buffer
}


void LuaScriptRunner::printStackTrace(lua_State *L)
{
   const int MAX_TRACE_LEN = 20;

   for(S32 level = 0; level < MAX_TRACE_LEN; level++)
   {
	   string str = getStackTraceLine(L, level);
	   if(str == "")
		   break;

	   if(level == 0)
		   logprintf("Stack trace:");

	   logprintf("  %s", str.c_str());
   }
}


// Register classes needed by all script runners
void LuaScriptRunner::registerClasses()
{
   LuaW_Registrar::registerClasses(L);    // Register all objects that use our automatic registration scheme

   // Lunar managed objects, these to be ported to LuaW
   Lunar<LuaGameInfo>::Register(L);
   Lunar<LuaTeamInfo>::Register(L);
   Lunar<LuaPlayerInfo>::Register(L);

   Lunar<LuaWeaponInfo>::Register(L);
   Lunar<LuaModuleInfo>::Register(L);
}


// Hand off any script arguments to Lua, by packing them in the arg table, which is where Lua traditionally stores cmd line args.
// By Lua convention, we'll put the name of the script into the 0th element.
void LuaScriptRunner::setLuaArgs(const Vector<string> &args)
{
   lua_getfield(L, LUA_REGISTRYINDEX, getScriptId()); // Put script's env table onto the stack  -- ..., env_table
   lua_pushliteral(L, "arg");                         //                                        -- ..., env_table, "arg"
   lua_createtable(L, args.size() + 1, 0);            // Create table with predefined slots     -- ..., env_table, "arg", table

   lua_pushstring(L, mScriptName.c_str());            //                                        -- ..., env_table, "arg", table, scriptName
   lua_rawseti(L, -2, 0);                             //                                        -- ..., env_table, "arg", table

   for(S32 i = 0; i < args.size(); i++)
   {
      lua_pushstring(L, args[i].c_str());             //                                        -- ..., env_table, "arg", table, string
      lua_rawseti(L, -2, i + 1);                      //                                        -- ..., env_table, "arg", table
   }
   
   lua_settable(L, -3);                               // Save it: env_table["arg"] = table      -- ..., env_table
   lua_pop(L, 1);                                     // Remove environment table from stack    -- ...
}


// Set up paths so that we can use require to load code in our scripts 
void LuaScriptRunner::setModulePath()   
{
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");

   lua_pushliteral(L, "package");                           // -- "package"
   lua_gettable(L, LUA_GLOBALSINDEX);                       // -- table (value of package global)

   lua_pushliteral(L, "path");                              // -- table, "path"
   lua_pushstring(L, (mScriptingDir + "/?.lua").c_str());   // -- table, "path", mScriptingDir + "/?.lua"
   lua_settable(L, -3);                                     // -- table
   lua_pop(L, 1);                                           // -- <<empty stack>>

   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");
}


// Advance timers by deltaT
void LuaScriptRunner::tickTimer(U32 deltaT)
{
   TNLAssert(false, "Not (yet) implemented!");
}


// Since all calls to lua are now done in protected mode, via lua_pcall, if we get here, we've probably encountered 
// a fatal error such as running out of memory.  Best just to shut the whole thing down.
int LuaScriptRunner::luaPanicked(lua_State *L)
{
   string msg = lua_tostring(L, 1);

   logprintf("Fatal error running Lua code: %s.  Possibly out of memory?  Shutting down Bitfighter.", msg.c_str());

   throw LuaException(msg);
   //shutdownBitfighter();

   return 0;
}


// These will be implemented by children classes, and will funnel back to the doSubscribe and doUnscubscribe methods below
S32 LuaScriptRunner::doSubscribe(lua_State *L)   
{ 
   lua_Integer eventType = getInt(L, -1);

   if(!mSubscriptions[eventType])
   {
      EventManager::get()->subscribe(getScriptId(), (EventManager::EventType)eventType);
      mSubscriptions[eventType] = true;
   }

   clearStack(L);

   return 0;
}


S32 LuaScriptRunner::doUnsubscribe(lua_State *L)
{
   lua_Integer eventType = LuaObject::getInt(L, -1);

   if(mSubscriptions[eventType])
   {
      EventManager::get()->unsubscribe(getScriptId(), (EventManager::EventType)eventType);
      mSubscriptions[eventType] = false;
   }

   LuaObject::clearStack(L);

   return 0;
}

//////////////////////////////////////////////////////

// Define 

#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, logprint,        ARRAYDEF({{ ANY,   END }}), 1 ) \
   METHOD(CLASS, print,           ARRAYDEF({{ ANY,   END }}), 1 ) \
   METHOD(CLASS, getMachineTime,  ARRAYDEF({{        END }}), 1 ) \
   METHOD(CLASS, findFile,        ARRAYDEF({{ STR,   END }}), 1 ) \


GENERATE_LUA_FUNARGS_TABLE(LuaScriptRunner, LUA_METHODS);    
   

void LuaScriptRunner::registerLooseFunctions(lua_State *L)
{
   // Register the functions listed above by generating a series of lines that look like this:
   // lua_register(L, "logprint", logprint);     

#  define REGISTER_LINE(class_, name, profiles, profileCount) \
   lua_register(L, #name, name );

   LUA_METHODS("unused", REGISTER_LINE);

#  undef REGISTER_LINE

   // Override a few Lua functions -- we can do this outside the structure above because they really don't need to be documented
   // Ensure we have a good stream of random numbers until we figure out why Lua's randoms suck so bad (bug reported in 5.1, fixed in 5.2?)
   lua_register(L, "getRandomNumber", getRandomNumber); 
   luaL_dostring(L, "math.random = getRandomNumber");
}

#undef LUA_METHODS


//////////
// What follows is a number of static functions, which will be registered directly with our Lua instance as functions
// that are not related to any particular object, but are just available locally.

static string buildPrintString(lua_State *L)
{
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  string out;

  lua_getglobal(L, "tostring");
  for(i = 1; i <= n; i++) 
  {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
    if(i > 1) 
       out += "\t";

    out += s;
    lua_pop(L, 1);  /* pop result */
  }

  return out;
}


// Write a message to the server logfile
S32 LuaScriptRunner::logprint(lua_State *L)
{
   string str = buildPrintString(L);

   logprintf(LogConsumer::LuaBotMessage, "%s", str.c_str());

   return 0;
}


// This code based directly on Lua's print function, try to replicate functionality
S32 LuaScriptRunner::print(lua_State *L)
{
   string str = buildPrintString(L);
   gConsole.output("%s\n", str.c_str());

  return 0;
}


S32 LuaScriptRunner::getMachineTime(lua_State *L)
{
   return LuaObject::returnInt(L, Platform::getRealMilliseconds());
}


// Find the specified file, in preparation for loading
S32 LuaScriptRunner::findFile(lua_State *L)
{
   static const char *methodName = "findFile()";
   checkArgCount(L, 1, methodName);

   string filename = getString(L, 1, "");

   FolderManager *folderManager = GameSettings::getFolderManager();

   string fullname = folderManager->findScriptFile(filename);     // Looks in luadir, levelgens dir, bots dir

   lua_pop(L, 1);    // Remove passed arg from stack
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");

   if(fullname == "")
   {
      logprintf(LogConsumer::LogError, "Could not find script file \"%s\"...", filename.c_str());
      return returnNil(L);
   }

   return returnString(L, fullname.c_str());
}


// General structure and perculiar error messages taken from lua math lib
S32 LuaScriptRunner::getRandomNumber(lua_State *L)
{
   S32 args = lua_gettop(L);

   if(args == 0)
      return returnFloat(L, TNL::Random::readF());

   if(args == 1)
   {
      S32 max = luaL_checkint(L, 1);
      luaL_argcheck(L, 1 <= max, 1, "interval is empty");
      return returnInt(L, TNL::Random::readI(1, max));
   }

   if(args == 2)
   {
      int min = luaL_checkint(L, 1);
      int max = luaL_checkint(L, 2);
      luaL_argcheck(L, min <= max, 2, "interval is empty");
      return returnInt(L, TNL::Random::readI(min, max));
   }

   else 
      return luaL_error(L, "wrong number of arguments");
}


// Borrowed from http://tdistler.com/2010/09/13/c-enums-in-lua
// Adds an enumerated type into Lua.
//
// L - Lua state.
// tname - The name of the enum type.
// <name:string><value:int> pairs, terminated by a null (0).
//
// EX: Assume the following enum in C-code:
//
//  typedef enum _TYPE { TYPE_FOO=0, TYPE_BAR, TYPE_BAZ, TYPE_MAX } TYPE;
//
// To map this to Lua, do the following:
//
//  add_enum_to_lua( L, "type",
//    "foo", TYPE_FOO,
//    "bar", TYPE_BAR,
//    "baz", TYPE_BAZ,
//    0);
//
// In Lua, you can access the enum as:
//  type.foo
//  type.bar
//  type.baz
//
// You can print the actual value in Lua by:
//  > print(type.foo.value)
//
bool add_enum_to_lua(lua_State* L, const char* tname, ...)
{
    // NOTE: Here's the Lua code we're building and executing to define the
    //       enum.
    //
    // <tname> = setmetatable( {}, { 
    //      __index = { 
    //          <name1> = <value1>, 
    //          }, 
    //          ... 
    //      },
    //      __newindex = function(table, key, value)
    //          error(\"Attempt to modify read-only table\")
    //      end,
    //      __metatable = false
    // });

    va_list args;
    stringstream code;
    char* ename;
    int evalue;
    
    code << tname << " = setmetatable({}, {";
    code << "__index = {";

    // Iterate over the variadic arguments adding the enum values.
    va_start(args, tname);
    while((ename = va_arg(args, char*)) != NULL)
    {
        evalue = va_arg(args, int);
        code << ename << "=" << evalue << ",";
    } 
    va_end(args);

    code << "},";
    code << "__newindex = function(table, key, value) error(\"Attempt to modify read-only table\") end,";
    code << "__metatable = false} )";

    // Execute lua code
    if ( luaL_loadbuffer(L, code.str().c_str(), code.str().length(), 0) || lua_pcall(L, 0, 0, 0) )
    {
        fprintf(stderr, "%s\n\n%s\n", code.str().c_str(),lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    return true;
}



#define setEnum(name)             { lua_pushinteger(L, name);               lua_setglobal(L, #name); }

// Set scads of global vars in the Lua instance that mimic the use of the enums we use everywhere
void LuaScriptRunner::setEnums(lua_State *L)
{
   // Object types -- only push those with shareWithLua set to true
#  define TYPE_NUMBER(value, shareWithLua, name)   if(shareWithLua)  {           \
                                                      lua_pushinteger(L, value); \
                                                      lua_setglobal  (L, name);  \
                                                   }
       TYPE_NUMBER_TABLE
#  undef TYPE_NUMBER


   // Module enums -- push all, using enum name as the Lua name
#  define MODULE_ITEM(value, b, c, d, e, f, g, h, i)  lua_pushinteger(L, value);  \
                                                      lua_setglobal  (L, #value); 
      MODULE_ITEM_TABLE
#  undef EVENT


   // Weapon enums -- push all, using enum name as the Lua name
#  define WEAPON_ITEM(value, b, c, d, e, f, g, h, i, j, k, l)  lua_pushinteger(L, value);  \
                                                               lua_setglobal  (L, #value); 
      WEAPON_ITEM_TABLE
#  undef WEAPON_ITEM

   add_enum_to_lua( L, "Weapons",
#  define WEAPON_ITEM(value, b, luaEnumName, d, e, f, g, h, i, j, k, l)  luaEnumName, value,
      WEAPON_ITEM_TABLE
#  undef WEAPON_ITEM
   (char*)NULL);  // Need to tell the compiler what size we are inputting to prevent possible problems with different compilers, sizeof(NULL) not always the same as sizeof(void*)

   // Game Types
#  define GAME_TYPE_ITEM(name, b, c)  lua_pushinteger(L, name); \
                                      lua_setglobal  (L, #name);
       GAME_TYPE_TABLE
#  undef GAME_TYPE_ITEM

   // Scoring Events
#  define SCORING_EVENT_ITEM(name)  lua_pushinteger(L, GameType::name); \
                                    lua_setglobal  (L, #name);
      SCORING_EVENT_TABLE
#  undef SCORING_EVENT_ITEM


   // Event handler events
#  define EVENT(name, b, c) lua_pushinteger(L, EventManager::name); \
                            lua_setglobal  (L, #name);
      EVENT_TABLE
#  undef EVENT

   setEnum(EngineeredTurret);
   setEnum(EngineeredForceField);
   setEnum(EngineeredTeleporterEntrance);
   setEnum(EngineeredTeleporterExit);

   // A few other misc constants -- in Lua, we reference the teams as first team == 1, so neutral will be 0 and hostile -1
   lua_pushinteger(L, 0);  lua_setglobal(L, "NeutralTeamIndx");
   lua_pushinteger(L, -1); lua_setglobal(L, "HostileTeamIndx");
}

#undef setEnumName
#undef setEnum
#undef setGTEnum
#undef setEventEnum

};


