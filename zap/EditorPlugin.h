//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _EDITOR_PLUGIN_H_
#define _EDITOR_PLUGIN_H_

#include "luaLevelGenerator.h"      // Parent class
#include "UIMenuItems.h"

#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

namespace Zap
{
class EditorPlugin : public LuaScriptRunner
{
   typedef LuaScriptRunner Parent;

private:
   static bool getMenuItemVectorFromTable(lua_State *L, S32 index, const char *methodName, Vector<boost::shared_ptr<MenuItem> > &menuItems);

   GridDatabase *mGridDatabase;
   Game *mGame;
   F32 mGridSize;
   string mDescription;
   string mRequestedBinding;

protected:
   void killScript();

public:
   // Constructors
   EditorPlugin();      // Dummy 0-args constructor, here to make boost happy!
   EditorPlugin(const string &scriptName, const Vector<string> &scriptArgs, F32 gridSize, 
                  GridDatabase *gridDatabase, Game *game);

   virtual ~EditorPlugin();  // Destructor

   bool prepareEnvironment();
   string getScriptName();
   string getDescription();
   string getRequestedBinding();

   const char *getErrorMessagePrefix();
     
   bool runGetArgsMenu(string &menuTitle, Vector<boost::shared_ptr<MenuItem> > &menuItems);    // Get menu def from the plugin

   // Lua methods
   S32 lua_getGridSize(lua_State *L);
   S32 lua_getSelectedObjects(lua_State *L);        // Return all selected objects in the editor
   S32 lua_getAllObjects(lua_State *L);             // Return all objects in the editor
   S32 lua_showMessage(lua_State *L);



   //// Lua interface
   LUAW_DECLARE_CLASS(EditorPlugin);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


};

#endif
