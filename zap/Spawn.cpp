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

#ifndef ZAP_DEDICATED
#  include "OpenglUtils.h"
#  include "UIMenuItems.h"
#  include "UIEditorMenus.h"
#  include "ClientGame.h"
#endif

#include "Spawn.h"
#include "game.h"
#include "gameType.h"
#include "NexusGame.h"           // For FlagSpawn::spawn()

#include "stringUtils.h"         // For itos()
#include "gameObjectRender.h"    // For renderSquareItem(), renderFlag(), drawCircle()
#include "moveObject.h"          // For Circle, Asteroid class defs


namespace Zap
{

// Statics:
#ifndef ZAP_DEDICATED
   EditorAttributeMenuUI *AbstractSpawn::mAttributeMenuUI = NULL;
#endif

// TODO: Move all time related stuff down to ItemSpawn

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


void AbstractSpawn::setRespawnTime(F32 time)       // in seconds
{
   mSpawnTime = time;
   mTimer.reset(time * 1000);
}


// In game radius -- these are never displayed or interacted with; radius doesn't matter
F32 AbstractSpawn::getRadius()
{
   return 1;      
}


F32 AbstractSpawn::getEditorRadius(F32 currentScale)
{
   return 12;     // Constant size, regardless of zoom
}


// Looking for <x> <y> {spawn-time}
bool AbstractSpawn::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 2)
      return false;

   Point pos;
   pos.read(argv);
   pos *= game->getGridSize();

   setPos(pos);

   S32 time = (argc > 2) ? atoi(argv[2]) : getDefaultRespawnTime();

   setRespawnTime(time);

   updateExtentInDatabase();

   return true;
}


string AbstractSpawn::toLevelCode(F32 gridSize) const
{
   // <<spawn class name>> <x> <y> <spawn timer>
   return string(appendId(getClassName())) + " " + geomToLevelCode(gridSize) + " " + itos(mSpawnTime);
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


#ifndef ZAP_DEDICATED

EditorAttributeMenuUI *AbstractSpawn::getAttributeMenu()
{
   if(getDefaultRespawnTime() == -1)  // No editing RespawnTimer for Ship Spawn
      return NULL;

   if(!mAttributeMenuUI)
   {
      ClientGame *clientGame = static_cast<ClientGame *>(getGame());

      mAttributeMenuUI = new EditorAttributeMenuUI(clientGame);

      CounterMenuItem *menuItem = new CounterMenuItem("Spawn Timer:", 999, 1, 0, 1000, "secs", "Never spawns", 
                                                      "Time it takes for each item to be spawned");
      mAttributeMenuUI->addMenuItem(menuItem);

      // Add our standard save and exit option to the menu
      mAttributeMenuUI->addSaveAndQuitMenuItem();
   }

   return mAttributeMenuUI;
}


void AbstractSpawn::startEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   attributeMenu->getMenuItem(0)->setIntValue(mSpawnTime);
}


void AbstractSpawn::doneEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   mSpawnTime = attributeMenu->getMenuItem(0)->getIntValue();
}


// Render some attributes when item is selected but not being edited
string AbstractSpawn::getAttributeString()
{

   if(getDefaultRespawnTime() == -1)
      return "";

   if(mSpawnTime == 0)
      return "Disabled";
   else
      return "Spawn time: " + itos(mSpawnTime) + " sec" + ( mSpawnTime != 1 ? "s" : "");
}

#endif

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
  *  @luaconst Spawn::Spawn()
  *  @luaconst Spawn::Spawn(geom)
  *  @luaconst Spawn::Spawn(geom, team)
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
bool Spawn::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 3)
      return false;

   S32 teamIndex = atoi(argv[0]);
   setTeam(teamIndex);

   Parent::processArguments(argc - 1, argv + 1, game);

   return true;
}


string Spawn::toLevelCode(F32 gridSize) const
{
   // Spawn <team> <x> <y> 
   return string(appendId(getClassName())) + " " + itos(getTeam()) + " " + geomToLevelCode(gridSize);
}


const char *Spawn::getOnScreenName()     { return "Spawn";        }
const char *Spawn::getOnDockName()       { return "Spawn";        }
const char *Spawn::getPrettyNamePlural() { return "Spawn Points"; }
const char *Spawn::getEditorHelpString() { return "Location where ships start.  At least one per team is required. [G]"; }

const char *Spawn::getClassName() const  { return "Spawn"; }


