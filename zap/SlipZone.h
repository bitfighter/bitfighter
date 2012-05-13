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

#ifndef _SLIPZONE_H_
#define _SLIPZONE_H_

#include "gameObject.h"
#include "polygon.h"

namespace Zap
{

class SlipZone : public PolygonObject
{
   typedef PolygonObject Parent;

public:
	F32 slipAmount;   // 0.0 to 1.0 , lower = more slippy

   SlipZone();       // Constructor
   SlipZone *clone() const;

   bool processArguments(S32 argc, const char **argv, Game *game);

   void render();
   S32 getRenderSortValue();

   void onAddedToGame(Game *theGame);
   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool collide(BfObject *hitObject);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   /////
   // Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   string toString(F32 gridSize) const;

   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);


   TNL_DECLARE_CLASS(SlipZone);
};

};
#endif
