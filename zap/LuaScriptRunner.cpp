//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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

#include <clipper.hpp>

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

   // Clean-up any game objects that were added in Lua with '.new()' but not added
   // with bf:addItem()

   // And delete the script's environment table from the Lua instance
   deleteScript(getScriptId());

   LUAW_DESTRUCTOR_CLEANUP;
}


const char *LuaScriptRunner::getErrorMessagePrefix() { return "SCRIPT"; }


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
   S32 stackDepth = lua_gettop(L);

   lua_getfield(L, LUA_REGISTRYINDEX, scriptId);   // Push REGISTRY[scriptId] onto the stack                -- table
   lua_getfield(L, -1, functionName);              // And get the requested function from the environment   -- table, function
   lua_remove(L, -2);                              // Remove table                                          -- function

   // Check if the top stack item is indeed a function (as we would expect)
   if(lua_isfunction(L, -1))
      return true;      // If so, return true, leaving the function on top of the stack

   // else
   lua_pop(L, 1);       // Get rid of the function
   TNLAssert(stackDepth == lua_gettop(L), "Stack not properly restored to the state it was in when we got here!");

   return false;
}


// Only used for loading helper functions
bool LuaScriptRunner::loadAndRunGlobalFunction(lua_State *L, const char *key, ScriptContext context)
{
   S32 stackDepth = lua_gettop(L);
   setScriptContext(L, context);

   lua_getfield(L, LUA_REGISTRYINDEX, key);     // Get function out of the registry      -- functionName()
   setEnvironment();                            // Set the environment for the code
   S32 err = lua_pcall(L, 0, 0, 0);             // Run it                                -- <<empty stack>>

   if(err != 0)
   {
      logError("Failed to load startup functions %s: %s", key, lua_tostring(L, -1));

      TNLAssert(stackDepth == lua_gettop(L), "Stack not properly restored to the state it was in when we got here!");
      return false;
   }

   TNLAssert(stackDepth == lua_gettop(L), "Stack not properly restored to the state it was in when we got here!");
   return true;
}


// Load our error handling function -- this will print a pretty stacktrace in the event things go wrong calling function.
// This function can safely throw errors.
void LuaScriptRunner::pushStackTracer()
{
   // _stackTracer is a function included in lua_helper_functions that manages the stack trace; it should ALWAYS be present.
   if(!loadFunction(L, getScriptId(), "_stackTracer"))
   {
      clearStack(L);
      throw LuaException("Method _stackTracer() could not be found!\n"
                         "Your scripting environment appears corrupted.  Consider reinstalling Bitfighter.");
   }
}


// Use this method to load an external script directly into the currently running script's
// environment.  This loaded script will be cleared when the parent script terminates
bool LuaScriptRunner::loadCompileRunEnvironmentScript(const string &scriptName) {
   // The timer is loaded in each script
   loadCompileScript(joindir(mScriptingDir, scriptName).c_str());
   setEnvironment();

   S32 err = lua_pcall(L, 0, 0, 0);

   if(err != 0)
   {
      logError("Failed to load script %s: %s", scriptName.c_str(), lua_tostring(L, -1));

      clearStack(L);
      return false;
   }

   return true;
}


