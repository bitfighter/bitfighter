//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _FUELZONE_H_
#define _FUELZONE_H_

#include "Zone.h"

namespace Zap
{

class Ship;

// FuelZone: a fuel-refill area for xtank vehicles.  Bitfighter ships are unaffected.
// While stopped and centered on the zone, xtank vehicles gain fuel at 1 unit/frame,
// paying fuel_cost money per unit (from the engine's XtankEngineInfo::cost field).
// Mirrors the ReloadZone implementation.
class FuelZone : public GameZone
{
   typedef GameZone Parent;

public:
   explicit FuelZone(lua_State *L = NULL);
   virtual ~FuelZone();

   FuelZone *clone() const;

   void render();
   bool processArguments(S32 argc, const char **argv, Game *game);
   void onAddedToGame(Game *theGame);

   const Vector<Point> *getCollisionPoly() const;
   bool collide(BfObject *hitObject);

   bool canAddToEditor();
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices = false);

   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();

   string toLevelCode() const;

   // Returns true when this zone is active for the given ship (neutral or same team, xtank only).
   bool isActiveForShip(const Ship *ship) const;

   TNL_DECLARE_CLASS(FuelZone);
};

};

#endif
