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

#include "luaUtil.h"
#include "GameSettings.h"
#include "Console.h"

namespace Zap
{


//const char LuaUtil::className[] = "LuaUtil";      // Class name as it appears to Lua scripts


// Constructor
//LuaUtil::LuaUtil()
//{
//   LUAW_CONSTRUCTOR_INITIALIZATIONS;
//}


// Destructor
//LuaUtil::~LuaUtil()
//{
//   LUAW_DESTRUCTOR_CLEANUP;
//}

//// Lua methods

////                Fn name               Param profiles                  Profile count                           
//#define LUA_METHODS(CLASS, METHOD) \
//   METHOD(CLASS,  getMachineTime,           ARRAYDEF({{ END }}), 1 )                             \
//   METHOD(CLASS,  logprint,              ARRAYDEF({{ END }}), 1 )                             \
//   METHOD(CLASS,  printToConsole,              ARRAYDEF({{ END }}), 1 )                             \
//   METHOD(CLASS,  getRandomNumber,              ARRAYDEF({{ END }}), 1 )                             \
//    METHOD(CLASS,  findFile,              ARRAYDEF({{ END }}), 1 )                             \
//                                                                                            
//GENERATE_LUA_METHODS_TABLE(LuaUtil, LUA_METHODS);
//GENERATE_LUA_FUNARGS_TABLE(LuaUtil, LUA_METHODS);
//
//#undef LUA_METHODS
//
//
//const char *LuaUtil::luaClassName = "LuaUtil";
//REGISTER_LUA_CLASS(LuaUtil);



// Define the methods we will expose to Lua... basically everything we want to use in lua code
// like LuaUtil:blah needs to be defined here.
//Lunar<LuaUtil>::RegType LuaUtil::methods[] =
//{
//   method(LuaUtil, getMachineTime),
//   method(LuaUtil, logprint),
//   method(LuaUtil, printToConsole),
//   method(LuaUtil, getRandomNumber),
//   method(LuaUtil, findFile),
//
//   {0,0}    // End method list
//};


// Write a message to the server logfile
S32 LuaUtil::logprint(lua_State *L)
{
   static const char *methodName = "LuaUtil:logprint()";
   checkArgCount(L, 2, methodName);

   logprintf(LogConsumer::LuaBotMessage, "%s: %s", getCheckedString(L, 1, methodName), getCheckedString(L, 2, methodName));

   return 0;
}


// This code based directly on Lua's print function, try to replicate functionality
S32 LuaUtil::printToConsole(lua_State *L)
{
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  string out;

  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      return luaL_error(L, LUA_QL("tostring") " must return a string to "
                           LUA_QL("print"));
    if(i > 1) 
       out += "\t";

    out += s;
    lua_pop(L, 1);  /* pop result */
  }
  gConsole.output("%s\n", out.c_str());

  return 0;
}


S32 LuaUtil::getMachineTime(lua_State *L)
{
   return returnInt(L, Platform::getRealMilliseconds());
}


// Find the specified file, in preparation for loading
S32 LuaUtil::findFile(lua_State *L)
{
   static const char *methodName = "LuaUtil:findFile()";
   checkArgCount(L, 1, methodName);

   string filename = getString(L, 1, "");

   FolderManager *folderManager = GameSettings::getFolderManager();

   string fullname = folderManager->findScriptFile(filename);     // Looks in luadir, levelgens dir, bots dir

   lua_pop(L, 1);    // Remove passed arg from stack
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");

   if(fullname == "")
   {
      logprintf(LogConsumer::LogError, "Could not find script file \"%s\"...", filename.c_str());
      return returnNil(L);
   }

   return returnString(L, fullname.c_str());
}


S32 LuaUtil::getRandomNumber(lua_State *L)
{
   static const char *methodName = "LuaUtil:getRandomNumber()";
   checkArgCount(L, 2, methodName);

   if(lua_isnil(L, 1))
   {
      lua_pop(L, 1);
      lua_pop(L, 2);

      return returnFloat(L, TNL::Random::readF());
   }

   S32 min = 1;
   S32 max = 0;

   if(lua_isnil(L,2))
      max = luaL_checkint(L, 1); 
   else
   {
      min = luaL_checkint(L, 1);
      max = luaL_checkint(L, 2);
   }

   lua_pop(L, 1);
   lua_pop(L, 2);

   return returnInt(L, TNL::Random::readI(min, max));
}


};


