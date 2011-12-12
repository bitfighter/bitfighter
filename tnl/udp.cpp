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
#include "tnlJournal.h"

#if defined ( TNL_OS_XBOX )

#include <xtl.h>

typedef int socklen_t;
#define NO_IPX_SUPPORT

#elif defined ( TNL_OS_WIN32 )
#include <winsock.h>
#if defined(TNL_COMPILER_MINGW)
   // mingw does not include support for IPX sockets
   typedef void* SOCKADDR_IPX;
#define NO_IPX_SUPPORT
#endif

typedef int socklen_t;

#elif defined ( TNL_OS_MAC_OSX )

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>

/* for PROTO_IPX */
#include <sys/ioctl.h>   /* ioctl() */
#define NO_IPX_SUPPORT
typedef sockaddr_in SOCKADDR_IN;
typedef sockaddr * PSOCKADDR;
typedef sockaddr SOCKADDR;
typedef in_addr IN_ADDR;

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

#define closesocket close

#elif defined TNL_OS_LINUX


#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>

/* for PROTO_IPX */
#include <sys/ioctl.h>   /* ioctl() */
#define NO_IPX_SUPPORT
typedef sockaddr_in SOCKADDR_IN;
typedef sockaddr * PSOCKADDR;
typedef sockaddr SOCKADDR;
typedef in_addr IN_ADDR;

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

#define closesocket close

#else

#endif


#include <stdio.h>


#if !defined(NO_IPX_SUPPORT)
#  include <wsipx.h>
#endif

#include "tnlLog.h"

