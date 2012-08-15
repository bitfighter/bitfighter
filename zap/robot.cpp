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

#include "robot.h"
#include "EventManager.h"
#include "playerInfo.h"          // For RobotPlayerInfo constructor
#include "goalZone.h"
#include "loadoutZone.h"
#include "soccerGame.h"          // For lua object defs
#include "NexusGame.h"           // For lua object defs
#include "EngineeredItem.h"      // For lua object defs
#include "PickupItem.h"          // For lua object defs
#include "teleporter.h"          // For lua object defs
#include "CoreGame.h"            // For lua object defs
#include "GameTypesEnum.h"
#include "ClientInfo.h"
#include "ServerGame.h"

//#include "../lua/luaprofiler-2.0.2/src/luaprofiler.h"      // For... the profiler!
#include "BotNavMeshZone.h"      // For BotNavMeshZone class definition
#include "luaUtil.h"

#ifndef ZAP_DEDICATED
#  include "OpenglUtils.h"
#endif

#include <math.h>


#define hypot _hypot    // Kill some warnings

namespace Zap
{

const bool QUIT_ON_SCRIPT_ERROR = true;


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Robot);

// Constructor, runs on client and server
Robot::Robot() : Ship(NULL, TEAM_NEUTRAL, Point(), 1, true),   
                 LuaScriptRunner() 
{
   mHasSpawned = false;
   mObjectTypeNumber = RobotShipTypeNumber;

   mCurrentZone = U16_MAX;
   flightPlanTo = U16_MAX;

   mPlayerInfo = new RobotPlayerInfo(this);
   mScore = 0;
   mTotalScore = 0;

   mErrorMsgPrefix = "***ROBOT ERROR***";

   for(S32 i = 0; i < ModuleCount; i++)         // Here so valgrind won't complain if robot updates before initialize is run
   {
      mModulePrimaryActive[i] = false;
      mModuleSecondaryActive[i] = false;
   }
#ifndef ZAP_DEDICATED
   mShapeType = ShipShape::Normal;
#endif

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor, runs on client and server
Robot::~Robot()
{
   setOwner(NULL);

   if(isGhost())
   {
      delete mPlayerInfo;     // On the server, mPlayerInfo will be deleted below, after event is fired
      return;
   }

   // Server only from here on down
   if(getGame())  // can be NULL if this robot was never added to game (bad / missing robot file)
   {
      EventManager::get()->fireEvent(getScriptId(), EventManager::PlayerLeftEvent, getPlayerInfo());

      if(getGame()->getGameType())
         getGame()->getGameType()->serverRemoveClient(mClientInfo);

      getGame()->removeBot(this);
      logprintf(LogConsumer::LogLuaObjectLifecycle, "Robot %s terminated (%d bots left)", mScriptName.c_str(), getGame()->getRobotCount());

   }

   mPlayerInfo->setDefunct();

   delete mPlayerInfo;
   if(mClientInfo.isValid())
      delete mClientInfo.getPointer();

   LUAW_DESTRUCTOR_CLEANUP;
}


// Reset everything on the robot back to the factory settings -- runs only when bot is spawning in GameType::spawnRobot()
// Only runs on server!
bool Robot::initialize(Point &pos)
{
   try
   {
      flightPlan.clear();

      mCurrentZone = U16_MAX;   // Correct value will be calculated upon first request

      Parent::initialize(pos);

      enableCollision();

      // WarpPositionMask triggers the spinny spawning visual effect
      setMaskBits(RespawnMask | HealthMask        | LoadoutMask         | PositionMask | 
                  MoveMask    | ModulePrimaryMask | ModuleSecondaryMask | WarpPositionMask);      // Send lots to the client

      TNLAssert(!isGhost(), "Didn't expect ghost here... this is supposed to only run on the server!");

      EventManager::get()->update();   // Ensure registrations made during bot initialization are ready to go
   }
   catch(LuaException &e)
   {
      logError("Robot error during spawn: %s.  Shutting robot down.", e.what());
      return false;
   }

   return true;
} 


// Server only
bool Robot::start()
{
   if(!startLua())
      return false;

   if(mClientInfo->getName() == "")                          // Make sure bots have a name
      mClientInfo->setName(getGame()->makeUnique("Robot").c_str());

   mHasSpawned = true;

   mGame->addToClientList(mClientInfo);

   return true;
}


bool Robot::prepareEnvironment()
{
   try
   {
      LuaScriptRunner::prepareEnvironment();

      // Push a pointer to this Robot to the Lua stack, then set the name of this pointer in the protected environment.  
      // This is the name that we'll use to refer to this robot from our Lua code.  

      if(!loadAndRunGlobalFunction(L, LUA_HELPER_FUNCTIONS_KEY) || !loadAndRunGlobalFunction(L, ROBOT_HELPER_FUNCTIONS_KEY))
         return false;

      // The following code pushes the robot onto the lua stack, and binds it to the variable "bot" which can 
      // be used to call functions using bot:f() syntax
      lua_getfield(L, LUA_REGISTRYINDEX, getScriptId());    // Put script's env table onto the stack  -- env_table
      lua_pushliteral(L, "bot");                            //                                        -- env_table, "bot"
      luaW_push<Robot>(L, this);                            //                                        -- env_table, "bot", *this

      lua_rawset(L, -3);                                    // env_table["bot"] = *this               -- env_table
      lua_pop(L, 1);                                        //                                        -- <<empty stack>>

      TNLAssert(lua_gettop(L) <= 0 || LuaObject::dumpStack(L), "Stack not cleared!");
   }
   catch(LuaException &e)
   {
      logError(e.what());
      return false;
   }

   return true;
}


// Loads script, runs getName, stores result in bot's clientInfo
bool Robot::startLua()
{
   if(!LuaScriptRunner::startLua(ROBOT) || !loadScript() || !runMain())
      return false;

   // Pass true so that if this bot doesn't have a TickEvent handler, we don't print a message
   EventManager::get()->subscribe(getScriptId(), EventManager::TickEvent, true);

   mSubscriptions[EventManager::TickEvent] = true;

   string name = runGetName();                                             // Run bot's getName function
   getClientInfo()->setName(getGame()->makeUnique(name.c_str()).c_str());  // Make sure name is unique

   return true;
}


static string getNextName()
{
   static const string botNames[] = {
      "Addison", "Alexis", "Amelia", "Audrey", "Chloe", "Claire", "Elizabeth", "Ella", 
      "Emily", "Emma", "Evelyn" "Gabriella", "Hailey", "Hannah", "Isabella", "Layla", 
      "Lillian", "Lucy", "Madison", "Natalie", "Olivia", "Riley", "Samantha", "Zoe" 
   };

   static S32 nameIndex = -1;
   nameIndex++;

   return botNames[nameIndex % ARRAYSIZE(botNames)];
}


// Run bot's getName function, return default name if fn isn't defined
string Robot::runGetName()
{
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");

   // Retrieve the bot's getName function, and put it on top of the stack
   bool ok = retrieveFunction("getName");     

   TNLAssert(ok, "getName function not found -- is robot_helper_functions corrupt?");

   if(!ok)
   {
      string name = getNextName();
      logError("Your scripting environment appears corrupted.  Consider reinstalling Bitfighter.");
      logError("Could not find getName function -- using default name \"%s\".", name.c_str());
      return name;
   }

   S32 error = lua_pcall(L, 0, 1, 0);    // Passing 0 params, expecting 1 back

   if(error == 0)    // Function returned normally     
   {
      if(!lua_isstring(L, -1))
      {
         string name = getNextName();
         logError("Robot error retrieving name (returned value was not a string).  Using \"%s\".", name.c_str());

         lua_pop(L, 1);          // Remove thing that wasn't a name from the stack
         TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");
         return name;
      }

      string name = lua_tostring(L, -1);

      lua_pop(L, 1);
      TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");
      return name;
   }

   // Got an error running getName()
   string name = getNextName();
   logError("Error running getName function (%s) -- using default name \"%s\".", lua_tostring(L, -1), name.c_str());

   lua_pop(L, 1);             // Remove error message from stack
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");
   return name;
}


// Advance timers by deltaT
void Robot::tickTimer(U32 deltaT)
{
   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack dirty!");

   bool ok = retrieveFunction("_tickTimer");       // Push timer function onto stack            -- function 
   TNLAssert(ok, "_tickTimer function not found -- is lua_helper_functions corrupt?");

   if(!ok)
   {      
      logError("Your scripting environment appears corrupted.  Consider reinstalling Bitfighter.");
      logError("Function _tickTimer() could not be found!  Terminating script.");

      deleteObject();      // Will probably fail for levelgens...

      TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");

      return;
   }

   luaW_push<Robot>(L, this);
   lua_pushnumber(L, deltaT);                   // Pass the time elapsed since we were last here   -- function, object, time
   S32 error = lua_pcall(L, 2, 0, 0);           // Pass two objects, expect none in return         -- <<empty stack>>

   if(error != 0)
   {
      logError("Robot error running _tickTimer(): %s.  Shutting robot down.", lua_tostring(L, -1));
      lua_pop(L, 1);    // Remove error message from stack

      deleteObject();   // Add bot to delete list, where it will be deleted in the proper manner
   }

   TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");
}



// Register our connector types with Lua
void Robot::registerClasses()
{
   // General classes 
   LuaScriptRunner::registerClasses();    // LuaScriptRunner is a parent class
}


// This only runs the very first time the robot is added to the level
// Note that level may not yet be ready, so the bot can't spawn yet
// Runs on client and server 
void Robot::onAddedToGame(Game *game)
{
   if(!isGhost())    // Robots are created with NULL ClientInfos.  We'll add a valid one here.
   {
      TNLAssert(mClientInfo.isNull(), "mClientInfo should be NULL");

      mClientInfo = new FullClientInfo(game, NULL, true);  // deleted in destructor
      mClientInfo->setShip(this);
      this->setOwner(mClientInfo);
   }

   Parent::onAddedToGame(game);
   
   if(isGhost())
      return;

   // Server only from here on out

   hasExploded = true;        // Becase we start off "dead", but will respawn real soon now...
   disableCollision();

   game->addBot(this);        // Add this robot to the list of all robots (can't do this in constructor or else it gets run on client side too...)
  
   EventManager::get()->fireEvent(getScriptId(), EventManager::PlayerJoinedEvent, getPlayerInfo());
}


void Robot::kill()
{
   if(hasExploded) 
      return;

   hasExploded = true;

   setMaskBits(ExplosionMask);
   if(!isGhost() && getOwner())
      getLoadout(getOwner()->mOldLoadout);

   disableCollision();

   // Dump mounted items
   for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
      if(mMountedItems[i].isValid())  // server quitting can make an object invalid.
         mMountedItems[i]->onMountDestroyed();
}


// Need this, as this may come from level or levelgen
bool Robot::processArguments(S32 argc, const char **argv, Game *game)
{
   string unused_String;
   return processArguments(argc, argv, game, unused_String); 
}


bool Robot::processArguments(S32 argc, const char **argv, Game *game, string &errorMessage)
{
   if(argc >= 1)
      setTeam(atoi(argv[0]));
   else
      setTeam(NO_TEAM);   
   

   string scriptName;

   if(argc >= 2)
      scriptName = argv[1];
   else
      scriptName = game->getSettings()->getIniSettings()->defaultRobotScript;

   FolderManager *folderManager = game->getSettings()->getFolderManager();

   if(scriptName != "")
      mScriptName = folderManager->findBotFile(scriptName);

   if(mScriptName == "")     // Bot script could not be located
   {
      errorMessage = "Could not find bot file " + scriptName;
      logprintf(LogConsumer::LuaBotMessage, "Could not find bot file %s", scriptName.c_str());
      return false;
   }

   setScriptingDir(folderManager->luaDir);      // Where our helper scripts are stored

   // Collect our arguments to be passed into the args table in the robot (starting with the robot name)
   // Need to make a copy or containerize argv[i] somehow, because otherwise new data will get written
   // to the string location subsequently, and our vals will change from under us.  That's bad!
   for(S32 i = 2; i < argc; i++)        // Does nothing if we have no args
      mScriptArgs.push_back(string(argv[i]));

   return true;
}


// Returns zone ID of current zone
S32 Robot::getCurrentZone()
{
   TNLAssert(dynamic_cast<ServerGame *>(getGame()), "Not a ServerGame");

   // We're in uncharted territory -- try to get the current zone
   mCurrentZone = BotNavMeshZone::findZoneContaining(BotNavMeshZone::getBotZoneDatabase(), getActualPos());

   return mCurrentZone;
}


// Setter method, not a robot function!
void Robot::setCurrentZone(S32 zone)
{
   mCurrentZone = zone;
}


F32 Robot::getAnglePt(Point point)
{
   return getActualPos().angleTo(point);
}


// Return coords of nearest ship... and experimental robot routine
bool Robot::findNearestShip(Point &loc)
{
   Vector<DatabaseObject *> foundObjects;
   Point closest;

   Point pos = getActualPos();
   Point extend(2000, 2000);
   Rect r(pos - extend, pos + extend);

   findObjects((TestFunc)isShipType, foundObjects, r);

   if(!foundObjects.size())
      return false;

   F32 dist = F32_MAX;
   bool found = false;

   for(S32 i = 0; i < foundObjects.size(); i++)
   {
      Ship *ship = static_cast<Ship *>(foundObjects[i]);
      F32 d = ship->getPos().distanceTo(pos);
      if(d < dist && d > 0)      // d == 0 means we're comparing to ourselves
      {
         dist = d;
         loc = ship->getPos();
         found = true;
      }
   }
   return found;
}


bool Robot::canSeePoint(Point point, bool wallOnly)
{
   Point difference = point - getActualPos();

   Point crossVector(difference.y, -difference.x);  // Create a point whose vector from 0,0 is perpenticular to the original vector
   crossVector.normalize(mRadius);                  // reduce point so the vector has length of ship radius

   // Edge points of ship
   Point shipEdge1 = getActualPos() + crossVector;
   Point shipEdge2 = getActualPos() - crossVector;

   // Edge points of point
   Point pointEdge1 = point + crossVector;
   Point pointEdge2 = point - crossVector;

   Vector<Point> thisPoints;
   thisPoints.push_back(shipEdge1);
   thisPoints.push_back(shipEdge2);
   thisPoints.push_back(pointEdge2);
   thisPoints.push_back(pointEdge1);

   Vector<Point> otherPoints;
   Rect queryRect(thisPoints);

   fillVector.clear();
   findObjects(wallOnly ? (TestFunc)isWallType : (TestFunc)isCollideableType, fillVector, queryRect);

   for(S32 i = 0; i < fillVector.size(); i++)
      if(fillVector[i]->getCollisionPoly(otherPoints) && polygonsIntersect(thisPoints, otherPoints))
         return false;

   return true;
}


void Robot::render(S32 layerIndex)
{
#ifndef ZAP_DEDICATED
   if(isGhost())                                         // Client rendering client's objects
      Parent::render(layerIndex);

   else if(layerIndex == 1 && flightPlan.size() != 0)    // Client hosting is rendering server objects
   {
      // Render from ship to start of flight plan
      Vector<Point> line(2);
      line.push_back(getActualPos());
      line.push_back(flightPlan[0]);
      renderLine(&line);

      // Render the flight plan
      renderPointVector(&flightPlan, GL_LINE_STRIP);
   }
#endif
}


void Robot::idle(BfObject::IdleCallPath path)
{
   TNLAssert(path != BfObject::ServerIdleControlFromClient, "Should never idle with ServerIdleControlFromClient");

   if(hasExploded)
      return;

   if(path != BfObject::ServerIdleMainLoop)   
      Parent::idle(path);                       
   else                         
   {
      U32 deltaT = mCurrentMove.time;

      TNLAssert(deltaT != 0, "Time should never be zero!");    

      tickTimer(deltaT);

      Parent::idle(BfObject::ServerIdleControlFromClient);   // Let's say the script is the client  ==> really not sure this is right
   }
}


// Clear out current move so that if none of the event handlers set the various move components, the bot will do nothing
void Robot::clearMove()
{
   mCurrentMove.fire = false;
   mCurrentMove.x = 0;
   mCurrentMove.y = 0;

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      mCurrentMove.modulePrimary[i] = false;
      mCurrentMove.moduleSecondary[i] = false;
   }
}


