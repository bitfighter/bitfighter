//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SAFEZONE_H_
#define _SAFEZONE_H_

#include "Zone.h"

namespace Zap
{

class Ship;

class SafeZone : public GameZone
{
   typedef GameZone Parent;

public:
   explicit SafeZone(lua_State *L = NULL);
   virtual ~SafeZone();

   SafeZone *clone() const;

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

   bool protectsShip(const Ship *ship) const;

   TNL_DECLARE_CLASS(SafeZone);
};

};

#endif
