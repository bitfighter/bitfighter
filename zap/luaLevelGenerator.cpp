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
#include "gameLoader.h"
#include "game.h"
#include "ServerGame.h"
#include "barrier.h"             // For PolyWall def

#include "stringUtils.h"         // fileExists

#include "tnlLog.h"

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
   mScriptType = ScriptTypeLevelgen;

   mGridDatabase = gridDatabase;
   mLuaGridDatabase = gridDatabase;
   mGame = game;
   mLuaGame = game;  // Set our parent member, too

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


string LuaLevelGenerator::getScriptName()
{
   return mScriptName;
}


// Used in addItem() below...
static const char *argv[LevelLoader::MAX_LEVEL_LINE_ARGS];



// TODO: Provide mechanism to modify basic level parameters like game length and teams.



// Let someone else do the work!
void LuaLevelGenerator::processLevelLoadLine(S32 argc, S32 id, const char **argv, GridDatabase *database, const string &levelFileName)
{
   mGame->processLevelLoadLine(argc, id, argv, database, levelFileName);
}


///// Initialize levelgen specific stuff
bool LuaLevelGenerator::prepareEnvironment()
{
   if(!LuaScriptRunner::prepareEnvironment())
      return false;

   if(!loadAndRunGlobalFunction(L, LUA_HELPER_FUNCTIONS_KEY, LevelgenContext))
      return false;

   // Set this first so we have this object available in the helper functions in case we need overrides
   setSelf(L, this, "levelgen");

   if(!loadAndRunGlobalFunction(L, LEVELGEN_HELPER_FUNCTIONS_KEY, LevelgenContext))
      return false;

   return true;
}


// This will need to run on both client (from editor) and server (in game)
void LuaLevelGenerator::killScript()
{
   mGame->deleteLevelGen(this);
}


static Point getPointFromTable(lua_State *L, int tableIndex, int key, const char *methodName)
{
   lua_rawgeti(L, tableIndex, key);    // Push Point onto stack
   if(lua_isnil(L, -1))
   {
      lua_pop(L, 1);
      return Point(0,0);
   }

   Point point = LuaBase::getCheckedVec(L, -1, methodName);
   lua_pop(L, 1);    // Clear value from stack

   return point;
}


/////
// Lua interface
/**
  *  @luaclass LuaLevelGenerator
  *  @brief Supervisor class of a levelgen with various utilities.
  */
//               Fn name    Param profiles         Profile count
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, addWall,           ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, addLevelLine,      ARRAYDEF({{ STR, END }}), 1 )                        \
   METHOD(CLASS, findGlobalObjects, ARRAYDEF({{ TABLE, INTS, END }, { INTS, END }}), 2 ) \
   METHOD(CLASS, getGridSize,       ARRAYDEF({{ END }}), 1 )                             \
   METHOD(CLASS, getPlayerCount,    ARRAYDEF({{ END }}), 1 )                             \
   METHOD(CLASS, setGameTime,       ARRAYDEF({{ NUM, END }}), 1 )                        \
   METHOD(CLASS, pointCanSeePoint,  ARRAYDEF({{ PT, PT, END }}), 1 )                     \
   METHOD(CLASS, globalMsg,         ARRAYDEF({{ STR, END }}), 1 )                        \
   METHOD(CLASS, teamMsg,           ARRAYDEF({{ STR, TEAM_INDX, END }}), 1 )             \
   METHOD(CLASS, privateMsg,        ARRAYDEF({{ STR, STR, END }}), 1 )                   \
   METHOD(CLASS, announce,          ARRAYDEF({{ STR, END }}), 1 )                        \
   METHOD(CLASS, subscribe,         ARRAYDEF({{ EVENT, END }}), 1 )                      \
   METHOD(CLASS, unsubscribe,       ARRAYDEF({{ EVENT, END }}), 1 )                      \
   METHOD(CLASS, getMachineTime,    ARRAYDEF({{ END }}), 1 )                             \
   METHOD(CLASS, getGameInfo,       ARRAYDEF({{ END }}), 1 )                             \

