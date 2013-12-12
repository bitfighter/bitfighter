//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Bitfighter Tests

#define BF_TEST

#include "DisplayManager.h"
#include "FontManager.h"

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
   testing::InitGoogleTest(&argc, argv);
   DisplayManager::initialize();
   int returnvalue = RUN_ALL_TESTS();
   FontManager::cleanup();
   DisplayManager::cleanup();
   return returnvalue;
}

