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

#ifndef _TNL_BITSET_H_
#define _TNL_BITSET_H_

//Includes
#ifndef _TNL_TYPES_H_
#include "tnlTypes.h"
#endif

namespace TNL {

/// Represents a word of bits.
///
/// A helper class to perform operations on a set of 32 bits.
class BitSet32
{
private:
   U32 mBits;

public:
   /// Default constructor initializes this bit set to all zeros.
   BitSet32()                         { mBits = 0; }

   /// Copy constructor.
   BitSet32(const BitSet32& in_rCopy) { mBits = in_rCopy.mBits; }

   /// Construct from an input U32.
   BitSet32(const U32 in_mask)        { mBits = in_mask; }

   /// @name Accessors
   /// @{

   /// Returns the U32 representation of the bit set.
   operator U32() const               { return mBits; }

   /// Returns the U32 representation of the bit set.
   U32 getMask() const                { return mBits; }

   /// @}

   /// @name Mutators
   ///
   /// Most of these methods take a word (ie, a BitSet32) of bits
   /// to operate with.
   /// @{

   /// Sets all the bits in the bit set to 1
   void set()                         { mBits  = 0xFFFFFFFFUL; }

   /// Sets all the bits in the bit set that are set in m.
   void set(const U32 m)              { mBits |= m; }

   /// For each bit set in s, sets or clears that bit in this, depending on whether b is true or false.
   void set(BitSet32 s, bool b)       { mBits = (mBits&~(s.mBits))|(b?s.mBits:0); }

   /// Clears all the bits in the bit set to 0.
   void clear()                       { mBits  = 0; }

   /// Clears all the bits in the bit set that are set in m.
   void clear(const U32 m)            { mBits &= ~m; }

   /// Flips all the bits in the bit set that are set in m.
   void toggle(const U32 m)           { mBits ^= m; }

   /// Test if the passed bits are set.
   bool test(const U32 m) const       { return (mBits & m) != 0; }

   /// Test if the passed bits and only the passed bits are set.
   bool testStrict(const U32 m) const { return (mBits & m) == m; }

   BitSet32& operator =(const U32 m)  { mBits  = m;  return *this; }
   BitSet32& operator|=(const U32 m)  { mBits |= m; return *this; }
   BitSet32& operator&=(const U32 m)  { mBits &= m; return *this; }
   BitSet32& operator^=(const U32 m)  { mBits ^= m; return *this; }

   BitSet32 operator|(const U32 m) const { return BitSet32(mBits | m); }
   BitSet32 operator&(const U32 m) const { return BitSet32(mBits & m); }
   BitSet32 operator^(const U32 m) const { return BitSet32(mBits ^ m); }

   /// @}
};

};

#endif //_TNL_BITSET_H_
