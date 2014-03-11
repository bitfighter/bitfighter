//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "robot.h"

#include "playerInfo.h"          // For RobotPlayerInfo constructor
#include "BotNavMeshZone.h"      // For BotNavMeshZone class definition
#include "gameObjectRender.h"
#include "GameSettings.h"

#include "MathUtils.h"           // For findLowestRootIninterval()
#include "GeomUtils.h"

#include "ServerGame.h"
#include "GameManager.h"


#define hypot _hypot    // Kill some warnings

namespace Zap
{

using namespace LuaArgs;

////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(Robot);

/**
 * @luafunc Robot::Robot(point position, int teamIndex, string scriptName, string scriptArg)
 *
 * @param [position] Starting position of the Robot. Defaults to (0, 0)
 * @param [teamIndex] Starting Team of the Robot. If unspecified, defaults
 * to balancing teams.
 * @param [scriptName] The bot script to use. Defaults to the server's default bot.
 * @param [scriptArg] Zero or more string arguments to pass to the script.
 */
Robot::Robot(lua_State *L) : Ship(NULL, TEAM_NEUTRAL, Point(0,0), true),   
                             LuaScriptRunner() 
{
   if(L)
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { PT, TEAM_INDX, END }, { PT, TEAM_INDX, STRS, END }}, 3 };
      S32 profile = checkArgList(L, constructorArgList, "Robot", "constructor");
      S32 i = 1;

      if(profile >= 1) {
         setPos(L, i++);
         setTeam(L, i++);
      }

      if(profile == 2)
      {
         mScriptName = GameSettings::getFolderManager()->findBotFile(getString(L, i++));

         while(i <= lua_gettop(L))
         {
            mScriptArgs.push_back(getString(L, i++));
         }
      }

      lua_pop(L, i - 1);
      TNLAssert(lua_gettop(L) == 0, "Stack dirty");
   }

   mHasSpawned = false;
   mObjectTypeNumber = RobotShipTypeNumber;

   mCurrentZone = U16_MAX;
   flightPlanTo = U16_MAX;

   mPlayerInfo = new RobotPlayerInfo(this);

#ifndef ZAP_DEDICATED
   mShapeType = ShipShape::Normal;
#endif

   mScriptType = ScriptTypeRobot;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor, runs on client and server
Robot::~Robot()
{
   // Items will be dismounted in Ship (Parent) destructor
   setOwner(NULL);

   if(isClient())
   {
      delete mPlayerInfo;     // On the server, mPlayerInfo will be deleted below, after event is fired
      return;
   }

   dismountAll();  // fixes dropping CTF flag without ClientInfo...

   // Server only from here on down
   if(getGame())  // can be NULL if this robot was never added to game (bad / missing robot file)
   {
      EventManager::get()->fireEvent(this, EventManager::PlayerLeftEvent, getPlayerInfo());

      if(getGame()->getGameType())
         getGame()->getGameType()->serverRemoveClient(mClientInfo);

      getGame()->removeBot(this);
      logprintf(LogConsumer::LogLuaObjectLifecycle, "Robot %s terminated (%d bots left)", mScriptName.c_str(), getGame()->getRobotCount());
   }

   delete mPlayerInfo;
   if(mClientInfo.isValid())
   {
      getGame()->removeFromClientList(mClientInfo.getPointer());
	   delete mClientInfo.getPointer();
   }

   // Even though a similar line gets called when parent classes are destructed, we need this here to set our very own personal copy
   // of luaProxy as defunct.  Each "level" of an object has their own private lauProxy object that needs to be individually marked.
   LUAW_DESTRUCTOR_CLEANUP;
}


// Reset everything on the robot back to the factory settings -- runs only when bot is spawning in GameType::spawnRobot()
// Only runs on server!
bool Robot::initialize(Point &pos)
{
   TNLAssert(!isGhost(), "Server only, dude!");

   try
   {
      flightPlan.clear();

      mCurrentZone = U16_MAX;   // Correct value will be calculated upon first request

      Parent::initialize(pos);

      // Robots added via robot.new() get intialized.  If the robot is added in a script's main() 
      // function, the bot will be reinitialized when the game starts.  This check avoids that.
      if(!isCollisionEnabled())
         enableCollision();

      // WarpPositionMask triggers the spinny spawning visual effect
      setMaskBits(RespawnMask | HealthMask        | LoadoutMask         | PositionMask | 
                  MoveMask    | ModulePrimaryMask | ModuleSecondaryMask | WarpPositionMask);      // Send lots to the client

      EventManager::get()->update();   // Ensure registrations made during bot initialization are ready to go
   }
   catch(LuaException &e)
   {
      logError("Robot error during spawn: %s.  Shutting robot down.", e.what());
      clearStack(L);
      return false;
   }

   return true;
} 


const char *Robot::getErrorMessagePrefix() { return "***ROBOT ERROR***"; }


