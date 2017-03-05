//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAMERECORDERPLAYBACK_H_
#define _GAMERECORDERPLAYBACK_H_

#include "gameConnection.h"

#include "UIMenus.h"
#include "UIItemListSelectMenu.h"

#include <stdio.h>

namespace Zap 
{


class ClientGame;
class ClientInfo;
class Timer;

class GameRecorderPlayback : public GameConnection
{
   typedef GameConnection Parent;

private:
   FILE *mFile;

   ClientGame *mGame;
   S32 mMilliSeconds;
   U32 mSizeToRead;
   SafePtr<ClientInfo> mClientInfoSpectating;

public:
   GameRecorderPlayback(ClientGame *game, const char *filename);
   ~GameRecorderPlayback();

   StringTableEntry mClientInfoSpectatingName;
   bool mIsButtonHeldDown;

   U32 mTotalTime;
   U32 mCurrentTime;

   bool isValid();

   bool lostContact();
   void addPendingMove(Move *theMove);
   void changeSpectate(S32 n);

   void updateSpectate();
   void processMoreData(TNL::U32 MilliSeconds);
   void restart();
};


class PlaybackSelectUserInterface : public LevelMenuSelectUserInterface
{
public:
   explicit PlaybackSelectUserInterface(ClientGame *game, UIManager *uiManager);

   void onActivate();
   void processSelection(U32 index);
};


class PlaybackServerDownloadUserInterface : public LevelMenuSelectUserInterface
{
public:
   explicit PlaybackServerDownloadUserInterface(ClientGame *game, UIManager *uiManager);
   void onActivate();
   void processSelection(U32 index);
   void receivedLevelList(const Vector<string> &levels);
};


class GameUserInterface;

class PlaybackGameUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   GameUserInterface *mGameInterface;
   SafePtr<GameRecorderPlayback> mPlaybackConnection;
   U32 mSpeed;
   U32 mSpeedRemainder;
   bool mVisible;
   Timer mDisableMouseTimer;

public:
   explicit PlaybackGameUserInterface(ClientGame *game, UIManager *uiManager);
   void onActivate();
   void onReactivate();

   bool onKeyDown(InputCode inputCode);
   void onKeyUp(InputCode inputCode);
   void onTextInput(char ascii);
   void onMouseMoved();

   void idle(U32 timeDelta);
   void render() const;
};

}
#endif
