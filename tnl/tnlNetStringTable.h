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

#ifndef _TNL_NETSTRINGTABLE_H_
#define _TNL_NETSTRINGTABLE_H_

#ifndef _TNL_TYPES_H_
#include "tnlTypes.h"
#endif

#include <string>

namespace TNL {


typedef size_t StringTableEntryId;     // size_t should be U32 on 32-bit systems, and U64 on 64 bit systems

//--------------------------------------
/// A global table for the hashing and tracking of network strings.
///
namespace StringTable
{
   /// Adds a string to the string table, and returns the id of the string.
   ///
   /// @param  string   String to check in the table (and add).
   /// @param  caseSens Determines whether case matters.
   StringTableEntryId insert(const char *string, bool caseSens = true);

   /// Adds a string to the string table, and returns the id of the string.
   ///
   /// @param  string   String to check in the table (and add).
   /// @param  len      Length of the string in bytes.
   /// @param  caseSens Determines whether case matters.
   StringTableEntryId insertn(const char *string, S32 len, bool caseSens = true);

   /// Determines if a string is in the string table, and returns the id of the string, or 0 if the string is not in the table.
   ///
   /// @param  string   String to check in the table (but not add).
   /// @param  caseSens Determines whether case matters.
   StringTableEntryId lookup(const char *string, bool caseSens = true);

   /// Determines if a string is in the string table, and returns the id of the string, or 0 if the string is not in the table.
   ///
   /// @param  string   String to check in the table (but not add).
   /// @param  len      Length of string in bytes.
   /// @param  caseSens Determines whether case matters.
   StringTableEntryId lookupn(const char *string, S32 len, bool caseSens = true);

   /// Hash a string into a U32.
   U32 hashString(const char* in_pString);

   /// Hash a string of given length into a U32.
   U32 hashStringn(const char* in_pString, S32 len);

   void incRef(StringTableEntryId index);   
   void decRef(StringTableEntryId index);
   const char *getString(StringTableEntryId index);
};

/// The StringTableEntry class encapsulates an entry in the network StringTable.
/// StringTableEntry instances offer several benefits over normal strings in
/// a networked simulation.  First, the ConnectionStringTable class is able to
/// cache string transfers, meaning that recently used strings will be transmitted
/// as string data once and then referred to by a short id bitfield thereafter.
/// String comparison operations are also much less expensive with StringTableEntry
/// instances as they result in just a single integer comparison.

class StringTableEntry {
private:
   StringTableEntryId mIndex; ///< index of the string table entry in the master pointer list
public:
    /// empty constructor gets NULL string automatically.
   inline StringTableEntry()
   { 
      mIndex = 0;
   }
   inline StringTableEntry(const char *string, bool caseSensitive = true)
   {
      mIndex = StringTable::insert(string, caseSensitive);
   }
   inline StringTableEntry(const std::string &string, bool caseSensitive = true)
   {
        mIndex = StringTable::insert(string.c_str(), caseSensitive);
   }
   inline StringTableEntry(const StringTableEntry &theString)
   {
      mIndex = theString.mIndex;
      incRef();
   }
   StringTableEntry &operator= (const StringTableEntry &s)
   {
      decRef();
      mIndex = s.mIndex;
      incRef();
      return *this;
   }
   StringTableEntry &operator= (const char *string)
   {
      decRef();
      mIndex = StringTable::insert(string);
      return *this;
   }

   inline void set(const char *string, bool caseSensitive = true)
   {
      decRef();
      mIndex = StringTable::insert(string, caseSensitive);
   }

   inline void setn(const char *string, U32 len, bool caseSensitive = true)
   {
      decRef();
      mIndex = StringTable::insertn(string, len, caseSensitive);
   }

   bool operator== (const StringTableEntry &s) const
   { 
      return s.mIndex == mIndex;
   }
   bool operator!= (const StringTableEntry &s) const
   {
      return s.mIndex != mIndex;
   }
   inline bool isNull() const    // True if string is empty ("")
   {
      return mIndex == 0;
   }
   inline bool isNotNull() const
   {
      return mIndex != 0;
   }
   inline bool isValid() const
   {
      return mIndex != 0;
   }

   operator bool () const
   {
      return mIndex != 0;
   }

   inline void decRef()
   {
      if(mIndex)
         StringTable::decRef(mIndex);
   }

   inline void incRef()
   {
      if(mIndex)
         StringTable::incRef(mIndex);
   }

   inline U64 getIndex() const 
   { 
      return mIndex;
   }
   
   inline const char *getString() const
   {
      return StringTable::getString(mIndex);
   }
};

typedef const StringTableEntry &StringTableEntryRef;

};

#endif
