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

#include "luaObject.h"

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
   //static lua_State *L = NULL;    

   // Constructor
   Console::Console()
   {
      mConsole = NULL;
      mScriptId = "console";    // Overwrite default name with something custom
      mErrorMsgPrefix = "Console";
   };     


   // Destructor
   Console::~Console()
   {
      if(mConsole)
         quit();

      if(L)
         lua_close(L);
   }


   extern ScreenInfo gScreenInfo;

   void Console::initialize()
   {
      TNLAssert(gScreenInfo.isActualized(), "Must run VideoSystem::actualizeScreenMode() before initializing console!");
      TNLAssert(!mConsole,                  "Only intialize once!");

      mConsole = OGLCONSOLE_Create(); 

      if(!mConsole)
         return;

      startLua(LEVELGEN);

      setCommandProcessorCallback(processConsoleCommandCallback);
   }


   // TODO: Merge with luaLevelGenerator version, which is almost identical
   bool Console::prepareEnvironment()  
   { 
      Parent::prepareEnvironment();

      if(!loadAndRunGlobalFunction(L, LUA_HELPER_FUNCTIONS_KEY) || !loadAndRunGlobalFunction(L, LEVELGEN_HELPER_FUNCTIONS_KEY))
         return false;

      TNLAssert(lua_gettop(L) == 0 || LuaObject::dumpStack(L), "Stack not cleared!");

      return true;
   }


   void Console::quit()
   {
      OGLCONSOLE_Quit();
      mConsole = NULL;
   }


   bool Console::isOk()
   {
      return mConsole && L;
   }


   // Structure of this code borrowed from naev
   void Console::processConsoleCommandCallback(OGLCONSOLE_Console console, char *cmdline)
   {
      gConsole.processCommand(cmdline);
   }


   static string consoleCommand = "";

   // Structure of this code influenced by naev
   void Console::processCommand(const char *cmdline)
   {
      if(consoleCommand == "" && strcmp(cmdline, "") == 0)
         return;

      setEnvironment();

      // If we are not on the first line of our command, we need to append the command to our existing line
      if(consoleCommand == "")      
         consoleCommand = cmdline;
      else
         consoleCommand += "\n" + string(cmdline);

      S32 status = luaL_loadbuffer(L, consoleCommand.c_str(), consoleCommand.length(), "ConsoleInput" );
      
      
      if(status == LUA_ERRSYNTAX)      // cmd is not a complete Lua statement yet -- need to add more input
      {
         size_t lmsg;
         const char *msg = lua_tolstring(L, -1, &lmsg);
         const char *tp = msg + lmsg - (sizeof(LUA_QL("<eof>")) - 1);

         if(strstr(msg, LUA_QL("<eof>")) == tp) 
            lua_pop(L, 1);
         else 
         {
            // Error -- print to console
            output("%s\n", lua_tostring(L, -1));
            consoleCommand = "";       // Reset command
         }
      }

      // Success -- print results to console
      else if(status == 0) 
      {
         //lua_remove(L, 1);
         if(lua_pcall(L, 0, LUA_MULTRET, 0)) 
         {
            output("%s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
         }
         if(lua_gettop(L) > 0) 
         {
            //output(lua_tostring(L, -1));     // Delme

            lua_getglobal(L, "print");
            lua_insert(L, 1);
            if(lua_pcall(L, lua_gettop(L) - 1, 0, 0) != 0)
               output("Error printing results.");
         }
      
         consoleCommand = "";    // Reset command
      }

      LuaObject::clearStack(L);
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

