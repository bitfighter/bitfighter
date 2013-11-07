//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LUALEVELGENERATOR_H_
#define _LUALEVELGENERATOR_H_

#include "LuaScriptRunner.h"  // Parent class


using namespace std;

namespace Zap
{

class GridDatabase;
class Game;

class LuaLevelGenerator: public LuaScriptRunner
{

private:
   Game *mGame;
   GridDatabase *mGridDatabase;

protected:
   void killScript();

public:
   LuaLevelGenerator();                // Default constructor

   // Standard constructor
   LuaLevelGenerator(Game *game, const string &scriptName = "", const Vector<string> &scriptArgs = Vector<string>(), GridDatabase *gridDatabase = NULL);
   virtual ~LuaLevelGenerator();       // Destructor

   bool prepareEnvironment();
   
   void onScriptInitialized();
   string getScriptName();

   virtual const char *getErrorMessagePrefix();

   // Lua methods
   virtual S32 lua_setGameTime(lua_State *L);       // Set the time for this level

   S32 lua_globalMsg(lua_State *L);
   S32 lua_teamMsg(lua_State *L);
   S32 lua_privateMsg(lua_State *L);
   S32 lua_announce(lua_State *L);

   // Implement LevelLoader abstract method
   //void processLevelLoadLine(S32 argc, S32 id, const char **argv, GridDatabase *database, const string &levelFileName);

   //// Lua interface
   LUAW_DECLARE_CLASS(LuaLevelGenerator);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


};

#endif

