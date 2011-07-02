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

#ifndef _TNL_EVENTCONNECTION_H_
#define _TNL_EVENTCONNECTION_H_

#ifndef _TNL_NETCONNECTION_H_
#include "tnlNetConnection.h"
#endif

#ifndef _TNL_NETEVENT_H_
#include "tnlNetEvent.h"
#endif

#ifndef _TNL_DATACHUNKER_H_
#include "tnlDataChunker.h"
#endif

#ifndef _TNL_RPC_H_
#include "tnlRPC.h"
#endif


namespace TNL {

/// EventConnection is a NetConnection subclass used for sending guaranteed and unguaranteed
/// event packages across a connection.
///
/// The EventConnection is responsible for transmitting NetEvents over the wire.
/// It deals with ensuring that the various types of NetEvents are delivered appropriately,
/// and with notifying the event of its delivery status.
///
/// The EventConnection is mainly accessed via postNetEvent(), which accepts NetEvents.
///
/// @see NetEvent for a more thorough explanation of how to use events.

class EventConnection : public NetConnection
{
   typedef NetConnection Parent;

   /// EventNote associates a single event posted to a connection with a sequence number for ordered processing
   struct EventNote
   {
      RefPtr<NetEvent> mEvent; ///< A safe reference to the event
      S32 mSeqCount; ///< the sequence number of this event for ordering
      EventNote *mNextEvent; ///< The next event either on the connection or on the PacketNotify
   };
public:
   /// EventPacketNotify tracks all the events sent with a single packet
   struct EventPacketNotify : public NetConnection::PacketNotify
   {
      EventNote *eventList; ///< linked list of events sent with this packet
      EventPacketNotify() { eventList = NULL; }
   };

   EventConnection();
   ~EventConnection();
protected:
   enum DebugConstants
   {
      DebugChecksum = 0xF00DBAAD,
      BitStreamPosBitSize = 16,
   };

   /// Allocates a PacketNotify for this connection
   PacketNotify *allocNotify() { return new EventPacketNotify; }

   /// Override processing to requeue any guaranteed events in the packet that was dropped
   void packetDropped(PacketNotify *notify);

   /// Override processing to notify for delivery and dereference any events sent in the packet
   void packetReceived(PacketNotify *notify);

   /// Writes pending events into the packet, and attaches them to the PacketNotify
   void writePacket(BitStream *bstream, PacketNotify *notify);

   /// Reads events from the stream, and queues them for processing
   void readPacket(BitStream *bstream);

   /// Returns true if there are events pending that should be sent across the wire
   virtual bool isDataToTransmit();

   /// Dispatches an event
   void processEvent(NetEvent *theEvent);


//----------------------------------------------------------------
// event manager functions/code:
//----------------------------------------------------------------

private:
   static ClassChunker<EventNote> mEventNoteChunker; ///< Quick memory allocator for net event notes

   EventNote *mSendEventQueueHead;          ///< Head of the list of events to be sent to the remote host
   EventNote *mSendEventQueueTail;          ///< Tail of the list of events to be sent to the remote host.  New events are tagged on to the end of this list
   EventNote *mUnorderedSendEventQueueHead; ///< Head of the list of events sent without ordering information
   EventNote *mUnorderedSendEventQueueTail; ///< Tail of the list of events sent without ordering information
   EventNote *mWaitSeqEvents;   ///< List of ordered events on the receiving host that are waiting on previous sequenced events to arrive.
   EventNote *mNotifyEventList; ///< Ordered list of events on the sending host that are waiting for receipt of processing on the client.

   S32 mNextSendEventSeq;  ///< The next sequence number for an ordered event sent through this connection
   S32 mNextRecvEventSeq;  ///< The next receive event sequence to process
   S32 mLastAckedEventSeq; ///< The last event the remote host is known to have processed

   enum {
      InvalidSendEventSeq = -1,
      FirstValidSendEventSeq = 0
   };

protected:
   U32 mEventClassCount;      ///< Number of NetEvent classes supported by this connection
   U32 mEventClassBitSize;    ///< Bit field width of NetEvent class count.  i.e. how many bits needed to represent all classes?
   U32 mEventClassVersion;    ///< The highest version number of events on this connection.  --> assigned, but never read

   /// Writes the NetEvent class count into the stream, so that the remote
   /// host can negotiate a class count for the connection
   void writeConnectRequest(BitStream *stream);

   /// Reads the NetEvent class count max that the remote host is requesting.
   /// If this host has MORE NetEvent classes declared, the mEventClassCount
   /// is set to the requested count, and is verified to lie on a boundary between versions.
   bool readConnectRequest(BitStream *stream, NetConnection::TerminationReason &reason);

   /// Writes the negotiated NetEvent class count into the stream.   
   void writeConnectAccept(BitStream *stream);

   /// Reads the negotiated NetEvent class count from the stream and validates that it is on
   /// a boundary between versions.
   bool readConnectAccept(BitStream *stream, NetConnection::TerminationReason &reason);

public:
   /// returns the highest event version number supported on this connection --> unused
   U32 getEventClassVersion() { return mEventClassVersion; }

   /// Posts a NetEvent for processing on the remote host
   bool postNetEvent(NetEvent *event);

   /// For fake connections (AI for instance)
   virtual bool canPostNetEvent() const { return true; }

   TNL_DECLARE_RPC(s2rTNLSendDataParts, (U8 type, ByteBufferPtr data));
private:
   TNL::ByteBuffer *mTNLDataBuffer;
   NetEvent *unpackNetEvent(BitStream *bstream);

};

};

#endif
