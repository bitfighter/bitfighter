//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _RELOADZONE_H_
#define _RELOADZONE_H_

#include "Zone.h"

namespace Zap
{

class Ship;

// ReloadZone: an ammo-refill area for xtank vehicles.  Ships that are Bitfighter
// ships are unaffected.  Xtank vehicles have their weapon refillTimers activated
// (and ammo replenished) while inside a friendly or neutral ReloadZone.
// Mirrors the SafeZone implementation.
class ReloadZone : public GameZone
{
   typedef GameZone Parent;

public:
   explicit ReloadZone(lua_State *L = NULL);
   virtual ~ReloadZone();

   ReloadZone *clone() const;

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

   // Returns true when this zone is active for the given ship (i.e. neutral or same team).
   bool isActiveForShip(const Ship *ship) const;

   TNL_DECLARE_CLASS(ReloadZone);
};

};

#endif
