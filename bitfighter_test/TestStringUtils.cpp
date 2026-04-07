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
   EXPECT_EQ("path/to", extractDirectory("path/to/"));
   EXPECT_EQ("path/to", extractDirectory("path/to/this"));
}


TEST(StringUtilsTest, extractFilename)
{
   EXPECT_EQ("file.txt", extractFilename("path/to/file.txt"));
   EXPECT_EQ("file.txt", extractFilename("path\\to\\file.txt"));
   EXPECT_EQ("file.txt", extractFilename("file.txt"));
   EXPECT_EQ("", extractFilename("path/to/"));
   EXPECT_EQ("this", extractFilename("path/to/this"));
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
   EXPECT_EQ("/path.with.dots/file", stripExtension("/path.with.dots/file"));
}


TEST(StringUtilsTest, ftos)
{
   EXPECT_EQ("1.23", ftos(1.23456f, 2));
   EXPECT_EQ("1.235", ftos(1.23456f, 3));
   EXPECT_EQ("1", ftos(1.0f, 2));
   EXPECT_EQ("1.23456", ftos(1.23456f));
}


TEST(StringUtilsTest, stof)
{
   EXPECT_DOUBLE_EQ(1.23, stof("1.23"));
   EXPECT_DOUBLE_EQ(-1.23, stof("-1.23"));
   EXPECT_DOUBLE_EQ(0.0, stof("abc"));
}


TEST(StringUtilsTest, replaceString)
{
   EXPECT_EQ("bar bar bar", replaceString("foo foo foo", "foo", "bar"));
   EXPECT_EQ("y x y x", replaceString("x x", "x", "y x")); // Infinite loop test check
   EXPECT_EQ("y", replaceString("x", "x", "y"));
   EXPECT_EQ("banana", replaceString("banana", "x", "y"));
}


TEST(StringUtilsTest, lcase_ucase)
{
   EXPECT_EQ("abc", lcase("ABC"));
   EXPECT_EQ("abc", lcase("abc"));
   EXPECT_EQ("123", lcase("123"));

   EXPECT_EQ("ABC", ucase("abc"));
   EXPECT_EQ("ABC", ucase("ABC"));
   EXPECT_EQ("123", ucase("123"));
}


TEST(StringUtilsTest, alphaNumericChecks)
{
   EXPECT_TRUE(isAlpha('a'));
   EXPECT_TRUE(isAlpha('Z'));
   EXPECT_FALSE(isAlpha('1'));
   EXPECT_FALSE(isAlpha(' '));

   EXPECT_TRUE(isDigit('0'));
   EXPECT_TRUE(isDigit('9'));
   EXPECT_FALSE(isDigit('a'));

   EXPECT_TRUE(isAlNum('a'));
   EXPECT_TRUE(isAlNum('1'));
   EXPECT_FALSE(isAlNum('!'));
}


TEST(StringUtilsTest, ctos)
{
   EXPECT_EQ("a", ctos('a'));
   EXPECT_EQ("", ctos('\0'));
}


TEST(StringUtilsTest, toString)
{
   EXPECT_EQ("test", toString(string("test")));
   EXPECT_EQ("123", toString((S32)123));
   EXPECT_EQ("-123", toString((S32)-123));
   EXPECT_EQ("Yes", toString(Yes));
   EXPECT_EQ("No", toString(No));
   EXPECT_EQ("Relative", toString(Relative));
   EXPECT_EQ("Absolute", toString(Absolute));
   EXPECT_EQ("Window", toString(DISPLAY_MODE_WINDOWED));
   EXPECT_EQ("Fullscreen", toString(DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED));
   EXPECT_EQ("RGBHEX", toString(ColorEntryModeHex));
}


TEST(StringUtilsTest, parseString)
{
   Vector<string> result = parseString("one two \"three four\" five");
   ASSERT_EQ(4, result.size());
   EXPECT_EQ("one", result[0]);
   EXPECT_EQ("two", result[1]);
   EXPECT_EQ("three four", result[2]);
   EXPECT_EQ("five", result[3]);

   Vector<string> result2;
   parseString("one;two;three", result2, ';');
   ASSERT_EQ(3, result2.size());
   EXPECT_EQ("one", result2[0]);
   EXPECT_EQ("two", result2[1]);
   EXPECT_EQ("three", result2[2]);
}


