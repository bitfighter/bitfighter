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

#ifndef _TNL_NETEVENT_H_
#define _TNL_NETEVENT_H_

#ifndef _TNL_NETBASE_H_
#include "tnlNetBase.h"
#endif

namespace TNL {

//----------------------------------------------------------------------------

class EventConnection;
class BitStream;

/// An event to be sent over the network.
///
/// @note TNL implements two methods of network data passing; this is one of them.
/// See GhostConnection for details of the other, which is referred to as ghosting.
///
/// TNL lets you pass NetEvent objects across EventConnection instances. There are three
/// types of events:
///      - <b>Unguaranteed events</b> are events which are sent once. If they don't
///        make it through the link, they are not resent. This is good for quick,
///        frequent status updates which are of transient interest, like voice
///        communication fragments.
///      - <b>Guaranteed events</b> are events which are guaranteed to be
///        delivered. If they don't make it through the link, they are sent as
///        needed. This is good for important, one-time information,
///        like which team a user wants to play on, or the current weather.
///        Guaranteed events are processed when they are received, so they may be processed
///        in a different order than they were sent in.
///      - <b>GuaranteedOrdered events</b> are events which are guaranteed to be
///        delivered, and whose process methods will be executed in the order the events were sent.
///        This is good for information which is not only important, but also order-critical, like
///        chat messages or file transfers.
///
/// There are 3 methods that you need to implement if you want to make a
/// basic NetEvent subclass, and 2 macros you need to call.
///
/// @code
/// // A simple NetEvent to transmit a string over the network.
/// class SimpleMessageEvent : public NetEvent
/// {
///    typedef NetEvent Parent;
///    char *msg;
/// public:
///    SimpleMessageEvent(const char *message = NULL);
///    ~SimpleMessageEvent();
///
///    virtual void pack   (EventConnection *conn, BitStream *bstream)
///    virtual void unpack (EventConnection *conn, BitStream *bstream);
///    virtual void process(EventConnection *conn);
///
///    TNL_DECLARE_CLASS(SimpleMessageEvent);
/// };
///
/// TNL_IMPLEMENT_NETEVENT(SimpleMessageEvent, NetClassGroupGameMask, 0);
/// @endcode
///
/// The first macro called, TNL_DECLARE_CLASS() registers the static class functions and NetClassRep object that will assign
/// this class a network ID and allow instances to be constructed by ID.
///
/// The second, TNL_IMPLEMENT_NETEVENT(), instantiates the NetClassRep and
/// tells it that the instances are NetEvent objects in the Game group.  The final
/// parameter to the TNL_IMPLEMENT_NETEVENT macro is the version number of the event
/// class.  Versioning allows a server to offer new event services without forcing
/// older clients to be updated.
///
/// In the constructor for the event the guarantee type of the event and the direction it will be
/// allowed to travel over the connection, must be specified by way of the constructor
/// for the base NetEvent class.  The guarantee type can be one of:
///      - <b>NetEvent::GuaranteedOrdered</b>, for guaranteed, ordered events
///      - <b>NetEvent::Guaranteed</b>, for guaranteed events
///      - <b>NetEvent::Unguaranteed</b>, for unguaranteed events
///
/// It is also a good idea to clearly specify which direction the event is allowed to travel.
/// If the program has a certain set of message events that are only sent from server to client,
/// then the network system can enforce that error checking automatically, in order to prevent hacks that may
/// otherwise crash or compromise the program.  The valid event directions are:
///      - <b>NetEvent::DirAny</b>, this event can be sent from server to client
///        or from client to server
///      - <b>NetEvent::DirServerToClient</b>, this event can only be sent from
///        server to client.  If the server receives an event of this type, it will
///        signal an error on the connection.
///      - <b>NetEvent::DirClientToServer</b>, this event can only be sent from client
///        to server.  If the client receives an event of this type, it will signal an
///        error on the connection.
///
/// @note TNL allows you to call NetConnection::setLastError() on the EventConnection passed to
///       the NetEvent. This will cause the connection to abort if invalid data is received, specifying
///       a reason to the user.
///
/// Of the 5 methods declared above; the constructor and destructor need only do
/// whatever book-keeping is needed for the specific implementation, in addition to calling
/// the NetEvent constructor with the direction and type information that the networking system
/// needs to function. In this case, the SimpleMessageEvent simply allocates/deallocates the space for the string,
/// and specifies the event as guaranteed ordered and bidirectional.
///
/// @code
///    SimpleMessageEvent::SimpleMessageEvent(const char *message = NULL)
///           : NetEvent(NetEvent::GuaranteedOrdered, NetEvent::DirAny)
///    {
///       // we marked this event as GuaranteedOrdered, and it can be sent in any direction
///       if(message)
///          msg = strdup(message);
///       else
///          msg = NULL;
///    }
///
///    SimpleMessageEvent::~SimpleMessageEvent()
///    {
///      free(msg);
///    }
/// @endcode
///
/// The 3 other functions that must be overridden for evern NetEvent are pack(), unpack() and process().
///
/// <b>pack()</b> is responsible for packing the event over the wire:
///
/// @code
/// void SimpleMessageEvent::pack(EventConnection* conn, BitStream *bstream)
/// {
///   bstream->writeString(msg);
/// }
/// @endcode
///
/// <b>unpack()</b> is responsible for unpacking the event on the other end:
///
/// @code
/// // The networking layer takes care of instantiating a new
/// // SimpleMessageEvent, which saves us a bit of effort.
/// void SimpleMessageEvent::unpack(EventConnection *conn, BitStream *bstream)
/// {
///   char buf[256];
///   bstream->readString(buf);
///   msg = strdup(buf);
/// }
/// @endcode
///
/// <b>process()</b> is called when the network layer is finished with things.
/// A typical case is that a GuaranteedOrdered event is unpacked and stored, but
/// not processed until the events preceding it in the sequence have been process()'d.
///
/// @code
/// // This just prints the event in the log. You might
/// // want to do something more clever here.
/// void SimpleMessageEvent::process(EventConnection *conn)
/// {
///   logprintf("Received a SimpleMessageEvent: %s", msg);
///
///   // An example of something more clever - kick people who say bad words.
///   // if(isBadWord(msg)) conn->setLastError("No swearing, naughtypants!");
/// }
/// @endcode
///
/// Posting an event to the remote host on a connection is simple:
///
/// @code
/// EventConnection *conn; // We assume you have filled this in.
///
/// conn->postNetEvent(new SimpleMessageEvent("This is a test!"));
/// @endcode
///
/// Finally, for more advanced applications, notifyPosted() is called when the event is posted
/// into the send queue, notifySent() is called whenever the event is
/// sent over the wire, in EventConnection::eventWritePacket(). notifyDelivered() is called
/// when the packet is finally received or (in the case of Unguaranteed packets) dropped.
///
/// @note the TNL_IMPLEMENT_NETEVENT groupMask specifies which "group" of EventConnections
/// the event can be sent over.  See TNL::Object for a further discussion of this.
class NetEvent : public Object
{
   friend class EventConnection;
public:
   enum EventDirection {
      DirUnset,          ///< Default value - NetConnection will Assert if an event is posted without a valid direction set.
      DirAny,            ///< This event can be sent from the server or the client
      DirServerToClient, ///< This event can only be sent from the server to the client
      DirClientToServer, ///< This event can only be sent from the client to the server
   } mEventDirection;    ///< Direction this event is allowed to travel in the network

