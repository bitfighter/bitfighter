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


#include "ship.h"             // Parent class
#include "LuaScriptRunner.h"
#include "game.h"             // For ClientInfo def

#include "EventManager.h"

namespace Zap
{

class MoveItem;
class ServerGame;

/**
 * This is the wrapper around the C++ object found in object.cc
 * Everything this object has done to it is passed on FROM Lua to the real C++
 * object through the pointer 'real_object'
 * Notice that I kept the function names the same for simplicity.
 */


class Robot : public Ship, public LuaScriptRunner
{
   typedef Ship Parent;

private:
   static const S32 RobotRespawnDelay = 1500;

   int attribute;
   string message;

   U16 mCurrentZone;                // Zone robot is currently in

   S32 mScore;
   S32 mTotalScore;

   LuaPlayerInfo *mPlayerInfo;      // Player info object describing the robot

   bool mHasSpawned;

   void tickTimer(U32 deltaT);      // Move bot's timer forward

   Point getNextWaypoint();                          // Helper function for getWaypoint()
   U16 findClosestZone(const Point &point);          // Finds zone closest to point, used when robots get off the map
   S32 doFindItems(lua_State *L, const char *methodName, Rect *scope = NULL); // Worker method for various find functions

public:
   Robot(lua_State *L = NULL);      // Combined Lua / C++ default constructor
   ~Robot();                        // Destructor

   bool initialize(Point &pos);

   void kill();

   void render(S32 layerIndex);
   void idle(IdleCallPath path);

   bool processArguments(S32 argc, const char **argv, Game *game);
   bool processArguments(S32 argc, const char **argv, Game *game, string &errorMessage);
   void onAddedToGame(Game *);

   S32 getCurrentZone();
   void setCurrentZone(S32 zone);
   bool canSeePoint(Point point, bool wallOnly = false);         // Is point within robot's LOS?

   Vector<Point> flightPlan;           // List of points to get from one point to another
   U16 flightPlanTo;                   // Zone our flightplan was calculated to

   // Some informational functions
   F32 getAnglePt(Point point);

   // External robot functions
   bool findNearestShip(Point &loc);   // Return location of nearest known ship within a given area

   bool isRobot();

   const char *getErrorMessagePrefix();

   LuaPlayerInfo *getPlayerInfo();
   bool start();

   bool prepareEnvironment();
   void registerClasses();
   string runGetName();                // Run bot's getName() function

   void clearMove();                   // Reset bot's move to do nothing


   S32 getScore();    // Return robot's score
   F32 getRating();   // Return robot's rating
   const char *getScriptName();

   Robot *clone() const;

   //S32 getGame(lua_State *L);  // Get a pointer to a game object, where we can run game-info oriented methods
   TNL_DECLARE_CLASS(Robot);

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(Robot);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 subscribe(lua_State *L);
   S32 unsubscribe(lua_State *L);

   S32 getCPUTime(lua_State *L);
   S32 getTime(lua_State *L);

   S32 setAngle(lua_State *L);
   S32 getAnglePt(lua_State *L);
   S32 hasLosPt(lua_State *L);

   // Navigation
   S32 getWaypoint(lua_State *L);

   // Finding stuff
   S32 findItems(lua_State *L);
   S32 findGlobalItems(lua_State *L);
   S32 findObjectById(lua_State *L);

   // Bad dudes
   S32 findClosestEnemy(lua_State *L);

   // Ship control
   S32 setThrust(lua_State *L);
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
   S32 setCurrLoadout(lua_State *L);       // Sets requested loadout to specified --> takes Loadout object

   S32 engineerDeployObject(lua_State *L);
   S32 dropItem(lua_State *L);
   S32 copyMoveFromObject(lua_State *L);
};



};

#endif

