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

#include "LuaBase.h"          // Header

#include "LuaWrapper.h"

#include "game.h"             // For loadTarget
#include "item.h"             // For item def
#include "UIMenuItems.h"      // For MenuItem def

#include "ship.h"             // For access to Ship's push function
#include "playerInfo.h"       // For access to PlayerInfo's push function
#include "ClientInfo.h"       // For one minor use below
#include "BfObject.h"         // Required for returnBfObject()


namespace Zap
{

// Constructor
LuaBase::LuaBase()
{
   // Do nothing
}


// Destructor
LuaBase::~LuaBase()
{
   // Do nothing
}


// Make sure we got the number of args we wanted
void LuaBase::checkArgCount(lua_State *L, S32 argsWanted, const char *methodName)
{
   S32 args = lua_gettop(L);

   if(args != argsWanted)     // Problem!
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s called with %d args, expected %d", methodName, args, argsWanted);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }
}


// === Centralized Parameter Checking ===
// Returns index of matching parameter profile; throws error if it can't find one.  If you get a valid profile index back,
// you can blindly convert the stack items with the confidence you'll get what you want; no further type checking is required.
// In writing this function, I tried to be extra clear, perhaps at the expense of slight redundancy.
S32 LuaBase::checkArgList(lua_State *L, const LuaFunctionProfile *functionInfos, const char *className, const char *functionName)
{
   const LuaFunctionProfile *functionInfo = NULL;

   // First, find the correct profile for this function
   for(S32 i = 0; functionInfos[i].functionName != NULL; i++)
      if(strcmp(functionInfos[i].functionName, functionName) == 0)
      {
         functionInfo = &functionInfos[i];
         break;
      }

   if(!functionInfo)
      return -1;

   return checkArgList(L, functionInfo->functionArgList, className, functionName);
}


S32 LuaBase::checkArgList(lua_State *L, const LuaFunctionArgList &functionArgList, const char *className, const char *functionName)
{
   S32 stackDepth = lua_gettop(L);
   S32 profileCount = functionArgList.profileCount;

   for(S32 i = 0; i < profileCount; i++)
   {
      const LuaBase::LuaArgType *candidateArgList = functionArgList.argList[i];     // argList is a 2D array
      bool validProfile = true;
      S32 stackPos = 0;

      for(S32 j = 0; candidateArgList[j] != END; j++)
      {
         bool ok = false;

         if(stackPos < stackDepth)
         {  
            stackPos++;
            ok = checkLuaArgs(L, candidateArgList[j], stackPos);
         }

         if(!ok)
         {
            validProfile = false;            // This profile is not the one we want... proceed to next i
            break;
         }
      }

      if(validProfile && (stackPos == stackDepth))
         return i;
   }
   
   // Uh oh... items on stack did not match any known parameter profile.  Try to construct a useful error message.
   char msg[2048];
   string params = prettyPrintParamList(functionArgList);
   dSprintf(msg, sizeof(msg), "Could not validate params for function %s::%s(). Expected%s: %s", 
                              className, functionName, functionArgList.profileCount > 1 ? " one of the following" : "", params.c_str());
   logprintf(LogConsumer::LogError, msg);

   dumpStack(L, "Current stack state");

   throw LuaException(msg);

   return -1;     // No valid profile found, but we never get here, so it doesn't really matter what we return, does it?
}


// Warning... may alter stackPos!
bool LuaBase::checkLuaArgs(lua_State *L, LuaBase::LuaArgType argType, S32 &stackPos)
{
   S32 stackDepth = lua_gettop(L);

   switch(argType)
   {
      case INT:      // Passthrough ok!
      case NUM:
         return lua_isnumber(L, stackPos);

      case INT_GE0:
         if(lua_isnumber(L, stackPos))
            return ((S32)(lua_tonumber(L, stackPos)) >= 0);

         return false;

      case NUM_GE0:
         if(lua_isnumber(L, stackPos))
            return (lua_tonumber(L, stackPos) >= 0);

         return false;

      case INTS:
      {
         bool ok = lua_isnumber(L, stackPos);

         if(ok)
            while(stackPos < stackDepth && lua_isnumber(L, stackPos))
               stackPos++;

         return ok;
      }

      case STR:               
         return lua_isstring(L, stackPos);

      case BOOL:               
         return lua_isboolean(L, stackPos);

      case PT:
         if(lua_ispoint(L, stackPos))
            return true;
         
         return false;

      // GEOM: A series of points, numbers, or a table containing a series of points or numbers
      case GEOM:
         if(lua_ispoint(L, stackPos))             // Series of Points
         {
            while(stackPos + 1 < stackDepth && lua_ispoint(L, stackPos + 1))
               stackPos++;

            return true;
         }
         else if lua_istable(L, stackPos)    // We have a table: should either contain an array of points or numbers
            return true;     // for now...  // TODO: Check!

         return false;

      case LOADOUT:
         return luaW_is<LuaLoadout>(L, stackPos);

      case ITEM:
         return luaW_is<Item>(L, stackPos);

      case TABLE:
         return lua_istable(L, stackPos);

      case WEAP_ENUM:
         if(lua_isnumber(L, stackPos))
         {
            lua_Integer i = lua_tointeger(L, stackPos);
            return (i >= 0 && i < WeaponCount);
         }
         return false;

      case WEAP_SLOT:
         if(lua_isnumber(L, stackPos))
         {
            lua_Integer i = lua_tointeger(L, stackPos);
            return (i >= 1 && i <= ShipWeaponCount);       // Slot 1, 2, or 3
         }
         return false;

      case MOD_ENUM:
         if(lua_isnumber(L, stackPos))
         {
            lua_Integer i = lua_tointeger(L, stackPos);
            return (i >= 0 && i < ModuleCount);
         }
         return false;

      case MOD_SLOT:
         if(lua_isnumber(L, stackPos))
         {
            lua_Integer i = lua_tointeger(L, stackPos);
            return (i >= 1 && i <= ShipModuleCount);       // Slot 1 or 2
         }
         return false;

      case TEAM_INDX:
         if(lua_isnumber(L, stackPos))
         {
            lua_Integer i = lua_tointeger(L, stackPos);
            // Special check for common error because Lua 1-based arrays suck monkey balls
            if(i == 0)
                logprintf(LogConsumer::LogError, "WARNING: It appears you have tried to add an item to teamIndex 0; "
                                                 "this is almost certainly an error.\n"
                                                 "If you want to add an item to the first team, specify team 1.  Remember "
                                                 "that Lua uses 1-based arrays.");
            i--;    // Subtract 1 because Lua indices start with 1, and we need to convert to C++ 0-based index
            return ((i >= 0 && i < Game::getAddTarget()->getTeamCount()) || (i + 1) == TEAM_NEUTRAL || (i + 1) == TEAM_HOSTILE);
         }
         return false;

      case ROBOT:
         return luaW_is<Robot>(L, stackPos);

      case LEVELGEN:
         return luaW_is<LuaLevelGenerator>(L, stackPos);

      case EVENT:
         if(lua_isnumber(L, stackPos))
         {
            lua_Integer i = lua_tointeger(L, stackPos);
            return (i >= 0 && i < EventManager::EventTypes);
         }
         return false;
               
      case BFOBJ:
         return luaW_is<BfObject>(L, stackPos);

      case MOVOBJ:
         return luaW_is<MoveObject>(L, stackPos);

      case ANY:
         stackPos = stackDepth;
         return true;

      default:
         TNLAssert(false, "Unknown arg type!");
         return false;
   }
}


// Pop a point object off stack, or grab two numbers and create a point from them
Point LuaBase::getPointOrXY(lua_State *L, S32 index)
{
   if(lua_ispoint(L, index))
   {
      const F32 *vec = lua_tovec(L, index);
      return Point(vec[0], vec[1]);
   }
   else
   {
      F32 x = getFloat(L, index);
      F32 y = getFloat(L, index + 1);
      return Point(x, y);
   }
}


// Will retrieve a list of points in one of several formats: points, F32s, or a table of points or F32s
Vector<Point> LuaBase::getPointsOrXYs(lua_State *L, S32 index)
{
   Vector<Point> points;
   S32 stackDepth = lua_gettop(L);

   if(lua_ispoint(L, index))          // List of points
   {
      S32 offset = 0;
      while(index + offset <= stackDepth && lua_ispoint(L, index + offset))
      {
         const F32 *vec = lua_tovec(L, index + offset);
         points.push_back(Point(vec[0], vec[1]));
         offset++;
      }
   }
   else if(lua_istable(L, index))
      getPointVectorFromTable(L, index, points);

   return points;
 }


// Make a nice looking string representation of the object at the specified index
static string stringify(lua_State *L, S32 index)
{
   int t = lua_type(L, index);
   //TNLAssert(t >= -1 && t <= LUA_TTHREAD, "Invalid type number!");
   if(t > LUA_TTHREAD || t < -1)
      return "Invalid object type id " + itos(t);

   switch (t) 
   {
      case LUA_TSTRING:   
         return "string: " + string(lua_tostring(L, index));
      case LUA_TBOOLEAN:  
         return "boolean: " + lua_toboolean(L, index) ? "true" : "false";
      case LUA_TNUMBER:    
         return "number: " + itos(S32(lua_tonumber(L, index)));
      default:             
         return lua_typename(L, t);
   }
}


// May interrupt a table traversal if this is called in the middle
void LuaBase::dumpTable(lua_State *L, S32 tableIndex, const char *msg)
{
   bool hasMsg = (strcmp(msg, "") != 0);
   logprintf("Dumping table at index %d %s%s%s", tableIndex, hasMsg ? "[" : "", msg, hasMsg ? "]" : "");

   TNLAssert(lua_type(L, tableIndex) == LUA_TTABLE || dumpStack(L), "No table at specified index!");

   // Compensate for other stuff we'll be putting on the stack
   if(tableIndex < 0)
      tableIndex -= 1;
                                                            // -- ... table  <=== arrive with table and other junk (perhaps) on the stack
   lua_pushnil(L);      // First key                        // -- ... table nil
   while(lua_next(L, tableIndex) != 0)                      // -- ... table nextkey table[nextkey]      
   {
      string key = stringify(L, -2);                  
      string val = stringify(L, -1);                  

      logprintf("%s - %s", key.c_str(), val.c_str());        
      lua_pop(L, 1);                                        // -- ... table key (Pop value; keep key for next iter.)
   }
}


bool LuaBase::dumpStack(lua_State* L, const char *msg)
{
    int top = lua_gettop(L);

    bool hasMsg = (strcmp(msg, "") != 0);
    logprintf("\nTotal in stack: %d %s%s%s", top, hasMsg ? "[" : "", msg, hasMsg ? "]" : "");

    for(S32 i = 1; i <= top; i++)
    {
      string val = stringify(L, i);
      logprintf("%d : %s", i, val.c_str());
    }

    return false;
 }


// Pop integer off stack, check its type, do bounds checking, and return it
lua_Integer LuaBase::getInt(lua_State *L, S32 index, const char *methodName, S32 minVal, S32 maxVal)
{
   lua_Integer val = getInt(L, index);

   if(val < minVal || val > maxVal)
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s called with out-of-bounds arg: %d (val=%d)", methodName, index, val);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   return val;
}


// Returns defaultVal if there is an invalid or missing value on the stack
lua_Integer LuaBase::getInt(lua_State *L, S32 index, S32 defaultVal)
{
   if(!lua_isnumber(L, index))
      return defaultVal;
   // else
   return lua_tointeger(L, index);
}


lua_Integer LuaBase::getInt(lua_State *L, S32 index)
{
   return lua_tointeger(L, index);
}


// Pop integer off stack, check its type, and return it (no bounds check)
lua_Integer LuaBase::getCheckedInt(lua_State *L, S32 index, const char *methodName)
{
   if(!lua_isnumber(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected numeric arg at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   return lua_tointeger(L, index);
}


// Pop a number off stack, convert to float, and return it (no bounds check)
F32 LuaBase::getFloat(lua_State *L, S32 index)
{
   return (F32)lua_tonumber(L, index);
}


// Pop a number off stack, convert to float, and return it (no bounds check)
F32 LuaBase::getCheckedFloat(lua_State *L, S32 index, const char *methodName)
{
   if(!lua_isnumber(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected numeric arg at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   return (F32) lua_tonumber(L, index);
}


// Return a bool at the specified index
bool LuaBase::getBool(lua_State *L, S32 index)
{
    return (bool) lua_toboolean(L, index);
}


// Pop a boolean off stack, and return it
bool LuaBase::getCheckedBool(lua_State *L, S32 index, const char *methodName, bool defaultVal)
{
   if(!lua_isboolean(L, index))
      return defaultVal;
   // else
   return (bool) lua_toboolean(L, index);
}


// Pop a string or string-like object off stack, check its type, and return it
const char *LuaBase::getString(lua_State *L, S32 index, const char *defaultVal)
{
   if(!lua_isstring(L, index))
      return defaultVal;
   // else
   return lua_tostring(L, index);
}


// Pop a string or string-like object off stack and return it
const char *LuaBase::getString(lua_State *L, S32 index)
{
   return lua_tostring(L, index);
}


// Pop a string or string-like object off stack, check its type, and return it
const char *LuaBase::getCheckedString(lua_State *L, S32 index, const char *methodName)
{
   if(!lua_isstring(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected string arg at position %d", methodName, index);
      logprintf(LogConsumer::LogError, msg);

      throw LuaException(msg);
   }

   return lua_tostring(L, index);
}


// Returns a float to a calling Lua function
S32 LuaBase::returnFloat(lua_State *L, F32 num)
{
   lua_pushnumber(L, num);
   return 1;
}


// Returns a boolean to a calling Lua function
S32 LuaBase::returnBool(lua_State *L, bool boolean)
{
   lua_pushboolean(L, boolean);
   return 1;
}


// Returns a string to a calling Lua function
S32 LuaBase::returnString(lua_State *L, const char *str)
{
   lua_pushstring(L, str);
   return 1;
}


// Returns nil to calling Lua function
S32 LuaBase::returnNil(lua_State *L)
{
   lua_pushnil(L);
   return 1;
}


// Returns a point to calling Lua function
S32 LuaBase::returnPoint(lua_State *L, const Point &pt)
{
   lua_pushvec(L, pt.x, pt.y);
   return 1;
}


// Return a table of points to calling Lua function
S32 LuaBase::returnPoints(lua_State *L, const Vector<Point> *points)
{
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not clean!");

   // Create an empty table with enough space reserved
   lua_createtable(L, points->size(), 0);                  //                                -- table                                                   
   S32 tableIndex = 1;     // Table will live on top of the stack, at index 1

   for(S32 i = 0; i < points->size(); i++)
   {
      lua_pushvec(L, points->get(i).x, points->get(i).y);  // Push point onto the stack      -- table, point
      lua_rawseti(L, tableIndex, i + 1);                   // + 1  => Lua indices 1-based    -- table[i + 1] = point                                      
   }

   return 1;
}


// Returns an int to a calling Lua function
S32 LuaBase::returnInt(lua_State *L, S32 num)
{
   lua_pushinteger(L, num);
   return 1;
}


// If we have a ship, return it, otherwise return nil
S32 LuaBase::returnShip(lua_State *L, Ship *ship)
{
   if(ship)
   {
      ship->push(L);
      return 1;
   }

   return returnNil(L);
}


S32 LuaBase::returnBfObject(lua_State *L, BfObject *bfObject)
{
   if(bfObject)
   {
      bfObject->push(L);
      return 1;
   }

   return returnNil(L);
}


S32 LuaBase::returnPlayerInfo(lua_State *L, Ship *ship)
{
   return returnPlayerInfo(L, ship->getClientInfo()->getPlayerInfo());
}


S32 LuaBase::returnPlayerInfo(lua_State *L, LuaPlayerInfo *playerInfo)
{
   playerInfo->push(L);
   return 1;
}


// Assume that table is at the top of the stack
void LuaBase::setfield (lua_State *L, const char *key, F32 value)
{
   lua_pushnumber(L, value);
   lua_setfield(L, -2, key);
}


void LuaBase::clearStack(lua_State *L)
{
   lua_settop(L, 0);
}


// Pulls values out of the table at specified index as strings, and puts them all into strings vector
void LuaBase::getPointVectorFromTable(lua_State *L, S32 index, Vector<Point> &points)
{
   // The following block loosely based on http://www.gamedev.net/topic/392970-lua-table-iteration-in-c---basic-walkthrough/

   lua_pushvalue(L, index);	// Push our table onto the top of the stack
   lua_pushnil(L);            // lua_next (below) will start the iteration, it needs nil to be the first key it pops

   // The table was pushed onto the stack at -1 (recall that -1 is equivalent to lua_gettop)
   // The lua_pushnil then pushed the table to -2, where it is currently located
   while(lua_next(L, -2))     // -2 is our table
   {
      // Grab the value at the top of the stack
      const F32 *vec = lua_tovec(L, -1);
      Point p(vec[0], vec[1]);
      points.push_back(p);

      lua_pop(L, 1);    // We extracted that value, pop it off so we can push the next element
   }
}


// Create a list of type names for displaying function signatures
static const char *argTypeNames[] = {
#  define LUA_ARG_TYPE_ITEM(a, name) name,
      LUA_ARG_TYPE_TABLE
#  undef LUA_ARG_TYPE_ITEM
};


// Return a nicely formatted list of acceptable parameter types.  Use a string to avoid dangling pointer.
string LuaBase::prettyPrintParamList(const Zap::LuaFunctionArgList &functionArgList)
{
   string msg;

   for(S32 i = 0; i < functionArgList.profileCount; i++)
   {
      //if(i > 0)
      msg += "\n\t";

      for(S32 j = 0; functionArgList.argList[i][j] != Zap::LuaBase::END; j++)
      {
         if(j > 0)
            msg += ", ";

         msg += argTypeNames[functionArgList.argList[i][j]];
      }
   }

   msg += "\n";

   return msg;
}


//static void printMethodList(const functionArgList &funProfile, const string &prefix)
//{
//   for(int i = 0; funProfile[i].functionName != NULL; i++)        // Iterate over functions
//      for(int j = 0; j < funProfile[i].profileCount; j++)         // Iterate over arg profiles for that function, generating one line for each
//      {
//         std::string line = prefix + "    --> " + funProfile[i].functionName + "(";
//            
//         for(int k = 0; funProfile[i].argList[j][k] != LuaBase::END; k++)  // Iterate over args within a given profile, appending each to the output line
//         {
//            if(k != 0)
//               line += ", ";
//            line += argTypeNames[funProfile[i].argList[j][k]];   
//         }
//         line += ")";
//
//         printf("%s\n", line.c_str());    // Print the line
//      }
//}


// Helper for printDocs(), called from luaW with -luadocs option
void LuaBase::printFunctions(const ArgMap &argMap, const map<ClassName, unsigned int> &nodeMap, 
                             const vector<Node> &nodeList, const string &prefix, unsigned int nodeIndex)
{
   if(prefix.length() > 8)
      printf(prefix.substr(0, prefix.length() - 8).c_str());

   if(prefix != "")
      printf(" +----- ");  

   printf("%s\n", nodeList[nodeIndex].first);  // Print ourselves
      
   //printMethodList(argMap.find(nodeList[nodeIndex].first)->second, "");

   if(nodeList[nodeIndex].second.size() == 0)
      return;

   // Output the children
   for(unsigned int i = 0; i < nodeList[nodeIndex].second.size(); i++)
   {
      std::string tmpPrefix = prefix;
      if(i < nodeList[nodeIndex].second.size() - 1)
         tmpPrefix += " |      ";
      else
         tmpPrefix += "        ";
      unsigned int index = nodeMap.find(nodeList[nodeIndex].second[i])->second;
      printFunctions(argMap, nodeMap, nodeList, tmpPrefix, index);      // Recursive!  
   }
}


void LuaBase::printLooseFunctions()
{
   printf("The following Bitfighter functions are also available:\n");
   //printMethodList(LuaScriptRunner::functionArgs, "");
}


#define SCRIPT_CONTEXT_KEY "running_script_context"

LuaBase::ScriptContext LuaBase::getScriptContext(lua_State *L)
{
   lua_getfield(L, LUA_REGISTRYINDEX, SCRIPT_CONTEXT_KEY);
   S32 context = lua_tointeger(L, -1);
   lua_pop(L, 1);    // Remove the value we just added from the stack

   // Bounds checking
   if(context < 0 || context > ScriptContextCount)
      return UnknownContext;

   return (ScriptContext)context;
}


void LuaBase::setScriptContext(lua_State *L, ScriptContext context)
{
   lua_pushinteger(L, context);
   lua_setfield(L, LUA_REGISTRYINDEX, SCRIPT_CONTEXT_KEY);     // Pops the int we just pushed from the stack
}


};


////////////////////////////////////////
////////////////////////////////////////

// This is deliberately outside the zap namespace -- it provides a bridge to the printFunctions function above from luaW
void printFunctions(const ArgMap &argMap, const std::map<ClassName, unsigned int> &nodeMap, 
                           const std::vector<Node> &nodeList, const std::string &prefix, unsigned int nodeIndex)
{
   Zap::LuaBase::printFunctions(argMap, nodeMap, nodeList, prefix, nodeIndex);
}


void printLooseFunctions()
{
   Zap::LuaBase::printLooseFunctions();
}
