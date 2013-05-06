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

#ifndef _MOVE_H_
#define _MOVE_H_

#include "shipItems.h"     // For ShipModuleCount, ShipWeaponCount

namespace TNL {
   class BitStream;
}

using namespace TNL;

namespace Zap
{

// Can represent a move by a human player or a robot
class Move 
{
public:
   Move();                          // Constructor

   F32 x, y;
   F32 angle;
   bool fire;
   bool modulePrimary[ShipModuleCount];    // Is given module primary component active?
   bool moduleSecondary[ShipModuleCount];  // Is given module secondary component active?
   U32 time;

   enum {
      MaxMoveTime = 127,
   };

   bool isAnyModActive() const;
   bool isEqualMove(Move *prev);    // Compares this move to the previous one -- are they the same?
   void pack(BitStream *stream, Move *prev, bool packTime);
   void unpack(BitStream *stream, bool unpackTime);
   void prepare();                  // Packs and unpacks move to ensure effects of rounding are same on client and server
};

};

#endif


