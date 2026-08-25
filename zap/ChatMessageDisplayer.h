//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CHAT_MESSAGE_DISPLAYER_H_
#define _CHAT_MESSAGE_DISPLAYER_H_

#include "ClientGame.h"
#include "Color.h"
#include "Timer.h"
#include "UI.h"
#include "tnlVector.h"

#include <string>

namespace Zap
{

struct ColorString
{
   Color color;
   string str;
   U32 groupId;

   void set(const string &s, const Color &c, U32 groupId = 0);
};


class ChatMessageDisplayer
{
private:
   U32 mFirst, mLast;
   bool mExpire;
   bool mTopDown;
   S32 mWrapWidth;
   S32 mFontSize;
   S32 mFontGap;
   bool mFull;

   U32 mNextGroupId;

   void advanceFirst();
   void advanceLast();

   Timer mChatScrollTimer;
   Timer mDisplayChatMessageTimer;

   ClientGame *mGame;

   Vector<ColorString> mMessages;

public:
   ChatMessageDisplayer(ClientGame *game, S32 msgCount, bool msgsExpire, bool topDown, S32 wrapWidth, S32 fontSize, S32 fontGap);
   ~ChatMessageDisplayer();

   void reset();

   void idle(U32 timeDelta);
   void render(S32 ypos, bool helperVisible, bool anouncementActive, F32 alpha) const;

   void onChatMessageReceived(const Color &msgColor, const string &msg);
   string substituteVars(const string &str);
};

}

#endif
