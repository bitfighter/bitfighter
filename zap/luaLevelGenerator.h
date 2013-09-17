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
   F32 mGridSize;
   GridDatabase *mGridDatabase;

protected:
   void killScript();

public:
   LuaLevelGenerator();                // Default constructor

   // Standard constructor
   LuaLevelGenerator(const string &scriptName, const Vector<string> &scriptArgs, F32 gridsize, 
                     GridDatabase *gridDatabase, Game *game);   
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

