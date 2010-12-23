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

#ifndef _TNL_RANDOM_H_
#define _TNL_RANDOM_H_


#ifndef _TNL_H_
#include "tnl.h"
#endif

namespace TNL {

/// The Random namespace is an interface to a cryptographically secure
/// pseudo random number generator (PRNG).  Internally the Random namespace
/// uses the Yarrow PRNG algorithm.
namespace Random {

/// Adds random "seed" data to the random number generator
void addEntropy(const U8 *randomData, U32 dataLen);

/// Reads random byte data from the random number generator
void read(U8 *outBuffer, U32 randomLen);

/// Reads a 0...U32_MAX random number from the random number generator
U32 readI();

/// Reads a random number between rangeStart and rangeEnd inclusive from the random number generator
U32 readI(U32 rangeStart, U32 rangeEnd);

/// Reads a floating point value from 0 to 1 from the random number generator
F32 readF();

/// Returns a single random bit.
bool readB();

/// Returns an opaque pointer to the random number generator's internal state
/// for use in certain encryption functions.
void *getState();
};

};

#endif //_TNL_RANDOM_H_