TEST(StringUtilsTest, parseStringAndStripLeadingSlash)
{
   Vector<string> result = parseStringAndStripLeadingSlash("/cmd arg1 arg2");
   ASSERT_EQ(3, result.size());
   EXPECT_EQ("cmd", result[0]);
   EXPECT_EQ("arg1", result[1]);
   EXPECT_EQ("arg2", result[2]);
}


TEST(StringUtilsTest, findPointerOfArg)
{
   const char *msg = "one two three four";
   EXPECT_STREQ("one two three four", findPointerOfArg(msg, 0));
   EXPECT_STREQ("two three four", findPointerOfArg(msg, 1));
   EXPECT_STREQ("three four", findPointerOfArg(msg, 2));
   EXPECT_STREQ("four", findPointerOfArg(msg, 3));
   EXPECT_STREQ("", findPointerOfArg(msg, 4));
   EXPECT_STREQ("", findPointerOfArg(msg, 40));
   EXPECT_STREQ("", findPointerOfArg(msg, -1));
}


TEST(StringUtilsTest, concatenate)
{
   string wordArry[] = {"one", "two", "three"};
   Vector<string> words(wordArry, ARRAYSIZE(wordArry));
   EXPECT_EQ("one two three", concatenate(words));
   EXPECT_EQ("two three", concatenate(words, 1));
}


TEST(StringUtilsTest, caseInsensitiveStringCompare)
{
   EXPECT_TRUE(caseInsensitiveStringCompare("abc", "ABC"));
   EXPECT_TRUE(caseInsensitiveStringCompare("ABC", "abc"));
   EXPECT_TRUE(caseInsensitiveStringCompare("AbC", "aBc"));
   EXPECT_FALSE(caseInsensitiveStringCompare("abc", "abcd"));
   EXPECT_FALSE(caseInsensitiveStringCompare("abc", "abd"));
}


TEST(StringUtilsTest, countCharInString)
{
   EXPECT_EQ(3, countCharInString("banana", 'a'));
   EXPECT_EQ(0, countCharInString("banana", 'z'));
   EXPECT_EQ(1, countCharInString("banana", 'b'));
}


TEST(StringUtilsTest, controlCharacters)
{
   EXPECT_TRUE(isControlCharacter(0x01));
   EXPECT_TRUE(isControlCharacter(0x1F));
   EXPECT_FALSE(isControlCharacter(' '));
   EXPECT_FALSE(isControlCharacter('a'));

   EXPECT_TRUE(containsControlCharacter("abc\x01xyz"));
   EXPECT_FALSE(containsControlCharacter("abc xyz"));
}


TEST(StringUtilsTest, sanitizeForJson)
{
   EXPECT_EQ("plain", sanitizeForJson("plain"));
   EXPECT_EQ("\\\"quoted\\\"", sanitizeForJson("\"quoted\""));
   EXPECT_EQ("\\\\backslash\\\\", sanitizeForJson("\\backslash\\"));
   EXPECT_EQ("\\n\\r\\t", sanitizeForJson("\n\r\t"));
   EXPECT_EQ("&amp;&lt;&gt;", sanitizeForJson("&<>"));
}


