//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Renders a simple message box, and waits for the user to hit a key
// None of this is currently used...  Why did I write it???

#include "UIMessage.h"

#include "UIManager.h"

#include "Colors.h"

#include "RenderUtils.h"
#include "OpenglUtils.h"

#include <stdio.h>

namespace Zap
{

MessageUserInterface::MessageUserInterface(ClientGame *game) : Parent(game)
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
   mWidth = gScreenInfo.getGameCanvasWidth() - 200;
   mHeight = gScreenInfo.getGameCanvasHeight() - 200;
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


void MessageUserInterface::render()
{
   const F32 canvasWidth  = (F32)gScreenInfo.getGameCanvasWidth();
   const F32 canvasHeight = (F32)gScreenInfo.getGameCanvasHeight();

   F32 hInset = F32(canvasHeight - mHeight) / 2;
   F32 wInset = F32(canvasWidth - mWidth) / 2;

   // Fade effect
   F32 fadeFactor;
   if(mFadeTimer.getCurrent() < 1000)      // getCurrent returns time remaining
      fadeFactor = (F32) mFadeTimer.getCurrent() / 1000.f;
   else
      fadeFactor = 1;

   getUIManager()->renderPrevUI(this);

   if(mBox)
   {
      F32 vertices[] = {
            wInset + mVertOffset,               hInset,
            canvasWidth - wInset + mVertOffset, hInset,
            canvasWidth - wInset + mVertOffset, canvasHeight - hInset,
            wInset + mVertOffset,               canvasHeight - hInset
      };

      glColor(Colors::red30, fadeFactor * 0.95f);  // Draw a box
      renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_TRIANGLE_FAN);

      glColor(Colors::white, fadeFactor);          // Add a border
      renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINE_LOOP);
   }

   // Draw title, message, and footer
   glColor(mMessageColor, fadeFactor);

   if(strcmp(mTitle, ""))  // If they are different
      drawCenteredString(vertMargin + hInset + mVertOffset, 30, mTitle);

   for(S32 i = 0; i < mNumLines; i++)
      drawCenteredString(vertMargin + 40 + hInset + i * 24 + mVertOffset, 18, mMessage[i]);

   if (!mFadeTime)
      drawCenteredString(canvasHeight - vertMargin - hInset - 18 + mVertOffset, 18, "Hit any key to continue");
}

};


