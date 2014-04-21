//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _ROBOT_H_
#define _ROBOT_H_


#include "ship.h"             // Parent class

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
   U16 mCurrentZone;                // Zone robot is currently in

   LuaPlayerInfo *mPlayerInfo;      // Player info object describing the robot

   bool mHasSpawned;

   Point getNextWaypoint();                          // Helper function for getWaypoint()
   U16 findClosestZone(const Point &point);          // Finds zone closest to point, used when robots get off the map

protected:
   void killScript();

public:
   explicit Robot(lua_State *L = NULL);   // Combined Lua / C++ default constructor
   virtual ~Robot();                      // Destructor

   bool initialize(Point &pos);

   void kill();

   void renderLayer(S32 layerIndex);
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

   bool isRobot();

   const char *getErrorMessagePrefix();

   LuaPlayerInfo *getPlayerInfo();
   bool start();

   bool prepareEnvironment();
   string runGetName();                // Run bot's getName() function

   void clearMove();                   // Reset bot's move to do nothing


   const char *getScriptName();

   Robot *clone() const;

   //S32 getGame(lua_State *L);  // Get a pointer to a game object, where we can run game-info oriented methods
   TNL_DECLARE_CLASS(Robot);

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(Robot);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_setAngle(lua_State *L);
   S32 lua_getAnglePt(lua_State *L);
   S32 lua_canSeePoint(lua_State *L);

   // Navigation
   S32 lua_getWaypoint(lua_State *L);

   // Finding stuff
   S32 lua_findVisibleObjects(lua_State *L);

   // Bad dudes
   S32 lua_findClosestEnemy(lua_State *L);

   // Ship control
   S32 lua_setThrust(lua_State *L);
   S32 lua_setThrustToPt(lua_State *L);

   S32 lua_getFiringSolution(lua_State *L);
   S32 lua_getInterceptCourse(lua_State *L);

   S32 lua_fireWeapon(lua_State *L);
   S32 lua_hasWeapon(lua_State *L);  

   S32 lua_fireModule(lua_State *L);
   S32 lua_hasModule(lua_State *L);

   S32 lua_setLoadoutWeapon(lua_State *L);
   S32 lua_setLoadoutModule(lua_State *L);

   S32 lua_globalMsg(lua_State *L);
   S32 lua_teamMsg(lua_State *L);
   S32 lua_privateMsg(lua_State *L);

   S32 lua_engineerDeployObject(lua_State *L);
   S32 lua_dropItem(lua_State *L);
   S32 lua_copyMoveFromObject(lua_State *L);

   S32 lua_removeFromGame(lua_State *L);
};



};

#endif

