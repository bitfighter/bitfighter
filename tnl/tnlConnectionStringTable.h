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

#ifndef _TNL_CONNECTIONSTRINGTABLE_H_
#define _TNL_CONNECTIONSTRINGTABLE_H_

#ifndef _TNL_NETSTRINGTABLE_H_
#include "tnlNetStringTable.h"
#endif

namespace TNL {

class NetConnection;
class BitStream;

/// ConnectionStringTable is a helper class to EventConnection for reducing duplicated string data sends
class ConnectionStringTable
{
public:
   enum StringTableConstants{
      EntryCount = 1024,
      EntryBitSize = 10,
   };

   struct Entry; 

   struct PacketEntry {
      PacketEntry *nextInPacket; ///< The next string table entry updated in the packet this is linked in.
      Entry *stringTableEntry; ///< The ConnectionStringTable::Entry this refers to
      StringTableEntry string; ///< The StringTableEntry that was set in that string
   };

public:
   struct PacketList {
      PacketEntry *stringHead;  ///< The head of the linked list of strings sent in this packet.
      PacketEntry *stringTail; ///< The tail of the linked list of strings sent in this packet.

      PacketList() { stringHead = stringTail = NULL; }
   };

   /// An entry in the EventConnection's string table
   struct Entry {
      StringTableEntry string; ///< Global string table entry of this string
                           ///< will be 0 if this string is unused.
      U32 index;           ///< Index of this entry.
      Entry *nextHash;     ///< The next hash entry for this id.
      Entry *nextLink;     ///< The next entry in the LRU list.
      Entry *prevLink;     ///< The prev entry in the LRU list.

      /// Does the other side now have this string?
      bool receiveConfirmed;
   };

private:
   Entry mEntryTable[EntryCount];
   Entry *mHashTable[EntryCount];
   StringTableEntry mRemoteStringTable[EntryCount];
   Entry mLRUHead, mLRUTail;

   NetConnection *mParent;

   /// Pushes an entry to the back of the LRU list.
   inline void pushBack(Entry *entry)
   {
      entry->prevLink->nextLink = entry->nextLink;
      entry->nextLink->prevLink = entry->prevLink;
      entry->nextLink = &mLRUTail;
      entry->prevLink = mLRUTail.prevLink;
      entry->nextLink->prevLink = entry;
      entry->prevLink->nextLink = entry;
   }
public:
   ConnectionStringTable(NetConnection *parent);

   void writeStringTableEntry(BitStream *stream, StringTableEntryRef string);
   StringTableEntry readStringTableEntry(BitStream *stream);

   void packetReceived(PacketList *note);
   void packetDropped(PacketList *note);
};

};

#endif

