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

#include "luaObject.h"
#include "luaUtil.h"
#include "luaGameInfo.h"
#include "tnlLog.h"           // For logprintf

#include "item.h"             // For getItem()
#include "flagItem.h"         // For getItem()
#include "robot.h"            // For getItem()
#include "NexusGame.h"      // For getItem()
#include "soccerGame.h"       // For getItem()
#include "projectile.h"       // For getItem()
#include "teleporter.h"
#include "speedZone.h"
#include "EngineeredItem.h"   // For getItem()
#include "PickupItem.h"       // For getItem()
#include "playerInfo.h"       // For playerInfo def
#include "UIMenuItems.h"      // For MenuItem def
#include "config.h"

#include "stringUtils.h"      // For joindir  
#include "lua.h"


namespace Zap
{

// Constructor
LuaObject::LuaObject()
{
   // Do nothing
}

// Destructor
LuaObject::~LuaObject()
{
   // Do nothing
}


void LuaObject::openLibs(lua_State *L)      // static
{
   luaL_openlibs(L);    // Load the standard libraries
   luaopen_vec(L);      // For vector math
}


bool LuaObject::shouldLuaGarbageCollectThisObject()
{
   return true;
}


// Returns a point to calling Lua function
S32 LuaObject::returnPoint(lua_State *L, const Point &point)
{
   return returnVec(L, point.x, point.y);
}


// Returns an existing LuaPoint to calling Lua function XXX not used?
template<class T>
S32 LuaObject::returnVal(lua_State *L, T value, bool letLuaDelete)
{
   Lunar<MenuItem>::push(L, value, letLuaDelete);     // true will allow Lua to delete this object when it goes out of scope
   return 1;
}


// Returns an int to a calling Lua function
S32 LuaObject::returnInt(lua_State *L, S32 num)
{
   lua_pushinteger(L, num);
   return 1;
}


// If we have a ship, return it, otherwise return nil
S32 LuaObject::returnShip(lua_State *L, Ship *ship)
{
   if(ship)
   {
      ship->luaProxy.push(L);
      return 1;
   }

   return returnNil(L);
}


S32 LuaObject::returnPlayerInfo(lua_State *L, Ship *ship)
{
   return returnPlayerInfo(L, ship->getControllingClient()->getPlayerInfo());
}


S32 LuaObject::returnPlayerInfo(lua_State *L, LuaPlayerInfo *playerInfo)
{
   playerInfo->push(L);
   return 1;
}


// Returns a vector to a calling Lua function
S32 LuaObject::returnVec(lua_State *L, F32 x, F32 y)
{
   lua_pushvec(L, x, y, 0, 0);
   return 1;
}

// Returns a float to a calling Lua function
S32 LuaObject::returnFloat(lua_State *L, F32 num)
{
   lua_pushnumber(L, num);
   return 1;
}


// Returns a boolean to a calling Lua function
S32 LuaObject::returnBool(lua_State *L, bool boolean)
{
   lua_pushboolean(L, boolean);
   return 1;
}


// Returns a string to a calling Lua function
S32 LuaObject::returnString(lua_State *L, const char *str)
{
   lua_pushstring(L, str);
   return 1;
}


// Returns nil to calling Lua function
S32 LuaObject::returnNil(lua_State *L)
{
   lua_pushnil(L);
   return 1;
}



void LuaObject::clearStack(lua_State *L)
{
   lua_pop(L, lua_gettop(L));
}


// Assume that table is at the top of the stack
void LuaObject::setfield (lua_State *L, const char *key, F32 value)
{
   lua_pushnumber(L, value);
   lua_setfield(L, -2, key);
}


// Make sure we got the number of args we wanted
void LuaObject::checkArgCount(lua_State *L, S32 argsWanted, const char *methodName)
{
   S32 args = lua_gettop(L);

   if(args != argsWanted)     // Problem!
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s called with %d args, expected %d", methodName, args, argsWanted);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }
}


// Pop integer off stack, check its type, do bounds checking, and return it
lua_Integer LuaObject::getInt(lua_State *L, S32 index, const char *methodName, S32 minVal, S32 maxVal)
{
   lua_Integer val = getInt(L, index, methodName);

   if(val < minVal || val > maxVal)
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s called with out-of-bounds arg: %d (val=%d)", methodName, index, val);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   return val;
}


// Returns defaultVal if there is an invalid or missing value on the stack
lua_Integer LuaObject::getInt(lua_State *L, S32 index, const char *methodName, S32 defaultVal)
{
   if(!lua_isnumber(L, index))
      return defaultVal;
   // else
   return lua_tointeger(L, index);
}


// Pop integer off stack, check its type, and return it (no bounds check)
lua_Integer LuaObject::getInt(lua_State *L, S32 index, const char *methodName)
{
   if(!lua_isnumber(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected numeric arg at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   return lua_tointeger(L, index);
}


// Pop a number off stack, convert to float, and return it (no bounds check)
F32 LuaObject::getFloat(lua_State *L, S32 index, const char *methodName)
{
   if(!lua_isnumber(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected numeric arg at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   return (F32) lua_tonumber(L, index);
}


// Pop a boolean off stack, and return it
bool LuaObject::getBool(lua_State *L, S32 index, const char *methodName)
{
   if(!lua_isboolean(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected boolean arg at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   return (bool) lua_toboolean(L, index);
}


// Pop a boolean off stack, and return it
bool LuaObject::getBool(lua_State *L, S32 index, const char *methodName, bool defaultVal)
{
   if(!lua_isboolean(L, index))
      return defaultVal;
   // else
   return (bool) lua_toboolean(L, index);
}


// Pop a string or string-like object off stack, check its type, and return it
const char *LuaObject::getString(lua_State *L, S32 index, const char *methodName, const char *defaultVal)
{
   if(!lua_isstring(L, index))
      return defaultVal;
   // else
   return lua_tostring(L, index);
}


// Pop a string or string-like object off stack, check its type, and return it
const char *LuaObject::getString(lua_State *L, S32 index, const char *methodName)
{
   if(!lua_isstring(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected string arg at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   return lua_tostring(L, index);
}


MenuItem *LuaObject::pushMenuItem (lua_State *L, MenuItem *menuItem)
{
  MenuItem *menuItemUserData = (MenuItem *)lua_newuserdata(L, sizeof(MenuItem));

  *menuItemUserData = *menuItem;
  luaL_getmetatable(L, "MenuItem");
  lua_setmetatable(L, -2);

  return menuItemUserData;
}


// Pulls values out of the table at specified index as strings, and puts them all into strings vector
void LuaObject::getStringVectorFromTable(lua_State *L, S32 index, const char *methodName, Vector<string> &strings)
{
   strings.clear();

   if(!lua_istable(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected table arg (which I wanted to convert to a string vector) at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   // The following block loosely based on http://www.gamedev.net/topic/392970-lua-table-iteration-in-c---basic-walkthrough/

   lua_pushvalue(L, index);	// Push our table onto the top of the stack
   lua_pushnil(L);            // lua_next (below) will start the iteration, it needs nil to be the first key it pops

   // The table was pushed onto the stack at -1 (recall that -1 is equivalent to lua_gettop)
   // The lua_pushnil then pushed the table to -2, where it is currently located
   while(lua_next(L, -2))     // -2 is our table
   {
      // Grab the value at the top of the stack
      if(!lua_isstring(L, -1))
      {
         char msg[256];
         dSprintf(msg, sizeof(msg), "%s expected a table of strings -- invalid value at stack position %d, table element %d", methodName, index, strings.size() + 1);
         logprintf(LogConsumer::LogError, msg);

         throw LuaException(msg);
      }

      strings.push_back(lua_tostring(L, -1));

      lua_pop(L, 1);    // We extracted that value, pop it off so we can push the next element
   }

   // We've got all the elements in the table, so clear it off the stack
   lua_pop(L, 1);
}


static ToggleMenuItem *getMenuItem(lua_State *L, S32 index)
{
  ToggleMenuItem *pushedMenuItem;

  luaL_checktype(L, index, LUA_TUSERDATA);      // Confirm the item at index is a full userdata
  pushedMenuItem = (ToggleMenuItem *)luaL_checkudata(L, index, "ToggleMenuItem");
  if(pushedMenuItem == NULL)
     luaL_typerror(L, index, "ToggleMenuItem");

  //MenuItem im = *pushedMenuItem;
  //if(!pushedMenuItem)
  //  luaL_error(L, "null menuItem");

  return pushedMenuItem;
}


// Pulls values out of the table at specified, verifies that they are MenuItems, and adds them to the menuItems vector
bool LuaObject::getMenuItemVectorFromTable(lua_State *L, S32 index, const char *methodName, Vector<MenuItem *> &menuItems)
{
   if(!lua_istable(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected table arg (which I wanted to convert to a menuItem vector) at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   // The following block (very) loosely based on http://www.gamedev.net/topic/392970-lua-table-iteration-in-c---basic-walkthrough/

   lua_pushvalue(L, index);	// Push our table onto the top of the stack                                               -- table table
   lua_pushnil(L);            // lua_next (below) will start the iteration, it needs nil to be the first key it pops    -- table table nil

   // The table was pushed onto the stack at -1 (recall that -1 is equivalent to lua_gettop)
   // The lua_pushnil then pushed the table to -2, where it is currently located
   while(lua_next(L, -2))     // -2 is our table
   {
      UserData *ud = static_cast<UserData *>(lua_touserdata(L, -1));

      if(!ud)                // Weeds out simple values, wrong userdata types still pass here
      {
         char msg[256];
         dSprintf(msg, sizeof(msg), "%s expected a MenuItem at position %d", methodName, menuItems.size() + 1);
         logprintf(LogConsumer::LogError, msg);

         throw LuaException(msg);
      }

      // We have a userdata
      LuaObject *obj = ud->objectPtr;                       // Extract the pointer
      MenuItem *menuItem = dynamic_cast<MenuItem *>(obj);   // Cast it to a MenuItem

      if(!menuItem)                                         // Cast failed -- not a MenuItem... we got some bad args
      {
         // TODO: This does not report a line number, for some reason...
         // Reproduce with code like this in a plugin
         //function getArgs()
         //   local items = { }  -- Create an empty table to hold our menu items
         //   
         //   -- Create the menu items we need for this script, adding them to our items table
         //   table.insert(items, ToggleMenuItem:new("Run mode:", { "One", "Two", "Mulitple" }, 1, false, "Specify run mode" ))
         //   table.insert(items, Point:new(1,2))
         //
         //   return "Menu title", items
         //end

         char msg[256];
         dSprintf(msg, sizeof(msg), "%s expected a MenuItem at position %d", methodName, menuItems.size() + 1);
         logprintf(LogConsumer::LogError, msg);

         throw LuaException(msg);
      }

      menuItems.push_back(menuItem);                        // Add the MenuItem to our list
      lua_pop(L, 1);                                        // We extracted that value, pop it off so we can push the next element
   }

   // We've got all the elements in the table, so clear it off the stack
   lua_pop(L, 1);

   return true;
}


// Pop a vec object off stack, check its type, and return it
Point LuaObject::getVec(lua_State *L, S32 index, const char *methodName)
{
   if(!lua_isvec(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected vector arg at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   const F32 *fff = lua_tovec(L, index);
   return Point(fff[0], fff[1]);
}


// Pop a point object off stack, or grab two numbers and create a point from that
Point LuaObject::getPointOrXY(lua_State *L, S32 index, const char *methodName)
{
   S32 args = lua_gettop(L);
   if(args == 1)
   {
      return getVec(L, index, methodName);
   }
   else if(args == 2)
   {
      F32 x = getFloat(L, index, methodName);
      F32 y = getFloat(L, index + 1, methodName);
      return Point(x, y);
   }

   // Uh oh...
   char msg[256];
   dSprintf(msg, sizeof(msg), "%s expected either a point or a pair of numbers at position %d", methodName, index);
   logprintf(LogConsumer::LogError, msg);

   throw LuaException(msg);
}


// Adapted from PiL book section 24.2.3
void LuaObject::dumpStack(lua_State* L)
{
    int top = lua_gettop(L);

    logprintf("\nTotal in stack: %d",top);

    for (S32 i = 1; i <= top; i++)
    {  // Repeat for each level 
        int t = lua_type(L, i);
        switch (t) {
            case LUA_TSTRING:   
                logprintf("string: '%s'", lua_tostring(L, i));
                break;
            case LUA_TBOOLEAN:  
                logprintf("boolean %s",lua_toboolean(L, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:    
                logprintf("number: %g", lua_tonumber(L, i));
                break;
            default:             
                logprintf("%s", lua_typename(L, t));
                break;
        }
    }
 }


////////////////////////////////////////
////////////////////////////////////////

// Constructor
LuaScriptRunner::LuaScriptRunner()
{
   L = NULL;
   mScriptingDirSet = false;
}


// Destructor
LuaScriptRunner::~LuaScriptRunner()
{
   if(L)
      lua_close(L);
}


void LuaScriptRunner::setScriptingDir(const string &scriptingDir)
{
   mScriptingDir = scriptingDir;
   mScriptingDirSet = true;
}


lua_State *LuaScriptRunner::getL()
{
   return L;
}


// Loads ouf script file into a Lua chunk, then runs it.  This has the effect of loading all our functions into the local environment,
// defining any globals, and executing any "loose" code not defined in a function.
bool LuaScriptRunner::loadScript()
{
   return loadScript(mScriptName) && runChunk();
}


bool LuaScriptRunner::loadScript(const string &scriptName)
{
   TNLAssert(mScriptingDirSet, "Must set scripting folder before intializing script!");

   // Load the script
   if(luaL_loadfile(L, scriptName.c_str()))
   {
      logError("Couldn't load script %s: %s -- Aborting.", scriptName.c_str(), lua_tostring(L, -1));
      return false;
   }

   return true;
}


// Runs the most recently loaded chunk
bool LuaScriptRunner::runChunk()
{
   try
   {
      // Initialize it by running all the code that's not contained in a function -- this loads all the functions into the global namespace
      if(lua_pcall(L, 0, 0, 0))     // Passing 0 params, getting 0 back
      {
         logError("Error initializing script %s: %s -- Aborting.", mScriptName.c_str(), lua_tostring(L, -1));
         return false;
      }

      return true;
   }
   catch(LuaException &e)
   {
      logError("Error running script %s: %s.  Aborting script.", mScriptName.c_str(), e.what());
      return false;
   }
}


// Don't forget to update the eventManager after running a robot's main function!
// return false if failed
bool LuaScriptRunner::runMain()
{
   return runMain(mScriptArgs);
}


bool LuaScriptRunner::runMain(const Vector<string> &args)
{
   setLuaArgs(args);

   try
   {   
      lua_getglobal(L, "_main");
   }
   catch(...)
   {
      logError("Function main() could not be found!  Aborting script.");
      return false;
   }

   try
   {
      //if(lua_isfunction(L, lua_gettop(L)))
      {
         if(lua_pcall(L, 0, 0, 0) != 0)
            throw LuaException(lua_tostring(L, -1));
      }
      //else     // Function main doesn't exist
      //{
      //   logError("Function main() could not be found!  Aborting script.");
      //   return false;
      //}
   }
   catch(LuaException &e)
   {
      logError("Error encountered while running main(): %s.  Aborting script.", e.what());
      return false;
   }

   return true;
}


// Load our standard lua helper functions  
// TODO: Read helpers into memory, store that as a static string in the bot code, and then pass that to Lua rather than rereading this
//       or better, find some way to load these into lua once, and then reuse the interpreter for every bot and levelgen and plugin
//       every time a bot or levelgen or plugin is created.  Or compile to bytecode and store that.  Or anything, really, that's more efficient.
bool LuaScriptRunner::loadHelperFunctions(const string &helperName)
{
   try
   {
      return loadScript(joindir(mScriptingDir, helperName).c_str()) && runChunk();
   }
   catch(LuaException &e)
   {
      logError("Error loading helper function %s: %s.  Aborting script.", helperName.c_str(), e.what());
      return false;
   }
}


bool LuaScriptRunner::startLua(ScriptType scriptType)
{
   TNLAssert(mScriptingDirSet, "Must set scripting folder before starting Lua interpreter!");
   TNLAssert(!L, "L should never be set here!");

   L = lua_open();     // Create a new Lua interpreter; will be shutdown in the destructor

   if(!L)
   {  
      // Failure here is likely to be something systemic, something bad.  Like smallpox.
      logError("Could not create Lua interpreter.  Aborting script.");     
      return false;
   }

   registerClasses();

   lua_atpanic(L, luaPanicked);     // Register our panic function 

#ifdef USE_PROFILER
   init_profiler(L);
#endif

   LuaUtil::openLibs(L);
   setModulePath();

   setPointerToThis();     


   // Load two sets of helper functions: the first is loaded for every script, the second is different for bots, levelgens, plugins, etc.
   // We expect to find these helper functions in mScriptingDir
   string helperFunctions;

   if(scriptType == ROBOT)
      helperFunctions = "robot_helper_functions.lua";
   else if(scriptType == LEVELGEN)
      helperFunctions = "levelgen_helper_functions.lua";
   else
      TNLAssert(false, "Helper functions not defined for scriptType!");
   
   return(loadHelperFunctions("lua_helper_functions.lua") && loadHelperFunctions(helperFunctions)); 
}


// Hand off any script arguments to Lua, by packing them in the args table, which is where Lua traditionally stores cmd line args
void LuaScriptRunner::setLuaArgs(const Vector<string> &args)
{
   // Put args into a table that the Lua script can read.  By Lua convention, we'll put the name of the robot/script into the 0th element.
   lua_createtable(L, args.size() + 1, 0);

   lua_pushstring(L, mScriptName.c_str());
   lua_rawseti(L, -2, 0);

   for(S32 i = 0; i < args.size(); i++)
   {
      lua_pushstring(L, args[i].c_str());
      lua_rawseti(L, -2, i + 1);
   }

   lua_setglobal(L, "arg");
}


 void LuaScriptRunner::setModulePath()   
 {                                                          // What's on the stack:
   lua_pushstring(L, "package");                            // "package"
   lua_gettable(L, LUA_GLOBALSINDEX);                       // value of package global -- appears to be a table
   lua_pushstring(L, "path");                               // table, "path"
   lua_pushstring(L, (mScriptingDir + "/?.lua").c_str());   // table, "path", mScriptingDir + "/?.lua"
   lua_settable(L, -3);                                     // table
   lua_pop(L, 1);                                           // stack should be empty
 }


// Overwrite Lua's panicky panic function with something that doesn't kill the whole game
// if something goes wrong!
int LuaScriptRunner::luaPanicked(lua_State *L)
{
   string msg = lua_tostring(L, 1);

   throw LuaException(msg);

   return 0;
}


////////////////////////////////////////
////////////////////////////////////////


LuaItem *LuaItem::getItem(lua_State *L, S32 index, U32 type, const char *functionName)
{
   switch(type)
   {
      case RobotShipTypeNumber:  // pass through
      case PlayerShipTypeNumber:
        return  Lunar<LuaShip>::check(L, index);

      case BulletTypeNumber:  // pass through
      case MineTypeNumber:
      case SpyBugTypeNumber:
         return Lunar<LuaProjectile>::check(L, index);

      case ResourceItemTypeNumber:
         return Lunar<ResourceItem>::check(L, index);
      case TestItemTypeNumber:
         return Lunar<TestItem>::check(L, index);
      case FlagTypeNumber:
         return Lunar<FlagItem>::check(L, index);

      case TeleportTypeNumber:
         return Lunar<Teleporter>::check(L, index);
      case AsteroidTypeNumber:
         return Lunar<Asteroid>::check(L, index);
      case RepairItemTypeNumber:
         return Lunar<RepairItem>::check(L, index);
      case EnergyItemTypeNumber:
         return Lunar<EnergyItem>::check(L, index);
      case SoccerBallItemTypeNumber:
         return Lunar<SoccerBallItem>::check(L, index);
      case TurretTypeNumber:
         return Lunar<Turret>::check(L, index);
      case ForceFieldProjectorTypeNumber:
         return Lunar<ForceFieldProjector>::check(L, index);


      default:
         char msg[256];
         dSprintf(msg, sizeof(msg), "%s expected item as arg at position %d", functionName, index);
         logprintf(LogConsumer::LogError, msg);

         throw LuaException(msg);
   }
}


void LuaItem::push(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
}


S32 LuaItem::getClassID(lua_State *L)
{
   TNLAssert(false, "Unimplemented method!");
   return -1;
}


};

