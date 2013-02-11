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

#ifndef _UIMSG_H_
#define _UIMSG_H_

#include "UI.h"
#include "game.h"

namespace Zap
{

class MessageUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   static const S32 mNumLines = 9;
   char *mTitle;
   U32 mHeight;
   U32 mWidth;
   U32 mFadeTime;    // Time message will be displayed (ms)   0 -> "Hit any key to continue"
   bool mBox;        // Draw box around message?
   Color mMessageColor;
   Timer mFadeTimer;
   U32 mVertOffset;

public:
   explicit MessageUserInterface(ClientGame *game);     // Constructor
   char *mMessage[mNumLines];
   void onActivate();
   void setMessage (S32 id, char *message);  // Set a line of message
   void setTitle(char *message);             // Set menu title
   void setSize(U32 width, U32 height);      // Set size of menu
   void setFadeTime(U32 time);
   void setStyle(U32 style);                 // Use a preset menu style
   void reset();
   void idle(U32 t);
   void render();
   void quit();
   bool onKeyDown(InputCode inputCode);
};


}

#endif


