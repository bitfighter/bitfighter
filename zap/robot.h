//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#ifndef _ROBOT_H_
#define _ROBOT_H_

#include "gameObject.h"
#include "moveObject.h"
#include "sparkManager.h"
#include "sfx.h"
#include "timer.h"
#include "shipItems.h"
#include "gameWeapons.h"
#include "ship.h"

//// Need to include lua headers this way
// extern "C" {  
// #include "../lua/include/lua.h"  
// #include "../lua/include/lualib.h"  
// #include "../lua/include/lauxlib.h"  
// }  
//
//#include "../lua/include/luna.h"
//#include "LuaCall.h"    // For calling Lua functions from C++

//#include "../luaplus/LuaPlus.h"

namespace Zap
{

class Item;







/**
 * This is the wrapper around the C++ object found in object.cc
 * Everything this object has done to it is passed on FROM Lua to the real C++
 * object through the pointer 'real_object'
 * Notice that I kept the function names the same for simplicity.
 */
















// class derived_class_name: public base_class_name
class Robot : public Ship
{
   typedef Ship Parent;

private:
   // Scripting stuff
   //lua_State *mLuaInterpreter;
   ////lua_State *L;
   //LuaPlus::LuaState *L;
   //struct luaL_reg misc;

   S32 mCurrentZone;


   //LuaPlus::LuaFunction<const void*> getMove();



public:
   Robot(StringTableEntry robotName="", S32 team = -1, Point p = Point(0,0), F32 m = 1.0);      // Constructor
   ~Robot();          // Destructor 
   
   void idle(IdleCallPath path);

   void processMove(U32 stateIndex);

   void processArguments(S32 argc, const char **argv);
   void onAddedToGame(Game *);

   void render(S32 layerIndex);
   S32 getCurrentZone();
   void setCurrentZone(S32 zone);

   // Some informational functions
   F32 getAngleXY(F32 x, F32 y);

   // Robot functions
   
   bool findNearestShip(Point &loc);      // Return location of nearest known ship within a given area


private:
  int attribute;
  std::string message;
/////////////////


   TNL_DECLARE_CLASS(Robot);
};



// The header file for the real C++ object

class LuaGameObject{

public:
  // Constants

  // Initialize the pointer
  //LuaGameObject(lua_State *L);
  //~LuaGameObject();

  S32 testval;


  static const char className[];

//static Luna<LuaGameObject>::RegType methods[];

 
  //void setObject(lua_State *L);

  // Methods we will need to use


  /* S32 getZoneCenterXY(lua_State *L);
   S32 getGatewayToXY(lua_State *L);
   S32 getZoneCount(lua_State *L);
   S32 getCurrentZone(lua_State *L);

   S32 getAngle(lua_State *L);
   S32 getPosXY(lua_State *L);

   S32 setAngle(lua_State *L);
   S32 setAngleXY(lua_State *L);
   S32 getAngleXY(lua_State *L);
   S32 hasLosXY(lua_State *L);

   S32 findObjects(lua_State *L);
   
   S32 setThrustAng(lua_State *L);
   S32 setThrustXY(lua_State *L);

   S32 fire(lua_State *L);
   S32 setWeapon(lua_State *L);*/
   //S32 globalMsg(lua_State *L);
   //S32 teamMsg(lua_State *L);
   //S32 getAimAngle(lua_State *L);
   //S32 logprint(lua_State *L);
  


private:
  // The pointer to the 'real object' defined in object.cc
  Robot* thisRobot;
};




};

#endif
