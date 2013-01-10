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
#include "ServerGame.h"
#include "LuaWrapper.h"
#include "barrier.h"             // For PolyWall def

namespace Zap
{

// Default constructor
LuaLevelGenerator::LuaLevelGenerator() { TNLAssert(false, "Don't use this constructor!"); }

// Standard constructor
LuaLevelGenerator::LuaLevelGenerator(const string &scriptName, const Vector<string> &scriptArgs, F32 gridSize, 
                                     GridDatabase *gridDatabase, Game *game)
{
   TNLAssert(fileExists(scriptName), "Files should be checked before we get here -- something has gone wrong!");

   mScriptName = scriptName;
   mScriptArgs = scriptArgs;

   mGridDatabase = gridDatabase;
   mGame = game;

   mGridSize = gridSize;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
LuaLevelGenerator::~LuaLevelGenerator()
{
   logprintf(LogConsumer::LogLuaObjectLifecycle, "deleted LuaLevelGenerator (%p)\n", this);
   LUAW_DESTRUCTOR_CLEANUP;
}


const char *LuaLevelGenerator::getErrorMessagePrefix() { return "***LEVELGEN ERROR***"; }


// Fire up a Lua interprer, load the script, and execute the chunk to get it in memory, then run its main() function
// Return false if there was an error, true if not
bool LuaLevelGenerator::runScript()
{
   return prepareEnvironment() && loadScript() && runMain(); 
}


string LuaLevelGenerator::getScriptName()
{
   return mScriptName;
}


// Used in addItem() below...
static const char *argv[LevelLoader::MAX_LEVEL_LINE_ARGS];


// Advance timers by deltaT
void LuaLevelGenerator::tickTimer(U32 deltaT)
{
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");

   bool ok = retrieveFunction("_tickTimer");       // Push timer function onto stack            -- function 
   TNLAssert(ok, "_tickTimer function not found -- is lua_helper_functions corrupt?");

   if(!ok)
   {      
      logError("Your scripting environment appears corrupted.  Consider reinstalling Bitfighter.");
      logError("Function _tickTimer() could not be found!  Terminating script.");

      TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");

      return;
   }

   luaW_push<LuaLevelGenerator>(L, this);
   lua_pushnumber(L, deltaT);                   // Pass the time elapsed since we were last here   -- function, object, time
   S32 error = lua_pcall(L, 2, 0, 0);           // Pass two objects, expect none in return         -- <<empty stack>>

   if(error != 0)
   {
      logError("Levelgen error running _tickTimer(): %s.", lua_tostring(L, -1));
      lua_pop(L, 1);    // Remove error message from stack

      //deleteObject();   // Add bot to delete list, where it will be deleted in the proper manner
   }

   TNLAssert(lua_gettop(L) <= 0 || LuaObject::dumpStack(L), "Stack not cleared!");
}


// TODO: Provide mechanism to modify basic level parameters like game length and teams.


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


// TODO: get rid of this function!!!!
// Pop a vec object off stack, check its type, and return it
static Point getCheckedVec(lua_State *L, S32 index, const char *methodName)
{
   if(!lua_isvec(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected vector arg at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   const F32 *vec = lua_tovec(L, index);
   return Point(vec[0], vec[1]);
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


// Deprecated
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
      LuaObject::clearStack(L);
   }

   mGame->parseLevelLine(line.c_str(), mGridDatabase, "Levelgen script: " + mScriptName);

   return 0;
}


// Simply grabs parameters from the Lua stack, and passes them off to processLevelLoadLine().  Unfortunately,
// this involves packing everything into an array of char strings, which is frightfully prone to programmer
// error and buffer overflows and such...
S32 LuaLevelGenerator::addItem(lua_State *L)
{
   static const char *methodName = "LevelGenerator:addItem()";

   // First check to see if item is a BfObject
   BfObject *obj = luaW_check<BfObject>(L, 1);

   if(obj)
   {
      // Silently ignore illegal items when being run from the editor.  For the moment, if mGame is not a server, then
      // we are running from the editor.  This could conceivably change, but for the moment it seems to hold true.
      if(mGame->isServer() || obj->canAddToEditor())
      {
         // Some objects require special handling
         if(obj->getObjectTypeNumber() == PolyWallTypeNumber)
            mGame->addPolyWall(static_cast<PolyWall *>(obj), mGridDatabase);
         else if(obj->getObjectTypeNumber() == WallItemTypeNumber)
            mGame->addWallItem(static_cast<WallItem *>(obj), mGridDatabase);
         else
         {
            obj->addToGame(mGame, mGridDatabase);

            mAddedObjects.push_back(obj);
         }
      }

      return 0;
   }

   // Otherwise, try older method of converting stack items to params to be passed to processLevelLoadLine

   S32 argc = min((S32)lua_gettop(L), (S32)LevelLoader::MAX_LEVEL_LINE_ARGS);     // Never more that MaxArgc args, please!

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
void LuaLevelGenerator::processLevelLoadLine(S32 argc, S32 id, const char **argv, GridDatabase *database, const string &levelFileName)
{
   mGame->processLevelLoadLine(argc, id, argv, database, levelFileName);
}


/**
 * @luafunc    Levelgen::addLevelLine(levelLine)
 * @brief      Adds an object to the editor by passing a line from a level file.
 * @deprecated This method is deprecated and will be removed in the future.  As an alternative, construct a BfObject directly and 
 *             add it to the game using the addItem() method.
 * @param      levelLine - string containing the line of levelcode.
*/
S32 LuaLevelGenerator::addLevelLine(lua_State *L)
{
   static const char *methodName = "LevelGenerator:addLevelLine()";

   checkArgCount(L, 1, methodName);
   const char *line = getCheckedString(L, 1, methodName);

   mGame->parseLevelLine(line, mGridDatabase, "Levelgen script: " + mScriptName);

   return 0;
}


// Set the duration of this level
S32 LuaLevelGenerator::setGameTime(lua_State *L)
{
   static const char *methodName = "Levelgen:setGameTime()";

   checkArgCount(L, 1, methodName);
   F32 time = getCheckedFloat(L, 1, methodName);

   mGame->setGameTime(time);

   return 0;
}


S32 LuaLevelGenerator::pointCanSeePoint(lua_State *L)
{
   static const char *methodName = "Levelgen:pointCanSeePoint()";

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
   static const char *methodName = "Levelgen:logprint()";
   checkArgCount(L, 1, methodName);

   logprintf(LogConsumer::LuaLevelGenerator, "Levelgen: %s", getCheckedString(L, 1, methodName));
   return 0;
}


/**
 * @luafunc    Levelgen::findObjectById(id)
 * @brief      Returns an object with the given id, or nil if none exists.
 * @descr      Finds an object with the specified user-assigned id.  If there are multiple objects with the same id (shouldn't happen, 
 *             but could, especially if the passed id is 0), this method will return the first object it finds with the given id.  
 *             Currently, all objects that have not been explicitly assigned an id have an id of 0.
 *
 * Note that ids can be assigned in the editor using the ! or # keys.
 *
 * @param      id - int id to search for.
 * @return     \e BfObject - Found object, or nil if no objects with the specified id could be found.
*/
S32 LuaLevelGenerator::findObjectById(lua_State *L)
{
   return LuaScriptRunner::findObjectById(L, mGame->getGameObjDatabase()->findObjects_fast());
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
   if(!LuaScriptRunner::prepareEnvironment())
      return false;

   if(!loadAndRunGlobalFunction(L, LUA_HELPER_FUNCTIONS_KEY, LevelgenContext) || !loadAndRunGlobalFunction(L, LEVELGEN_HELPER_FUNCTIONS_KEY, LevelgenContext))
      return false;

   setSelf(L, this, "levelgen");

   return true;
}


// Send message to all players
S32 LuaLevelGenerator::globalMsg(lua_State *L)
{
   static const char *methodName = "Levelgen:globalMsg()";
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
}


//// Lua methods
const char *LuaLevelGenerator::luaClassName = "LuaLevelGenerator";

const luaL_reg LuaLevelGenerator::luaMethods[] =
{
   { "addWall",          luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::addWall>          },
   { "addItem",          luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::addItem>          },
   { "addLevelLine",     luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::addLevelLine>     },

   { "findObjectById",   luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::findObjectById>   },
                                                                                                
   { "getGridSize",      luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::getGridSize>      },
   { "getPlayerCount",   luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::getPlayerCount>   },
   { "setGameTime",      luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::setGameTime>      },
   { "pointCanSeePoint", luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::pointCanSeePoint> },

   { "globalMsg",        luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::globalMsg>        },

   { "subscribe",        luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::subscribe>        },
   { "unsubscribe",      luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::unsubscribe>      },

   { NULL, NULL }   
};


S32 LuaLevelGenerator::subscribe(lua_State *L)   { return doSubscribe(L, LevelgenContext);   }
S32 LuaLevelGenerator::unsubscribe(lua_State *L) { return doUnsubscribe(L); }


const LuaFunctionProfile LuaLevelGenerator::functionArgs[] = { NULL, {{{ }}, 0 } };

REGISTER_LUA_CLASS(LuaLevelGenerator);

};