S32 Spawn::getDefaultRespawnTime()
{
   return -1;
}


void Spawn::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
#ifndef ZAP_DEDICATED
   Point pos = getPos();

   glPushMatrix();
      glTranslatef(pos.x, pos.y, 0);
      glScalef(1/currentScale, 1/currentScale, 1);    // Make item draw at constant size, regardless of zoom
      renderSquareItem(Point(0,0), getColor(), 1, &Colors::white, 'S');
   glPopMatrix();
#endif
}


void Spawn::renderDock()
{
   renderEditor(1, false);
}


/////
// Lua interface
/**
  *  @luaclass Spawn
  *  @brief Marks locations where ships and robots should spawn.
  *  @geom  The geometry of Spawns is a single point.
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
   linkToIdleList(&game->idlingObjects);
}

void ItemSpawn::idle(IdleCallPath path)
{
   // Only on server
   if(path != BfObject::ServerIdleMainLoop)
      return;

   if(mTimer.update(mCurrentMove.time))
      spawn();
}


// These methods exist solely to make ItemSpawn instantiable so it can be instantiated by Lua... even though it never will
const char *ItemSpawn::getClassName() const                                       { TNLAssert(false, "Not implemented!"); return ""; }
S32 ItemSpawn::getDefaultRespawnTime()                                            { TNLAssert(false, "Not implemented!");  return 0; }
void ItemSpawn::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled) { TNLAssert(false, "Not implemented!"); }
void ItemSpawn::renderDock()                                                      { TNLAssert(false, "Not implemented!"); }


/////
// Lua interface
/**
  *  @luaclass ItemSpawn
  *  @brief Class of Spawns that emit various objects at various times.
  *  @geom  The geometry of all ItemSpawns is a single point.
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
  *  @luafunc time ItemSpawn::getSpawnTime()
  *  @brief   Gets the item's spawn time.
  *  @return   \e time: Number representing spawn time in seconds.
  */
S32 ItemSpawn::lua_getSpawnTime(lua_State *L)
{
   return returnFloat(L, mSpawnTime / 1000.0f);
}


/**
  *  @luafunc ItemSpawn::setSpawnTime(time)
  *  @brief   Sets time between item emission events.
  *  @descr   Note that setting the spawn time also resets the timer, so that the next item will be spawned after \e time seconds.
  *  @param   \e time: Number representing spawn time in seconds.
  */
S32 ItemSpawn::lua_setSpawnTime(lua_State *L)
{
   checkArgList(L, functionArgs, "ItemSpawn", "setSpawnTime");

   setRespawnTime(getFloat(L, 1));

   return 0;
}


