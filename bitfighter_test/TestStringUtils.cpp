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


TEST(StringUtilsTest, isPositiveInteger)
{
   EXPECT_TRUE(isPositiveInteger("0"));
   EXPECT_TRUE(isPositiveInteger("123"));
   EXPECT_TRUE(isPositiveInteger("000123"));

   EXPECT_FALSE(isPositiveInteger("-1"));
   EXPECT_FALSE(isPositiveInteger("1.2"));
   EXPECT_FALSE(isPositiveInteger("abc"));
   EXPECT_FALSE(isPositiveInteger("12a"));
   EXPECT_FALSE(isPositiveInteger(""));
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


TEST(StringUtilsTest, chopComment)
{
   EXPECT_EQ("This is a ",        chopComment("This is a #comment"));
   EXPECT_EQ("",                  chopComment("# This is a comment"));
   EXPECT_EQ("This is a comment", chopComment("This is a comment#"));
   EXPECT_EQ("This is a comment", chopComment("This is a comment"));
   EXPECT_EQ("",                  chopComment("#"));

}


TEST(StringUtilsTest, itos)
{
   EXPECT_EQ("0", itos((S32)0));
   EXPECT_EQ("123", itos((S32)123));
   EXPECT_EQ("-123", itos((S32)-123));

   EXPECT_EQ("0", itos((U32)0));
   EXPECT_EQ("123", itos((U32)123));
   EXPECT_EQ("4294967295", itos((U32)0xFFFFFFFF));

   EXPECT_EQ("123456789012345", itos((U64)123456789012345ULL));
   EXPECT_EQ("18446744073709551615", itos((U64)0xFFFFFFFFFFFFFFFFULL));

   EXPECT_EQ("-123456789012345", itos((S64)-123456789012345LL));
   EXPECT_EQ("9223372036854775807", itos((S64)0x7FFFFFFFFFFFFFFFLL));
}


TEST(StringUtilsTest, stripZeros)
{
   EXPECT_EQ("1.1", stripZeros("1.100"));
   EXPECT_EQ("1", stripZeros("1.000"));
   EXPECT_EQ("0", stripZeros("0"));
   EXPECT_EQ("", stripZeros(".000"));
}


TEST(StringUtilsTest, extractDirectory)
{
   EXPECT_EQ("path/to", extractDirectory("path/to/file.txt"));
   EXPECT_EQ("path\\to", extractDirectory("path\\to\\file.txt"));
   EXPECT_EQ("", extractDirectory("file.txt"));
}


TEST(StringUtilsTest, sanitizeForSqlLeavesSafeStrings)
{
   EXPECT_EQ("plain", sanitizeForSql("plain"));
   EXPECT_EQ("", sanitizeForSql(""));
}


TEST(StringUtilsTest, sanitizeForSqlEscapesQuotes)
{
   EXPECT_EQ("O''Brien", sanitizeForSql("O'Brien"));
   EXPECT_EQ("''quoted''", sanitizeForSql("'quoted'"));
}


TEST(StringUtilsTest, sanitizeForSqlEscapesBackslashesAndMixed)
{
   EXPECT_EQ(R"(path\\to)", sanitizeForSql(R"(path\to)"));
   EXPECT_EQ(R"(C:\\temp\\O''Brien)", sanitizeForSql(R"(C:\temp\O'Brien)"));
}


};
