//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Bitfighter Tests

#define BF_TEST

#include "DisplayManager.h"
#include "FontManager.h"
#include "GameManager.h"
#include "GameSettings.h"
#include "VideoSystem.h"
#include "physfs.hpp"

#include "stringUtils.h"

#include "tnlLog.h"

#ifdef TNL_OS_WIN32 
#  include <windows.h>     // For ARRAYSIZE def
#endif

#include <gtest/gtest.h>

namespace Zap
{
void exitToOs()            { TNLAssert(false, "Should never be called!"); }
void exitToOs(S32 errcode) { TNLAssert(false, "Should never be called!"); }
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


// This is a global setup object; it will be called before and after any tests are run
class GlobalTestEnvironment : public ::testing::Environment
{
public:
   virtual ~GlobalTestEnvironment() {}

   // Override this to define how to set up the environment
   virtual void SetUp() 
   { 
      InputCodeManager::initializeKeyNames();
      RenderManager::init();
      GameSettings settings;
      FontManager::initialize(settings.get(), false);
      VideoSystem::init();
      PhysFS::init("");    // Should be argv[0]... see how this works instead
      VideoSystem::actualizeScreenMode(&settings, false, false);
      GameManager::initialize();
   }


   // Override this to define how to tear down the environment
   virtual void TearDown() 
   {
      // Do nothing
   }
};


int main(int argc, char **argv) 
{
   //GlobalTestEnvironment *AddGlobalTestEnvironment(GlobalTestEnvironment* env);
   ::testing::AddGlobalTestEnvironment(new GlobalTestEnvironment());

   // Uncomment to see lots of events... we should do this from time to time and eliminate as many messages as possible
   //const S32 consoleEvents = LogConsumer::AllErrorTypes |
   //                          LogConsumer::LuaLevelGenerator | LogConsumer::LuaBotMessage |
   //                          LogConsumer::ConsoleMsg;
   //StdoutLogConsumer StdoutLog;
   //StdoutLog.setMsgTypes(consoleEvents);

   testing::InitGoogleTest(&argc, argv);

   if(!checkResources())
   {
      printf("FAILED: Invalid test environment! Are you sure you copied everything from 'resources/' into 'exe/'?\n");
      testing::internal::posix::Abort();
   }


   DisplayManager::initialize();
   int returnvalue = RUN_ALL_TESTS();
   FontManager::cleanup();
   DisplayManager::cleanup();
   return returnvalue;
}