// Server only
bool Robot::start()
{
   if(!getGame() || !runScript(!getGame()->isTestServer()))   // Load the script, execute the chunk to get it in memory, then run its main() function
      return false;

   // Pass true so that if this bot doesn't have a TickEvent handler, we don't print a message
   EventManager::get()->subscribe(this, EventManager::TickEvent, RobotContext, true);

   mSubscriptions[EventManager::TickEvent] = true;

   string name = runGetName();                                          // Run bot's getName function
   mClientInfo->setName(getGame()->makeUnique(name.c_str()).c_str());   // Make sure name is unique

   mHasSpawned = true;

   mGame->addToClientList(mClientInfo);

   return true;
}


bool Robot::prepareEnvironment()
{
   try
   {
      if(!LuaScriptRunner::prepareEnvironment())
         return false;

      // Set this first so we have this object available in the helper functions in case we need overrides
      setSelf(L, this, "bot");

      if(!loadAndRunGlobalFunction(L, ROBOT_HELPER_FUNCTIONS_KEY, RobotContext))
         return false;
   }
   catch(LuaException &e)
   {
      logError(e.what());
      clearStack(L);
      return false;
   }

   return true;
}


static string getNextName()
{
   static const string botNames[] = {
      "Addison", "Alexis", "Amelia", "Audrey", "Chloe", "Claire", "Elizabeth", "Ella",
      "Emily", "Emma", "Evelyn", "Gabriella", "Hailey", "Hannah", "Isabella", "Layla",
      "Lillian", "Lucy", "Madison", "Natalie", "Olivia", "Riley", "Samantha", "Zoe"
   };

   static U8 nameIndex = 0;
   return botNames[(nameIndex++) % ARRAYSIZE(botNames)];
}


// Run bot's getName function, return default name if fn isn't defined
string Robot::runGetName()
{
   TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack dirty!");

   // error will only be true if: 1) getName doesn't exist, which should never happen -- getName is stubbed out in robot_helper_functions.lua
   //                             2) getName generates an error
   //                             3) something is hopelessly corrupt (see 1)
   // Note that it is valid for getName to return a nil (or not be implemented in the bot itself) in which case a default name will be chosen.
   // If error is true, runCmd will terminate bot script by running killScript(), so we don't need to worry about making things too nice.
   bool error = runCmd("getName", 1);     

   string name = "";

   if(!error)
   {
      if(lua_isstring(L, -1))   // getName should have left a name on the stack
      {
         name = lua_tostring(L, -1);

         if(name == "")
            name = getNextName();
      }
      else
      {
         // If getName is not implemented, or returns nil, this is not an error; it just means we pick a name for the bot
         if(!lua_isnil(L, -1))
            logprintf(LogConsumer::LogWarning, "Robot error retrieving name (returned value was not a string).  Using \"%s\".", name.c_str());

         name = getNextName();
      }
   }

   clearStack(L);
   return name;
}


// This only runs the very first time the robot is added to the level.
// Note that level may not yet be ready, so the bot can't spawn yet.
// Runs on client and server.
void Robot::onAddedToGame(Game *game)
{
   if(isGhost())     // No clients allowed!
   {
      // Parent::onAddedToGame(game);  ==> This was never actually included in the game, but I feel like it should be added. -CE
      return;
   }

   // Robots are created with NULL ClientInfos.  We'll add a valid one here.
   TNLAssert(mClientInfo.isNull(), "mClientInfo should be NULL");

   // Bots from a variety of sources will pass through here.  Sources other than those added by a level
   // will have to run ClientInfo::setClientClass() after the bot is added.
   ClientInfo::ClientClass clientClass = getTeam() < 0 ? ClientInfo::ClassRobotAddedByLevelNoTeam : ClientInfo::ClassRobotAddedByLevel;

   mClientInfo = new FullClientInfo(game, NULL, "Robot", clientClass);  // deleted in destructor
   mClientInfo->setShip(this);

   setOwner(mClientInfo);
   
   mHasExploded = true;        // Because we start off "dead", but will respawn real soon now...
   disableCollision();

   game->addBot(this);        // Add this robot to the list of all robots (can't do this in constructor or else it gets run on client side too...)
  
   EventManager::get()->fireEvent(this, EventManager::PlayerJoinedEvent, getPlayerInfo());

   // Check whether a script file has been specified. If not, use the default
   if(mScriptName == "")
   {
      string scriptName = game->getSettings()->getIniSettings()->defaultRobotScript;
      mScriptName = GameSettings::getFolderManager()->findBotFile(scriptName);
   }

   mLuaGame = game;
   mLuaGridDatabase = game->getGameObjDatabase();

   if(!start())
   {
      deleteObject(0);
      return;
   }

   disableCollision();
   game->getGameType()->serverAddClient(mClientInfo.getPointer());
   enableCollision();

   Parent::onAddedToGame(game);
}


void Robot::killScript()
{
   deleteObject();
}


