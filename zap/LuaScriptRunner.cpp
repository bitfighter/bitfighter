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
#include "LuaModule.h"
#include "BfObject.h"
#include "ship.h"
#include "BotNavMeshZone.h"
#include "Engineerable.h"
#include "game.h"
#include "ServerGame.h"
#include "GeomUtils.h"

#include "GameTypesEnum.h"
#include "TeamConstants.h"

#include "config.h"
#include "GameSettings.h"
#include "Console.h"           // For gConsole

#include "stringUtils.h"

#include "../clipper/clipper.hpp"

#include "tnlLog.h"            // For logprintf
#include "tnlRandom.h"

#include <iostream>            // For enum code
#include <sstream>             // For enum code
#include <string>


namespace Zap
{

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
   // These MUST be overriden in child classes
   mLuaGame = NULL;
   mLuaGridDatabase = NULL;

   static U32 mNextScriptId = 0;

   // Initialize all subscriptions to unsubscribed -- bits will automatically subscribe to onTick later
   for(S32 i = 0; i < EventManager::EventTypes; i++)
      mSubscriptions[i] = false;

   mScriptId = "script" + itos(mNextScriptId++);
   mScriptType = ScriptTypeInvalid;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
LuaScriptRunner::~LuaScriptRunner()
{
   // Make sure we're unsubscribed to all those events we subscribed to.  Don't want to
   // send an event to a dead bot, after all...
   for(S32 i = 0; i < EventManager::EventTypes; i++)
      if(mSubscriptions[i])
         EventManager::get()->unsubscribeImmediate(this, (EventManager::EventType)i);

   // And delete the script's environment table from the Lua instance
   deleteScript(getScriptId());

   LUAW_DESTRUCTOR_CLEANUP;
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


// Load the script, execute the chunk to get it in memory, then run its main() function
// Return false if there was an error, true if not
bool LuaScriptRunner::runScript(bool cacheScript)
{
   return prepareEnvironment() && loadScript(cacheScript) && runMain(); 
}


// Sets the environment for the function on the top of the stack to that associated with name
// Starts with a function on the stack
void LuaScriptRunner::setEnvironment()
{                                    
   // Grab the script's environment table from the registry, place it on the stack
   lua_getfield(L, LUA_REGISTRYINDEX, getScriptId());    // Push REGISTRY[scriptId] onto stack           -- function, table
   lua_setfenv(L, -2);                                   // Set that table to be the env for function    -- function
}


// Retrieve the environment from the registry, and put the requested function from that environment onto the stack.  Returns true
// if it works, false if the specified function could not be found.  If this fails, it will remove the non-function from the stack.
// Remember that not every failure to load a function is a problem; some functions are expected but optional.
bool LuaScriptRunner::loadFunction(lua_State *L, const char *scriptId, const char *functionName)
{
   lua_getfield(L, LUA_REGISTRYINDEX, scriptId);   // Push REGISTRY[scriptId] onto the stack                -- table
   lua_getfield(L, -1, functionName);              // And get the requested function from the environment   -- table, function
   lua_remove(L, -2);                              // Remove table                                          -- function

   // Check if the top stack item is indeed a function (as we would expect)
   if(lua_isfunction(L, -1))
      return true;      // If so, return true, leaving the function on top of the stack

   // else
   clearStack(L);
   return false;
}


// Only used for loading helper functions
bool LuaScriptRunner::loadAndRunGlobalFunction(lua_State *L, const char *key, ScriptContext context)
{
   setScriptContext(L, context);

   lua_getfield(L, LUA_REGISTRYINDEX, key);     // Get function out of the registry      -- functionName()
   setEnvironment();                            // Set the environment for the code
   S32 err = lua_pcall(L, 0, 0, 0);             // Run it                                -- <<empty stack>>

   if(err != 0)
   {
      logError("Failed to load startup functions %s: %s", key, lua_tostring(L, -1));

      clearStack(L);
      return false;
   }

   return true;
}


// Load our error handling function -- this will print a pretty stacktrace in the event things go wrong calling function.
// This function can safely throw errors.
void LuaScriptRunner::pushStackTracer()
{
   // _stackTracer is a function included in lua_helper_functions that manages the stack trace; it should ALWAYS be present.
   if(!loadFunction(L, getScriptId(), "_stackTracer"))       
      throw LuaException("Method _stackTracer() could not be found!\n"
                         "Your scripting environment appears corrupted.  Consider reinstalling Bitfighter.");
}


// Loads script from file into a Lua chunk, then runs it.  This has the effect of loading all our functions into the local environment,
// defining any globals, and executing any "loose" code not defined in a function.  If we're going to get any compile errors, they'll
// show up here.
bool LuaScriptRunner::loadScript(bool cacheScript)
{
   static const S32 MAX_CACHE_SIZE = 16;

   // On a dedicated server, we'll always cache our scripts; on a regular server, we'll cache script except when the user is testing
   // from the editor.  In that case, we'll want to see script changes take place immediately, and we're willing to pay a small
   // performance penalty on level load to get that.

   TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack dirty!");

   try
   {
      pushStackTracer();            // -- _stackTracer

      if(!cacheScript)
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


      // If we are here, script loaded and compiled; everything should be dandy.
      TNLAssert((lua_gettop(L) == 2 && lua_isfunction(L, 1) && lua_isfunction(L, 2)) 
                        || dumpStack(L), "Expected a single function on the stack!");

      setEnvironment();

      // The script has been compiled, and the result is sitting on the stack.  The next step is to run it; this executes all the 
      // "loose" code and loads the functions into the current environment.  It does not directly execute any of the functions.
      // Any errors are handed off to the stack tracer we pushed onto the stack earlier.
      if(lua_pcall(L, 0, 0, -2))      // Passing 0 args, expecting none back
         throw LuaException("Error starting script:\n" + string(lua_tostring(L, -1)));

      clearStack(L);    // Remove the _stackTracer from the stack
      return true;
   }
   catch(LuaException &e)
   {
      // We can't load the script as requested.  Sorry!
      logError("%s", e.msg.c_str());                                // Also clears the stack
      return false;
   }

   return false;
}


// Don't forget to update the eventManager after running a robot's main function!
// Returns false if failed
bool LuaScriptRunner::runMain()
{
   return runMain(mScriptArgs);
}


// Takes the passed args, puts them into a Lua table called arg, pushes it on the stack, and runs the "main" function.
bool LuaScriptRunner::runMain(const Vector<string> &args)
{
   TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack dirty!");

   setLuaArgs(args);
   bool error = runCmd("main", 0);
   return !error;
}


// Returns true if there was an error, false if everything ran ok
bool LuaScriptRunner::runCmd(const char *function, S32 returnValues)
{
   try 
   {
      S32 args = lua_gettop(L);  // Number of args on stack     // -- <<args>>

      pushStackTracer();                                        // -- <<args>>, _stackTracer

      if(!loadFunction(L, getScriptId(), function))             // -- <<args>>, _stackTracer, function
         throw LuaException("Cannot load method" + string(function) +"()!\n");

      // Reorder the stack a little
      if(args > 0)
      {
         lua_insert(L, 1);                                      // -- function, <<args>>, _stackTracer
         lua_insert(L, 1);                                      // -- _stackTracer, function, <<args>>
      }

      S32 error = lua_pcall(L, args, returnValues, -2 - args);  // -- _stackTracer, <<return values>>
      if(error)
      {
         string msg = lua_tostring(L, -1);
         lua_pop(L, 1);    // Remove the message from the stack, so it won't appear in our stack dump

         throw LuaException("In method " + string(function) +"():\n" + msg);
      }

      lua_remove(L, 1);    // Remove _stackTracer               // -- <<return values>>

      // Do not clear stack -- caller probably wants <<return values>>
      return false;
   }

   catch(const LuaException &e)
   {
      logprintf(LogConsumer::LogError, "%s\n%s", getErrorMessagePrefix(), e.msg.c_str());
      logprintf(LogConsumer::LogError, "Dump of Lua/C++ stack:");
      dumpStack(L);
      logprintf(LogConsumer::LogError, "Terminating script");

      killScript();
      clearStack(L);
      return true;
   }

   return true;
}


// Start Lua and get everything configured
bool LuaScriptRunner::startLua()
{
   TNLAssert(!L,               "L should not have been created yet!");
   TNLAssert(mScriptingDirSet, "Must set scripting folder before starting Lua interpreter!");

   // Prepare the Lua global environment
   try 
   {
      L = lua_open();               // Create a new Lua interpreter; will be shutdown in the destructor

      // Failure here is likely to be something systemic, something bad.  Like smallpox.
      if(!L)
         throw LuaException("Could not instantiate the Lua interpreter.");

      configureNewLuaInstance();    // Throws any errors it encounters

      return true;
   }

   catch(const LuaException &e)
   {
      // Lua just isn't going to work out for this session.
      logprintf(LogConsumer::LogError, "=====FATAL LUA ERROR=====\n%s\n=========================", e.msg.c_str());
      lua_close(L);
      L = NULL;
      return false;
   }

   return false;
}


// Prepare a new Lua environment ("L") for use -- only called from startLua() above, which has catch block, so we can throw errors
void LuaScriptRunner::configureNewLuaInstance()
{
   lua_atpanic(L, luaPanicked);  // Register our panic function 

#ifdef USE_PROFILER
   init_profiler(L);
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
   loadCompileSaveHelper("lua_helper_functions.lua",      LUA_HELPER_FUNCTIONS_KEY);
   loadCompileSaveHelper("robot_helper_functions.lua",    ROBOT_HELPER_FUNCTIONS_KEY);
   loadCompileSaveHelper("levelgen_helper_functions.lua", LEVELGEN_HELPER_FUNCTIONS_KEY);

   // Register all our classes in the global namespace... they will be copied below when we copy the environment

   registerClasses();            // Perform class and global function registration once per lua_State
   registerLooseFunctions(L);    // Register some functions not associated with a particular class

   // Set scads of global vars in the Lua instance that mimic the use of the enums we use everywhere.
   // These will be copied into the script's environment when we run createEnvironment.
   setEnums(L);
   setGlobalObjectArrays(L);
}


void LuaScriptRunner::loadCompileSaveHelper(const string &scriptName, const char *registryKey)
{
   loadCompileSaveScript(joindir(mScriptingDir, scriptName).c_str(), registryKey);
}


// Load script from specified file, compile it, and store it in the registry.
// All callers of this script have catch blocks, so we can throw errors if something goes wrong.
void LuaScriptRunner::loadCompileSaveScript(const char *filename, const char *registryKey)
{
   loadCompileScript(filename);                       // Throws if there is an error
   lua_setfield(L, LUA_REGISTRYINDEX, registryKey);   // Save compiled code in registry
}


// Load script and place on top of the stack.
// All callers of this script have catch blocks, so we can throw errors if something goes wrong.
void LuaScriptRunner::loadCompileScript(const char *filename)
{
   // luaL_loadfile: Loads a file as a Lua chunk. This function uses lua_load to load the chunk in the file named filename. 
   // If filename is NULL, then it loads from the standard input. The first line in the file is ignored if it starts with a #.
   // Returns:
   // 0: no errors;
   // LUA_ERRSYNTAX: syntax error during pre-compilation;  [[ err == 3 ]]
   // LUA_ERRMEM: memory allocation error.  [[ err == 4 ]]

   if(luaL_loadfile(L, filename) != 0)
      throw LuaException("Error compiling script " + string(filename) + "\n" + string(lua_tostring(L, -1)));
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


// This is our sandboxed Lua environment that gets set as the global environment for any
// user script that is run.
//
// This is basically a whitelist of Lua functions known to be safe, taken from here:
//    http://lua-users.org/wiki/SandBoxes
static const char *sandbox_env =
      "sandbox_e = {"
      "  ipairs = ipairs,"
      "  next = next,"
      "  pairs = pairs,"
      "  pcall = pcall,"
      "  select = select,"
      "  tonumber = tonumber,"
      "  tostring = tostring,"
      "  type = type,"
      "  unpack = unpack,"
      "  coroutine = { create = coroutine.create, resume = coroutine.resume,"
      "     running = coroutine.running, status = coroutine.status, "
      "     wrap = coroutine.wrap },"
      "  string = { byte = string.byte, char = string.char, find = string.find,"
      "     format = string.format, gmatch = string.gmatch, gsub = string.gsub,"
      "     len = string.len, lower = string.lower, match = string.match,"
      "     rep = string.rep, reverse = string.reverse, sub = string.sub,"
      "     upper = string.upper },"
      "  table = { insert = table.insert, maxn = table.maxn, remove = table.remove,"
      "     sort = table.sort },"
      "  math = { abs = math.abs, acos = math.acos, asin = math.asin,"
      "     atan = math.atan, atan2 = math.atan2, ceil = math.ceil, cos = math.cos,"
      "     cosh = math.cosh, deg = math.deg, exp = math.exp, floor = math.floor,"
      "     fmod = math.fmod, frexp = math.frexp, huge = math.huge,"
      "     ldexp = math.ldexp, log = math.log, log10 = math.log10, max = math.max,"
      "     min = math.min, modf = math.modf, pi = math.pi, pow = math.pow,"
      "     rad = math.rad, sin = math.sin, sinh = math.sinh, sqrt = math.sqrt,"
      "     tan = math.tan, tanh = math.tanh,"
      "     random = math.random,"   // This points to our math.random override in registerLooseFunctions()
      "  },"
      "  os = { clock = os.clock, difftime = os.difftime, time = os.time },"
      "}"
      "";


bool LuaScriptRunner::prepareEnvironment()              
{
   if(!L)
   {
      logprintf(LogConsumer::LogError, "%s %s.", getErrorMessagePrefix(), "Lua interpreter doesn't exist.  Aborting environment setup");
      return false;
   }

   TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack dirty!");

   // First register our sandbox as the global namespace
   luaL_dostring(L, sandbox_env);
   lua_getglobal(L, "sandbox_e");                        // -- environment sandbox_e
   lua_setfield(L, LUA_REGISTRYINDEX, getScriptId());    // -- <<empty stack>>

   luaL_dostring(L, "e = table.copy(_G)");               // Copy global environment to create a local scripting environment
   lua_getglobal(L, "e");                                //                                        -- environment e   
   lua_setfield(L, LUA_REGISTRYINDEX, getScriptId());    // Store copied table in the registry     -- <<empty stack>> 
   lua_pushnil(L);                                       //
   lua_setglobal(L, "e");                                //

   // Make non-static Lua methods in this class available via "bf".  We have to static_cast here
   // because it is possible for two 'setSelf' methods to be called from a subclass that calls
   // its own prepareEnvironment() method and subsequently this one (its parent), e.g. in the
   // case of bots.
   setSelf(L, static_cast<LuaScriptRunner*>(this), "bf");

   return true;
}


void LuaScriptRunner::killScript()
{
   TNLAssert(false, "Not implemented for this class");
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

   clearStack(L);
}

/*
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
*/

// Register classes needed by all script runners
void LuaScriptRunner::registerClasses()
{
   LuaW_Registrar::registerClasses(L);    // Register all objects that use our automatic registration scheme
}


// Hand off any script arguments to Lua, by packing them in the arg table, which is where Lua traditionally stores cmd line args.
// By Lua convention, we'll put the name of the script into the 0th element.
void LuaScriptRunner::setLuaArgs(const Vector<string> &args)
{
   S32 stackDepth = lua_gettop(L);

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

   TNLAssert(stackDepth == lua_gettop(L), "Stack not properly restored to the state it was in when we got here!");
}


// Set up paths so that we can use require to load code in our scripts 
void LuaScriptRunner::setModulePath()   
{
   TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack dirty!");

   lua_pushliteral(L, "package");                           // -- "package"
   lua_gettable(L, LUA_GLOBALSINDEX);                       // -- table (value of package global)

   lua_pushliteral(L, "path");                              // -- table, "path"
   lua_pushstring(L, (mScriptingDir + "/?.lua").c_str());   // -- table, "path", mScriptingDir + "/?.lua"
   lua_settable(L, -3);                                     // -- table
   lua_pop(L, 1);                                           // -- <<empty stack>>

   TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack not cleared!");
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


// Called by various children classes
S32 LuaScriptRunner::findObjectById(lua_State *L, const Vector<DatabaseObject *> *objects)
{
   S32 id = getInt(L, 1);

   for(S32 i = 0; i < objects->size(); i++)
   {
      BfObject *bfObject = static_cast<BfObject *>(objects->get(i));
      if(bfObject->getUserAssignedId() == id)
         return returnBfObject(L, bfObject);
   }

   return returnNil(L);
}


// These will be implemented by children classes, and will funnel back to the doSubscribe and doUnscubscribe methods below
S32 LuaScriptRunner::doSubscribe(lua_State *L, ScriptContext context)   
{ 
   lua_Integer eventType = getInt(L, -1);

   if(!mSubscriptions[eventType])
   {
      EventManager::get()->subscribe(this, (EventManager::EventType)eventType, context);
      mSubscriptions[eventType] = true;
   }

   clearStack(L);

   return 0;
}


S32 LuaScriptRunner::doUnsubscribe(lua_State *L)
{
   lua_Integer eventType = getInt(L, -1);

   if(mSubscriptions[eventType])
   {
      EventManager::get()->unsubscribe(this, (EventManager::EventType)eventType);
      mSubscriptions[eventType] = false;
   }

   clearStack(L);

   return 0;
}

//////////////////////////////////////////////////////


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
   S32 evalue;
   bool include;
    
   code << tname << " = setmetatable({}, {";
   code << "__index = {";

   // Iterate over the variadic arguments adding the enum values.
   va_start(args, tname);
   while((ename = va_arg(args, char*)) != NULL)
   {
      include = va_arg(args, S32);
      evalue = va_arg(args, S32);
      if(include)
         code << ename << "=" << evalue << ",";
   } 
   va_end(args);

   code << "},";
   code << "__newindex = function(table, key, value) error(\"Attempt to modify read-only table\") end,";
   code << "__metatable = false} )";

   // Execute lua code
   if( luaL_loadbuffer(L, code.str().c_str(), code.str().length(), 0) || lua_pcall(L, 0, 0, 0) )
   {
      fprintf(stderr, "%s\n\n%s\n", code.str().c_str(), lua_tostring(L, -1));
      lua_pop(L, 1);
      return false;
   }

   return true;
}


#define setEnum(name)             { lua_pushinteger(L, name);   lua_setglobal(L, #name); }

// Set scads of global vars in the Lua instance that mimic the use of the enums we use everywhere
void LuaScriptRunner::setEnums(lua_State *L)
{
   // Note for casting of NULL below:
   // Need to tell the compiler what size we are inputting to prevent possible problems with different compilers, sizeof(NULL) not always the same as sizeof(void*)
   // New way

   // Object types -- only push those with shareWithLua set to true
   add_enum_to_lua(L, "ObjType",
   #  define TYPE_NUMBER(value, shareWithLua, luaEnumName, e)   luaEnumName, shareWithLua, value,
          TYPE_NUMBER_TABLE
   #  undef TYPE_NUMBER
      (char*)NULL); 


   // Module enums -- push all, using enum name as the Lua name
   add_enum_to_lua(L, "Module",
   #  define MODULE_ITEM(value, luaEnumName, c, d, e, f, g, h, i)  luaEnumName, true, value,
         MODULE_ITEM_TABLE
   #  undef MODULE_ITEM
      (char*)NULL); 


   // Weapons
   // Add ModuleCount as offset so we can tell weapons and modules apart when changing loadout
   add_enum_to_lua(L, "Weapon",
   #  define WEAPON_ITEM(value, b, luaEnumName, d, e, f, g, h, i, j, k, l)  luaEnumName, true, value + ModuleCount,
         WEAPON_ITEM_TABLE
   #  undef WEAPON_ITEM
      (char*)NULL);  


   // Game Types
   add_enum_to_lua(L, "GameType",
   #  define GAME_TYPE_ITEM(value, b, luaEnumName, d)  luaEnumName, true, value,
          GAME_TYPE_TABLE
   #  undef GAME_TYPE_ITEM
      (char*)NULL);  


   // Scoring Events
   add_enum_to_lua(L, "ScoringEvent",
   #  define SCORING_EVENT_ITEM(value, luaEnumName)  luaEnumName, true, value,
         SCORING_EVENT_TABLE
   #  undef SCORING_EVENT_ITEM
      (char*)NULL);


   // Event handler events -- not sure if we need this one
   add_enum_to_lua(L, "Event",
   #  define EVENT(value, luaEnumName, c, d) luaEnumName, true, EventManager::value,
         EVENT_TABLE
   #  undef EVENT
      (char*)NULL);


   // Engineerable objects
   add_enum_to_lua(L, "EngineerBuildObject",
   #  define ENGR_OBJ(value, luaEnumName, c) luaEnumName, true, value,
         ENGINEER_BUILD_OBJECTS_TABLE
   #  undef ENGR_OBJ
      (char*)NULL);


   // Polygon boolean operations
   add_enum_to_lua(L, "ClipType",
   #  define CLIP_TYPE_ITEM(luaEnumName, value) #luaEnumName, true, value,
         CLIP_TYPE_TABLE
   #  undef CLIP_TYPE_ITEM
      (char*)NULL);

   // TODO: Document this one!!

   // Create a table at level run time that has team, indexed by name?
   // Team.blue, Team.red, Team.neutral, etc.
   // A few other misc constants -- in Lua, we reference the teams as first team == 1 (1-indexed)
   // but we'll sent neutral (-1) and hostile (-2) as they are from c++
   add_enum_to_lua(L, "Team", "Neutral", true, (TEAM_NEUTRAL),
                              "Hostile", true, (TEAM_HOSTILE),
                              (char*)NULL);
}

#undef setEnumName
#undef setEnum
#undef setGTEnum
#undef setEventEnum


void LuaScriptRunner::setGlobalObjectArrays(lua_State *L)
{
   // TODO:  Some how be more dynamic with the creation of these tables

   // ModuleInfo
   lua_newtable(L);                                // table

   for(S32 i = 0; i < ModuleCount; i++)
   {
      lua_pushinteger(L, i);                       // table, index
      lua_newtable(L);                             // table, index, table

      lua_pushstring(L, "name");                   // table, index, table, key
      lua_pushstring(L, gModuleInfo[i].mName);     // table, index, table, key, value
      lua_rawset(L, -3);                           // table, index, table

      lua_pushstring(L, "classId");                // table, index, table, key
      lua_pushinteger(L, i);                       // table, index, table, key, value
      lua_rawset(L, -3);                           // table, index, table

      lua_rawset(L, -3);                           // table
   }

   lua_setglobal(L, "ModuleInfo");


   // WeaponInfo
   lua_newtable(L);                                // table

   for(S32 i = 0; i < WeaponCount; i++)
   {
      WeaponInfo weaponInfo = WeaponInfo::getWeaponInfo(WeaponType(i));

      lua_pushinteger(L, i + ModuleCount);         // table, index
      lua_newtable(L);                             // table, index, table

      lua_pushstring(L, "name");
      lua_pushstring(L, weaponInfo.name.getString());
      lua_rawset(L, -3);

      lua_pushstring(L, "classId");
      lua_pushinteger(L, i);
      lua_rawset(L, -3);

      lua_pushstring(L, "fireDelay");
      lua_pushinteger(L, weaponInfo.fireDelay);
      lua_rawset(L, -3);

      lua_pushstring(L, "minEnergy");
      lua_pushinteger(L, weaponInfo.minEnergy);
      lua_rawset(L, -3);

      lua_pushstring(L, "energyDrain");
      lua_pushinteger(L, weaponInfo.drainEnergy);
      lua_rawset(L, -3);

      lua_pushstring(L, "projectileVelocity");
      lua_pushinteger(L, weaponInfo.projVelocity);
      lua_rawset(L, -3);

      lua_pushstring(L, "projectileLifeTime");
      lua_pushinteger(L, weaponInfo.projLiveTime);
      lua_rawset(L, -3);

      lua_pushstring(L, "damage");
      lua_pushnumber(L, weaponInfo.damageAmount);
      lua_rawset(L, -3);

      lua_pushstring(L, "damageSelf");
      lua_pushnumber(L, weaponInfo.damageAmount * weaponInfo.damageSelfMultiplier);
      lua_rawset(L, -3);

      lua_pushstring(L, "canDamageTeammate");
      lua_pushboolean(L, weaponInfo.canDamageTeammate);
      lua_rawset(L, -3);

      lua_rawset(L, -3);                           // table
   }

   lua_setglobal(L, "WeaponInfo");
}


//// Lua interface
/**
 * @luaclass LuaScriptRunner
 * 
 * @brief Main class for holding global methods accessible by all script runners
 * 
 * @descr Script runners include levelgens, robots, and editor plugins. The
 * methods here can be called from all three. However, some may be disabled for
 * a particular script runner.
 */

/**
 * These non-static methods work with in-game objects and can be dependent on script type and
 * what 'Game' it's called from (i.e. ServerGame or ClientGame)
 */
//               Fn name    Param profiles         Profile count
#define LUA_METHODS(CLASS, METHOD) \
      METHOD(CLASS, pointCanSeePoint,  ARRAYDEF({{ PT, PT, END }}), 1 ) \
      METHOD(CLASS, findObjectById,    ARRAYDEF({{ INT, END }}), 1 )    \
      METHOD(CLASS, findAllObjects,    ARRAYDEF({{ TABLE, INTS, END }, { INTS, END }}), 2 ) \
      METHOD(CLASS, findAllObjectsInArea,  ARRAYDEF({{ TABLE, PT, PT, INTS, END }, { PT, PT, INTS, END }}), 2 ) \
      METHOD(CLASS, addItem,           ARRAYDEF({{ BFOBJ, END }}), 1 )  \
      METHOD(CLASS, getGameInfo,       ARRAYDEF({{ END }}), 1 )         \
      METHOD(CLASS, getPlayerCount,    ARRAYDEF({{ END }}), 1 )         \
      METHOD(CLASS, subscribe,         ARRAYDEF({{ EVENT, END }}), 1 )  \
      METHOD(CLASS, unsubscribe,       ARRAYDEF({{ EVENT, END }}), 1 )  \


GENERATE_LUA_FUNARGS_TABLE(LuaScriptRunner, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(LuaScriptRunner, LUA_METHODS);

void LuaScriptRunner::registerLooseFunctions(lua_State *L)
{
   ProfileMap moduleProfiles = LuaModuleRegistrarBase::getModuleProfiles();

   ProfileMap::iterator it;
   for(it = moduleProfiles.begin(); it != moduleProfiles.end(); it++)
   {
      if((*it).first == "global")
      {
         vector<LuaStaticFunctionProfile> &profiles = (*it).second;
         for(U32 i = 0; i < profiles.size(); i++)
         {
            LuaStaticFunctionProfile &profile = profiles[i];
            lua_pushcfunction(L, profile.function);                 // -- fn
            lua_setglobal(L, profile.functionName);                 // --
         }
      }
      else
      {
         lua_createtable(L, 0, 0);                                  // -- table

         vector<LuaStaticFunctionProfile> &profiles = (*it).second;
         for(U32 i = 0; i < profiles.size(); i++)
         {
            LuaStaticFunctionProfile &profile = profiles[i];
            lua_pushcfunction(L, profile.function);                 // -- table, fn
            lua_setfield(L, -2, profile.functionName);              // -- table
         }
         lua_setglobal(L, (*it).first.c_str());                     // --
      }
   }

   // Override a few Lua functions -- we can do this outside the structure above because they really don't need to be documented
   // Ensure we have a good stream of random numbers until we figure out why Lua's randoms suck so bad (bug reported in 5.1, fixed in 5.2?)
   luaL_dostring(L, "math.random = getRandomNumber");
}

#undef LUA_NON_STATIC_METHODS
#undef LUA_METHODS


const char *LuaScriptRunner::luaClassName = "LuaScriptRunner";
REGISTER_LUA_CLASS(LuaScriptRunner);


/**
 * @luafunc bool LuaScriptRunner::pointCanSeePoint(point point1, point point2)
 * 
 * @brief Returns `true` if the two specified points can see one another.
 * 
 * @param point1 First point.
 * @param point2 Second point.
 * 
 * @return `true` if objects have a line of sight from one to the other,
 * `false`otherwise.
 */
S32 LuaScriptRunner::lua_pointCanSeePoint(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "pointCanSeePoint");

   Point p1 = getPointOrXY(L, 1);
   Point p2 = getPointOrXY(L, 2);

   TNLAssert(mLuaGridDatabase != NULL, "Grid Database must not be NULL!");

   return returnBool(L, mLuaGridDatabase->pointCanSeePoint(p1, p2));
}


/**
 * @luafunc BfObject LuaScriptRunner::findObjectById(num id)
 * 
 * @brief Returns an object with the given id, or nil if none exists.
 * 
 * @descr Finds an object with the specified user-assigned id. If there are
 * multiple objects with the same id (shouldn't happen, but could, especially if
 * the passed id is 0), this method will return the first object it finds with
 * the given id. Currently, all objects that have not been explicitly assigned
 * an id have an id of 0.
 * 
 * Note that ids can be assigned in the editor using the ! or # keys.
 * 
 * @param id id to search for.
 * 
 * @return The found BfObject, or `nil` if no objects with the specified id
 * could be found.
 */
S32 LuaScriptRunner::lua_findObjectById(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "findObjectById");

   TNLAssert(mLuaGame != NULL, "Game must not be NULL!");

   return findObjectById(L, mLuaGame->getGameObjDatabase()->findObjects_fast());
}


static void checkFillTable(lua_State *L, S32 size)
{
   // We are expecting a table to be on top of the stack when we get here.  If not, we can add one.
   if(!lua_istable(L, -1))
   {
      TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack not cleared!");

      logprintf(LogConsumer::LogWarning,
                  "Finding objects will be far more efficient if your script provides a table -- see scripting docs for details!");
      lua_createtable(L, size, 0);    // Create a table, with enough slots pre-allocated for our data
   }

   TNLAssert((lua_gettop(L) == 1 && lua_istable(L, -1)) || dumpStack(L), "Should only have table!");
}


/**
 * @luafunc table LuaScriptRunner::findAllObjects(table results, ObjType objType, ...)
 *
 * @brief Finds all items of the specified type anywhere on the level.
 *
 * @descr Can specify multiple types. The table argument is optional, but
 * levelgens that call this function frequently will perform better if they
 * provide a reusable table in which found objects can be stored. By providing a
 * table, you will avoid incurring the overhead of construction and destruction
 * of a new one.
 *
 * If a table is not provided, the function will create a table and return it on
 * the stack.
 *
 * @param results (optional) Reusable table into which results can be written.
 * @param objType One or more ObjTypes specifying what types of objects to find.
 *
 * @return A reference back to the passed table, or a new table if one was not
 * provided.
 *
 * @code
 * items = { } -- Reusable container for findAllObjects. Because it is defined outside
 *             -- any functions, it will have global scope.
 *
 * function countObjects(objType, ...) -- Pass one or more object types
 *   table.clear(items) -- Remove any items in table from previous use
 *   levelgen:findGlobalObjects(items, objType, ...) -- Put all items of specified type(s) into items table
 *   print(#items) -- Print the number of items found to the console
 * end
 * @endcode
 */
S32 LuaScriptRunner::lua_findAllObjects(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "findAllObjects");

   TNLAssert(mLuaGridDatabase != NULL, "Grid Database must not be NULL!");

   fillVector.clear();
   static Vector<U8> types;

   types.clear();

   // We expect the stack to look like this: -- [fillTable], objType1, objType2, ...
   // We'll work our way down from the top of the stack (element -1) until we find something that is not a number.
   // We expect that when we find something that is not a number, the stack will only contain our fillTable.  If the stack
   // is empty at that point, we'll add a table, and warn the user that they are using a less efficient method.
   while(lua_isnumber(L, -1))
   {
      U8 typenum = (U8)lua_tointeger(L, -1);

      // Requests for botzones have to be handled separately; not a problem, we'll just do the search here, and add them to
      // fillVector, where they'll be merged with the rest of our search results.
      if(typenum != BotNavMeshZoneTypeNumber)
         types.push_back(typenum);
      else
         BotNavMeshZone::getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, fillVector);

      lua_pop(L, 1);
   }