bool Robot::isRobot()
{
   return true;
}


LuaPlayerInfo *Robot::getPlayerInfo()
{
   return mPlayerInfo;
}


S32 Robot::getScore()
{
   return mScore;
}


F32 Robot::getRating()
{
   return mTotalScore == 0 ? 0.5f : (F32)mScore / (F32)mTotalScore;
}


const char *Robot::getScriptName()
{
   return mScriptName.c_str();
}


Robot *Robot::clone() const
{
   return new Robot(*this);
}


// Another helper function: returns id of closest zone to a given point
U16 Robot::findClosestZone(const Point &point)
{
   U16 closestZone = U16_MAX;

   // First, do a quick search for zone based on the buffer; should be 99% of the cases

   // Search radius is just slightly larger than twice the zone buffers added to objects like barriers
   S32 searchRadius = 2 * BotNavMeshZone::BufferRadius + 1;

   Vector<DatabaseObject*> objects;
   Rect rect = Rect(point.x + searchRadius, point.y + searchRadius, point.x - searchRadius, point.y - searchRadius);

   BotNavMeshZone::getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, objects, rect);

   for(S32 i = 0; i < objects.size(); i++)
   {
      BotNavMeshZone *zone = static_cast<BotNavMeshZone *>(objects[i]);
      Point center = zone->getCenter();

      if(getGame()->getGameObjDatabase()->pointCanSeePoint(center, point))  // This is an expensive test
      {
         closestZone = zone->getZoneId();
         break;
      }
   }

   // Target must be outside extents of the map, find nearest zone if a straight line was drawn
   if(closestZone == U16_MAX)
   {
      Point extentsCenter = getGame()->getWorldExtents().getCenter();

      F32 collisionTimeIgnore;
      Point surfaceNormalIgnore;

      DatabaseObject* object = BotNavMeshZone::getBotZoneDatabase()->findObjectLOS(BotNavMeshZoneTypeNumber,
            ActualState, point, extentsCenter, collisionTimeIgnore, surfaceNormalIgnore);

      BotNavMeshZone *zone = static_cast<BotNavMeshZone *>(object);

      if (zone != NULL)
         closestZone = zone->getZoneId();
   }

   return closestZone;
}

