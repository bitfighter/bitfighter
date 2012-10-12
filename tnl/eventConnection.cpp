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
#include "tnlLog.h"
#include "tnlNetInterface.h"

namespace TNL {

ClassChunker<EventConnection::EventNote> EventConnection::mEventNoteChunker;

EventConnection::EventConnection()
{
   // Event management data:
   mNotifyEventList = NULL;
   mSendEventQueueHead = NULL;
   mSendEventQueueTail = NULL;
   mUnorderedSendEventQueueHead = NULL;
   mUnorderedSendEventQueueTail = NULL;
   mWaitSeqEvents = NULL;

   mNextSendEventSeq = FirstValidSendEventSeq;
   mNextRecvEventSeq = FirstValidSendEventSeq;
   mLastAckedEventSeq = -1;
   mEventClassCount = 0;
   mEventClassBitSize = 0;
   mTNLDataBuffer = NULL;
}

static const U32 mTNLDataBufferMaxSize = 1024 * 256;

EventConnection::~EventConnection()
{
   clearAllPacketNotifies(); ///BUGFIX --> http://www.garagegames.com/community/forums/viewthread/80511

   while(mNotifyEventList)
   {
      EventNote *temp = mNotifyEventList;
      mNotifyEventList = temp->mNextEvent;
      
      temp->mEvent->notifyDelivered(this, true);
      mEventNoteChunker.free(temp);
   }
   while(mUnorderedSendEventQueueHead)
   {
      EventNote *temp = mUnorderedSendEventQueueHead;
      mUnorderedSendEventQueueHead = temp->mNextEvent;
      
      temp->mEvent->notifyDelivered(this, true);
      mEventNoteChunker.free(temp);
   }
   while(mSendEventQueueHead)
   {
      EventNote *temp = mSendEventQueueHead;
      mSendEventQueueHead = temp->mNextEvent;
      
      temp->mEvent->notifyDelivered(this, true);
      mEventNoteChunker.free(temp);
   }
   if(mTNLDataBuffer)
      delete mTNLDataBuffer;
}

void EventConnection::writeConnectRequest(BitStream *stream)
{
   Parent::writeConnectRequest(stream);
   stream->write(NetClassRep::getNetClassCount(getNetClassGroup(), NetClassTypeEvent));   // Essentially a count of RPCs (c2s, s2c, etc.)
}


// Reads the NetEvent class count max that the remote host is requesting.
// If this host has MORE NetEvent classes declared, the mEventClassCount
// is set to the requested count, and is verified to lie on a boundary between versions.
// This gets run when someone is connecting to us
bool EventConnection::readConnectRequest(BitStream *stream, NetConnection::TerminationReason &reason)
{
   if(!Parent::readConnectRequest(stream, reason))
      return false;

   U32 remoteClassCount;
   stream->read(&remoteClassCount);

   U32 localClassCount = NetClassRep::getNetClassCount(getNetClassGroup(), NetClassTypeEvent);   // Essentially a count of RPCs 

   // If remote client has more classes defined than we do, hope/assume they're defined in the same order, so that we at least agree
   // on the available set of RPCs.
   // This implies the client is higher version than the server  
   if(localClassCount <= remoteClassCount)
      mEventClassCount = localClassCount;    // We're only willing to support as many as we have
   else     // We have more RPCs on the local machine ==> implies server is higher version than client
   {
      mEventClassCount = remoteClassCount;   // We're willing to support the number of classes the client has

      // Check if the next RPC is a higher version than the current one specified by mEventClassCount
      if(!NetClassRep::isVersionBorderCount(getNetClassGroup(), NetClassTypeEvent, mEventClassCount))
      {
         reason = ReasonIncompatibleRPCCounts;
         return false;     // If not, abort connection
      }
   }

   mEventClassVersion = NetClassRep::getClass(getNetClassGroup(), NetClassTypeEvent, mEventClassCount-1)->getClassVersion();
   mEventClassBitSize = getNextBinLog2(mEventClassCount);
   return true;
}


void EventConnection::writeConnectAccept(BitStream *stream)
{
   Parent::writeConnectAccept(stream);
   stream->write(mEventClassCount);    // Tell the client how many RPCs we, the server, are willing to support
                                       // (we may support more... see how this val is calced above)
}


bool EventConnection::readConnectAccept(BitStream *stream, NetConnection::TerminationReason &reason)
{
   if(!Parent::readConnectAccept(stream, reason))
      return false;

   stream->read(&mEventClassCount);                                                      // Number of RPCs the remote server is willing to support
   U32 myCount = NetClassRep::getNetClassCount(getNetClassGroup(), NetClassTypeEvent);   // Number we, the client, support

   if(mEventClassCount > myCount)      // Normally, these should be equal.  If the server is not willing to support as many RPCs as we want to use,
      return false;                    // then bail.

   if(!NetClassRep::isVersionBorderCount(getNetClassGroup(), NetClassTypeEvent, mEventClassCount))
      return false;

   mEventClassBitSize = getNextBinLog2(mEventClassCount);
   return true;
}

void EventConnection::processEvent(NetEvent *theEvent)
{
   if(getConnectionState() == NetConnection::Connected)
      theEvent->process(this);
}

void EventConnection::packetDropped(PacketNotify *pnotify)
{
   Parent::packetDropped(pnotify);
   EventPacketNotify *notify = static_cast<EventPacketNotify *>(pnotify);

   EventNote *walk = notify->eventList;
   EventNote **insertList = &mSendEventQueueHead;
   EventNote *temp;
   
   while(walk)
   {
      switch(walk->mEvent->mGuaranteeType)
      {
         case NetEvent::GuaranteedOrdered:
            // It was a guaranteed ordered packet, reinsert it back into
            // mSendEventQueueHead in the right place (based on seq numbers)

            logprintf(LogConsumer::LogEventConnection, "EventConnection %s: DroppedGuaranteed - %d", getNetAddressString(), walk->mSeqCount);
            while(*insertList && (*insertList)->mSeqCount < walk->mSeqCount)
               insertList = &((*insertList)->mNextEvent);
            
            temp = walk->mNextEvent;
            walk->mNextEvent = *insertList;
            if(!walk->mNextEvent)
               mSendEventQueueTail = walk;
            *insertList = walk;
            insertList = &(walk->mNextEvent);
            walk = temp;
            break;
         case NetEvent::Guaranteed:
            // It was a guaranteed packet, put it at the top of
            // mUnorderedSendEventQueueHead.
            temp = walk->mNextEvent;
            walk->mNextEvent = mUnorderedSendEventQueueHead;
            mUnorderedSendEventQueueHead = walk;
            if(!walk->mNextEvent)
               mUnorderedSendEventQueueTail = walk;
            walk = temp;
            break;
         case NetEvent::Unguaranteed:
            // Or else it was an unguaranteed packet, notify that
            // it was _not_ delivered and blast it.
            walk->mEvent->notifyDelivered(this, false);
            temp = walk->mNextEvent;
            mEventNoteChunker.free(walk);
            walk = temp;
      }
   }
}

void EventConnection::packetReceived(PacketNotify *pnotify)
{
   Parent::packetReceived(pnotify);

   EventPacketNotify *notify = static_cast<EventPacketNotify *>(pnotify);

   EventNote *walk = notify->eventList;
   EventNote **noteList = &mNotifyEventList;

   while(walk)
   {
      EventNote *next = walk->mNextEvent;
      if(walk->mEvent->mGuaranteeType != NetEvent::GuaranteedOrdered)
      {
         walk->mEvent->notifyDelivered(this, true);
         mEventNoteChunker.free(walk);
         walk = next;
      }
      else
      {
         while(*noteList && (*noteList)->mSeqCount < walk->mSeqCount)
            noteList = &((*noteList)->mNextEvent);
         
         walk->mNextEvent = *noteList;
         *noteList = walk;
         noteList = &walk->mNextEvent;
         walk = next;
      }
   }
   while(mNotifyEventList && mNotifyEventList->mSeqCount == mLastAckedEventSeq + 1)
   {
      mLastAckedEventSeq++;
      EventNote *next = mNotifyEventList->mNextEvent;
      logprintf(LogConsumer::LogEventConnection, "EventConnection %s: NotifyDelivered - %d", getNetAddressString(), mNotifyEventList->mSeqCount);
      mNotifyEventList->mEvent->notifyDelivered(this, true);
      mEventNoteChunker.free(mNotifyEventList);
      mNotifyEventList = next;
   }
}

void EventConnection::writePacket(BitStream *bstream, PacketNotify *pnotify)
{
   Parent::writePacket(bstream, pnotify);
   EventPacketNotify *notify = static_cast<EventPacketNotify *>(pnotify);
   
   bool have_something_to_send = bstream->getBitPosition() >= 128;

   if(mConnectionParameters.mDebugObjectSizes)
      bstream->writeInt(DebugChecksum, 32);

   EventNote *packQueueHead = NULL, *packQueueTail = NULL;

   while(mUnorderedSendEventQueueHead)
   {
      if(bstream->isFull())
         break;
      // get the first event
      EventNote *ev = mUnorderedSendEventQueueHead;
      ConnectionStringTable::PacketEntry *strEntry = getCurrentWritePacketNotify()->stringList.stringTail;

      bstream->writeFlag(true);
      S32 start = bstream->getBitPosition();

      if(mConnectionParameters.mDebugObjectSizes)
         bstream->advanceBitPosition(BitStreamPosBitSize);
      
      S32 classId = ev->mEvent->getClassId(getNetClassGroup());
      bstream->writeInt(classId, mEventClassBitSize);

      ev->mEvent->pack(this, bstream);
      logprintf(LogConsumer::LogEventConnection, "EventConnection %s: WroteEvent %s - %d bits", getNetAddressString(), ev->mEvent->getDebugName(), bstream->getBitPosition() - start);

      if(mConnectionParameters.mDebugObjectSizes)
         bstream->writeIntAt(bstream->getBitPosition(), BitStreamPosBitSize, start);

      // check for packet overrun, and rewind this update if there
      // was one:
      if(!bstream->isValid() || bstream->getBitPosition() >= MaxPreferredPacketDataSize*8 - MinimumPaddingBits)
      {
         mStringTable->packetRewind(&getCurrentWritePacketNotify()->stringList, strEntry);  // we never sent those stuff (TableStringEntry), so let it drop
         TNLAssert(have_something_to_send || bstream->getBitPosition() < MaxPacketDataSize*8 - MinimumPaddingBits, "Packet too big to send");
         if(have_something_to_send)
         {
            bstream->setBitPosition(start - 1);
            bstream->clearError();
            break;
         }
         else //if(bstream->getBitPosition() < MaxPacketDataSize*8 - MinimumPaddingBits)
         {
            TNLAssertV(false, ("%s Packet too big to send, one or more events may be unable to send", ev->mEvent->getDebugName()));
            // dequeue the event:
            mUnorderedSendEventQueueHead = ev->mNextEvent;
            ev->mNextEvent = NULL;
            ev->mEvent->notifyDelivered(this, false);
            mEventNoteChunker.free(ev);
            bstream->setBitPosition(start - 1);
            bstream->clearError();
            break;
         }
      }
      have_something_to_send = true;

      // dequeue the event and add this event onto the packet queue
      mUnorderedSendEventQueueHead = ev->mNextEvent;
      ev->mNextEvent = NULL;

      if(!packQueueHead)
         packQueueHead = ev;
      else
         packQueueTail->mNextEvent = ev;
      packQueueTail = ev;
   }
   
   bstream->writeFlag(false);   
   S32 prevSeq = -2;
   
   while(mSendEventQueueHead)
   {
      if(bstream->isFull())
         break;
      
      // if the event window is full, stop processing
      if(mSendEventQueueHead->mSeqCount > mLastAckedEventSeq + 126)
         break;

      // get the first event
      EventNote *ev = mSendEventQueueHead;
      S32 eventStart = bstream->getBitPosition();
      ConnectionStringTable::PacketEntry *strEntry = getCurrentWritePacketNotify()->stringList.stringTail;

      bstream->writeFlag(true);

      if(!bstream->writeFlag(ev->mSeqCount == prevSeq + 1))
         bstream->writeInt(ev->mSeqCount, 7);
      prevSeq = ev->mSeqCount;

      if(mConnectionParameters.mDebugObjectSizes)
         bstream->advanceBitPosition(BitStreamPosBitSize);

      S32 start = bstream->getBitPosition();

      S32 classId = ev->mEvent->getClassId(getNetClassGroup());
      bstream->writeInt(classId, mEventClassBitSize);
      ev->mEvent->pack(this, bstream);

      ev->mEvent->getClassRep()->addInitialUpdate(bstream->getBitPosition() - start);
      logprintf(LogConsumer::LogEventConnection, "EventConnection %s: WroteEvent %s - %d bits", getNetAddressString(), ev->mEvent->getDebugName(), bstream->getBitPosition() - start);

      if(mConnectionParameters.mDebugObjectSizes)
         bstream->writeIntAt(bstream->getBitPosition(), BitStreamPosBitSize, start - BitStreamPosBitSize);

      // check for packet overrun, and rewind this update if there
      // was one:
      if(!bstream->isValid() || bstream->getBitPosition() >= MaxPreferredPacketDataSize*8 - MinimumPaddingBits)
      {
         mStringTable->packetRewind(&getCurrentWritePacketNotify()->stringList, strEntry);  // we never sent those stuff (TableStringEntry), so let it drop
         if(have_something_to_send)
         {
            bstream->setBitPosition(eventStart);
            bstream->clearError();
            break;
         }
         else
         {
            TNLAssertV(false, ("%s Packet too big to send, one or more events may be unable to send", ev->mEvent->getDebugName()));
            for(EventNote *walk = ev->mNextEvent; walk; walk = walk->mNextEvent)
               walk->mSeqCount--;    // removing a GuaranteedOrdered needs to re-order mSeqCount
            mNextSendEventSeq--;

            // dequeue the event:
            mSendEventQueueHead = ev->mNextEvent;
            ev->mNextEvent = NULL;
            ev->mEvent->notifyDelivered(this, false);
            mEventNoteChunker.free(ev);
            bstream->setBitPosition(eventStart);
            bstream->clearError();
            break;
         }
      }
      have_something_to_send = true;

      // dequeue the event:
      mSendEventQueueHead = ev->mNextEvent;      
      ev->mNextEvent = NULL;
      if(!packQueueHead)
         packQueueHead = ev;
      else
         packQueueTail->mNextEvent = ev;
      packQueueTail = ev;
   }
   for(EventNote *ev = packQueueHead; ev; ev = ev->mNextEvent)
      ev->mEvent->notifySent(this);
      
   notify->eventList = packQueueHead;
   bstream->writeFlag(0);
}

void EventConnection::readPacket(BitStream *bstream)
{
   Parent::readPacket(bstream);

   if(mConnectionParameters.mDebugObjectSizes)
   {
      U32 USED_EXTERNAL sum = bstream->readInt(32);
      TNLAssert(sum == DebugChecksum, "Invalid checksum.");
   }
   
   S32 prevSeq = -2;
   EventNote **waitInsert = &mWaitSeqEvents;
   bool unguaranteedPhase = true;
   
   while(true)
   {
      bool bit = bstream->readFlag();
      if(unguaranteedPhase && !bit)
      {
         unguaranteedPhase = false;
         bit = bstream->readFlag();
      }
      if(!unguaranteedPhase && !bit)
         break;
      
      S32 seq = -1;
      
      if(!unguaranteedPhase) // get the sequence
      {
         if(bstream->readFlag())
            seq = (prevSeq + 1) & 0x7f;
         else
            seq = bstream->readInt(7);
         prevSeq = seq;
      }

      NetEvent *evt = unpackNetEvent(bstream);
      if(!evt)
         return;

      if(unguaranteedPhase)
      {
         processEvent(evt);
         delete evt;
         if(mErrorBuffer[0])
            return;
         continue;
      }
      seq |= (mNextRecvEventSeq & ~0x7F);
      if(seq < mNextRecvEventSeq)
         seq += 128;
      
      EventNote *note = mEventNoteChunker.alloc();
      note->mEvent = evt;
      note->mSeqCount = seq;
      logprintf(LogConsumer::LogEventConnection, "EventConnection %s: RecvdGuaranteed %d", getNetAddressString(), seq);

      while(*waitInsert && (*waitInsert)->mSeqCount < seq)
         waitInsert = &((*waitInsert)->mNextEvent);
      
      note->mNextEvent = *waitInsert;
      *waitInsert = note;
      waitInsert = &(note->mNextEvent);
   }
   while(mWaitSeqEvents && mWaitSeqEvents->mSeqCount == mNextRecvEventSeq)
   {
      mNextRecvEventSeq++;
      EventNote *temp = mWaitSeqEvents;
      mWaitSeqEvents = temp->mNextEvent;
      
      logprintf(LogConsumer::LogEventConnection, "EventConnection %s: ProcessGuaranteed %d", getNetAddressString(), temp->mSeqCount);
      processEvent(temp->mEvent);
      mEventNoteChunker.free(temp);
      if(mErrorBuffer[0])
         return;
   }
}

bool EventConnection::postNetEvent(NetEvent *theEvent)
{
   // Check if the direction this event moves is a valid direction.
   TNLAssertV(   (theEvent->getEventDirection() != NetEvent::DirUnset)
      && (theEvent->getEventDirection() != NetEvent::DirServerToClient || isConnectionToClient())
      && (theEvent->getEventDirection() != NetEvent::DirClientToServer || isConnectionToServer()),
      ("Trying to send wrong event direction in %s", theEvent->getClassName()));

   S32 classId = theEvent->getClassId(getNetClassGroup());
   if(U32(classId) >= mEventClassCount && getConnectionState() == Connected)
      return false;

   theEvent->notifyPosted(this);

   EventNote *event = mEventNoteChunker.alloc();
   event->mEvent = theEvent;
   event->mNextEvent = NULL;

   if(event->mEvent->mGuaranteeType == NetEvent::GuaranteedOrdered)
   {
      event->mSeqCount = mNextSendEventSeq++;
      if(!mSendEventQueueHead)
         mSendEventQueueHead = event;
      else
         mSendEventQueueTail->mNextEvent = event;
      mSendEventQueueTail = event;
   }
   else if(event->mEvent->mGuaranteeType == NetEvent::GuaranteedOrderedBigData)
   {
      BitStream bstream;
      const U32 start = 0;
      const U32 partsSize = 512;

      if(mConnectionParameters.mDebugObjectSizes)
         bstream.advanceBitPosition(BitStreamPosBitSize);
      
      S32 classId = event->mEvent->getClassId(getNetClassGroup());
      bstream.writeInt(classId, mEventClassBitSize);

      event->mEvent->pack(this, &bstream);
      logprintf(LogConsumer::LogEventConnection, "EventConnection %s: WroteEvent %s - %d bits", getNetAddressString(), event->mEvent->getDebugName(), bstream.getBitPosition() - start);

      if(mConnectionParameters.mDebugObjectSizes)
         bstream.writeIntAt(bstream.getBitPosition(), BitStreamPosBitSize, start);

      U32 size = bstream.getBytePosition();

      for(U32 i=0; i<size; i+=partsSize)
      {
         if(i+partsSize < size)
         {
            ByteBuffer *bytebuffer = new ByteBuffer(&bstream.getBuffer()[i], partsSize);
            bytebuffer->takeOwnership();  // may have to use this to prevent errors.
            s2rTNLSendDataParts(0, ByteBufferPtr(bytebuffer));
         }
         else
         {
            ByteBuffer *bytebuffer = new ByteBuffer(&bstream.getBuffer()[i], size-i);
            bytebuffer->takeOwnership();
            s2rTNLSendDataParts(1, ByteBufferPtr(bytebuffer));
         }
      }
      mEventNoteChunker.free(event);
   }
   else
   {
      event->mSeqCount = InvalidSendEventSeq;
      if(!mUnorderedSendEventQueueHead)
         mUnorderedSendEventQueueHead = event;
      else
         mUnorderedSendEventQueueTail->mNextEvent = event;
      mUnorderedSendEventQueueTail = event;
   }
   return true;
}

bool EventConnection::isDataToTransmit()
{
   return mUnorderedSendEventQueueHead || mSendEventQueueHead || Parent::isDataToTransmit();
}


NetEvent *EventConnection::unpackNetEvent(BitStream *bstream)
{
   U32 endingPosition;
   if(mConnectionParameters.mDebugObjectSizes)
      endingPosition = bstream->readInt(BitStreamPosBitSize);

   U32 classId = bstream->readInt(mEventClassBitSize);
   if(classId >= mEventClassCount)
   {
      setLastError("Invalid packet -- classId too high.");
      return NULL;
   }

   NetEvent *evt = (NetEvent *) Object::create(getNetClassGroup(), NetClassTypeEvent, classId);
   if(!evt)
   {
      setLastError("Invalid packet -- couldn't create event.");
      return NULL;
   }

   // Check if the direction this event moves is a valid direction.
   if(   (evt->getEventDirection() == NetEvent::DirUnset)
      || (evt->getEventDirection() == NetEvent::DirServerToClient && isConnectionToClient())
      || (evt->getEventDirection() == NetEvent::DirClientToServer && isConnectionToServer()) )
   {
      setLastError("Invalid Packet -- event direction wrong. %s", evt->getClassName());
      delete evt;
      return NULL;
   }


   evt->unpack(this, bstream);
   if(mErrorBuffer[0])
   {
      delete evt;
      return NULL;
   }

   if(mConnectionParameters.mDebugObjectSizes)
   {
      TNLAssert(endingPosition == bstream->getBitPosition(),
                avar("Unpack did not match pack for event of class %s.", evt->getClassName()) );
   }
   return evt;
}




TNL_IMPLEMENT_RPC(EventConnection, s2rTNLSendDataParts, (U8 type, ByteBufferPtr data), (type, data), NetClassGroupAllMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   if(mTNLDataBuffer)
   {
      if(mTNLDataBuffer->getBufferSize() < mTNLDataBufferMaxSize)  // limit memory, to avoid eating too much memory.
         mTNLDataBuffer->appendBuffer(*data.getPointer());
   }
   else
   {
      mTNLDataBuffer = new ByteBuffer(*data.getPointer());
      mTNLDataBuffer->takeOwnership();
   }

   if(type == 1 && mTNLDataBuffer->getBufferSize() != 0 && mTNLDataBuffer->getBufferSize() < mTNLDataBufferMaxSize)
   {
      BitStream bstream(mTNLDataBuffer->getBuffer(), mTNLDataBuffer->getBufferSize());
      NetEvent *evt = unpackNetEvent(&bstream);
      if(evt)
      {
         processEvent(evt);
         delete evt;
      }
   }

   if(type != 0)
   {
      delete mTNLDataBuffer;
      mTNLDataBuffer = NULL;
   }
}



};
