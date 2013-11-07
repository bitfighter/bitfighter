//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "InputCode.h"           // For InputCodeManager and associated enums

#if defined(ZAP_DEDICATED) || defined(TNL_OS_MOBILE)
#define BF_NO_CONSOLE
#endif

#ifdef BF_NO_CONSOLE

namespace Zap {

class Console
{

public:
   // Do nothing
   void output(const char *s, ...) {}
   bool onKeyDown(char ascii) { return false; }
   bool onKeyDown(InputCode inputCode) { return false; }
   bool isVisible() { return false; }
   void show() {}
   bool isOk() { return false; }
   void toggleVisibility() {}
   void initialize() {}
   void onScreenModeChanged() {}
   void onScreenResized() {}
};



#else

#include "LuaScriptRunner.h"     // Parent class
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

protected:
   void killScript();

public:
   Console();     // Constructor
   virtual ~Console();    // Destructor

   void initialize();
   void quit();

   const char *getErrorMessagePrefix();

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

#endif // BF_NO_CONSOLE

// Provide transparent access to our global console instance
extern Console gConsole;

};

#endif