   mLuaGridDatabase->findObjects(types, fillVector);

   // This will guarantee a table at the top of the stack
   checkFillTable(L, fillVector.size());

   S32 pushed = 0;      // Count of items we put into our table

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      static_cast<BfObject *>(fillVector[i])->push(L);
      pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   TNLAssert(lua_gettop(L) == 1 || dumpStack(L), "Stack has unexpected items on it!");

   return 1;
}


/**
 * @luafunc table LuaScriptRunner::findAllObjectsInArea(table results, point point1, point point2, ObjType objType, ...)
 *
 * @brief Finds all items of the specified type(s) in a given search area.
 *
 * @descr Multiple object types can be specified. A search rectangle will be
 * constructed from the two points given, with each point positioned at opposite
 * corners. A reusable fill table can be given to increase performance if this
 * method is called frequently.
 *
 * @note See LuaScriptRunner::findAllObjects for a code example with using a
 * fill table.
 *
 * @param results (Optional) A reusable fill table.
 * @param point1 One corner of a search rectangle.
 * @param point2 Another corner of a search rectangle diagonally opposite to the
 * first.
 * @param objType The \ref ObjTypeEnum to look for. Multiple can be specified.
 *
 * @return A table with any found objects.
 */
S32 LuaScriptRunner::lua_findAllObjectsInArea(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "findAllObjectsInArea");

   TNLAssert(mLuaGridDatabase != NULL, "Grid Database must not be NULL!");

   static Vector<U8> types;

   types.clear();
   fillVector.clear();

   bool hasBotZoneType = false;

   // We expect the stack to look like this: -- [fillTable], objType1, objType2, ...
   // We'll work our way down from the top of the stack (element -1) until we find something that is not a number.
   while(lua_isnumber(L, -1))
   {
      U8 typenum = (U8)lua_tointeger(L, -1);

      // Requests for botzones have to be handled separately
      if(typenum != BotNavMeshZoneTypeNumber)
         types.push_back(typenum);
      else
         hasBotZoneType = true;

      lua_pop(L, 1);
   }

   // We should be left with 2 points and maybe a table
   Point p1 = getPointOrXY(L, -1);
   Point p2 = getPointOrXY(L, -2);
   lua_pop(L, 2);

   Rect searchArea = Rect(p1, p2);

   if(hasBotZoneType)
      BotNavMeshZone::getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, fillVector, searchArea);

   mLuaGridDatabase->findObjects(types, fillVector, searchArea);

   // This will guarantee a table at the top of the stack
   checkFillTable(L, fillVector.size());


   S32 pushed = 0;      // Count of items we put into our table

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      static_cast<BfObject *>(fillVector[i])->push(L);
      pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   TNLAssert(lua_gettop(L) == 1 || dumpStack(L), "Stack has unexpected items on it!");

   return 1;
}


