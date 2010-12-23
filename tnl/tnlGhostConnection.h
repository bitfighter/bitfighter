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

#ifndef _TNL_GHOSTCONNECTION_H_
#define _TNL_GHOSTCONNECTION_H_

#ifndef _TNL_EVENTCONNECTION_H_
#include "tnlEventConnection.h"
#endif

#ifndef _TNL_RPC_H_
#include "tnlRPC.h"
#endif

namespace TNL {

struct GhostInfo;

/// GhostConnection is a subclass of EventConnection that manages the transmission
/// (ghosting) and updating of NetObjects over a connection.
///
/// The GhostConnection is responsible for doing scoping calculations (on the server side)
/// and transmitting most-recent ghost information to the client.
///
/// Ghosting is the most complex, and most powerful, part of TNL's capabilities. It
/// allows the information sent to clients to be very precisely matched to what they need, so that
/// no excess bandwidth is wasted.  Each GhostConnection has a <b>scope object</b> that is responsible
/// for determining what other NetObject instances are relevant to that connection's client.  Each time
/// GhostConnection sends a packet, NetObject::performScopeQuery() is called on the scope object, which
/// calls GhostConnection::objectInScope() for each relevant object.
///
/// Each object that is in scope, and in need of update (based on its maskbits) is given a priority
/// ranking by calling that object's getUpdatePriority() method.  The packet is then filled with
/// updates, ordered by priority. This way the most important updates get through first, with less
/// important updates being sent as space is available.
///
/// There is a cap on the maximum number of ghosts that can be active through a GhostConnection at once.
/// The enum GhostIdBitSize (defaults to 10) determines how many bits will be used to transmit the ID for
/// each ghost, so the maximum number is 2^GhostIdBitSize or 1024.  This can be easily raised; see the
/// GhostConstants enum.
///
/// Each object ghosted is assigned a ghost ID; the client is <b>only</b> aware of the ghost ID. This acts
/// to enhance simulation security, as it becomes difficult to map objects from one connection to another,
/// or to reliably identify objects from ID alone. IDs are also reassigned based on need, making it hard
/// to track objects that have fallen out of scope (as any object which the player shouldn't see would).
///
/// resolveGhost() is used on the client side, and resolveObjectFromGhostIndex() on the server side, to
/// convert ghost IDs to object references.
///
/// @see NetObject for more information on network object functionality.
class GhostConnection : public EventConnection
{
   typedef EventConnection Parent;
   friend class ConnectionMessageEvent;
public:
   /// GhostRef tracks an update sent in one packet for the ghost of one NetObject.
   ///
   /// When we are notified that a pack is sent/lost, this is used to determine what
   /// updates need to be resent and so forth.
   struct GhostRef
   {
      U32 mask;              ///< The mask of bits that were updated in this packet
      U32 ghostInfoFlags;    ///< GhostInfo::Flags bitset, determes if the ghost is in a
                             ///  special processing mode (created/deleted)
      GhostInfo *ghost;      ///< The ghost information for the object on the connection that sent
                             ///  the packet this GhostRef is attached to
      GhostRef *nextRef;     ///< The next ghost updated in this packet
      GhostRef *updateChain; ///< A pointer to the GhostRef on the least previous packet that
                             ///  updated this ghost, or NULL, if no prior packet updated this ghost
   };

   /// Notify structure attached to each packet with information about the ghost updates in the packet
   struct GhostPacketNotify : public EventConnection::EventPacketNotify
   {
      GhostRef *ghostList; ///< list of ghosts updated in this packet
      GhostPacketNotify() { ghostList = NULL; }
   };

protected:

   /// Override of EventConnection's allocNotify, to use the GhostPacketNotify structure.
   PacketNotify *allocNotify() { return new GhostPacketNotify; }

   /// Override to properly update the GhostInfo's for all ghosts that had upates in the dropped packet.
   void packetDropped(PacketNotify *notify);

   /// Override to update flags associated with the ghosts updated in this packet.
   void packetReceived(PacketNotify *notify);

   /// Performs the scoping query in order to determine if there is data to send from this GhostConnection.
   void prepareWritePacket();