// Loads script with name mScriptName into a Lua chunk, then runs it.  This has the effect of loading all our functions into the local
// environment, defining any globals, and executing any "loose" code not defined in a function.  If we're going to get any compile errors,
// they'll show up here.
bool LuaScriptRunner::loadScript(bool cacheScript)
{
   if(mScriptName == "")
      return true;

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
      {
          // We can't load the script as requested.  Sorry!
         string msg = "Error starting script:\n" + string(lua_tostring(L, -1));
         logError("%s", msg.c_str());         // Also calls clearStack(L)
         return false;
      }

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


// Returns true if string ran successfully
bool LuaScriptRunner::runString(const string &code)
{
   luaL_loadstring(L, code.c_str());
   setEnvironment();
   return !lua_pcall(L, 0, 0, 0);      // lua_pcall returns 0 in case of success
}


// Don't forget to update the eventManager after running a robot's main function!
// Returns false if failed
bool LuaScriptRunner::runMain()
{
   return runMain(mScriptArgs);
}


// Takes the passed args, puts them into a Lua table called arg, pushes it on the stack, and runs the "main" function.
// Returns false if there was an error
bool LuaScriptRunner::runMain(const Vector<string> &args)
{
   if(mScriptName == "")
      return true;

   TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack dirty!");

   setLuaArgs(args);

   S32 error = runCmd("main", lua_gettop(L), 0);
   return !error;
}


// Forward declare
void improveErrorMessages_global(string& msg, const string& startStr);

// Modifies the following messages:
//    attempt to call global 'setThrustToPt' (a nil value)
//    attempt to index global 'abc' (a nil value)\r\nStack Traceback\n...
//    attempt to perform arithmetic on global 'iii' (a nil value)
// and inserts a helpful debugging message
void improveErrorMessages(string &msg)
{
   improveErrorMessages_global(msg, "attempt to call global");
   improveErrorMessages_global(msg, "attempt to index global");
   improveErrorMessages_global(msg, "attempt to perform arithmetic on global");
}


// See if this looks like an undefined variable error; if so, make the message friendlier
// Does nothing if msg doesn't start with startStr
void improveErrorMessages_global(string &msg, const string &startStr)
{
   size_t start = msg.find(startStr + " '");

   if(start == string::npos)        // Not found
      return;

   start += startStr.length() + 2;  // Make start be the index of the end of startStr (+2 for the " '")

   size_t end = msg.find("' (a nil value)", start);
   if(end == string::npos)          // Not found
      return;

   string var = msg.substr(start, end - start);    // var is the problematic variable (abc in the sample above)

   size_t insertPos = msg.find("Stack Traceback", end);
   if(end == string::npos)          // Not found
      return;

   msg.insert(insertPos, ">>> This error could mean that '" + var + 
                         "' has a value of nil or that it has not been defined.\r\n");
}


// Returns true if there was an error, false if everything ran ok
bool LuaScriptRunner::runCmd(const char* function, S32 argCount, S32 returnValueCount)
{
   S32 stackDepth = lua_gettop(L);

   // argCount args are already on the stack... we'll refer to these as collectively as <<args>>
   pushStackTracer();                                       // -- <<whatever>>, <<args>>, _stackTracer

   S32 error;
   string errorMessage;

   if(loadFunction(L, getScriptId(), function))             // -- <<whatever>>, <<args>>, _stackTracer, function
   {
      // Reorder the stack a little
      if(argCount > 0)
      {
         // top should be 1 in all cases except for sendData() where we duplicate stack args, 
         // in which it should be argCount + 1
         // top = # items on stack - 2 [_stackTracer and function we want to run] - argCount + 1 [top of stack is 1 not 0]
         S32 top = lua_gettop(L) - 2 - argCount + 1;

         // This assert is  intended to check that if we're running onDataReceived, there should be argCount + 1 items on the stack, 
         // but for any other function, there should be 1.  This is because onDataReceived, unlike other functions, needs to keep some 
         // items on the stack in order to be able to replicate them for subsequent calls of the function (if, for example, we're 
         // sending data to multiple bots).  Because the arguments can be unbounded in type and number, we never copy them into
         // C++ land; we just duplicate them from the stack as needed.  For other functions, we have the arguments in C++, so we 
         // can just push them onto the stack multiple times for firing an event for multiple listeners.
         //
         // It's a bit messy because we're trying to pack those two conditions into a single Assert that will disappear in production
         // builds. Take advantage of guaranteed left-to-right evaluation order of the || to get this to work.
         TNLAssert( ((strcmp(function, "onDataReceived") == 0) && top == (argCount + 1)) || (top == 1),
            "Unexpected number of items on stack!");
         lua_insert(L, top);                                // -- <<whatever>>, function, <<args>>, _stackTracer
         lua_insert(L, top);                                // -- <<whatever>>, _stackTracer, function, <<args>>
      }

      // Note that lua_pcall will remove args and the function we called from the stack

      // lua_pcall returns 0 in case of success or one of the following error codes (defined in lua.h, vals shown in ()s below):
      //    LUA_ERRRUN: a runtime error. (2)
      //    LUA_ERRMEM : memory allocation error.For such errors, Lua does not call the error handler function. (4)
      //    LUA_ERRERR : error while running the error handler function.  (5)
      error = lua_pcall(L, argCount, returnValueCount, -2 - argCount);  // -- <<whatever>>, _stackTracer, <<return values>>
      //dumpStack(L, "after pcall");

   }
   else
      error = -1;

   if(!error)
   {
      lua_remove(L, -1 - returnValueCount);    // Remove _stackTracer           // -- <<whatever>>, <<return values>>
      // Currently, the only time <<whatever>> is anything is when we're dealing with onDataReceived, 
      // in which case it will have argCount items.

      // Inital starting depth will be same as argCount, except with onDataReceived, in which case it will
      // be double argCount (because we had to duplicate the args to keep a copy for subsequent calls).
      // Therefore, generally we'll end up with only our return vals on the stack, (stackDepth - argCount == 0)
      // but with onDataReceived, we also have a copy our args (stackDepth - argCount == argCount).
      TNLAssert(lua_gettop(L) == (stackDepth - argCount + returnValueCount), "Unexpected number of items on Lua stack!");

      // Do not clear stack -- caller probably wants <<whatever>> and <<return values>>
      return false;
   }

   // There was an error... handle it!l

   if(error == -1)         // Handler was removed after subscription
   {
      // The only way this can get triggered is if the handler function has been deleted by the time 
      // we get here (we check for its existence when a script subscribes).  In practice, this has 
      // probably never happened.  
      string text = "Cannot find Lua function " + string(function) + "()!\n";
      logprintf(LogConsumer::LogError, "%s\n%s", getErrorMessagePrefix(), text.c_str());
   }
   else                    // A "normal" error occurred (i.e. error in the code)
   {
      string msg = lua_tostring(L, -1);
      lua_pop(L, 1);       // Remove the message from the stack, so it won't appear in our stack dump

      improveErrorMessages(msg);    // Modifies msg
      string text = "In method " + string(function) + "():\n" + msg;

      logprintf(LogConsumer::LogError, "%s\n%s", getErrorMessagePrefix(), text.c_str());
      logprintf(LogConsumer::LogError, "Dump of Lua/C++ stack:");
   }

   logprintf(LogConsumer::LogError, "Terminating script");

   killScript();
   lua_settop(L, stackDepth - argCount);   // Remove <<args>> and other cruft     -- <<whatever>>

   return true;
}


// Start Lua and get everything configured
bool LuaScriptRunner::startLua(const string &scriptingDir)
{
   TNLAssert(!L, "L should not have been created yet!");

   mScriptingDir = scriptingDir;

   // Prepare the Lua global environment
   L = lua_open();               // Create a new Lua interpreter; will be shutdown in the destructor

   // Failure here is likely to be something systemic, something bad.  Like smallpox.
   if(!L)
   {
      string msg = "Could not instantiate the Lua interpreter.";

      // Lua just isn't going to work out for this session.
      logprintf(LogConsumer::LogError, "=====FATAL LUA ERROR=====\n%s\n=========================", msg.c_str());
      lua_close(L);
      L = NULL;
      return false;
   }

   if(!configureNewLuaInstance(L))
   {
      // An error message will have been printed by configureNewLuaInstance()
      lua_close(L);
      L = NULL;
      return false;
   }

   return true;
}


// Prepare a new Lua environment ("L") for use -- called from startLua(), and testing.
bool LuaScriptRunner::configureNewLuaInstance(lua_State *L)
{
   try 
   {
      lua_atpanic(L, luaPanicked);  // Register our panic function

#ifdef USE_PROFILER
      init_profiler(L);
#endif

      luaL_openlibs(L);    // Load the standard libraries

      // This allows the safe use of 'require' in our scripts
      setModulePath();

      // Register all our classes in the global namespace... they will be copied below when we copy the environment
      registerClasses();            // Perform class and global function registration once per lua_State
      registerLooseFunctions(L);    // Register some functions not associated with a particular class

      // Set scads of global vars in the Lua instance that mimic the use of the enums we use everywhere.
      // These will be copied into the script's environment when we run createEnvironment.
      setEnums(L);
      setGlobalObjectArrays(L);

      // Immediately execute the lua helper functions (these are global and need to be loaded before sandboxing)
      loadCompileRunHelper("lua_helper_functions.lua");

      // Load our vector library
      loadCompileRunHelper("luavec.lua");

      // Load our helper functions and store copies of the compiled code in the registry where we can use them for starting new scripts
      loadCompileSaveHelper("robot_helper_functions.lua",    ROBOT_HELPER_FUNCTIONS_KEY);
      loadCompileSaveHelper("levelgen_helper_functions.lua", LEVELGEN_HELPER_FUNCTIONS_KEY);
      loadCompileSaveHelper("timer.lua",                     SCRIPT_TIMER_KEY);

      // Perform sandboxing now
      // Only code executed before this point can access dangerous functions
      loadCompileRunHelper("sandbox.lua");

      return true;
   }
   catch(LuaException &e)
   {
      logprintf(LogConsumer::LogError, "Error configuring Lua interpreter: %s", e.msg.c_str());
      return false;
   }
}


void LuaScriptRunner::loadCompileSaveHelper(const string &scriptName, const char *registryKey)
{
   loadCompileSaveScript(joindir(mScriptingDir, scriptName).c_str(), registryKey);
}


// Load a script from the scripting directory by basename (e.g. "my_script.lua").
// Throws LuaException when there's an error compiling or running the script.
void LuaScriptRunner::loadCompileRunHelper(const string &scriptName)
{
   loadCompileScript(joindir(mScriptingDir, scriptName).c_str());
   if(lua_pcall(L, 0, 0, 0))
      throw LuaException("Error running " + scriptName + ": " + string(lua_tostring(L, -1)));
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

   if(filename[0] != '\0' && luaL_loadfile(L, filename) != 0)
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
      logprintf(LogConsumer::LogError, "%s %s.", getErrorMessagePrefix(), 
                "Lua interpreter doesn't exist.  Aborting environment setup");
      return false;
   }

   TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack dirty!");

   lua_pushvalue(L, LUA_GLOBALSINDEX);                      // -- globalEnv
   luaTableCopy(L);                                         // -- localEnvCopy
   TNLAssert(!lua_isnoneornil(L, -1), "Failed to copy _G");
   lua_setfield(L, LUA_REGISTRYINDEX, getScriptId());       // --

   // Make non-static Lua methods in this class available via "bf".  We have to
   // static_cast here because it is possible for two 'setSelf' methods to be
   // called from a subclass that calls its own prepareEnvironment() method and
   // subsequently this one (its parent), e.g. in the case of bots.
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


//template<typename T>
//void LuaScriptRunner::getLuaGlobalVar(const char* varName)
//{
//   S32 stackDepth = lua_gettop(L);
//
//   lua_getfield(L, LUA_REGISTRYINDEX, getScriptId());   // Push REGISTRY[scriptId] onto stack            -- registry table
//   lua_getfield(L, -1, varName);                        // Get value of variable from environment table  -- registry table, value of var 
//   int var = lua_tointeger(L, -1);
//   lua_pop(L, 1);
//   lua_pop(L, 1);
//
//   TNLAssert(stackDepth == lua_gettop(L), "Stack not properly restored to the state it was in when we got here!");
//}



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
   S32 id = S32(getInt(L, 1));

   for(S32 i = 0; i < objects->size(); i++)
   {
      BfObject *bfObject = static_cast<BfObject *>(objects->get(i));
      if(bfObject->getUserAssignedId() == id)
         return returnBfObject(L, bfObject);
   }

   logprintf(LogConsumer::LuaScriptMessage, "No object with ID %d was found", id);

   return returnNil(L);
}


