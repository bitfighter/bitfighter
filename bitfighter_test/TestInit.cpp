#include "BitfighterTestEnvironment.h"

// Bitfighter Tests

#define BF_TEST

#include "DisplayManager.h"
#include "FontManager.h"
#include "Renderer.h"
#include "TestRenderer.h"

#include "stringUtils.h"


#include <gtest/gtest.h>

#ifndef ARRAYSIZE
#  define ARRAYSIZE(a) sizeof(a)/sizeof(a[0])
#endif


namespace Zap
{
    void exitToOs(S32 errcode) { TNLAssert(false, "Should never be called!"); }
    void shutdownBitfighter()  { TNLAssert(false, "Should never be called!"); };
}

/**
 * Don't put tests in here! Use one file per class. Your tests go in the file
 * corresponding to the class which is the main subject of your tests.
 */



::testing::Environment* const env =
    ::testing::AddGlobalTestEnvironment(new BitfighterTestEnvironment());