GENERATE_LUA_METHODS_TABLE(LuaLevelGenerator, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(LuaLevelGenerator, LUA_METHODS);

#undef LUA_METHODS


const char *LuaLevelGenerator::luaClassName = "LuaLevelGenerator";
REGISTER_LUA_CLASS(LuaLevelGenerator);


// Deprecated 
// TODO: Needs documentation
S32 LuaLevelGenerator::lua_addWall(lua_State *L)
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
      LuaBase::clearStack(L);
   }

   mGame->parseLevelLine(line.c_str(), mGridDatabase, "Levelgen script: " + mScriptName);

   return 0;
}


/**
 * @luafunc    LuaLevelGenerator::addLevelLine(string levelLine)
 * @brief      Adds an object to the editor by passing a line from a level file.
 * @deprecated This method is deprecated and will be removed in the future.  As an alternative, construct a BfObject directly and 
 *             add it to the game using the addItem() method.
 * @param      levelLine string containing the line of levelcode.
 */
S32 LuaLevelGenerator::lua_addLevelLine(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "addLevelLine");

   mGame->parseLevelLine(getString(L, 1), mGridDatabase, "Levelgen script: " + mScriptName);

   return 0;
}


/**
 * @luafunc    LuaLevelGenerator::setGameTime(num timeInMinutes)
 * @brief      Sets the time remaining in the current game to the specified value
 * @param      timeInMinutes Time, in minutes, that the game should continue.  Can be fractional.
 */
S32 LuaLevelGenerator::lua_setGameTime(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "setGameTime");

   mGame->setGameTime(getFloat(L, 1));

   return 0;
}


/**
 * @luafunc bool LuaLevelGenerator::pointCanSeePoint(point point1, point point2)
 * @brief   Returns true if the two specified points can see one another.
 * @param   point1 First point.
 * @param   point2 Second point.
 * @return  `true` if objects have LOS from one to the other, `false` otherwise
 */
S32 LuaLevelGenerator::lua_pointCanSeePoint(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "pointCanSeePoint");

   Point p1 = getPointOrXY(L, 1);
   Point p2 = getPointOrXY(L, 2);

   return returnBool(L, mGridDatabase->pointCanSeePoint(p1, p2));
}


/**
 * @luafunc    LuaLevelGenerator::logprint(string message)
 * @brief      Writes a line of text to the console and the system log.
 * @descr      Function can be called without Levelgen: prefix;  For example:
 * @code
 *    logprint("Hello world!")
 * @endcode
 * @param      message Message to write.
 */
S32 LuaLevelGenerator::lua_logprint(lua_State *L)
{
   static const char *methodName = "LuaLevelGenerator:logprint()";
   checkArgCount(L, 1, methodName);

   logprintf(LogConsumer::LuaLevelGenerator, "Levelgen: %s", getCheckedString(L, 1, methodName));
   return 0;
}


/**
  *   @luafunc table LuaLevelGenerator::findGlobalObjects(table results, ObjType objType, ...)
  *   @brief   Finds all items of the specified type anywhere on the level.
  *   @descr   Can specify multiple types.  The \e table argument is optional, but levelgens that call this function frequently will perform
  *            better if they provide a reusable table in which found objects can be stored.  By providing a table, you will avoid
  *            incurring the overhead of construction and destruction of a new one.
  *
  *   If a table is not provided, the function will create a table and return it on the stack.
  *
  *   @param  results (Optional) Reusable table into which results can be written.
  *   @param  objType One or more ObjTypes specifying what types of objects to find.
  *   @return A reference back to the passed table, or a new table if one was not provided.
  *
  *   @code
  *   items = { }     -- Reusable container for findGlobalObjects.  Because it is defined outside
  *                   -- any functions, it will have global scope.
  *
  *   function countObjects(objType, ...)                -- Pass one or more object types
  *     table.clear(items)                               -- Remove any items in table from previous use
  *     levelgen:findGlobalObjects(items, objType, ...)  -- Put all items of specified type(s) into items table
  *     print(#items)                                    -- Print the number of items found to the console
  *   end
  *   @endcode
  */
S32 LuaLevelGenerator::lua_findGlobalObjects(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "findGlobalObjects");

   return LuaScriptRunner::findObjects(L, mGame->getGameObjDatabase(), NULL, NULL);
}


