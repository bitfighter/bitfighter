//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnl.h"
#include "tnlNetInterface.h"
#include "tnlLog.h"
#include "tnlNonce.h"
#include "tnlRandom.h"

#include <stdio.h>

using namespace TNL;

int main(int argc, const char **argv)
{
   if(argc < 2)
   {
      printf("Usage: tnlping <remoteAddress> [sourceAddress]\n\n"
      "Example 1: Simple usage expecting port 28000\n   tnlping 192.168.1.2\n\n"
      "Example 2: Advanced usage with specific port\n   tnlping 192.168.1.2:28001\n\n");
      return 1;
   }
   
   U8 randData[sizeof(U32) + sizeof(S64)];
   *((U32 *) randData) = Platform::getRealMilliseconds();
   *((S64 *) (randData + sizeof(U32))) = Platform::getHighPrecisionTimerValue();
   TNL::Random::addEntropy(randData, sizeof(randData));

   Address remoteAddress(argv[1]);
   Address sourceAddress(argc > 2 ? argv[2] : "IP:Any:0");

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
               printf("TNL Service is UP (pingtime = %d)\n", Platform::getRealMilliseconds() - time);
               return 0;
            }
         }
         Platform::sleep(1);
         if(Platform::getRealMilliseconds() - time > 1000)
            break;
      }
   }
   
   printf("TNL Service is DOWN\n");
   
   return 1;
}