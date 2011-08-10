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

#include "UINameEntry.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "UIChat.h"
#include "game.h"
#include "gameConnection.h"
#include "UIRebindKeys.h"
#include "ScreenInfo.h"

#include "SDL/SDL_opengl.h"

namespace Zap
{

// Constructor
RebindKeysUserInterface::RebindKeysUserInterface(ClientGame *game) : Parent(game)
{
   title = "ENTER TEXT:";
   buffer[0] = 0;
   memset(buffer, 0, sizeof(buffer));
   secret = false;
   cursorPos = 0;
   resetOnActivate = true;
}


void RebindKeysUserInterface::onActivate()
{
   if(resetOnActivate)
   {
      buffer[0] = 0;
      cursorPos = 0;
   }
}

void RebindKeysUserInterface::render()
{
   glColor3f(1,1,1);

   const S32 fontSize = 20;
   const S32 fontSizeBig = 30;

   U32 y = (gScreenInfo.getGameCanvasHeight() / 2) - fontSize;

   drawCenteredString(y, fontSize, title);
   y += 45;

   char astbuffer[BUFFER_LENGTH + 1];
   const char *renderBuffer=buffer;
   if(secret)
   {
      S32 i;
      for(i = 0; i < BUFFER_LENGTH; i++)
      {
         if(!buffer[i])
            break;
         astbuffer[i] = '*';
      }
      astbuffer[i] = 0;
      renderBuffer = astbuffer;
   }
   drawCenteredString(y, fontSizeBig, renderBuffer);

   U32 width = getStringWidth(fontSizeBig, renderBuffer);
   S32 x = (gScreenInfo.getGameCanvasWidth() - width) / 2;

   if(LineEditor::cursorBlink)  
      drawString(x + getStringWidthf(fontSizeBig, renderBuffer, cursorPos), y, fontSizeBig, "_");
}

void RebindKeysUserInterface::idle(U32 timeDelta)
{
   LineEditor::updateCursorBlink(timeDelta);
}

void RebindKeysUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   switch (keyCode)
   {
      case KEY_ENTER:
         onAccept(buffer);
         break;
      case KEY_BACKSPACE:
      case KEY_DELETE:
         if(cursorPos > 0)
         {
            cursorPos--;
            for(U32 i = cursorPos; buffer[i]; i++)
               buffer[i] = buffer[i+1];
         }
         break;
      case KEY_ESCAPE:
         onEscape();
         break;
      default:
         if (ascii)
         {
            for(U32 i = BUFFER_LENGTH - 1; i > cursorPos; i--)
               buffer[i] = buffer[i-1];
            if(cursorPos < BUFFER_LENGTH-1)
            {
               buffer[cursorPos] = ascii;
               cursorPos++;
            }
         }
         break;
   }
}

void RebindKeysUserInterface::setText(const char *text)
{
   if(strlen(text) > BUFFER_LENGTH)
   {
      strncpy(buffer, text, BUFFER_LENGTH);
      buffer[BUFFER_LENGTH] = 0;
   }
   else
      strcpy(buffer, text);
}


};


