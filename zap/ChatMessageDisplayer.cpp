//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ChatMessageDisplayer.h"

#include "DisplayManager.h"
#include "FontManager.h"
#include "GameSettings.h"
#include "InputCode.h"
#include "RenderUtils.h"
#include "Renderer.h"
#include "ScissorsManager.h"
#include "stringUtils.h"

#include "config.h"

namespace Zap
{

void ColorString::set(const string &s, const Color &c, U32 id)
{
   str = s;
   color = c;
   groupId = id;
}


static string getSubstVarVal(ClientGame *game, const string &var)
{
   InputCode inputCode = game->getSettings()->getInputCodeManager()->getKeyBoundToBindingCodeName(var);
   if(inputCode != KEY_UNKNOWN)
      return string("[") + InputCodeManager::inputCodeToString(inputCode) + "]";

   if(caseInsensitiveStringCompare(var, "playerName"))
      return game->getClientInfo()->getName().getString();

   return "%" + var + "%";
}


ChatMessageDisplayer::ChatMessageDisplayer(ClientGame *game, S32 msgCount, bool expire, bool topDown, S32 wrapWidth, S32 fontSize, S32 fontWidth)
{
   mDisplayChatMessageTimer.setPeriod(5000);
   mChatScrollTimer.setPeriod(100);

   mMessages.resize(msgCount + 1);

   reset();

   mGame      = game;
   mExpire    = expire;
   mTopDown   = topDown;
   mWrapWidth = wrapWidth;
   mFontSize  = fontSize;
   mFontGap   = fontWidth;

   mNextGroupId = 0;
}


ChatMessageDisplayer::~ChatMessageDisplayer() { /* Do nothing */ }


void ChatMessageDisplayer::reset()
{
   mFirst = mLast = 0;
   mFull = false;
}


void ChatMessageDisplayer::idle(U32 timeDelta)
{
   mChatScrollTimer.update(timeDelta);

   if(mExpire && mDisplayChatMessageTimer.update(timeDelta))
   {
      mDisplayChatMessageTimer.reset();

      if(mFirst > mLast)
      {
         if(mTopDown)
            mChatScrollTimer.reset();

         advanceLast();
      }
   }
}


void ChatMessageDisplayer::advanceFirst()
{
   ++mFirst;


   if(mLast % mMessages.size() == mFirst % mMessages.size())
   {
      ++mLast;

      mFull = true;
   }
}


void ChatMessageDisplayer::advanceLast()
{
   ++mLast;


   U32 id = mMessages[mLast % mMessages.size()].groupId;

   while(mMessages[(mLast + 1) % mMessages.size()].groupId == id && mFirst > mLast)
      ++mLast;


   mFull = false;

   TNLAssert(mLast <= mFirst, "index error! -- add check to correct this!");
}


void ChatMessageDisplayer::onChatMessageReceived(const Color &msgColor, const string &msg)
{
   FontManager::pushFontContext(ChatMessageContext);
   Vector<string> lines = wrapString(substituteVars(msg), mWrapWidth, mFontSize, "      ");
   FontManager::popFontContext();

   for(S32 i = 0; i < lines.size(); ++i)
   {
      advanceFirst();
      mMessages[mFirst % mMessages.size()].set(lines[i], msgColor, mNextGroupId);
   }

   ++mNextGroupId;


   mDisplayChatMessageTimer.reset();

   if(!mTopDown)
      mChatScrollTimer.reset();
}


string ChatMessageDisplayer::substituteVars(const string &str)
{
   string s = str;

   bool inside = false;
   std::size_t startPos = 0;
   std::size_t endPos = 0;

   for(std::size_t i = 0; i < s.length(); ++i)
   {
      if(s[i] == '%')
      {
         if(!inside)
         {
            startPos = i + 1;
            inside = true;
         }
         else
         {
            endPos = i - startPos;
            inside = false;

            string var = s.substr(startPos, endPos);
            string val = getSubstVarVal(mGame, var);

            s.replace(startPos - 1, endPos + 2, val);

            i += val.length() - var.length() - 2;
         }
      }
   }

   return s;
}


void ChatMessageDisplayer::render(S32 anchorPos, bool helperVisible, bool announcementActive, F32 alpha) const
{
   bool isScrolling = (mChatScrollTimer.getCurrent() > 0);

   if(mFirst == mLast && !(mTopDown && isScrolling))
      return;

   S32 lineHeight = mFontSize + mFontGap;

   static ScissorsManager scissorsManager;

   if(isScrolling)
   {
      S32 displayAreaHeight = (mMessages.size() - 1) * lineHeight;
      S32 displayAreaYPos = anchorPos + (mTopDown ? displayAreaHeight : lineHeight);

      scissorsManager.enable(true, mGame->getSettings()->getIniSettings()->mSettings.getVal<DisplayMode>("WindowMode"),
                             0.0f, F32(displayAreaYPos - displayAreaHeight), F32(DisplayManager::getScreenInfo()->getGameCanvasWidth()), F32(displayAreaHeight));
   }

   S32 y = anchorPos + S32(mChatScrollTimer.getFraction() * lineHeight);

   if(mTopDown)
      y += (mFirst - mLast - 1) * lineHeight;

   S32 renderExtra = 0;
   if(isScrolling)
   {
      if(mTopDown)
         renderExtra = 1;
      else if(mFull)
         renderExtra = 1;
   }

   U32 last = mLast;
   if(announcementActive)
   {
      if(!mExpire && mFirst >= (U32)mMessages.size() - 1)
         ++last;


      y -= lineHeight;
   }

   FontManager::pushFontContext(ChatMessageContext);

   for(U32 i = mFirst; i != last - renderExtra; --i)
   {
      U32 index = i % (U32)mMessages.size();

      Renderer::get().setColor(mMessages[index].color, alpha);

      drawString(UserInterface::horizMargin, y, mFontSize, mMessages[index].str.c_str());

      y -= lineHeight;
   }

   FontManager::popFontContext();

   scissorsManager.disable();
}

}
