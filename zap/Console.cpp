//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Console.h"       // Our header

#ifndef BF_NO_CONSOLE

#  include "DisplayManager.h"   // For ScreenInfo object
#  include "tnlAssert.h"        // For TNLAssert, of course

#  include <stdio.h>            // For vsnprintf
#  include <stdarg.h>           // For va_args support

   using namespace TNL;

#endif

namespace Zap
{


Console gConsole;    // For the moment, we'll just have one console for everything.  This may change later, but probably won't.

// Constructor
Console::Console()
{
   mConsole = NULL;

   mScriptId = "console";    // Overwrite default name with something custom
   mScriptType = ScriptTypeConsole;
};     


// Destructor
Console::~Console()
{
   if(mConsole)
      quit();

   if(L)
   {
      lua_close(L);
      L = NULL;
   }
}


void Console::initialize()
{
#ifndef BF_NO_CONSOLE

   TNLAssert(DisplayManager::getScreenInfo()->isActualized(), "Must run VideoSystem::actualizeScreenMode() before initializing console!");
   TNLAssert(!mConsole,                  "Only intialize once!");

   mConsole = OGLCONSOLE_Create(); 

   if(!mConsole)
      return;

   prepareEnvironment();   

   setCommandProcessorCallback(processConsoleCommandCallback);

#endif
}


const char *Console::getErrorMessagePrefix() { return "Console"; }


// TODO: Merge with luaLevelGenerator version, which is almost identical
bool Console::prepareEnvironment()  
{ 
#ifndef BF_NO_CONSOLE

   if(!Parent::prepareEnvironment())
      return false;

   setScriptContext(L, ConsoleContext);

   TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack not cleared!");

#endif

   return true;
}


void Console::quit()
{
#ifndef BF_NO_CONSOLE

   OGLCONSOLE_Quit();
   mConsole = NULL;

#endif
}


void Console::killScript()
{
   // Do nothing
}


bool Console::isOk()
{
#ifndef BF_NO_CONSOLE

   return mConsole && L;

#else
   return false;
#endif
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
#ifndef BF_NO_CONSOLE

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
      std::size_t lmsg;
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
      if(lua_pcall(L, 0, LUA_MULTRET, 0)) 
      {
         output("%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }
      if(lua_gettop(L) > 0) 
      {
         lua_getglobal(L, "print");
         lua_insert(L, 1);
         if(lua_pcall(L, lua_gettop(L) - 1, 0, 0) != 0)
            output("Error printing results.");
      }
      
      consoleCommand = "";    // Reset command
   }

   clearStack(L);

#endif
}


void Console::onScreenModeChanged()
{
#ifndef BF_NO_CONSOLE

   if(!mConsole)
      return;

   OGLCONSOLE_CreateFont();
   OGLCONSOLE_Reshape();

#endif
}


void Console::onScreenResized()
{
#ifndef BF_NO_CONSOLE

   if(!mConsole)
      return;

   OGLCONSOLE_Reshape();

#endif
}


bool Console::onKeyDown(char ascii)
{
#ifndef BF_NO_CONSOLE

   return OGLCONSOLE_CharEvent(ascii);

#else
   return false;
#endif
}


bool Console::onKeyDown(InputCode inputCode)
{
#ifndef BF_NO_CONSOLE

   return OGLCONSOLE_KeyEvent(inputCode, InputCodeManager::getState(KEY_SHIFT));

#else
   return false;
#endif
}


// User pressed Enter, time to run command
void Console::setCommandProcessorCallback(void(*callback)(OGLCONSOLE_Console console, char *cmd))
{
#ifndef BF_NO_CONSOLE

   OGLCONSOLE_EnterKey(callback);

#endif
}


void Console::render() const
{
#ifndef BF_NO_CONSOLE

   OGLCONSOLE_setCursor((Platform::getRealMilliseconds() / 100) % 2);     // Make cursor blink
   OGLCONSOLE_Draw();   

#endif
}


bool Console::isVisible()
{
#ifndef BF_NO_CONSOLE

   return OGLCONSOLE_GetVisibility();

#else
   return false;
#endif
}


void Console::show()
{
#ifndef BF_NO_CONSOLE

   OGLCONSOLE_ShowConsole();

#endif
}


void Console::hide()
{
#ifndef BF_NO_CONSOLE

   OGLCONSOLE_HideConsole();

#endif
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
#ifndef BF_NO_CONSOLE

   va_list args;
   static char message[MAX_CONSOLE_OUTPUT_LENGTH];    // Reusable buffer

   va_start(args, format);
   vsnprintf(message, sizeof(message), format, args); 
   va_end(args);

   OGLCONSOLE_Output(mConsole, message); 

#endif
}


// Consider adding line editing using http://www.cs.utah.edu/~bigler/code/libedit.html
//http://tiswww.case.edu/php/chet/readline/readline.html  <-- programming docs
// http://gnuwin32.sourceforge.net/packages/readline.htm
// also see lua.c
};
