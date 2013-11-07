//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAMENETINTERFACE_H_
#define _GAMENETINTERFACE_H_

#include "tnlNetInterface.h"

using namespace TNL;

namespace Zap
{

class Game;

class GameNetInterface : public NetInterface
{
   typedef NetInterface Parent;
   Game *mGame;

public:
   enum PacketType
   {
      Ping = FirstValidInfoPacketId,
      PingResponse,
      Query,
      QueryResponse,
   };

   GameNetInterface(const Address &bindAddress, Game *theGame);
   virtual ~GameNetInterface();

   void handleInfoPacket(const Address &remoteAddress, U8 packetType, BitStream *stream);
   void sendPing(const Address &theAddress, const Nonce &clientNonce);
   void sendQuery(const Address &theAddress, const Nonce &clientNonce, U32 identityToken);
   void processPacket(const Address &sourceAddress, BitStream *pStream);

   Game *getGame() { return mGame; }
};

};

#endif

