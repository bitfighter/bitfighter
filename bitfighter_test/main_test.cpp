//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Bitfighter Tests

#define BF_TEST

#include "DisplayManager.h"
#include "FontManager.h"
#include "tnlLog.h"

#include "stringUtils.h"

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


// Returns true if all resource folders are in place, false otherwise
bool checkResources()
{
   const string dirs[] = { "editor_plugins", "fonts", "levels", "music", "robots", "scripts", "sfx", "testing" };

   for(S32 i = 0; i < ARRAYSIZE(dirs); i++)
      if(!fileExists(dirs[i]))
         return false;

      return true;
}


int main(int argc, char **argv) 
{
   // Uncomment to see lots of events... we should do this from time to time and eliminate as many messages as possible
   //const S32 consoleEvents = LogConsumer::AllErrorTypes |
   //                          LogConsumer::LuaLevelGenerator | LogConsumer::LuaBotMessage |
   //                          LogConsumer::ConsoleMsg;
   //StdoutLogConsumer StdoutLog;
   //StdoutLog.setMsgTypes(consoleEvents);

   testing::InitGoogleTest(&argc, argv);

   if(!checkResources())
   {
      logprintf("FAILED: Invalid test environment! Are you sure you copied everything from 'resources/' into 'exe/'?");
      testing::internal::posix::Abort();
   }


   DisplayManager::initialize();
   int returnvalue = RUN_ALL_TESTS();
   FontManager::cleanup();
   DisplayManager::cleanup();
   return returnvalue;
}

