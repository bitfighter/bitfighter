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

#ifndef _ZONE_H_
#define _ZONE_H_

#include "gameObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "gameObjectRender.h"
#include "polygon.h"

namespace Zap
{

class Zone : public PolygonObject
{
   typedef PolygonObject Parent;

public:
   Zone();              // C++ constructor
   virtual ~Zone();     // Destructor
   Zone *clone() const;

   virtual void render();
   S32 getRenderSortValue();
   virtual bool processArguments(S32 argc, const char **argv, Game *game);

   virtual bool getCollisionPoly(Vector<Point> &polyPoints) const;     // More precise boundary for precise collision detection
   virtual bool collide(BfObject *hitObject);

   /////
   // Editor methods
   virtual const char *getEditorHelpString();
   virtual const char *getPrettyNamePlural();
   virtual const char *getOnDockName();
   virtual const char *getOnScreenName();

   bool hasTeam();      
   bool canBeHostile(); 
   bool canBeNeutral(); 

   virtual string toString(F32 gridSize) const;

   virtual void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   virtual void renderDock();

   //// Lua interface
   LUAW_DECLARE_CLASS(Zone);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


////////////////////////////////////////
////////////////////////////////////////

// Extends above with some methods related to client/server interaction; Zone itself is server-only
class GameZone : public Zone
{
   typedef Zone Parent;

public:
   virtual U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   virtual void unpackUpdate(GhostConnection *connection, BitStream *stream);
};


};


#endif
