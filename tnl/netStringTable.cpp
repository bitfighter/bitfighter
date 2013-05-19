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
#include "tnlNetBase.h"
#include "tnlNetConnection.h"
#include "tnlNetStringTable.h"
#include "tnlDataChunker.h"
#include "tnlNetInterface.h"

namespace TNL {

namespace StringTable
{

/// @name Implementation details
/// @{

/// This is internal to the _StringTable class.
struct Node
{
   StringTableEntryId masterIndex; ///< index of the Node pointer in the master list
   StringTableEntryId nextIndex; ///< next string in this hash bucket.
   U32 hash; ///< stored hash value of this string.
   U16 stringLen; ///< length of string in this node.
   U16 refCount; ///< number of StringTableEntry's that reference this node
   char stringData[1]; ///< String data, with space for the NULL token.  Node structure is allocated as strlen + sizeof(Node);
};

enum {
   InitialHashTableSize = 1237, ///< Initial size of string hash table
   InitialNodeListSize = 2048, ///< Initial size of node id remap table
   CompactThreshold = 32768, ///< Number of string bytes freed before compaction occurs.
};

Node **mNodeList = NULL; ///< Master list of string table entry nodes
StringTableEntryId *mBuckets = NULL; ///< Hash table buckets, organized by string index

U32 mNumBuckets = 0; ///< number of hash buckets in the table
U32 mNodeListSize = 0; ///< number of elements in the node list
StringTableEntryId mNodeListFreeEntry = 0; ///< index of first free entry in the node list

U32 mItemCount = 0; ///< number of strings in the table

DataChunker *mMemPool = NULL; ///< memory pool from which string table data is allocated
U32 mFreeStringDataSize = 0; ///< number of bytes freed by deallocated strings.  When this number exceeds CompactThreshold, the table is compacted.

// a little note about the free list...
// the free list is essentially an index linked list encoded in the node
// list.  The first entry in the list is mNodeListFreeEntry.
// When the string table is compacted, we need to know which elements
// in the node list actually point to nodes, and which ones are members
// of the free list... so, what we do is, free list entries are
// actually coded with the next index shifted left 1 and or'd with a 1
// so we have the lowest bit set.  We know that pointers allocated by the
// data chunker are on at least 4-byte boundaries, so any pointer with
// the low bit set is assumed to be a free list entry.


/// Resize the StringTable to be able to hold newSize items. This 
/// is called automatically by the StringTable when the table is
/// full past a certain threshhold.
///
/// @param newSize   Number of new items to allocate space for.
void resizeHashTable(const U32 newSize);

/// compacts the string data associated with the string table.
void compact();

//---------------------------------------------------------------
//
// StringTable functions
//
//---------------------------------------------------------------

namespace {
bool sgToLowerTableInit = true;
U8   sgToLowerTable[256];

void initToLowerTable()
{
   for (U32 i = 0; i < 256; i++) {
      U8 c = dTolower(i);
      sgToLowerTable[i] = c * c;
   }

   sgToLowerTableInit = false;
}

} // namespace {}

U32 hashString(const char* str)
{
   if (sgToLowerTableInit)
      initToLowerTable();

   U32 ret = 0;
   U8 c;
   while((c = *str++) != 0) {
      ret <<= 1;
      ret ^= sgToLowerTable[c];
   }
   return ret;
}

U32 hashStringn(const char* str, S32 len)
{
   if (sgToLowerTableInit)
      initToLowerTable();

   U32 ret = 0;
   U8 c;
   while((c = *str++) != 0 && len--) {
      ret <<= 1;
      ret ^= sgToLowerTable[c];
   }
   return ret;
}

//--------------------------------------
void init()
{
   mMemPool = new DataChunker;
   mBuckets = (StringTableEntryId *) malloc(InitialHashTableSize * sizeof(StringTableEntryId));
   for(U32 i = 0; i < InitialHashTableSize; i++)
      mBuckets[i] = 0;

   mNumBuckets = InitialHashTableSize;
   mItemCount = 0;

   mNodeList = (Node **) malloc(InitialNodeListSize * sizeof(Node *));
   for(U32 i = 1; i < InitialNodeListSize; i++)
      mNodeList[i] = (Node *) (( (i + 1) << 1) | 1); // see the doco in stringTable.h for how free list entries are coded
   
   mNodeList[InitialNodeListSize - 1] = NULL;
   mNodeList[0] = (Node *) mMemPool->alloc(sizeof(Node));
   mNodeList[0]->stringData[0] = 0;
   mNodeList[0]->stringLen = 0;
   mNodeList[0]->refCount = 1;
   mNodeList[0]->masterIndex = mNodeList[0]->nextIndex = 0;
   mNodeListSize = InitialNodeListSize;

   mNodeListFreeEntry = (1 << 1) | 1;
   mFreeStringDataSize = 0;
}

void destroy()
{
   free(mBuckets);
   mBuckets = NULL;
   free(mNodeList);
   mNodeList = NULL;
   delete mMemPool;
}

//--------------------------------------

//--------------------------------------

StringTableEntryId insert(const char* val, const bool caseSens)
{
   if(!val)
      return 0;
   return insertn(val, strlen(val), caseSens);
}

//--------------------------------------
void validate()
{
   // count all the nodes in the node list:
   U32 nodeCount = 0;
   for(U32 i = 0; i < mNodeListSize; i++)
   {
      if(mNodeList[i] && !(StringTableEntryId(mNodeList[i]) & 1))        
        nodeCount++;
   }
   TNLAssert(nodeCount == mItemCount, "Error!!!");
   U32 freeListCount = 0;
   StringTableEntryId walk = mNodeListFreeEntry;
   while(walk)
   {
      walk = StringTableEntryId(mNodeList[walk >> 1]);   
      if(!((walk >> 1) < mNodeListSize))
         TNLAssert((walk >> 1) < mNodeListSize, "Out of range node index!!!");
      freeListCount++;
   }
   TNLAssert(freeListCount + nodeCount == mNodeListSize, "Error!!!!");
   // walk through all the bucket chains...
   // and make sure there are no free entries...

   for(U32 i = 0; i < mNumBuckets; i++)
   {
      StringTableEntryId walk = mBuckets[i];
      while(walk)
      {
        TNLAssert(walk < mNodeListSize, "Out of range node index!!!");
        Node *node = mNodeList[walk];
        TNLAssert((StringTableEntryId(node) & 1) == 0, "Free list entry in node chain!!!");
        TNLAssert(walk == node->masterIndex, "Master/node index mismatch.");
        walk = node->nextIndex;
      }
   }
}


StringTableEntryId insertn(const char* val, S32 len, const bool caseSens)
{
   if(!val || !*val || len == 0)
      return 0;
   if(!mBuckets)
      init();
   StringTableEntryId *walk;
   Node *stringNode;
   U32 key = hashStringn(val, len);
   walk = &mBuckets[key % mNumBuckets]; // find the bucket that the string would belong in

   // walk all the nodes in the bucket to see if the string is already in the table
   while(*walk)
   {
      stringNode = mNodeList[*walk];
      if((caseSens && !strncmp(stringNode->stringData, val, len) && stringNode->stringData[len] == 0) ||
         (!caseSens && !strnicmp(stringNode->stringData, val, len) && stringNode->stringData[len] == 0) )
      {
         // the string was found, so bump the reference count and return the node id
         stringNode->refCount++;
         return *walk;
      }
      // step to the next node in the hash bucket.
      walk = &(stringNode->nextIndex);
   }
   
   // the string was not found in the table.  So allocate a new node for the string

   // first, make sure there is a free node pointer:
   if(!mNodeListFreeEntry)
   {
      U32 oldNodeListSize = mNodeListSize;
      mNodeListSize += InitialNodeListSize;
      mNodeList = (Node **) realloc(mNodeList, mNodeListSize * sizeof(Node *));
      for(U32 i = oldNodeListSize; i < mNodeListSize; i++)
         mNodeList[i] = (Node *) (((i + 1) << 1) | 1);
      mNodeList[mNodeListSize - 1] = 0;
      mNodeListFreeEntry = (oldNodeListSize << 1) | 1;
   }
   // now allocate a new string node, and fill it in.
   stringNode = (Node *) mMemPool->alloc(sizeof(Node) + len);
   stringNode->stringLen = len;
   stringNode->refCount = 1;
   stringNode->masterIndex = mNodeListFreeEntry >> 1; // shift off the low bit flag for the free list
   stringNode->nextIndex = 0;
   stringNode->hash = key;
   *walk = stringNode->masterIndex;

   // dequeue the next free entry in the node list
   mNodeListFreeEntry = (StringTableEntryId) mNodeList[mNodeListFreeEntry >> 1];
   TNLAssert(!mNodeListFreeEntry || (mNodeListFreeEntry & 1), "Error in freeList!!");
   mNodeList[stringNode->masterIndex] = stringNode;
   
   strncpy(stringNode->stringData, val, len);
   stringNode->stringData[len] = 0;    // Null terminate
   mItemCount++;

   // Check for hash table resize
   if(mItemCount > 2 * mNumBuckets) {
      resizeHashTable(4 * mNumBuckets - 1);
   }
   return stringNode->masterIndex;
}

//--------------------------------------
StringTableEntryId lookup(const char* val, const bool  caseSens)
{
   StringTableEntryId *walk;
   Node *stringNode;
   U32 key = hashString(val);
   walk = &mBuckets[key % mNumBuckets];
   while(*walk)
   {
      stringNode = mNodeList[*walk];
      if((caseSens && !strcmp(stringNode->stringData, val)) ||
         (!caseSens && !stricmp(stringNode->stringData, val)) )
         return *walk;
      walk = &(stringNode->nextIndex);
   }
   return 0;
}

//--------------------------------------
StringTableEntryId lookupn(const char* val, S32 len, const bool  caseSens)
{
   StringTableEntryId *walk;
   Node *stringNode;
   U32 key = hashStringn(val, len);
   walk = &mBuckets[key % mNumBuckets];
   while(*walk)
   {
      stringNode = mNodeList[*walk];
      if((caseSens && !strncmp(stringNode->stringData, val, len) && stringNode->stringData[len] == 0) ||
         (!caseSens && !strnicmp(stringNode->stringData, val, len) && stringNode->stringData[len] == 0) )
         return *walk;
      walk = &(stringNode->nextIndex);
   }
   return 0; 
}

//--------------------------------------
void resizeHashTable(const U32 newSize)
{
   StringTableEntryId walk;
   StringTableEntryId head = 0;
   StringTableEntryId temp;

   U32 i;
   // reverse individual bucket lists
   // we do this because new strings are added at the end of bucket
   // lists so that case sens strings are always after their
   // corresponding case insens strings

   for(i = 0; i < mNumBuckets; i++) {
      walk = mBuckets[i];
      while(walk)
      {
         temp = mNodeList[walk]->nextIndex;
         mNodeList[walk]->nextIndex = head;
         head = walk;
         walk = temp;
      }
   }
   mBuckets = (StringTableEntryId *) realloc(mBuckets, newSize * sizeof(StringTableEntryId));
   for(i = 0; i < newSize; i++) {
      mBuckets[i] = 0;
   }
   mNumBuckets = newSize;
   walk = head;
   while(walk)
   {
      U32 key;
      Node *temp = mNodeList[walk];
      
      walk = temp->nextIndex;
      key = temp->hash;
      temp->nextIndex = mBuckets[key % newSize];
      mBuckets[key % newSize] = temp->masterIndex;
   }
}

void compact()
{
   DataChunker *newData = new DataChunker;
   for(U32 i = 1; i < mNodeListSize; i++)
   {
      Node *theNode, *newNode;
      theNode = mNodeList[i];
      
      // if the low bit is set, it's an entry in the free list.
      // if it is NULL it is the last entry (see the constructor)
      // this *may* not be the best fix, but it was a crash -pw
      if(((StringTableEntryId) theNode) & 1 || theNode == NULL)
         continue;
      newNode = (Node *) newData->alloc(sizeof(Node) + theNode->stringLen);
      newNode->stringLen = theNode->stringLen;
      newNode->refCount = theNode->refCount;
      newNode->masterIndex = theNode->masterIndex;
      newNode->nextIndex = theNode->nextIndex;
      newNode->hash = theNode->hash;
      strcpy(newNode->stringData, theNode->stringData);
      mNodeList[i] = newNode; 
   }
   delete mMemPool;
   mMemPool = newData;
   mFreeStringDataSize = 0;
}

void incRef(StringTableEntryId index)
{
    mNodeList[index]->refCount++;
}

void decRef(StringTableEntryId index)
{
   Node *theNode = mNodeList[index];
   if(--theNode->refCount)
      return;

   // remove from the hash table first:
   StringTableEntryId *walk = &mBuckets[theNode->hash % mNumBuckets];
   Node *stringNode;
   while(*walk)
   {
      stringNode = mNodeList[*walk];
      if(stringNode == theNode)
      {
         *walk = theNode->nextIndex;
         break;
      }
      walk = &(stringNode->nextIndex);
   }

   mFreeStringDataSize += mNodeList[index]->stringLen + sizeof(Node);
   mNodeList[index] = (Node *) mNodeListFreeEntry;
   mNodeListFreeEntry = (index << 1) | 1;

   if(mFreeStringDataSize > CompactThreshold)
      compact();
   mItemCount--;
   if(!mItemCount)
      destroy();
}

const char *getString(StringTableEntryId index)
{
   if(!index)
      return "";

   return mNodeList[index]->stringData;
}


};

};
