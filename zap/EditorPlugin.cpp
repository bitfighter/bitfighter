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

#include "EditorPlugin.h"         // Header

#ifndef ZAP_DEDICATED
#  include "UIMenuItems.h"      // delete
#endif


namespace Zap
{
// Default constructor
EditorPlugin::EditorPlugin() { TNLAssert(false, "Don't use this constructor!"); }

// Constructor
EditorPlugin::EditorPlugin(const string &scriptName, const Vector<string> &scriptArgs, F32 gridSize, 
                           GridDatabase *gridDatabase, LevelLoader *caller) : 
      Parent(scriptName, scriptArgs, gridSize, gridDatabase, caller)
{
   // Do nothing
}


void EditorPlugin::registerClasses()
{
   Parent::registerClasses();  

   // Specific classes needed for LevelGen scripts
   //Lunar<LuaLevelGenerator>::Register(L);

#ifndef ZAP_DEDICATED
   Lunar<ToggleMenuItem>::Register(L);
   Lunar<YesNoMenuItem>::Register(L);
   Lunar<CounterMenuItem>::Register(L);
   Lunar<TextEntryMenuItem>::Register(L);
#endif
}


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
      error = true;
      return true;
   }

   return false;
#endif
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

   // The following block (very) loosely based on http://www.gamedev.net/topic/392970-lua-table-iteration-in-c---basic-walkthrough/

   lua_pushvalue(L, index);	// Push our table onto the top of the stack                                               -- table table
   lua_pushnil(L);            // lua_next (below) will start the iteration, it needs nil to be the first key it pops    -- table table nil

   // The table was pushed onto the stack at -1 (recall that -1 is equivalent to lua_gettop)
   // The lua_pushnil then pushed the table to -2, where it is currently located
   while(lua_next(L, -2))     // -2 is our table
   {
      UserData *ud = static_cast<UserData *>(lua_touserdata(L, -1));

      if(!ud)                 // Weeds out simple values, wrong userdata types still pass here
      {
         char msg[1024];
         dSprintf(msg, sizeof(msg), "%s expected a MenuItem at position %d", methodName, menuItems.size() + 1);

         throw LuaException(msg);
      }

      // We have a userdata
      LuaObject *obj = ud->objectPtr;                       // Extract the pointer
      MenuItem *menuItem = static_cast<MenuItem *>(obj);    // Cast it to a MenuItem

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

#endif
   return true;
}



}