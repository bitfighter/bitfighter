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

#ifndef _LUABASE_H_
#define _LUABASE_H_

#include "lua.h"
#include "../lua/include/lunar.h"

#include "LuaException.h"           // LuaException def

#include "Point.h"
#include "tnlTypes.h"
#include "tnlVector.h"

#include <string>
#include <vector>
#include <map>

using namespace std;
using namespace TNL;

namespace Zap
{

// Forward declarations
class Ship;
class LuaPlayerInfo;
class MenuItem;
class BfObject;
struct LuaFunctionProfile;    // Defined below
struct LuaFunctionArgList;    // Defined below


typedef const char* ClassName;
typedef map <ClassName, const LuaFunctionProfile *> ArgMap;      // Map of class name and arguments list, for documentation
typedef pair<ClassName, vector<ClassName> > Node;

class LuaBase
{

public:
   //                 Enum       Name
#  define LUA_ARG_TYPE_TABLE \
   LUA_ARG_TYPE_ITEM( BOOL,      "Boolean"                                     ) \
   LUA_ARG_TYPE_ITEM( INT,       "Integer"                                     ) \
   LUA_ARG_TYPE_ITEM( INT_GE0,   "Integer >= 0"                                ) \
   LUA_ARG_TYPE_ITEM( INTS,      "One or more integers"                        ) \
   LUA_ARG_TYPE_ITEM( NUM,       "Number"                                      ) \
   LUA_ARG_TYPE_ITEM( NUM_GE0,   "Number >= 0"                                 ) \
   LUA_ARG_TYPE_ITEM( STR,       "String"                                      ) \
   LUA_ARG_TYPE_ITEM( PT,        "Point (or two numbers)"                      ) \
   LUA_ARG_TYPE_ITEM( TABLE,     "Lua table"                                   ) \
   LUA_ARG_TYPE_ITEM( LOADOUT,   "Loadout Object"                              ) \
   LUA_ARG_TYPE_ITEM( ITEM,      "Item Object"                                 ) \
   LUA_ARG_TYPE_ITEM( WEAP_ENUM, "WeaponEnum"                                  ) \
   LUA_ARG_TYPE_ITEM( WEAP_SLOT, "Weapon slot #"                               ) \
   LUA_ARG_TYPE_ITEM( MOD_ENUM,  "ModuleEnum"                                  ) \
   LUA_ARG_TYPE_ITEM( MOD_SLOT,  "Module slot #"                               ) \
   LUA_ARG_TYPE_ITEM( TEAM_INDX, "Team index"                                  ) \
   LUA_ARG_TYPE_ITEM( GEOM,      "Geometry (see documentation)"                ) \
   LUA_ARG_TYPE_ITEM( ROBOT,     "Robot Object"                                ) \
   LUA_ARG_TYPE_ITEM( LEVELGEN,  "Levelgen Script"                             ) \
   LUA_ARG_TYPE_ITEM( EVENT,     "Event"                                       ) \
   LUA_ARG_TYPE_ITEM( MOVOBJ,    "MoveObject"                                  ) \
   LUA_ARG_TYPE_ITEM( BFOBJ,     "BfObject (or child class)"                   ) \
   LUA_ARG_TYPE_ITEM( ANY,       "Any combination of 0 or more arguments"      ) \
      

   // Create the enum declaration
   enum LuaArgType {
#     define LUA_ARG_TYPE_ITEM(value, b) value,
         LUA_ARG_TYPE_TABLE
#     undef LUA_ARG_TYPE_ITEM
      END      // End of list sentinel value
   };

public:
   enum ScriptContext {
      RobotContext,        // When a robot is running (usually during the init phase)
      LevelgenContext,     // When a levelgen is running (usually during the init phase)
      PluginContext,       // When a plugin runs
      ConsoleContext,      // For code running from the console
      ScriptContextCount,
      UnknownContext
   };

   LuaBase();     // Constuctor
   ~LuaBase();    // Destructor

   static void checkArgCount(lua_State *L, S32 argsWanted, const char *methodName);
   static void setfield (lua_State *L, const char *key, F32 value);

