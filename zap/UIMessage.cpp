//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

// Renders a simple message box, and waits for the user to hit a key
// None of this is currently used...  Why did I write it???

#include "UIMessage.h"
#include "UIMenus.h"
#include "Colors.h"
#include "ScreenInfo.h"

#include "SDL/SDL_opengl.h"

#include <stdio.h>

namespace Zap
{

extern Color gErrorMessageTextColor;

MessageUserInterface::MessageUserInterface(Game *game) : Parent(game) { /* Do nothing */ }

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
   mMessageColor = gErrorMessageTextColor;
   mVertOffset = 0;
   mTitle = (char*)"";
}


void MessageUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();
}


void MessageUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(!mFadeTime)  // If message isn't the fading sort, then...
      quit();      // ...quit the interface when any key is pressed...  any key at all.
}


void MessageUserInterface::idle(U32 timeDelta)
{
   if(mFadeTime && mFadeTimer.update(timeDelta))
      quit();
}


void MessageUserInterface::render()
{
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   U32 hInset = (canvasHeight - mHeight) / 2;
   U32 wInset = (canvasWidth - mWidth) / 2;

   // Fade effect
   F32 fadeFactor;
   if(mFadeTimer.getCurrent() < 1000)      // getCurrent returns time remaining
      fadeFactor = (F32) mFadeTimer.getCurrent() / 1000.0;
   else
      fadeFactor = 1;


   getUIManager()->renderPrevUI();

   glEnableBlend;


   if(mBox)
   {
      glColor4f(.3, 0, 0, fadeFactor * 0.95);    // Draw a box
      glBegin(GL_POLYGON);
         glVertex(wInset + mVertOffset, hInset);
         glVertex(canvasWidth - wInset + mVertOffset, hInset);
         glVertex(canvasWidth - wInset + mVertOffset, canvasHeight - hInset);
         glVertex(wInset + mVertOffset, canvasHeight - hInset);
      glEnd();

      glColor4f(1, 1, 1, fadeFactor); // Add a border
      glBegin(GL_LINE_LOOP);
         glVertex(wInset + mVertOffset, hInset);
         glVertex(canvasWidth - wInset + mVertOffset, hInset);
         glVertex(canvasWidth - wInset + mVertOffset, canvasHeight - hInset);
         glVertex(wInset + mVertOffset, canvasHeight - hInset);
      glEnd();
   }

   // Draw title, message, and footer
   glColor4f(mMessageColor.r, mMessageColor.g, mMessageColor.b, fadeFactor);

   if(mTitle != "")
      drawCenteredString(vertMargin + hInset + mVertOffset, 30, mTitle);

   for(S32 i = 0; i < mNumLines; i++)
      drawCenteredString(vertMargin + 40 + hInset + i * 24 + mVertOffset, 18, mMessage[i]);

   if (!mFadeTime)
      drawCenteredString(canvasHeight - vertMargin - hInset - 18 + mVertOffset, 18, "Hit any key to continue");

   glDisableBlend;

}

};