// Robot just died
void Robot::kill()
{
   if(mHasExploded) 
      return;

   mHasExploded = true;

   setMaskBits(ExplodedMask);
   if(!isGhost() && getOwner())
      getOwner()->saveActiveLoadout(mLoadout);

   disableCollision();

   dismountAll();
}


// Need this, as this may come from level or levelgen
bool Robot::processArguments(S32 argc, const char **argv, Game *game)
{
   string errorMessage;

   bool retcode = processArguments(argc, argv, game, errorMessage);

   if(errorMessage != "")
   {
      string line;
      
      for(S32 i = 0; i < argc; i++)
      {
         if(i > 0)
            line += " ";
         line += argv[i];
      }

      if(GameManager::getServerGame())
         logprintf(LogConsumer::LogLevelError, "Levelcode error in level %s, line \"%s\":\n\t%s",
                   GameManager::getServerGame()->getCurrentLevelFileName().c_str(), line.c_str(), errorMessage.c_str());
      else
         logprintf(LogConsumer::LogLevelError, "Levelcode error, line \"%s\":\n\t%s",
                   line.c_str(), errorMessage.c_str());
   }

   return retcode;
}


// Expect [team] [bot script file] [bot args]
bool Robot::processArguments(S32 argc, const char **argv, Game *game, string &errorMessage)
{
   S32 team;

   if(argc <= 1)
      team = NO_TEAM;   
   else
   {
      team = atoi(argv[0]);

      if(team != NO_TEAM && (team < 0 || team >= game->getTeamCount()))
      {
         errorMessage = "Invalid team specified";
         return false;
      }
   }

   setTeam(team);
   
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
      return false;
   }

   // Collect our arguments to be passed into the args table in the robot (starting with the robot name)
   // Need to make a copy or containerize argv[i] somehow, because otherwise new data will get written
   // to the string location subsequently, and our vals will change from under us.  That's bad!
   for(S32 i = 2; i < argc; i++)        // Does nothing if we have no args
      mScriptArgs.push_back(string(argv[i]));

   // I'm not sure this goes here, but it needs to be set early in setting up the Robot, but after
   // the constructor
   //
   // Our 'Game' pointer in LuaScriptRunner is the same as the one in this game object
   mLuaGame = game;
   mLuaGridDatabase = game->getGameObjDatabase();

   return true;
}


// Returns zone ID of current zone
S32 Robot::getCurrentZone()
{
   TNLAssert(getGame()->isServer(), "Not a ServerGame");

   // We're in uncharted territory -- try to get the current zone
   mCurrentZone = static_cast<ServerGame *>(getGame())->findZoneContaining(getActualPos());

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

   Rect queryRect(thisPoints);

   fillVector.clear();
   mGame->getGameObjDatabase()->findObjects(wallOnly ? (TestFunc)isWallType : (TestFunc)isCollideableType, fillVector, queryRect);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      const Vector<Point> *otherPoints = fillVector[i]->getCollisionPoly();
      if(otherPoints && polygonsIntersect(thisPoints, *otherPoints))
         return false;
   }

   return true;
}


void Robot::renderLayer(S32 layerIndex)
{
#ifndef ZAP_DEDICATED
   if(isGhost())                                         // Client rendering client's objects
      Parent::renderLayer(layerIndex);

   else if(layerIndex == 1 && flightPlan.size() != 0)    // WARNING!!  Client hosting is rendering server objects
      renderFlightPlan(getActualPos(), flightPlan.last(), flightPlan);
#endif
}


