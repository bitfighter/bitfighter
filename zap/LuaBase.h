//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LUABASE_H_
#define _LUABASE_H_

#include "lua.h"
#include "LuaException.h"     // LuaException def

#include "Point.h"
#include "tnlTypes.h"
#include "tnlVector.h"

#include "shipItems.h"
#include "WeaponInfo.h"

#include <string>
#include <vector>
#include <map>

#define ARRAYDEF(...) __VA_ARGS__                  // Wrap inline array definitions so they don't confuse the preprocessor   

using namespace std;
using namespace TNL;

namespace Zap
{

// Forward declarations
class Ship;
class Team;
class LuaPlayerInfo;
class MenuItem;
class BfObject;
class ServerGame;
struct LuaFunctionProfile;    // Defined below
struct LuaFunctionArgList;    // Defined below


typedef const char* ClassName;
typedef map <ClassName, const LuaFunctionProfile *> ArgMap;      // Map of class name and arguments list, for documentation
typedef pair<ClassName, vector<ClassName> > Node;

/**
 * Because windef.h (which is included by windows.h) typedefs BOOL and INT types,
 * we have to put this enum in a namespace. The `using namespace LuaArgs` statements
 * could be removed entirely if constructor arguments could be checked using the
 * FUNARG system.
 */
namespace LuaArgs
{

   //                 Enum       Name
#  define LUA_ARG_TYPE_TABLE \
   LUA_ARG_TYPE_ITEM( BOOL,        "Boolean"                                      ) \
   LUA_ARG_TYPE_ITEM( INT,         "Integer"                                      ) \
   LUA_ARG_TYPE_ITEM( INT_GE0,     "Integer >= 0"                                 ) \
   LUA_ARG_TYPE_ITEM( INTS,        "One or more integers"                         ) \
   LUA_ARG_TYPE_ITEM( NUM,         "Number"                                       ) \
   LUA_ARG_TYPE_ITEM( NUM_GE0,     "Number >= 0"                                  ) \
   LUA_ARG_TYPE_ITEM( STR,         "String"                                       ) \
   LUA_ARG_TYPE_ITEM( STRS,        "One or more strings"                          ) \
   LUA_ARG_TYPE_ITEM( PT,          "Lua point"                                    ) \
   LUA_ARG_TYPE_ITEM( SIMPLE_LINE, "Pair of Lua points (singly or in table)"      ) \
   LUA_ARG_TYPE_ITEM( LINE,        "Two or more Lua points (singly or in table)"  ) \
   LUA_ARG_TYPE_ITEM( POLY,        "Three or more Lua points (singly or in table)") \
   LUA_ARG_TYPE_ITEM( TABLE,       "Lua table"                                    ) \
   LUA_ARG_TYPE_ITEM( ITEM,        "Item Object"                                  ) \
   LUA_ARG_TYPE_ITEM( WEAP_ENUM,   "WeaponEnum"                                   ) \
   LUA_ARG_TYPE_ITEM( WEAP_SLOT,   "Weapon slot #"                                ) \
   LUA_ARG_TYPE_ITEM( MOD_ENUM,    "ModuleEnum"                                   ) \
   LUA_ARG_TYPE_ITEM( MOD_SLOT,    "Module slot #"                                ) \
   LUA_ARG_TYPE_ITEM( TEAM_INDX,   "Team index"                                   ) \
   LUA_ARG_TYPE_ITEM( GEOM,        "Geometry (see documentation)"                 ) \
   LUA_ARG_TYPE_ITEM( ROBOT,       "Robot Object"                                 ) \
   LUA_ARG_TYPE_ITEM( LEVELGEN,    "Levelgen Script"                              ) \
   LUA_ARG_TYPE_ITEM( EVENT,       "Event"                                        ) \
   LUA_ARG_TYPE_ITEM( MOVOBJ,      "MoveObject"                                   ) \
   LUA_ARG_TYPE_ITEM( BFOBJ,       "BfObject (or child class)"                    ) \
   LUA_ARG_TYPE_ITEM( ANY,         "Any combination of 0 or more arguments"       ) \
      

