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
#include "tnlNetObject.h"
#include "tnlGhostConnection.h"
#include "tnlNetInterface.h"

namespace TNL {

GhostConnection *NetObject::mRPCSourceConnection = NULL;
GhostConnection *NetObject::mRPCDestConnection = NULL;
bool NetObject::mIsInitialUpdate = false;

NetObject::NetObject()
{
   // netFlags will clear itself to 0
   mNetIndex = U32(-1);
   mFirstObjectRef = NULL;
   mPrevDirtyList = NULL;
   mNextDirtyList = NULL;
   mDirtyMaskBits = 0;
}

NetObject::~NetObject()
{
   while(mFirstObjectRef)
      mFirstObjectRef->connection->detachObject(mFirstObjectRef);

   if(mDirtyMaskBits)
   {
      if(mPrevDirtyList)
         mPrevDirtyList->mNextDirtyList = mNextDirtyList;
      else
         mDirtyList = mNextDirtyList;
      if(mNextDirtyList)
         mNextDirtyList->mPrevDirtyList = mPrevDirtyList;
   }
}

NetObject *NetObject::mDirtyList = NULL;

void NetObject::setMaskBits(U32 orMask)
{
   TNLAssert(orMask != 0, "Invalid net mask bits set.");
   TNLAssert(mDirtyMaskBits == 0 || (mPrevDirtyList != NULL || mNextDirtyList != NULL || mDirtyList == this), "Invalid dirty list state.");
   if(!mDirtyMaskBits)
   {
      TNLAssert(mNextDirtyList == NULL && mPrevDirtyList == NULL, "Object with zero mask already in list.");
      if(mDirtyList)
      {
         mNextDirtyList = mDirtyList;
         mDirtyList->mPrevDirtyList = this;
      }
      mDirtyList = this;
   }
   mDirtyMaskBits |= orMask;
   TNLAssert(mDirtyMaskBits == 0 || (mPrevDirtyList != NULL || mNextDirtyList != NULL || mDirtyList == this), "Invalid dirty list state.");
}

void NetObject::clearMaskBits(U32 orMask)
{
   if(mDirtyMaskBits)
   {
      mDirtyMaskBits &= ~orMask;
      if(!mDirtyMaskBits)
      {
         if(mPrevDirtyList)
            mPrevDirtyList->mNextDirtyList = mNextDirtyList;
         else
            mDirtyList = mNextDirtyList;
         if(mNextDirtyList)
            mNextDirtyList->mPrevDirtyList = mPrevDirtyList;
         mNextDirtyList = mPrevDirtyList = NULL;
      }
   }
   
   for(GhostInfo *walk = mFirstObjectRef; walk; walk = walk->nextObjectRef)
   {
      if(walk->updateMask && walk->updateMask == orMask)
      {
         walk->updateMask = 0;
         walk->connection->ghostPushToZero(walk);
      }
      else
         walk->updateMask &= ~orMask;
   }
}

void NetObject::collapseDirtyList()
{
   Vector<NetObject *> tempV;
   for(NetObject *t = mDirtyList; t; t = t->mNextDirtyList)
      tempV.push_back(t);

   for(NetObject *obj = mDirtyList; obj; )
   {
      NetObject *next = obj->mNextDirtyList;
      U32 orMask = obj->mDirtyMaskBits;

      obj->mNextDirtyList = NULL;
      obj->mPrevDirtyList = NULL;
      obj->mDirtyMaskBits = 0;

      if(orMask)
      {
         for(GhostInfo *walk = obj->mFirstObjectRef; walk; walk = walk->nextObjectRef)
         {
            if(!walk->updateMask)
            {
               walk->updateMask = orMask;
               walk->connection->ghostPushNonZero(walk);
            }
            else
               walk->updateMask |= orMask;
         }
      }
      obj = next;
   }
   mDirtyList = NULL;
   for(S32 i = 0; i < tempV.size(); i++)
   {
      TNLAssert(tempV[i]->mNextDirtyList == NULL && tempV[i]->mPrevDirtyList == NULL && tempV[i]->mDirtyMaskBits == 0, "Error in collapse");
   }
}

bool NetObject::onGhostAdd(GhostConnection *theConnection)
{
   return true;
}
void NetObject::onGhostAddBeforeUpdate(GhostConnection *theConnection)
{
}


void NetObject::onGhostRemove()
{
}

void NetObject::onGhostAvailable(GhostConnection *)
{
}

//-----------------------------------------------------------------------------

F32 NetObject::getUpdatePriority(NetObject*, U32, S32 updateSkips)
{
   return F32(updateSkips) * 0.1f;

   //return 0;
}

U32 NetObject::packUpdate(GhostConnection*, U32, BitStream*)
{
   return 0;
}

void NetObject::unpackUpdate(GhostConnection*, BitStream*)
{
   // Do nothing
}

void NetObject::performScopeQuery(GhostConnection *connection)
{
   // default behavior - since we have no idea here about
   // the contents of the world, or why they matter, just scope
   // the scope object.
   connection->objectInScope(this);
}

void NetObject::postRPCEvent(NetObjectRPCEvent *theEvent)
{
   RefPtr<NetObjectRPCEvent> event = theEvent;

   TNLAssert((!isGhost() && theEvent->mRPCDirection == RPCToGhost) ||
               (isGhost() && theEvent->mRPCDirection == RPCToGhostParent),
         "Invalid RPC call - going in the wrong direction!");

   // ok, see what kind of an object this is:
   if(isGhost())
      mOwningConnection->postNetEvent(theEvent);
   else if(NetObject::getRPCDestConnection())
   {
      NetObject::getRPCDestConnection()->postNetEvent(theEvent);
   }
   else
   {
      for(GhostInfo *walk = mFirstObjectRef; walk; walk = walk->nextObjectRef)
      {
         if(!(walk->flags & GhostInfo::NotAvailable))
            walk->connection->postNetEvent(theEvent);
      }
   }
}

void NetObjectRPCEvent::pack(EventConnection *ps, BitStream *bstream)
{
   GhostConnection *gc = static_cast<GhostConnection *>(ps);
   S32 ghostIndex = -1;
   if(!mDestObject.isNull())
      ghostIndex = gc->getGhostIndex(mDestObject);

   if(bstream->writeFlag(ghostIndex != -1))
   {
      bstream->writeInt(ghostIndex, GhostConnection::GhostIdBitSize);
      RPCEvent::pack(ps, bstream);
   }
}

void NetObjectRPCEvent::unpack(EventConnection *ps, BitStream *bstream)
{
   // make sure this is a valid place for this event to be...
   GhostConnection *gc = static_cast<GhostConnection *>(ps);

   if( (gc->doesGhostTo() && mRPCDirection == RPCToGhost) ||
       (gc->doesGhostFrom() && mRPCDirection == RPCToGhostParent) )
   {
      if(bstream->readFlag())
      {
         S32 ghostIndex = bstream->readInt(GhostConnection::GhostIdBitSize);
         RPCEvent::unpack(ps, bstream);

         if(mRPCDirection == RPCToGhost)
            mDestObject = gc->resolveGhost(ghostIndex);
         else
            mDestObject = gc->resolveGhostParent(ghostIndex);
      }
   }
   else
      gc->setLastError("Invalid Packet.");
}

void NetObjectRPCEvent::process(EventConnection *ps)
{
   if(mDestObject.isNull())
      return;

   if(!checkClassType(mDestObject))
      return;

   Object *thisPointer = (Object *) mDestObject.getPointer();

   NetObject::mRPCSourceConnection = (GhostConnection *) ps;
   mFunctor->dispatch(thisPointer);

   NetObject::mRPCSourceConnection = NULL;
}

};