void Robot::idle(BfObject::IdleCallPath path)
{
   TNLAssert(path != BfObject::ServerProcessingUpdatesFromClient, "Should never idle with ServerProcessingUpdatesFromClient");

   if(mHasExploded)
      return;

   if(path != BfObject::ServerIdleMainLoop)   
      Parent::idle(path);                       
   else                         
   {
      mSendSpawnEffectTimer.update(mCurrentMove.time); // This is to fix robot go spinny, since we skipped Ship::idle(ServerIdleMainLoop)

      // Robot Timer ticks are now processed in the global Lua Timer along with
      // levelgens in ServerGame::idle()

      Parent::idle(BfObject::ServerProcessingUpdatesFromClient);   // Let's say the script is the client  ==> really not sure this is right
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

   getGame()->getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, objects, rect);

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
      Point extentsCenter = getGame()->getWorldExtents()->getCenter();

      F32 collisionTimeIgnore;
      Point surfaceNormalIgnore;

      DatabaseObject* object = getGame()->getBotZoneDatabase()->findObjectLOS(BotNavMeshZoneTypeNumber,
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
   METHOD(CLASS,  setAngle,             ARRAYDEF({{ PT, END }, { NUM, END }}), 2 )           \
   METHOD(CLASS,  getAnglePt,           ARRAYDEF({{ PT, END }              }), 1 )           \
   METHOD(CLASS,  canSeePoint,          ARRAYDEF({{ PT, END }              }), 1 )           \
                                                                                             \
   METHOD(CLASS,  getWaypoint,          ARRAYDEF({{ PT, END }}), 1 )                         \
                                                                                             \
   METHOD(CLASS,  setThrust,            ARRAYDEF({{ NUM, NUM, END }, { NUM, PT, END}}), 2 )  \
   METHOD(CLASS,  setThrustToPt,        ARRAYDEF({{ PT,       END }                 }), 1 )  \
                                                                                             \
   METHOD(CLASS,  fireWeapon,           ARRAYDEF({{ WEAP_ENUM, END }}), 1 )                  \
   METHOD(CLASS,  hasWeapon,            ARRAYDEF({{ WEAP_ENUM, END }}), 1 )                  \
                                                                                             \
   METHOD(CLASS,  fireModule,           ARRAYDEF({{ MOD_ENUM, END }}), 1 )                   \
   METHOD(CLASS,  hasModule,            ARRAYDEF({{ MOD_ENUM, END }}), 1 )                   \
                                                                                             \
   METHOD(CLASS,  setLoadoutWeapon,     ARRAYDEF({{ WEAP_SLOT, WEAP_ENUM, END }}), 1 )       \
   METHOD(CLASS,  setLoadoutModule,     ARRAYDEF({{ MOD_SLOT,  MOD_ENUM,  END }}), 1 )       \
                                                                                             \
   METHOD(CLASS,  globalMsg,            ARRAYDEF({{ STR, END }}), 1 )                        \
   METHOD(CLASS,  teamMsg,              ARRAYDEF({{ STR, END }}), 1 )                        \
   METHOD(CLASS,  privateMsg,           ARRAYDEF({{ STR, STR, END }}), 1 )                   \
                                                                                             \
   METHOD(CLASS,  findVisibleObjects,   ARRAYDEF({{ TABLE, INTS, END }, { INTS, END }}), 2 ) \
   METHOD(CLASS,  findClosestEnemy,     ARRAYDEF({{              END }, { NUM,  END }}), 2 ) \
                                                                                             \
   METHOD(CLASS,  getFiringSolution,    ARRAYDEF({{ BFOBJ, END }}), 1 )                      \
   METHOD(CLASS,  getInterceptCourse,   ARRAYDEF({{ BFOBJ, END }}), 1 )                      \
                                                                                             \
   METHOD(CLASS,  engineerDeployObject, ARRAYDEF({{ INT,    END }}), 1 )                     \
   METHOD(CLASS,  dropItem,             ARRAYDEF({{         END }}), 1 )                     \
   METHOD(CLASS,  copyMoveFromObject,   ARRAYDEF({{ MOVOBJ, END }}), 1 )                     \


GENERATE_LUA_METHODS_TABLE(Robot, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(Robot, LUA_METHODS);

#undef LUA_METHODS


const char *Robot::luaClassName = "Robot";
REGISTER_LUA_SUBCLASS(Robot, Ship);


/**
 * @luafunc Robot::setAngle(num angle)
 *
 * @brief Sets the Robot's angle to `angle`
 *
 * @param angle The desired angle in radians
 */

/**
 * @luafunc Robot::setAngle(point p)
 *
 * @brief Sets the Robot's angle to point towards `p`
 *
 * @param p The point to point at
 */
S32 Robot::lua_setAngle(lua_State *L)
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


/**
 * @luafunc num Robot::getAnglePt(point p)
 *
 * @brief Gets the angle from the Robot to point `p`
 *
 * @return The angle in radians
 */
S32 Robot::lua_getAnglePt(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "getAnglePt");

   Point point = getPointOrXY(L, 1);

   return returnFloat(L, getAnglePt(point));
}


/**
 * @luafunc bool Robot::canSeePoint(point pt)
 * 
 * @brief Does this robot have line-of-sight to the given point.
 * 
 * @descr Line-of-sight a straight path from the robot to the object without any
 * stationary, collideable objects in the way
 * 
 * @param pt point to test
 * 
 * @return `true` if this bot can see the given point, `false` otherwise
 */
S32 Robot::lua_canSeePoint(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "canSeePoint");

   Point point = getPointOrXY(L, 1);

   return returnBool(L, canSeePoint(point));
}


/**
 * @luafunc point Robot::getWaypoint(point p)
 * 
 * @brief Get next waypoint to head toward in order to move to `p`
 * 
 * @descr Finds a path from the current position to `p` using the built-in
 * pathing utility. The algorithm will consider teleporters and tries to take
 * the shortest route to `p`.
 *
 * @param p The destination point
 * 
 * @return The next point to head towards, or `nil` if no path can be found
 */

// Note that this function will be called frequently by various robots, so any
// optimizations will be helpful.
S32 Robot::lua_getWaypoint(lua_State *L)
{
   TNLAssert(getGame()->isServer(), "Not a ServerGame");

   checkArgList(L, functionArgs, "Robot", "getWaypoint");

   Point target = getPointOrXY(L, 1);

   // If we can see the target, go there directly
   if(canSeePoint(target, true))
   {
      flightPlan.clear();
      return returnPoint(L, target);
   }

   // TODO: cache destination point; if it hasn't moved, then skip ahead.

   U16 targetZone = static_cast<ServerGame *>(getGame())->findZoneContaining(target); // Where we're going  ===> returns zone id

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
         BotNavMeshZone *zone = static_cast<BotNavMeshZone *>(getGame()->getBotZoneDatabase()->getObjectByIndex(targetZone));

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

   const Vector<BotNavMeshZone *> *zones = static_cast<ServerGame *>(getGame())->getBotZones();  // Our pre-cached list of nav zones

   if(getGame()->getGameType()->cachedBotFlightPlans.find(pathIndex) == getGame()->getGameType()->cachedBotFlightPlans.end())
   {
      // Not found so calculate flight plan
      flightPlan = AStar::findPath(zones, currentZone, targetZone, target);

      // Add to cache
      getGame()->getGameType()->cachedBotFlightPlans[pathIndex] = flightPlan;
   }
   else
      flightPlan = getGame()->getGameType()->cachedBotFlightPlans[pathIndex];

   if(flightPlan.size() > 0)
      return returnPoint(L, flightPlan.last());
   else
      return returnNil(L);    // Out of options, end of the road
}


/**
 * @luafunc Ship Robot::findClosestEnemy(num range)
 * 
 * @brief Finds the closest enemy ship or robot that is within the specified
 * distance.
 * 
 * @descr Finds closest enemy within specified distance of the bot. If dist is
 * omitted, this will use standard scanner range, taking into account whether
 * the bot has the Sensor module. To search the entire map, specify -1 for the
 * range.
 * 
 * @param range (Optional) Radius in which to search. Use -1 to search entire
 * map. If omitted, will use normal scanner range.
 * 
 * @return Ship object representing closest enemy, or nil if none were found.
 */
S32 Robot::lua_findClosestEnemy(lua_State *L)
{
   S32 profile = checkArgList(L, functionArgs, "Robot", "findClosestEnemy");

   Point pos = getActualPos();
   Rect queryRect(pos, pos);
   bool useRange = true;

   if(profile == 0)           // Args: None
      queryRect.expand(getGame()->computePlayerVisArea(this));  
   else                       // Args: Range
   {
      F32 range = getFloat(L, 1);
      if(range == -1)
         useRange = false;
        else
         queryRect.expand(Point(range, range));
   }


   F32 minDist = F32_MAX;
   Ship *closest = NULL;

   fillVector.clear();

   if(useRange)
      getGame()->getGameObjDatabase()->findObjects((TestFunc)isShipType, fillVector, queryRect);   
   else
      getGame()->getGameObjDatabase()->findObjects((TestFunc)isShipType, fillVector);   

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      // Ignore self 
      if(fillVector[i] == this) 
         continue;

      // Ignore ship/robot if it's dead or cloaked
      Ship *ship = static_cast<Ship *>(fillVector[i]);
      if(ship->mHasExploded || !ship->isVisible(hasModule(ModuleSensor)))
         continue;

      // Ignore ships on same team during team games
      if(ship->getTeam() == getTeam() && getGame()->getGameType()->isTeamGame())
         continue;

      F32 dist = ship->getActualPos().distSquared(getActualPos());
      if(dist < minDist)
      {
         minDist = dist;
         closest = ship;
      }
   }

   return returnShip(L, closest);    // Handles closest == NULL
}


