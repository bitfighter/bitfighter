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
// Only servers use this function, client only holds Token received in PingResponse.
// This function can be changed at any time without breaking compatibility.
U32 computeSimpleToken(const Nonce &nonce)
{
   return ((nonce.data[1] ^ nonce.data[3])        |
          ((nonce.data[0] ^ nonce.data[5]) << 8)  |
          ((nonce.data[2] ^ nonce.data[7]) << 16) |
          ((nonce.data[4] ^ nonce.data[6]) << 24)) ^ 0x638542e6;
}


static void handlePing(Game *game, const Address &remoteAddress, Socket &socket, BitStream *stream, S32 clientId)
{
   TNLAssert(game->isServer(), "Expected this to be a server!");

   Nonce clientNonce;
   clientNonce.read(stream);

   U32 protocolVersion;
   stream->read(&protocolVersion);

   if(protocolVersion != CS_PROTOCOL_VERSION)   // Ignore pings from incompatible versions
      return;

   U32 clientIdentityToken = computeSimpleToken(clientNonce);
   PacketStream pingResponse;

   pingResponse.write(U8(GameNetInterface::PingResponse));
   clientNonce.write(&pingResponse);
   pingResponse.write(clientIdentityToken);

   //pingResponse.write(clientId);    // Disable before 019a release
#ifndef TNL_DEBUG
#  error Disable the above, please!
#endif

   pingResponse.sendto(socket, remoteAddress);
}


// Process response to Ping, written in handlePing(), above
static void handlePingResponse(Game *game, const Address &remoteAddress, BitStream *stream)
{
   TNLAssert(!game->isServer(), "Expected this to be a client!");

   Nonce nonce;
   U32 clientIdentityToken;
   S32 serverId;

   nonce.read(stream);
   stream->read(&clientIdentityToken);

   //stream->read(&serverId);   // Disable before 019a release
    serverId = 0;
#ifndef TNL_DEBUG
#  error Disable the above, please!
#endif
            
   game->gotPingResponse(remoteAddress, nonce, clientIdentityToken, serverId);
}


static void handleQuery(Game *game, const Address &remoteAddress, Socket &socket, BitStream *stream)
{
   TNLAssert(game->isServer(), "Expected this to be a server!");

   Nonce nonce;
   U32 clientIdentityToken;

   nonce.read(stream);
   stream->read(&clientIdentityToken);

   if(clientIdentityToken == computeSimpleToken(nonce))
   {
      PacketStream queryResponse;
      queryResponse.write(U8(GameNetInterface::QueryResponse));

      nonce.write(&queryResponse);
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
   TNLAssert(!game->isServer(), "Expected this to be a client!");

   Nonce nonce;
   StringTableEntry name;
   StringTableEntry descr;
   U32 playerCount, maxPlayers, botCount;
   bool dedicated, test, passwordRequired;

   nonce.read(stream);
   stream->readStringTableEntry(&name);
   stream->readStringTableEntry(&descr);

   stream->read(&playerCount);
   stream->read(&maxPlayers);
   stream->read(&botCount);
   dedicated = stream->readFlag();
   test = stream->readFlag();
   passwordRequired = stream->readFlag();

   // Alert the user
   game->gotQueryResponse(remoteAddress, nonce, name.getString(), descr.getString(), 
                          playerCount, maxPlayers, botCount, dedicated, test, passwordRequired);
}


void GameNetInterface::handleInfoPacket(const Address &remoteAddress, U8 packetType, BitStream *stream)
{
   switch(packetType)
   {
      case Ping:
         if(mGame->isServer())
            handlePing(mGame, remoteAddress, mSocket, stream, mGame->getClientId());
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

      default:
         TNLAssert(false, "Unknown packetType!");
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


