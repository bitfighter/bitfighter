#ifndef _CHAT_CHECK_H_
#define _CHAT_CHECK_H_
#include "tnlTypes.h"

namespace Zap{
class ChatCheck
{
   TNL::U32 mMillisecondsClock;
   TNL::U32 mChatTimer;
   TNL::U32 mChatPrevMessageSum;
   TNL::U32 mChatPrevMessageMode;
   bool mChatTimerBlocked;
public:
   bool mChatMute;  // can mute something forever..

   ChatCheck();
   // the chat mode is: 0 = global, 1 = team, 2 = private; used when chatting the same message again, this time global
   bool checkMessage(const char *message, TNL::U32 mode);
};
}
#endif
