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
#include "config.h"              // For definition of FolderManager struct
#include "game.h"
#include "stringUtils.h"
#include "luaUtil.h"
#include "ServerGame.h"

#ifndef ZAP_DEDICATED
#  include "UIMenuItems.h"      // delete
#endif

#ifdef WIN32
#  define vsnprintf vsnprintf_s    // Use secure version on windows
#endif 

namespace Zap
{

// Default constructor
LuaLevelGenerator::LuaLevelGenerator() { TNLAssert(false, "Don't use this constructor!"); }

// Standard constructor
LuaLevelGenerator::LuaLevelGenerator(const string &scriptName, const string &scriptDir, const Vector<string> &scriptArgs, F32 gridSize, 
                                     GridDatabase *gridDatabase, LevelLoader *caller, bool inEditor)
{
   TNLAssert(fileExists(scriptName), "Files should be checked before we get here -- something has gone wrong!");

   mScriptName = scriptName;
   mScriptArgs = scriptArgs;
   setScriptingDir(scriptDir);

   mGridDatabase = gridDatabase;

   mGridSize = gridSize;
   mCaller = caller;
   mInEditor = inEditor;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Fire up a Lua interprer, load the script, and execute the chunk to get it in memory
bool LuaLevelGenerator::loadScript()
{
   return startLua(LEVELGEN) && LuaScriptRunner::loadScript();    
}


bool LuaLevelGenerator::runScript()
{
   return loadScript() && runMain(); 
}


string LuaLevelGenerator::getScriptName()
{
   return mScriptName;
}


#ifndef ZAP_DEDICATED
/* This function not used anywhere?
      static ToggleMenuItem *getMenuItem(lua_State *L, S32 index)
      {
         LuaUtil::dumpStack(L);

        //luaL_checktype(L, index, LUA_TUSERDATA);      // Confirm the item at index is a full userdata
        ToggleMenuItem *pushedMenuItem = static_cast<ToggleMenuItem *>(lua_touserdata(L, index));
        if(pushedMenuItem == NULL) 
           luaL_typerror(L, index, "ToggleMenuItem");

        //MenuItem im = *pushedMenuItem;
        //if(!pushedMenuItem)
        //  luaL_error(L, "null menuItem");

        return pushedMenuItem;
      }
*/
#endif


// Run the script's getArgsMenu() function -- return false if function is not present or returns nil, true otherwise
bool LuaLevelGenerator::runGetArgsMenu(string &menuTitle, Vector<MenuItem *> &menuItems, bool &error)
{
#ifdef ZAP_DEDICATED
   return false;
#else
   error = false;
   try
   {   
      TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");

      bool ok = retrieveFunction("getArgsMenu");     // If not found, it's OK... Not all plugins will have this

      if(!ok)
      {
         LuaObject::clearStack(L);
         return false;     
      }

      if(lua_pcall(L, 0, 2, 0))     // Passing 0 params, getting 2 back
      {
         // This should only happen if the getArgs() function is missing
         logError("Error running getArgsMenu() -- %s", lua_tostring(L, -1));
         error = true;
         return true;
      }

      if(lua_isnil(L, 1))     // Function returned nil, return false
      {
         clearStack(L);        // In case there's other junk on there
         return false;
      }

      menuTitle = getCheckedString(L, 1, "getArgsMenu");
      getMenuItemVectorFromTable(L, 2, "getArgsMenu", menuItems);

      lua_pop(L, 2);

      TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");

      return true;
   }
   catch(LuaException &e)
   {
      logError("Error running %s: %s.  Aborting script.", "function getArgs()", e.what());
      error = true;
      return true;
   }

   return false;
#endif
}


const char LuaLevelGenerator::className[] = "LuaLevelGenerator";      // Class name as it appears to Lua scripts

// Used in addItem() below...
static const char *argv[LevelLoader::MAX_LEVEL_LINE_ARGS];



// Destructor
LuaLevelGenerator::~LuaLevelGenerator()
{
   logprintf(LogConsumer::LogLuaObjectLifecycle, "deleted LuaLevelGenerator (%p)\n", this);
   LUAW_DESTRUCTOR_CLEANUP;
}

// TODO: Provide mechanism to modify basic level parameters like game length and teams.


void LuaLevelGenerator::logError(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   char buffer[2048];

   vsnprintf(buffer, sizeof(buffer), format, args);

   logprintf(LogConsumer::LogError, "***LEVELGEN ERROR*** %s",buffer);

   va_end(args);

   printStackTrace(L);

   LuaObject::clearStack(L);
}


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


Point LuaLevelGenerator::getPointFromTable(lua_State *L, int tableIndex, int key, const char *methodName)
{
   lua_rawgeti(L, tableIndex, key);    // Push Point onto stack
   if(lua_isnil(L, -1))
   {
      lua_pop(L, 1);
      return Point(0,0);
   }

   Point point = getCheckedVec(L, -1, methodName);
   lua_pop(L, 1);    // Clear value from stack

   return point;
}


S32 LuaLevelGenerator::addWall(lua_State *L)
{
   static const char *methodName = "LevelGeneratorEditor:addWall()";

   string line = "BarrierMaker";

   try
   {
      F32 width = getCheckedFloat(L, 1, methodName);      // Width is first arg
      line += " " + ftos(width, 1);

      // Third arg is a table of coordinate values in "editor space" (i.e. these will be multiplied by gridsize before being used)
      S32 points = (S32)lua_objlen(L, 3);    // Get the number of points in our table of points

      for(S32 i = 1; i <= points; i++)       // Remember, Lua tables start with index 1
      {
         Point p = getPointFromTable(L, 3, i, methodName);
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
      argv[i] = getCheckedString(L, i + 1, methodName);

   processLevelLoadLine(argc, 0, argv, mGridDatabase, "Levelgen script: " + mScriptName);      // For now, all ids are 0!

   return 0;
}


// Let someone else do the work!
void LuaLevelGenerator::processLevelLoadLine(int argc, U32 id, const char **argv, GridDatabase *database, const string &levelFileName)
{
   mCaller->processLevelLoadLine(argc, id, argv, database, mInEditor, levelFileName);
}


// Feed a raw level line into the level processor
S32 LuaLevelGenerator::addLevelLine(lua_State *L)
{
   static const char *methodName = "LevelGenerator:addLevelLine()";

   checkArgCount(L, 1, methodName);
   const char *line = getCheckedString(L, 1, methodName);

   mCaller->parseLevelLine(line, mGridDatabase, mInEditor, "Levelgen script: " + mScriptName);

   return 0;
}


// Set the duration of this level
S32 LuaLevelGenerator::setGameTime(lua_State *L)
{
   static const char *methodName = "LevelGenerator:setGameTime()";

   checkArgCount(L, 1, methodName);
   F32 time = getCheckedFloat(L, 1, methodName);

   mCaller->setGameTime(time);

   return 0;
}


S32 LuaLevelGenerator::pointCanSeePoint(lua_State *L)
{
   static const char *methodName = "LevelGenerator:pointCanSeePoint()";

   checkArgCount(L, 2, methodName);

   // Still need mGridSize because we deal with the coordinates used in the level file, which have to be multiplied by
   // GridSize to get in-game coordinates
   Point p1 = getCheckedVec(L, 1, methodName) *= mGridSize;    
   Point p2 = getCheckedVec(L, 2, methodName) *= mGridSize;

   return returnBool(L, mGridDatabase->pointCanSeePoint(p1, p2));
}


// Write a message to the server logfile
S32 LuaLevelGenerator::logprint(lua_State *L)
{
   static const char *methodName = "LuaLevelGenerator:logprint()";
   checkArgCount(L, 1, methodName);

   logprintf(LogConsumer::LuaLevelGenerator, "LuaLevelGenerator: %s", getCheckedString(L, 1, methodName));
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
bool LuaLevelGenerator::prepareEnvironment()
{
   // Push a pointer to this Script to the Lua stack, then set the name of this pointer in the protected environment.  
   // This is the name that we'll use to refer to this levelgen from our Lua code.  
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");

   luaL_dostring(L, "e = table.copy(_G)");               // Copy global environment to create our bot environment
   lua_getglobal(L, "e");                                //                                        -- environment e   
   lua_setfield(L, LUA_REGISTRYINDEX, getScriptId());    // Store copied table in the registry     -- <<empty stack>> 

   if(!loadAndRunGlobalFunction(L, LUA_HELPER_FUNCTIONS_KEY) || !loadAndRunGlobalFunction(L, LEVELGEN_HELPER_FUNCTIONS_KEY))
      return false;

   lua_getfield(L, LUA_REGISTRYINDEX, getScriptId());    // Put script's env table onto the stack  -- env_table

   // TODO: Do we still really need GRID_SIZE?           
   lua_pushnumber(L, mGridSize);                         //                                        -- env_table, mGridSize
   lua_pushliteral(L, "_GRID_SIZE");                     //                                        -- env_table, mGridSize, "_GRID_SIZE"
   lua_rawset(L, -3);                                    // env_table["_GRID_SIZE"] = *this        -- env_table
                                                         
   lua_pushliteral(L, "levelgen");                       //                                        -- env_table, "levelgen"
   luaW_push(L, this);                                   //                                        -- env_table, "levelgen", *this
   lua_rawset(L, -3);                                    // env_table["levelgen"] = *this          -- env_table

   lua_pop(L, 1);                                        // Cleanup                                -- <<empty stack>>

   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");

   return true;
}


// Send message to all players
S32 LuaLevelGenerator::globalMsg(lua_State *L)
{
   static const char *methodName = "LuaLevelGenerator:globalMsg()";
   checkArgCount(L, 1, methodName);

   const char *message = getCheckedString(L, 1, methodName);

   GameType *gt = gServerGame->getGameType();
   if(gt)
   {
      gt->sendChatFromController(message);

      // Fire our event handler
      EventManager::get()->fireEvent(getScriptId(), EventManager::MsgReceivedEvent, message, NULL, true);
   }

   return 0;
}


// Register our connector types with Lua
void LuaLevelGenerator::registerClasses()
{
   // General classes
   LuaScriptRunner::registerClasses();    // LuaScriptRunner is a parent class

   // Specific classes needed for LevelGen scripts
   //Lunar<LuaLevelGenerator>::Register(L);

#ifndef ZAP_DEDICATED
   // These are for creating editor plugins
   Lunar<ToggleMenuItem>::Register(L);
   Lunar<YesNoMenuItem>::Register(L);
   Lunar<CounterMenuItem>::Register(L);
   Lunar<TextEntryMenuItem>::Register(L);
#endif
}


//// Lua methods
const char *LuaLevelGenerator::luaClassName = "LuaLevelGenerator";

const luaL_reg LuaLevelGenerator::luaMethods[] =
{
   { "addWall",          luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::addWall>          },
   { "addItem",          luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::addItem>          },
   { "addLevelLine",     luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::addLevelLine>     },
                                                                                                
   { "getGridSize",      luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::getGridSize>      },
   { "getPlayerCount",   luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::getPlayerCount>   },
   { "setGameTime",      luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::setGameTime>      },
   { "pointCanSeePoint", luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::pointCanSeePoint> },

   { "globalMsg",        luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::globalMsg>        },

   { NULL, NULL }   
};


const LuaFunctionProfile LuaLevelGenerator::functionArgs[] =
{
   { NULL, { }, 0 }
};


REGISTER_LUA_CLASS(LuaLevelGenerator);


};

