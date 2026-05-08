//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "safeZone.h"

#include "game.h"
#include "ship.h"
#include "gameObjectRender.h"
#include "stringUtils.h"

namespace Zap
{

using namespace LuaArgs;

TNL_IMPLEMENT_NETOBJECT(SafeZone);


SafeZone::SafeZone(lua_State *L)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = SafeZoneTypeNumber;
   setTeam(TEAM_NEUTRAL);

   if(L)   // Coming from Lua -- grab params from L
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { POLY, TEAM_INDX, END }}, 2 };
      S32 profile = checkArgList(L, constructorArgList, "SafeZone", "constructor");

      if(profile == 1)        // Geom, team
         setGeomTeamParams(L);
   }

}


SafeZone::~SafeZone()
{
}


SafeZone *SafeZone::clone() const
{
   return new SafeZone(*this);
}


void SafeZone::render()
{
   renderSafeZone(getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle());
}


void SafeZone::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices)
{
   // Safe zones are not placeable in the editor, but existing ones should still be visible.
   renderSafeZone(getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle());
}


bool SafeZone::processArguments(S32 rawArgc, const char **rawArgv, Game *game)
{
   const S32 MAX_ARGV_SIZE = Geometry::MAX_POLY_POINTS * 2 + 1;
   S32 argc = 0;
   const char *argv[MAX_ARGV_SIZE];
   for(S32 i = 0; i < rawArgc; i++)
   {
      char c = rawArgv[i][0];
      // Ignore optional alphabetic tokens so newer level params remain backward-compatible.
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


const char *SafeZone::getOnScreenName()     { return "Safe Zone";  }
const char *SafeZone::getPrettyNamePlural() { return "Safe Zones"; }
const char *SafeZone::getOnDockName()       { return "Safe Zone";  }
const char *SafeZone::getEditorHelpString() { return "Safe zones are hand-authored in level files and are not editor-placeable."; }

bool SafeZone::hasTeam()      { return true; }
bool SafeZone::canBeHostile() { return true; }
bool SafeZone::canBeNeutral() { return true; }


bool SafeZone::canAddToEditor()
{
   return false;
}


string SafeZone::toLevelCode() const
{
   return string(appendId(getClassName())) + " " + itos(getTeam()) + " " + geomToLevelCode();
}


void SafeZone::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();
}


const Vector<Point> *SafeZone::getCollisionPoly() const
{
   return getOutline();
}


bool SafeZone::collide(BfObject *hitObject)
{
   return false;
}


bool SafeZone::protectsShip(const Ship *ship) const
{
   if(!ship)
      return false;

   const S32 zoneTeam = getTeam();
   return zoneTeam == TEAM_NEUTRAL || zoneTeam == ship->getTeam();
}


};
