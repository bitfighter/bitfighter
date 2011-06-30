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


#include "gameObject.h"
#include "polygon.h"

namespace Zap
{

extern S32 gMaxPolygonPoints;

class SlipZone : public EditorPolygon
{
   typedef GameObject Parent;

public:
	F32 slipAmount;   // 0.0 to 1.0 , lower = more slippy

   SlipZone();       // Constructor
   SlipZone *SlipZone::clone() const;

   bool processArguments(S32 argc, const char **argv, Game *game);

   void render();
   S32 getRenderSortValue();

   void onAddedToGame(Game *theGame);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool collide(GameObject *hitObject);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   /////
   // Editor methods
   const char *getEditorHelpString() { return "Areas of higher than normal inertia."; }
   const char *getPrettyNamePlural() { return "Inertia zones"; }
   const char *getOnDockName() { return "Inertia"; }
   const char *getOnScreenName() { return "Inertia"; }
   string toString();

   void renderEditor(F32 currentScale);


   /////
   // Future home for Lua methods
   GameObject *getGameObject() { return this; }   // Return the underlying GameObject

   TNL_DECLARE_CLASS(SlipZone);
};

};
