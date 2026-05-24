//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "fuelZone.h"

#include "game.h"
#include "ship.h"
#include "gameObjectRender.h"
#include "stringUtils.h"

namespace Zap
{

using namespace LuaArgs;

TNL_IMPLEMENT_NETOBJECT(FuelZone);


FuelZone::FuelZone(lua_State *L)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = FuelZoneTypeNumber;
   setTeam(TEAM_NEUTRAL);

   if(L)   // Coming from Lua -- grab params from L
   {
	  static LuaFunctionArgList constructorArgList = { {{ END }, { POLY, TEAM_INDX, END }}, 2 };
	  S32 profile = checkArgList(L, constructorArgList, "FuelZone", "constructor");

	  if(profile == 1)        // Geom, team
		 setGeomTeamParams(L);
   }
}


FuelZone::~FuelZone()
{
   // Do nothing
}


FuelZone *FuelZone::clone() const
{
   return new FuelZone(*this);
}


void FuelZone::render()
{
   renderFuelZone(getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle());
}


void FuelZone::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices)
{
   renderFuelZone(getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle());
}


bool FuelZone::processArguments(S32 rawArgc, const char **rawArgv, Game *game)
{
   const S32 MAX_ARGV_SIZE = Geometry::MAX_POLY_POINTS * 2 + 1;
   S32 argc = 0;
   const char *argv[MAX_ARGV_SIZE];
   for(S32 i = 0; i < rawArgc; i++)
   {
	  char c = rawArgv[i][0];
	  // Ignore optional alphabetic tokens for backward-compatibility.
	  if((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
	  {
		 if(argc < MAX_ARGV_SIZE)
		 {
			argv[argc] = rawArgv[i];
			argc++;
		 }
	  }
   }

   if(argc < 7)
	  return false;

   setTeam(atoi(argv[0]));     // Team is first arg
   return Parent::processArguments(argc - 1, argv + 1, game);
}


const char *FuelZone::getOnScreenName()     { return "Fuel Zone";  }
const char *FuelZone::getPrettyNamePlural() { return "Fuel Zones"; }
const char *FuelZone::getOnDockName()       { return "Fuel Zone";  }
const char *FuelZone::getEditorHelpString() { return "Fuel zones are hand-authored in level files and are not editor-placeable."; }

bool FuelZone::hasTeam()      { return true; }
bool FuelZone::canBeHostile() { return true; }
bool FuelZone::canBeNeutral() { return true; }


bool FuelZone::canAddToEditor()
{
   return false;
}


string FuelZone::toLevelCode() const
{
   return string(appendId(getClassName())) + " " + itos(getTeam()) + " " + geomToLevelCode();
}


void FuelZone::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
	  setScopeAlways();
}


const Vector<Point> *FuelZone::getCollisionPoly() const
{
   return getOutline();
}


bool FuelZone::collide(BfObject *hitObject)
{
   return false;
}


bool FuelZone::isActiveForShip(const Ship *ship) const
{
   if(!ship)
	  return false;

   // Bitfighter ships are unaffected by fuel zones.
   if(!ship->isXtankVehicle())
	  return false;

   const S32 zoneTeam = getTeam();
   return zoneTeam == TEAM_NEUTRAL || zoneTeam == ship->getTeam();
}


};
