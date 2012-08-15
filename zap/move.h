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

#include "Point.h"
#include "../tnl/tnlLog.h"
#include "../tnl/tnlTypes.h"
#include "../tnl/tnlBitStream.h"


using namespace TNL;

const S32 ShipModuleCount = 2; // TODO: Get rid of these  (including ship.h makes things go crazy, extern doesnt seem to work)
const S32 ShipWeaponCount = 3;

namespace Zap
{

// Some angle conversion functions:

const F32 radiansToDegreesConversion = 360.0f * FloatInverse2Pi;
const F32 degreesToRadiansConversion = 1 / radiansToDegreesConversion;

inline F32 radiansToDegrees(F32 angle) { return angle * radiansToDegreesConversion; }
inline F32 degreesToRadians(F32 angle) { return angle * degreesToRadiansConversion; }
inline F32 radiansToUnit(F32 angle)    { return angle * FloatInverse2Pi; }
inline F32 unitToRadians(F32 angle)    { return angle * Float2Pi; }


// Can represent a move by a human player or a robot
class Move 
{
public:
   Move();                          // Constructor

   F32 x, y;
   F32 angle;
   bool fire;
   bool moduleActive[ShipModuleCount];    // Is given module active?
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


