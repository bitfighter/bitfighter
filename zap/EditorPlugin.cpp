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
#include "ClientGame.h"
#include "UIEditor.h"
#include "UIManager.h"

#include "tnlLog.h"


namespace Zap
{
// Default constructor
EditorPlugin::EditorPlugin() { TNLAssert(false, "Don't use this constructor!"); }

// Constructor
EditorPlugin::EditorPlugin(const string &scriptName, const Vector<string> &scriptArgs, F32 gridSize, 
                           GridDatabase *gridDatabase, Game *game)
{
   mScriptName = scriptName;
   mScriptArgs = scriptArgs;
   mScriptType = ScriptTypeEditorPlugin;

   mGridDatabase = gridDatabase;
   mLuaGridDatabase = gridDatabase;

   mGridSize = gridSize;
   mGame = game;
   mLuaGame = game;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
EditorPlugin::~EditorPlugin()
{
   logprintf(LogConsumer::LogLuaObjectLifecycle, "deleted EditorPlugin (%p)\n", this);
   LUAW_DESTRUCTOR_CLEANUP;
}


const char *EditorPlugin::getErrorMessagePrefix() { return "***PLUGIN ERROR***"; }


// Run the script's getArgsMenu() function -- return true if error, false if not
bool EditorPlugin::runGetArgsMenu(string &menuTitle, Vector<boost::shared_ptr<MenuItem> > &menuItems)
{
#ifdef ZAP_DEDICATED
   return false;

#else
   // We'll load the functin first to see if it exists, then throw it away.  It will be loaded again when we attempt
   // to run it.  This is inefficient; however, it makes our architecture cleaner, and it is in a higly performance
   // insensitive area, so it will probably be ok.

   // First check if function exists... if it does not, there will be no menu items, so we can return false.  
   // This is not an error condition.
   if(!loadFunction(L, getScriptId(), "getArgsMenu"))
      return false;  

   // Function exists, and is on the stack.  Clear it away because it will be reloaded by runCmd().
   clearStack(L);

   static const S32 numResults = 4;
   bool error = runCmd("getArgsMenu", numResults);

   if(error)        
   {
      clearStack(L);
      return true;     
   }

   // We specified that we expect `numResults` items back; this means that
   // even if the function returns nothing (legit), there will be `numResults`
   // nils on the stack.  We'll check the top of the stack -- if that's nil,
   // we'll assume that the script intended to return no menu items, which is
   // totally legit.
   if(lua_isnil(L, 1))
   {
      clearStack(L);    // Get rid of the nils
      return false;     // No error
   }

   // There's something on the stack.  Look for what we expect, throw an exception if we get any guff.
   try
   {
      // We expect to find at most one table and three strings. There's no
      // need to enforce a specific order, so we'll just interpret each result
      // based on what we've found so far. This lets scripts using the new
      // system to put the returned values in a human-readable order, while
      // maintaining backwards-compatibility with old plugins.
      for(S32 i = 1; i <= numResults; i++)
      {
         if(lua_istable(L, i))
         {
            // check any supplied tables for menu items and put them into menuItems
            getMenuItemVectorFromTable(L, i, "getArgsMenu", menuItems);
         }
         else if(lua_isstring(L, i))
         {
            // Any strings found are interpretted as the menu title, plugin
            // description, and requested keybinding (in that order), ignoring
            // empty strings.
            if(menuTitle == "")
            {
               menuTitle = getString(L, i);
            }
            else if(mDescription == "")
            {
               mDescription = getString(L, i);
            }
            else if(mRequestedBinding == "")
            {
               mRequestedBinding = getString(L, i);
            }
         }
      }
   }
   catch(LuaException &e)
   {
      logError("Error running function getArgsMenu(): %s.  Aborting script.", e.what());
      clearStack(L);
      return true;      // Error!
   }

   clearStack(L);       // Probably unnecessary, but let's be thorough here
   return false;        // No error!

#endif
}


string EditorPlugin::getScriptName()
{
   return mScriptName;
}


string EditorPlugin::getDescription()
{
   return mDescription;
}


string EditorPlugin::getRequestedBinding()
{
   return mRequestedBinding;
}


bool EditorPlugin::prepareEnvironment()
{
   if(!LuaScriptRunner::prepareEnvironment())
      return false;

   if(!loadAndRunGlobalFunction(L, LUA_HELPER_FUNCTIONS_KEY, PluginContext) /*|| !loadAndRunGlobalFunction(L, LEVELGEN_HELPER_FUNCTIONS_KEY)*/)
      return false;

   setSelf(L, this, "plugin");

   return true;
}


// Pulls values out of the table at specified, verifies that they are MenuItems, and adds them to the menuItems vector
bool EditorPlugin::getMenuItemVectorFromTable(lua_State *L, S32 index, const char *methodName, Vector<boost::shared_ptr<MenuItem> > &menuItems)
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

      menuItems.push_back(boost::shared_ptr<MenuItem>(menuItem));   // Add the MenuItem to our list
      lua_pop(L, 1);                   // Remove extracted element from stack                                -- menuName table table nextIndex
   }                                                                                                   // OR -- menuName table 0 on last iteration

