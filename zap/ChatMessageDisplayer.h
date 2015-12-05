//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CHAT_MESSAGE_DISPLAYER_H_
#define _CHAT_MESSAGE_DISPLAYER_H_

#ifdef ZAP_DEDICATED
#  error "ChatMessageDisplayer.h should not be included in dedicated build"
#endif


#include "RenderManager.h"

#include "Color.h"
#include "Timer.h"

using namespace std;

namespace Zap
{


struct ColorTimerString
{
   Color color;
   string str;
   U32 groupId;
   Timer timer;
   Timer fadeTimer;

   void set(const string &s, const Color &c, S32 time, U32 groupId);
   bool idle(U32 timeDelta);
};


////////////////////////////////////////
////////////////////////////////////////

class ClientGame;

class ChatMessageDisplayer: public RenderManager
{
private:
   enum MessageDisplayMode {
      ShortTimeout,            // Traditional message display mode (6 MessageDisplayCount lines, messages timeout after DisplayMessageTimeout)
      ShortFixed,              // Same length as ShortTimeout, but without timeout
      LongFixed,               // Long form: Display MessageStoreCount messages, no timout
      MessageDisplayModes
   };

   U32 mFirst, mLast;
   bool mTopDown;    // Render from top down or bottom up?
   S32 mWrapWidth;
   S32 mFontSize, mFontGap;
   bool mFull;       // Our message displayer is full up
   MessageDisplayMode mDisplayMode;
   S32 mMessagesToShowInShortMode;
   
   U32 mNextGroupId;

   void advanceFirst();

   Timer mChatScrollTimer;

   ClientGame *mGame;

   // These are the messages, their colors, and their expiration timers 
   Vector<ColorTimerString> mMessages;
   string substitueVars(const string &str) const;

   // Translate the show message style enum into behaviors
   S32 getNumberOfMessagesToShow(bool composingMessage) const;
   bool showExpiredMessages(bool composingMessage) const;

public:
   // Constructor
   ChatMessageDisplayer(ClientGame *game, S32 msgCount, bool topDown, S32 wrapWidth, S32 fontSize, S32 fontGap);
   virtual ~ChatMessageDisplayer();

   void reset();

   void idle(U32 timeDelta);
   void render(S32 ypos, F32 helperFadeIn, bool composingMessage, bool anouncementActive, F32 alpha) const;   // Render incoming chat msgs

   void onChatMessageReceived(const Color &msgColor, const string &msg);
   void toggleDisplayMode();
};




}

#endif