/**
 * @luafunc Robot::setThrust(num speed, num angle)
 *
 * @brief Makes the Robot thrust at `speed` along `angle`.
 *
 * @param speed The desired speed in units per second.
 * @param angle The desired angle in radians.
 */

/**
 * @luafunc Robot::setThrust(num speed, point p)
 *
 * @brief Makes the Robot thrust at `speed` towards `p`.
 *
 * @param speed The desired speed in units per second.
 * @param p The point move towards.
 */
S32 Robot::lua_setThrust(lua_State *L)
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


/**
 * @luafunc Robot::setThrustToPt(point p)
 *
 * @brief Makes the Robot thrust to `p`, stopping when it reaches that point.
 *
 * @param p The point move towards.
 */
S32 Robot::lua_setThrustToPt(lua_State *L)
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


/**
 * @luafunc Robot::fireWeapon(Weapon weapon)
 *
 * @brief Shoots the given weapon if it is equipped.
 *
 * @param weapon \ref WeaponEnum to fire.
 */
S32 Robot::lua_fireWeapon(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "fireWeapon");

   WeaponType weapon = getWeaponType(L, 1);
   bool hasWeapon = false;

   // Check the weapons we have on board -- if any match the requested weapon, select it
   for(S32 i = 0; i < ShipWeaponCount; i++)
      if(mLoadout.getWeapon(i) == weapon)
      {
         hasWeapon = true;
         selectWeapon(i);
         break;
      }

   // If weapon was equipped, fire!
   if(hasWeapon)
   {
      Move move = getCurrentMove();
      move.fire = true;
      setCurrentMove(move);
   }
   else
      throw LuaException("The weapon given to bot:fireWeapon(weapon) is not equipped!");

   return 0;
}


