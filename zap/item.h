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

#ifndef _ITEM_H_
#define _ITEM_H_

#include "moveObject.h"       // Parent class
#include "EditorObject.h"     // Parent class
#include "luaObject.h"        // Parent class

#include"goalZone.h"          // For GoalZone def
#include "Timer.h"

namespace Zap
{

class Ship;
class GameType;

////////////////////////////////////////
////////////////////////////////////////

class PickupItem : public Item
{
   typedef Item Parent;

private:
   bool mIsVisible;
   bool mIsMomentarilyVisible; // Used if item briefly flashes on and off, like if a ship is sitting on a repair item when it reappears
   Timer mRepopTimer;
   S32 mRepopDelay;            // Period of mRepopTimer

protected:
   enum MaskBits {
      PickupMask    = Parent::FirstFreeMask << 0,
      FirstFreeMask = Parent::FirstFreeMask << 1
   };

public:
   PickupItem(Point p = Point(), float radius = 1, S32 repopDelay = 20000);      // Constructor

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString(F32 gridSize) const;

   void idle(GameObject::IdleCallPath path);
   bool isVisible() { return mIsVisible; }

   U32 getRepopDelay() { return mRepopDelay; }


   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   bool collide(GameObject *otherObject);
   virtual bool pickup(Ship *theShip) = 0;
   virtual void onClientPickup() = 0;
};

};

#endif


