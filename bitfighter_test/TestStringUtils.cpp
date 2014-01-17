//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "stringUtils.h"
#include "gtest/gtest.h"

#ifdef TNL_OS_WIN32 
#  include <windows.h>        // For ARRAYSIZE def
#endif

namespace Zap {

TEST(StringUtilsTest, stringContainsAllTheSameCharacter)
{
   EXPECT_TRUE(stringContainsAllTheSameCharacter("A"));
   EXPECT_TRUE(stringContainsAllTheSameCharacter("AA"));
   EXPECT_TRUE(stringContainsAllTheSameCharacter("AAA"));
   EXPECT_TRUE(stringContainsAllTheSameCharacter(""));

   EXPECT_FALSE(stringContainsAllTheSameCharacter("Aa"));
   EXPECT_FALSE(stringContainsAllTheSameCharacter("AB"));
}


TEST(StringUtilsTest, isHex)
{
   EXPECT_TRUE(isHex("0"));
   EXPECT_TRUE(isHex("9"));
   EXPECT_TRUE(isHex("A"));
   EXPECT_TRUE(isHex("F"));
   EXPECT_TRUE(isHex("a"));
   EXPECT_TRUE(isHex("f"));
   EXPECT_TRUE(isHex("deadbeef"));
   EXPECT_TRUE(isHex("DeaDBeeF"));
   EXPECT_TRUE(isHex("8675309"));

   EXPECT_FALSE(isHex("g"));
   EXPECT_FALSE(isHex("deadbeet"));
   EXPECT_FALSE(isHex("G"));
   EXPECT_FALSE(isHex("44/"));      // '/' comes just before '0' 
   EXPECT_FALSE(isHex("12:345"));   // ':' comes just after '9'
   EXPECT_FALSE(isHex("@bcdef"));   // '@' comes just before 'A'
   EXPECT_FALSE(isHex("c01`"));     // '`' comes just before 'a'
}


TEST(StringUtilsTest, listToString)
{
   Vector<string> empty;
   EXPECT_EQ("", listToString(empty, ", "));

   string wordArry[] = {"one", "two", "three"};
   Vector<string> words(wordArry, ARRAYSIZE(wordArry));
   EXPECT_EQ("one;two;three", listToString(words, ";"));
   EXPECT_EQ("one, two, three", listToString(words, ", "));
}

};
