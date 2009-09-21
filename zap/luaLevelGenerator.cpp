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
#include "robot.h"      // For static methods, which should be moved elsewhere...
#include "gameType.h"  

namespace Zap
{

// C++ Constructor
LuaLevelGenerator::LuaLevelGenerator(string path, Vector<string> scriptArgs)    
{
   mFilename = path + scriptArgs[0];

   lua_State *L = lua_open();    // Create a new Lua interpreter

   if(!L)
   {
      logError("Could not create Lua interpreter to run %s.  Skipping...", mFilename.c_str());
      return;
   }

   runScript(L, scriptArgs);
   Robot::cleanupAndTerminate(L);
}


const char LuaLevelGenerator::className[] = "LuaLevelGenerator";      // Class name as it appears to Lua scripts


// Lua Constructor
LuaLevelGenerator::LuaLevelGenerator(lua_State *L)
{
   // Do nothing
}


// Destructor
LuaLevelGenerator::~LuaLevelGenerator()
{
   logprintf("deleted LuaLevelGenerator (%p)\n", this);     
}


// Some rudimentary error logging.  Perhaps, someday, will become a sort of in-game error console.
// For now, though, pass all errors through here.
void LuaLevelGenerator::logError(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   char buffer[2048];

   dVsprintf(buffer, sizeof(buffer), format, args);
   logprintf("***LEVELGEN ERROR*** in %s ::: %s", mFilename.c_str(), buffer);

   va_end(args);
}


// Define the methods we will expose to Lua... basically everything we want to use in lua code
// like levelgen:blah needs to be defined here.
Lunar<LuaLevelGenerator>::RegType LuaLevelGenerator::methods[] =
{
   method(LuaLevelGenerator, addWall),
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


extern void constructBarriers(Game *theGame, const Vector<F32> &barrier, F32 width, bool solid);

S32 LuaLevelGenerator::addWall(lua_State *L)
{

// Lua supplies a width, a boolean (solid), and a series of points

   static const char *methodName = "LevelGenerator:addWall()";
   GameType::BarrierRec barrier;

   try 
   {
      barrier.width = getFloat(L, 1, methodName);      // Width is first arg
      barrier.solid = getBool(L, 2, methodName);       // Solid param is second arg

      // Third arg is a table of coordinate values in "editor space" (i.e. these will be multiplied by gridsize before being used)
      S32 points = (S32)lua_objlen(L, 3);    // Get the number of points in our table of points

     for(S32 i = 1; i <= points; i++)       // Remember, Lua tables start with index 1
     {
        Point p = getPointFromTable(L, 3, i);
        barrier.verts.push_back(p.x * gServerGame->getGridSize());  
        barrier.verts.push_back(p.y * gServerGame->getGridSize());  
     }
   }
   catch(string err)
   {
      logError("Error adding wall in %s: %s", methodName, err);
   }

   gServerGame->getGameType()->mBarriers.push_back(barrier);
   constructBarriers(gServerGame, barrier.verts, barrier.width, barrier.solid);

   return 0;
}


//S32 LuaLevelGenerator::addRepairItem(lua_State *L)
//{
//
   //// Lua supplies a location and a respawn time

   //static const char *methodName = "LevelGenerator:addRepairItem()";

   //TNL::Object *theObject = TNL::Object::create("RepairItem");      // Create an object of the type specified on the line
   //GameObject *object = dynamic_cast<GameObject*>(theObject);       // Force our new object to be a GameObject


   //// From game.cpp #548
   //gServerWorldBounds = gServerGame->computeWorldObjectExtents();    // Make sure this is current if we process a robot that needs this for intro code
   //object->addToGame(this);

   //bool validArgs = object->processArguments(argc - 1, argv + 1);

   //if(!validArgs)
   //{
   //   logprintf("Object %s had invalid parameters, ignoring...", obj);
   //   object->removeFromGame();
   //   object->destroySelf();
   //}                                                      
   //              

   //return 0;
//}


// Write a message to the server logfile
S32 LuaLevelGenerator::logprint(lua_State *L)
{
   static const char *methodName = "LuaLevelGenerator:logprint()";
   checkArgCount(L, 1, methodName);

   logprintf("LuaLevelGenerator: %s", getString(L, 1, methodName));
   return 0;
}


extern ServerGame *gServerGame;
S32 LuaLevelGenerator::getGridSize(lua_State *L)    
{ 
   return returnFloat(L, gServerGame->getGridSize()); 
}


bool LuaLevelGenerator::loadLuaHelperFunctions(lua_State *L)
{
   // Load our standard robot library  TODO: Read the file into memory, store that as a static string in the bot code, and then pass that to Lua rather than rereading this
   // every time a bot is created.
   static const char *fname = "lua_helper_functions.lua";

   if(luaL_loadfile(L, fname))
   {
      logError("Error loading lua helper functions %s.  Skipping levelgen script...", fname);
      return false;
   }

   // Now run the loaded code
   if(lua_pcall(L, 0, 0, 0))     // Passing 0 params, getting none back
   {
      logError("Error during initializing lua helper functions: %s.  Skipping...", lua_tostring(L, -1));
      return false;
   }

   return true;
}


bool LuaLevelGenerator::loadLevelGenHelperFunctions(lua_State *L)
{
   static const char *fname = "levelgen_helper_functions.lua";

   if(luaL_loadfile(L, fname))
   {
      logError("Error loading levelgen helper functions %s.  Skipping...", fname);
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


void LuaLevelGenerator::runScript(lua_State *L, Vector<string> scriptArgs)
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

   lua_pushlightuserdata(L, (void *)this);
   lua_setglobal(L, "LevelGen");

   setLuaArgs(L, scriptArgs);    // Put our args in to the Lua table "args" 
                                 // MUST BE SET BEFORE LOADING LUA HELPER FNS (WHICH F$%^S WITH GLOBALS IN LUA)

   if(!loadLuaHelperFunctions(L)) return;
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
