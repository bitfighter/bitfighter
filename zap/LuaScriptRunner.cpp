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
         EventManager::get()->unsubscribeImmediate(this, (EventManager::EventType)i);

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


// Load the script, execute the chunk to get it in memory, then run its main() function
// Return false if there was an error, true if not
bool LuaScriptRunner::runScript()
{
   return prepareEnvironment() && loadScript() && runMain(); 
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
bool LuaScriptRunner::loadScript()
{
   static const S32 MAX_CACHE_SIZE = 16;

   // On a dedicated server, we'll always cache our scripts; on a regular server, we'll cache script except when the user is testing
   // from the editor.  In that case, we'll want to see script changes take place immediately, and we're willing to pay a small
   // performance penalty on level load to get that.
#ifdef ZAP_DEDICATED
   bool cacheScripts = true;
#else
   bool cacheScripts = gServerGame && !gServerGame->isTestServer();
#endif

   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");

   try
   {
      pushStackTracer();            // -- _stackTracer

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


      // If we are here, script loaded and compiled; everything should be dandy.
      TNLAssert((lua_gettop(L) == 2 && lua_isfunction(L, 1) && lua_isfunction(L, 2)) 
                        || LuaObject::dumpStack(L), "Expected a single function on the stack!");

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
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");

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
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");

   lua_pushliteral(L, "package");                           // -- "package"
   lua_gettable(L, LUA_GLOBALSINDEX);                       // -- table (value of package global)

   lua_pushliteral(L, "path");                              // -- table, "path"
   lua_pushstring(L, (mScriptingDir + "/?.lua").c_str());   // -- table, "path", mScriptingDir + "/?.lua"
   lua_settable(L, -3);                                     // -- table
   lua_pop(L, 1);                                           // -- <<empty stack>>

   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");
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
   static const char *methodName = "Levelgen:findObjectById()";
   checkArgCount(L, 1, methodName);

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
   lua_Integer eventType = LuaObject::getInt(L, -1);

   if(mSubscriptions[eventType])
   {
      EventManager::get()->unsubscribe(this, (EventManager::EventType)eventType);
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


// General structure and perculiar error messages taken from lua math lib.  Docs for that are as follows; we should adhere to them as well:
//
// This function is an interface to the simple pseudo-random generator function rand provided by ANSI C. (No guarantees can be given for its 
// statistical properties.) When called without arguments, returns a uniform pseudo-random real number in the range [0,1). When called with 
// an integer number m, math.random returns a uniform pseudo-random integer in the range [1, m]. When called with two integer numbers m and n, 
// math.random returns a uniform pseudo-random integer in the range [m, n].
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
   #  define TYPE_NUMBER(value, shareWithLua, luaEnumName, d)   luaEnumName, shareWithLua, value,
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
   add_enum_to_lua(L, "Weapon",
   #  define WEAPON_ITEM(value, b, luaEnumName, d, e, f, g, h, i, j, k, l)  luaEnumName, true, value,
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
   #  define SCORING_EVENT_ITEM(value, luaEnumName)  luaEnumName, true, GameType::value,
         SCORING_EVENT_TABLE
   #  undef SCORING_EVENT_ITEM
      (char*)NULL);


   // Event handler events -- not sure if we need this one
   add_enum_to_lua(L, "Event",
   #  define EVENT(value, luaEnumName, c) luaEnumName, true, EventManager::value,
         EVENT_TABLE
   #  undef EVENT
      (char*)NULL);


   // Engineerable objects
   add_enum_to_lua(L, "EngineerBuildObject",
   #  define ENGR_OBJ(value, luaEnumName, c) luaEnumName, true, value,
         ENGINEER_BUILD_OBJECTS_TABLE
   #  undef ENGR_OBJ
      (char*)NULL);


   // TODO: Document this one!!

   // Create a table at level run time that has team, indexed by name?
   // Team.blue, Team.red, Team.neutral, etc.
   // A few other misc constants -- in Lua, we reference the teams as first team == 1, so neutral will be 0 and hostile -1
   add_enum_to_lua(L, "Team", "Neutral", true, (TEAM_NEUTRAL + 1),
                              "Hostile", true, (TEAM_HOSTILE + 1),
                              (char*)NULL);
}

#undef setEnumName
#undef setEnum
#undef setGTEnum
#undef setEventEnum

};


