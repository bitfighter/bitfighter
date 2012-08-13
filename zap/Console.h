//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
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

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "InputCode.h"     // for InputCodeManager and associated enums
#include "oglconsole.h"
#include "tnlTypes.h"

using namespace TNL;

namespace Zap
{

class Console {

private:
   OGLCONSOLE_Console mConsole;
   // Lua interpreter

public:
   Console();     // Constructor
   ~Console();    // Destructor

   void initialize();
   void quit();

   // Handle events
   void onScreenModeChanged();
   void onScreenResized();
   bool onKeyDown(char ascii);
   bool onKeyDown(InputCode inputCode);
   void setCommandProcessorCallback(void(*callback)(OGLCONSOLE_Console console, char *cmd));

   void render();

   bool isVisible();
   void show();
   void hide();
   void toggleVisibility();

   void output(const char *s, ...);    // Print message to console
};

// Provide transparent access to our global console instance
extern Console gConsole;

};

#endif