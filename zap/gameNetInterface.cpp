//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
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

#include "game.h"
#include "version.h"

namespace Zap
{

// Constructor
GameNetInterface::GameNetInterface(const Address &bindAddress, Game *theGame) : NetInterface(bindAddress)
{
   mGame = theGame;
};

// Destructor
GameNetInterface::~GameNetInterface()
{
   // Do nothing
}


void GameNetInterface::processPacket(const Address &sourceAddress, BitStream *pStream)
{
   if(! mGame->isServer())   // ignore request if we are a client, we won't let it start any connection, if some outside network thinks we are a server.
   {
      U8 packetType = pStream->getBuffer()[0];
      if(packetType == ConnectChallengeRequest || packetType == ConnectRequest)
         return;
   }
   Parent::processPacket(sourceAddress, pStream);
}


// using this and not computeClientIdentityToken fix problem with random ping timed out in game lobby.
// Only server use this function, client only holds Token received in PingResponse.
// This function can be changed at any time without breaking compatibility.
U32 computeSimpleToken(const Address &theAddress, const Nonce &theNonce)
{
   return ( (theNonce.data[1] ^ theNonce.data[3])
      | ((theNonce.data[0] ^ theNonce.data[5]) << 8)
      | ((theNonce.data[2] ^ theNonce.data[7]) << 16)
      | ((theNonce.data[4] ^ theNonce.data[6]) << 24) )
      ^ 0x638542e6;
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

            U32 token = computeSimpleToken(remoteAddress, clientNonce);
            PacketStream pingResponse;
            pingResponse.write(U8(PingResponse));
            clientNonce.write(&pingResponse);
            pingResponse.write(token);
            pingResponse.sendto(mSocket, remoteAddress);
         }
         break;

      case PingResponse: 
         if(!mGame->isServer())  // Client only
         {
            Nonce theNonce;
            U32 clientIdentityToken;
            theNonce.read(stream);
            stream->read(&clientIdentityToken);
            
            mGame->gotPingResponse(remoteAddress, theNonce, clientIdentityToken);
         }
         break;

      case Query:
         if(mGame->isServer())
         {
            Nonce theNonce;
            U32 clientIdentityToken;
            theNonce.read(stream);
            stream->read(&clientIdentityToken);

            if(clientIdentityToken == computeSimpleToken(remoteAddress, theNonce))
            {
               PacketStream queryResponse;
               queryResponse.write(U8(QueryResponse));

               theNonce.write(&queryResponse);
               queryResponse.writeStringTableEntry(mGame->getSettings()->getHostName());
               queryResponse.writeStringTableEntry(mGame->getSettings()->getHostDescr());

               queryResponse.write(mGame->getPlayerCount());
               queryResponse.write(mGame->getMaxPlayers());
               queryResponse.write(mGame->getRobotCount());
               queryResponse.writeFlag(mGame->isDedicated());
               queryResponse.writeFlag(mGame->isTestServer());
               queryResponse.writeFlag(mGame->getSettings()->getServerPassword() != "");

               queryResponse.sendto(mSocket, remoteAddress);
            }
         }
         break;

      case QueryResponse: 
         if(!mGame->isServer())  // Client only
         {
            Nonce theNonce;
            StringTableEntry name;
            StringTableEntry descr;
            U32 playerCount, maxPlayers, botCount;
            bool dedicated, test, passwordRequired;

            theNonce.read(stream);
            stream->readStringTableEntry(&name);
            stream->readStringTableEntry(&descr);

            stream->read(&playerCount);
            stream->read(&maxPlayers);
            stream->read(&botCount);
            dedicated = stream->readFlag();
            test = stream->readFlag();
            passwordRequired = stream->readFlag();

            // Alert the user
            mGame->gotQueryResponse(remoteAddress, theNonce, name.getString(), descr.getString(), playerCount, maxPlayers, botCount, dedicated, test, passwordRequired);
         }
         break;
   }
}


// Send ping to the server.  If server has different PROTOCOL_VERSION, the packet will be ignored.  This prevents players
// from seeing servers they are incompatible with.
void GameNetInterface::sendPing(const Address &theAddress, const Nonce &clientNonce)
{
   PacketStream packet;
   packet.write(U8(Ping));
   clientNonce.write(&packet);
   packet.write(CS_PROTOCOL_VERSION);
   packet.sendto(mSocket, theAddress);
}

void GameNetInterface::sendQuery(const Address &theAddress, const Nonce &clientNonce, U32 identityToken)
{
   PacketStream packet;
   packet.write(U8(Query));
   clientNonce.write(&packet);
   packet.write(identityToken);
   packet.sendto(mSocket, theAddress);
}

};


