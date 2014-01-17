//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ChatCheck.h"

#include "Intervals.h"
#include "tnlPlatform.h"

using namespace TNL;

namespace Zap
{


ChatCheck::ChatCheck()
{
   mChatMute = false;
   mChatTimer = 0;
   mChatTimerBlocked = false;
   mMillisecondsClock = 0;
   mChatPrevMessageSum = 0;
   mChatPrevMessageMode = 0;
}

// Destructor
ChatCheck::~ChatCheck()
{
   // Do nothing
}


bool ChatCheck::checkMessage(const char *message, U32 mode)
{
   // Currently only checks if a person tried to chat too fast and flood the chat area.
   // Can go farther by checking if the message contains inappropriate or blocked words, and return false if it does.

   if(mChatMute)
      return false;

   U32 millisecondsClock = Platform::getRealMilliseconds();

	if(mMillisecondsClock - millisecondsClock < 10)       // Makes up for inaccurate clock slightly going backwards by 1 or 2 ms
      millisecondsClock = mMillisecondsClock;  

   U32 milliseconds = millisecondsClock - mMillisecondsClock;

   if(mChatTimer > milliseconds)
      mChatTimer -= milliseconds;
   else
   {
      mChatTimer = 0;
      mChatTimerBlocked = false;
   }

   mMillisecondsClock = millisecondsClock;

   if(mChatTimerBlocked)
      return false;

   U32 chatPrevMessageSum = 0;
   for(S32 i = 0; message[i] != 0; i++)
      chatPrevMessageSum += U32(message[i]) * (i + 0x4EC691);

   if(chatPrevMessageSum == mChatPrevMessageSum && mChatPrevMessageMode >= mode && mode <= 1 && mChatTimer != 0)
      return false;

   mChatPrevMessageSum = chatPrevMessageSum;
   mChatPrevMessageMode = mode;

   mChatTimer += TWO_SECONDS;

   if(mChatTimer > (U32)SIX_SECONDS)
   {
      mChatTimer += TWO_SECONDS;
      mChatTimerBlocked = true;
      return false;
   }

   return true;
}

}
