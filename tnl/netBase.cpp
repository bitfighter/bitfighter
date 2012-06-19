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
#include "tnlVector.h"
#include "tnlLog.h"

namespace TNL
{

#define INITIAL_CRC_VALUE 0xFFFFFFFF

NetClassRep *NetClassRep::mClassLinkList = NULL;
U32 NetClassRep::mNetClassBitSize[NetClassGroupCount][NetClassTypeCount] = {{0, },};
Vector<NetClassRep *> NetClassRep::mClassTable[NetClassGroupCount][NetClassTypeCount];
U32 NetClassRep::mClassCRC[NetClassGroupCount] = {INITIAL_CRC_VALUE, };

bool NetClassRep::mInitialized = false;

NetClassRep::NetClassRep()
{
   mInitialUpdateCount = 0;
   mInitialUpdateBitsUsed = 0;
   mPartialUpdateCount = 0;
   mPartialUpdateBitsUsed = 0;
}

Object* NetClassRep::create(const char* className)
{
   TNLAssert(mInitialized, "creating an object before NetClassRep::initialize.");

   for (NetClassRep *walk = mClassLinkList; walk; walk = walk->mNextClass)
      if (!strcmp(walk->getClassName(), className))
         return walk->create();

   //TNLAssertV(0, ("Couldn't find class rep for dynamic class: %s", className));   // Bad level line, typically not fatal in production

   return NULL;
}

//--------------------------------------
Object* NetClassRep::create(const U32 groupId, const U32 typeId, const U32 classId)
{
   TNLAssert(mInitialized, "creating an object before NetClassRep::initialize.");
   TNLAssert(classId < U32(mClassTable[groupId][typeId].size()), "Class id out of range.");
   TNLAssert(mClassTable[groupId][typeId][classId] != NULL, "No class with declared id type.");

   if(mClassTable[groupId][typeId][classId])
      return mClassTable[groupId][typeId][classId]->create();

   return NULL;
}

//--------------------------------------

// Sort by class version first, then by class name
static S32 QSORT_CALLBACK ACRCompare(const void *aptr, const void *bptr)
{
   const NetClassRep *a = *((const NetClassRep **) aptr);
   const NetClassRep *b = *((const NetClassRep **) bptr);

   if(a->getClassVersion() != b->getClassVersion())
      return a->getClassVersion() - b->getClassVersion();
   return strcmp(a->getClassName(), b->getClassName());
}


// These used only for logging purposes below
const char *NetClassGroupNames[] = { "Game Group", "Community Group", "Master Group", "Unused Group" };
const char *NetClassTypeNames[] = { "Object Type", "Data Type", "Event Type" };

void NetClassRep::initialize()
{
   if(mInitialized)
      return;

   Vector<NetClassRep *> dynamicTable;
      
   NetClassRep *walk;
   
   for(U32 group = 0; group < NetClassGroupCount; group++)
   {
      U32 groupMask = 1 << group;
      for(U32 type = 0; type < NetClassTypeCount; type++)
      {
         for(walk = mClassLinkList; walk; walk = walk->mNextClass)
         {
            if(walk->getClassType() == (NetClassType)type && walk->mClassGroupMask & groupMask)
               dynamicTable.push_back(walk);
         }
         if(dynamicTable.size() == 0)
            continue;

         // Sort by class version then class name, so at least everyone agrees on the order of the classes
         qsort((void *) &dynamicTable[0], dynamicTable.size(), sizeof(NetClassRep *), ACRCompare);

         logprintf(LogConsumer::LogNetBase, "Class Group: %d (%s) Class Type: %d (%s)  Count: %d", 
                                            group, NetClassGroupNames[group], type, NetClassTypeNames[type], dynamicTable.size());

         for(S32 i = 0; i < dynamicTable.size(); i++)
            logprintf(LogConsumer::LogNetBase, "\tClass %d: %s", i + 1, dynamicTable[i]->getClassName());

         mClassTable[group][type] = dynamicTable;
   
         for(S32 i = 0; i < mClassTable[group][type].size(); i++)
            mClassTable[group][type][i]->mClassId[group] = i;

         mNetClassBitSize[group][type] = 
               getBinLog2(getNextPow2(mClassTable[group][type].size() + 1));
         dynamicTable.clear();
      }
   }
   mInitialized = true;
}


// Only called on exit
void NetClassRep::logBitUsage()
{
   logprintf(LogConsumer::LogNetBase, "Net Class Bit Usage (i.e. how much data did we transmit?):");

   bool atLeastOne = false;

   for(NetClassRep *walk = mClassLinkList; walk; walk = walk->mNextClass)
   {
      if(walk->mInitialUpdateCount)
      {
         logprintf(LogConsumer::LogNetBase, "%s (Initialized) - Count: %d   Total: %d   Avg Size: %g", 
               walk->mClassName, walk->mInitialUpdateCount, walk->mInitialUpdateBitsUsed, 
               walk->mInitialUpdateBitsUsed / F32(walk->mInitialUpdateCount));
         atLeastOne = true;
      }

      if(walk->mPartialUpdateCount)
      {
         logprintf(LogConsumer::LogNetBase, "%s (Updated) - Count: %d   Total: %d   Avg Size: %g", 
               walk->mClassName, walk->mPartialUpdateCount, walk->mPartialUpdateBitsUsed, 
               walk->mPartialUpdateBitsUsed / F32(walk->mPartialUpdateCount));
         atLeastOne = true;
      }
   }

   if(!atLeastOne)
      logprintf(LogConsumer::LogNetBase, "No net class data was transmitted");
}




SafePtrData::~SafePtrData()
{
   SafeObjectRef *walk = mFirstObjectRef;
   while(walk)
   {
      SafeObjectRef *next = walk->mNextObjectRef;
      walk->mObject = NULL;
      walk->mPrevObjectRef = NULL;
      walk->mNextObjectRef = NULL;
      walk = next;
   }
}

void RefPtrData::destroySelf()
{
   delete this;
}

RefPtrData::~RefPtrData()
{
   TNLAssert(mRefCount == 0, "Error! Object deleted with non-zero reference count!");
}

//--------------------------------------
NetClassRep* Object::getClassRep() const
{
   return NULL;
}

};