   /// Override to write ghost updates into each packet.
   void writePacket(BitStream *bstream, PacketNotify *notify);

   /// Override to read updated ghost information from the packet stream.
   void readPacket(BitStream *bstream);

   /// Override to check if there is data pending on this GhostConnection.
   bool isDataToTransmit();

//----------------------------------------------------------------
// ghost manager functions/code:
//----------------------------------------------------------------

protected:
   GhostInfo **mGhostArray;   ///< Array of GhostInfo structures used to track all the objects ghosted by this side of the connection.
                              ///
                              ///  For efficiency, ghosts are stored in three segments - the first segment contains GhostInfos
                              ///  that have pending updates, the second ghostrefs that need no updating, and last, free
                              ///  GhostInfos that may be reused.

   S32 mGhostZeroUpdateIndex; ///< Index in mGhostArray of first ghost with 0 update mask (ie, with no updates).
   S32 mGhostFreeIndex;       ///< index in mGhostArray of first free ghost.

   bool mGhosting;         ///< Am I currently ghosting objects over?
   bool mScoping;          ///< Am I currently allowing objects to be scoped?
   U32  mGhostingSequence; ///< Sequence number describing this ghosting session.

   NetObject **mLocalGhosts;        ///< Local ghost array for remote objects, or NULL if mGhostTo is false.

   GhostInfo *mGhostRefs;           ///< Allocated array of ghostInfos, or NULL if mGhostFrom is false.
   GhostInfo **mGhostLookupTable;   ///< Table indexed by object id->GhostInfo, or NULL if mGhostFrom is false.

   SafePtr<NetObject> mScopeObject; ///< The local NetObject that performs scoping queries to determine what
                                    ///  objects to ghost to the client.

   void clearGhostInfo();
   void deleteLocalGhosts();
   bool validateGhostArray();

   void freeGhostInfo(GhostInfo *);

   /// Notifies subclasses that the remote host is about to start ghosting objects.
   virtual void onStartGhosting();                              

   /// Notifies subclasses that the server has stopped ghosting objects on this connection.
   virtual void onEndGhosting();

public:
   GhostConnection();
   ~GhostConnection();

   void setGhostFrom(bool ghostFrom); ///< Sets whether ghosts transmit from this side of the connection.
   void setGhostTo(bool ghostTo);     ///< Sets whether ghosts are allowed from the other side of the connection.

   bool doesGhostFrom() { return mGhostArray != NULL; } ///< Does this GhostConnection ghost NetObjects to the remote host?
   bool doesGhostTo() { return mLocalGhosts != NULL; }  ///< Does this GhostConnection receive ghosts from the remote host?

   /// Returns the sequence number of this ghosting session.
   U32 getGhostingSequence() { return mGhostingSequence; }

   enum GhostConstants {
      GhostIdBitSize = 10,            ///< Size, in bits, of the integer used to transmit ghost IDs
      GhostLookupTableSizeShift = 10, ///< The size of the hash table used to lookup source NetObjects by remote ghost ID is 1 << GhostLookupTableSizeShift.

      MaxGhostCount = (1 << GhostIdBitSize),   ///< Maximum number of ghosts that can be active at any one time.
      GhostCountBitSize = GhostIdBitSize + 1,  ///< Size of the field needed to transmit the total number of ghosts.

      GhostLookupTableSize = (1 << GhostLookupTableSizeShift), ///< Size of the hash table used to lookup source NetObjects by remote ghost ID.
      GhostLookupTableMask = (GhostLookupTableSize - 1),       ///< Hashing mask for table lookups.

   };

   void setScopeObject(NetObject *object);                           ///< Sets the object that is queried at each packet to determine
                                                                     ///  what NetObjects should be ghosted on this connection.
   NetObject *getScopeObject() { return (NetObject*)mScopeObject; }; ///< Returns the current scope object.

   void objectInScope(NetObject *object);          ///< Indicate that the specified object is currently in scope.
                                                   ///
                                                   ///  Method called by the scope object to indicate that the specified object is in scope.
   void objectLocalScopeAlways(NetObject *object); ///< The specified object should be always in scope for this connection.
   void objectLocalClearAlways(NetObject *object); ///< The specified object should not be always in scope for this connection.

