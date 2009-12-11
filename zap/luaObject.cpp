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

#include "luaObject.h"
#include "luaGameInfo.h"    // For LuaPoint
#include "tnlLog.h"         // For logprintf

#include "gameItems.h"     // For getItem()
#include "item.h"          // For getItem()
#include "flagItem.h"      // For getItem()
#include "robot.h"         // For getItem()
#include "huntersGame.h"   // For getItem()
#include "soccerGame.h"    // For getItem()
#include "projectile.h"    // For getItem()
#include "teleporter.h"
#include "engineeredObjects.h"    // For getItem()



namespace Zap
{

void LuaObject::setLuaArgs(lua_State *L, Vector<string> args)
{
   // Now pass in any args specified in the level file.  By convention, we'll pass in the name of the robot/script as the 0th element.
   lua_createtable(L, args.size(), 0);

   for(S32 i = 0; i < args.size(); i++)
   {
      lua_pushstring(L, args[i].c_str());
      lua_rawseti(L, -2, i);
   }
   lua_setglobal(L, "arg");
}


// Returns a point to calling Lua function
S32 LuaObject::returnPoint(lua_State *L, Point point)
{
   //lua_createtable(L, 0, 2);         // creates a table with 2 fields
   //setfield(L, "x", point.x);        // table.x = x
   //setfield(L, "y", point.y);        // table.y = y

   LuaPoint *pt = new LuaPoint(point);
   Lunar<LuaPoint>::push(L, pt, true);     // true will allow Lua to delete this object when it goes out of scope

   return 1;
}


// Returns an existing LuaPoint to calling Lua function
S32 LuaObject::returnLuaPoint(lua_State *L, LuaPoint *point)
{
   Lunar<LuaPoint>::push(L, point, true);     // true will allow Lua to delete this object when it goes out of scope
   return 1;
}


// Returns an int to a calling Lua function
S32 LuaObject::returnInt(lua_State *L, S32 num)
{
   lua_pushinteger(L, num);
   return 1;
}


// Returns a float to a calling Lua function
S32 LuaObject::returnFloat(lua_State *L, F32 num)
{
   lua_pushnumber(L, num);
   return 1;
}


// Returns a boolean to a calling Lua function
S32 LuaObject::returnBool(lua_State *L, bool boolean)
{
   lua_pushboolean(L, boolean);
   return 1;
}


// Returns a string to a calling Lua function
S32 LuaObject::returnString(lua_State *L, const char *str)
{
   lua_pushstring(L, str);
   return 1;
}


// Returns nil to calling Lua function
S32 LuaObject::returnNil(lua_State *L)
{
   lua_pushnil(L);
   return 1;
}


// Overwrite Lua's panicky panic function with something that doesn't kill the whole game
// if something goes wrong!
int LuaObject::luaPanicked(lua_State *L)
{
   string msg = lua_tostring(L, 1);
   lua_getglobal(L, "ERROR");    // <-- what is this for?

   throw LuaException(msg);

   return 0;
}


void LuaObject::clearStack(lua_State *L)
{
   lua_pop(L, lua_gettop(L));
}


void LuaObject::cleanupAndTerminate(lua_State *L)
{
   if(L)
   {
      // Force gc to clear out any lingering references
      lua_gc(L, LUA_GCCOLLECT, 0);  // Fallback
      lua_close(L);
   }
}

// Assume that table is at the top of the stack
void LuaObject::setfield (lua_State *L, const char *key, F32 value)
{
   lua_pushnumber(L, value);
   lua_setfield(L, -2, key);
}


// Make sure we got the number of args we wanted
void LuaObject::checkArgCount(lua_State *L, S32 argsWanted, const char *functionName)
{
   S32 args = lua_gettop(L);

   if(args != argsWanted)     // Problem!
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s called with %d args, expected %d", functionName, args, argsWanted);
      logprintf(msg);

      throw LuaException(msg);
   }
}


// Pop integer off stack, check its type, do bounds checking, and return it
lua_Integer LuaObject::getInt(lua_State *L, S32 index, const char *functionName, S32 minVal, S32 maxVal)
{
   lua_Integer val = getInt(L, index, functionName);

   if(val < minVal || val > maxVal)
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s called with out-of-bounds arg: %d (val=%d)", functionName, index, val);
      logprintf(msg);

      throw LuaException(msg);
   }

   return val;
}


