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

#ifndef _TNL_NETINTERFACE_H_
#define _TNL_NETINTERFACE_H_

#ifndef _TNL_VECTOR_H_
#include "tnlVector.h"
#endif

#ifndef _TNL_NETBASE_H_
#include "tnlNetBase.h"
#endif

#include "tnlClientPuzzle.h"

#ifndef _TNL_NETOBJECT_H_
#include "tnlNetObject.h"
#endif

#ifndef _TNL_NETCONNECTION_H_
#include "tnlNetConnection.h"
#endif

namespace TNL {

class AsymmetricKey;
class Certificate;
struct ConnectionParameters;

/// NetInterface class.
///
/// Manages all valid and pending notify protocol connections for a port/IP. If you are
/// providing multiple services or servicing multiple networks, you may have more than
/// one NetInterface.
///
/// <b>Connection handshaking basic overview:</b>
///
/// TNL does a two phase connect handshake to prevent a several types of
/// Denial-of-Service (DoS) attacks.
///
/// The initiator of the connection (client) starts the connection by sending
/// a unique random nonce (number, used once) value to the server as part of
/// the ConnectChallengeRequest packet.
/// C->S: ConnectChallengeRequest, Nc
///
/// The server responds to the ConnectChallengeRequest with a "Client Puzzle"
/// that has the property that verifying a solution to the puzzle is computationally
/// simple, but can be of a specified computational, brute-force difficulty to
/// compute the solution itself.  The client puzzle is of the form:
/// secureHash(Ic, Nc, Ns, X) = Y >> k, where Ic is the identity of the client,
/// and X is a value computed by the client such that the high k bits of the value
/// y are all zero.  The client identity is computed by the server as a partial hash
/// of the client's IP address and port and some random data on the server.
/// its current nonce (Ns), Nc, k, and the server's authentication certificate.
/// S->C: ConnectChallengeResponse, Nc, Ns, Ic, Cs
///
/// The client, upon receipt of the ConnectChallengeResponse, validates the packet
/// sent by the server and computes a solution to the puzzle the server sent.  If
/// the connection is to be authenticated, the client can also validate the server's
/// certificate (if it's been signed by a Certificate Authority), and then generates
/// a shared secret from the client's key pair and the server's public key.  The client
/// response to the server consists of:
/// C->S: ConnectRequest, Nc, Ns, X, Cc, sharedSecret(key1, sequence1, NetConnectionClass, class-specific sendData)
///
/// The server then can validation the solution to the puzzle the client submitted, along
/// with the client identity (Ic).
/// Until this point the server has allocated no memory for the client and has
/// verified that the client is sending from a valid IP address, and that the client
/// has done some amount of work to prove its willingness to start a connection.
/// As the server load increases, the server may choose to increase the difficulty (k) of
/// the client puzzle, thereby making a resource depletion DoS attack successively more
/// difficult to launch.
///
/// If the server accepts the connection, it sends a connect accept packet that is
/// encrypted and hashed using the shared secret.  The contents of the packet are
/// another sequence number (sequence2) and another key (key2).  The sequence numbers 
/// are the initial send and receive sequence numbers for the connection, and the
/// key2 value becomes the IV of the symmetric cipher.  The connection subclass is
/// also allowed to write any connection specific data into this packet.
///
/// This system can operate in one of 3 ways: unencrypted, encrypted key exchange (ECDH),
/// or encrypted key exchange with server and/or client signed certificates (ECDSA).
/// 
/// The unencrypted communication mode is NOT secure.  Packets en route between hosts
/// can be modified without detection by the hosts at either end.  Connections using
/// the secure key exchange are still vulnerable to Man-in-the-middle attacks, but still
/// much more secure than the unencrypted channels.  Using certificate(s) signed by a
/// trusted certificate authority (CA), makes the communications channel as securely
/// trusted as the trust in the CA.
///
/// <b>Arranged Connection handshaking:</b>
///
/// NetInterface can also facilitate "arranged" connections.  Arranged connections are
/// necessary when both parties to the connection are behind firewalls or NAT routers.
/// Suppose there are two clients, A and B that want to esablish a direct connection with
/// one another.  If A and B are both logged into some common server S, then S can send
/// A and B the public (NAT'd) address, as well as the IP addresses each client detects
/// for itself.
///
/// A and B then both send "Punch" packets to the known possible addresses of each other.
/// The punch packet client A sends enables the punch packets client B sends to be 
/// delivered through the router or firewall since it will appear as though it is a service
/// response to A's initial packet.
///
/// Upon receipt of the Punch packet by the "initiator"
/// of the connection, an ArrangedConnectRequest packet is sent.
/// if the non-initiator of the connection gets an ArrangedPunch
/// packet, it simply sends another Punch packet to the
/// remote host, but narrows down its Address range to the address
/// it received the packet from.
/// The ArrangedPunch packet from the intiator contains the nonce 
/// for the non-initiator, and the nonce for the initiator encrypted
/// with the shared secret.
/// The ArrangedPunch packet for the receiver of the connection
/// contains all that, plus the public key/keysize or the certificate
/// of the receiver.


class NetInterface : public Object
{
   friend class NetConnection;
public:
   /// PacketType is encoded as the first byte of each packet.
   ///
   /// Subclasses of NetInterface can add custom, non-connected data
   /// packet types starting at FirstValidInfoPacketId, and overriding 
   /// handleInfoPacket to process them.
   ///
   /// Packets that arrive with the high bit of the first byte set
   /// (i.e. the first unsigned byte is greater than 127), are
   /// assumed to be connected protocol packets, and are dispatched to
   /// the appropriate connection for further processing.