/**
 * @luafunc LuaScriptRunner::addItem(BfObject obj)
 *
 * @brief Add a BfObject to the game or editor. Any object constructed in a
 * levelgen will not appear in the game world or editor until this method is
 * called on it.
 *
 * @param obj Any BfObject to be added to the editor
 */
S32 LuaScriptRunner::lua_addItem(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "addItem");

   TNLAssert(mLuaGame != NULL, "Game must not be NULL!");
   TNLAssert(mLuaGridDatabase != NULL, "Grid Database must not be NULL!");

   // First check to see if item is a BfObject
   BfObject *obj = luaW_check<BfObject>(L, 1);
   lua_pop(L, 1);

   if(obj)
   {
      // Silently ignore illegal items when being run from the editor.  For the moment, if mGame is not a server, then
      // we are running from the editor.  This could conceivably change, but for the moment it seems to hold true.
      if(mLuaGame->isServer() || obj->canAddToEditor())
      {
         // Some objects require special handling
         if(obj->getObjectTypeNumber() == PolyWallTypeNumber)
            mLuaGame->addPolyWall(obj, mLuaGridDatabase);
         else if(obj->getObjectTypeNumber() == WallItemTypeNumber)
            mLuaGame->addWallItem(obj, mLuaGridDatabase);
         else
            obj->addToGame(mLuaGame, mLuaGridDatabase);
      }
   }

   return 0;
}