/**
 * @luafunc bool Robot::hasWeapon(Weapon weapon)
 * 
 * @brief Does the robot have the given \ref WeaponEnum.
 * 
 * @param weapon The \ref WeaponEnum to check.
 * 
 * @return `true` if the bot has the weapon, `false` if not.
 */
S32 Robot::lua_hasWeapon(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "hasWeapon");
   WeaponType weap = getWeaponType(L, 1);

   for(S32 i = 0; i < ShipWeaponCount; i++)
      if(mLoadout.getWeapon(i) == weap)
         return returnBool(L, true);      // We have it!

   return returnBool(L, false);           // We don't!
}


/**
 * @luafunc Robot::fireModule(Module module)
 * 
 * @brief Activates/fires the given module if it is equipped
 * 
 * @param module \ref ModuleEnum to fire
 */
S32 Robot::lua_fireModule(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "fireModule");

   ShipModule module = getShipModule(L, 1);

   bool hasModule = false;

   // Check if module is equipped and fire it
   for(S32 i = 0; i < ShipModuleCount; i++)
      if(getModule(i) == module)
      {
         hasModule = true;
         mCurrentMove.modulePrimary[i] = true;
         break;
      }

   if(!hasModule)
      throw LuaException("The module given to bot:fireModule(module) is not equipped!");

   return 0;
}


/**
 * @luafunc bool Robot::hasModule(Module module)
 * 
 * @brief Does the robot have the given \ref ModuleEnum.
 * 
 * @param module The \ref ModuleEnum to check
 * 
 * @return `true` if the bot has the \ref ModuleEnum, `false` if not.
 */
S32 Robot::lua_hasModule(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "hasModule");
   ShipModule module = getShipModule(L, 1);

   for(S32 i = 0; i < ShipModuleCount; i++)
      if(mLoadout.getModule(i) == module)
         return returnBool(L, true);      // We have it!

   return returnBool(L, false);           // We don't!
}


/**
 * @luafunc Robot::setLoadoutWeapon(int slot, Weapon weapon)
 * 
 * @brief Request a new loadout where the given weapon slot is changed to the
 * given weapon. This still requires the bot to change to its new loadout.
 * 
 * @param slot Weapon slot to set.
 * 
 * @param weapon \ref WeaponEnum to set.
 */
S32 Robot::lua_setLoadoutWeapon(lua_State *L)
{
   checkArgList(L, functionArgs, "Ship", "setLoadoutWeapon");

   // Slots start at index 1, but c++ wants 0
   S32 slot = getInt(L, 1) - 1;
   WeaponType weapon = getWeaponType(L, 2);

   // Make a copy of our current loadout and adjust it
   LoadoutTracker loadout = mLoadout;
   loadout.setWeapon(slot, weapon);

   // Now request the new one
   getOwner()->requestLoadout(loadout);

   return 0;
}


/**
 * @luafunc Robot::setLoadoutModule(int slot, Module module)
 * 
 * @brief Request a new loadout where the given module slot is changed to the
 * given module. This still requires the bot to change to its new loadout.
 * 
 * @param slot Module slot to set.
 * 
 * @param module \ref ModuleEnum to set.
 */
S32 Robot::lua_setLoadoutModule(lua_State *L)
{
   checkArgList(L, functionArgs, "Ship", "setLoadoutModule");

   // Slots start at index 1, but c++ wants 0
   S32 slot = getInt(L, 1) - 1;
   ShipModule module = getShipModule(L, 2);

   // Make a copy of our current loadout and adjust it
   LoadoutTracker loadout = mLoadout;
   loadout.setModule(slot, module);

   // Now request the new one
   getOwner()->requestLoadout(loadout);

   return 0;
}


/**
 * @luafunc Robot::globalMsg(string message)
 * 
 * @brief Send a message to all players.
 * 
 * @param message Message to send.
 */
S32 Robot::lua_globalMsg(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "globalMsg");

   const char *message = getString(L, 1);

   GameType *gt = getGame()->getGameType();
   if(gt)
   {
      gt->sendChat(mClientInfo->getName(), mClientInfo, message, true, mClientInfo->getTeamIndex());

      // Clean up before firing event
      lua_pop(L, 1);

      // Fire our event handler
      EventManager::get()->fireEvent(this, EventManager::MsgReceivedEvent, message, getPlayerInfo(), true);
   }

   return 0;
}


// Send message to team (what happens when neutral/hostile robot does this???)
/**
 * @luafunc Robot::teamMsg(string message)
 * 
 * @brief Send a message to this Robot's team.
 * 
 * @param message Message to send.
 */
S32 Robot::lua_teamMsg(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "teamMsg");

   const char *message = getString(L, 1);

   GameType *gt = getGame()->getGameType();
   if(gt)
   {
      gt->sendChat(mClientInfo->getName(), mClientInfo, message, false, mClientInfo->getTeamIndex());

      // Clean up before firing event
      lua_pop(L, 1);

      // Fire our event handler
      EventManager::get()->fireEvent(this, EventManager::MsgReceivedEvent, message, getPlayerInfo(), false);
   }

   return 0;
}