   enum PacketType
   {
      ConnectChallengeRequest       = 0, ///< Initial packet of the two-phase connect process
      ConnectChallengeResponse      = 1, ///< Response packet to the ChallengeRequest containing client identity, a client puzzle, and possibly the server's public key.
      ConnectRequest                = 2, ///< A connect request packet, including all puzzle solution data and connection initiation data.
      ConnectReject                 = 3, ///< A packet sent to notify a host that a ConnectRequest was rejected.
      ConnectAccept                 = 4, ///< A packet sent to notify a host that a connection was accepted.
      Disconnect                    = 5, ///< A packet sent to notify a host that the specified connection has terminated.
      Punch                         = 6, ///< A packet sent in order to create a hole in a firewall or NAT so packets from the remote host can be received.
      ArrangedConnectRequest        = 7, ///< A connection request for an "arranged" connection.
      FirstValidInfoPacketId        = 8, ///< The first valid ID for a NetInterface subclass's info packets.
   };

protected:
   Vector<NetConnection *> mConnectionList;      ///< List of all the connections that are in a connected state on this NetInterface.
   Vector<NetConnection *> mConnectionHashTable; ///< A resizable hash table for all connected connections.  This is a flat hash table (no buckets).

   Vector<NetConnection *> mPendingConnections; ///< List of connections that are in the startup state, where the remote host has not fully
                                                ///  validated the connection.

   RefPtr<AsymmetricKey> mPrivateKey;  ///< The private key used by this NetInterface for secure key exchange.
   RefPtr<Certificate> mCertificate;   ///< A certificate, signed by some Certificate Authority, to authenticate this host.
   ClientPuzzleManager mPuzzleManager; ///< The object that tracks the current client puzzle difficulty, current puzzle and solutions for this NetInterface.

   /// @name NetInterfaceSocket Socket
   ///
   /// State regarding the socket this NetInterface controls.
   ///
   /// @{

   ///
   Socket    mSocket;   ///< Network socket this NetInterface communicates over.

   /// @}

   U32 mCurrentTime;            ///< Current time tracked by this NetInterface.
   bool mRequiresKeyExchange;   ///< True if all connections outgoing and incoming require key exchange.
   U32  mLastTimeoutCheckTime;  ///< Last time all the active connections were checked for timeouts.
   U8  mRandomHashData[12];    ///< Data that gets hashed with connect challenge requests to prevent connection spoofing.
   bool mAllowConnections;      ///< Set if this NetInterface allows connections from remote instances.

