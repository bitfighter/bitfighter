//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "move.h"
#include "Point.h"

#include "stringUtils.h"
#include "MathUtils.h"     // For radiansToUnit() def

#include "tnlBitStream.h"

#ifdef TNL_OS_WIN32
#  include <windows.h>     // For ARRAYSIZE
#endif


namespace Zap
{

// Constructor
Move::Move()
{
   initialize();
}


Move::Move(F32 x, F32 y, F32 angle)
{
   initialize();
   set(x, y, angle);
}

// Destructor
Move::~Move()
{
   // Do nothing
}


void Move::initialize()
{
   fire = false; 
   time = 32; 
   x = 0;
   y = 0;
   angle = 0;

   for(U32 i = 0; i < ARRAYSIZE(modulePrimary); i++)
   {
      modulePrimary[i]   = false;
      moduleSecondary[i] = false;
   }
}


void Move::set(F32 x, F32 y, F32 angle)
{
   // writeFloat() only handles values from 0-1
   TNLAssert(x >= -1 && x <= 1, "x must be between -1 and 1!");
   TNLAssert(y >= -1 && y <= 1, "y must be between -1 and 1!");

   this->x = x;
   this->y = y;
   this->angle = angle;
}


bool Move::isAnyModActive() const
{
   for(U32 i = 0; i < ARRAYSIZE(modulePrimary); i++)
      if(modulePrimary[i] || moduleSecondary[i])
         return true;

   return false;
}


bool Move::isEqualMove(const Move *move) const
{
   for(U32 i = 0; i < ARRAYSIZE(modulePrimary); i++)
      if(move->modulePrimary[i] != modulePrimary[i] || move->moduleSecondary[i] != moduleSecondary[i])
         return false;

   return move->x == x &&
          move->y == y &&
          move->angle == angle &&
          move->fire == fire;
}


static const U8 MoveTimeBits = 16;
static const U8 AngleBits = 12;
static const U8 XYBits = 5;

void Move::pack(BitStream *stream, Move *prev, bool packTime)
{
   if(!stream->writeFlag(prev && isEqualMove(prev)))
   {
      stream->writeFloat(fabs(x), XYBits);
      stream->writeFlag(x < 0);
      stream->writeFloat(fabs(y), XYBits);
      stream->writeFlag(y < 0);

      // This needs to be signed, otherwise, the ship can't face up!  
      // The writeAngle here will be between -2048 and 2048 because the 'angle' is
      // always between -tau/2 and tau/2 due to the output of the various atan2() calls we make
      S32 writeAngle = (S32) floor(radiansToUnit(angle) * (1 << AngleBits) + 0.5f);     // floor(angle / 2pi * 4096 + .5)

      stream->writeSignedInt(writeAngle, AngleBits);
      stream->writeFlag(fire);

      for(S32 i = 0; i < ShipModuleCount; i++)
      {
         stream->writeFlag(modulePrimary[i]);
         stream->writeFlag(moduleSecondary[i]);
      }
   }
   if(packTime)
   {
      if(time >= U32(MaxMoveTime))
      {
         stream->writeRangedU32(127, 0, MaxMoveTime);
         stream->writeInt(time - MaxMoveTime, MoveTimeBits);   // More bits, but sent less frequently
      }
      else
         stream->writeRangedU32(time, 0, MaxMoveTime);
   }
}


void Move::unpack(BitStream *stream, bool unpackTime)
{
   if(!stream->readFlag())
   {
      x = stream->readFloat(XYBits);
      if(stream->readFlag()) 
         x = -x;

      y = stream->readFloat(XYBits);
      if(stream->readFlag()) 
         y = -y;

      // angle must output between -tau/2 and tau/2
      angle = unitToRadians(stream->readSignedInt(AngleBits) / F32(1 << AngleBits));
      fire = stream->readFlag();

      for(S32 i = 0; i < ShipModuleCount; i++)
      {
         modulePrimary[i] = stream->readFlag();
         moduleSecondary[i] = stream->readFlag();
      }
   }

   if(unpackTime)
   {
      time = stream->readRangedU32(0, MaxMoveTime);
      if(time >= U32(MaxMoveTime))
         time = stream->readInt(MoveTimeBits) + MaxMoveTime;
   }
}


// Pack and unpack the move to ensure the effects of rounding are same on client and server
void Move::prepare()    
{
   PacketStream stream;
   pack(&stream, NULL, false);
   stream.setBytePosition(0);
   unpack(&stream, false);
}


// Primarily for debugging purposes, to help visualize the moves
string Move::toString()
{
   string sep = " | ";
   string firing = fire ? "FIRE!!" : "NOFIRE";
   string modpstr, modsstr;

   Point p(x,y);

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      modpstr += modulePrimary[i]   ? "1" : "0";
      modsstr += moduleSecondary[i] ? "1" : "0";
   }

   return "(" + p.toString() + ")" + sep + ftos(angle) + sep + firing + sep + modpstr + sep + modsstr;
}


};

