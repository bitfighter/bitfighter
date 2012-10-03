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

#if defined(ZAP_DEDICATED) || defined(TNL_OS_MOBILE)

namespace Zap {

class Console
{

public:
   // Do nothing
   void output(const char *s, ...) {}
   bool onKeyDown(char ascii) {}
   bool onKeyDown(InputCode inputCode) {}
   bool isVisible() {}
   void show() {}
   bool isOk() {}
   void toggleVisibility() {}
   void initialize() {}
   void onScreenModeChanged() {}
};



#else

#include "LuaScriptRunner.h"     // Parent class
#include "InputCode.h"           // For InputCodeManager and associated enums
#include "oglconsole.h"
#include "lua.h"
#include "tnlTypes.h"


using namespace TNL;

namespace Zap
{

class Console: public LuaScriptRunner
{
   typedef LuaScriptRunner Parent;

private:
   OGLCONSOLE_Console mConsole;

public:
   Console();     // Constructor
   ~Console();    // Destructor

   void initialize();
   void quit();

   bool prepareEnvironment();

   // Handle events
   void onScreenModeChanged();
   void onScreenResized();
   bool onKeyDown(char ascii);
   bool onKeyDown(InputCode inputCode);
   void setCommandProcessorCallback(void(*callback)(OGLCONSOLE_Console console, char *cmd));
   static void processConsoleCommandCallback(OGLCONSOLE_Console console, char *cmdline);
   void processCommand(const char *cmdline);

   void render();

   bool isVisible();
   void show();
   void hide();
   void toggleVisibility();

   bool isOk();

   void output(const char *s, ...);    // Print message to console
};

#endif // defined(ZAP_DEDICATED) || defined(TNL_OS_MOBILE)

// Provide transparent access to our global console instance
extern Console gConsole;

};

#endif