// Pop integer off stack, check its type, and return it (no bounds check)
lua_Integer LuaObject::getInt(lua_State *L, S32 index, const char *functionName)
{
   if(!lua_isnumber(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected numeric arg at position %d", functionName, index);
      logprintf(msg);

      throw LuaException(msg);
   }

   return lua_tointeger(L, index);
}


// Pop a number off stack, convert to float, and return it (no bounds check)
F32 LuaObject::getFloat(lua_State *L, S32 index, const char *functionName)
{
   if(!lua_isnumber(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected numeric arg at position %d", functionName, index);
      logprintf(msg);

      throw LuaException(msg);
   }

   return (F32) lua_tonumber(L, index);
}


// Pop a boolean off stack, and return it
bool LuaObject::getBool(lua_State *L, S32 index, const char *functionName)
{
   if(!lua_isboolean(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected boolean arg at position %d", functionName, index);
      logprintf(msg);

      throw LuaException(msg);
   }

   return (bool) lua_toboolean(L, index);
}


// Pop a string or string-like object off stack, check its type, and return it
const char *LuaObject::getString(lua_State *L, S32 index, const char *functionName)
{
   if(!lua_isstring(L, index))
   {
      char msg[256];
      dSprintf(msg, sizeof(msg), "%s expected string arg at position %d", functionName, index);
      logprintf(msg);

      throw LuaException(msg);
   }

   return lua_tostring(L, index);
}


// Pop a point object off stack, check its type, and return it
Point LuaObject::getPoint(lua_State *L, S32 index, const char *functionName)
{
   return Lunar<LuaPoint>::check(L, index)->getPoint();
}


LuaItem *LuaItem::getItem(lua_State *L, S32 index, U32 type, const char *functionName)
{
   switch(type)
   {
      case ShipType:
        return  Lunar<LuaShip>::check(L, index);

      case BulletType:
            // pass through
      case MineType:
            // pass through
      case SpyBugType:
         return Lunar<LuaProjectile>::check(L, index);

      case ResourceItemType:
         return Lunar<ResourceItem>::check(L, index);
      case TestItemType:
         return Lunar<TestItem>::check(L, index);
      case FlagType:
         return Lunar<FlagItem>::check(L, index);



         // gofast type

      case RobotType:
         return Lunar<LuaShip>::check(L, index);
      case TeleportType:
         return Lunar<Teleporter>::check(L, index);
      case AsteroidType:
         return Lunar<Asteroid>::check(L, index);
      case RepairItemType:
         return Lunar<RepairItem>::check(L, index);
      case SoccerBallItemType:
         return Lunar<SoccerBallItem>::check(L, index);
      case NexusFlagType:
         return Lunar<HuntersFlagItem>::check(L, index);
      case TurretType:
         return Lunar<Turret>::check(L, index);
      case ForceFieldProjectorType:
         return Lunar<ForceFieldProjector>::check(L, index);

      default:
         char msg[256];
         dSprintf(msg, sizeof(msg), "%s expected item as arg at position %d", functionName, index);
         logprintf(msg);

         throw LuaException(msg);
   }
}


// Adapted from http://cc.byexamples.com/20081119/lua-stack-dump-for-c/
void LuaObject::stackdump(lua_State* l)
{
    int top = lua_gettop(l);

    logprintf("total in stack %d\n",top);

    for (S32 i = 1; i <= top; i++)
    {  /* repeat for each level */
        int t = lua_type(l, i);
        switch (t) {
            case LUA_TSTRING:  /* strings */
                logprintf("string: '%s'", lua_tostring(l, i));
                break;
            case LUA_TBOOLEAN:  /* booleans */
                logprintf("boolean %s",lua_toboolean(l, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:  /* numbers */
                logprintf("number: %g", lua_tonumber(l, i));
                break;
            default:  /* other values */
                logprintf("%s", lua_typename(l, t));
                break;
        }
    }
 }

////////////////////////////////////
////////////////////////////////////

const char LuaPoint::className[] = "Point";      // Class name as it appears to Lua scripts

// Lua Constructor
LuaPoint::LuaPoint(lua_State *L)
{
   static const char *methodName = "LuaPoint constructor";

   checkArgCount(L, 2, methodName);
   F32 x =  getFloat(L, 1, methodName);
   F32 y =  getFloat(L, 2, methodName);

   mPoint = Point(x, y);
}


// C++ Constructor -- specify items
LuaPoint::LuaPoint(Point point)
{
   mPoint = point;
}


// Destructor
LuaPoint::~LuaPoint()
{
   // logprintf("deleted LuaPoint object (%p)\n", this);
}


// Define the methods we will expose to Lua
Lunar<LuaPoint>::RegType LuaPoint::methods[] =
{
   method(LuaPoint, x),
   method(LuaPoint, y),

   method(LuaPoint, setx),
   method(LuaPoint, sety),
   method(LuaPoint, setxy),

   method(LuaPoint, equals),
   method(LuaPoint, distanceTo),
   method(LuaPoint, distSquared),
   method(LuaPoint, angleTo),
   method(LuaPoint, len),
   method(LuaPoint, lenSquared),
   method(LuaPoint, normalize),


   {0,0}    // End method list
};


S32 LuaPoint::x(lua_State *L)  { return returnFloat(L, mPoint.x); }
S32 LuaPoint::y(lua_State *L)  { return returnFloat(L, mPoint.y); }


S32 LuaPoint::setxy(lua_State *L)
{
   static const char *methodName = "LuaPoint:setxy()";
   checkArgCount(L, 2, methodName);
   F32 x =  getFloat(L, 1, methodName);
   F32 y =  getFloat(L, 2, methodName);

   mPoint = Point(x, y);
   return 0;
}


S32 LuaPoint::setx(lua_State *L)
{
   static const char *methodName = "LuaPoint:setx()";
   checkArgCount(L, 1, methodName);
   F32 x =  getFloat(L, 1, methodName);

   mPoint.x = x;
   return 0;
}


S32 LuaPoint::sety(lua_State *L)
{
   static const char *methodName = "LuaPoint:sety()";
   checkArgCount(L, 1, methodName);
   F32 y =  getFloat(L, 1, methodName);

   mPoint.y = y;
   return 0;
}

#define abs(x) (x) > 0 ? (x) : -(x)

// Are two points equal?
S32 LuaPoint::equals(lua_State *L)
{
   static const char *methodName = "LuaPoint:equals()";
   checkArgCount(L, 1, methodName);
   Point point = LuaObject::getPoint(L, 1, methodName);

   double EPSILON = .000000001;
   return returnBool(L, (abs(mPoint.x - point.x) < EPSILON)  &&  (abs(mPoint.y - point.y) < EPSILON) );
}


S32 LuaPoint::len(lua_State *L)
{
   checkArgCount(L, 0, "LuaPoint:len()");
   return returnFloat(L, mPoint.len());
}


S32 LuaPoint::lenSquared(lua_State *L)
{
   checkArgCount(L, 0, "LuaPoint:lenSquared()");
   return returnFloat(L, mPoint.lenSquared());
}


S32 LuaPoint::distanceTo(lua_State *L)
{
   static const char *methodName = "LuaPoint:distanceTo()";

   checkArgCount(L, 1, methodName);
   Point point = LuaObject::getPoint(L, 1, methodName);

   return returnFloat(L, mPoint.distanceTo(point));
}


S32 LuaPoint::distSquared(lua_State *L)
{
   static const char *methodName = "LuaPoint:distSquared()";

   checkArgCount(L, 1, methodName);
   Point point = LuaObject::getPoint(L, 1, methodName);

   return returnFloat(L, mPoint.distSquared(point));
}


S32 LuaPoint::angleTo(lua_State *L)
{
   static const char *methodName = "LuaPoint:angleTo()";

   checkArgCount(L, 1, methodName);
   Point point = LuaObject::getPoint(L, 1, methodName);

   return returnFloat(L, mPoint.angleTo(point));
}


S32 LuaPoint::normalize(lua_State *L)
{
   static const char *methodName = "LuaPoint:normalize()";

   checkArgCount(L, 1, methodName);
   F32 len = LuaObject::getFloat(L, 1, methodName);

   mPoint.normalize(len);
   return returnLuaPoint(L, this);
}


}
