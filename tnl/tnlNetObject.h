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

#ifndef _TNL_NETOBJECT_H_
#define _TNL_NETOBJECT_H_

#ifndef _TNL_NETBASE_H_
#include "tnlNetBase.h"
#endif

#ifndef _TNL_BITSTREAM_H_
#include "tnlBitStream.h"
#endif

#ifndef _TNL_RPC_H_
#include "tnlRPC.h"
#endif

namespace TNL {
//----------------------------------------------------------------------------
class GhostConnection;
class NetInterface;
class NetObjectRPCEvent;

struct GhostInfo;

//-----------------------------------------------------------------------------
/// Superclass for ghostable networked objects.
///
/// @section NetObject_intro Introduction To NetObject And Ghosting
///
/// One of the most powerful aspects of the Torque Network Library is its support
/// for ghosting and prioritized, most-recent-state network updates. The way
/// this works is a bit complex, but it is immensely efficient. Let's run
/// through the steps that the server goes through for each client in this part
/// of TNL's architecture:
///      - First, the server determines what objects are in-scope for the client.
///        This is done by calling performScopeQuery() on the object which is
///        considered the "scope" object. This could be a simulation avatar of the character,
///        a flyby camera, a vehicle the user is controlling, or something else.
///      - Second, it instructs the client to create "ghost" instances of those objects
///        to represent the source objects on the server.  Finally, it sends updates
///        to the ghosts of those objects whose state has been updated on the server,
///        prioritizing the updates based on how relevant the object is to that particular client.
///
/// There several significant advantages to using this networking system:
///      - Efficient network usage, since we only send data that has changed. In addition,
///        since we only care about most-recent data, if a packet is dropped, we don't waste
///        effort trying to deliver stale data.
///      - Cheating protection; since TNL doesn't deliver information about game objects which
///        aren't "in scope", the ability for clients to learn about objects outside their immediate
///        perceptions can be curtailed by an agressive scoping function.
///
/// @section NetObject_Implementation An Example Implementation
///
/// The basis of the ghost implementation in TNL is NetObject.  Each NetObject maintains an <b>updateMask</b>,
/// a 32 bit word representing up to 32 independent states for the object.  When a NetObject's state changes
/// it calls the setMaskBits method to notify the network layer that the state has changed and needs to be
/// updated on all clients that have that NetObject in scope.
///
/// Using a NetObject is very simple; let's go through a simple example implementation:
///
/// @code
/// class SimpleNetObject : public NetObject
/// {
/// public:
///   typedef NetObject Parent;
///   TNL_DECLARE_CLASS(SimpleNetObject);
/// @endcode
///
/// Above is the standard boilerplate code for a NetObject subclass.
///
/// @code
///    char message1[256];
///    char message2[256];
///    enum States {
///       Message1Mask = BIT(0),
///       Message2Mask = BIT(1),
///    };
/// @endcode
///
/// The example class has two object "states" that each instance keeps track of, message1 and message2.
/// A real game object might have states for health, velocity and position, or some other set of fields.
/// Each class has 32 bits to work with, so it's possible to be very specific when defining states.
/// In general, individual state bits should be assigned only to things that are updated independently -
/// so if you update the position field and the velocity at the same time always, you could use a single
/// bit to represent that state change.
///
/// @code
///    SimpleNetObject()
///    {
///       // in order for an object to be considered by the network system,
///       // the Ghostable net flag must be set.
///       // the ScopeAlways flag indicates that the object is always scoped
///       // on all active connections.
///       mNetFlags.set(ScopeAlways | Ghostable);
///       strcpy(message1, "Hello World 1!");
///       strcpy(message2, "Hello World 2!");
///    }
/// @endcode
///
/// Here is the constructor. The code initializes the net flags, indicating that
/// the SimpleNetObject should always be scoped, and that it can be ghosted to remote hosts
///
/// @code
///    U32 packUpdate(GhostConnection *, U32 mask, BitStream *stream)
///    {
///       // check which states need to be updated, and write updates
///       if(stream->writeFlag(mask & Message1Mask))
///          stream->writeString(message1);
///       if(stream->writeFlag(mask & Message2Mask))
///          stream->writeString(message2);
///
///       // the return value from packUpdate can set which states still
///       // need to be updated for this object.
///       return 0;
///    }
/// @endcode
///
/// Here's half of the meat of the networking code, the packUpdate() function. (The other half, unpackUpdate(),
/// is shown below.) The comments in the code pretty much explain everything, however, notice that the
/// code follows a pattern of if(writeFlag(mask & StateMask)) { ... write data ... }. The packUpdate()/unpackUpdate()
/// functions are responsible for reading and writing the update flags to the BitStream.  This means the GhostConnection
/// doesn't have to send the 32 bit updateMask with every packet.
///
/// @code
///    void unpackUpdate(GhostConnection *, BitStream *stream)
///    {
///       // the unpackUpdate function must be symmetrical to packUpdate
///       if(stream->readFlag())
///       {
///          stream->readString(message1);
///          logprintf("Got message1: %s", message1);
///       }
///       if(stream->readFlag())
///       {
///          stream->readString(message2);
///          logprintf("Got message2: %s", message2);
///       }
///    }
/// @endcode
///
/// The other half of the networking code in any NetObject, unpackUpdate(). In SimpleNetObject, all
/// the code does is print the new messages to the log; however, in a more advanced object, the code might
/// trigger animations, update complex object properties, or even spawn new objects, based on what packet
/// data is unpacked.
///
/// @code
///    void setMessage1(const char *msg)
///    {
///       setMaskBits(Message1Mask);
///       strcpy(message1, msg);
///    }
///    void setMessage2(const char *msg)
///    {
///       setMaskBits(Message2Mask);
///       strcpy(message2, msg);
///    }
/// @endcode
///
/// Here are the accessors for the two properties. It is good to encapsulate state
/// variables, so that you don't have to remember to make a call to setMaskBits every time you change
/// anything; the accessors can do it for you. In a more complex object, you might need to set
/// multiple mask bits when you change something; this can be done using the | operator, for instance,
/// setMaskBits( Message1Mask | Message2Mask ); if you changed both messages.
///
/// @code
/// TNL_IMPLEMENT_NETOBJECT(SimpleNetObject);
/// @endcode
///
/// Finally, we use the NetObject implementation macro, TNL_IMPLEMENT_NETOBJECT(), to implement our
/// NetObject. It is important that we use this, as it makes TNL perform certain initialization tasks
/// that allow us to send the object over the network. TNL_IMPLEMENT_CLASS() doesn't perform these tasks, see
/// the documentation on NetClassRep for more details.
///
/// @nosubgrouping

class NetObject : public Object
{
   friend class GhostConnection;
   friend class GhostAlwaysObjectEvent;
   friend class NetObjectRPCEvent;

