//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Spawn.h"

#include "game.h"
#include "Level.h"

#include "stringUtils.h"         // For itos()
#include "gameObjectRender.h"    // For renderSquareItem(), renderFlag(), drawCircle()
#include "moveObject.h"          // For Circle, Asteroid class defs

#include "gameConnection.h"
#include "tnlRandom.h"

namespace Zap
{

using namespace LuaArgs;

// Statics:
#ifndef ZAP_DEDICATED
   EditorAttributeMenuUI *AbstractSpawn::mAttributeMenuUI = NULL;
#endif

// TODO: Move all time related stuff down to ItemSpawn

// Time is in seconds
AbstractSpawn::AbstractSpawn(const Point &pos, S32 time)
{
   setPos(pos);
   setRespawnTime(time);
};


AbstractSpawn::AbstractSpawn(const AbstractSpawn &copy) : PointObject(copy), mTimer(copy.mTimer)
{
   mSpawnTime = copy.mSpawnTime;
}


AbstractSpawn::~AbstractSpawn()
{
   // Do nothing
}


void AbstractSpawn::setRespawnTime(S32 time)       // in seconds
{
   mSpawnTime = time;
   mTimer.reset(time * 1000);
}


// In game radius -- these are never displayed or interacted with; radius doesn't matter
F32 AbstractSpawn::getRadius() const
{
   return 1;      
}


F32 AbstractSpawn::getEditorRadius(F32 currentScale) const
{
   return 12;     // Constant size, regardless of zoom
}


// Looking for <x> <y> {spawn-time}
bool AbstractSpawn::processArguments(S32 argc, const char **argv, Level *level)
{
   if(argc < 2)
      return false;

   Point pos;
   pos.read(argv);
   pos *= level->getLegacyGridSize();

   setPos(pos);

   S32 time = (argc > 2) ? atoi(argv[2]) : getDefaultRespawnTime();

   setRespawnTime(time);

   updateExtentInDatabase();

   return true;
}


string AbstractSpawn::toLevelCode() const
{
   // <<spawn class name>> <x> <y> <spawn timer>
   return string(appendId(getClassName())) + " " + geomToLevelCode() + " " + itos(mSpawnTime);
}


void AbstractSpawn::setSpawnTime(S32 spawnTime)
{
   mSpawnTime = spawnTime;
}


S32 AbstractSpawn::getSpawnTime() const
{
   return mSpawnTime;
}


bool AbstractSpawn::updateTimer(U32 deltaT)
{
   return mTimer.update(deltaT);
}


void AbstractSpawn::resetTimer()
{
   mTimer.reset();
}


U32 AbstractSpawn::getPeriod()
{
   return mTimer.getPeriod();
}


// Render some attributes when item is selected but not being edited
void AbstractSpawn::fillAttributesVectors(Vector<string> &keys, Vector<string> &values)
{
   if(getDefaultRespawnTime() == -1)
      return;

   keys.push_back("Spawn time:");

   if(mSpawnTime == 0)
      values.push_back("Will not spawn");
   else
      values.push_back(itos(mSpawnTime) + " sec" + ( mSpawnTime != 1 ? "s" : ""));
}


////////////////////////////////////////
////////////////////////////////////////

// Standard ship spawn point

TNL_IMPLEMENT_CLASS(Spawn);

// C++ constructor
Spawn::Spawn(const Point &pos) : AbstractSpawn(pos)
{
   initialize();
}

/**
 * @luafunc Spawn::Spawn()
 * @luafunc Spawn::Spawn(geom)
 * @luafunc Spawn::Spawn(geom, team)
 */
// Lua constructor
Spawn::Spawn(lua_State *L) : AbstractSpawn(Point(0,0))
{
   initialize();
   if(L)
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { PT, END }, { PT, TEAM_INDX, END }}, 3 };
      S32 profile = checkArgList(L, constructorArgList, "Spawn", "constructor");

      if(profile == 1)
         setPos(L, 1);

      else if(profile == 2)
      {
         setPos(L, 1);
         setTeam(L, 2);
      }
   }
}


