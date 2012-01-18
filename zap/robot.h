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


#include "ship.h"          // Parent class
#include "luaObject.h"
#include "game.h"          // For ClientInfo def

namespace Zap
{

class Robot;
class LuaPlayerInfo;

class EventManager
{
public:
   // Need to keep synced with eventFunctions!
   enum EventType {
      TickEvent = 0,          // (time) --> Standard game tick event
      ShipSpawnedEvent,       // (ship) --> Ship (or robot) spawns
      ShipKilledEvent,        // (ship) --> Ship (or robot) is killed
      PlayerJoinedEvent,      // (playerInfo) --> Player joined game
      PlayerLeftEvent,        // (playerInfo) --> Player left game
      MsgReceivedEvent,       // (message, sender-player, public-bool) --> Chat message sent
      NexusOpenedEvent,       // () --> Nexus opened (nexus games only)
      NexusClosedEvent,       // () --> Nexus closed (nexus games only)
      EventTypes
   };

private:
   // Some helper functions
   bool isSubscribed(lua_State *L, EventType eventType);
   bool isPendingSubscribed(lua_State *L, EventType eventType);
   bool isPendingUnsubscribed(lua_State *L, EventType eventType);

   void removeFromSubscribedList(lua_State *L, EventType eventType);
   void removeFromPendingSubscribeList(lua_State *subscriber, EventType eventType);
   void removeFromPendingUnsubscribeList(lua_State *unsubscriber, EventType eventType);

   void handleEventFiringError(lua_State *L, EventType eventType, const char *errorMsg);

   bool mIsPaused;
   S32 mStepCount;           // If running for a certain number of steps, this will be > 0, while mIsPaused will be true
   static bool mConstructed;

public:
   EventManager();                  // C++ constructor
   EventManager(lua_State *L);      // Lua Constructor

   static EventManager *get();      // Provide access to the single EventManager instance
   bool suppressEvents();

   static Vector<lua_State *> subscriptions[EventTypes];
   static Vector<lua_State *> pendingSubscriptions[EventTypes];
   static Vector<lua_State *> pendingUnsubscriptions[EventTypes];
   static bool anyPending;

   void subscribe(lua_State *L, EventType eventType);
   void unsubscribe(lua_State *L, EventType eventType);
   void unsubscribeImmediate(lua_State *L, EventType eventType);     // Used when bot dies, and we know there won't be subscription conflicts
   void update();                                                    // Act on events sitting in the pending lists

   // We'll have several different signatures for this one...
   void fireEvent(EventType eventType);
   void fireEvent(EventType eventType, U32 deltaT);      // Tick
   void fireEvent(EventType eventType, Ship *ship);      // ShipSpawned, ShipKilled
   void fireEvent(lua_State *L, EventType eventType, const char *message, LuaPlayerInfo *player, bool global);     // MsgReceived
   void fireEvent(lua_State *L, EventType eventType, LuaPlayerInfo *player);  // PlayerJoined, PlayerLeft

   // Allow the pausing of event firing for debugging purposes
   void setPaused(bool isPaused);
   void togglePauseStatus();
   bool isPaused();
   void addSteps(S32 steps);     // Each robot will cause the step counter to decrement
};


////////////////////////////////////////
////////////////////////////////////////


class MoveItem;
class LuaRobot;
class ServerGame;
//class ClientInfo;

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
   U16 mCurrentZone;             // Zone robot is currently in

   S32 mScore;
   S32 mTotalScore;

   static const S32 RobotRespawnDelay = 1500;

   LuaPlayerInfo *mPlayerInfo;      // Player info object describing the robot

   bool mHasSpawned;

   void tickTimer(U32 deltaT);      // Move bot's timer forward

public:
   Robot();      // Constructor
   ~Robot();                                                                                    // Destructor

   bool initialize(Point &pos);

   void kill();

   lua_State *getL();

   void logError(const char *format, ...);   // In case of error...

   public:
   void render(S32 layerIndex);
   void idle(IdleCallPath path);
   void clearMove();

   bool processArguments(S32 argc, const char **argv, Game *game);
   void onAddedToGame(Game *);

   S32 getCurrentZone(ServerGame *game);
   void setCurrentZone(S32 zone);
   bool canSeePoint(Point point);         // Is point within robot's LOS?

   Vector<Point> flightPlan;              // List of points to get from one point to another
   U16 flightPlanTo;                      // Zone our flightplan was calculated to

   // Some informational functions
   F32 getAnglePt(Point point);

   // External robot functions
   bool findNearestShip(Point &loc);      // Return location of nearest known ship within a given area

   bool isRobot();
   //static S32 getRobotCount() { return robots.size(); }

   LuaRobot *mLuaRobot;                   // Could make private and make a public setter method...

   LuaPlayerInfo *getPlayerInfo();

   static Vector<Robot *> robots;         // Grand master list of all robots in the current game
   static void startBots();               // Loop through all our bots and run thier main() functions
   bool start();

   static Robot *findBot(lua_State *L);   // Find the bot that owns this L

   bool startLua();                       // Fire up bot's Lua processor
   void setPointerToThis();
   void registerClasses();
   string runGetName();                   // Run bot's getName() function

   S32 getScore();    // Return robot's score
   F32 getRating();   // Return robot's rating
   const char *getScriptName();

private:
   int attribute;
   string message;

   TNL_DECLARE_CLASS(Robot);
};


////////////////////////////////////////
////////////////////////////////////////


class LuaRobot : public LuaShip
{
   typedef LuaShip Parent;

private:
   Robot *thisRobot;                                 // Pointer to an actual C++ Robot object

   Point getNextWaypoint();                          // Helper function for getWaypoint()
   U16 findClosestZone(const Point &point);          // Finds zone closest to point, used when robots get off the map
   S32 findAndReturnClosestZone(lua_State *L, const Point &point); // Wraps findClosestZone and handles returning the result to Lua
   S32 doFindItems(lua_State *L, Rect *scope = NULL);    // Worker method for various find functions

   void setEnums(lua_State *L);                          // Set a whole slew of enum values that we want the scripts to have access to
   bool subscriptions[EventManager::EventTypes];         // Keep track of which events we're subscribed to for rapid unsubscription upon death
   void doSubscribe(lua_State *L, EventManager::EventType eventType);   // Do the work of subscribing, wrapped by subscribe()

public:
  // Constants

  // Initialize the pointer
   LuaRobot(lua_State *L);     // Lua constructor
   virtual ~LuaRobot();        // Destructor

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
   S32 setAnglePt(lua_State *L);
   S32 getAnglePt(lua_State *L);
   S32 hasLosPt(lua_State *L);

   // Navigation
   S32 getWaypoint(lua_State *L);

   S32 findItems(lua_State *L);
   S32 findGlobalItems(lua_State *L);

   // Ship control
   S32 setThrust(lua_State *L);
   S32 setThrustPt(lua_State *L);
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
   S32 setCurrLoadout(lua_State *L);        // Sets requested loadout to specified --> takes Loadout object

   S32 subscribe(lua_State *L);
   S32 unsubscribe(lua_State *L);


   //// Ship info
   //S32 getActiveWeapon(lua_State *L);

   S32 engineerDeployObject(lua_State *L);
   S32 dropItem(lua_State *L);
   S32 copyMoveFromObject(lua_State *L);

   S32 getGame(lua_State *L);  // Get a pointer to a game object, where we can run game-info oriented methods
   Ship *getObj();             // This handles delegation properly when we're dealing with methods inherited from LuaShip
};


////////////////////////////////////////
////////////////////////////////////////

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

