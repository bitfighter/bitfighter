//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnl.h"
#include "tnlNetInterface.h"
#include "tnlLog.h"
#include "tnlNonce.h"
#include "tnlRandom.h"
#include <string>

#include <stdio.h>

using namespace TNL;

int main(int argc, const char **argv)
{
   if(argc < 2)
   {
      printf("Usage: tnlping <remoteAddress> [displayName] [sourceAddress]\n\n"
      "Example 1: Simple usage expecting port 28000\n   tnlping bitfighter.org\n\n"
      "Example 2: Advanced usage with specific port\n   tnlping 192.168.1.2:28001\n\n"
      "Display name only used in return message.\n"
      "Default port = 28000; Return codes are suitable for monitoring with Nagios.\n");
      return 3;   // Nagios status = Unknown
   }
   
   U8 randData[sizeof(U32) + sizeof(S64)];
   *((U32 *) randData) = Platform::getRealMilliseconds();
   *((S64 *) (randData + sizeof(U32))) = Platform::getHighPrecisionTimerValue();
   TNL::Random::addEntropy(randData, sizeof(randData));

   Address remoteAddress(argv[1]);
   string serviceName(argc > 2 ? argv[2] : "TNL Service");
   Address sourceAddress(argc > 3 ? argv[3] : "IP:Any:0");

   Nonce clientNonce;
   clientNonce.getRandom();

   Socket sourceSocket(sourceAddress);

   PacketStream out;
   out.write(U8(NetInterface::ConnectChallengeRequest));
   clientNonce.write(&out);
   out.writeFlag(false);
   out.writeFlag(false);

   for(U32 tryCount = 0; tryCount < 5; tryCount++)
   {
      U32 time = Platform::getRealMilliseconds();
      out.sendto(sourceSocket, remoteAddress);
      for(;;)
      {
         PacketStream incoming;
         Address incomingAddress;
         if(incoming.recvfrom(sourceSocket, &incomingAddress) == NoError)
         {
            U8 packetType;
            Nonce theNonce;
            incoming.read(&packetType);
            theNonce.read(&incoming);
            if(packetType == NetInterface::ConnectChallengeResponse && theNonce == clientNonce)
            {
               printf("%s is UP (pingtime = %d)\n", serviceName.c_str(), Platform::getRealMilliseconds() - time);
               return 0;   // Nagios status OK
            }
         }
         Platform::sleep(1);
         if(Platform::getRealMilliseconds() - time > 1000)
            break;
      }
   }
   
   printf("%s is DOWN\n", serviceName.c_str());
   
   return 2;   // Nagios status Critical
}