//// Lua methods

//                Fn name               Param profiles                  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS,  getCPUTime,           ARRAYDEF({{ END }}), 1 )                             \
   METHOD(CLASS,  getTime,              ARRAYDEF({{ END }}), 1 )                             \
                                                                                             \
   METHOD(CLASS,  setAngle,             ARRAYDEF({{ PT, END }, { NUM, END }}), 2 )           \
   METHOD(CLASS,  getAnglePt,           ARRAYDEF({{ PT, END }              }), 1 )           \
   METHOD(CLASS,  hasLosPt,             ARRAYDEF({{ PT, END }              }), 1 )           \
                                                                                             \
   METHOD(CLASS,  getWaypoint,          ARRAYDEF({{ PT, END }}), 1 )                         \
                                                                                             \
   METHOD(CLASS,  setThrust,            ARRAYDEF({{ NUM, NUM, END }, { NUM, PT, END}}), 2 )  \
   METHOD(CLASS,  setThrustToPt,        ARRAYDEF({{ PT,       END }                 }), 1 )  \
                                                                                             \
   METHOD(CLASS,  fire,                 ARRAYDEF({{            END }}), 1 )                  \
   METHOD(CLASS,  setWeapon,            ARRAYDEF({{ WEAP_ENUM, END }}), 1 )                  \
   METHOD(CLASS,  setWeaponIndex,       ARRAYDEF({{ WEAP_SLOT, END }}), 1 )                  \
   METHOD(CLASS,  hasWeapon,            ARRAYDEF({{ WEAP_ENUM, END }}), 1 )                  \
                                                                                             \
   METHOD(CLASS,  activateModule,       ARRAYDEF({{ MOD_ENUM, END }}), 1 )                   \
   METHOD(CLASS,  activateModuleIndex,  ARRAYDEF({{ MOD_SLOT, END }}), 1 )                   \
   METHOD(CLASS,  setReqLoadout,        ARRAYDEF({{ LOADOUT,  END }}), 1 )                   \
   METHOD(CLASS,  setCurrLoadout,       ARRAYDEF({{ LOADOUT,  END }}), 1 )                   \
                                                                                             \
   METHOD(CLASS,  globalMsg,            ARRAYDEF({{ STR, END }}), 1 )                        \
   METHOD(CLASS,  teamMsg,              ARRAYDEF({{ STR, END }}), 1 )                        \
                                                                                             \
   METHOD(CLASS,  findItems,            ARRAYDEF({{ TABLE, INTS, END }, { INTS, END }}), 2 ) \
   METHOD(CLASS,  findGlobalItems,      ARRAYDEF({{ TABLE, INTS, END }, { INTS, END }}), 2 ) \
                                                                                             \
   METHOD(CLASS,  getFiringSolution,    ARRAYDEF({{ ITEM, END }}), 1 )                       \
   METHOD(CLASS,  getInterceptCourse,   ARRAYDEF({{ ITEM, END }}), 1 )                       \
                                                                                             \
   METHOD(CLASS,  engineerDeployObject, ARRAYDEF({{ INT,  END }}), 1 )                       \
   METHOD(CLASS,  dropItem,             ARRAYDEF({{       END }}), 1 )                       \
   METHOD(CLASS,  copyMoveFromObject,   ARRAYDEF({{ ITEM, END }}), 1 )                       \

