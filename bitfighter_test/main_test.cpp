//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Bitfighter Tests - main entry point
//
// NOTE: We link gtest (not gtest_main) because on Windows SDL2main.lib
// requires a function named SDL_main (via #define main SDL_main in SDL.h).
// gtest_main's built-in main() is compiled without SDL.h and thus isn't
// renamed, causing an unresolved external.  By providing our own main() in a
// file that includes SDL.h (transitively through BitfighterTestEnvironment.h),
// the SDL_main rename is applied correctly.

#include "BitfighterTestEnvironment.h"
#include <gtest/gtest.h>


int main(int argc, char **argv)
{
   testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}