/**
 * @luafunc LuaGameInfo LuaScriptRunner::getGameInfo()
 *
 * @brief Returns the LuaGameInfo object.
 *
 * @descr LuaGameInfo can be used to grab information about the currently running
 * game, including the GameType. This only works in-game, not with editor
 * plugins.
 *
 * @return The LuaGameInfo object.
 */
S32 LuaScriptRunner::lua_getGameInfo(lua_State *L)
{
   TNLAssert(mLuaGame != NULL, "Game must not be NULL!");
   TNLAssert(dynamic_cast<ServerGame*>(mLuaGame), "Not ServerGame??");

   if(!mLuaGame->isServer())
   {
      logprintf(LogConsumer::LuaBotMessage, "'getGameInfo' can only be called in-game");
      returnNil(L);
   }

   return returnGameInfo(L, static_cast<ServerGame*>(mLuaGame));
}


/**
 * @luafunc num LuaScriptRunner::getPlayerCount()
 *
 * @return Current number of connected players.
 */
S32 LuaScriptRunner::lua_getPlayerCount(lua_State *L)
{
   TNLAssert(mLuaGame != NULL, "Game must not be NULL!");

   return returnInt(L, mLuaGame ? mLuaGame->getPlayerCount() : 1);
}


/**
 * @luafunc LuaScriptRunner::subscribe(Event event)
 *
 * @brief Manually subscribe to notifications when the specified \ref EventEnum
 * occurs.
 *
 * @param event The \ref EventEnum to subscribe to.
 *
 * @see The \ref EventEnum page for a list of events and their callback
 * signatures.
 */
S32 LuaScriptRunner::lua_subscribe(lua_State *L)
{
   ScriptContext context = UnknownContext;

   if(mScriptType == ScriptTypeRobot)
      context = RobotContext;
   else if (mScriptType == ScriptTypeLevelgen)
      context = LevelgenContext;

   // Subscribing is only allowed for bots and levelgens
   else
   {
      logprintf(LogConsumer::LuaBotMessage, "Calling 'subscribe()' only allowed in-game.  Not subscribing..");
      return 0;
   }

   return doSubscribe(L, context);
}


/**
 * @luafunc LuaScriptRunner::unsubscribe(Event event)
 *
 * @brief Manually unsubscribe to the specified \ref EventEnum.
 */
S32 LuaScriptRunner::lua_unsubscribe(lua_State *L)
{
   return doUnsubscribe(L);
}


};