GENERATE_LUA_METHODS_TABLE(Robot, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(Robot, LUA_METHODS);

#undef LUA_METHODS


const char *Robot::luaClassName = "Robot";
REGISTER_LUA_SUBCLASS(Robot, Ship);


// Return CPU time... use for timing things
S32 Robot::getCPUTime(lua_State *L)
{
   return returnInt(L, getGame()->getCurrentTime());
}


S32 Robot::getTime(lua_State *L)
{
   return returnInt(L, getCurrentMove().time);
}


// Turn to angle a (in radians, or toward a point)
S32 Robot::setAngle(lua_State *L)
{
   S32 profile = checkArgList(L, functionArgs, "Robot", "setAngle");

   Move move = getCurrentMove();

   if(profile == 0)        // Args: PT    ==> Aim towards point
   {
      Point point = getPointOrXY(L, 1);
      move.angle = getAnglePt(point);
   }

   else if(profile == 1)   // Args: NUM   ==> Aim at this angle (radians)
      move.angle = getFloat(L, 1);

   setCurrentMove(move);
   return 0;
}


// Get angle toward point
S32 Robot::getAnglePt(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "getAnglePt");

   Point point = getPointOrXY(L, 1);

   return returnFloat(L, getAnglePt(point));
}


// Can robot see point P?
S32 Robot::hasLosPt(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "hasLosPt");

   Point point = getPointOrXY(L, 1);

   return returnBool(L, canSeePoint(point));
}


