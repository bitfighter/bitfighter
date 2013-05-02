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

#ifndef TNLUDP_H_
#define TNLUDP_H_

#ifndef _TNL_TYPES_H_
#include "tnlTypes.h"
#endif

#ifndef _TNL_VECTOR_H_
#include "tnlVector.h"
#endif

// JMQ: hate...Xwindows...headers
#if defined(TNL_OS_LINUX) && defined(None)
#undef None
#endif

#include <string>


namespace TNL {
/// @}
//------------------------------------------------------------------------------

/// Enumeration for transport protocols for the TNL.
/// Currently only IP and IPX are supported - the IPv6 constant
/// is for future use.
enum TransportProtocol
{
   IPProtocol,    ///< The standard Internet routing protocol
   TCPProtocol,   ///< The standard Internet guaranteed delivery protocol
   IPXProtocol,   ///< Novell's IPX protocol
   IPv6Protocol,  ///< The next generation 128-bit address internet protocol (not currently supported by TNL)
};

struct IPAddress;

/// Representation of a network address.
struct Address
{
private:
   bool mIsValid;

public:
   /// One of: IPXAddress, IPAddress
   U16 transport;
   U16 port;         ///< <b>For IP:</b> sin_port <b>For IPX:</b> sa_socket
   U32 netNum[4];    ///< Address data, sized for future IPv6 usage

   /// Constants specify
   enum NamedAddress {
      None,
      Localhost,
      Broadcast,
      Any,
   };

   /// Constructs an address using a protocol, named address type and port
   Address(TransportProtocol type = IPProtocol, NamedAddress name = Any, U16 port = 0);

   /// Constructs an address from a string, of the form "transport:address:port"
   /// where transport is one of ip, ipx, or (in the future) ipv6
   Address(const char *string)
   {
      set(string);
   }

   /// Constructs an address from an IP address.
   Address(const IPAddress &theAddress)
   {
      set(theAddress);
   }

   virtual bool isValid() { return mIsValid; }

   /// Sets the address to the specified string, returning true if
   /// the string was a valid address.  Note that this call may block
   /// if the address portion of the string requires a DNS lookup.
   bool set(const char *string);
   bool set(std::string);                // The same, but with string parameter

   /// Sets the address to the specified IPAddress.
   bool set(const IPAddress &address);

   /// returns the formatted string corresponding to the address.
   const char *toString() const;

   /// returns true if theAddress is equal to this.
   bool operator==(const Address &theAddress) const;

   /// returns true if theAddress is not equal to this.
   bool operator!=(const Address &theAddress) const { return !operator==(theAddress); }

   /// returns true if the addresses are equal, not including the port.
   bool isEqualAddress(const Address &theAddress) const {
      return transport == theAddress.transport &&
             netNum[0] == theAddress.netNum[0] &&
             netNum[1] == theAddress.netNum[1] &&
             netNum[2] == theAddress.netNum[2] &&
             netNum[3] == theAddress.netNum[3]; }

   /// Returns a 32-bit hash of this network address.
   U32 hash() const { return netNum[0] ^ (U32(port) << 8) ^ (netNum[1] << 16) ^ (netNum[1] >> 16) ^ (netNum[2] << 5); }

   /// Returns a packed IPAddress of this address
   IPAddress toIPAddress() const;
};

/// Packed representaion of an IPAddress.
struct IPAddress
{
   U32 netNum; ///< Address of the host in IP address format.
   U16 port;   ///< Port field of the network address.
};

typedef const IPAddress &IPAddressRef;

/// Packet level network constants
const U32 MaxPacketDataSize = 1500;  ///< The maximum size of a data packet sent through the Net::sendto and Net::recvfrom functions
const U32 MaxPreferredPacketDataSize = 576;  ///< The maximum preferred size for sending


/// Error conditions that can be returned by sendto and recvfrom.
enum NetError {
   NoError,               ///< The operation succeeded without error.
   InvalidPacketProtocol, ///< The program attempted to send a packet to an address using a socket that didn't support the protocol of the address.
   WouldBlock,            ///< The operation would have blocked, for example if there was no data to read.
   UnknownError,          ///< There was some other, unknown error.
};

/// The Socket class encapsulates a platform's network socket.
class Socket
{
   S32 mPlatformSocket;    ///< The OS-level socket
   U32 mTransportProtocol; ///< The transport type this socket uses.
public:
   enum {
      DefaultBufferSize = 32768, ///< The default send and receive buffer sizes
   };

   /// Opens a socket on the specified address/port
   ///
   /// A connectPort of 0 will bind to any available port.
   /// Passing a valid address for ipBindInterface will attempt to bind this socket to a particular IP address on the local machine.
   Socket(const Address &bindAddress, U32 sendBufferSize = DefaultBufferSize, U32 recvBufferSize = DefaultBufferSize, bool acceptsBroadcast = true, bool nonblockingIO = true);

   /// Closes the socket.
   ~Socket();

   /// Returns true if the socket was created successfully.
   bool isValid();

   /// Sends a packet to the address through sourceSocket.
   NetError sendto(const Address &address, const U8 *buffer, S32 bufferSize);

   /// Read an incoming packet.
   ///
   /// @param   address         Address originating the packet.
   /// @param   buffer          Buffer in to which to read the packet.
   /// @param   bufferSize      Size of the buffer (this prevents buffer overflows!).
   /// @param   bytesRead       Specifies the number of bytes which were actually in the packet.
   NetError recvfrom(Address *address, U8 *buffer, S32 bufferSize, S32 *bytesRead);

   /// Returns the Address corresponding to this socket, as bound on the local machine.
   Address getBoundAddress();

   /// Returns the list of network addresses this host can be bound to.  Currently this only
   /// returns IP addresses, with the port field set to 0.
   static void getInterfaceAddresses(Vector<Address> *addressVector);

   virtual NetError connect(const Address &theAddress);
   virtual NetError recv(U8 *buffer, S32 bufferSize, S32 *bytesRead);
   virtual NetError send(const U8 *buffer, S32 bufferSize);
};

//inline void read(BitStream &s, IPAddress *val)
//{
//   s.read(&val->netNum);
//   s.read(&val->port);
//}
//
//inline void write(BitStream &s, const IPAddress &val)
//{
//   s.write(val.netNum);
//   s.write(val.port);
//}

};
#endif /* TNLUDP_H_ */