   /// Structure used to track packets that are delayed in sending for simulating a high-latency connection.
   ///
   /// The DelaySendPacket is allocated as sizeof(DelaySendPacket) + packetSize;
   struct DelaySendPacket
   {
      DelaySendPacket *nextPacket; ///< The next packet in the list of delayed packets.
      Address remoteAddress;       ///< The address to send this packet to.
      U32 sendTime;                ///< Time when we should send the packet.
      U32 packetSize;              ///< Size, in bytes, of the packet data.
      SafePtr<NetConnection> receiveTo; // Used if delayed receiving
      bool isReceive;
      U8 packetData[1];            ///< Packet data.
   };
   DelaySendPacket *mSendPacketList; ///< List of delayed packets pending to send.

   enum NetInterfaceConstants {
      ChallengeRetryCount = 4,     ///< Number of times to send connect challenge requests before giving up.
      ChallengeRetryTime = 2500,   ///< Timeout interval in milliseconds before retrying connect challenge.

      ConnectRetryCount = 4,       ///< Number of times to send connect requests before giving up.
      ConnectRetryTime = 2500,     ///< Timeout interval in milliseconds before retrying connect request.

      PunchRetryCount = 6,         ///< Number of times to send groups of firewall punch packets before giving up.
      PunchRetryTime = 2500,       ///< Timeout interval in milliseconds before retrying punch sends.

      TimeoutCheckInterval = 1500, ///< Interval in milliseconds between checking for connection timeouts.
      PuzzleSolutionTimeout = 30000, ///< If the server gives us a puzzle that takes more than 30 seconds, time out.
   };

   /// Computes an identity token for the connecting client based on the address of the client and the
   /// client's unique nonce value.
   U32 computeClientIdentityToken(const Address &theAddress, const Nonce &theNonce);

   /// Finds a connection instance that this NetInterface has initiated.
   NetConnection *findPendingConnection(const Address &address);

   /// Adds a connection the list of pending connections.
   void addPendingConnection(NetConnection *conn);

   /// Removes a connection from the list of pending connections.
   void removePendingConnection(NetConnection *conn);

   /// Finds a connection by address from the pending list and removes it.
   void findAndRemovePendingConnection(const Address &address);

   /// Adds a connection to the internal connection list.
   void addConnection(NetConnection *connection);

   /// Remove a connection from the list.
   void removeConnection(NetConnection *connection);

   /// Begins the connection handshaking process for a connection.  Called from NetConnection::connect()
   void startConnection(NetConnection *conn);

   /// Sends a connect challenge request on behalf of the connection to the remote host.
   void sendConnectChallengeRequest(NetConnection *conn);

   /// Handles a connect challenge request by replying to the requestor of a connection with a
   /// unique token for that connection, as well as (possibly) a client puzzle (for DoS prevention),
   /// or this NetInterface's public key.
   void handleConnectChallengeRequest(const Address &addr, BitStream *stream);

   /// Sends a connect challenge request to the specified address.  This can happen as a result
   /// of receiving a connect challenge request, or during an "arranged" connection for the non-initiator
   /// of the connection.
   void sendConnectChallengeResponse(const Address &addr, Nonce &clientNonce, bool wantsKeyExchange, bool wantsCertificate);

   /// Processes a ConnectChallengeResponse, by issueing a connect request if it was for
   /// a connection this NetInterface has pending.
   void handleConnectChallengeResponse(const Address &address, BitStream *stream);

   /// Continues computation of the solution of a client puzzle, and issues a connect request
   /// when the solution is found.
   void continuePuzzleSolution(NetConnection *conn);

   /// Sends a connect request on behalf of a pending connection.
   void sendConnectRequest(NetConnection *conn);

   /// Handles a connection request from a remote host.
   ///
   /// This will verify the validity of the connection token, as well as any solution
   /// to a client puzzle this NetInterface sent to the remote host.  If those tests
   /// pass, it will construct a local connection instance to handle the rest of the
   /// connection negotiation.
   void handleConnectRequest(const Address &address, BitStream *stream);

   /// Sends a connect accept packet to acknowledge the successful acceptance of a connect request.
   void sendConnectAccept(NetConnection *conn);

