//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Bitfighter Tests

#define BF_TEST

#include "DisplayManager.h"
#include "FontManager.h"
#include "tnlLog.h"

#include "tnl.h"

#include <gtest/gtest.h>

namespace Zap
{
void exitToOs(S32 errcode) { TNLAssert(false, "Should never be called!"); }
void shutdownBitfighter()  { TNLAssert(false, "Should never be called!"); };
}

/**
 * Don't put tests in here! Use one file per class. Your tests go in the file
 * corresponding to the class which is the main subject of your tests.
 */

using namespace Zap;
int main(int argc, char **argv) 
{
   // Uncomment to see lots of events... we should do this from time to time and eliminate as many messages as possible
   //const S32 consoleEvents = LogConsumer::AllErrorTypes |
   //                          LogConsumer::LuaLevelGenerator | LogConsumer::LuaBotMessage |
   //                          LogConsumer::ConsoleMsg;
   //StdoutLogConsumer StdoutLog;
   //StdoutLog.setMsgTypes(consoleEvents);

   testing::InitGoogleTest(&argc, argv);
   DisplayManager::initialize();
   int returnvalue = RUN_ALL_TESTS();
   FontManager::cleanup();
   DisplayManager::cleanup();
   return returnvalue;
}