   // This doesn't really need to be virtual, but something here does, to allow dynamic_casting to occur... I picked
   // this one pretty much arbitrarily...  it won't be overridden.
   static void getPointVectorFromTable(lua_State *L, S32 index, Vector<Point> &points);
   static Point getPointOrXY(lua_State *L, S32 index);
   static Vector<Point> getPointsOrXYs(lua_State *L, S32 index);

   // All of these return<T> functions work in the same way.  Include at the end of a child class method.
   // Usage: return returnInt(L, int);

   //template<class T> S32 returnVal(lua_State *L, T value, bool letLuaDelete = true);

   // The basics:
   static S32 returnInt(lua_State *L, S32 num);
   static S32 returnFloat(lua_State *L, F32 num);
   static S32 returnString(lua_State *L, const char *str);
   static S32 returnBool(lua_State *L, bool boolean);
   static S32 returnNil(lua_State *L);

   // More complex objects:
   static S32 returnPoint(lua_State *L, const Point &point);
   static S32 returnPoints(lua_State *L, const Vector<Point> *);
   static S32 returnMenuItem(lua_State *L, MenuItem *menuItem);
   static S32 returnShip(lua_State *L, Ship *ship);                // Handles null references properly
   static S32 returnBfObject(lua_State *L, BfObject *bfObject);

   static S32 returnPlayerInfo(lua_State *L, Ship *ship);
   static S32 returnPlayerInfo(lua_State *L, LuaPlayerInfo *playerInfo);

   static void clearStack(lua_State *L);

   static F32 getFloat(lua_State *L, S32 index);
   static F32 getCheckedFloat(lua_State *L, S32 index, const char *methodName);

   static bool getBool(lua_State *L, S32 index);
   static bool getCheckedBool(lua_State *L, S32 index, const char *methodName, bool defaultVal);

   static lua_Integer getInt(lua_State *L, S32 index);
   static lua_Integer getInt(lua_State *L, S32 index, S32 defaultVal);
   static lua_Integer getInt(lua_State *L, S32 index, const char *methodName, S32 minVal, S32 maxVal);

   static S32 getTeamIndex(lua_State *L, S32 index);

   static lua_Integer getCheckedInt(lua_State *L, S32 index, const char *methodName);

   static const char *getString(lua_State *L, S32 index);
   static const char *getCheckedString(lua_State *L, S32 index, const char *methodName);

   static const char *getString(lua_State *L, S32 index, const char *defaultVal);

   /////
   // Script context
   static ScriptContext getScriptContext(lua_State *L);
   static void setScriptContext(lua_State *L, ScriptContext context);

   /////
   // Documenting and help
   static S32 checkArgList(lua_State *L, const LuaFunctionProfile *functionInfos,   const char *className, const char *functionName);
   static S32 checkArgList(lua_State *L, const LuaFunctionArgList &functionArgList, const char *className, const char *functionName);

   static bool checkLuaArgs(lua_State *L, LuaBase::LuaArgType argType, S32 &stackPos);

   static string prettyPrintParamList(const LuaFunctionArgList &functionInfo);
   static void printFunctions(const ArgMap &argMap, const map<ClassName, unsigned int> &nodeMap, 
                              const vector<Node> &nodeList, const string &prefix, unsigned int nodeIndex);
   static void printLooseFunctions();


   /////
   // Debugging helpers
   static void dumpTable(lua_State *L, S32 tableIndex, const char *msg = "");
   static bool dumpStack(lua_State* L, const char *msg = "");
};


static const int MAX_PROFILE_ARGS = 6;          // Max used so far = 3
static const int MAX_PROFILES = 4;              // Max used so far = 2


// This is a list of possible arguments for a function, along with the number of arguments actually presented
struct LuaFunctionArgList {
   LuaBase::LuaArgType argList[MAX_PROFILES][MAX_PROFILE_ARGS];
   const int profileCount;
};


// This is an asociation of a LuaFunctionArgList with the function name it is associated with
struct LuaFunctionProfile {
   const char         *functionName;
   LuaFunctionArgList  functionArgList;   
};

};


#endif