/**
 * @luafunc Robot::privateMsg(string message, string playerName)
 * 
 * @brief Send a private message to a player.
 * 
 * @param message Message to send.
 * 
 * @param playerName Name of player to which to send a message.
 */
// Note that identical code is found in LuaLevelGenerator::lua_privateMsg()
S32 Robot::lua_privateMsg(lua_State *L)
{
   checkArgList(L, functionArgs, luaClassName, "privateMsg");

   const char *message = getString(L, 1);
   const char *playerName = getString(L, 2);

   mGame->sendPrivateChat(mClientInfo->getName(), playerName, message);

   // No event fired for private message

   return 0;
}


/**
 * @luafunc table Robot::findVisibleObjects(table t, ObjType types, ...)
 * 
 * @brief Finds all items of the specified type within ship's area of vision.
 * 
 * @descr This search will exclude the bot itself as well as other cloaked ships
 * in the area (unless this bot has sensor equipped)
 * 
 * Can specify multiple types. The table argument is optional, but bots that
 * call this function frequently will perform better if they provide a reusable
 * table in which found objects can be stored. By providing a table, you will
 * avoid incurring the overhead of construction and destruction of a new one.
 * 
 * If a table is not provided, the function will create a table and return it on
 * the stack.
 * 
 * @param t (Optional) Reusable table into which results can be written.
 * 
 * @param types One or more \ref ObjTypeEnum specifying what types of objects to
 * find.
 * 
 * @return Either a reference back to the passed table, or a new table if one
 * was not provided.
 * 
 * @code
 *   -- Reusable container for findGlobalObjects. Because it is defined outside
 *   -- any functions, it will have global scope.
 *   items = { }
 *
 *   -- Pass one or more object types
 *   function countObjects(objType, ...)
 *     table.clear(items) -- Remove any items in table from previous use
 *     bot:findVisibleObjects(items, objType, ...)
 *     print(#items) -- Print the number of items found
 *   end
 * @endcode
 */
S32 Robot::lua_findVisibleObjects(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "findVisibleObjects");

   Point pos = getActualPos();
   Rect queryRect(pos, pos);
   queryRect.expand(getGame()->computePlayerVisArea(this));

   fillVector.clear();
   static Vector<U8> types;

   types.clear();

   // We expect the stack to look like this: -- [fillTable], objType1, objType2, ...
   // We'll work our way down from the top of the stack (element -1) until we find something that is not a number.
   // We expect that when we find something that is not a number, the stack will only contain our fillTable.  If the stack
   // is empty at that point, we'll add a table, and warn the user that they are using a less efficient method.
   while(lua_gettop(L) > 0 && lua_isnumber(L, -1))
   {
      U8 typenum = (U8)lua_tointeger(L, -1);

      // Requests for botzones have to be handled separately; not a problem, we'll just do the search here, and add them to
      // fillVector, where they'll be merged with the rest of our search results.
      if(typenum != BotNavMeshZoneTypeNumber)
         types.push_back(typenum);
      else
         getGame()->getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, fillVector, queryRect);

      lua_pop(L, 1);
   }

   // Get other objects on screen-visible area only
   getGame()->getGameObjDatabase()->findObjects(types, fillVector, queryRect);


   // We are expecting a table to be on top of the stack when we get here.  If not, we can add one.
   if(!lua_istable(L, -1))
   {
      TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack not cleared!");

      logprintf(LogConsumer::LogWarning,
                  "Finding objects will be far more efficient if your script provides a table -- see scripting docs for details!");
      lua_createtable(L, fillVector.size(), 0);    // Create a table, with enough slots pre-allocated for our data
   }

   TNLAssert((lua_gettop(L) == 1 && lua_istable(L, -1)) || dumpStack(L), "Should only have table!");


   S32 pushed = 0;      // Count of items we put into our table

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      if(isShipType(fillVector[i]->getObjectTypeNumber()))
      {
         if(fillVector[i] == this)  // Don't add this bot to the list of found objects!
            continue;

         // Ignore ship/robot if it's dead or cloaked (unless bot has sensor)
         Ship *ship = static_cast<Ship *>(fillVector[i]);
         bool callerHasSensor = this->hasModule(ModuleSensor);
         if(!ship->isVisible(callerHasSensor) || ship->mHasExploded)
            continue;
      }

      static_cast<BfObject *>(fillVector[i])->push(L);
      pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   TNLAssert(lua_gettop(L) == 1 || dumpStack(L), "Stack has unexpected items on it!");

   return 1;
}