   typedef Object Parent;

   NetObject *mPrevDirtyList;
   NetObject *mNextDirtyList;
   U32 mDirtyMaskBits;

   static NetObject *mDirtyList;
   U32 mNetIndex;              ///< The index of this ghost on the other side of the connection.
   GhostInfo *mFirstObjectRef; ///< Head of the linked list of GhostInfos for this object.

   static bool mIsInitialUpdate; ///< Managed by GhostConnection - set to true when this is an initial update
   SafePtr<NetObject> mServerObject; ///< Direct pointer to the parent object on the server if it is a local connection
   GhostConnection *mOwningConnection; ///< The connection that owns this ghost, if it's a ghost
protected:
   enum NetFlag
   {
      IsGhost =            BIT(1),  ///< Set if this is a ghost.
      ScopeLocal =         BIT(2),  ///< If set, this object ghosts only to the local client.
      Ghostable =          BIT(3),  ///< Set if this object can ghost at all.
      MaxNetFlagBit = 15
   };

   BitSet32 mNetFlags;  ///< Flags field describing this object, from NetFlag.

   /// RPC method source connection
   static GhostConnection *mRPCSourceConnection;

   /// NetObject RPC method destination connection.
   static GhostConnection *mRPCDestConnection;

   /// Returns true if this pack/unpackUpdate is the initial one for the object
   bool isInitialUpdate() { return mIsInitialUpdate; }
public:
   NetObject();
   ~NetObject();


