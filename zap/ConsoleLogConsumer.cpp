//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ConsoleLogConsumer.h"
#include "Console.h"


namespace Zap
{

static const S32 consoleEvents = LogConsumer::AllErrorTypes | LogConsumer::LuaLevelGenerator | 
                                 LogConsumer::LuaBotMessage | LogConsumer::ConsoleMsg;

#ifndef BF_NO_CONSOLE

// Constructor
ConsoleLogConsumer::ConsoleLogConsumer(Console *gameConsole) : Parent()
{
   mConsole = gameConsole;
   setMsgTypes(consoleEvents);   // These are the types of events we'll log to the console
}


void ConsoleLogConsumer::writeString(const char *string)
{
   TNLAssert(mConsole, "Expect a console here!");
   mConsole->output(string);
}

#endif

}