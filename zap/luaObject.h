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

#ifndef _LUAOBJECT_H_
#define _LUAOBJECT_H_


#include "lua.h"
#include "../lua/include/lunar.h"
#include "tnl.h"
#include "point.h"
#include <string>

using namespace std;
using namespace TNL;


// Set some enumeration helpers that we'll need to pass enumeration constants to programs
#define setEnum(name) { lua_pushinteger(L, name); lua_setglobal(L, #name); }
#define setGTEnum(name) { lua_pushinteger(L, GameType::name); lua_setglobal(L, #name); }
#define setEventEnum(name) { lua_pushinteger(L, EventManager::name); lua_setglobal(L, #name); }

#define method(class, name) {#name, &class::name}


namespace Zap
{

class GameObject;
class LuaPoint;


class LuaObject
{

protected:
   static int luaPanicked(lua_State *L);
   static void clearStack(lua_State *L);
   static void checkArgCount(lua_State *L, S32 argsWanted, const char *functionName);
   static F32 getFloat(lua_State *L, S32 index, const char *functionName);
   static bool getBool(lua_State *L, S32 index, const char *functionName);
   static lua_Integer getInt(lua_State *L, S32 index, const char *functionName);
   static lua_Integer getInt(lua_State *L, S32 index, const char *functionName, S32 minVal, S32 maxVal);
   static const char *getString(lua_State *L, S32 index, const char *functionName);
   static Point getPoint(lua_State *L, S32 index, const char *functionName);
   static Point getPointOrXY(lua_State *L, S32 index, const char *functionName);

   static void setfield (lua_State *L, const char *key, F32 value);



public:
   // All of these return<T> functions work in the same way.  Include at the and of a child class method.
   // Usage: return returnInt(L, int);
   static S32 returnPoint(lua_State *L, Point &point);
   static S32 returnLuaPoint(lua_State *L, LuaPoint *point);
   static S32 returnInt(lua_State *L, S32 num);
   static S32 returnFloat(lua_State *L, F32 num);
   static S32 returnString(lua_State *L, const char *str);
   static S32 returnBool(lua_State *L, bool boolean);
   static S32 returnNil(lua_State *L);

   static void stackdump(lua_State* L);
   static void cleanupAndTerminate(lua_State *L);

   static void setLuaArgs(lua_State *L, Vector<string> args);     // Used by bots and levelgens

};

////////////////////////////////////////
////////////////////////////////////////

class LuaItem : public LuaObject
{
public:
   // "= 0" ==> make these methods "pure virtual" functions, and must be implemented in child classes!
   virtual S32 getLoc(lua_State *L) = 0;       // Center of item (returns point)
   virtual S32 getRad(lua_State *L) = 0;       // Radius of item (returns number)
   virtual S32 getVel(lua_State *L) = 0;       // Speed of item (returns point)
   virtual S32 getTeamIndx(lua_State *L) = 0;  // Team of item (returns team index)
   virtual GameObject *getGameObject() = 0;    // Return the underlying GameObject

   virtual void push(lua_State *L) { TNLAssert(false, "Unimplemented method!"); }                 // Push item onto stack
   virtual S32 getClassID(lua_State *L) { TNLAssert(false, "Unimplemented method!"); return -1; } // Object's class

   static LuaItem *getItem(lua_State *L, S32 index, U32 type, const char *functionName);
};


////////////////////////////////////////
////////////////////////////////////////

class LuaPoint : public LuaObject
{
private:
   Point mPoint;                  // Underlying point container

public:
   LuaPoint(lua_State *L);        // Lua constructor
   LuaPoint(Point point);         // C++ constructor

   ~LuaPoint();                   // Destructor

   static const char className[];

   static Lunar<LuaPoint>::RegType methods[];

   Point getPoint() { return mPoint; }

   S32 x(lua_State *L);
   S32 y(lua_State *L);

   S32 setxy(lua_State *L);
   S32 setx(lua_State *L);
   S32 sety(lua_State *L);
   S32 setAngle(lua_State *L);
   S32 setPolar(lua_State *L);

   S32 equals(lua_State *L);     // Does point equal another point?

   // Wrap some of our standard point methods
   S32 distanceTo(lua_State *L);
   S32 distSquared(lua_State *L);
   S32 angleTo(lua_State *L);
   S32 len(lua_State *L);
   S32 lenSquared(lua_State *L);
   S32 normalize(lua_State *L);

};

};

#endif