   /// collapseDirtyList pushes all the mDirtyMaskBits down into
   /// the GhostInfo's for each object, and clears out the dirty
   /// list.
   static void collapseDirtyList();

   /// Returns the connection from which the current RPC method originated,
   /// or NULL if not currently within the processing of an RPC method call.
   static GhostConnection *getRPCSourceConnection() { return mRPCSourceConnection; }

   /// Sets the connection to which all NetObject RPCs will be destined.  Calling this function
   /// with a NULL value will target NetObject RPCs to every connection for which that object is
   /// currently ghosted.
   static void setRPCDestConnection(GhostConnection *destConnection) { mRPCDestConnection = destConnection; }

   /// Returns the connection that serves as the destination of NetObject RPC method calls.
   static GhostConnection *getRPCDestConnection() { return mRPCDestConnection; }

   /// onGhostAdd is called on the client side of a connection after
   /// the constructor and after the first call to unpackUpdate (the
   /// initial call).  Returning true signifies no error - returning
   /// false causes the connection to abort.
   virtual bool onGhostAdd(GhostConnection *theConnection);
   virtual void onGhostAddBeforeUpdate(GhostConnection *theConnection);

   /// onGhostRemove is called on the client side before the destructor
   /// when ghost has gone out of scope and is about to be deleted from the client.
   virtual void onGhostRemove();

   /// onGhostAvailable is called on the server side after the server knows that
   /// the ghost is available and addressable via the getGhostIndex().
   virtual void onGhostAvailable(GhostConnection *theConnection);

   /// Notify the network system that one or more of this object's states have
   /// been changed.
   ///
   /// @note This is a server side call. It has no meaning for ghosts.
   void setMaskBits(U32 orMask);

   /// Notify the network system that one or more of its states does not
   /// need to be updated.
   ///
   /// @note This is a server side call. It has no meaning for ghosts.
   void clearMaskBits(U32 orMask);

   /// Called to determine the relative update priority of an object.
   ///
   /// All objects that are in scope and that have out of date
   /// states are queried and sorted by priority before being updated.  If there
   /// is not enough room in a single packet for all out of date objects, the skipped
   /// objects will have an incremented updateSkips the next time that connection
   /// prepares to send a packet. Typically the update priority is scaled by
   /// updateSkips so that as data becomes stale, it becomes more of a priority to
   /// update.
   virtual F32 getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips);

   /// Write the object's state to a packet.
   ///
   /// packUpdate is called on an object when it is to be written into a
   /// packet stream for transmission to the client.  The updateMask parameter
   /// contains the out-of-date state mask for the object.  The initial update mask
   /// for any object to a client will always be 0xFFFFFFFF, signifying that all
   /// states are out of date. It is often useful to check for this mask, to write
   /// one-time initialization information for that object.
   virtual U32  packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);

   /// Unpack data written by packUpdate().
   ///
   /// unpackUpdate is called on the client to read an update out of a
   /// packet's bit stream.  Because the update mask is not encoded by
   /// the network system directly, each unpack update function will have
   /// to determine from the bit stream which states are being updated.
   virtual void unpackUpdate(GhostConnection *connection, BitStream *stream);

   /// For a scope object, determine what is in scope.
   ///
   /// performScopeQuery is called on a NetConnection's scope object
   /// to determine which objects in the world are in scope for that
   /// connection.
   virtual void performScopeQuery(GhostConnection *connection);

   /// getNetIndex returns the index tag used to identify the server copy
   /// of a client object.
   U32 getNetIndex() { return mNetIndex; }

   /// isGhost returns true if this object is a ghost of a server object.
   bool isGhost() const;

   /// isClient provides more readable wrapper around isGhost.
   bool isClient() const;

   /// isServer provides more readable wrapper around isGhost.
   bool isServer() const;


   /// isScopeLocal returns true if this object is scoped always to the
   /// local client.
   bool isScopeLocal() const;

   /// isGhostable returns true if this object can be ghosted to any clients.
   bool isGhostable() const;

   /// Return a hash for this object.
   ///
   /// @note This is based on its location in memory.
   U32 getHashId() const;

   /// Internal method called by NetObject RPC events when they are packed.
   void postRPCEvent(NetObjectRPCEvent *theEvent);

};