// Get next waypoint to head toward when traveling from current location to x,y
// Note that this function will be called frequently by various robots, so any
// optimizations will be helpful.
S32 Robot::getWaypoint(lua_State *L)  
{
   TNLAssert(dynamic_cast<ServerGame *>(getGame()), "Not a ServerGame");

   checkArgList(L, functionArgs, "Robot", "getWaypoint");

   ServerGame *serverGame = (ServerGame *) getGame();

   Point target = getPointOrXY(L, 1);

   // If we can see the target, go there directly
   if(canSeePoint(target, true))
   {
      flightPlan.clear();
      return returnPoint(L, target);
   }

   // TODO: cache destination point; if it hasn't moved, then skip ahead.

   U16 targetZone = BotNavMeshZone::findZoneContaining(BotNavMeshZone::getBotZoneDatabase(), target); // Where we're going  ===> returns zone id

   if(targetZone == U16_MAX)       // Our target is off the map.  See if it's visible from any of our zones, and, if so, go there
   {
      targetZone = findClosestZone(target);

      if(targetZone == U16_MAX)
         return returnNil(L);
   }

   // Make sure target is still in the same zone it was in when we created our flightplan.
   // If we're not, our flightplan is invalid, and we need to skip forward and build a fresh one.
   if(flightPlan.size() > 0 && targetZone == flightPlanTo)
   {
      // In case our target has moved, replace final point of our flightplan with the current target location
      flightPlan[0] = target;

      // First, let's scan through our pre-calculated waypoints and see if we can see any of them.
      // If so, we'll just head there with no further rigamarole.  Remember that our flightplan is
      // arranged so the closest points are at the end of the list, and the target is at index 0.
      Point dest;
      bool found = false;
//      bool first = true;

      while(flightPlan.size() > 0)
      {
         Point last = flightPlan.last();

         // We'll assume that if we could see the point on the previous turn, we can
         // still see it, even though in some cases, the turning of the ship around a
         // protruding corner may make it technically not visible.  This will prevent
         // rapidfire recalcuation of the path when it's not really necessary.

         // removed if(first) ... Problems with Robot get stuck after pushed from burst or mines.
         // To save calculations, might want to avoid (canSeePoint(last))
         if(canSeePoint(last, true))
         {
            dest = last;
            found = true;
//            first = false;
            flightPlan.pop_back();   // Discard now possibly superfluous waypoint
         }
         else
            break;
      }

      // If we found one, that means we found a visible waypoint, and we can head there...
      if(found)
      {
         flightPlan.push_back(dest);    // Put dest back at the end of the flightplan
         return returnPoint(L, dest);
      }
   }

   // We need to calculate a new flightplan
   flightPlan.clear();

   U16 currentZone = getCurrentZone();     // Zone we're in

   if(currentZone == U16_MAX)      // We don't really know where we are... bad news!  Let's find closest visible zone and go that way.
      currentZone = findClosestZone(getActualPos());

   if(currentZone == U16_MAX)      // That didn't go so well...
      return returnNil(L);

   // We're in, or on the cusp of, the zone containing our target.  We're close!!
   if(currentZone == targetZone)
   {
      Point p;
      flightPlan.push_back(target);

      if(!canSeePoint(target, true))           // Possible, if we're just on a boundary, and a protrusion's blocking a ship edge
      {
         BotNavMeshZone *zone = static_cast<BotNavMeshZone *>(BotNavMeshZone::getBotZoneDatabase()->getObjectByIndex(targetZone));

         p = zone->getCenter();
         flightPlan.push_back(p);
      }
      else
         p = target;

      return returnPoint(L, p);
   }

   // If we're still here, then we need to find a new path.  Either our original path was invalid for some reason,
   // or the path we had no longer applied to our current location
   flightPlanTo = targetZone;

   // check cache for path first
   pair<S32,S32> pathIndex = pair<S32,S32>(currentZone, targetZone);

   const Vector<BotNavMeshZone *> *zones = BotNavMeshZone::getBotZones();      // Grab our pre-cached list of nav zones

   if(serverGame->getGameType()->cachedBotFlightPlans.find(pathIndex) == serverGame->getGameType()->cachedBotFlightPlans.end())
   {
      // Not found so calculate flight plan
      flightPlan = AStar::findPath(zones, currentZone, targetZone, target);

      // Add to cache
      serverGame->getGameType()->cachedBotFlightPlans[pathIndex] = flightPlan;
   }
   else
      flightPlan = serverGame->getGameType()->cachedBotFlightPlans[pathIndex];

   if(flightPlan.size() > 0)
      return returnPoint(L, flightPlan.last());
   else
      return returnNil(L);    // Out of options, end of the road
}


