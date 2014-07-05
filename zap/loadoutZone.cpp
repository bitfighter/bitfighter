//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "loadoutZone.h"

#include "Level.h"
#include "game.h"

#include "gameObjectRender.h"
#include "stringUtils.h"

namespace Zap
{

using namespace LuaArgs;

TNL_IMPLEMENT_NETOBJECT(LoadoutZone);

/**
*  @luafunc LoadoutZone::LoadoutZone()
*  @luafunc LoadoutZone::LoadoutZone(team, geom)
*  @brief %LoadoutZone constructor.
*  @descr Default team is Neutral.
*/

// Combined Lua / C++ constructor
LoadoutZone::LoadoutZone(lua_State *L)    
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = LoadoutZoneTypeNumber;
   setTeam(TEAM_NEUTRAL); 

   if(L)   // Coming from Lua -- grab params from L
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { POLY, TEAM_INDX, END }}, 2 };
      S32 profile = checkArgList(L, constructorArgList, "LoadoutZone", "constructor");

      if(profile == 1)         // Geom, team
      {
         setGeom(L,  1);
         setTeam(L, -1);
      }
   }

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
LoadoutZone::~LoadoutZone()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


LoadoutZone *LoadoutZone::clone() const
{
   return new LoadoutZone(*this);
}


void LoadoutZone::render() const
{
   renderLoadoutZone(getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle());
}


void LoadoutZone::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices) const
{
   render();
   PolygonObject::renderEditor(currentScale, snappingToWallCornersEnabled);
}


void LoadoutZone::renderDock(const Color &color) const
{
   renderZone(color, getOutline(), getFill());
}


// Create objects from parameters stored in level file
bool LoadoutZone::processArguments(S32 argc2, const char **argv2, Level *level)
{
   // Need to handle or ignore arguments that starts with letters,
   // so a possible future version can add parameters without compatibility problem.
   S32 argc = 0;
   const char *argv[Geometry::MAX_POLY_POINTS * 2 + 1];
   for(S32 i = 0; i < argc2; i++)  // the idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char c = argv2[i][0];
      //switch(c)
      //{
      //case 'A': Something = atof(&argv2[i][1]); break;  // using second char to handle number
      //}
      if((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
      {
         if(argc < Geometry::MAX_POLY_POINTS * 2 + 1)
         {  argv[argc] = argv2[i];
            argc++;
         }
      }
   }

   if(argc < 7)
      return false;

   setTeam(atoi(argv[0]));     // Team is first arg
   return Parent::processArguments(argc - 1, argv + 1, level);
}


const char *LoadoutZone::getOnScreenName()     const  { return "Loadout";       }
const char *LoadoutZone::getPrettyNamePlural() const  { return "Loadout Zones"; }
const char *LoadoutZone::getOnDockName()       const  { return "Loadout";       }
const char *LoadoutZone::getEditorHelpString() const  { return "Area to finalize ship modifications.  Each team should have at least one."; }

bool LoadoutZone::hasTeam()      { return true; }
bool LoadoutZone::canBeHostile() { return true; }
bool LoadoutZone::canBeNeutral() { return true; }


string LoadoutZone::toLevelCode() const
{
   return string(appendId(getClassName())) + " " + itos(getTeam()) + " " + geomToLevelCode();
}


void LoadoutZone::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();
}


// More precise boundary for precise collision detection
const Vector<Point> *LoadoutZone::getCollisionPoly() const
{
   return getOutline();
}


// Gets called on both client and server
bool LoadoutZone::collide(BfObject *hitObject)
{
   // This is probably a useless assert in the long-term, but tests are crashing here due to refactor blues
   TNLAssert(getGame(), "Expect a game here!");

   // Anyone can use neutral loadout zones
   if(!isGhost() &&                                                           // On the server
         (hitObject->getTeam() == getTeam() || getTeam() == TEAM_NEUTRAL) &&  // The zone is on the same team as hitObject, or it's neutral
         isShipType(hitObject->getObjectTypeNumber()))                        // The thing that hit the zone is a ship
      getGame()->updateShipLoadout(hitObject);

   return false;
}


U32 LoadoutZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   return Parent::packUpdate(connection, updateMask, stream);
}


void LoadoutZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);
}


/////
// Lua interface

/**
  *  @luaclass LoadoutZone
  *  @brief Provides place for players to change ship configuration.
  */
//                Fn name                  Param profiles            Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_FUNARGS_TABLE(LoadoutZone, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(LoadoutZone, LUA_METHODS);

#undef LUA_METHODS


const char *LoadoutZone::luaClassName = "LoadoutZone";
REGISTER_LUA_SUBCLASS(LoadoutZone, Zone);

};


