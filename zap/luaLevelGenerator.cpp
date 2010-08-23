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

#include "luaLevelGenerator.h"
#include "gameType.h"
#include "config.h"     // For definition of ConfigDirectories struct
namespace Zap
{

static F32 mGridSize;
static LevelLoader *mCaller;

#ifdef TNL_OS_LINUX
//const S32 LevelLoader::MaxArgc;
#endif

// C++ Constructor
LuaLevelGenerator::LuaLevelGenerator(string path, Vector<string> scriptArgs, F32 gridSize, LevelLoader *caller)
{
   mFilename = path + scriptArgs[0];

   lua_State *L = lua_open();    // Create a new Lua interpreter

   if(!L)
   {
      logError("Could not create Lua interpreter to run %s.  Skipping...", mFilename.c_str());
      return;
   }
   mGridSize = gridSize;
   mCaller = caller;
   runScript(L, scriptArgs, gridSize);
   cleanupAndTerminate(L);
}


const char LuaLevelGenerator::className[] = "LuaLevelGenerator";      // Class name as it appears to Lua scripts

// Used in addItem() below...
static const char *argv[LevelLoader::MaxArgc];
//static const char argv_buffer[LevelLoader::MaxArgc][LevelLoader::MaxArgLen];


// Lua Constructor
LuaLevelGenerator::LuaLevelGenerator(lua_State *L)
{
   // Initialize some primitive string handling stuff, used in addItem() below
   /*for(U32 i = 0; i < LevelLoader::MaxArgc; i++)
      argv[i] = argv_buffer[i];*/
}


// Destructor
LuaLevelGenerator::~LuaLevelGenerator()
{
   logprintf(LogConsumer::LogLuaObjectLifecycle, "deleted LuaLevelGenerator (%p)\n", this);
}


// Some rudimentary error logging.  Perhaps, someday, will become a sort of in-game error console.
// For now, though, pass all errors through here.
void LuaLevelGenerator::logError(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   char buffer[2048];

   vsnprintf(buffer, sizeof(buffer), format, args);
   logprintf(LogConsumer::LogError, "***LEVELGEN ERROR*** in %s ::: %s", mFilename.c_str(), buffer);

   va_end(args);
}


// Define the methods we will expose to Lua... basically everything we want to use in lua code
// like levelgen:blah needs to be defined here.
Lunar<LuaLevelGenerator>::RegType LuaLevelGenerator::methods[] =
{
   method(LuaLevelGenerator, addWall),
   method(LuaLevelGenerator, addItem),
   method(LuaLevelGenerator, addLevelLine),

   method(LuaLevelGenerator, getGridSize),

   {0,0}    // End method list
};


// Note that this uses rawgeti and therefore bypasses any metamethods set on the table
S32 getIntegerFromTable(lua_State *L, int tableIndex, int key)
{
   lua_rawgeti(L, tableIndex, key);    // Push value onto stack
   if(lua_isnil(L, -1))
   {
      lua_pop(L, 1);
      return 0;
   }

   S32 rtn = (S32)lua_tointeger(L, -1);
   lua_pop(L, 1);    // Clear value from stack
   return rtn;
}


Point getPointFromTable(lua_State *L, int tableIndex, int key)
{
   lua_rawgeti(L, tableIndex, key);    // Push Point onto stack
   if(lua_isnil(L, -1))
   {
      lua_pop(L, 1);
      return Point(0,0);
   }

   Point point = Lunar<LuaPoint>::check(L, -1)->getPoint();
   lua_pop(L, 1);    // Clear value from stack

   return point;
}


string ftos(F32 i) // convert float to string
{
   char outString[100];
   dSprintf(outString, sizeof(outString), "%2.2f", i);
   return outString;
}


S32 LuaLevelGenerator::addWall(lua_State *L)
{
   static const char *methodName = "LevelGeneratorEditor:addWall()";
   GameType::BarrierRec barrier;

   string line = "BarrierMaker";

   try
   {
      F32 width = getFloat(L, 1, methodName);      // Width is first arg
      line += " " + ftos(width);
      //barrier.solid = getBool(L, 2, methodName);       // Solid param is second arg

      // Third arg is a table of coordinate values in "editor space" (i.e. these will be multiplied by gridsize before being used)
      S32 points = (S32)lua_objlen(L, 3);    // Get the number of points in our table of points

      for(S32 i = 1; i <= points; i++)       // Remember, Lua tables start with index 1
      {
         Point p = getPointFromTable(L, 3, i);
         line = line + " " + ftos(p.x) + " " + ftos(p.y);
      }
   }

   catch(LuaException &e)
   {
      logError("Error adding wall in %s: %s", methodName, e.what());
   }

   mCaller->parseArgs(line.c_str());

   return 0;
}



// Simply grabs parameters from the Lua stack, and passes them off to processLevelLoadLine().  Unfortunately,
// this involves packing everything into an array of char strings, which is frightfully prone to programmer
// error and buffer overflows and such...
S32 LuaLevelGenerator::addItem(lua_State *L)
{
   static const char *methodName = "LevelGenerator:addItem()";

   S32 argc = min(lua_gettop(L), LevelLoader::MaxArgc);     // Never more that MaxArgc args, please.

   if(argc == 0)
   {
      logError("Object had no parameters, ignoring...");
      return 0;
   }

   for(S32 i = 0; i < argc; i++)      // argc was already bounds checked above
      argv[i] = getString(L, i + 1, methodName);

   processLevelLoadLine(argc, 0, argv);      // For now, all ids are 0!

   //clearStack();

   return 0;
}


// Let someone else do the work!
void LuaLevelGenerator::processLevelLoadLine(int argc, U32 id, const char **argv)
{
   mCaller->processLevelLoadLine(argc, id, argv);
}


// Feed a raw level line into the level processor
S32 LuaLevelGenerator::addLevelLine(lua_State *L)
{
   static const char *methodName = "LevelGenerator:addLevelLine()";

   checkArgCount(L, 1, methodName);
   const char *line = getString(L, 1, methodName);

   mCaller->parseArgs(line);

   //clearStack();

   return 0;
}


// Write a message to the server logfile
S32 LuaLevelGenerator::logprint(lua_State *L)
{
   static const char *methodName = "LuaLevelGenerator:logprint()";
   checkArgCount(L, 1, methodName);

   logprintf(LogConsumer::LuaLevelGenerator, "LuaLevelGenerator: %s", getString(L, 1, methodName));
   return 0;
}


S32 LuaLevelGenerator::getGridSize(lua_State *L)
{
   return returnFloat(L, mGridSize);
}


extern ConfigDirectories gConfigDirs;
extern string joindir(const string &path, const string &filename);

// TODO: This is almost identical to the same-named function in robot.cpp, but each call their own logError function.  How can we combine?
bool LuaLevelGenerator::loadLuaHelperFunctions(lua_State *L, const char *caller)
{
   // Load our standard robot library  TODO: Read the file into memory, store that as a static string in the bot code, and then pass that to Lua rather than rereading this
   // every time a bot is created.
   string fname = joindir(gConfigDirs.luaDir, "lua_helper_functions.lua");

   if(luaL_loadfile(L, fname.c_str()))
   {
      logError("Error loading lua helper functions %s: %s.  Can't run %s...", fname.c_str(), lua_tostring(L, -1), caller);
      return false;
   }

   // Now run the loaded code
   if(lua_pcall(L, 0, 0, 0))     // Passing 0 params, getting none back
   {
      logError("Error during initializing lua helper functions %s: %s.  Can't run %s...", fname.c_str(), lua_tostring(L, -1), caller);
      return false;
   }

   return true;
}


bool LuaLevelGenerator::loadLevelGenHelperFunctions(lua_State *L)
{
   string fname = joindir(gConfigDirs.luaDir, "levelgen_helper_functions.lua");

   if(luaL_loadfile(L, fname.c_str()))
   {
      logError("Error loading levelgen helper functions %s.  Skipping...", fname.c_str());
      return false;
   }

   // Now run the loaded code
   if(lua_pcall(L, 0, 0, 0))     // Passing 0 params, getting none back
   {
      logError("Error during initializing levelgen helper functions: %s.  Skipping...", lua_tostring(L, -1));
      return false;
   }

   return true;
}


void LuaLevelGenerator::runScript(lua_State *L, Vector<string> scriptArgs, F32 gridSize)
{
   // Register this class Luna
   Lunar<LuaLevelGenerator>::Register(L);
   Lunar<LuaPoint>::Register(L);
   Lunar<LuaUtil>::Register(L);

   lua_atpanic(L, luaPanicked);    // Register our panic function

   // Load some libraries
   luaopen_base(L);
   luaopen_math(L);
   luaopen_table(L);    // Needed for custom iterators and "values" function included in robot_helper_functions.lua
   luaopen_debug(L);    // Needed for "strict" implementation


   lua_pushnumber(L, gridSize);
   lua_setglobal(L, "_GRID_SIZE");

   lua_pushlightuserdata(L, (void *)this);
   lua_setglobal(L, "LevelGen");

   setLuaArgs(L, scriptArgs);    // Put our args in to the Lua table "args"
                                 // MUST BE SET BEFORE LOADING LUA HELPER FNS (WHICH F$%^S WITH GLOBALS IN LUA)

   if(!loadLuaHelperFunctions(L, "levelgen script")) return;
   if(!loadLevelGenHelperFunctions(L)) return;


   if(luaL_loadfile(L, mFilename.c_str()))
   {
      logError("Error loading levelgen script: %s.  Skipping...", lua_tostring(L, -1));
      return;
   }

   // Now run the loaded code
   if(lua_pcall(L, 0, 0, 0))     // Passing 0 params, getting none back
   {
      logError("Lua error encountered running levelgen script: %s.  Skipping...", lua_tostring(L, -1));
      return;
   }
}


};