void Spawn::initialize()
{
   mObjectTypeNumber = ShipSpawnTypeNumber;
   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
Spawn::~Spawn()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


Spawn *Spawn::clone() const
{
   return new Spawn(*this);
}


// Spawn <team> <x> <y>
bool Spawn::processArguments(S32 argc, const char **argv, Level *level)
{
   if(argc < 3)
      return false;

   S32 teamIndex = atoi(argv[0]);
   setTeam(teamIndex);

   Parent::processArguments(argc - 1, argv + 1, level);

   return true;
}


string Spawn::toLevelCode() const
{
   // Spawn <team> <x> <y> 
   return string(appendId(getClassName())) + " " + itos(getTeam()) + " " + geomToLevelCode();
}


const char *Spawn::getOnScreenName()      const { return "Spawn";        }
const char *Spawn::getOnDockName()        const { return "Spawn";        }
const char *Spawn::getPrettyNamePlural()  const { return "Spawn Points"; }
const char *Spawn::getEditorHelpString()  const { return "Location where ships start.  At least one per team is required. [G]"; }

const char *Spawn::getClassName() const  { return "Spawn"; }


S32 Spawn::getDefaultRespawnTime()
{
   return -1;
}


void Spawn::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices) const
{
#ifndef ZAP_DEDICATED
   renderSpawn(getPos(), 1/currentScale, getColor());
#endif
}


void Spawn::renderDock(const Color &color) const
{
   renderEditor(1, false);
}


/////
// Lua interface
/**
 * @luaclass Spawn
 * 
 * @brief Marks locations where ships and robots should spawn.
 * 
 * @geom The geometry of Spawns is a single point.
 */
//               Fn name    Param profiles         Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_METHODS_TABLE(Spawn, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(Spawn, LUA_METHODS);

#undef LUA_METHODS


const char *Spawn::luaClassName = "Spawn";
REGISTER_LUA_SUBCLASS(Spawn, BfObject);


////////////////////////////////////////
////////////////////////////////////////

// Constructor
ItemSpawn::ItemSpawn(const Point &pos, S32 time) : Parent(pos, time)
{
   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
ItemSpawn::~ItemSpawn()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


// This is primarily here to keep this class from being abstract... but we'll use it for some trivial code deduplication purposes since we have to have it
// Children classes will add the spawned item, and call this.
void ItemSpawn::spawn()
{
   resetTimer();     // Reset the spawn timer
}


void ItemSpawn::onAddedToGame(Game *game)
{
   Parent::onAddedToGame(game);
}


void ItemSpawn::idle(IdleCallPath path)
{
   bool triggered = mTimer.update(mCurrentMove.time);

   // Only spawn on server
   if(triggered && path == BfObject::ServerIdleMainLoop)
      spawn();
}


// These methods exist solely to make ItemSpawn instantiable so it can be instantiated by Lua... even though it never will
const char *ItemSpawn::getClassName() const                                       { TNLAssert(false, "Not implemented!"); return ""; }
S32 ItemSpawn::getDefaultRespawnTime()                                            { TNLAssert(false, "Not implemented!"); return 0;  }
void ItemSpawn::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices) const { TNLAssert(false, "Not implemented!"); }
void ItemSpawn::renderDock(const Color &color) const                              { TNLAssert(false, "Not implemented!"); }


/////
// Lua interface
/**
 * @luaclass ItemSpawn
 *
 * @brief Class of Spawns that emit various objects at various times.
 *
 * @geom The geometry of all ItemSpawns is a single point.
 */