S32 LuaScriptRunner::doSubscribe(lua_State *L, ScriptContext context)   
{ 
   EventManager::EventType eventType = getInt2<EventManager::EventType>(L, -1);

   if(!mSubscriptions[eventType])
   {
      EventManager::get()->subscribe(this, eventType, context);
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
   //       enum.  http://lua-users.org/wiki/ReadOnlyTables
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
   #  define GAME_TYPE_ITEM(value, b, luaEnumName, d, e, f)  luaEnumName, true, value,
          GAME_TYPE_TABLE
   #  undef GAME_TYPE_ITEM
      (char*)NULL);  


   // Scoring Events
   add_enum_to_lua(L, "ScoringEvent",
   #  define SCORING_EVENT_ITEM(value, luaEnumName, c)  luaEnumName, true, value,
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
   #  define CLIP_TYPE_ITEM(luaEnumName, value, c) #luaEnumName, true, value,
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
 * @luaclass ScriptRunner
 * 
 * @brief Main class for holding global methods accessible by all script runners
 * 
 * @descr Script runners include levelgens, robots, and editor plugins. The
 * methods here can be called from all three. However, some may be disabled for
 * a particular script runner.
 * 
 * In a levelgen script, there is an object magically available called 'levelgen' 
 * that gives you access to these methods.
 */

/**
 * These non-static methods work with in-game objects and can be dependent on script type and
 * what 'Game' it's called from (i.e. ServerGame or ClientGame)
 */
//                     Fn name                      Param profiles, Profile count
#define LUA_METHODS(CLASS, METHOD) \
      METHOD(CLASS, pointCanSeePoint,      ARRAYDEF({{ PT, PT, END }}), 1 ) \
      METHOD(CLASS, findObjectById,        ARRAYDEF({{ INT, END }}), 1 )    \
      METHOD(CLASS, findAllObjects,        ARRAYDEF({{ TABLE, INTS, END }, { TABLE, END }, { INTS, END }, { END }}), 4 ) \
      METHOD(CLASS, findAllObjectsInArea,  ARRAYDEF({{ TABLE, PT, PT, INTS, END }, { PT, PT, INTS, END }}), 2 ) \
      METHOD(CLASS, addItem,               ARRAYDEF({{ BFOBJ, END }}), 1 )  \
      METHOD(CLASS, getGameInfo,           ARRAYDEF({{ END }}), 1 )         \
      METHOD(CLASS, getPlayerCount,        ARRAYDEF({{ END }}), 1 )         \
      METHOD(CLASS, subscribe,             ARRAYDEF({{ EVENT, END }}), 1 )  \
      METHOD(CLASS, unsubscribe,           ARRAYDEF({{ EVENT, END }}), 1 )  \
      METHOD(CLASS, sendData,              ARRAYDEF({{ ANY, END }}), 1 )    \


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
   luaL_dostring(L, "math.tau = math.pi * 2");
}

#undef LUA_NON_STATIC_METHODS
#undef LUA_METHODS


const char *LuaScriptRunner::luaClassName = "LuaScriptRunner";
REGISTER_LUA_CLASS(LuaScriptRunner);


/**
 * @luafunc bool ScriptRunner::pointCanSeePoint(point point1, point point2)
 * 
 * @brief Returns `true` if the two specified points can see one another.
 * 
 * @param point1 First point.
 * @param point2 Second point.
 * 
 * @return `true` if objects have a line of sight from one to the other,
 * `false` otherwise.
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
 * @luafunc BfObject ScriptRunner::findObjectById(num id)
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

   TNLAssert(mLuaGridDatabase != NULL, "Grid Database must not be NULL!");

   return findObjectById(L, mLuaGridDatabase->findObjects_fast());
}


/**
 * @luafunc table ScriptRunner::findAllObjects(ObjType objType, ...)
 *
 * @brief Returns a table containing a list of objects of the specified type 
 * anywhere on the level.
 *
 * @descr Can specify multiple object types.
 *
 * If no object types are provided, this function will return every object on
 * the level (warning, may be slow).
 *
 * @param objType \link ObjType ObjTypes\endlink specifying what types of objects to find.
 *
 * @return A table with any found objects.
 *
 * @code
 * function countObjects(objType, ...)         -- Pass one or more object types
 *   objects = bf:findAllObjects(objType, ...) -- Find all objects of specified type(s)
 *   print(#objects)                           -- Print the number of items found to the console
 * end
 * 
 * function listZoneIds()
 *    zones = levelgen:findAllObjects(ObjType.GoalZone)
 *    for i = 1, #zones do
 *       id = zones[i]:getId()
 *       print(id)
 *    end
 * end
 * 
 * 
 * @endcode
 */
S32 LuaScriptRunner::lua_findAllObjects(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "findAllObjects");

   TNLAssert(mLuaGridDatabase != NULL, "Grid Database must not be NULL!");

   fillVector.clear();
   static Vector<U8> types;

   types.clear();    // Needed because types is a reusable static vector, and we need to clear out any residuals

   // We expect the stack to look like this: -- objType1, objType2, ...
   // or this, if using the deprecated fill table option -- [fillTable], objType1, objType2, ...
   // We'll work our way down from the top of the stack (element -1) until we find something that is not a number.
   // We expect that when we find something that is not a number, the stack will only contain a fillTable.  If the stack
   // is empty at that point, we'll add a table later.
   // Note that even if stack is empty, lua_isnumber will return a value... which makes no sense!
   while(lua_gettop(L) > 0 && lua_isnumber(L, -1))
   {
      U8 typenum = (U8)lua_tointeger(L, -1);

      // Requests for botzones have to be handled separately; not a problem, we'll just do the search here, and add them to
      // fillVector, where they'll be merged with the rest of our search results.
      if(typenum != BotNavMeshZoneTypeNumber)
         types.push_back(typenum);
      else
         mLuaGame->getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, fillVector);

      lua_pop(L, 1);
   }

   const Vector<DatabaseObject *> * results;

   if(types.size() == 0)
      results = mLuaGridDatabase->findObjects_fast();
   else
   {
      mLuaGridDatabase->findObjects(types, fillVector);
      results = &fillVector;
   }

   // This will guarantee a table at the top of the stack to return our found objects
   if(!lua_istable(L, -1))
   {
      TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack not cleared!");

      lua_createtable(L, results->size(), 0);    // Create a table, with enough slots pre-allocated for our data
   }
   else
      logprintf(LogConsumer::LuaScriptMessage, "Usage of a fill table with findAllObjects() "
            "is deprecated and will be removed in the future.  Instead, don't use one");

   TNLAssert((lua_gettop(L) == 1 && lua_istable(L, -1)) || dumpStack(L), "Should only have table!");

   S32 pushed = 0;      // Count of items we put into our table

   for(S32 i = 0; i < results->size(); i++)
   {
      static_cast<BfObject *>(results->get(i))->push(L);
      pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   TNLAssert(lua_gettop(L) == 1 || dumpStack(L), "Stack has unexpected items on it!");

   return 1;
}


