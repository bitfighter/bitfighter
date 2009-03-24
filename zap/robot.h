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

#include "gameType.h"
#include "gameObject.h"
#include "moveObject.h"

#include "sparkManager.h"
#include "sfx.h"
#include "timer.h"
#include "shipItems.h"
#include "gameWeapons.h"
#include "ship.h"

#include "lua.h"
#include "../lua/include/luna.h"


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
   lua_State *L;                // Main Lua state variable

   void logError(string err);   // In case of error...
   string mFilename;            // Name of file script was loaded from

   S32 mCurrentZone;            // Zone robot is currently in

   enum {
      RobotRespawnDelay = 1500,
   };

public:
   Robot(StringTableEntry robotName="", S32 team = -1, Point p = Point(0,0), F32 m = 1.0);      // Constructor
   ~Robot();          // Destructor 
   
   void idle(IdleCallPath path);

   void processMove(U32 stateIndex);

   bool processArguments(S32 argc, const char **argv);
   void onAddedToGame(Game *);

   void render(S32 layerIndex);

   S32 getCurrentZone();
   void setCurrentZone(S32 zone);
   bool canSeePoint(Point point);         // Is point within robot's LOS?

   Vector<S32> flightPlan;                // List of zones to get from one point to another
   Point flightPlanTo;                    // Where our flightplan was calculated to

   // Some informational functions
   F32 getAngleXY(F32 x, F32 y);

   // External robot functions
   bool findNearestShip(Point &loc);      // Return location of nearest known ship within a given area

   Timer respawnTimer;
   bool mInGame;
   void resetLocation(Point location);    

   bool isRobot() { return true; }

private:
  int attribute;
  std::string message;

   TNL_DECLARE_CLASS(Robot);
};



class LuaClass
{

protected:
   static int luaPanicked(lua_State *L);
   static void clearStack(lua_State *L);

   static S32 returnPoint(lua_State *L, Point point);     // Returns a point... usage: return returnPoint(L, point);
   S32 returnInt(lua_State *L, S32 num);                  // Usage: return returnInt(L, int);
   S32 returnString(lua_State *L, const char *str);                
   static S32 returnNil(lua_State *L);                    // Returns nil... usage: return returnNil(L);
   static void setfield (lua_State *L, const char *key, F32 value);
};


extern enum ScoringEvent;

class LuaRobot : public LuaClass
{

private:
   Point getNextWaypoint();      // Helper function for getWaypoint()

public:
  // Constants

  // Initialize the pointer
  LuaRobot(lua_State *L);      // Constructor
  ~LuaRobot();                 // Destructor

   static const char className[];

   static Luna<LuaRobot>::RegType methods[];

   // Methods we will need to use
   S32 getZoneCenterXY(lua_State *L);
   S32 getGatewayToXY(lua_State *L);
   S32 getZoneCount(lua_State *L);
   S32 getCurrentZone(lua_State *L);

   S32 getAngle(lua_State *L);
   S32 getPosXY(lua_State *L);

   S32 setAngle(lua_State *L);
   S32 setAngleXY(lua_State *L);
   S32 getAngleXY(lua_State *L);
   S32 hasLosXY(lua_State *L);

   S32 hasFlag(lua_State *L);


   // Navigation
   S32 findObjects(lua_State *L);
   S32 getWaypoint(lua_State *L);


   // Ship control
   S32 setThrustAng(lua_State *L);
   S32 setThrustXY(lua_State *L);

   S32 fire(lua_State *L);
   S32 setWeapon(lua_State *L);
   S32 globalMsg(lua_State *L);
   S32 teamMsg(lua_State *L);
   S32 getAimAngle(lua_State *L);

   S32 logprint(lua_State *L);

   // Game info
   S32 getGameType(lua_State *L);
   S32 getFlagCount(lua_State *L);
   S32 getWinningScore(lua_State *L);
   S32 getGameTimeTotal(lua_State *L);
   S32 getGameTimeRemaining(lua_State *L);
   S32 getLeadingScore(lua_State *L);
   S32 getLeadingTeam(lua_State *L);
  
   S32 getLevelName(lua_State *L);
   S32 getGridSize(lua_State *L);
   S32 getIsTeamGame(lua_State *L);

   S32 getEventScore(lua_State *L);


private:
  Robot* thisRobot;              // Pointer to an actual C++ Robot object

};


// Class to restore Lua stack to the state it was in when we found it.
// Concept based on code from http://www.codeproject.com/KB/cpp/luaincpp.aspx
// ---------------------------------------------------------------------------
// VERSION              : 1.00
// DATE                 : 1-Sep-2005
// AUTHOR               : Richard Shephard
// ---------------------------------------------------------------------------

//class LuaProtectStack
//{
//public:
//   LuaProtectStack(LuaRobot *lgo)
//   {
//      mLuaRobot = lgo;
//      mTop = lua_gettop(mLuaRobot->mState);
//   }
//
//   virtual ~LuaProtectStack(void)
//   {
//      if(mLuaRobot->mState)
//         lua_settop (mLuaRobot->mState, mTop);
//   }
//
//protected:
//   LuaRobot *mLuaRobot;
//   S32 mTop;
//};



};

#endif