   lua_pop(L, 1);                      // Remove the "0"                                                     -- menuName table

#endif
   return true;
}


void EditorPlugin::killScript()
{
   // Do something?
}


//// Lua methods
/**
 * @luaclass EditorPlugin
 * 
 * @brief Main object for running methods related to editor plugins.
 * 
 * @descr The current editor plugin is always available in a global variable
 * called `plugin`.
 */
const char *EditorPlugin::luaClassName = "EditorPlugin";

REGISTER_LUA_CLASS(EditorPlugin);

//               Fn name    Param profiles         Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getGridSize,        ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getSelectedObjects, ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getAllObjects,      ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, showMessage,        ARRAYDEF({{ STR, END }, { STR, BOOL, END }}), 2 ) \

GENERATE_LUA_METHODS_TABLE(EditorPlugin, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(EditorPlugin, LUA_METHODS);

#undef LUA_METHODS


/**
 * @luafunc num EditorPlugin::getGridSize()
 * 
 * @brief Returns the current Grid Size setting.
 * 
 * @return The current GridSize setting in the editor.
 */
S32 EditorPlugin::lua_getGridSize(lua_State *L)
{
   return returnFloat(L, mGridSize);    
}


/**
 * @luafunc table EditorPlugin::getSelectedObjects()
 * 
 * @brief Returns a list of all selected objects in the editor.
 * 
 * @descr
 * The following code sample shows how to visit each object selected in
 * the editor. Here, we nudge every selected item 100 pixels to the right.
 * 
 * @code
 *   local t = plugin:getSelectedObjects()  -- Get every selected object
 *
 *   for i, v in ipairs(t) do               -- Iterate over list
 *      local g = v:getGeom()               -- Get the object's geometry
 *      g = Geom.translate(g, 100, 0)       -- Add 100 to the x-coords
 *      v:setGeom(g)                        -- Save the new geometry
 *   end
 * @endcode
 *
 * This result is sorted by the time at which the objects was selected,
 * so `t[1]` will always be the first selected object and `t[#t]` will
 * always be the last.
 * 
 * @return Table containing all the objects that are currently selected in
 * the editor, ordered by selection time.
 */
S32 EditorPlugin::lua_getSelectedObjects(lua_State *L)
{
   S32 count = mGridDatabase->getObjectCount();

   lua_createtable(L, 0, 0);    // Create a table with no slots --> we don't know how many selected items there will be!

   const Vector<DatabaseObject *> *objects = mGridDatabase->findObjects_fast();
   map<U32, BfObject*> orderedSelectedItems;

   for(S32 i = 0; i < count; i++)
   {
      BfObject *obj = static_cast<BfObject *>(objects->get(i));
         
      if(obj && obj->isSelected())
      {
         // This mask is a combination of the object's selection time (in the
         // upper 16 bits) and its serial number (in the lower 16 bits). This
         // orders objects by selection time, while also keeping objects which
         // are selected simultaneously from clobbering each other
         orderedSelectedItems.insert(pair<U32, BfObject*>((obj->getSelectedTime() << 16) | (obj->getSerialNumber() & 0xFFFF) , obj));
      }
   }

   S32 pushed = 0;

   map<U32, BfObject*>::iterator it;
   for(it = orderedSelectedItems.begin(); it != orderedSelectedItems.end(); it++)
   {
      BfObject *obj = (*it).second;
      obj->push(L);
      pushed++;         // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   return 1;
}


/**
 * @luafunc table EditorPlugin::getAllObjects()
 * 
 * @brief Returns a table of all objects in the editor.
 * 
 * @return Lua table containing all the objects in the editor.
 */
S32 EditorPlugin::lua_getAllObjects(lua_State *L)
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


/**
 * @luafunc EditorPlugin::showMessage(string msg, bool good)
 * 
 * @brief
 * Display a big message on-screen.
 * @desc
 *
 * Display a message to the user like the message displayed when saving a
 * file. Please be courteous and give the user some feedback about whether or
 * not your plugin has run successfully.
 *
 * @note
 * There is no guarantee that your message will fit onscreen.
 *
 * @param msg The text to display.
 * @param good Controls the color of the displayed text as follows:
 *  - `true`: green
 *  - `false`: red
 */
S32 EditorPlugin::lua_showMessage(lua_State *L)
{
   S32 profile = checkArgList(L, functionArgs, "EditorPlugin", "showMessage");
   const char* msg = getString(L, 1);

   bool good = true;
   if(profile >= 1)
   {
      good = lua_toboolean(L, -1);
   }

   ClientGame* cg = dynamic_cast<ClientGame*>(mGame);
   if(cg)
   {
      cg->getUIManager()->getUI<EditorUserInterface>()->setSaveMessage(msg, good);
   }

   clearStack(L);

   return 0;
}


}