// Thrust at velocity v toward angle a
S32 Robot::setThrust(lua_State *L)
{
   S32 profile = checkArgList(L, functionArgs, "Robot", "setThrust");

   F32 ang;
   F32 vel = getFloat(L, 1);

   if(profile == 0)           // Args: NUM, NUM  (speed, angle)
      ang = getFloat(L, 2);

   else if(profile == 1)      // Args: NUM, PT   (speed, destination)
   {
      Point point = getPointOrXY(L, 2);

      ang = getAnglePt(point) - 0 * FloatHalfPi;
   }

   Move move = getCurrentMove();

   move.x = vel * cos(ang);
   move.y = vel * sin(ang);

   setCurrentMove(move);

   return 0;
}


// Thrust toward specified point, but slow speed so that we land directly on that point if it is within range
S32 Robot::setThrustToPt(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "setThrustToPt");

   Point point = getPointOrXY(L, 1);

   F32 ang = getAnglePt(point) - 0 * FloatHalfPi;

   Move move = getCurrentMove();

   F32 dist = getActualPos().distanceTo(point);

   F32 vel = dist / ((F32) move.time);      // v = d / t, t is in ms

   if(vel > 1.f)
      vel = 1.f;

   move.x = vel * cos(ang);
   move.y = vel * sin(ang);

   setCurrentMove(move);

  return 0;
}


// Fire current weapon if possible
S32 Robot::fire(lua_State *L)
{
   Move move = getCurrentMove();
   move.fire = true;
   setCurrentMove(move);

   return 0;
}


// Set weapon to specified weapon, if we have it
S32 Robot::setWeapon(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "setWeapon");

   U32 weap = (U32)getInt(L, 1);

   // Check the weapons we have on board -- if any match the requested weapon, activate it
   for(S32 i = 0; i < ShipWeaponCount; i++)
      if((U32)getWeapon(i) == weap)
      {
         selectWeapon(i);
         break;
      }

   // If we get here without having found our weapon, then nothing happens.  Better luck next time!
   return 0;
}


// Set weapon to index of slot (i.e. 1, 2, or 3)
S32 Robot::setWeaponIndex(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "setWeaponIndex");

   U32 weap = (U32)getInt(L, 1); // Acceptable range = (1, ShipWeaponCount) -- has already been verified by checkArgList()
   selectWeapon(weap - 1);       // Correct for the fact that index in C++ is 0 based

   return 0;
}


