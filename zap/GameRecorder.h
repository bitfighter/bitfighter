//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAMERECORDER_H_
#define _GAMERECORDER_H_

#include <stdio.h>
#include "tnlGhostConnection.h"
#include "tnlNetObject.h"
#include "gameConnection.h"


namespace Zap {

class ServerGame;
class WriteBufferThread;

class GameRecorderServer : public GameConnection
{
   typedef GhostConnection Parent;

private:
   WriteBufferThread *mWriter;
   ServerGame *mGame;
   TNL::NetObject mNetObj;
   U32 mMilliSeconds;

public:
   string mFileName;

   static string buildGameRecorderExtension();

   GameRecorderServer(ServerGame *game);
   ~GameRecorderServer();

   void idle(TNL::U32 MilliSeconds);
};

}
#endif