   /// Handles a connect accept packet, putting the connection associated with the
   /// remote host (if there is one) into an active state.
   void handleConnectAccept(const Address &address, BitStream *stream);

   /// Sends a connect rejection to a valid connect request in response to possible error
   /// conditions (server full, wrong password, etc).
   void sendConnectReject(ConnectionParameters *theParams, const Address &theAddress, NetConnection::TerminationReason reason);

   /// Handles a connect rejection packet by notifying the connection object
   /// that the connection was rejected.
   void handleConnectReject(const Address &address, BitStream *stream);

   /// Begins the connection handshaking process for an arranged connection.
   void startArrangedConnection(NetConnection *conn);

   /// Sends Punch packets to each address in the possible connection address list.
   void sendPunchPackets(NetConnection *conn);

   /// Handles an incoming Punch packet from a remote host.
   void handlePunch(const Address &theAddress, BitStream *stream);

   /// Sends an arranged connect request.
   void sendArrangedConnectRequest(NetConnection *conn);

   /// Handles an incoming connect request from an arranged connection.
   void handleArrangedConnectRequest(const Address &theAddress, BitStream *stream);
   
   /// Dispatches a disconnect packet for a specified connection.
   void handleDisconnect(const Address &address, BitStream *stream);

   /// Handles an error reported while reading a packet from this remote connection.
   void handleConnectionError(NetConnection *theConnection, const char *errorString);

   /// Disconnects the given connection and removes it from the NetInterface
   void disconnect(NetConnection *conn, NetConnection::TerminationReason reason, const char *reasonString);
   /// @}
public:
   /// @param   bindAddress    Local network address to bind this interface to.
   NetInterface(const Address &bindAddress);
   ~NetInterface();

   /// Returns the address of the first network interface in the list that the socket on this NetInterface is bound to.
   Address getFirstBoundInterfaceAddress();

   /// Sets the private key this NetInterface will use for authentication and key exchange
   void setPrivateKey(AsymmetricKey *theKey);

   /// Requires that all connections use encryption and key exchange
   void setRequiresKeyExchange(bool requires) { mRequiresKeyExchange = requires; }

   /// Sets the public certificate that validates the private key and stores
   /// information about this host.  If no certificate is set, this interface can
   /// still initiate and accept encrypted connections, but they will be vulnerable to
   /// man in the middle attacks, unless the remote host can validate the public key
   /// in another way.
   void setCertificate(Certificate *theCertificate);

   /// Returns whether or not this NetInterface allows connections from remote hosts.
   bool doesAllowConnections() { return mAllowConnections; }

   /// Sets whether or not this NetInterface allows connections from remote hosts.
   void setAllowsConnections(bool conn) { mAllowConnections = conn; }

   /// Returns the Socket associated with this NetInterface
   Socket &getSocket() { return mSocket; }

   /// Sends a packet to the remote address over this interface's socket.
   NetError sendto(const Address &address, BitStream *stream);

   /// Sends a packet to the remote address after millisecondDelay time has elapsed.
   ///
   /// This is used to simulate network latency on a LAN or single computer.
   void sendtoDelayed(const Address *address, NetConnection *receiveTo, BitStream *stream, U32 millisecondDelay);

   /// Dispatch function for processing all network packets through this NetInterface.
   void checkIncomingPackets();

   /// Processes a single packet, and dispatches either to handleInfoPacket or to
   /// the NetConnection associated with the remote address.
   virtual void processPacket(const Address &address, BitStream *packetStream);

   /// Handles all packets that don't fall into the category of connection handshake or game data.
   virtual void handleInfoPacket(const Address &address, U8 packetType, BitStream *stream);

   /// Checks all connections on this interface for packet sends, and for timeouts and all valid
   /// and pending connections.
   void processConnections();

   /// Returns the list of connections on this NetInterface.
   Vector<NetConnection *> &getConnectionList() { return mConnectionList; }

   /// looks up a connected connection on this NetInterface
   NetConnection *findConnection(const Address &remoteAddress);

   /// returns the current process time for this NetInterface
   U32 getCurrentTime() { return mCurrentTime; }
};

};

#endif