   NetObject *resolveGhost(S32 id);                  ///< Given an object's ghost id, returns the ghost of the object (on the client side).
   NetObject *resolveGhostParent(S32 id);            ///< Given an object's ghost id, returns the source object (on the server side).
   void ghostPushNonZero(GhostInfo *gi);             ///< Moves the specified GhostInfo into the range of the ghost array for non-zero updateMasks.
   void ghostPushToZero(GhostInfo *gi);              ///< Moves the specified GhostInfo into the range of the ghost array for zero updateMasks.
   void ghostPushZeroToFree(GhostInfo *gi);          ///< Moves the specified GhostInfo into the range of the ghost array for free (unused) GhostInfos.
   inline void ghostPushFreeToZero(GhostInfo *info); ///< Moves the specified GhostInfo from the free area into the range of the ghost array for zero updateMasks.

   S32 getGhostIndex(NetObject *object); ///< Returns the client-side ghostIndex of the specified server object, or -1 if the object is not available on the client.

   /// Returns true if the object is available on the client.
   bool isGhostAvailable(NetObject *object) { return getGhostIndex(object) != -1; }

   void resetGhosting();                   ///< Stops ghosting objects from this GhostConnection to the remote host, which causes all ghosts to be destroyed on the client.
   void activateGhosting();                ///< Begins ghosting objects from this GhostConnection to the remote host, starting with the GhostAlways objects.
   bool isGhosting() { return mGhosting; } ///< Returns true if this connection is currently ghosting objects to the remote host.

   void detachObject(GhostInfo *info);                      ///< Notifies the GhostConnection that the specified GhostInfo should no longer be scoped to the client.

   /// RPC from server to client before the GhostAlwaysObjects are transmitted
   TNL_DECLARE_RPC(rpcStartGhosting, (U32 sequence));

   /// RPC from client to server sent when the client receives the rpcGhostAlwaysActivated
   TNL_DECLARE_RPC(rpcReadyForNormalGhosts, (U32 sequence));

   /// RPC from server to client sent to notify that ghosting should stop
   TNL_DECLARE_RPC(rpcEndGhosting, ());
};

//----------------------------------------------------------------------------

/// Each GhostInfo structure tracks the state of a single NetObject's ghost for a single GhostConnection.
struct GhostInfo
{
   // NOTE:
   // if the size of this structure changes, the
   // NetConnection::getGhostIndex function MUST be changed
   // to reflect.

   NetObject *obj; ///< The real object on the server.
   U32 updateMask; ///< The current out-of-date state mask for the object for this connection.
   GhostConnection::GhostRef *lastUpdateChain; ///< The GhostRef for this object in the last packet it was updated in,
                                               ///   or NULL if that last packet has been notified yet.
   GhostInfo *nextObjectRef;  ///< Next GhostInfo for this object in the doubly linked list of GhostInfos across
                              ///  all connections that scope this object
   GhostInfo *prevObjectRef;  ///< Previous GhostInfo for this object in the doubly linked list of GhostInfos across
                              ///  all connections that scope this object

   GhostConnection *connection; ///< The connection that owns this GhostInfo
   GhostInfo *nextLookupInfo;   ///< Next GhostInfo in the hash table for NetObject*->GhostInfo*
   U32 updateSkipCount;         ///< How many times this object has NOT been updated in writePacket

   U32 flags;      ///< Current flag status of this object for this connection.
   F32 priority;   ///< Priority for the update of this object, computed after the scoping process has run.
   U32 index;      ///< Fixed index of the object in the mGhostRefs array for the connection, and the ghostId of the object on the client.
   S32 arrayIndex; ///< Position of the object in the mGhostArray for the connection, which changes as the object is pushed to zero, non-zero and free.

