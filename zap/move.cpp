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

#include "move.h"
#include <math.h>

namespace Zap
{

// Constructor
Move::Move()
{
   fire = false; time = 32;
   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
      module[i] = false;
}

bool Move::isEqualMove(Move *prev)
{
   bool modsUnchanged = true;

   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
      modsUnchanged = modsUnchanged && (prev->module[i] == module[i]);

   return   prev->x == x &&
            prev->y == y &&
            prev->angle == angle &&
            prev->fire == fire &&
            modsUnchanged;
}

void Move::pack(BitStream *stream, Move *prev, bool packTime)
{
   if(!stream->writeFlag(prev && isEqualMove(prev)))
      {
      stream->writeFloat(fabs(x), 5);
      stream->writeFlag(x < 0);
      stream->writeFloat(fabs(y), 5);
      stream->writeFlag(y < 0);

      //RDW This needs to be signed.
      //Otherwise, the ship can't face up!
      S32 writeAngle = S32(radiansToUnit(angle) * 0x1000);
      //RDW Should this be writeSignedInt?
      //The BitStream header leads me to believe it should.
      stream->writeInt(writeAngle, 12);
      stream->writeFlag(fire);

      for(U32 i = 0; i < (U32)ShipModuleCount; i++)
         stream->writeFlag(module[i]);
   }
   if(packTime)
      stream->writeRangedU32(time, 0, MaxMoveTime);
}

void Move::unpack(BitStream *stream, bool unpackTime)
{
   if(!stream->readFlag())
   {
      x = stream->readFloat(5);
      if(stream->readFlag()) x = -x;
      y = stream->readFloat(5);
      if(stream->readFlag()) y = -y;

      angle = unitToRadians(stream->readInt(12) / F32(0x1000));
      fire = stream->readFlag();
      for(U32 i = 0; i < (U32)ShipModuleCount; i++)
         module[i] = stream->readFlag();
   }

   if(unpackTime)
      time = stream->readRangedU32(0, MaxMoveTime);
}

void Move::prepare()    // Packs and unpacks move to ensure effects of rounding are same on client and server
{
   PacketStream stream;
   pack(&stream, NULL, false);
   stream.setBytePosition(0);
   unpack(&stream, false);
}

};