/**
 * @luafunc table ScriptRunner::findAllObjectsInArea(point point1, point point2, ObjType objType, ...)
 *
 * @brief Finds all items of the specified type(s) in a given search area.
 *
 * @descr Multiple object types can be specified. A search rectangle will be
 * constructed from the two points given, with each point positioned at opposite
 * corners.
 *
 * @note See findAllObjects() for a code example
 *
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

   // We expect the stack to look like this: -- point1, point2, objType1, objType2, ...
   // or this, if using the deprecated fill table option -- [fillTable], point1, point2, objType1, objType2, ...
   // We'll work our way down from the top of the stack (element -1) until we find something that is not a number.
   while(lua_gettop(L) > 0 && lua_isnumber(L, -1))
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
      mLuaGame->getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, fillVector, searchArea);

   mLuaGridDatabase->findObjects(types, fillVector, searchArea);

   // This will guarantee a table at the top of the stack to return our found objects
   if(!lua_istable(L, -1))
   {
      TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack not cleared!");

      lua_createtable(L, fillVector.size(), 0);    // Create a table, with enough slots pre-allocated for our data
   }
   else
      logprintf(LogConsumer::LuaScriptMessage, "Usage of a fill table with findAllObjectsInArea() "
            "is deprecated and will be removed in the future.  Instead, don't use one");

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
 * @luafunc ScriptRunner::addItem(BfObject obj)
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
 * @luafunc GameInfo ScriptRunner::getGameInfo()
 *
 * @brief Returns the GameInfo object.
 *
 * @descr GameInfo can be used to grab information about the currently running
 * game, including the GameType. This only works in-game, not with editor
 * plugins.
 *
 * @return The GameInfo object.
 */
