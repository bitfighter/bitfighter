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

#ifndef _UIREBINDKEYS_H_
#define _UIREBINDKEYS_H_

#include "UI.h"
#include "timer.h"

namespace Zap
{

class RebindKeysUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   static const S32 BUFFER_LENGTH = 32;

protected:
   const char *title;
   char buffer[BUFFER_LENGTH + 1];
   U32 cursorPos;
   bool secret;
   bool resetOnActivate;

public:
   RebindKeysUserInterface(Game *game);      // Constructor

   void onActivate();
   void render();
   void idle(U32 t);

   void onKeyDown(KeyCode keyCode, char ascii);

   virtual void onAccept(const char *text) = 0;
   virtual void onEscape() = 0;
   const char *getText() { return buffer; }
   void setText(const char *text);
};


};

#endif

