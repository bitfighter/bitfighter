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

#include "Console.h"       // Our header

#include "ScreenInfo.h"    // For ScreenInfo object
#include "tnlAssert.h"     // For TNLAssert, of course

#include <stdio.h>         // For vsnprintf
#include <stdarg.h>        // For va_args support

#ifdef WIN32
#  define vsnprintf vsnprintf_s    // Use secure version on windows
#endif 


using namespace TNL;

namespace Zap
{
   // Constructor
   Console::Console()
   {
      mConsole = NULL;
   };     


   // Destructor
   Console::~Console()
   {
      if(mConsole)
         quit();
   }


   extern ScreenInfo gScreenInfo;

   void Console::initialize()
   {
      TNLAssert(gScreenInfo.isActualized(), "Must run VideoSystem::actualizeScreenMode() before initializing console!");
      mConsole = OGLCONSOLE_Create();  
   }


   void Console::quit()
   {
      OGLCONSOLE_Quit();
      mConsole = NULL;
   }


   void Console::onScreenModeChanged()
   {
      if(!mConsole)
         return;

      OGLCONSOLE_CreateFont();
      OGLCONSOLE_Reshape();
   }


   void Console::onScreenResized()
   {
      if(!mConsole)
         return;

      OGLCONSOLE_Reshape();
   }


   bool Console::onKeyDown(char ascii)
   {
      return OGLCONSOLE_CharEvent(ascii);
   }


   bool Console::onKeyDown(InputCode inputCode)
   {
      return OGLCONSOLE_KeyEvent(inputCode, InputCodeManager::getState(KEY_SHIFT));
   }


   // User pressed Enter, time to run command
   void Console::setCommandProcessorCallback(void(*callback)(OGLCONSOLE_Console console, char *cmd))
   {
      OGLCONSOLE_EnterKey(callback);
   }


   void Console::render()
   {
      OGLCONSOLE_setCursor((Platform::getRealMilliseconds() / 100) % 2);     // Make cursor blink
      OGLCONSOLE_Draw();   
   }


   bool Console::isVisible()
   {
      return OGLCONSOLE_GetVisibility();
   }


   void Console::show()
   {
      OGLCONSOLE_ShowConsole();
   }


   void Console::hide()
   {
      OGLCONSOLE_HideConsole();
   }


   void Console::toggleVisibility()
   {
      if(isVisible())
         hide();
      else
         show();
   }


   // Print message to console
   void Console::output(const char *format, ...)
   {
      va_list args;
      static char message[MAX_CONSOLE_OUTPUT_LENGTH];    // Reusable buffer

      va_start(args, format);
      vsnprintf(message, sizeof(message), format, args); 
      va_end(args);

      OGLCONSOLE_Output(mConsole, message); 
   }
};


// Consider adding line editing using http://www.cs.utah.edu/~bigler/code/libedit.html
//http://tiswww.case.edu/php/chet/readline/readline.html  <-- programming docs
// http://gnuwin32.sourceforge.net/packages/readline.htm
// also see lua.c

// Code for the lua handler
//static void cli_input( unsigned int wid, char *unused )
//{
//   (void) unused;
//   int status;
//   char *str;
//   lua_State *L;
//   char buf[LINE_LENGTH];
//
//   /* Get the input. */
//   str = window_getInput( wid, "inpInput" );
//
//   /* Ignore useless stuff. */
//   if (str == NULL)
//      return;
//
//   /* Put the message in the console. */
//   nsnprintf( buf, LINE_LENGTH, "%s %s",
//         cli_firstline ? "> " : ">>", str );
//   cli_addMessage( buf );
//
//   /* Set up state. */
//   L = cli_state;
//
//   /* Set up for concat. */
//   if (!cli_firstline)           /* o */
//      lua_pushliteral(L, "\n");  /* o \n */
//
//   /* Load the string. */
//   lua_pushstring( L, str );     /* s */
//
//   /* Concat. */
//   if (!cli_firstline)           /* o \n s */
//      lua_concat(L, 3);          /* s */
//
//   status = luaL_loadbuffer( L, lua_tostring(L,-1), lua_strlen(L,-1), "=cli" );
//
//   /* String isn't proper Lua yet. */
//   if (status == LUA_ERRSYNTAX) {
//      size_t lmsg;
//      const char *msg = lua_tolstring(L, -1, &lmsg);
//      const char *tp = msg + lmsg - (sizeof(LUA_QL("<eof>")) - 1);
//      if (strstr(msg, LUA_QL("<eof>")) == tp) {
//         /* Pop the loaded buffer. */
//         lua_pop(L, 1);
//         cli_firstline = 0;
//      }
//      else {
//         /* Real error, spew message and break. */
//         cli_addMessage( lua_tostring(L, -1) );
//         lua_settop(L, 0);
//         cli_firstline = 1;
//      }
//   }
//   /* Print results - all went well. */
//   else if (status == 0) {
//      lua_remove(L,1);
//      if (lua_pcall(L, 0, LUA_MULTRET, 0)) {
//         cli_addMessage( lua_tostring(L, -1) );
//         lua_pop(L,1);
//      }
//      if (lua_gettop(L) > 0) {
//         lua_getglobal(L, "print");
//         lua_insert(L, 1);
//         if (lua_pcall(L, lua_gettop(L)-1, 0, 0) != 0)
//            cli_addMessage( "Error printing results." );
//      }
//
//      /* Clear stack. */
//      lua_settop(L, 0);
//      cli_firstline = 1;
//   }
//
//   /* Clear the box now. */
//   window_setInput( wid, "inpInput", NULL );
//}
//
