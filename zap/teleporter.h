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

#include "gameConnection.h"
#include "gameObject.h"
#include "projectile.h"   // For LuaItem
#include "point.h"    

#include "../tnl/tnlNetObject.h"

namespace Zap
{

class Teleporter : public GameObject, public LuaItem
{
private:
   S32 mLastDest;    // Destination of last ship through
   Point mPos;
   Vector<Point> mDest;

public:
   bool doSplash;
   U32 timeout;
   U32 mTime;

   static const S32 TeleporterRadius = 75;       // Overall size of the teleporter


   enum {
      InitMask     = BIT(0),
      TeleportMask = BIT(1),

      TeleporterTriggerRadius = 50,
      TeleporterDelay = 1500,             // Time teleporter remains idle after it has been used
      TeleporterExpandTime = 1350,
      TeleportInExpandTime = 750,
      TeleportInRadius = 120,
   };

   Teleporter();     // Constructor
   bool processArguments(S32 argc, const char **argv);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void idle(GameObject::IdleCallPath path);
   void render();

   void onAddedToGame(Game *theGame);

   Teleporter findTeleporterAt(Point pos);      // Find a teleporter at pos

   TNL_DECLARE_CLASS(Teleporter);

   ///// Lua Interface

   Teleporter(lua_State *L);             //  Lua constructor
   static const char className[];
   static Lunar<Teleporter>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, TeleportType); }   // Object's class    
   void push(lua_State *L) { Lunar<Teleporter>::push(L, this); }         // Push item onto stack

   S32 getLoc(lua_State *L) { return returnPoint(L, mPos); }                     // Center of item (returns point)
   S32 getRad(lua_State *L) { return returnInt(L, TeleporterTriggerRadius); }    // Radius of item (returns number)
   S32 getVel(lua_State *L) { return returnPoint(L, Point(0,0)); }               // Speed of item (returns point)
   S32 getTeamIndx(lua_State *L) { return returnInt(L, Item::NEUTRAL_TEAM + 1); }    // All teleporters are neutral
   GameObject *getGameObject() { return this; }                                  // Return the underlying GameObject
};

};

