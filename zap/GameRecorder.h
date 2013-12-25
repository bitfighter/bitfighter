#ifndef _GAMERECORDER_H_
#define _GAMERECORDER_H_

#include <stdio.h>
#include "tnlGhostConnection.h"
#include "tnlNetObject.h"
#include "gameConnection.h"

namespace Zap {

class ServerGame;

class GameRecorderServer : public GameConnection
{
   typedef GhostConnection Parent;
   FILE *mFile;
   ServerGame *mGame;
   TNL::NetObject mNetObj;
   U32 mMilliSeconds;
public:
   GameRecorderServer(ServerGame *game);
   ~GameRecorderServer();

   void idle(TNL::U32 MilliSeconds);
};

#ifndef ZAP_DEDICATED
class ClientGame;
class GameRecorderPlayback : public GameConnection
{
   typedef GameConnection Parent;
   FILE *mFile;
   S32 mMilliSeconds;
   U32 mSizeToRead;
public:
   GameRecorderPlayback(ClientGame *game);
   ~GameRecorderPlayback();

   bool lostContact();
   void addPendingMove(Move *theMove);

   void updateTimers(TNL::U32 MilliSeconds);
};
#endif

}
#endif
