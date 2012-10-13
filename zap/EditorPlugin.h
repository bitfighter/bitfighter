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

#ifndef _EDITOR_PLUGIN_H_
#define _EDITOR_PLUGIN_H_

#include "luaLevelGenerator.h"      // Parent class


namespace Zap
{
class EditorPlugin : public LuaScriptRunner
{
   typedef LuaScriptRunner Parent;

private:
   static bool getMenuItemVectorFromTable(lua_State *L, S32 index, const char *methodName, Vector<MenuItem *> &menuItems);

   GridDatabase *mGridDatabase;
   LevelLoader *mCaller;
   F32 mGridSize;

public:
   // Constructors
   EditorPlugin();      // Dummy 0-args constructor, here to make boost happy!
   EditorPlugin(const string &scriptName, const Vector<string> &scriptArgs, F32 gridSize, 
                  GridDatabase *gridDatabase, LevelLoader *caller);

   ~EditorPlugin();     // Destructor

   bool prepareEnvironment();
   string getScriptName();
   bool loadScript();

   const char *getErrorMessagePrefix();
     
   bool runGetArgsMenu(string &menuTitle, Vector<MenuItem *> &menuItems, bool &error);    // Get menu def from the plugin

   // Lua methods
   S32 getGridSize(lua_State *L);
   S32 addLevelLine(lua_State *L);
   S32 getSelectedObjects(lua_State *L);        // Return all selected objects in the editor
   S32 getAllObjects(lua_State *L);             // Return all objects in the editor


   //// Lua interface
   LUAW_DECLARE_CLASS(EditorPlugin);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

};


};

#endif