   // Create the enum declaration
   enum LuaArgType {
#     define LUA_ARG_TYPE_ITEM(value, b) value,
         LUA_ARG_TYPE_TABLE
#     undef LUA_ARG_TYPE_ITEM
      END      // End of list sentinel value
   };
};

enum ScriptContext {
   RobotContext,        // When a robot is running (usually during the init phase)
   LevelgenContext,     // When a levelgen is running (usually during the init phase)
   PluginContext,       // When a plugin runs
   ConsoleContext,      // For code running from the console
   ScriptContextCount,
   UnknownContext
};

void checkArgCount(lua_State *L, S32 argsWanted, const char *methodName);
void setfield (lua_State *L, const char *key, F32 value);

bool isPointAtTableIndex(lua_State *L, S32 tableIndex, S32 indexWithinTable);
Point getCheckedVec(lua_State *L, S32 index, const char *methodName);      // TODO: Delete me



// This doesn't really need to be virtual, but something here does, to allow dynamic_casting to occur... I picked
// this one pretty much arbitrarily...  it won't be overridden.
void getPointVectorFromTable(lua_State *L, S32 index, Vector<Point> &points);
Point getPointOrXY(lua_State *L, S32 index);
Vector<Point> getPointsOrXYs(lua_State *L, S32 index);
Vector<Vector<Point> > getPolygons(lua_State *L, S32 index);

WeaponType getWeaponType(lua_State *L, S32 index);
ShipModule getShipModule(lua_State *L, S32 index);

// All of these return<T> functions work in the same way.  Include at the end of a child class method.
// Usage: return returnInt(L, int);

//template<class T> S32 returnVal(lua_State *L, T value, bool letLuaDelete = true);

// The basics:
S32 returnInt(lua_State *L, S32 num);
S32 returnFloat(lua_State *L, F32 num);
S32 returnString(lua_State *L, const char *str);
S32 returnBool(lua_State *L, bool boolean);
S32 returnNil(lua_State *L);

// More complex objects:
S32 returnPoint(lua_State *L, const Point &point);
S32 returnPoints(lua_State *L, const Vector<Point> *);
S32 returnPolygons(lua_State *L, const Vector<Vector<Point> > &polys);
S32 returnMenuItem(lua_State *L, MenuItem *menuItem);
S32 returnShip(lua_State *L, Ship *ship);                // Handles null references properly
S32 returnTeam(lua_State *L, Team *team);
S32 returnTeamIndex(lua_State *L, S32 teamIndex);
S32 returnBfObject(lua_State *L, BfObject *bfObject);

S32 returnPlayerInfo(lua_State *L, Ship *ship);
S32 returnPlayerInfo(lua_State *L, LuaPlayerInfo *playerInfo);
S32 returnGameInfo(lua_State *L, ServerGame *serverGame);
S32 returnShipModule(lua_State *L, ShipModule module);
S32 returnWeaponType(lua_State *L, WeaponType weapon);

void clearStack(lua_State *L);

F32 getFloat(lua_State *L, S32 index);
F32 getCheckedFloat(lua_State *L, S32 index, const char *methodName);

bool getBool(lua_State *L, S32 index);
bool getCheckedBool(lua_State *L, S32 index, const char *methodName, bool defaultVal);

lua_Integer getInt(lua_State *L, S32 index);
lua_Integer getInt(lua_State *L, S32 index, S32 defaultVal);
lua_Integer getInt(lua_State *L, S32 index, const char *methodName, S32 minVal, S32 maxVal);

S32 getTeamIndex(lua_State *L, S32 index);

lua_Integer getCheckedInt(lua_State *L, S32 index, const char *methodName);

const char *getString(lua_State *L, S32 index);
const char *getCheckedString(lua_State *L, S32 index, const char *methodName);

const char *getString(lua_State *L, S32 index, const char *defaultVal);

S32 luaTableCopy(lua_State *L);

/////
// Script context
ScriptContext getScriptContext(lua_State *L);
void setScriptContext(lua_State *L, ScriptContext context);

/////
// Documenting and help
S32 checkArgList(lua_State *L, const LuaFunctionProfile *functionInfos,   const char *className, const char *functionName);
S32 checkArgList(lua_State *L, const LuaFunctionArgList &functionArgList, const char *className, const char *functionName);
S32 checkArgList(lua_State *L, const char *moduleName, const char *functionName);

bool checkLuaArgs(lua_State *L, LuaArgs::LuaArgType argType, S32 &stackPos);

string prettyPrintParamList(const LuaFunctionArgList &functionInfo);

/////
// Debugging helpers -- both return false so they can be jammed into an assert
bool dumpTable(lua_State *L, S32 tableIndex, const char *msg = "");
bool dumpStack(lua_State* L, const char *msg = "");

static const int MAX_PROFILE_ARGS = 6;          // Max used so far = 3
static const int MAX_PROFILES = 4;              // Max used so far = 2


// This is a list of possible arguments for a function, along with the number of arguments actually presented
struct LuaFunctionArgList {
   LuaArgs::LuaArgType argList[MAX_PROFILES][MAX_PROFILE_ARGS];
   int profileCount;
};


// This is an asociation of a LuaFunctionArgList with the function name it is associated with
struct LuaFunctionProfile {
   const char         *functionName;
   LuaFunctionArgList  functionArgList;   
};


// Like a LuaFunctionProfile, but with a pointer to a function
struct LuaStaticFunctionProfile
{
   const char         *functionName;
   LuaFunctionArgList  functionArgList;
   lua_CFunction       function;
};

};


#endif

