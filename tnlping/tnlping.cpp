//-----------------------------------------------------------------------------------
//
//   Torque Network Library - TNL Network program reachability tester
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
      printf("Usage: tnlping <remoteAddress> [sourceAddress]");
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
               printf("TNL Service is UP (pingtime = %d)", Platform::getRealMilliseconds() - time);
               return 0;
            }
         }
         Platform::sleep(1);
         if(Platform::getRealMilliseconds() - time > 1000)
            break;
      }
   }
   printf("TNL Service is DOWN.");
   return 1;
}