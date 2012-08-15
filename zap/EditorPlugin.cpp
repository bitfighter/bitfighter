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


}