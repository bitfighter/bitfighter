//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CHAT_CHECK_H_
#define _CHAT_CHECK_H_

#include "tnlTypes.h"

using namespace TNL;

namespace Zap
{

class ChatCheck
{
private:
   U32 mMillisecondsClock;
   U32 mChatTimer;
   U32 mChatPrevMessageSum;
   U32 mChatPrevMessageMode;

   bool mChatTimerBlocked;

public:
   bool mChatMute;  // Can mute something forever..

   ChatCheck();
   virtual ~ChatCheck();

   // The chat mode is: 0 = global, 1 = team, 2 = private; used when chatting the same message again, this time global
   bool checkMessage(const char *message, U32 mode);
};

}
#endif
