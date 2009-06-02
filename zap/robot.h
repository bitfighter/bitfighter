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

#ifndef _ROBOT_H_
#define _ROBOT_H_


#include "gameObject.h"
#include "moveObject.h"
#include "ship.h"

#include "sparkManager.h"
#include "sfx.h"
#include "timer.h"
#include "shipItems.h"
#include "gameWeapons.h"

#include "luaObject.h"


namespace Zap
{

class Item;
class LuaRobot;

/**
 * This is the wrapper around the C++ object found in object.cc
 * Everything this object has done to it is passed on FROM Lua to the real C++
 * object through the pointer 'real_object'
 * Notice that I kept the function names the same for simplicity.
 */

class Robot : public Ship
{
   typedef Ship Parent;

private:
   // Scripting stuff
   lua_State *L;                // Main Lua state variable

   void logError(const char *format, ...);   // In case of error...
   string mFilename;                         // Name of file script was loaded from

   S32 mCurrentZone;            // Zone robot is currently in
   U32 mLastMoveTime;           // Keep track of how long it's been since robot's last move was processed

   static U32 mRobotCount;

   enum {
      RobotRespawnDelay = 1500,
   };

   //void push(lua_State *L) {  Lunar<LuaRobot>::push(L, mLuaRobot); }

public:
   Robot(StringTableEntry robotName="", S32 team = -1, Point p = Point(0,0), F32 m = 1.0);      // Constructor
   ~Robot();          // Destructor

   bool initialize(Point p);
   void kill(DamageInfo *theInfo);
   void kill();

   void idle(IdleCallPath path);

   void processMove(U32 stateIndex);

   Point mTarget;     // TODO: Get rid of this!!

   bool processArguments(S32 argc, const char **argv);
   void onAddedToGame(Game *);

   void render(S32 layerIndex);

   S32 getCurrentZone();
   void setCurrentZone(S32 zone);
   bool canSeePoint(Point point);         // Is point within robot's LOS?

   Vector<Point> flightPlan;              // List of points to get from one point to another
   S32 flightPlanTo;                      // Zone our flightplan was calculated to

   // Some informational functions
   F32 getAngleXY(Point point);

   // External robot functions
   bool findNearestShip(Point &loc);      // Return location of nearest known ship within a given area

   Timer respawnTimer;

   bool isRobot() { return true; }
   static U32 getRobotCount() { return mRobotCount; }

   LuaRobot *mLuaRobot;    // Could make private and make a public setter method...


private:
  int attribute;
  std::string message;

   TNL_DECLARE_CLASS(Robot);
};


class LuaRobot : public LuaShip
{
private:
   Point getNextWaypoint();                          // Helper function for getWaypoint()
   S32 findClosestZone(Point point);                 // Finds zone closest to point, used when robots get off the map
   S32 findAndReturnClosestZone(lua_State *L, Point point); // Wraps findClosestZone and handles returning the result to Lua
   S32 doFindItems(lua_State *L, Rect scope);        // Worker method for various find functions

   Robot *thisRobot;              // Pointer to an actual C++ Robot object

public:
  // Constants

  // Initialize the pointer
  LuaRobot(lua_State *L);      // Constructor
  ~LuaRobot();                 // Destructor

   static const char className[];

   static Lunar<LuaRobot>::RegType methods[];

   S32 getClassID(lua_State *L);

   S32 getCPUTime(lua_State *L);
   S32 getTime(lua_State *L);

   S32 getZoneCenter(lua_State *L);
   S32 getGatewayFromZoneToZone(lua_State *L);
   S32 getZoneCount(lua_State *L);
   S32 getCurrentZone(lua_State *L);

   //S32 getAngle(lua_State *L);
   //S32 getLoc(lua_State *L);


   S32 setAngle(lua_State *L);
   S32 setAngleXY(lua_State *L);
   S32 getAngleXY(lua_State *L);
   S32 hasLosXY(lua_State *L);

   S32 hasFlag(lua_State *L);


   // Navigation
   S32 getWaypoint(lua_State *L);

   S32 findItems(lua_State *L);
   S32 findGlobalItems(lua_State *L);

   // Ship control
   S32 setThrust(lua_State *L);
   S32 setThrustXY(lua_State *L);
   S32 setThrustToPt(lua_State *L);

   S32 getFiringSolution(lua_State *L);
   S32 getInterceptCourse(lua_State *L);

   S32 fire(lua_State *L);
   S32 setWeapon(lua_State *L);
   S32 setWeaponIndex(lua_State *L);
   S32 hasWeapon(lua_State *L);

   S32 globalMsg(lua_State *L);
   S32 teamMsg(lua_State *L);

   S32 activateModule(lua_State *L);       // Activate module this cycle --> takes module index
   S32 activateModuleIndex(lua_State *L);  // Activate module this cycle --> takes module enum

   S32 setReqLoadout(lua_State *L);        // Sets requested loadout to specified --> takes Loadout object
   S32 getCurrLoadout(lua_State *L);       // Returns current loadout (Loadout)
   S32 getReqLoadout(lua_State *L);        // Returns requested loadout (Loadout)


   //// Ship info
   //S32 getActiveWeapon(lua_State *L);

   S32 logprint(lua_State *L);

   S32 getGame(lua_State *L);             // Get a pointer to a game object, where we can run game-info oriented methods
   Ship *getObj() { return thisRobot; }   // This handles delegation properly when we're dealing with methods inherited from LuaShip
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
