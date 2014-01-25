//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "gameNetInterface.h"

#include "game.h"
#include "version.h"

namespace Zap
{

// Constructor
GameNetInterface::GameNetInterface(const Address &bindAddress, Game *theGame) : NetInterface(bindAddress)
{
   mGame = theGame;
}


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


// Using this and not computeClientIdentityToken fix problem with random ping timed out in game lobby.
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


static void handlePing(Game *game, const Address &remoteAddress, Socket &socket, BitStream *stream)
{
   Nonce clientNonce;
   clientNonce.read(stream);

   U32 protocolVersion;
   stream->read(&protocolVersion);

   if(protocolVersion != CS_PROTOCOL_VERSION)   // Ignore pings from incompatible versions
      return;

   U32 token = computeSimpleToken(remoteAddress, clientNonce);
   PacketStream pingResponse;

   pingResponse.write(U8(GameNetInterface::PingResponse));
   clientNonce.write(&pingResponse);
   pingResponse.write(token);
   pingResponse.sendto(socket, remoteAddress);
}


static void handlePingResponse(Game *game, const Address &remoteAddress, BitStream *stream)
{
   Nonce nonce;
   U32 clientIdentityToken;

   nonce.read(stream);
   stream->read(&clientIdentityToken);
            
   game->gotPingResponse(remoteAddress, nonce, clientIdentityToken);
}


static void handleQuery(Game *game, const Address &remoteAddress, Socket &socket, BitStream *stream)
{
   Nonce theNonce;
   U32 clientIdentityToken;

   theNonce.read(stream);
   stream->read(&clientIdentityToken);

   if(clientIdentityToken == computeSimpleToken(remoteAddress, theNonce))
   {
      PacketStream queryResponse;
      queryResponse.write(U8(GameNetInterface::QueryResponse));

      theNonce.write(&queryResponse);
      queryResponse.writeStringTableEntry(game->getSettings()->getHostName());
      queryResponse.writeStringTableEntry(game->getSettings()->getHostDescr());

      queryResponse.write(game->getPlayerCount());
      queryResponse.write(game->getMaxPlayers());
      queryResponse.write(game->getRobotCount());
      queryResponse.writeFlag(game->isDedicated());
      queryResponse.writeFlag(game->isTestServer());
      queryResponse.writeFlag(game->getSettings()->getServerPassword() != "");

      queryResponse.sendto(socket, remoteAddress);
   }
}


static void handleQueryResponse(Game *game, const Address &remoteAddress, BitStream *stream)
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
   game->gotQueryResponse(remoteAddress, theNonce, name.getString(), descr.getString(), 
                          playerCount, maxPlayers, botCount, dedicated, test, passwordRequired);
}


void GameNetInterface::handleInfoPacket(const Address &remoteAddress, U8 packetType, BitStream *stream)
{
   switch(packetType)
   {
      case Ping:
         if(mGame->isServer())
            handlePing(mGame, remoteAddress, mSocket, stream);
         break;

      case PingResponse: 
         if(!mGame->isServer())  
            handlePingResponse(mGame, remoteAddress, stream);
         break;

      case Query:
         if(mGame->isServer())
            handleQuery(mGame, remoteAddress, mSocket, stream);
         break;

      case QueryResponse: 
         if(!mGame->isServer())  
            handleQueryResponse(mGame, remoteAddress, stream);
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


