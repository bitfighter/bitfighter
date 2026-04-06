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


TEST(StringUtilsTest, extractExtension)
{
   EXPECT_EQ("txt", extractExtension("file.txt"));
   EXPECT_EQ("txt", extractExtension("path/to/file.txt"));
   EXPECT_EQ("gz", extractExtension("file.tar.gz"));
   EXPECT_EQ("", extractExtension("file_with_no_extension"));
   EXPECT_EQ("", extractExtension("path.to/file"));
   EXPECT_EQ("", extractExtension("file"));
}


TEST(StringUtilsTest, isInteger)
{
   EXPECT_TRUE(isPositiveInteger("123"));
   EXPECT_TRUE(isPositiveInteger("0"));
   EXPECT_FALSE(isPositiveInteger("12a3"));
   EXPECT_FALSE(isPositiveInteger(""));
   EXPECT_FALSE(isPositiveInteger(NULL));
   EXPECT_FALSE(isPositiveInteger("-123"));
   EXPECT_FALSE(isPositiveInteger(" "));
}


TEST(StringUtilsTest, stripExtension)
{
   EXPECT_EQ("file", stripExtension("file.txt"));
   EXPECT_EQ("path/to/file", stripExtension("path/to/file.txt"));
   EXPECT_EQ("file.tar", stripExtension("file.tar.gz"));
   EXPECT_EQ("file_with_no_extension", stripExtension("file_with_no_extension"));
   EXPECT_EQ("path.to/file", stripExtension("path.to/file"));
   EXPECT_EQ("file", stripExtension("file"));
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


TEST(StringUtilsTest, getFilesFromFolder)
{
   // Create a temporary directory for testing
   string testDir = "test_folder_getFilesFromFolder";
   makeSureFolderExists(testDir);

   // Create some files
   writeFile(joindir(testDir, "test1.level"), "content");
   writeFile(joindir(testDir, "test2.TXT"), "content");
   writeFile(joindir(testDir, "test3.other"), "content");

   Vector<string> files;
   string extensions[] = {"level", "txt"};   // Expected usage, lc extensions

   // Test filtering by extensions
   EXPECT_TRUE(getFilesFromFolder(testDir, files, extensions, 2));
   EXPECT_EQ(2, files.size());

   Vector<string> files2;
   string extensions2[] = {"LEVEL", "TxT"};         // Try again, with mixed case extensions

   // Test filtering by extensions
   EXPECT_TRUE(getFilesFromFolder(testDir, files2, extensions2, 2));
   EXPECT_EQ(2, files2.size());


   bool foundLevel = false;
   bool foundTxt = false;
   for(S32 i = 0; i < files.size(); i++)
   {
      if(files[i] == "test1.level") foundLevel = true;
      if(files[i] == "test2.TXT") foundTxt = true;
   }
   EXPECT_TRUE(foundLevel);
   EXPECT_TRUE(foundTxt);

   // Clean up (best effort)
   remove(joindir(testDir, "test1.level").c_str());
   remove(joindir(testDir, "test2.TXT").c_str());
   remove(joindir(testDir, "test3.other").c_str());
   remove(testDir.c_str());
}


};