inline bool NetObject::isGhost() const
{
    return mNetFlags.test(IsGhost);
}


// For readability
inline bool NetObject::isClient() const { return  isGhost(); }
inline bool NetObject::isServer() const { return !isGhost(); }


inline bool NetObject::isScopeLocal() const
{
    return mNetFlags.test(ScopeLocal);
}

inline bool NetObject::isGhostable() const
{
    return mNetFlags.test(Ghostable);
}

// New method gives same results as old, but without the type-punning
inline U32 NetObject::getHashId() const
{
   union {
      const NetObject *ptr;
      U32 hash;
   };

   ptr = this;
   return hash;
}



/// The TNL_IMPLEMENT_NETOBJECT macro should be used for all subclasses of NetObject that
/// will be transmitted with the ghosting system.
#define TNL_IMPLEMENT_NETOBJECT(className) \
   TNL::NetClassRep* className::getClassRep() const { return &className::dynClassRep; } \
   TNL::NetClassRepInstance<className> className::dynClassRep(#className, TNL::NetClassGroupGameMask, TNL::NetClassTypeObject, 0)

#define TNL_IMPLEMENT_NETOBJECT_VERSION(className, version) \
   TNL::NetClassRep* className::getClassRep() const { return &className::dynClassRep; } \
   TNL::NetClassRepInstance<className> className::dynClassRep(#className, TNL::NetClassGroupGameMask, TNL::NetClassTypeObject, version)

/// Direction that a NetObject RPC method call should travel.
enum NetObjectRPCDirection {
   RPCToGhost = 1,
   RPCToGhostParent = 2,
};

/// Event class for handling RPC method invocations on NetObjects.
///
/// When an RPC method is called on the server side NetObject, it
/// is broadcast to all connections that have that NetObject in scope.
/// When an RPC method is called on a ghost (on the client), it is
/// handled by the originating server object.
class NetObjectRPCEvent : public RPCEvent
{
public:
   /// Destination object of the RPC invocation
   SafePtr<NetObject> mDestObject;

   /// Direction this RPC should travel
   NetObjectRPCDirection mRPCDirection;

   /// Constructor - initializes the base class's data
   NetObjectRPCEvent(NetObject *destObject, RPCGuaranteeType type, NetObjectRPCDirection dir) :
      RPCEvent(type, RPCDirAny) { mDestObject = destObject; mRPCDirection = dir; }
   void pack(EventConnection *ps, BitStream *bstream);
   void unpack(EventConnection *ps, BitStream *bstream);
   void process(EventConnection *ps);
};

/// Macro used to declare the implementation of an RPC method on a NetObject subclass.
///
/// The macro should be used in place of a member function parameter declaration,
/// with the body code (to be executed on the remote side of the RPC) immediately
/// following the TNL_IMPLEMENT_NETOBJECT_RPC macro call.
#define TNL_IMPLEMENT_NETOBJECT_RPC(className, name, args, argNames, groupMask, guaranteeType, eventDirection, rpcVersion) \
class RPCEV_##className##_##name : public TNL::NetObjectRPCEvent { \
public: \
   TNL::FunctorDecl<void (className::*)args> mFunctorDecl;\
   RPCEV_##className##_##name(TNL::NetObject *theObject = NULL) : TNL::NetObjectRPCEvent(theObject, guaranteeType, eventDirection), mFunctorDecl(&className::name##_remote) { mFunctor = &mFunctorDecl; } \
   TNL_DECLARE_CLASS( RPCEV_##className##_##name ); \
   bool checkClassType(TNL::Object *theObject) { return dynamic_cast<className *>(theObject) != NULL; } }; \
   TNL_IMPLEMENT_NETEVENT( RPCEV_##className##_##name, groupMask, rpcVersion ); \
   void className::name args { RPCEV_##className##_##name *theEvent = new RPCEV_##className##_##name(this); theEvent->mFunctorDecl.set argNames ; postRPCEvent(theEvent); } \
   TNL::NetEvent * className::name##_construct args { RPCEV_##className##_##name *theEvent = new RPCEV_##className##_##name(this); theEvent->mFunctorDecl.set argNames ; return theEvent; } \
   void className::name##_remote args

};

#endif
