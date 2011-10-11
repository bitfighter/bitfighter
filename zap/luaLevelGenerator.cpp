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
#include "config.h"     // For definition of FolderManager struct
#include "game.h"
#include "stringUtils.h"
#include "luaUtil.h"


namespace Zap
{


// C++ Constructor
LuaLevelGenerator::LuaLevelGenerator(const string &scriptName, const string &scriptDir, const Vector<string> &scriptArgs, F32 gridSize, 
                                     GridDatabase *gridDatabase, LevelLoader *caller, OGLCONSOLE_Console console)
{
   TNLAssert(fileExists(scriptName), "Files should be checked before we get here -- something has gone wrong!");

   mScriptName = scriptName;
   mScriptArgs = scriptArgs;
   setScriptingDir(scriptDir);

   mConsole = console;
   mGridDatabase = gridDatabase;

   mGridSize = gridSize;
   mCaller = caller;
}


bool LuaLevelGenerator::runScript()
{
   return startLua(LEVELGEN) && loadScript() && runMain();
}


const char LuaLevelGenerator::className[] = "LuaLevelGenerator";      // Class name as it appears to Lua scripts

// Used in addItem() below...
static const char *argv[LevelLoader::MAX_LEVEL_LINE_ARGS];


// Lua Constructor
LuaLevelGenerator::LuaLevelGenerator(lua_State *L)
{
   TNLAssert(false, "Why use this constructor?");
   throw LuaException("LuaLevelGenerator should never be constructed by a lua script!");
}


// Destructor
LuaLevelGenerator::~LuaLevelGenerator()
{
   logprintf(LogConsumer::LogLuaObjectLifecycle, "deleted LuaLevelGenerator (%p)\n", this);
}

// TODO: Provide mechanism to modify basic level parameters like game length and teams.


extern OGLCONSOLE_Console gConsole;

void LuaLevelGenerator::logError(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   char buffer[2048];

   vsnprintf(buffer, sizeof(buffer), format, args);

   logError(buffer, mScriptName.c_str());

   va_end(args);
}


void LuaLevelGenerator::logError(const char *msg, const char *filename)
{
   logprintf(LogConsumer::LogError, "***LEVELGEN ERROR*** in %s ::: %s", filename, msg);
   OGLCONSOLE_Output(gConsole, "%s\n", msg);    // Print message to the console
}


// Define the methods we will expose to Lua... basically everything we want to use in lua code
// like levelgen:blah needs to be defined here.
Lunar<LuaLevelGenerator>::RegType LuaLevelGenerator::methods[] =
{
   method(LuaLevelGenerator, addWall),
   method(LuaLevelGenerator, addItem),
   method(LuaLevelGenerator, addLevelLine),

   method(LuaLevelGenerator, getGridSize),
   method(LuaLevelGenerator, getPlayerCount),
   method(LuaLevelGenerator, setGameTime),
   method(LuaLevelGenerator, pointCanSeePoint),

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


S32 LuaLevelGenerator::addWall(lua_State *L)
{
   static const char *methodName = "LevelGeneratorEditor:addWall()";
   WallRec barrier;

   string line = "BarrierMaker";

   try
   {
      F32 width = getFloat(L, 1, methodName);      // Width is first arg
      line += " " + ftos(width, 1);
      //barrier.solid = getBool(L, 2, methodName);       // Solid param is second arg

      // Third arg is a table of coordinate values in "editor space" (i.e. these will be multiplied by gridsize before being used)
      S32 points = (S32)lua_objlen(L, 3);    // Get the number of points in our table of points

      for(S32 i = 1; i <= points; i++)       // Remember, Lua tables start with index 1
      {
         Point p = getPointFromTable(L, 3, i);
         line = line + " " + ftos(p.x, 2) + " " + ftos(p.y, 2);
      }
   }

   catch(LuaException &e)
   {
      logError(e.what());
   }

   mCaller->parseLevelLine(line.c_str(), mGridDatabase, false, "Levelgen script: " + mScriptName);

   return 0;
}


// Simply grabs parameters from the Lua stack, and passes them off to processLevelLoadLine().  Unfortunately,
// this involves packing everything into an array of char strings, which is frightfully prone to programmer
// error and buffer overflows and such...
S32 LuaLevelGenerator::addItem(lua_State *L)
{
   static const char *methodName = "LevelGenerator:addItem()";

   S32 argc = min(lua_gettop(L), LevelLoader::MAX_LEVEL_LINE_ARGS);     // Never more that MaxArgc args, please.

   if(argc == 0)
   {
      logError("Object had no parameters, ignoring...");
      return 0;
   }

   for(S32 i = 0; i < argc; i++)      // argc was already bounds checked above
      argv[i] = getString(L, i + 1, methodName);

   processLevelLoadLine(argc, 0, argv, mGridDatabase, false, "Levelgen script: " + mScriptName);      // For now, all ids are 0!

   //clearStack();

   return 0;
}


// Let someone else do the work!
void LuaLevelGenerator::processLevelLoadLine(int argc, U32 id, const char **argv, GridDatabase *database, bool inEditor, const string &levelFileName)
{
   mCaller->processLevelLoadLine(argc, id, argv, database, false, levelFileName);
}


// Feed a raw level line into the level processor
S32 LuaLevelGenerator::addLevelLine(lua_State *L)
{
   static const char *methodName = "LevelGenerator:addLevelLine()";

   checkArgCount(L, 1, methodName);
   const char *line = getString(L, 1, methodName);

   mCaller->parseLevelLine(line, mGridDatabase, false, "Levelgen script: " + mScriptName);

   //clearStack();

   return 0;
}


// Set the duration of this level
S32 LuaLevelGenerator::setGameTime(lua_State *L)
{
   static const char *methodName = "LevelGenerator:setGameTime()";

   checkArgCount(L, 1, methodName);
   F32 time = getFloat(L, 1, methodName);

   mCaller->setGameTime(time);

   //clearStack();

   return 0;
}


S32 LuaLevelGenerator::pointCanSeePoint(lua_State *L)
{
   static const char *methodName = "LevelGenerator:pointCanSeePoint()";

   checkArgCount(L, 2, methodName);

   Point p1 = getPoint(L, 1, methodName) *= mGridSize;
   Point p2 = getPoint(L, 2, methodName) *= mGridSize;

   return returnBool(L, mGridDatabase->pointCanSeePoint(p1, p2));

   //clearStack();
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


S32 LuaLevelGenerator::getPlayerCount(lua_State *L)
{
   return returnInt(L, gServerGame ? gServerGame->getPlayerCount() : 1 );
}


///// Initialize levelgen specific stuff
void LuaLevelGenerator::setPointerToThis()
{
   lua_pushnumber(L, mGridSize);
   lua_setglobal(L, "_GRID_SIZE");

   Lunar<LuaLevelGenerator>::push(L, this);  
   lua_setglobal(L, "levelgen");
}


// Register our connector types with Lua
void LuaLevelGenerator::registerClasses()
{
   Lunar<LuaLevelGenerator>::Register(L);
   Lunar<LuaPoint>::Register(L);
   Lunar<LuaUtil>::Register(L);

}


/*

////////////////////////////////////////
////////////////////////////////////////

// C++ Constructor
LuaEditorPlugin::LuaEditorPlugin(const string &scriptName, const string &scriptDir, const Vector<string> *scriptArgs, F32 gridSize, 
                                 GridDatabase *gridDatabase, LevelLoader *caller, OGLCONSOLE_Console console)
                      : Parent(scriptName, scriptDir, scriptArgs, gridSize, gridDatabase, caller, console)
{

}


const char LuaEditorPlugin::className[] = "LuaEditorPlugin";      // Class name as it appears to Lua scripts

//// Used in addItem() below...
//static const char *argv[LevelLoader::MAX_LEVEL_LINE_ARGS];


// Lua Constructor
LuaEditorPlugin::LuaEditorPlugin(lua_State *L)
{
   TNLAssert(false, "Why use this constructor?");
   throw LuaException("Trying to initalize LuaLevelGenerator without proper arguments is not allowed");
}


// Destructor
LuaEditorPlugin::~LuaEditorPlugin()
{
   // Do nothing
}



bool LuaEditorPlugin::runMain()
{
   try
   {
      lua_getglobal(L, "_main");       // _main calls main --> see lua_helper_functions.lua
      if(lua_pcall(L, 0, 0, 0) != 0)
         throw LuaException(lua_tostring(L, -1));
   }
   catch(LuaException &e)
   {
      logError("Plugin error running main(): %s.  Shutting plugin down.", e.what());
      return false;
   }
   return true;
}
*/
};