/**
 * @luafunc num LuaLevelGenerator::getGridSize()
 * @brief   Returns current gridSize setting.
 * @descr   Note that non-default gridSizes are rare in modern level design.
 * @return  Current gridSize.
 */
S32 LuaLevelGenerator::lua_getGridSize(lua_State *L)
{
   return returnFloat(L, mGridSize);
}


/**
 * @luafunc num LuaLevelGenerator::getMachineTime()
 * @brief   Returns current machine time as an integer.
 * @descr   Machine time is given in milliseconds.  This may be inaccurate because machine time is
 *          usually stored as a long instead of an integer
 * @return  Current machine time in milliseconds
 */
S32 LuaLevelGenerator::lua_getMachineTime(lua_State *L)
{
   return returnInt(L, Platform::getRealMilliseconds());
}


/**
 * @luafunc GameInfo LuaLevelGenerator::getGameInfo()
 * @brief   Returns the GameInfo object.
 * @descr   GameInfo can be used to grab information about the currently running game, including the GameType
 * @return  The GameInfo object
 */
S32 LuaLevelGenerator::lua_getGameInfo(lua_State *L)
{
   TNLAssert(dynamic_cast<ServerGame*>(mGame), "Not ServerGame??");
   return returnGameInfo(L, static_cast<ServerGame*>(mGame));
}


/**
 * @luafunc num LuaLevelGenerator::getPlayerCount()
 * @brief   Returns current number of players.
 * @return  Current number of players.
 */
S32 LuaLevelGenerator::lua_getPlayerCount(lua_State *L)
{
   return returnInt(L, mGame ? mGame->getPlayerCount() : 1 );
}


/**
 * @luafunc LuaLevelGenerator::globalMsg(string message)
 * @brief   Broadcast a message to all players.
 * @param   message Message to broadcast.
 */
S32 LuaLevelGenerator::lua_globalMsg(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "globalMsg");

   const char *message = getString(L, 1);

   mGame->sendGlobalChatFromController(message);

   // Fire our event handler
   EventManager::get()->fireEvent(this, EventManager::MsgReceivedEvent, message, NULL, true);

   return 0;
}


/**
 * @luafunc LuaLevelGenerator::teamMsg(string message, num teamIndex)
 * @brief   Broadcast a message to players of a team.
 * @param   message Message to broadcast.
 * @param   teamIndex Index of team to which to send a message.
 */
S32 LuaLevelGenerator::lua_teamMsg(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "teamMsg");

   const char *message = getString(L, 1);
   S32 teamIndex = getTeamIndex(L, 2);

   mGame->sendTeamChatFromController(message, teamIndex);

   // Fire our event handler
   EventManager::get()->fireEvent(this, EventManager::MsgReceivedEvent, message, NULL, true);

   return 0;
}


/**
 * @luafunc LuaLevelGenerator::privateMsg(string message, string playerName)
 * @brief   Broadcast a private message to a player.
 * @param   message Message to broadcast.
 * @param   playerName Name of player to which to send a message.
 */
S32 LuaLevelGenerator::lua_privateMsg(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "privateMsg");

   const char *message = getString(L, 1);
   const char *playerName = getString(L, 2);

   mGame->sendPrivateChatFromController(message, playerName);

   // No event fired for private message

   return 0;
}


/**
 * @luafunc LuaLevelGenerator::announce(string message)
 * @brief   Broadcast an announcement.
 * @param   message Message to broadcast.
 */
S32 LuaLevelGenerator::lua_announce(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "announce");

   const char *message = getString(L, 1);

   mGame->sendAnnouncementFromController(message);

   return 0;
}


/**
 * @luafunc LuaLevelGenerator::subscribe(Event event)
 * @brief Manually subscribe to the specified Event
 */
S32 LuaLevelGenerator::lua_subscribe(lua_State *L)
{
   return doSubscribe(L, LevelgenContext);
}


/**
 * @luafunc LuaLevelGenerator::unsubscribe(Event event)
 * @brief Manually unsubscribe to the specified Event
 */
S32 LuaLevelGenerator::lua_unsubscribe(lua_State *L)
{
   return doUnsubscribe(L);
}


};

