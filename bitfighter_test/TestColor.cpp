//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Colors.h"

#include "gtest/gtest.h"

namespace Zap
{

TEST(ColorTest, IniParsing)
{
   // Valid inputs
   EXPECT_EQ(Colors::red, Color::iniValToColor("#ff0000"));
   EXPECT_EQ(Colors::red, Color::iniValToColor("1 0 0"));

   EXPECT_EQ(Colors::yellow, Color::iniValToColor("#ffff00"));
   EXPECT_EQ(Colors::yellow, Color::iniValToColor("1 1 0"));

   // Invalid inputs
   EXPECT_EQ(Colors::black, Color::iniValToColor("hello charlie!"));
   EXPECT_EQ(Colors::black, Color::iniValToColor("1 0"));

   // Quasi invalid inputs
   EXPECT_EQ(Colors::black, Color::iniValToColor("2 1 0"));
   EXPECT_EQ(Colors::black, Color::iniValToColor("#ff00ff0"));
   EXPECT_EQ(Colors::black, Color::iniValToColor("#ff00f"));
   EXPECT_EQ(Colors::black, Color::iniValToColor("0 1"));
   EXPECT_EQ(Colors::black, Color::iniValToColor("0 1 0 1"));


}
	
};