// Do we have a given weapon in our current loadout?
S32 Robot::hasWeapon(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "hasWeapon");
   U32 weap = (U32)getInt(L, 1);

   for(S32 i = 0; i < ShipWeaponCount; i++)
      if((U32)getWeapon(i) == weap)
         return returnBool(L, true);      // We have it!

   return returnBool(L, false);           // We don't!
}


// Activate module this cycle --> takes module enum.
// If specified module is not part of the loadout, does nothing.
S32 Robot::activateModule(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "activateModule");

   ShipModule mod = (ShipModule) getInt(L, 1);

   for(S32 i = 0; i < ShipModuleCount; i++)
      if(getModule(i) == mod)
      {
         activateModulePrimary(i);
         break;
      }

   return 0;
}


// Activate module this cycle --> takes module index
S32 Robot::activateModuleIndex(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "activateModuleIndex");

   U32 indx = (U32)getInt(L, 1);

   activateModulePrimary(indx);

   return 0;
}


// Sets requested loadout to specified
S32 Robot::setReqLoadout(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "setReqLoadout");

   Vector<U8> vec;

   LuaLoadout *loadout = luaW_check<LuaLoadout>(L, 1);

   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      vec.push_back(loadout->getLoadoutItem(i));

   getOwner()->sRequestLoadout(vec);

   return 0;
}


// Sets loadout to specified 
S32 Robot::setCurrLoadout(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "setCurrLoadout");

   Vector<U8> vec;

   LuaLoadout *loadout = luaW_check<LuaLoadout>(L, 1);
   
   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      vec.push_back(loadout->getLoadoutItem(i));

   if(getGame()->getGameType()->validateLoadout(vec) == "")
      setLoadout(vec);

   return 0;
}


// Send message to all players
S32 Robot::globalMsg(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "globalMsg");

   const char *message = getString(L, 1);

   GameType *gt = getGame()->getGameType();
   if(gt)
   {
      gt->sendChatFromRobot(true, message, getClientInfo());

      // Fire our event handler
      EventManager::get()->fireEvent(getScriptId(), EventManager::MsgReceivedEvent, message, getPlayerInfo(), true);
   }

   return 0;
}


// Send message to team (what happens when neutral/hostile robot does this???)
S32 Robot::teamMsg(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "teamMsg");

   const char *message = getString(L, 1);

   GameType *gt = getGame()->getGameType();
   if(gt)
   {
      gt->sendChatFromRobot(false, message, getClientInfo());

      // Fire our event handler
      EventManager::get()->fireEvent(getScriptId(), EventManager::MsgReceivedEvent, message, getPlayerInfo(), false);
   }

   return 0;
}


// Return list of all items of specified type within normal visible range... does no screening at this point
S32 Robot::findItems(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "findItems");

   Point pos = getActualPos();
   Rect queryRect(pos, pos);
   queryRect.expand(getGame()->computePlayerVisArea(this));  // XXX This may be wrong...  computePlayerVisArea is only used client-side

   return doFindItems(L, "Robot:findItems", &queryRect);
}


// Same but gets all visible items from whole game... out-of-scope items will be ignored
S32 Robot::findGlobalItems(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "findGlobalItems");

   return doFindItems(L, "Robot:findGlobalItems");
}


// If scope is NULL, we find all items
S32 Robot::doFindItems(lua_State *L, const char *methodName, Rect *scope)
{
   fillVector.clear();
   static Vector<U8> types;

   types.clear();

   // We expect the stack to look like this: -- [fillTable], objType1, objType2, ...
   // We'll work our way down from the top of the stack (element -1) until we find something that is not a number.
   // We expect that when we find something that is not a number, the stack will only contain our fillTable.  If the stack
   // is empty at that point, we'll add a table, and warn the user that they are using a less efficient method.
   while(lua_isnumber(L, -1))
   {
      U8 typenum = (U8)lua_tointeger(L, -1);

      // Requests for botzones have to be handled separately; not a problem, we'll just do the search here, and add them to
      // fillVector, where they'll be merged with the rest of our search results.
      if(typenum != BotNavMeshZoneTypeNumber)
         types.push_back(typenum);
      else
      {
         if(scope)   // Get other objects on screen-visible area only
            BotNavMeshZone::getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, fillVector, *scope);
         else        // Get all objects
            BotNavMeshZone::getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, fillVector);
      }

      lua_pop(L, 1);
   }
   
   if(scope)      // Get other objects on screen-visible area only
      getGame()->getGameObjDatabase()->findObjects(types, fillVector, *scope);
   else           // Get all objects
      getGame()->getGameObjDatabase()->findObjects(types, fillVector);


   // We are expecting a table to be on top of the stack when we get here.  If not, we can add one.
   if(!lua_istable(L, -1))
   {
      TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");

      logprintf(LogConsumer::LogWarning, 
                  "Finding objects will be far more efficient if your script provides a table -- see scripting docs for details!");
      lua_createtable(L, fillVector.size(), 0);    // Create a table, with enough slots pre-allocated for our data
   }

   TNLAssert((lua_gettop(L) == 1 && lua_istable(L, -1)) || LuaObject::dumpStack(L), "Should only have table!");


   S32 pushed = 0;      // Count of items we put into our table

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      if(isShipType(fillVector[i]->getObjectTypeNumber()))
      {
         // Ignore self (use cheaper typeNumber check first) 
         // TODO: Do we need the cast here, or can we compare fillVector[i] to this directly??
         if(fillVector[i]->getObjectTypeNumber() == RobotShipTypeNumber && static_cast<Robot *>(fillVector[i]) == this) 
            continue;

         // Ignore ship/robot if it's dead or cloaked
         Ship *ship = static_cast<Ship *>(fillVector[i]);
         if(!ship->isVisible() || ship->hasExploded)
            continue;
      }

      static_cast<BfObject *>(fillVector[i])->push(L);
      pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   TNLAssert(lua_gettop(L) == 1 || LuaObject::dumpStack(L), "Stack has unexpected items on it!");

   return 1;
}