//               Fn name    Param profiles         Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getSpawnTime, ARRAYDEF({{          END }}), 1 ) \
   METHOD(CLASS, setSpawnTime, ARRAYDEF({{ NUM_GE0, END }}), 1 ) \
   METHOD(CLASS, spawnNow,     ARRAYDEF({{          END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(ItemSpawn, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(ItemSpawn, LUA_METHODS);

#undef LUA_METHODS


const char *ItemSpawn::luaClassName = "ItemSpawn";
REGISTER_LUA_SUBCLASS(ItemSpawn, BfObject);


/**
 * @luafunc num ItemSpawn::getSpawnTime()
 *
 * @brief Get the interval between item emissions, in seconds.
 *
 * @return The spawn time in seconds.
 */
S32 ItemSpawn::lua_getSpawnTime(lua_State *L)
{
   return returnFloat(L, mSpawnTime / 1000.0f);
}


/**
 * @luafunc ItemSpawn::setSpawnTime(num seconds)
 * 
 * @brief Sets time between item emission events, in seconds.
 * 
 * @descr Note that setting the spawn time also resets the timer, so that the
 * next item will be spawned after time seconds.
 * 
 * @param seconds Spawn time in seconds.
 */
S32 ItemSpawn::lua_setSpawnTime(lua_State *L)
{
   checkArgList(L, functionArgs, "ItemSpawn", "setSpawnTime");

   setRespawnTime(getInt(L, 1));

   return 0;
}


/**
 * @luafunc ItemSpawn::spawnNow()
 * 
 * @brief Force the ItemSpawn to spawn an item immediately.
 * 
 * @descr This method also resets the spawn timer.
 */
S32 ItemSpawn::lua_spawnNow(lua_State *L)
{
   if(!getGame())
   {
      const char *msg = "Cannot spawn item before spawn has been added to game!";
      logprintf(LogConsumer::LogError, msg);
      throw LuaException(msg);
   }

   spawn();
   return 0;
}


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(AsteroidSpawn);

// Constructor
AsteroidSpawn::AsteroidSpawn(const Point &pos, S32 time) : Parent(pos, time)
{
   initialize();
}

/**
 * @luafunc AsteroidSpawn::AsteroidSpawn()
 * @luafunc AsteroidSpawn::AsteroidSpawn(geom)
 * @luafunc AsteroidSpawn::AsteroidSpawn(geom, time)
 */
AsteroidSpawn::AsteroidSpawn(lua_State *L) : Parent(Point(0,0), DEFAULT_RESPAWN_TIME)
{
   initialize();
   
   if(L)
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { PT, END }, { PT, NUM, END }}, 3 };
      S32 profile = checkArgList(L, constructorArgList, "AsteroidSpawn", "constructor");

      if(profile >= 1)
         setPos(L, 1);

      if(profile == 2)
         setRespawnTime(getInt(L, 2));
   }
}


// Destructor
AsteroidSpawn::~AsteroidSpawn()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


void AsteroidSpawn::onGhostAvailable(GhostConnection *theConnection)
{
   s2cSetTimeUntilSpawn(mTimer.getCurrent());
}


void AsteroidSpawn::initialize()
{
   mNetFlags.set(Ghostable);   // So we can render on the client
   mObjectTypeNumber = AsteroidSpawnTypeNumber;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


AsteroidSpawn *AsteroidSpawn::clone() const
{
   return new AsteroidSpawn(*this);
}


const char *AsteroidSpawn::getOnScreenName()     const  { return "AsteroidSpawn";         }
const char *AsteroidSpawn::getOnDockName()       const  { return "ASP";                   }
const char *AsteroidSpawn::getPrettyNamePlural() const  { return "Asteroid Spawn Points"; }
const char *AsteroidSpawn::getEditorHelpString() const  { return "Periodically spawns a new asteroid."; }

const char *AsteroidSpawn::getClassName() const  { return "AsteroidSpawn"; }

S32 AsteroidSpawn::getDefaultRespawnTime()
{
   return DEFAULT_RESPAWN_TIME;
}


void AsteroidSpawn::setRespawnTime(S32 spawnTime)
{
   Parent::setRespawnTime(spawnTime);

   // let clients know about the new spawn time
   if(!isGhost())
      s2cSetTimeUntilSpawn(mTimer.getCurrent());
}


void AsteroidSpawn::spawn()
{
   Parent::spawn();

   Game *game = getGame();

   Asteroid *asteroid = new Asteroid();   // Create a new asteroid

   F32 ang = TNL::Random::readF() * Float2Pi;

   asteroid->setPosAng(getPos(), ang);

   asteroid->addToGame(game, game->getGameObjDatabase());              // And add it to the list of game objects
   s2cSetTimeUntilSpawn(mTimer.getCurrent());
}


U32 AsteroidSpawn::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   //U32 retMask = Parent::packUpdate(connection, updateMask, stream);  // Goes to empty function NetObject::packUpdate

   if(stream->writeFlag(updateMask & InitialMask))
      ((GameConnection *) connection)->writeCompressedPoint(getPos(), stream);

   return 0; // retMask;
}


void AsteroidSpawn::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())  // InitialMask
   {
      Point pos;
      ((GameConnection *) connection)->readCompressedPoint(pos, stream);

      setPos(pos);      // Also sets object extent
   }
}


// Used for rendering in-game
void AsteroidSpawn::renderLayer(S32 layerIndex)
{
#ifndef ZAP_DEDICATED
   if(layerIndex != -1)
      return;

   renderAsteroidSpawn(getPos(), mTimer.getCurrent());
#endif
}


void AsteroidSpawn::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices) const
{
#ifndef ZAP_DEDICATED
   renderAsteroidSpawnEditor(getPos(), 1/currentScale);
#endif
}


void AsteroidSpawn::renderDock(const Color &color) const
{
#ifndef ZAP_DEDICATED
   renderAsteroidSpawnEditor(getPos());
#endif
}


TNL_IMPLEMENT_NETOBJECT_RPC(AsteroidSpawn, s2cSetTimeUntilSpawn, (S32 millis), (millis),
                            NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   mTimer.reset(millis);
}

/////
// Lua interface
/**
 * @luaclass AsteroidSpawn
 * 
 * @brief Spawns \link Asteroid Asteroids \endlink at regular intervals.
 * 
 * @geom The geometry of AsteroidSpawns is a single point.
 */
//               Fn name    Param profiles         Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_METHODS_TABLE(AsteroidSpawn, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(AsteroidSpawn, LUA_METHODS);