namespace TNL {

static NetError getLastError();
static S32 initCount = 0;

static bool init()
{
   bool success = true;
#if defined ( TNL_OS_WIN32 )
   if(!initCount)
   {
      WSADATA stWSAData;
      success = !WSAStartup(0x0101, &stWSAData);

      if(success)
         logprintf(LogConsumer::LogNetInterface, "Winsock initialization succeeded.");
      else
         logprintf(LogConsumer::LogError, "Winsock initialization failed.");
       
   }
#elif defined( TNL_OS_XBOX )
      XNetStartupParams xnsp;
      memset( &xnsp, 0, sizeof(xnsp) );
      xnsp.cfgSizeOfStruct = sizeof(xnsp);
      xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
      INT iResult = XNetStartup( &xnsp );
      if( iResult != NO_ERROR )
         success = false;


      WSADATA WsaData;
      iResult = WSAStartup( 0x0101, &WsaData );
      if( iResult != NO_ERROR )
         success = false;

      if(success)
         logprintf(LogNetInterface, "Winsock initialization succeeded.")
      else
         logprintf(LogError, "Winsock initialization failed.")

#endif
   initCount++;
   return success;
}

static void shutdown()
{
   initCount--;
#ifdef TNL_OS_WIN32
   if(!initCount)
   {
      WSACleanup();
   }
#endif
}

static void TNLToSocketAddress(const Address &address, SOCKADDR *sockAddr, socklen_t *addressSize)
{
   if(address.transport == IPProtocol || address.transport == TCPProtocol)
   {
      memset(sockAddr, 0, sizeof(SOCKADDR_IN));
      ((SOCKADDR_IN *) sockAddr)->sin_family = AF_INET;
      ((SOCKADDR_IN *) sockAddr)->sin_port = htons(address.port);
      ((SOCKADDR_IN *) sockAddr)->sin_addr.s_addr = htonl(address.netNum[0]);
      *addressSize = sizeof(SOCKADDR_IN);
   }
#ifndef NO_IPX_SUPPORT
   else if(address.transport == IPXProtocol)
   {
      memset(sockAddr, 0, sizeof(SOCKADDR_IPX));
      ((SOCKADDR_IPX *) sockAddr)->sa_family = AF_IPX;
      ((SOCKADDR_IPX *) sockAddr)->sa_socket = htons(address.port);
      writeU32ToBuffer(address.netNum[0], (U8 *) ((SOCKADDR_IPX *) sockAddr)->sa_netnum);
      writeU32ToBuffer(address.netNum[1], (U8 *) ((SOCKADDR_IPX *) sockAddr)->sa_nodenum);
      writeU16ToBuffer(address.netNum[2], (U8 *) ((SOCKADDR_IPX *) sockAddr)->sa_nodenum+4);
      *addressSize = sizeof(SOCKADDR_IPX);
   }
#endif
}

static void SocketToTNLAddress(const SOCKADDR *sockAddr, Address *address)
{
   if(sockAddr->sa_family == AF_INET)
   {
      address->transport = IPProtocol;
      address->port = htons(((SOCKADDR_IN *) sockAddr)->sin_port);
      U32 addr = htonl(((SOCKADDR_IN *) sockAddr)->sin_addr.s_addr);
      address->netNum[0] = addr;
      address->netNum[1] = 0;
      address->netNum[2] = 0;
      address->netNum[3] = 0;
   }
#ifndef NO_IPX_SUPPORT
   else if(sockAddr->sa_family == AF_IPX)
   {
      address->transport = IPXProtocol;
      address->port = htons(((SOCKADDR_IPX *) sockAddr)->sa_socket);
      address->netNum[0] = readU32FromBuffer((U8 *) ((SOCKADDR_IPX *) sockAddr)->sa_netnum);
      address->netNum[1] = readU32FromBuffer((U8 *) ((SOCKADDR_IPX *) sockAddr)->sa_nodenum);
      address->netNum[2] = readU16FromBuffer((U8 *) ((SOCKADDR_IPX *) sockAddr)->sa_nodenum+4);
      address->netNum[3] = 0;
   }
#endif
}

Socket::Socket(const Address &bindAddress, U32 sendBufferSize, U32 recvBufferSize, bool acceptsBroadcast, bool nonblockingIO)
{
   TNL_JOURNAL_READ_BLOCK(Socket::Socket,
         TNL_JOURNAL_READ( (&mPlatformSocket) );
      return;
   )
   init();
   mPlatformSocket = INVALID_SOCKET;
   mTransportProtocol = bindAddress.transport;

   const char *socketType = "UDP";

   if(bindAddress.transport == IPProtocol)
      mPlatformSocket = socket(AF_INET, SOCK_DGRAM, 0);
   else if(bindAddress.transport == TCPProtocol)
   {
      socketType = "TCP";
      mPlatformSocket = socket(AF_INET, SOCK_STREAM, 0);
   }
#if !defined(NO_IPX_SUPPORT)
   else if(bindAddress.transport == IPXProtocol)
   {
      socketType = "IPX";
      mPlatformSocket = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
   }
#endif
   else
   {
      logprintf(LogConsumer::LogError, "Attempted to create a socket bound to an invalid transport.");
   }
   if(mPlatformSocket != INVALID_SOCKET)
   {
      S32 error = 0;
      SOCKADDR address;
      socklen_t addressSize = sizeof(address);

      TNLToSocketAddress(bindAddress, &address, &addressSize);
      error = bind(mPlatformSocket, &address, addressSize);

      Address boundAddress;
      addressSize = sizeof(address);

      getsockname(mPlatformSocket, (PSOCKADDR) &address, &addressSize);
      SocketToTNLAddress(&address, &boundAddress);

      logprintf(LogConsumer::LogUDP, "%s socket created - bound to address: %s", socketType, boundAddress.toString());

      // set the send and receive buffer sizes
      error = setsockopt(mPlatformSocket, SOL_SOCKET, SO_RCVBUF, (char *) &recvBufferSize, sizeof(recvBufferSize));
      if(!error)
      {
         logprintf(LogConsumer::LogUDP, "%s socket receive buffer size set to %d.", socketType, recvBufferSize);
         error = setsockopt(mPlatformSocket, SOL_SOCKET, SO_SNDBUF, (char *) &sendBufferSize, sizeof(sendBufferSize));
      }
      else
         logprintf(LogConsumer::LogError, "%s socket error: unable to set the receive buffer size on socket.", socketType);

      if(!error)
      {
         logprintf(LogConsumer::LogUDP, "%s socket send buffer size set to %d.", socketType, recvBufferSize);

         if(mTransportProtocol != TCPProtocol)
         {
            // set the broadcast allowed flag
            S32 bc = acceptsBroadcast;
            error = setsockopt(mPlatformSocket, SOL_SOCKET, SO_BROADCAST, (char*)&bc, sizeof(bc));
         }
      }
      else
         logprintf(LogConsumer::LogError, "%s socket error: unable to set the send buffer size on socket.", socketType);


      // set the nonblocking IO flag
      if(!error)
      {
#if defined ( TNL_OS_WIN32 ) || defined ( TNL_OS_XBOX )
         DWORD notblock = nonblockingIO;
         S32 error = ioctlsocket(mPlatformSocket, FIONBIO, &notblock);
#else
         U32 notblock = nonblockingIO;
         S32 error;
         error = ioctl(mPlatformSocket, FIONBIO, &notblock);
#endif
      }
      else
      {
         logprintf(LogConsumer::LogError, "%s socket error: unable to set broadcast mode on socket.", socketType);
      }

      if(!error)
      {
         logprintf(LogConsumer::LogUDP, "%s socket non-blocking IO set.  Socket initialized.", socketType);
      }
      else
      {
         logprintf(LogConsumer::LogError, "Error prevented successful initialization of %s socket.", socketType);
         closesocket(mPlatformSocket);
         mPlatformSocket = INVALID_SOCKET;
      }
   }
   TNL_JOURNAL_WRITE_BLOCK(Socket::Socket,
      TNL_JOURNAL_WRITE( (mPlatformSocket) );
   )
}

Socket::~Socket()
{
   TNL_JOURNAL_READ_BLOCK(Socket::~Socket,
      return;
   )

   TNL_JOURNAL_WRITE_BLOCK(Socket::~Socket, ;)

   if(mPlatformSocket != INVALID_SOCKET)
      closesocket(mPlatformSocket);
   shutdown();
}

NetError Socket::sendto(const Address &address, const U8 *buffer, S32 bufferSize)
{
   TNL_JOURNAL_READ_BLOCK(Socket::sendto,
      return NoError;
   )

   TNL_JOURNAL_WRITE_BLOCK(Socket::sendto, ;
   )

   if(address.transport != mTransportProtocol)
      return InvalidPacketProtocol;

   SOCKADDR destAddress;
   socklen_t addressSize;

   TNLToSocketAddress(address, &destAddress, &addressSize);
   if(::sendto(mPlatformSocket, (const char*)buffer, bufferSize, 0,
         &destAddress, addressSize) == SOCKET_ERROR)
      return getLastError();
   else
      return NoError;
}

NetError Socket::recvfrom(Address *address, U8 *buffer, S32 bufferSize, S32 *outSize)
{
   TNL_JOURNAL_READ_BLOCK(Socket::recvfrom,
      bool wouldBlock;
      TNL_JOURNAL_READ( (&wouldBlock) );
      if(wouldBlock)
         return WouldBlock;



      TNL_JOURNAL_READ( (&address->transport) );
      TNL_JOURNAL_READ( (&address->port) );
      TNL_JOURNAL_READ( (&address->netNum[0]) );
      TNL_JOURNAL_READ( (&address->netNum[1]) );
      TNL_JOURNAL_READ( (&address->netNum[2]) );
      TNL_JOURNAL_READ( (&address->netNum[3]) );
      TNL_JOURNAL_READ( (outSize) );
      TNL_JOURNAL_READ( (*outSize, buffer) );
      return NoError;
   )

   SOCKADDR sa;
   socklen_t addrLen = sizeof(sa);
   S32 bytesRead = SOCKET_ERROR;

   bytesRead = ::recvfrom(mPlatformSocket, (char *) buffer, bufferSize, 0, &sa, &addrLen);
   if(bytesRead == SOCKET_ERROR)
   {
      TNL_JOURNAL_WRITE_BLOCK(Socket::recvfrom,
         TNL_JOURNAL_WRITE ( (true) );
      )
      return WouldBlock;
   }

   SocketToTNLAddress(&sa, address);

   *outSize = bytesRead;

   TNL_JOURNAL_WRITE_BLOCK(Socket::recvfrom,
      TNL_JOURNAL_WRITE( (false) );
      TNL_JOURNAL_WRITE( (address->transport) );
      TNL_JOURNAL_WRITE( (address->port) );
      TNL_JOURNAL_WRITE( (address->netNum[0]) );
      TNL_JOURNAL_WRITE( (address->netNum[1]) );
      TNL_JOURNAL_WRITE( (address->netNum[2]) );
      TNL_JOURNAL_WRITE( (address->netNum[3]) );
      TNL_JOURNAL_WRITE( (*outSize) );
      TNL_JOURNAL_WRITE( (*outSize, buffer) );
   )
   return NoError;
}

NetError Socket::connect(const Address &theAddress)
{
   SOCKADDR destAddress;
   socklen_t addressSize;

   TNLToSocketAddress(theAddress, &destAddress, &addressSize);
   if(::connect(mPlatformSocket, &destAddress, addressSize) == -1)
      return getLastError();
   return NoError;
}

NetError Socket::send(const U8 *buffer, S32 bufferSize)
{
   if(::send(mPlatformSocket, (const char *) buffer, bufferSize, 0) == SOCKET_ERROR)
      return getLastError();
   return NoError;
}

NetError Socket::recv(U8 *buffer, S32 bufferSize, S32 *bytesRead)
{
   *bytesRead = ::recv(mPlatformSocket, (char *) buffer, bufferSize, 0);
   if(*bytesRead == -1)
      return getLastError();
   return NoError;
}

Address Socket::getBoundAddress()
{
   SOCKADDR address;
   Address returnAddress;

   socklen_t addressSize = sizeof(address);
   getsockname(mPlatformSocket, (PSOCKADDR) &address, &addressSize);
   SocketToTNLAddress(&address, &returnAddress);

   return returnAddress;
}

bool Socket::isValid()
{
   return mPlatformSocket != INVALID_SOCKET;
}

#if defined ( TNL_OS_WIN32 )
void Socket::getInterfaceAddresses(Vector<Address> *addressVector)
{
   typedef struct {
      DWORD dwAddr;
      DWORD dwIndex;
      DWORD dwMask;
      DWORD dwBCastAddr;
      DWORD dwReasmSize;
      unsigned short unused1;
      unsigned short unused2;
   } MIB_IPADDRROW;
   typedef struct {
      DWORD dwNumEntries;
      MIB_IPADDRROW table[1];
   } MIB_IPADDRTABLE, *PMIB_IPADDRTABLE;
   typedef DWORD  (WINAPI *GetIPAddrTableFn_t)(PMIB_IPADDRTABLE,PULONG,BOOL);

   static GetIPAddrTableFn_t GetIpAddrTableFn = NULL;

   if(!GetIpAddrTableFn)
   {
      HMODULE module = LoadLibrary("iphlpapi.dll");
      if(!module)
         return;

      GetIpAddrTableFn = (GetIPAddrTableFn_t) GetProcAddress(module, "GetIpAddrTable");
   }

   // Under Win32, we use the IP helper library to query all
   // available network interfaces.
   PMIB_IPADDRTABLE pIPAddrTable;
   DWORD dwSize = 0;

   pIPAddrTable = (MIB_IPADDRTABLE*) malloc( sizeof( MIB_IPADDRTABLE) );

   // Make an initial call to GetIpAddrTable to get the
   // necessary size into the dwSize variable
   if (GetIpAddrTableFn(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER)
   {
      free( pIPAddrTable );
      pIPAddrTable = (MIB_IPADDRTABLE *) malloc ( dwSize );
   }

   // Make a second call to GetIpAddrTable to get the
   // actual data we want
   DWORD dwRetVal;
   if ( (dwRetVal = GetIpAddrTableFn( pIPAddrTable, &dwSize, 0 )) == NO_ERROR ) {
      for(U32 i = 0; i < pIPAddrTable->dwNumEntries; i++)
      {
         // construct an Address for this interface
         Address a;
         a.transport = IPProtocol;
         a.port = 0;
         a.netNum[0] = htonl(pIPAddrTable->table[i].dwAddr);
         a.netNum[1] = a.netNum[2] = a.netNum[3] = 0;
         // don't add the loopback address or the
         if(a.netNum[0] != INADDR_ANY && a.netNum[0] != 0x7F000001)
            addressVector->push_back(a);
      }
   }
   free(pIPAddrTable);
}

#elif defined (TNL_OS_MAC_OSX)
#include <ifaddrs.h>

void Socket::getInterfaceAddresses(Vector<Address> *addressVector)
{
   struct ifaddrs *addrs;
   getifaddrs(&addrs);

   for(struct ifaddrs *walk = addrs; walk; walk = walk->ifa_next)
   {
      Address theAddress;
      SocketToTNLAddress(walk->ifa_addr, &theAddress);
     if(theAddress.netNum[0] != INADDR_ANY && theAddress.netNum[0] != 0x7F000001)
     {
        //logprintf("found IF address %s", theAddress.toString());
       addressVector->push_back(theAddress);
      }
   }

   freeifaddrs(addrs);
}

#elif defined (TNL_OS_LINUX)
#include <stdio.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if.h>

void Socket::getInterfaceAddresses(Vector<Address> *addressVector)
{
   int sfd = socket(AF_INET, SOCK_STREAM, 0);
   if(sfd < 0)
     return;

   FILE *f = fopen("/proc/net/dev", "r");
   if(!f)
   {
      close(sfd);
      return;
   }
   char *ret;  // Only used to suppress warnings on cpp
   char buf[1024];
   ret = fgets(buf, 1024, f);
   ret = fgets(buf, 1024, f);

   struct ifreq ifr;
   struct sockaddr_in *sin = (struct sockaddr_in *) &ifr.ifr_addr;
   memset(&ifr, 0, sizeof(ifr));


   while(fgets(buf, 1024, f))
   {
      char *s = buf;
      while(*s == ' ')
         s++;
      char *end = strchr(s, ':');
      if(!end)
         continue;
      *end = 0;

      strcpy(ifr.ifr_name, s);
      sin->sin_family = AF_INET;
      if(ioctl(sfd, SIOCGIFADDR, &ifr) == 0)
      {
         Address theAddress;
         SocketToTNLAddress((struct sockaddr *) sin, &theAddress);
    if(theAddress.netNum[0] != INADDR_ANY && theAddress.netNum[0] != 0x7F000001)
    {
            addressVector->push_back(theAddress);
         }
      }
   }
   fclose(f);
   close(sfd);
}
#endif

bool Address::operator ==(const Address &theAddress) const
{
   return transport == theAddress.transport &&
      port == theAddress.port &&
      netNum[0] == theAddress.netNum[0] &&
      netNum[1] == theAddress.netNum[1] &&
      netNum[2] == theAddress.netNum[2] &&
      netNum[3] == theAddress.netNum[3];
}

// Constructor
Address::Address(TransportProtocol type, Address::NamedAddress name, U16 aPort)
{
   transport = type;
   port = aPort;
   if(transport == IPProtocol)
   {
      netNum[1] = netNum[2] = netNum[3] = 0;
      switch(name)
      {
         case None:
            netNum[0] = 0;
            break;
         case Localhost:
            netNum[0] = htonl(0x7F000001);         // 127.0.0.1; i.e. loopback address
            break;
         case Broadcast:
            netNum[0] = htonl(INADDR_BROADCAST);  // http://www-2.cs.cmu.edu/~srini/15-441/F01.full/www/assignments/P2/htmlsim_split/node19.html
            break;
         case Any:
            netNum[0] = htonl(INADDR_ANY);        // INADDR_ANY : anyone from any network can connect, using any IP address bound to the PC.
            break;
      }
   }
   else if(transport == IPXProtocol)
   {
      for(U32 i = 0; i < 4; i++)
         netNum[i] = 0xFFFFFFFF;
   }

   mIsValid = true;
}

bool Address::set(const IPAddress &address)
{
   transport = IPProtocol;
   port = address.port;
   netNum[0] = address.netNum;
   netNum[1] =netNum[2] = netNum[3] = 0;
   mIsValid = (netNum[0] != 0);
   return true;

}

IPAddress Address::toIPAddress() const
{
   IPAddress ret;
   ret.port = port;
   ret.netNum = netNum[0];
   return ret;
}

bool Address::set(std::string addressString)
{
   return set(addressString.c_str());
}

bool Address::set(const char *addressString)
{
   if(addressString[0] == 0) // zero string length should be invalid
   {
      mIsValid = false;
      return false;
   }

   init();
   if(strnicmp(addressString, "ipx:", 4))
   {
      bool isTCP = false;     // assume IP if it doesn't have ipx: at the front.

      // Check for IP: or TCP: prefix, and strip it off
      if(!strnicmp(addressString, "ip:", 3))
         addressString += 3;  // eat off the ip:
      else if(!strnicmp(addressString, "tcp:", 4))
      {
         addressString += 4;
         isTCP = true;
      }

      SOCKADDR_IN ipAddr;
      char remoteAddr[256];
      if(strlen(addressString) > 255)
      {
         mIsValid = false;
         return false;
      }

      strcpy(remoteAddr, addressString);

      // Strip off port number, save it in portString
      char *portString = strchr(remoteAddr, ':');
      if(portString)
         *portString++ = 0;

      // Check for some special cases
      if(!stricmp(remoteAddr, "broadcast"))
         ipAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
      else if(!stricmp(remoteAddr, "localhost"))
         ipAddr.sin_addr.s_addr = htonl(0x7F000001);
      else if(!stricmp(remoteAddr, "any"))
         ipAddr.sin_addr.s_addr = htonl(INADDR_ANY);
      else     // Everything else
      {
         ipAddr.sin_addr.s_addr = inet_addr(remoteAddr);
         if(ipAddr.sin_addr.s_addr == INADDR_NONE)
         {
#if defined (TNL_OS_XBOX)
            mIsValid = false;
            return false;
#else
            struct hostent *hp;
            if((hp = gethostbyname(remoteAddr)) == NULL)
            {
               mIsValid = false;
               return false;
            }
            else
               memcpy(&ipAddr.sin_addr.s_addr, hp->h_addr, sizeof(IN_ADDR));
#endif
         }
      }
      if(portString)
         ipAddr.sin_port = htons(atoi(portString));
      else
         ipAddr.sin_port = htons(0);
      ipAddr.sin_family = AF_INET;
      SocketToTNLAddress((SOCKADDR *) &ipAddr, this);
      if(isTCP)
         transport = TCPProtocol;
      if((netNum[0] | netNum[1] | netNum[2] | netNum[3]) == 0)  // IP address of 0.0.0.0 is probably not valid
      {
         mIsValid = false;
         return false;
      }
      mIsValid = true;
      return true;
   }
   else     // addressString started with "ipx:"
   {
      S32 i;
      S32 aPort;

      transport = IPXProtocol;
      for(i = 0; i < 4; i++)
         netNum[i] = 0xFFFFFFFF;

      // it's an IPX string
      addressString += 4;
      if(!stricmp(addressString, "broadcast"))
      {
         port = 0;
         mIsValid = true;
         return true;
      }
      else if(sscanf(addressString, "broadcast:%d", &aPort) == 1)
      {
         port = aPort;
         mIsValid = true;
         return true;
      }
      else
      {
         U32 aNodeNum[6];
         U32 aNetNum[4];
         S32 count = sscanf(addressString, "%2x%2x%2x%2x:%2x%2x%2x%2x%2x%2x:%d",
            &aNetNum[0], &aNetNum[1], &aNetNum[2], &aNetNum[3],
            &aNodeNum[0], &aNodeNum[1], &aNodeNum[2], &aNodeNum[3], &aNodeNum[4], &aNodeNum[5],
            &aPort);

         if(count == 10)
         {
            aPort = 0;
            count++;
         }
         if(count != 11)
         {
            mIsValid = false;
            return false;
         }

         netNum[0] = (aNetNum[0] << 24) |
                     (aNetNum[1] << 16) |
                     (aNetNum[2] << 8 ) |
                     aNetNum[3];
         netNum[1] = (aNodeNum[0] << 24) |
                     (aNodeNum[1] << 16) |
                     (aNodeNum[2] << 8 ) |
                     aNodeNum[3];
         netNum[2] = (aNodeNum[4] << 8) |
                      aNodeNum[5];
         netNum[3] = 0;
         port = aPort;
         mIsValid = true;
         return true;
      }
   }
}

const char *Address::toString() const
{
   static char addressBuffer[256];
   if(transport == IPProtocol)
   {
      SOCKADDR_IN ipAddr;
      socklen_t addrLen = sizeof(ipAddr);
      TNLToSocketAddress(*this, (SOCKADDR *) &ipAddr, &addrLen);

      if(ipAddr.sin_addr.s_addr == htonl(INADDR_BROADCAST))
         dSprintf(addressBuffer, 256, "IP:Broadcast:%d", ntohs(ipAddr.sin_port));
      else if(ipAddr.sin_addr.s_addr == htonl(INADDR_ANY))
         dSprintf(addressBuffer, 256, "IP:Any:%d", ntohs(ipAddr.sin_port));
      else
         dSprintf(addressBuffer, 256, "IP:%d.%d.%d.%d:%d", U8( netNum[0] >> 24 ),
            U8 (netNum[0] >> 16 ), U8 (netNum[0] >> 8), U8(netNum[0]), port);
   }
   else
   {
      dSprintf(addressBuffer, 256, "IPX:%.2X%.2X%.2X%.2X:%.2X%.2X%.2X%.2X%.2X%.2X:%d",
         U8(netNum[0] >> 24), U8(netNum[0] >> 16), U8(netNum[0] >> 8), U8(netNum[0]),
         U8(netNum[1] >> 24), U8(netNum[1] >> 16), U8(netNum[1] >> 8), U8(netNum[1]),
         U8(netNum[2] >> 8), U8(netNum[2]), port);
   }
   return addressBuffer;
}

NetError getLastError()
{
#if defined ( TNL_OS_WIN32 ) || defined ( TNL_OS_XBOX )
   S32 err = WSAGetLastError();
   switch(err)
   {
      case WSAEWOULDBLOCK:
         return WouldBlock;
      default:
         return UnknownError;
   }
#else
   if(errno == EAGAIN)
      return WouldBlock;
   return UnknownError;
#endif
}

};