/**
  *  @luafunc ItemSpawn::spawnNow()
  *  @brief   Tell %ItemSpawn to spawn an item immediately.
  *  @descr   This method also resets the spawn timer.
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

// TODO:  change to TNL_IMPLEMENT_NETOBJECT for next version of Bitfighter that breaks compatibility
TNL_IMPLEMENT_CLASS(AsteroidSpawn);

// Constructor
AsteroidSpawn::AsteroidSpawn(const Point &pos, S32 time) : Parent(pos, time)
{
   initialize();
}

/**
  *  @luaconst AsteroidSpawn::AsteroidSpawn()
  *  @luaconst AsteroidSpawn::AsteroidSpawn(geom)
  *  @luaconst AsteroidSpawn::AsteroidSpawn(geom, time)
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
         setRespawnTime(getFloat(L, 2));
   }
}


// Destructor
AsteroidSpawn::~AsteroidSpawn()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


void AsteroidSpawn::initialize()
{
   // Uncomment for asteroid spawn on the client when we go to 019
//   mNetFlags.set(Ghostable);   // So we can render on the client
   mObjectTypeNumber = AsteroidSpawnTypeNumber;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


AsteroidSpawn *AsteroidSpawn::clone() const
{
   return new AsteroidSpawn(*this);
}


const char *AsteroidSpawn::getOnScreenName()     { return "AsteroidSpawn";         }
const char *AsteroidSpawn::getOnDockName()       { return "ASP";                   }
const char *AsteroidSpawn::getPrettyNamePlural() { return "Asteroid Spawn Points"; }
const char *AsteroidSpawn::getEditorHelpString() { return "Periodically spawns a new asteroid."; }

const char *AsteroidSpawn::getClassName() const  { return "AsteroidSpawn"; }

S32 AsteroidSpawn::getDefaultRespawnTime()
{
   return DEFAULT_RESPAWN_TIME;
}


void AsteroidSpawn::spawn()
{
   Parent::spawn();

   Game *game = getGame();

   Asteroid *asteroid = new Asteroid();   // Create a new asteroid

   F32 ang = TNL::Random::readF() * Float2Pi;

   asteroid->setPosAng(getPos(), ang);

   asteroid->addToGame(game, game->getGameObjDatabase());              // And add it to the list of game objects
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
void AsteroidSpawn::render(S32 layerIndex)
{
#ifndef ZAP_DEDICATED
   if(layerIndex != -1)
      return;

   GameType *gameType = getGame()->getGameType();

   S32 time = gameType->getRemainingGameTimeInMs() + gameType->getRenderingOffset();

   renderAsteroidSpawn(getPos(), time);
#endif
}


void AsteroidSpawn::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
#ifndef ZAP_DEDICATED
   Point pos = getPos();

   glPushMatrix();
      glTranslate(pos);
      glScale(1/currentScale);    // Make item draw at constant size, regardless of zoom
      renderAsteroidSpawnEditor(Point(0,0));
   glPopMatrix();
#endif
}


void AsteroidSpawn::renderDock()
{
#ifndef ZAP_DEDICATED
   renderAsteroidSpawnEditor(getPos());
#endif
}


/////
// Lua interface
/**
  *  @luaclass AsteroidSpawn
  *  @brief Spawns \link Asteroid Asteroids \endlink at regular intervals.
  *  @geom  The geometry of AsteroidSpawns is a single point.
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

TNL_IMPLEMENT_CLASS(CircleSpawn);

// C++ constructor
CircleSpawn::CircleSpawn(const Point &pos, S32 time) : Parent(pos, time)
{
   initialize();
}


// Lua constructor
/**
  *  @luaconst CircleSpawn::CircleSpawn()
  *  @luaconst CircleSpawn::CircleSpawn(geom)
  *  @luaconst CircleSpawn::CircleSpawn(geom, time)
  */
CircleSpawn::CircleSpawn(lua_State *L) : Parent(Point(0,0), DEFAULT_RESPAWN_TIME)
{
   initialize();
   
   if(L)
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { PT, END }, { PT, NUM, END }}, 3 };
      S32 profile = checkArgList(L, constructorArgList, "CircleSpawn", "constructor");

      if(profile >= 1)
         setPos(L, 1);

      if(profile == 2)
         setRespawnTime(getFloat(L, 2));
   }
}


// Destructor
CircleSpawn::~CircleSpawn()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


void CircleSpawn::initialize()
{
   mObjectTypeNumber = CircleSpawnTypeNumber;
   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


CircleSpawn *CircleSpawn::clone() const
{
   return new CircleSpawn(*this);
}


const char *CircleSpawn::getOnScreenName()     { return "CircleSpawn";         }
const char *CircleSpawn::getOnDockName()       { return "CSP";                 }
const char *CircleSpawn::getPrettyNamePlural() { return "Circle Spawn Points"; }
const char *CircleSpawn::getEditorHelpString() { return "Periodically spawns a new circle."; }

const char *CircleSpawn::getClassName() const  { return "CircleSpawn"; }


S32 CircleSpawn::getDefaultRespawnTime()
{
   return DEFAULT_RESPAWN_TIME;
}


void CircleSpawn::spawn()
{
   Parent::spawn();

   Game *game = getGame();

   for(S32 i = 0; i < 10; i++)
   {
      Circle *circle = new Circle();   // Create a new Circle
      F32 ang = TNL::Random::readF() * FloatTau;

      circle->setPosAng(getPos(), ang);

      circle->addToGame(game, game->getGameObjDatabase());     // And add it to the list of game objects
   }
}


static void renderCircleSpawn(const Point &pos)
{
#ifndef ZAP_DEDICATED
   F32 scale = 0.8f;
   static const Point p(0,0);

   glPushMatrix();
      glTranslatef(pos.x, pos.y, 0);
      glScalef(scale, scale, 1);
      drawCircle(p, 8);

      glColor(Colors::white);
      drawCircle(p, 13);
   glPopMatrix();  
#endif
}


void CircleSpawn::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
#ifndef ZAP_DEDICATED
   Point pos = getPos();

   glPushMatrix();
      glTranslate(pos);
      glScale(1 / currentScale);    // Make item draw at constant size, regardless of zoom
      renderCircleSpawn(Point(0,0));
   glPopMatrix();   
#endif
}