TEST(StringUtilsTest, trim)
{
   EXPECT_EQ("abc", trim("  abc  "));
   EXPECT_EQ("abc", trim("\n\t abc \r\n"));
   EXPECT_EQ("abc  ", trim_left("  abc  "));
   EXPECT_EQ("  abc", trim_right("  abc  "));

   string s = "  abc  ";
   trim_in_place(s);
   EXPECT_EQ("abc", s);

   s = "  abc  ";
   trim_left_in_place(s);
   EXPECT_EQ("abc  ", s);

   s = "  abc  ";
   trim_right_in_place(s);
   EXPECT_EQ("  abc", s);
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
   string extensions[] = {"LeVeL", "tXt"};

   // Test filtering by extensions (case-insensitive)
   EXPECT_TRUE(getFilesFromFolder(testDir, files, extensions, 2));
   EXPECT_EQ(2, files.size());

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


TEST(StringUtilsTest, fileUtils)
{
   string testFile = "test_file.txt";
   string content = "Hello World!";

   ASSERT_TRUE(writeFile(testFile, content));
   EXPECT_TRUE(fileExists(testFile));
   EXPECT_EQ(content, readFile(testFile));

   string appendContent = " Append";
   ASSERT_TRUE(writeFile(testFile, appendContent, true));
   EXPECT_EQ(content + appendContent, readFile(testFile));

   string destFile = "test_file_copy.txt";
   EXPECT_TRUE(copyFile(testFile, destFile));
   EXPECT_TRUE(fileExists(destFile));
   EXPECT_EQ(content + appendContent, readFile(destFile));

   string testDir = "test_dir_utils";
   EXPECT_TRUE(makeSureFolderExists(testDir));
   EXPECT_TRUE(fileExists(testDir));

   EXPECT_TRUE(copyFileToDir(testFile, testDir));
   EXPECT_TRUE(fileExists(joindir(testDir, testFile)));

   remove(testFile.c_str());
   remove(destFile.c_str());
   remove(joindir(testDir, testFile).c_str());
   remove(testDir.c_str());
}


TEST(StringUtilsTest, joinDir)
{
   EXPECT_EQ("path/file", joindir("path", "file"));
   EXPECT_EQ("path/file", joindir("path/", "file"));
   EXPECT_EQ("/path/file", joindir("/path/", "file"));
   EXPECT_EQ("path/file", joindir("path/", "/file"));
   EXPECT_EQ("file", joindir("", "file"));

   EXPECT_EQ("path/file", strictjoindir("path", "file"));
   EXPECT_EQ("path/file", strictjoindir("path/", "file"));
   EXPECT_EQ("path/sub/file", strictjoindir("path", "sub", "file"));
}


TEST(StringUtilsTest, safeFilename)
{
   EXPECT_TRUE(safeFilename("file.txt"));
   EXPECT_FALSE(safeFilename("path/file.txt"));
   EXPECT_FALSE(safeFilename("path\\file.txt"));
}


TEST(StringUtilsTest, getExecutableDir)
{
   string dir = getExecutableDir();
   EXPECT_FALSE(dir.empty());
}


TEST(StringUtilsTest, makeFilenameFromString)
{
   EXPECT_EQ("My_Level", makeFilenameFromString("My Level"));
   EXPECT_EQ("My_Level", makeFilenameFromString("My.Level"));
   EXPECT_EQ("My.Level", makeFilenameFromString("My.Level", true));
}


TEST(StringUtilsTest, writeLevelString)
{
   EXPECT_EQ("Normal", writeLevelString("Normal"));
   EXPECT_EQ("\"String with space\"", writeLevelString("String with space"));
   EXPECT_EQ("\"String with #\"", writeLevelString("String with #"));
   EXPECT_EQ("\"String with \"\"quotes\"\"\"", writeLevelString("String with \"quotes\""));
}


TEST(StringUtilsTest, charTypeChecks)
{
   EXPECT_TRUE(isPrintable('a'));
   EXPECT_FALSE(isPrintable('\x01'));

   EXPECT_TRUE(isHex('0'));
   EXPECT_TRUE(isHex('f'));
   EXPECT_TRUE(isHex('A'));
   EXPECT_FALSE(isHex('g'));
}


// These are comparitors for the actual sort function; they do not do sorting themselves.
TEST(StringUtilsTest, sorting)
{
   EXPECT_TRUE(alphaSort("a", "b"));
   EXPECT_TRUE(alphaSort("A", "b"));
   EXPECT_FALSE(alphaSort("b", "a"));

   EXPECT_TRUE(alphaNumberSort("2", "10"));
   EXPECT_FALSE(alphaNumberSort("10", "a"));
   EXPECT_TRUE(alphaNumberSort("1", "2"));
}


TEST(StringUtilsTest, s_fprintf)
{
   string testFile = "test_fprintf.txt";
   FILE *f = fopen(testFile.c_str(), "w");
   ASSERT_TRUE(f != NULL);

   s_fprintf(f, "%d %s", 123, "test");
   fclose(f);

   EXPECT_EQ("123 test", readFile(testFile));
   remove(testFile.c_str());
}


};
