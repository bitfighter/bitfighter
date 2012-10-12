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

#include "EditorPlugin.h"     // Header
#include "gridDB.h"           // For database methods
#include "BfObject.h"         // So we can cast to BfObject


namespace Zap
{
// Default constructor
EditorPlugin::EditorPlugin() { TNLAssert(false, "Don't use this constructor!"); }

// Constructor
EditorPlugin::EditorPlugin(const string &scriptName, const Vector<string> &scriptArgs, F32 gridSize, 
                           GridDatabase *gridDatabase, LevelLoader *caller) : 
      Parent(scriptName, scriptArgs, gridSize, gridDatabase, caller)
{
   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
EditorPlugin::~EditorPlugin()
{
   logprintf(LogConsumer::LogLuaObjectLifecycle, "deleted EditorPlugin (%p)\n", this);
   LUAW_DESTRUCTOR_CLEANUP;
}


const char *EditorPlugin::getErrorMessagePrefix() { return "***PLUGIN ERROR***"; }


// Run the script's getArgsMenu() function -- return false if function is not present or returns nil, true otherwise
bool EditorPlugin::runGetArgsMenu(string &menuTitle, Vector<MenuItem *> &menuItems, bool &error)
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
      LuaObject::clearStack(L);
      error = true;
      return true;
   }

   return false;
#endif
}


bool EditorPlugin::prepareEnvironment()
{
   LuaScriptRunner::prepareEnvironment();

   //if(!loadAndRunGlobalFunction(L, LUA_HELPER_FUNCTIONS_KEY) || !loadAndRunGlobalFunction(L, LEVELGEN_HELPER_FUNCTIONS_KEY))
   //   return false;

   setSelf(L, this, "plugin");

   return true;
}


// Pulls values out of the table at specified, verifies that they are MenuItems, and adds them to the menuItems vector
bool EditorPlugin::getMenuItemVectorFromTable(lua_State *L, S32 index, const char *methodName, Vector<MenuItem *> &menuItems)
{
#ifdef ZAP_DEDICATED
   throw LuaException("Dedicated server should not use MenuItem");
#else
   if(!lua_istable(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected table arg (which I wanted to convert to a menuItem vector) at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   // The following block is (very) loosely based on http://www.gamedev.net/topic/392970-lua-table-iteration-in-c---basic-walkthrough/

   lua_pushvalue(L, index);	// Push our table onto the top of the stack                                    -- menuName table table
   lua_pushnil(L);            // lua_next (below) will start the iteration, nil must be first key it pops    -- menuName table table nil

   // The table was pushed onto the stack at -1 (recall that -1 is equivalent to lua_gettop)
   // The lua_pushnil then pushed the table to -2, where it is currently located
   // lua_next pops a key from the stack, and pushes a key-value pair from the table at the given index 
   // (the "next" pair after the given key).  When iteration is finished, lua_next returns 0.
   while(lua_next(L, -2))     // -2 is our table                                                             -- menuName table table nextIndex menuItem
   {
      MenuItem *menuItem = luaW_check<MenuItem>(L, -1);                                                   // -- menuName table table nextIndex menuItem

      if(!menuItem)        // Cast failed -- not a MenuItem... we got some bad args
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
         dSprintf(msg, sizeof(msg), "%s expected a MenuItem at table index %d", methodName, menuItems.size() + 1);
         logprintf(LogConsumer::LogError, msg);

         throw LuaException(msg);
      }

      menuItems.push_back(menuItem);   // Add the MenuItem to our list
      lua_pop(L, 1);                   // Remove extracted element from stack                                -- menuName table table nextIndex
   }                                                                                                   // OR -- menuName table 0 on last iteration

   lua_pop(L, 1);                      // Remove the "0"                                                     -- menuName table

#endif
   return true;
}


//// Lua methods
/**
 *  @luaclass EditorPlugin
 *  @brief    Main object for running methods related to editor plugins.
 *  @descr    The current editor plugin is always available in a global variable called "plugin".
 */

const char *EditorPlugin::luaClassName = "EditorPlugin";

REGISTER_LUA_CLASS(EditorPlugin);

//               Fn name    Param profiles         Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getGridSize,        ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, addLevelLine,       ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getSelectedObjects, ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getAllObjects,      ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(EditorPlugin, LUA_METHODS);

#undef LUA_METHODS


const LuaFunctionProfile EditorPlugin::functionArgs[] = { { NULL, { }, 0 } };


// These two methods are reproduced here because it is (probably) better to have the plugin object stand on its own rather than
// appear to be a subclass of levelgen.  Therefore, we need to have these methods be members of the class that we expose to Lua.

/**
 * @luafunc  num EditorPlugin::getGridSize()
 * @brief    Returns the current Grid Size setting.
 * @return   \e num - Current GridSize setting in the editor.
*/
S32 EditorPlugin::getGridSize(lua_State *L)
{
   return Parent::getGridSize(L);    
}


/**
 * @luafunc    EditorPlugin::addLevelLine(levelLine)
 * @brief      Adds an object to the editor by passing a line from a level file.
 * @deprecated This method is deprecated and will be removed in the future.  As an alternative, construct a BfObject directly and 
 *             add it to the game using %BfObject's \link BfObject::addToGame() addToGame() \endlink method.
 * @param      levelLine - string containing the line of levelcode.
 * @return     \e num - Current GridSize setting in the editor.
*/
S32 EditorPlugin::addLevelLine(lua_State *L)
{
   return Parent::addLevelLine(L);    
}


/**
 * @luafunc  table EditorPlugin::getSelectedObjects()
 * @brief    Returns a list of all selected objects in the editor.
 * @return   \e table - Lua table containing all the objects in the editor that are currently selected.
*/
S32 EditorPlugin::getSelectedObjects(lua_State *L)
{
   S32 count = mGridDatabase->getObjectCount();

   lua_createtable(L, 0, 0);    // Create a table with no slots --> we don't know how many selected items there will be!

   const Vector<DatabaseObject *> *objects = mGridDatabase->findObjects_fast();

   S32 pushed = 0;

   for(S32 i = 0; i < count; i++)
   {
      BfObject *obj = static_cast<BfObject *>(objects->get(i));
         
      if(!obj->isSelected())
         continue;

      obj->push(L);
      pushed++;         // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   return 1;
}


/**
 * @luafunc  table EditorPlugin::getAllObjects()
 * @brief    Returns a list of all objects in the editor.
 * @return   \e table - Lua table containing all the objects in the editor.
*/
S32 EditorPlugin::getAllObjects(lua_State *L)
{
   S32 count = mGridDatabase->getObjectCount();

   lua_createtable(L, count, 0);    // Create a table with enough slots for our objects

   const Vector<DatabaseObject *> *objects = mGridDatabase->findObjects_fast();

   S32 pushed = 0;

   for(S32 i = 0; i < count; i++)
   {
      BfObject *obj = static_cast<BfObject *>(objects->get(i));

      obj->push(L);
      pushed++;         // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   return 1;
}


}