static bool calcInterceptCourse(BfObject *target, Point aimPos, F32 aimRadius, S32 aimTeam, F32 aimVel, 
                                F32 aimLife, bool ignoreFriendly, F32 &interceptAngle)
{
   Point offset = target->getPos() - aimPos;    // Account for fact that robot doesn't fire from center
   offset.normalize(aimRadius * 1.2f);          // 1.2 ==> fudge factor to prevent robot from not shooting because it thinks it will hit itself
   aimPos += offset;

   bool targetIsShip = isShipType(target->getObjectTypeNumber());

   if(targetIsShip)
   {
      Ship *potential = static_cast<Ship *>(target);

      // Is it dead or cloaked?  If so, ignore
      if(!potential->isVisible() || potential->hasExploded)
         return false;
   }

   if(ignoreFriendly && target->getTeam() == aimTeam)      // Is target on our team?
      return false;                                        // ...if so, skip it!

   // Calculate where we have to shoot to hit this...
   Point Vs = target->getVel();

   Point d = target->getPos() - aimPos;

   F32 t;      // t is set in next statement
   if(!FindLowestRootInInterval(Vs.dot(Vs) - aimVel * aimVel, 2 * Vs.dot(d), d.dot(d), aimLife * 0.001f, t))
      return false;

   Point leadPos = target->getPos() + Vs * t;

   // Calculate distance
   Point delta = (leadPos - aimPos);

   // Make sure we can see it...
   Point n;
   TestFunc testFunc = isWallType;

   if(!targetIsShip)    // If the target isn't a ship, take forcefields into account
      testFunc = isFlagCollideableType;

   if(target->findObjectLOS(testFunc, ActualState, aimPos, target->getPos(), t, n))
      return false;

   // See if we're gonna clobber our own stuff...
   target->disableCollision();

   Point delta2 = delta;
   delta2.normalize(aimLife * aimVel / 1000);

   BfObject *hitObject = target->findObjectLOS((TestFunc)isWithHealthType, 0, aimPos, aimPos + delta2, t, n);
   target->enableCollision();

   if(ignoreFriendly && hitObject && hitObject->getTeam() == aimTeam)
      return false;

   interceptAngle = delta.ATAN2();

   return true;
}


// Given an object, which angle do we need to be at to fire to hit it?
// Returns nil if a workable solution can't be found
// Logic adapted from turret aiming algorithm
// Note that bot WILL fire at teammates if you ask it to!
S32 Robot::getFiringSolution(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "getFiringSolution");

   Item *target = luaW_check<Item>(L, 1);

   WeaponInfo weap = GameWeapon::weaponInfo[getSelectedWeapon()];    // Robot's active weapon

   F32 interceptAngle;

   if(calcInterceptCourse(target, getActualPos(), getRadius(), getTeam(), (F32)weap.projVelocity, (F32)weap.projLiveTime, false, interceptAngle))
      return returnFloat(L, interceptAngle);

   return returnNil(L);
}


// Given an object, what angle do we need to fly toward in order to collide with an object?  This
// works a lot like getFiringSolution().
S32 Robot::getInterceptCourse(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "getInterceptCourse");

   Item *target = luaW_check<Item>(L, 1);

   F32 interceptAngle;     // <== will be set by calcInterceptCourse() below

   if(calcInterceptCourse(target, getActualPos(), getRadius(), getTeam(), 256, 3000, false, interceptAngle))
      return returnFloat(L, interceptAngle);
      
   return returnNil(L);
}


S32 Robot::engineerDeployObject(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "engineerDeployObject");

   S32 type = (S32)lua_tointeger(L, 0);

   return returnBool(L, getOwner()->sEngineerDeployObject(type));
}


S32 Robot::dropItem(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "dropItem");

   S32 count = mMountedItems.size();
   for(S32 i = count - 1; i >= 0; i--)
      mMountedItems[i]->onItemDropped();

   return 0;
}


S32 Robot::copyMoveFromObject(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "copyMoveFromObject");

   Item *obj = luaW_check<Item>(L, 1);

   Move move = obj->getCurrentMove();
   move.time = getCurrentMove().time;     // Keep current move time
   setCurrentMove(move);

   return 0;
}


};
