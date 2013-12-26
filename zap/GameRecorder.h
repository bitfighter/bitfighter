#ifndef _GAMERECORDER_H_
#define _GAMERECORDER_H_

#include <stdio.h>
#include "tnlGhostConnection.h"
#include "tnlNetObject.h"
#include "gameConnection.h"

#ifndef ZAP_DEDICATED
#include "UIMenus.h"
#endif

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
class ClientInfo;
class GameRecorderPlayback : public GameConnection
{
   typedef GameConnection Parent;
   FILE *mFile;
   ClientGame *mGame;
   S32 mMilliSeconds;
   U32 mSizeToRead;
   SafePtr<ClientInfo> mClientInfoSpectating;
   bool isButtonHeldDown;
public:
   GameRecorderPlayback(ClientGame *game, const char *filename);
   ~GameRecorderPlayback();

   bool lostContact();
   void addPendingMove(Move *theMove);

   void updateTimers(TNL::U32 MilliSeconds);
};

class PlaybackSelectUserInterface : public LevelMenuSelectUserInterface
{
public:
   explicit PlaybackSelectUserInterface(ClientGame *game);
   void onActivate();
   void processSelection(U32 index);
};

#endif

}
#endif
