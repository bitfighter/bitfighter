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

#include "Zone.h"
#include "game.h"
#include "stringUtils.h"
#include "Geometry.h"
#include "Colors.h"

namespace Zap
{

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

   readGeom(argc, argv, 0, game->getGridSize());

   updateExtentInDatabase();

   return true;
}


string Zone::toLevelCode(F32 gridSize) const
{
   return appendId("Zone") + " " + geomToLevelCode(gridSize);
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
  *  @luaconst Zone::Zone()
  *  @luaconst Zone::Zone(geom)
  *  @luaclass Zone
  *  @brief Invisible objects, used mainly for generating events.
  */
//                Fn name                  Param profiles            Profile count                           
#define LUA_METHODS(CLASS, METHOD) \

GENERATE_LUA_FUNARGS_TABLE(Zone, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(Zone, LUA_METHODS);

#undef LUA_METHODS

const char *Zone::luaClassName = "Zone";
REGISTER_LUA_SUBCLASS(Zone, BfObject);



////////////////////////////////////////
////////////////////////////////////////

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