void CircleSpawn::renderDock()
{
   renderCircleSpawn(getPos());
}


/////
// Lua interface
/**
  *  @luaconst CircleSpawn::CircleSpawn()
  *  @luaconst CircleSpawn::CircleSpawn(geom)
  *  @luaconst CircleSpawn::CircleSpawn(geom, time)
  *  @luaclass CircleSpawn
  *  @brief Spawns \link Circle Circles \endlink at regular intervals.
  *  @geom  The geometry of CircleSpawns is a single point.
  */
//               Fn name    Param profiles         Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_METHODS_TABLE(CircleSpawn, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(CircleSpawn, LUA_METHODS);

#undef LUA_METHODS


const char *CircleSpawn::luaClassName = "CircleSpawn";
REGISTER_LUA_SUBCLASS(CircleSpawn, ItemSpawn);

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
         setRespawnTime(getFloat(L, 3));
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
bool FlagSpawn::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 3)
      return false;

   setTeam(atoi(argv[0]));
   
   return Parent::processArguments(argc - 1, argv + 1, game);     // then read the rest of the args
}


string FlagSpawn::toLevelCode(F32 gridSize) const
{
   // FlagSpawn <team> <x> <y> <spawn timer for nexus> -- Need to insert the team into the string we get from AbstractSpawn()
   string str1 = Parent::toLevelCode(gridSize);
   std::size_t firstarg = str1.find(' ');
   return str1.substr(0, firstarg) + " " + itos(getTeam()) + str1.substr(firstarg);
}


FlagSpawn *FlagSpawn::clone() const
{
   return new FlagSpawn(*this);
}


void FlagSpawn::spawn()
{
   Parent::spawn();     // Resets timer

   if(getGame()->getGameType()->getGameTypeId() == NexusGame)
      NexusGameType::releaseFlag(getGame(), getPos());
}


bool FlagSpawn::updateTimer(S32 deltaT)
{
   return mTimer.update(deltaT);
}


void FlagSpawn::resetTimer()
{
   mTimer.reset();
}


const char *FlagSpawn::getOnScreenName()     { return "FlagSpawn";         }
const char *FlagSpawn::getOnDockName()       { return "FlagSpawn";         }
const char *FlagSpawn::getPrettyNamePlural() { return "Flag Spawn points"; }
const char *FlagSpawn::getEditorHelpString() { return "Location where flags (or balls in Soccer) spawn after capture."; }

const char *FlagSpawn::getClassName() const  { return "FlagSpawn"; }


S32 FlagSpawn::getDefaultRespawnTime()
{
   return DEFAULT_RESPAWN_TIME;
}


void FlagSpawn::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
#ifndef ZAP_DEDICATED
   Point pos = getPos();

   glPushMatrix();
      glTranslatef(pos.x + 1, pos.y, 0);
      glScalef(0.4f / currentScale, 0.4f / currentScale, 1);
      Color color = *getColor();  // To avoid taking address of temporary
      renderFlag(0, 0, &color);

      glColor(Colors::white);
      drawCircle(-4, 0, 26);
   glPopMatrix();
#endif
}


void FlagSpawn::renderDock()
{
   renderEditor(1, false);
}


/////
// Lua interface
/**
  *  @luaclass FlagSpawn
  *  @brief Spawns \link FlagItem Flags \endlink at regular intervals during %Nexus games, serves as starting point for flags and soccer balls in other games.
  *  @descr During Nexus games, %FlagSpawn acts like any other ItemSpawn, emitting flags at regular intervals.  During games that use flags (such as ZoneControl),
  *         FlagSpawns mark locations where flags can be returned to when flags are "sent home".  In Soccer games, marks the location that the 
  *         \link SoccerBallItem SoccerBall \endlink is returned to after a goal is scored.

  *  Note that in flag games, any place a flag starts will become a %FlagSpawn, and in Soccer, the location the SoccerBall starts will become a %FlagSpawn.
  *  @geom  The geometry of FlagSpawns is a single point.
  */
//               Fn name    Param profiles         Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_METHODS_TABLE(FlagSpawn, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(FlagSpawn, LUA_METHODS);

#undef LUA_METHODS


const char *FlagSpawn::luaClassName = "FlagSpawn";
REGISTER_LUA_SUBCLASS(FlagSpawn, ItemSpawn);


};    // namespace
