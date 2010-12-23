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

#include "tnl.h"

#include "tnlEventConnection.h"
#include "tnlBitStream.h"

namespace TNL {

//--------------------------------------------------------------------
static ClassChunker<ConnectionStringTable::PacketEntry> packetEntryFreeList(4096);

ConnectionStringTable::ConnectionStringTable(NetConnection *parent)
{
   mParent = parent;
   for(U32 i = 0; i < EntryCount; i++)
   {
      mEntryTable[i].nextHash = NULL;
      mEntryTable[i].nextLink = &mEntryTable[i+1];
      mEntryTable[i].prevLink = &mEntryTable[i-1];
      mEntryTable[i].index = i;
      mHashTable[i] = NULL;
   }
   mLRUHead.nextLink = &mEntryTable[0];
   mEntryTable[0].prevLink = &mLRUHead;
   mLRUTail.prevLink = &mEntryTable[EntryCount-1];
   mEntryTable[EntryCount-1].nextLink = &mLRUTail;
}

void ConnectionStringTable::writeStringTableEntry(BitStream *stream, StringTableEntryRef string)
{
   // see if the entry is in the hash table right now
   U32 hashIndex = string.getIndex() % EntryCount;
   Entry *sendEntry = NULL;
   for(Entry *walk = mHashTable[hashIndex]; walk; walk = walk->nextHash)
   {
      if(walk->string == string)
      {
         // it's in the table
         // first, push it to the back of the LRU list.
         pushBack(walk);
         sendEntry = walk;
         break;
      }
   }
   if(!sendEntry)
   {
      // not in the hash table, means we have to add it
      // pull the new entry from the LRU list.
      sendEntry = mLRUHead.nextLink;

      // push it to the end of the LRU list
      pushBack(sendEntry);

      // remove the string from the hash table
      Entry **hashWalk;
      for (hashWalk = &mHashTable[sendEntry->string.getIndex() % EntryCount]; *hashWalk; hashWalk = &((*hashWalk)->nextHash))
      {
         if(*hashWalk == sendEntry)
         {
            *hashWalk = sendEntry->nextHash;
            break;
         }
      }
      
      sendEntry->string = string;
      sendEntry->receiveConfirmed = false;
      sendEntry->nextHash = mHashTable[hashIndex];
      mHashTable[hashIndex] = sendEntry;
   }
   stream->writeInt(sendEntry->index, EntryBitSize);
   if(!stream->writeFlag(sendEntry->receiveConfirmed))
   {
      stream->writeString(sendEntry->string.getString());
      PacketEntry *entry = packetEntryFreeList.alloc();

      entry->stringTableEntry = sendEntry;
      entry->string = sendEntry->string;
      entry->nextInPacket = NULL;

      PacketList *note = &mParent->getCurrentWritePacketNotify()->stringList;

      if(!note->stringHead)
         note->stringHead = entry;
      else
         note->stringTail->nextInPacket = entry;
      note->stringTail = entry;
   }
}

StringTableEntry ConnectionStringTable::readStringTableEntry(BitStream *stream)
{
   U32 index = stream->readInt(EntryBitSize);

   char buf[256];
   if(!stream->readFlag())
   {
      stream->readString(buf);
      mRemoteStringTable[index].set(buf);
   }
   return mRemoteStringTable[index];
}

void ConnectionStringTable::packetReceived(PacketList *note)
{
   PacketEntry *walk = note->stringHead;
   while(walk)
   {
      PacketEntry *next = walk->nextInPacket;
      if(walk->stringTableEntry->string == walk->string)
         walk->stringTableEntry->receiveConfirmed = true;
      packetEntryFreeList.free(walk);
      walk = next;
   }
}

void ConnectionStringTable::packetDropped(PacketList *note)
{
   PacketEntry *walk = note->stringHead;
   while(walk)
   {
      PacketEntry *next = walk->nextInPacket;
      packetEntryFreeList.free(walk);
      walk = next;
   }
}

};
