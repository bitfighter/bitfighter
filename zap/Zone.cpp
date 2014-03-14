//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Zone.h"

#include "game.h"
#include "Colors.h"
#include "GeomUtils.h"

#include "gameObjectRender.h"

namespace Zap
{

using namespace LuaArgs;

TNL_IMPLEMENT_CLASS(Zone);    // Allows classes to be autoconstructed by name


// Combined Lua / C++ constructor)
Zone::Zone(lua_State *L)   
{
   if(L)
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { POLY, END }}, 2 };
      S32 profile = checkArgList(L, constructorArgList, "Zone", "constructor");
         
      if(profile == 1)
         setGeom(L, 1);
   }

   setTeam(TEAM_NEUTRAL);
   mObjectTypeNumber = ZoneTypeNumber;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
Zone::~Zone()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


Zone *Zone::clone() const
{
   return new Zone(*this);
}


void Zone::render()
{
   // Do nothing -- zones aren't rendered in-game
}


void Zone::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   renderZone(&Colors::white, getOutline(), getFill());
   PolygonObject::renderEditor(currentScale, snappingToWallCornersEnabled);
}


void Zone::renderDock()
{
   renderZone(&Colors::white, getOutline(), getFill());
}


S32 Zone::getRenderSortValue()
{
   return -1;
}


// Create objects from parameters stored in level file
bool Zone::processArguments(S32 argc2, const char **argv2, Game *game)
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

   if(argc < 6)
      return false;

   readGeom(argc, argv, 0, game->getLegacyGridSize());
   updateExtentInDatabase();

   // Make sure our Zone doesn't have invalid geometry
   if(getExtent().getHeight() == 0 && getExtent().getWidth() == 0)
      return false;

   return true;
}


string Zone::toLevelCode() const
{
   return appendId("Zone") + " " + geomToLevelCode();
}


const char *Zone::getOnScreenName()     { return "Zone";  }
const char *Zone::getOnDockName()       { return "Zone";  }
const char *Zone::getPrettyNamePlural() { return "Zones"; }
const char *Zone::getEditorHelpString() { return "Generic area, does not appear in-game, possibly useful to scripts."; }

bool Zone::hasTeam()      { return false; }
bool Zone::canBeHostile() { return false; }
bool Zone::canBeNeutral() { return false; }


// More precise boundary for precise collision detection
const Vector<Point> *Zone::getCollisionPoly() const
{
   return getOutline();
}


// Gets called on both client and server
bool Zone::collide(BfObject *hitObject)
{
   return false;
}


/////
// Lua interface

/**
 * @luafunc Zone::Zone()
 * @luafunc Zone::Zone(geom)
 * @luaclass Zone
 * 
 * @brief Invisible objects, used mainly for generating events.
 */
//                Fn name                  Param profiles            Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS,  containsPoint,           ARRAYDEF({{ PT, END }}),        1 ) \

GENERATE_LUA_FUNARGS_TABLE(Zone, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(Zone, LUA_METHODS);

#undef LUA_METHODS

const char *Zone::luaClassName = "Zone";
REGISTER_LUA_SUBCLASS(Zone, BfObject);

/**
 * @luafunc bool Zone::containsPoint(point p)
 * 
 * @brief
 * Check whether `p` lies inside of this Zone.
 *
 * @desc
 * Determines if `p` is contained by this zone, according to the  winding
 * number algorithm. Points which lie on boundary of the polygon are
 * considered inside of it. If a polygon is self-intersecting, this method
 * will return true as long as `p` lies within some non-self-intersection
 * subpolygon.
 *
 * @param p The point to check.
 * 
 * @return `true` if `p` lies within the zone, `false` otherwise
 */
int Zone::lua_containsPoint(lua_State *L)
{
   checkArgList(L, functionArgs, "Zone", "containsPoint");

   Point pt = getPointOrXY(L, 1);
   const Vector<Point> *poly = getCollisionPoly();

   return returnBool(L, polygonContainsPoint(poly->address(), poly->size(), pt));
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
GameZone::GameZone()
{
   // Do nothing
}

// Destructor
GameZone::~GameZone()
{
   // Do nothing
}


U32 GameZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & GeomMask))
      packGeom(connection, stream);

   if(hasTeam() && stream->writeFlag(updateMask & TeamMask))
      writeThisTeam(stream);

   return 0;
}


void GameZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())                 // GeomMask
      unpackGeom(connection, stream);

   if(hasTeam() && stream->readFlag())    // TeamMask
      readThisTeam(stream);
}


};


