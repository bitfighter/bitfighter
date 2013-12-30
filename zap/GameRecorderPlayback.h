#ifndef _GAMERECORDERPLAYBACK_H_
#define _GAMERECORDERPLAYBACK_H_

#include <stdio.h>
#include "tnlGhostConnection.h"
#include "tnlNetObject.h"
#include "gameConnection.h"

#include "UIMenus.h"

namespace Zap {


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


class GameUserInterface;

class PlaybackGameUserInterface : public UserInterface
{
   GameUserInterface *mGameInterface;
   SafePtr<GameRecorderPlayback> mPlaybackConnection;
public:
   explicit PlaybackGameUserInterface(ClientGame *game);
   void onActivate();

   bool onKeyDown(InputCode inputCode);
   void onKeyUp(InputCode inputCode);
   void onTextInput(char ascii);

   void idle(U32 timeDelta);
   void render();
};

}
#endif