static bool calcInterceptCourse(BfObject *target, Point aimPos, F32 aimRadius, S32 aimTeam, F32 aimVel, 
                                F32 aimLife, bool ignoreFriendly, bool botHasSensor, F32 &interceptAngle)
{
   if(target == NULL)
      return false;

   Point offset = target->getPos() - aimPos;    // Account for fact that robot doesn't fire from center
   offset.normalize(aimRadius * 1.2f);          // 1.2 ==> fudge factor to prevent robot from not shooting because it thinks it will hit itself
   aimPos += offset;

   bool targetIsShip = isShipType(target->getObjectTypeNumber());

   if(targetIsShip)
   {
      Ship *potential = static_cast<Ship *>(target);

      // Is it dead or cloaked?  If so, ignore
      if(!potential->isVisible(botHasSensor) || potential->mHasExploded)
         return false;
   }

   if(ignoreFriendly && target->getTeam() == aimTeam)      // Is target on our team?
      return false;                                        // ...if so, skip it!

   // Calculate where we have to shoot to hit this...
   Point Vs = target->getVel();

   Point d = target->getPos() - aimPos;

   F32 t;      // t is set in next statement
   if(!findLowestRootInInterval(Vs.dot(Vs) - aimVel * aimVel, 2 * Vs.dot(d), d.dot(d), aimLife * 0.001f, t))
      return false;

   Point leadPos = target->getPos() + Vs * t;

   // Calculate distance
   Point delta = (leadPos - aimPos);

   // Make sure we can see it...
   Point n;

   DatabaseObject* objectInTheWay = target->findObjectLOS(isFlagCollideableType, ActualState, aimPos, target->getPos(), t, n);

   if(objectInTheWay && objectInTheWay != target)
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


/**
 * @luafunc num Robot::getFiringSolution(BfObject obj)
 *
 * @brief Calculate the angle to fire at in order to hit `obj` with the current
 * Weapon.
 *
 * @param obj The intended target.
 *
 * @return The angle to fire at, or `nil` if no solution can be found.
 *
 * @note The bot WILL fire at teammates if you ask it to!
 */
S32 Robot::lua_getFiringSolution(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "getFiringSolution");

   BfObject *target = luaW_check<BfObject>(L, 1);

   WeaponInfo weap = WeaponInfo::getWeaponInfo(mLoadout.getActiveWeapon());    // Robot's active weapon

   F32 interceptAngle;

   if(calcInterceptCourse(target, getActualPos(), getRadius(), getTeam(), (F32)weap.projVelocity, 
                          (F32)weap.projLiveTime, false, hasModule(ModuleSensor), interceptAngle))
      return returnFloat(L, interceptAngle);

   return returnNil(L);
}


/**
 * @luafunc num Robot::getInterceptCourse(BfObject obj)
 *
 * @brief Calculate the angle to fly at in order to collide with `obj`.
 *
 * @param obj The intended target.
 *
 * @return The angle to fly at, or `nil` if no solution can be found.
 */
S32 Robot::lua_getInterceptCourse(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "getInterceptCourse");

   BfObject *target = luaW_check<BfObject>(L, 1);

   F32 interceptAngle;     // <== will be set by calcInterceptCourse() below

   if(calcInterceptCourse(target, getActualPos(), getRadius(), getTeam(), 256, 3000, false, hasModule(ModuleSensor), interceptAngle))
      return returnFloat(L, interceptAngle);
      
   return returnNil(L);
}


/**
 * @luafunc bool Robot::engineerDeployObject(EngineerBuildObject which)
 *
 * @brief Deploy the an engineered object of type `which`
 *
 * @param which The \ref EngineerBuildObjectEnum to build.
 *
 * @return `true` if the item was successfully deployed, false otherwise.
 */
S32 Robot::lua_engineerDeployObject(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "engineerDeployObject");

   S32 type = (S32)lua_tointeger(L, 0);

   return returnBool(L, getOwner()->sEngineerDeployObject(type));
}


/**
 * @luafunc void Robot::dropItem()
 *
 * @brief Drop all carried items.
 */
S32 Robot::lua_dropItem(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "dropItem");

   S32 count = mMountedItems.size();
   for(S32 i = count - 1; i >= 0; i--)
      mMountedItems[i]->dismount(DISMOUNT_NORMAL);

   return 0;
}


/**
 * @luafunc void Robot::copyMoveFromObject(BfObject obj)
 *
 * @brief Move identically to `obj`.
 *
 * @param obj The BfObject to mimic.
 */
S32 Robot::lua_copyMoveFromObject(lua_State *L)
{
   checkArgList(L, functionArgs, "Robot", "copyMoveFromObject");

   MoveObject *obj = luaW_check<MoveObject>(L, 1);

   Move move = obj->getCurrentMove();
   move.time = getCurrentMove().time;     // Keep current move time
   setCurrentMove(move);

   return 0;
}


// Override the one from BfObject
S32 Robot::lua_removeFromGame(lua_State *L)
{
   // Handle Robot removal gracefully.  Robot doesn't need to call
   // BfObject::removeFromGame() as it does all the work in its destructor
   //
   // Here we add a delay in case we were called from a Lua onShipSpawned event
   deleteObject(100);

   return 0;
}


};
