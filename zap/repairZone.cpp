//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "repairZone.h"

#include "game.h"
#include "ship.h"
#include "gameObjectRender.h"
#include "stringUtils.h"

namespace Zap
{

using namespace LuaArgs;

TNL_IMPLEMENT_NETOBJECT(RepairZone);


RepairZone::RepairZone(lua_State *L)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = RepairZoneTypeNumber;
   setTeam(TEAM_NEUTRAL);

   if(L)   // Coming from Lua -- grab params from L
   {
	  static LuaFunctionArgList constructorArgList = { {{ END }, { POLY, TEAM_INDX, END }}, 2 };
	  S32 profile = checkArgList(L, constructorArgList, "RepairZone", "constructor");

	  if(profile == 1)        // Geom, team
		 setGeomTeamParams(L);
   }
}


RepairZone::~RepairZone()
{
   // Do nothing
}


RepairZone *RepairZone::clone() const
{
   return new RepairZone(*this);
}


void RepairZone::render()
{
   renderRepairZone(getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle());
}


void RepairZone::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices)
{
   renderRepairZone(getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle());
}


bool RepairZone::processArguments(S32 rawArgc, const char **rawArgv, Game *game)
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


const char *RepairZone::getOnScreenName()     { return "Repair Zone";  }
const char *RepairZone::getPrettyNamePlural() { return "Repair Zones"; }
const char *RepairZone::getOnDockName()       { return "Repair Zone";  }
const char *RepairZone::getEditorHelpString() { return "Repair zones are hand-authored in level files and are not editor-placeable."; }

bool RepairZone::hasTeam()      { return true; }
bool RepairZone::canBeHostile() { return true; }
bool RepairZone::canBeNeutral() { return true; }


bool RepairZone::canAddToEditor()
{
   return false;
}


string RepairZone::toLevelCode() const
{
   return string(appendId(getClassName())) + " " + itos(getTeam()) + " " + geomToLevelCode();
}


void RepairZone::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
	  setScopeAlways();
}


const Vector<Point> *RepairZone::getCollisionPoly() const
{
   return getOutline();
}


bool RepairZone::collide(BfObject *hitObject)
{
   return false;
}


bool RepairZone::isActiveForShip(const Ship *ship) const
{
   if(!ship)
	  return false;

   // Bitfighter ships are unaffected by repair zones.
   if(!ship->isXtankVehicle())
	  return false;

   const S32 zoneTeam = getTeam();
   return zoneTeam == TEAM_NEUTRAL || zoneTeam == ship->getTeam();
}


};