   enum GuaranteeType {
      GuaranteedOrdered = 0, ///< Event delivery is guaranteed and will be processed in the order it
                             ///  was sent relative to other ordered events.
      Guaranteed = 1,        ///< Event delivery is guaranteed and will be processed in the order it
                             ///  was received.
      Unguaranteed = 2,      ///< Event delivery is not guaranteed - however, the event will remain
      GuaranteedOrderedBigData = 3

                             ///  ordered relative to other unguaranteed events.
   } mGuaranteeType;        ///< Type of data guarantee this event supports

   /// Constructor - should always be called by subclasses.
   ///
   /// Subclasses MUST pass in an event direction and guarantee type, or else the network
   /// system will error on the event. Events are by default GuaranteedOrdered, however,
   /// the default direction is unset which will result in asserts.
   NetEvent(GuaranteeType gType = GuaranteedOrdered, EventDirection evDir = DirUnset)
   {
      mGuaranteeType = gType;
      mEventDirection = evDir;
   }

   /// Pack is called on the origin side of the connection to write an event's
   /// data into a packet.
   virtual void pack(EventConnection *ps, BitStream *bstream) = 0;

   /// Unpack is called on the destination side of the connection to read an event's
   /// data out of a packet.
   virtual void unpack(EventConnection *ps, BitStream *bstream) = 0;

   /// Process is called to process the event data when it has been unpacked.
   ///
   /// For a guaranteed, ordered event, process is called only once all prior events
   /// have been received and processed.  For unguaranteed events, process is called
   /// immediately after unpack.
   virtual void process(EventConnection *ps) = 0;

   /// notifyPosted is called on an event when it is posted to a particular EventConnection, before it is added to the send queue.
   /// This allows events to post additional events to the connection that will be send _before_ this event
   virtual void notifyPosted(EventConnection *ps) {}

   /// notifySent is called on each event after all of the events for a packet have
   /// been written into the packet stream.
   virtual void notifySent(EventConnection *ps) {}

   /// notifyDelivered is called on the source event after it has been received
   /// and processed by the other side of the connection.
   ///
   /// If the packet delivery fails on an unguaranteed event, madeIt will be
   /// false, otherwise it will be true.
   virtual void notifyDelivered(EventConnection *ps, bool madeIt) {}

   /// getEventDirection returns the direction this event is allowed to travel in on a connection
   EventDirection getEventDirection()
   {
      return mEventDirection;
   }

   /// getDebugName is used to construct event names for packet
   /// logging in debug mode.
   virtual const char *getDebugName()
   {
      return getClassName();
   }
};


/// The IMPLEMENT_NETEVENT macro is used for implementing events
/// that can be sent from server to client or from client to server
#define TNL_IMPLEMENT_NETEVENT(className,groupMask,classVersion) \
   TNL::NetClassRep* className::getClassRep() const { return &className::dynClassRep; } \
   TNL::NetClassRepInstance<className> className::dynClassRep(#className,groupMask, TNL::NetClassTypeEvent, classVersion)

};

#endif
