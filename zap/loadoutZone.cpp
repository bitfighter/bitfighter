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

#include "loadoutZone.h"
#include "game.h"
#include "stringUtils.h"

namespace Zap
{

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

   if(!L)   // C+ constructor, use default params
      setTeam(0);

   else     // Coming from Lua -- grab params from L
   {
      S32 profile = checkArgList(L, functionArgs, "LoadoutZone", "constructor");

      if(profile == 0)              // No args constructor
         setTeam(TEAM_NEUTRAL);     

      else if(profile == 1)         // Team, Geom
      {
         setTeam(L, 1);
         setGeom(L, 2);
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


void LoadoutZone::render()
{
   renderLoadoutZone(getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle());
}


void LoadoutZone::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   render();
   PolygonObject::renderEditor(currentScale, snappingToWallCornersEnabled);
}


void LoadoutZone::renderDock()
{
  renderZone(getColor(), getOutline(), getFill());
}


// Create objects from parameters stored in level file
bool LoadoutZone::processArguments(S32 argc2, const char **argv2, Game *game)
{
   // Need to handle or ignore arguments that starts with letters,
   // so a possible future version can add parameters without compatibility problem.
   S32 argc = 0;
   const char *argv[65]; // 32 * 2 + 1 = 65
   for(S32 i = 0; i < argc2; i++)  // the idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char c = argv2[i][0];
      //switch(c)
      //{
      //case 'A': Something = atof(&argv2[i][1]); break;  // using second char to handle number
      //}
      if((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
      {
         if(argc < 65)
         {  argv[argc] = argv2[i];
            argc++;
         }
      }
   }

   if(argc < 7)
      return false;

   setTeam(atoi(argv[0]));     // Team is first arg
   return Parent::processArguments(argc - 1, argv + 1, game);
}


const char *LoadoutZone::getOnScreenName()     { return "Loadout";       }
const char *LoadoutZone::getPrettyNamePlural() { return "Loadout Zones"; }
const char *LoadoutZone::getOnDockName()       { return "Loadout";       }
const char *LoadoutZone::getEditorHelpString() { return "Area to finalize ship modifications.  Each team should have at least one."; }

bool LoadoutZone::hasTeam()      { return true; }
bool LoadoutZone::canBeHostile() { return true; }
bool LoadoutZone::canBeNeutral() { return true; }


string LoadoutZone::toString(F32 gridSize) const
{
   return string(appendId(getClassName())) + " " + itos(getTeam()) + " " + geomToString(gridSize);
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
   // Anyone can use neutral loadout zones
   if(!isGhost() &&                                                           // On the server
         (hitObject->getTeam() == getTeam() || getTeam() == TEAM_NEUTRAL) &&  // The zone is on the same team as hitObject, or it's neutral
         isShipType(hitObject->getObjectTypeNumber()))                        // The thing that hit the zone is a ship
      getGame()->getGameType()->SRV_updateShipLoadout(hitObject);      

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
GENERATE_LUA_METHODS_TABLE_NEW(LoadoutZone, LUA_METHODS);

#undef LUA_METHODS


const char *LoadoutZone::luaClassName = "LoadoutZone";
REGISTER_LUA_SUBCLASS(LoadoutZone, Zone);

};


