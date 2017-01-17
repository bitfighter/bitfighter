//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Renders a simple message box, and waits for the user to hit a key
// None of this is currently used...  Why did I write it???

#include "UIMessage.h"

#include "UIManager.h"

#include "RenderUtils.h"
#include "Colors.h"

namespace Zap
{

MessageUserInterface::MessageUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   // Do nothing
}


// Destructor
MessageUserInterface::~MessageUserInterface()
{
   // Do nothing
}


void MessageUserInterface::onActivate()
{
   if(mFadeTime)
      mFadeTimer.reset(mFadeTime);
}


void MessageUserInterface::reset()
{
   mTitle = (char*)"Message";     // Default title
   mWidth = DisplayManager::getScreenInfo()->getGameCanvasWidth() - 200;
   mHeight = DisplayManager::getScreenInfo()->getGameCanvasHeight() - 200;
   mFadeTime = 0;          // 0 --> "Hit any key to continue"
   mMessageColor = Colors::white;
   mVertOffset = 0;
   mBox = true;

   for(S32 i = 0; i < mNumLines; i++)
      mMessage[i] = (char*)"";
}


// First line is 1
void MessageUserInterface::setMessage(S32 id, char *message)
{
   if (id < 1 || id > mNumLines)       // Protect devs from themselves...
      logprintf(LogConsumer::LogError, "Invalid line number in setMessage: %d", id);
   else
      mMessage[id-1] = message;
}


void MessageUserInterface::setTitle(char *message)
{
   mTitle = message;
}


void MessageUserInterface::setSize(U32 width, U32 height)
{
   mWidth = width;
   mHeight = height;
}


void MessageUserInterface::setFadeTime(U32 time)
{
   mFadeTime = time;
}


void MessageUserInterface::setStyle(U32 style)
{
   mFadeTime = 3500;
   mBox = false;
   mMessageColor = Colors::ErrorMessageTextColor;
   mVertOffset = 0;
   mTitle = (char*)"";
}


void MessageUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();
}


bool MessageUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode))
      return true;

   if(!mFadeTime)    // If message isn't the fading sort, then...
      quit();        // ...quit the interface when any key is pressed...  any key at all.  Sort of.

   return false;
}


void MessageUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   if(mFadeTime && mFadeTimer.update(timeDelta))
      quit();
}


void MessageUserInterface::render() const
{
   const S32 canvasWidth  = DisplayManager::getScreenInfo()->getGameCanvasWidth();
   const S32 canvasHeight = DisplayManager::getScreenInfo()->getGameCanvasHeight();

   S32 hInset = (canvasHeight - mHeight) / 2;
   S32 wInset = (canvasWidth - mWidth) / 2;

   // Fade effect
   F32 fadeFactor;
   if(mFadeTimer.getCurrent() < 1000)      // getCurrent returns time remaining
      fadeFactor = (F32) mFadeTimer.getCurrent() / 1000.f;
   else
      fadeFactor = 1;

   getUIManager()->renderPrevUI(this);

   if(mBox)
   {
      S32 x1 = wInset + mVertOffset;
      S32 y1 = hInset;
      S32 w = canvasWidth - 2*(wInset + mVertOffset);
      S32 h = canvasHeight - 2*hInset;

      RenderUtils::drawFilledRect(x1, y1, w, h, Colors::red30, fadeFactor * 0.95f, Colors::white, fadeFactor);
   }

   // Draw title, message, and footer
   if(strcmp(mTitle, ""))  // If they are different
      RenderUtils::drawCenteredString_fixed(vertMargin + hInset + mVertOffset + 30, 30, mMessageColor, fadeFactor, mTitle);

   for(S32 i = 0; i < mNumLines; i++)
      RenderUtils::drawCenteredString_fixed(vertMargin + 58 + hInset + i * 24 + mVertOffset, 18, mMessageColor, fadeFactor, mMessage[i]);

   if (!mFadeTime)
      RenderUtils::drawCenteredString_fixed(canvasHeight - vertMargin - hInset + mVertOffset, 18, mMessageColor, fadeFactor, "Hit any key to continue");
}

};