S32 LuaScriptRunner::lua_getGameInfo(lua_State *L)
{
   TNLAssert(mLuaGame != NULL, "Game must not be NULL!");
   TNLAssert(dynamic_cast<ServerGame*>(mLuaGame), "Not ServerGame??");

   if(!mLuaGame->isServer())
   {
      logprintf(LogConsumer::LuaScriptMessage, "'getGameInfo' can only be called while playing a game");
      returnNil(L);
   }

   return returnGameInfo(L, static_cast<ServerGame*>(mLuaGame));
}


/**
 * @luafunc num ScriptRunner::getPlayerCount()
 *
 * @return Current number of connected players.
 */
S32 LuaScriptRunner::lua_getPlayerCount(lua_State *L)
{
   TNLAssert(mLuaGame != NULL, "Game must not be NULL!");

   return returnInt(L, mLuaGame ? mLuaGame->getPlayerCount() : 1);
}


/**
 * @luafunc ScriptRunner::subscribe(Event event)
 *
 * @brief Manually subscribe to notifications when the specified event.
 * occurs.
 *
 * @param event The \link EventEnum Event\endlink to subscribe to.
 *
 * @see The \link EventEnum Event\endlink page for a list of events and their callback
 * signatures.
 *
 * Note that this method is equivalent to the subscribe method on \link Robot bots\endlink and \link LevelGenerator levelgens\endlink.
 */