#undef LUA_METHODS


const char *AsteroidSpawn::luaClassName = "AsteroidSpawn";
REGISTER_LUA_SUBCLASS(AsteroidSpawn, ItemSpawn);


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_CLASS(FlagSpawn);

// C++ constructor
FlagSpawn::FlagSpawn(const Point &pos, S32 time, S32 teamIndex) : Parent(pos, time)
{
   initialize();

   if(teamIndex != TeamNotSpecified)
      setTeam(teamIndex);
}


// Lua constructor
FlagSpawn::FlagSpawn(lua_State *L) : Parent(Point(0,0), DEFAULT_RESPAWN_TIME)
{
   initialize();

   if(L)
   {
      //                                  Profile:         0          1                 2                          3
      static LuaFunctionArgList constructorArgList = { {{ END }, { PT, END }, { PT, TEAM_INDX, END }, { PT, TEAM_INDX, NUM, END }}, 4 };
      
      S32 profile = checkArgList(L, constructorArgList, "FlagSpawn", "constructor");

      if(profile >= 1)
         setPos(L, 1);

      if(profile >= 2)
         setTeam(L, 2);

      if(profile == 3)      
         setRespawnTime(getInt(L, 3));
   }
}


// Destructor
FlagSpawn::~FlagSpawn()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


void FlagSpawn::initialize()
{
   mObjectTypeNumber = FlagSpawnTypeNumber;
   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// FlagSpawn <team> <x> <y> {spawn time}
bool FlagSpawn::processArguments(S32 argc, const char **argv, Level *level)
{
   if(argc < 3)
      return false;

   setTeam(atoi(argv[0]));
   
   return Parent::processArguments(argc - 1, argv + 1, level);    // Read the rest of the args
}


string FlagSpawn::toLevelCode() const
{
   // FlagSpawn <team> <x> <y> <spawn timer for nexus> -- Need to insert the team into the string we get from AbstractSpawn()
   string str1 = Parent::toLevelCode();
   std::size_t firstarg = str1.find(' ');
   return str1.substr(0, firstarg) + " " + itos(getTeam()) + str1.substr(firstarg);
}


FlagSpawn *FlagSpawn::clone() const
{
   return new FlagSpawn(*this);
}


void FlagSpawn::spawn()
{
   Parent::spawn();                    // Resets timer
   getGame()->releaseFlag(getPos());   // Asserts if game is not Nexus
}


bool FlagSpawn::updateTimer(S32 deltaT)
{
   return mTimer.update(deltaT);
}


void FlagSpawn::resetTimer()
{
   mTimer.reset();
}


const char *FlagSpawn::getOnScreenName()     const  { return "FlagSpawn";         }
const char *FlagSpawn::getOnDockName()       const  { return "FlagSpawn";         }
const char *FlagSpawn::getPrettyNamePlural() const  { return "Flag Spawn points"; }
const char *FlagSpawn::getEditorHelpString() const  { return "Location where flags (or balls in Soccer) spawn after capture."; }

const char *FlagSpawn::getClassName() const  { return "FlagSpawn"; }


S32 FlagSpawn::getDefaultRespawnTime()
{
   return DEFAULT_RESPAWN_TIME;
}


void FlagSpawn::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices) const
{
#ifndef ZAP_DEDICATED
   Point pos = getPos();
   renderFlagSpawn(pos, currentScale, getColor());
#endif
}


void FlagSpawn::renderDock(const Color &color) const
{
   renderEditor(1, false);
}


/////
// Lua interface
/**
 * @luaclass FlagSpawn
 * 
 * @brief Spawns \link FlagItem Flags \endlink at regular intervals during Nexus
 * games, serves as starting point for flags and soccer balls in other games.
 * 
 * @descr During Nexus games, FlagSpawn acts like any other ItemSpawn, emitting
 * flags at regular intervals. During games that use flags (such as
 * ZoneControl), FlagSpawns mark locations where flags can be returned to when
 * flags are "sent home". In Soccer games, marks the location that the \link
 * SoccerBallItem SoccerBall \endlink is returned to after a goal is scored.
 * 
 * Note that in flag games, any place a flag starts will become a FlagSpawn, and
 * in Soccer, the location the SoccerBall starts will become a FlagSpawn.
 * 
 * @geom The geometry of FlagSpawns is a single point.
 */
//               Fn name    Param profiles         Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_METHODS_TABLE(FlagSpawn, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(FlagSpawn, LUA_METHODS);

#undef LUA_METHODS


const char *FlagSpawn::luaClassName = "FlagSpawn";
REGISTER_LUA_SUBCLASS(FlagSpawn, ItemSpawn);


};    // namespace