    enum Flags
    {
      InScope = BIT(0),             ///< This GhostInfo's NetObject is currently in scope for this connection.
      ScopeLocalAlways = BIT(1),    ///< This GhostInfo's NetObject is always in scope for this connection.
      NotYetGhosted = BIT(2),       ///< This GhostInfo's NetObject has not been sent to or constructed on the remote host.
      Ghosting = BIT(3),            ///< This GhostInfo's NetObject has been sent to the client, but the packet it was sent in hasn't been acked yet.
      KillGhost = BIT(4),           ///< The ghost of this GhostInfo's NetObject should be destroyed ASAP.
      KillingGhost = BIT(5),        ///< The ghost of this GhostInfo's NetObject is in the process of being destroyed.

      /// Flag mask - if any of these are set, the object is not yet available for ghost ID lookup.
      NotAvailable = (NotYetGhosted | Ghosting | KillGhost | KillingGhost),
    };
};


inline void GhostConnection::ghostPushNonZero(GhostInfo *info)
{
   TNLAssert(info->arrayIndex >= mGhostZeroUpdateIndex && info->arrayIndex < mGhostFreeIndex, "Out of range arrayIndex.");
   TNLAssert(mGhostArray[info->arrayIndex] == info, "Invalid array object.");
   if(info->arrayIndex != mGhostZeroUpdateIndex)
   {
      mGhostArray[mGhostZeroUpdateIndex]->arrayIndex = info->arrayIndex;
      mGhostArray[info->arrayIndex] = mGhostArray[mGhostZeroUpdateIndex];
      mGhostArray[mGhostZeroUpdateIndex] = info;
      info->arrayIndex = mGhostZeroUpdateIndex;
   }
   mGhostZeroUpdateIndex++;
   //TNLAssert(validateGhostArray(), "Invalid ghost array!");
}

inline void GhostConnection::ghostPushToZero(GhostInfo *info)
{
   TNLAssert(info->arrayIndex < mGhostZeroUpdateIndex, "Out of range arrayIndex.");
   TNLAssert(mGhostArray[info->arrayIndex] == info, "Invalid array object.");
   mGhostZeroUpdateIndex--;
   if(info->arrayIndex != mGhostZeroUpdateIndex)
   {
      mGhostArray[mGhostZeroUpdateIndex]->arrayIndex = info->arrayIndex;
      mGhostArray[info->arrayIndex] = mGhostArray[mGhostZeroUpdateIndex];
      mGhostArray[mGhostZeroUpdateIndex] = info;
      info->arrayIndex = mGhostZeroUpdateIndex;
   }
   //TNLAssert(validateGhostArray(), "Invalid ghost array!");
}

inline void GhostConnection::ghostPushZeroToFree(GhostInfo *info)
{
   TNLAssert(info->arrayIndex >= mGhostZeroUpdateIndex && info->arrayIndex < mGhostFreeIndex, "Out of range arrayIndex.");
   TNLAssert(mGhostArray[info->arrayIndex] == info, "Invalid array object.");
   mGhostFreeIndex--;
   if(info->arrayIndex != mGhostFreeIndex)
   {
      mGhostArray[mGhostFreeIndex]->arrayIndex = info->arrayIndex;
      mGhostArray[info->arrayIndex] = mGhostArray[mGhostFreeIndex];
      mGhostArray[mGhostFreeIndex] = info;
      info->arrayIndex = mGhostFreeIndex;
   }
   //TNLAssert(validateGhostArray(), "Invalid ghost array!");
}

inline void GhostConnection::ghostPushFreeToZero(GhostInfo *info)
{
   TNLAssert(info->arrayIndex >= mGhostFreeIndex, "Out of range arrayIndex.");
   TNLAssert(mGhostArray[info->arrayIndex] == info, "Invalid array object.");
   if(info->arrayIndex != mGhostFreeIndex)
   {
      mGhostArray[mGhostFreeIndex]->arrayIndex = info->arrayIndex;
      mGhostArray[info->arrayIndex] = mGhostArray[mGhostFreeIndex];
      mGhostArray[mGhostFreeIndex] = info;
      info->arrayIndex = mGhostFreeIndex;
   }
   mGhostFreeIndex++;
   //TNLAssert(validateGhostArray(), "Invalid ghost array!");
}

};

#endif
