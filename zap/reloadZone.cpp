//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "reloadZone.h"

#include "game.h"
#include "ship.h"
#include "gameObjectRender.h"
#include "stringUtils.h"

namespace Zap
{

using namespace LuaArgs;

TNL_IMPLEMENT_NETOBJECT(ReloadZone);


ReloadZone::ReloadZone(lua_State *L)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = ReloadZoneTypeNumber;
   setTeam(TEAM_NEUTRAL);

   if(L)   // Coming from Lua -- grab params from L
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { POLY, TEAM_INDX, END }}, 2 };
      S32 profile = checkArgList(L, constructorArgList, "ReloadZone", "constructor");

      if(profile == 1)        // Geom, team
         setGeomTeamParams(L);
   }
}


// Constructor
ReloadZone::~ReloadZone()
{
    // Do nothing
}


ReloadZone *ReloadZone::clone() const
{
   return new ReloadZone(*this);
}


void ReloadZone::render()
{
   renderReloadZone(getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle());
}


void ReloadZone::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices)
{
   renderReloadZone(getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle());
}


bool ReloadZone::processArguments(S32 rawArgc, const char **rawArgv, Game *game)
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


const char *ReloadZone::getOnScreenName()     { return "Reload Zone";  }
const char *ReloadZone::getPrettyNamePlural() { return "Reload Zones"; }
const char *ReloadZone::getOnDockName()       { return "Reload Zone";  }
const char *ReloadZone::getEditorHelpString() { return "Reload zones are hand-authored in level files and are not editor-placeable."; }

bool ReloadZone::hasTeam()      { return true; }
bool ReloadZone::canBeHostile() { return true; }
bool ReloadZone::canBeNeutral() { return true; }


bool ReloadZone::canAddToEditor()
{
   return false;
}


string ReloadZone::toLevelCode() const
{
   return string(appendId(getClassName())) + " " + itos(getTeam()) + " " + geomToLevelCode();
}


void ReloadZone::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();
}


const Vector<Point> *ReloadZone::getCollisionPoly() const
{
   return getOutline();
}


bool ReloadZone::collide(BfObject *hitObject)
{
   return false;
}


bool ReloadZone::isActiveForShip(const Ship *ship) const
{
   if(!ship)
      return false;

   // Bitfighter ships are unaffected by reload zones.
   if(!ship->isXtankVehicle())
      return false;

   const S32 zoneTeam = getTeam();
   return zoneTeam == TEAM_NEUTRAL || zoneTeam == ship->getTeam();
}


};
