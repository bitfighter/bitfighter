//-----------------------------------------------------------------------------------
//
//   Torque Network Library
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "../libtomcrypt/mycrypt.h"
#include "tnl.h"
#include "tnlRandom.h"
#include "tnlJournal.h"

namespace TNL {

namespace Random {

static bool initialized = false;
static prng_state prng;
static U32 entropyAdded = 0;

static void initialize()
{
   initialized = true;
   yarrow_start(&prng);
   yarrow_ready(&prng);
}

void *getState()                                      // Doesn't seem to be called
{
   if(!initialized)
      initialize();

   return &prng;
}


// Needs at least 16 bytes of entropy to be effective.  Can call repeated times to accumulate entropy.
void addEntropy(const U8 *randomData, U32 dataLen)    
{
   if(!initialized)
      initialize();
   yarrow_add_entropy(randomData, dataLen, &prng);
   entropyAdded += dataLen;
   
   if(entropyAdded >= 16)
   {
      yarrow_ready(&prng);
      entropyAdded = 0;
   }
}

void read(U8 *outBuffer, U32 randomLen)
{
   if(!initialized)
      initialize();

   yarrow_read(outBuffer, randomLen, &prng);
}

U32 readI()
{
   U8 randomBuffer[4];
   read(randomBuffer, 4);
   U32 result = (U32(randomBuffer[0]) << 24) | (U32(randomBuffer[1]) << 16) |
                (U32(randomBuffer[2]) << 8 ) | U32(randomBuffer[3]);
   return result;
}

U32 readI(U32 rangeStart, U32 rangeEnd)
{
   if(rangeStart == rangeEnd)
      return rangeStart;

   TNLAssert(rangeStart <= rangeEnd, "Random::readI - invalid range.");
   if(rangeStart > rangeEnd)
      rangeEnd = rangeStart + 1;

   return (readI() % (rangeEnd - rangeStart + 1)) + rangeStart;
}

F32 readF()
{
   return F32 ( F64(readI()) / F64(U32_MAX) );
}

bool readB()
{
   U8 randomBuffer;
   read(&randomBuffer, 1);

   return(randomBuffer & 1);
}
}; 

};
