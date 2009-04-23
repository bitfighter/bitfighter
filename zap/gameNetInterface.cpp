//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "gameNetInterface.h"
#include "UIQueryServers.h"
#include "game.h"

namespace Zap
{

extern const char *gServerPassword;
extern bool gDedicatedServer;

// Constructor
GameNetInterface::GameNetInterface(const Address &bindAddress, Game *theGame) : NetInterface(bindAddress)
{
   mGame = theGame;
};

// Add an address to the ban list
void GameNetInterface::banHost(const Address &bannedAddress, U32 bannedMilliseconds)
{
   BannedHost h;
   h.theAddress = bannedAddress;
   h.banDuration = bannedMilliseconds;
   mBanList.push_back(h);
}

// Check if address is on the ban list
bool GameNetInterface::isHostBanned(const Address &theAddress)
{
   for(S32 i = 0; i < mBanList.size(); i++)
      if(theAddress.isEqualAddress(mBanList[i].theAddress))
         return true;

   return false;
}

void GameNetInterface::processPacket(const Address &sourceAddress, BitStream *pStream)
{
   for(S32 i = 0; i < mBanList.size(); i++)
      if(sourceAddress.isEqualAddress(mBanList[i].theAddress))
         return;        // Ignore packets from banned hosts

   Parent::processPacket(sourceAddress, pStream);
}

void GameNetInterface::checkBanlistTimeouts(U32 timeElapsed)
{
   for(S32 i = 0; i < mBanList.size(); )
   {
      if(mBanList[i].banDuration < timeElapsed)
         mBanList.erase_fast(i);
      else
      {
         mBanList[i].banDuration -= timeElapsed;
         i++;
      }
   }
}

void GameNetInterface::handleInfoPacket(const Address &remoteAddress, U8 packetType, BitStream *stream)
{
   switch(packetType)
   {
      case Ping:
         if(mGame->isServer())
         {
            Nonce clientNonce;
            clientNonce.read(stream);

            U32 protocolVersion;
            stream->read(&protocolVersion);

            if(protocolVersion != CS_PROTOCOL_VERSION)   // Ignore pings from incompatible versions
               break;

            U32 token = computeClientIdentityToken(remoteAddress, clientNonce);
            PacketStream pingResponse;
            pingResponse.write(U8(PingResponse));
            clientNonce.write(&pingResponse);
            pingResponse.write(token);
            pingResponse.sendto(mSocket, remoteAddress);
         }
         break;
      case PingResponse:
         {
            Nonce theNonce;
            U32 clientIdentityToken;
            theNonce.read(stream);
            stream->read(&clientIdentityToken);
            gQueryServersUserInterface.gotPingResponse(remoteAddress, theNonce, clientIdentityToken);
         }
         break;
      case Query:
         {
            Nonce theNonce;
            U32 clientIdentityToken;
            theNonce.read(stream);
            stream->read(&clientIdentityToken);
            if(clientIdentityToken == computeClientIdentityToken(remoteAddress, theNonce))
            {
               PacketStream queryResponse;
               queryResponse.write(U8(QueryResponse));

               theNonce.write(&queryResponse);
               queryResponse.writeString(gServerGame->getHostName(), QueryServersUserInterface::MaxServerNameLen);
               queryResponse.writeString(gServerGame->getHostDescr(), QueryServersUserInterface::MaxServerDescrLen);

               queryResponse.write(gServerGame->getPlayerCount());
               queryResponse.write(gServerGame->getMaxPlayers());
               queryResponse.write(gServerGame->getRobotCount());
               queryResponse.writeFlag(gDedicatedServer);
               queryResponse.writeFlag(gServerGame->isTestServer());
               queryResponse.writeFlag(gServerPassword != NULL);

               queryResponse.sendto(mSocket, remoteAddress);
            }
         }
         break;
      case QueryResponse:
         {
            Nonce theNonce;
            char nameString[256];
            char descrString[256];
            U32 playerCount, maxPlayers, botCount;
            bool dedicated, test, passwordRequired;

            theNonce.read(stream);
            stream->readString(nameString);
            stream->readString(descrString);

            stream->read(&playerCount);
            stream->read(&maxPlayers);
            stream->read(&botCount);
            dedicated = stream->readFlag();
            test = stream->readFlag();
            passwordRequired = stream->readFlag();

            gQueryServersUserInterface.gotQueryResponse(remoteAddress, theNonce, nameString, descrString, playerCount, maxPlayers, botCount, dedicated, test, passwordRequired);
         }
         break;
   }
}


// Send ping to the server.  If server has different PROTOCOL_VERSION, the packet will be ignored.
void GameNetInterface::sendPing(const Address &theAddress, const Nonce &clientNonce)
{
   TNL::logprintf("pinging server %s...", theAddress.toString());

   PacketStream packet;
   packet.write(U8(Ping));
   clientNonce.write(&packet);
   packet.write(CS_PROTOCOL_VERSION);
   packet.sendto(mSocket, theAddress);
}

void GameNetInterface::sendQuery(const Address &theAddress, const Nonce &clientNonce, U32 identityToken)
{
   TNL::logprintf("querying server %s...", theAddress.toString());

   PacketStream packet;
   packet.write(U8(Query));
   clientNonce.write(&packet);
   packet.write(identityToken);
   packet.sendto(mSocket, theAddress);
}

};

