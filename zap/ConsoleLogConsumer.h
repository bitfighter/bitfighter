//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CONSOLE_LOG_CONSUMER_H_
#define _CONSOLE_LOG_CONSUMER_H_

#include "tnlLog.h"

using namespace TNL;

namespace Zap
{

#ifndef BF_NO_CONSOLE

class Console;
// If BF_NO_CONSOLE is defined, console output will be merged into normal stdout logging elsewhere

class ConsoleLogConsumer : public LogConsumer    // Dumps to our game console
{
   typedef LogConsumer Parent;

private:
   Console *mConsole;

   void writeString(const char *string);

public:
   ConsoleLogConsumer(Console *gameConsole);
};

#endif

}

#endif