S32 LuaScriptRunner::lua_subscribe(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "subscribe");
   ScriptContext context = UnknownContext;

   if(mScriptType == ScriptTypeRobot)
      context = RobotContext;
   else if (mScriptType == ScriptTypeLevelgen)
      context = LevelgenContext;

   // Subscribing is only allowed for bots and levelgens
   else
   {
      logprintf(LogConsumer::LuaScriptMessage, "Calling 'subscribe()' only allowed in-game.  Not subscribing.");
      return 0;
   }

   return doSubscribe(L, context);
}


/**
 * @luafunc ScriptRunner::unsubscribe(Event event)
 *
 * @brief Manually unsubscribe to the specified \link EventEnum Event\endlink.
 *
 * Note that this method is equivalent to the unsubscribe method on \link Robot bots\endlink and \link LevelGenerator levelgens\endlink.
 */
S32 LuaScriptRunner::lua_unsubscribe(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "unsubscribe");
   return doUnsubscribe(L);
}


/**
 * @luafunc void ScriptRunner::sendData(Any data)
 *
 * @brief Allows scripts to send data to one another.
 *
 * @descr This is a very powerful and flexible function.  Make sure that your event handling 
 * functions have the right parameters for the data you want to send.  If the number of items 
 * you want to send varies, a good solution is to wrap them in a table and pass that.
 * This functionality gives you plenty of rope... use it wisely.
 * 
 * It is possible to share simple values or tables between scripts.  When tables are shared, 
 * only a pointer is passed; the table itself is not copied.  This allows you to create <i>shared table</i>.
 *
 * What this means is that all scripts that receive the table via the \link DataReceived Event\endlink event
 * can modify the shared table, and every other script can see those changes instantly.  This allows levelgens and bots
 * to communicate directly without firing further events just by updating a shared table.
 * 
 * For example, let's assume we have a levelgen script and a bot:
 * 
 * @code
 * -- bot
 * function onDataReceived(p1) { tbl = p1 } end  -- tbl will be a global (the default in lua), pointing at p1
 * bf:subscribe(Event.DataReceived)              -- Listen for sendData messages
 * @endcode
 * 
 * @code
 * -- levelgen
 * tbl = {x=10, y=15}   -- Define a table with x and y attributes
 * bf:sendData(tbl)     -- Send the table to all scripts who are subscribed to DataReceived events
 * @endcode
 * 
 * ... later ...
 * 
 * @code
 * -- levelgen
 * tbl.x = 20           -- Update the x attribute of tbl
 * @endcode
 * 
 * @code
 * -- bot
 * print(tbl.x)         -- prints 20
 * tbl.x = 30           -- Updates tbl in the levelgen
 * @endcode
 * 
 * @code
 * -- levelgen
 * print(tbl.x)         -- prints 30
 * @endcode
 * 
 * This example can be scaled up to any number of bots, all of which can form a "hive mind".  Any valid manipulation of
 * the shared table, including adding functions, elements, sub-tables, etc. is allowed.
 * 
 * Here is a complete example that uses a shared table to let the levelgen set a destination in a shared table that 
 * bots will fly towards.  Note that the `sendData()` function is only called when a new bot is added.
 * 
 * First the levelgen code:
 * 
 * @code 
 * -- Levelgen to direct bots to fly towards a changing destination using shared tables
 *   
 * sharedDest = {x=0, y=0}
 * 
 * function main()
 *     bf:subscribe(Event.PlayerJoined)                -- Listen for players (or bots) joining
 *     Timer:scheduleRepeating(changeDest, 3 * 1000)   -- Run changeDest every 2 seconds
 *     dest = 0
 * end
 * 
 * -- Event handlers
 * function onPlayerJoined(playerInfo)
 *     -- Notify new bots of sharedDest; all robots will get this, but it doesn't hurt if they get it twice
 *     if playerInfo:isRobot() then
 *         bf:sendData(sharedDest)
 *         dest = dest + 2
 *         if dest > 4 then dest = dest - 4 end
 *     end
 * end
 * 
 * -- Called by timer
 * function changeDest()
 *     -- Advance destination
 *     dest = dest + 1
 *     if dest > 4 then dest = 1 end
 * 
 *     -- Update coordinates
 *     if     dest == 1 then sharedDest.x = 800; sharedDest.y = 0
 *     elseif dest == 2 then sharedDest.x = 800; sharedDest.y = 800
 *     elseif dest == 3 then sharedDest.x = 0;   sharedDest.y = 800
 *     elseif dest == 4 then sharedDest.x = 0;   sharedDest.y = 0
 *     end
 * end
 * @endcode
 * 
 * And the bot:
 * 
 * @code
 * -- Bot always moves towards a destination provided by the levelgen via shared table
 * 
 * -- Set up shared table so we have somewhere to go until we get the onDataReceived event
 * sharedDest = {x=0, y=0}
 * 
 * function main()
 *     bf:subscribe(Event.DataReceived)  -- Listen for sendData messages
 * end
 * 
 * function onDataReceived(tbl)
 *     sharedDest = tbl                  -- Hook up shared table
 * end
 * 
 * function onTick()
 *     bot:setThrustToPt(sharedDest)     -- Always move towards sharedDest
 *     bot:setAngle(sharedDest)          -- Aim in the direction of travel (looks nice)
 * end
 * @endcode
 * 
 * Even though this capability allows almost unlimited low-overhead communication, you may still prefer 
 * to use the sendData function because that will trigger an event handler on the receiver, whereas 
 * unless the script is monitoring values in a shared table, it will have no idea when something
 * changs.  The details of how you design your scripts will determine which approach, or combination 
 * of approaches, is best.
 * 
 * Note that the sendData function will send data to bots and the levelgen if they are 
 * subscribed to the \link DataReceived Event\endlink event, 
 * but it will not be sent back to the sender, even if they are subscribed.
 *
 * @param data Data to be sent (can be zero or more numeric, string, table, or other Lua values)
 */
S32 LuaScriptRunner::lua_sendData(lua_State* L)
{
   // No need to check the args because... anything goes!!

   // Fire our event handler
   EventManager::get()->fireEvent(this, EventManager::DataReceivedEvent);

   TNLAssert(lua_gettop(L) == 0, "Stack is dirty!");

   return 0;
}


};

