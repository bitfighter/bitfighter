//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _REPAIRZONE_H_
#define _REPAIRZONE_H_

#include "Zone.h"

namespace Zap
{

class Ship;

// RepairZone: an armor-repair area for xtank vehicles.  Bitfighter ships are unaffected.
// While stopped and centered on the zone, xtank vehicles gain +1 armor HP per side every
// 150 ms (3 xtank frames at 20 fps), paying armor_cost * body_size money per point.
// Mirrors the FuelZone / ReloadZone implementation.
class RepairZone : public GameZone
{
   typedef GameZone Parent;

public:
   explicit RepairZone(lua_State *L = NULL);
   virtual ~RepairZone();

   RepairZone *clone() const;

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

   TNL_DECLARE_CLASS(RepairZone);
};

};

#endif
