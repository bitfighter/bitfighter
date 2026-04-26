//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "stringUtils.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <vector>

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
   EXPECT_FALSE(isHex(""));
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
   EXPECT_EQ("This is a ",                  chopComment("This is a #comment"));
   EXPECT_EQ("",                            chopComment("# This is a comment"));
   EXPECT_EQ("This is a comment",           chopComment("This is a comment#"));
   EXPECT_EQ("This is a comment",           chopComment("This is a comment"));
   EXPECT_EQ("",                            chopComment("#"));
   EXPECT_EQ("LevelName \"My #1 Level\" ",  chopComment("LevelName \"My #1 Level\" # comment"));
   EXPECT_EQ("LevelName \"My #1 Level\"",   chopComment("LevelName \"My #1 Level\""));
   EXPECT_EQ("LevelName \"\"\"#\"\"\"",     chopComment("LevelName \"\"\"#\"\"\""));
   EXPECT_EQ("No hash here",                chopComment("No hash here"));
   EXPECT_EQ("Quotes \"but no hash\"",      chopComment("Quotes \"but no hash\""));
   EXPECT_EQ("\"#HashAtStart\"",            chopComment("\"#HashAtStart\""));
   EXPECT_EQ("\"HashAtEnd#\"",              chopComment("\"HashAtEnd#\""));
   EXPECT_EQ("Mix \"#1\" and \"#2\"",       chopComment("Mix \"#1\" and \"#2\""));
   EXPECT_EQ("Mix \"#1\" and \"#2\" ",      chopComment("Mix \"#1\" and \"#2\" # then comment"));
   EXPECT_EQ("\"\"\"#\"\"\" \"\"\"#\"\"\"", chopComment("\"\"\"#\"\"\" \"\"\"#\"\"\""));
   EXPECT_EQ("Escaped \"\"quote\"\" ",      chopComment("Escaped \"\"quote\"\" # comment"));
}


TEST(StringUtilsTest, chopCommentWithQuotes)
{
   EXPECT_EQ("\"#not_a_comment\" ", chopComment("\"#not_a_comment\" #comment"));
   EXPECT_EQ("No comment here \"#\"", chopComment("No comment here \"#\""));
   EXPECT_EQ("\"A \"\"#\"\" B\" ", chopComment("\"A \"\"#\"\" B\" #comment"));
   EXPECT_EQ("\"quoted # hashtag\"", chopComment("\"quoted # hashtag\""));
   EXPECT_EQ("\"quoted # hashtag\" ", chopComment("\"quoted # hashtag\" #comment"));
   EXPECT_EQ("\"\"", chopComment("\"\"#\"\"")); // "" is an empty quoted string, then # starts a comment
   EXPECT_EQ("\"escaped \"\" quote\" ", chopComment("\"escaped \"\" quote\" #comment"));
   EXPECT_EQ("mixed \"#\" and ", chopComment("mixed \"#\" and #comment"));
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
#if !defined(TNL_OS_WIN32)
   EXPECT_EQ("/", extractDirectory("/file.txt"));
#endif
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

   // Hidden files or dots immediately following a slash should not be considered as extensions
   EXPECT_EQ("", extractExtension(".hidden"));
   EXPECT_EQ("", extractExtension("path/.hidden"));
   EXPECT_EQ("", extractExtension("path\\.hidden"));
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

   // Bug fixes: filenames starting with a dot or where the only dot is immediately after a slash
   EXPECT_EQ(".hidden", stripExtension(".hidden"));
   EXPECT_EQ("path/.hidden", stripExtension("path/.hidden"));
   EXPECT_EQ("path\\.hidden", stripExtension("path\\.hidden"));
}


TEST(StringUtilsTest, ftos)
{
   EXPECT_EQ("1.23", ftos(1.23456f, 2));
   EXPECT_EQ("1.235", ftos(1.23456f, 3));
   EXPECT_EQ("1", ftos(1.0f, 2));
   EXPECT_EQ("1.23456", ftos(1.23456f));
}


TEST(StringUtilsTest, ftosBugReproduction)
{
   // Large digits value - could cause buffer overflow if not handled
   // itos(200) is "200", format string becomes "%2.200f"
   // outString has size 100.
   string result = ftos(1.23456f, 200);
   EXPECT_TRUE(result.length() < 100);

   // Negative digits value
   // itos(-1) is "-1", format string becomes "%2.-1f"
   EXPECT_EQ("1.23456", ftos(1.23456f, -1));
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


TEST(StringUtilsTest, replaceStringEmptyFrom)
{
   // These should return the original string and not hang
   EXPECT_EQ("test", replaceString("test", "", "replacement"));
   EXPECT_EQ("test", replaceString((const char *)"test", "", "replacement"));
}


TEST(StringUtilsTest, replaceStringNullChecks)
{
   // NULL input string should return empty string and not crash
   EXPECT_EQ("", replaceString((const char *)NULL, "find", "replace"));

   // NULL replacement string should treat it as an empty replacement
   EXPECT_EQ("f", replaceString("foo", "o", (const char *)NULL));
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


TEST(StringUtilsTest, safeCctypeWrappers)
{
   // toLower / toUpper
   EXPECT_EQ('a', toLower('A'));
   EXPECT_EQ('a', toLower('a'));
   EXPECT_EQ('A', toUpper('a'));
   EXPECT_EQ('A', toUpper('A'));
   EXPECT_EQ((char)0xFF, toLower((char)0xFF));
   EXPECT_EQ((char)0xFF, toUpper((char)0xFF));

   // isSpace
   EXPECT_TRUE(isSpace(' '));
   EXPECT_TRUE(isSpace('\n'));
   EXPECT_FALSE(isSpace('a'));
   EXPECT_FALSE(isSpace((char)0xFF));

   // isAlpha / isDigit / isAlNum
   EXPECT_TRUE(isAlpha('a'));
   EXPECT_FALSE(isAlpha('1'));
   EXPECT_TRUE(isDigit('1'));
   EXPECT_FALSE(isDigit('a'));
   EXPECT_TRUE(isAlNum('a'));
   EXPECT_TRUE(isAlNum('1'));
   EXPECT_FALSE(isAlNum(' '));
   EXPECT_FALSE(isAlNum((char)0xFF));

   // isPrint
   EXPECT_TRUE(isPrintable('a'));
   EXPECT_FALSE(isPrintable('\1'));

   // isPunct
   EXPECT_TRUE(isPunct('.'));
   EXPECT_FALSE(isPunct('a'));

   // isHex
   EXPECT_TRUE(TNL::isHex('a'));
   EXPECT_TRUE(TNL::isHex('A'));
   EXPECT_TRUE(TNL::isHex('0'));
   EXPECT_FALSE(TNL::isHex('g'));
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
   Vector<string> words;

   // NULL input - currently would crash
   parseString((const char*)NULL, words, ',');
   EXPECT_TRUE(words.empty());

   // Empty string input
   parseString("", words, ',');
   EXPECT_TRUE(words.empty());

   Vector<string> result = parseString("one two \"three four\" five");
   ASSERT_EQ(4, result.size());
   EXPECT_EQ("one", result[0]);
   EXPECT_EQ("two", result[1]);
   EXPECT_EQ("three four", result[2]);
   EXPECT_EQ("five", result[3]);

   // New tests for spaces within quotes
   result = parseString("\" space at start\"");
   ASSERT_EQ(1, result.size());
   EXPECT_EQ(" space at start", result[0]);

   result = parseString("\"space at end \"");
   ASSERT_EQ(1, result.size());
   EXPECT_EQ("space at end ", result[0]);

   result = parseString("\" \"");
   ASSERT_EQ(1, result.size());
   EXPECT_EQ(" ", result[0]);

   result = parseString("\"\"");
   ASSERT_EQ(1, result.size());
   EXPECT_EQ("", result[0]);

   // Test escaped quotes
   result = parseString("\"a \"\"quoted\"\" word\"");
   ASSERT_EQ(1, result.size());
   EXPECT_EQ("a \"quoted\" word", result[0]);

   result = parseString("mixed \"quoted space\" and \"\"\"escaped quote\"\"\"");
   ASSERT_EQ(4, result.size());
   EXPECT_EQ("mixed", result[0]);
   EXPECT_EQ("quoted space", result[1]);
   EXPECT_EQ("and", result[2]);
   EXPECT_EQ("\"escaped quote\"", result[3]);

   Vector<string> result2;
   parseString("one;two;three", result2, ';');
   ASSERT_EQ(3, result2.size());
   EXPECT_EQ("one", result2[0]);
   EXPECT_EQ("two", result2[1]);
   EXPECT_EQ("three", result2[2]);
}


TEST(StringUtilsTest, parseStringTruncation)
{
   // Create a word longer than 126 characters (original buffer size was 126)
   string longWord(130, 'a');
   string input = longWord + ";short";

   Vector<string> words;
   parseString(input.c_str(), words, ';');

   ASSERT_EQ(2, words.size());
   EXPECT_EQ(longWord, words[0]);
   EXPECT_EQ("short", words[1]);
}


TEST(StringUtilsTest, parseStringAndStripLeadingSlash)
{
   Vector<string> result = parseStringAndStripLeadingSlash("/cmd arg1 arg2");
   ASSERT_EQ(3, result.size());
   EXPECT_EQ("cmd", result[0]);
   EXPECT_EQ("arg1", result[1]);
   EXPECT_EQ("arg2", result[2]);

   // Bug fix: NULL input
   result = parseStringAndStripLeadingSlash(NULL);
   EXPECT_TRUE(result.empty());
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

   // Bug fix: NULL input
   EXPECT_STREQ("", findPointerOfArg(NULL, 0));

   // New tests for leading and multiple spaces
   EXPECT_STREQ("word1 word2", findPointerOfArg("  word1 word2", 0));
   EXPECT_STREQ("word2", findPointerOfArg("  word1 word2", 1));
   EXPECT_STREQ("word2", findPointerOfArg("word1  word2", 1));
   EXPECT_STREQ("word2", findPointerOfArg("word1\tword2", 1));
   EXPECT_STREQ("word2   ", findPointerOfArg("   word1   word2   ", 1));
}


TEST(StringUtilsTest, concatenate)
{
   string wordArry[] = {"one", "two", "three"};
   Vector<string> words(wordArry, ARRAYSIZE(wordArry));
   EXPECT_EQ("one two three", concatenate(words));
   EXPECT_EQ("two three", concatenate(words, 1));

   // Bug fix: negative startingWith
   EXPECT_EQ("one two three", concatenate(words, -1));
}


TEST(StringUtilsTest, caseInsensitiveStringCompare)
{
   EXPECT_TRUE(caseInsensitiveStringCompare("abc", "ABC"));
   EXPECT_TRUE(caseInsensitiveStringCompare("ABC", "abc"));
   EXPECT_TRUE(caseInsensitiveStringCompare("AbC", "aBc"));
   EXPECT_FALSE(caseInsensitiveStringCompare("abc", "abcd"));
   EXPECT_FALSE(caseInsensitiveStringCompare("abc", "abd"));
}


TEST(StringUtilsTest, caseInsensitiveStringCompareNonASCII)
{
   // Use characters with the high bit set to ensure we handle them correctly.
   // On systems where char is signed, passing a char with the high bit set
   // to tolower() without a cast to unsigned char is undefined behavior.
   string s1 = "\xFF";
   string s2 = "\xFF";
   EXPECT_TRUE(caseInsensitiveStringCompare(s1, s2));

   s1 = "A\x80";
   s2 = "a\x80";
   EXPECT_TRUE(caseInsensitiveStringCompare(s1, s2));
}


TEST(StringUtilsTest, stricmpNonASCII)
{
   EXPECT_EQ(0, stricmp("\xFF", "\xFF"));
   EXPECT_EQ(0, stricmp("A\x80", "a\x80"));
   EXPECT_NE(0, stricmp("\xFF", "\xFE"));
}


TEST(StringUtilsTest, strnicmpNonASCII)
{
   EXPECT_EQ(0, strnicmp("\xFF", "\xFF", 1));
   EXPECT_EQ(0, strnicmp("A\x80", "a\x80", 2));
   EXPECT_NE(0, strnicmp("\xFF", "\xFE", 1));
   EXPECT_EQ(0, strnicmp("abc\xFF", "ABC\xFF", 3));
}


TEST(StringUtilsTest, countCharInString)
{
   EXPECT_EQ(3, countCharInString("banana", 'a'));
   EXPECT_EQ(0, countCharInString("banana", 'z'));
   EXPECT_EQ(1, countCharInString("banana", 'b'));

   // Test with embedded null characters
   string s = "abc";
   s += '\0';
   s += "abc";
   // s is "abc\0abc", length is 7
   EXPECT_EQ(2, countCharInString(s, 'a'));
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
   EXPECT_EQ("", sanitizeForJson(NULL));

   // Control characters
   EXPECT_EQ("a\\u0001b", sanitizeForJson("a\x01""b"));
   EXPECT_EQ("\\u001F", sanitizeForJson("\x1F"));

   // Verify all control characters (1-31)
   for(int i = 1; i <= 31; ++i)
   {
      char input[2] = {(char)i, 0};
      string result = sanitizeForJson(input);

      // Some control characters have special short escapes in sanitizeForJson
      if(i == '\b')      EXPECT_EQ("\\b", result);
      else if(i == '\f') EXPECT_EQ("\\f", result);
      else if(i == '\n') EXPECT_EQ("\\n", result);
      else if(i == '\r') EXPECT_EQ("\\r", result);
      else if(i == '\t') EXPECT_EQ("\\t", result);
      else
      {
         char expected[8];
         sprintf(expected, "\\u00%.2X", i);
         EXPECT_EQ(expected, result) << "Failed for control character " << i;
      }
   }
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
#if defined(TNL_OS_WIN32)
   EXPECT_EQ("path\\file", joindir("path", "file"));
   EXPECT_EQ("path\\file", joindir("path\\", "file"));
   EXPECT_EQ("\\path\\file", joindir("\\path\\", "file"));
   EXPECT_EQ("file", joindir("", "file"));

   EXPECT_EQ("path\\file", strictjoindir("path", "file"));
   EXPECT_EQ("path\\file", strictjoindir("path\\", "file"));
   EXPECT_EQ("path\\sub\\file", strictjoindir("path", "sub", "file"));
#else
   EXPECT_EQ("path/file", joindir("path", "file"));
   EXPECT_EQ("path/file", joindir("path/", "file"));
   EXPECT_EQ("/path/file", joindir("/path/", "file"));
   EXPECT_EQ("file", joindir("", "file"));

   EXPECT_EQ("path/file", strictjoindir("path", "file"));
   EXPECT_EQ("path/file", strictjoindir("path/", "file"));
   EXPECT_EQ("path/sub/file", strictjoindir("path", "sub", "file"));
#endif
}


TEST(StringUtilsTest, safeFilename)
{
   EXPECT_TRUE(safeFilename("file.txt"));
   EXPECT_FALSE(safeFilename("path/file.txt"));
   EXPECT_FALSE(safeFilename("path\\file.txt"));

   // Bug fix: NULL input
   EXPECT_FALSE(safeFilename(NULL));
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

   // Spaces and other special characters are replaced with underscores
   EXPECT_EQ("Hello_World_", makeFilenameFromString("Hello World!"));
   EXPECT_EQ("a_b_c", makeFilenameFromString("a b c"));

   // No dot in name — allowLastDot has no effect
   EXPECT_EQ("MyLevel", makeFilenameFromString("MyLevel", true));
   EXPECT_EQ("MyLevel", makeFilenameFromString("MyLevel", false));

   // Multiple dots — only the last dot is preserved with allowLastDot
   EXPECT_EQ("my_cool_level", makeFilenameFromString("my.cool.level", false));
   EXPECT_EQ("my_cool.level", makeFilenameFromString("my.cool.level", true));

   // Bug fix: dot at position 0 — sentinel was 0, conflicting with valid first index
   // Without the fix, ".level" with allowLastDot=true incorrectly returned "_level"
   EXPECT_EQ("_level", makeFilenameFromString(".level", false));
   EXPECT_EQ(".level", makeFilenameFromString(".level", true));

   // Leading dot followed by another dot — only the *last* dot is preserved
   EXPECT_EQ("_my_level", makeFilenameFromString(".my.level", false));
   EXPECT_EQ("_my.level", makeFilenameFromString(".my.level", true));

   // Dot-only string
   EXPECT_EQ("_", makeFilenameFromString(".", false));
   EXPECT_EQ(".", makeFilenameFromString(".", true));

   // Empty string
   EXPECT_EQ("", makeFilenameFromString(""));
   EXPECT_EQ("", makeFilenameFromString("", true));
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

   EXPECT_TRUE(TNL::isHex('0'));
   EXPECT_TRUE(TNL::isHex('f'));
   EXPECT_TRUE(TNL::isHex('A'));
   EXPECT_FALSE(TNL::isHex('g'));
}


// These are comparitors for the actual sort function; they do not do sorting themselves.
TEST(StringUtilsTest, sorting)
{
   EXPECT_TRUE(alphaSort("a", "b"));
   EXPECT_TRUE(alphaSort("A", "b"));
   EXPECT_TRUE(alphaSort("a", "B"));
   EXPECT_TRUE(alphaSort("A", "B"));
   EXPECT_FALSE(alphaSort("b", "a"));

   EXPECT_TRUE(alphaNumberSort("2", "10"));
   EXPECT_TRUE(alphaNumberSort("10", "a"));
   EXPECT_TRUE(alphaNumberSort("1", "2"));
   EXPECT_TRUE(alphaNumberSort("0", "1"));
   EXPECT_TRUE(alphaNumberSort("0", "abc"));
   EXPECT_TRUE(alphaNumberSort("abc", "def"));
   EXPECT_FALSE(alphaNumberSort("abc", "10"));
   EXPECT_FALSE(alphaNumberSort("2xyz", "1xyz"));

   // Positive itegers sort numerically
   EXPECT_TRUE(alphaNumberSort("2", "11"));
   EXPECT_FALSE(alphaNumberSort("2", "1"));

   // Mixed strings: leading numeric portion determines order
   EXPECT_TRUE(alphaNumberSort("2xyz", "11xyz"));   // 2 < 11
   EXPECT_TRUE(alphaNumberSort("1abc", "2abc"));    // 1 < 2
   EXPECT_FALSE(alphaNumberSort("10foo", "9foo"));  // 10 > 9
   EXPECT_FALSE(alphaNumberSort("2xyz", "2xyz"));   // equal -> alphaSort tie-break (false: not strictly less)
}


// Verify sort order of strings with numeric prefixes and varying alpha suffixes.
// Sort order: by numeric prefix first, then alphabetically within ties.
// "1BBB" "2AAA" "12AAA" "12BBB" "9BBB" "1ZZZ" -> "1BBB" "1ZZZ" "2AAA" "9BBB" "12AAA" "12BBB"
TEST(StringUtilsTest, sortingNumericPrefixAlphaSuffix)
{
   vector<string> items = { "1BBB", "2AAA", "12AAA", "12BBB", "9BBB", "1ZZZ" };
   std::sort(items.begin(), items.end(), alphaNumberSort);

   EXPECT_EQ("1BBB",  items[0]);
   EXPECT_EQ("1ZZZ",  items[1]);
   EXPECT_EQ("2AAA",  items[2]);
   EXPECT_EQ("9BBB",  items[3]);
   EXPECT_EQ("12AAA", items[4]);
   EXPECT_EQ("12BBB", items[5]);

   // Pairwise: same numeric prefix, alpha suffix breaks ties
   EXPECT_TRUE(alphaNumberSort("1BBB", "1ZZZ"));   // 1==1, "1BBB" < "1ZZZ"
   EXPECT_FALSE(alphaNumberSort("1ZZZ", "1BBB"));  // 1==1, "1ZZZ" > "1BBB"
   EXPECT_TRUE(alphaNumberSort("12AAA", "12BBB")); // 12==12, "12AAA" < "12BBB"
   EXPECT_FALSE(alphaNumberSort("12BBB", "12AAA")); // 12==12, "12BBB" > "12AAA"

   // Pairwise: different numeric prefixes always win regardless of alpha suffix
   EXPECT_TRUE(alphaNumberSort("1ZZZ", "2AAA"));   // 1 < 2 even though Z > A
   EXPECT_TRUE(alphaNumberSort("2AAA", "9BBB"));   // 2 < 9
   EXPECT_TRUE(alphaNumberSort("9BBB", "12AAA"));  // 9 < 12
   EXPECT_FALSE(alphaNumberSort("2AAA", "1BBB"));  // 2 > 1
}


// Verify that alphaNumberSort handles numbers larger than S32_MAX correctly.
TEST(StringUtilsTest, sortingLargeNumbers)
{
   EXPECT_TRUE(alphaNumberSort("2147483647", "2147483648"));
   EXPECT_FALSE(alphaNumberSort("2147483648", "2147483647"));
   EXPECT_TRUE(alphaNumberSort("10000000000000000000", "10000000000000000001"));
   EXPECT_FALSE(alphaNumberSort("10000000000000000000", "2"));
   EXPECT_TRUE(alphaNumberSort("2", "10000000000000000000"));

   // Mixed strings with large numbers
   EXPECT_TRUE(alphaNumberSort("2abc", "2147483648abc"));
   EXPECT_FALSE(alphaNumberSort("2147483648abc", "2abc"));

   // Natural sort for embedded numbers
   EXPECT_TRUE(alphaNumberSort("abc2", "abc10"));
   EXPECT_TRUE(alphaNumberSort("abc1", "abc2"));
   EXPECT_TRUE(alphaNumberSort("a1b2", "a1b10"));
   EXPECT_TRUE(alphaNumberSort("123a456", "123a457"));

   // Leading zeros
   EXPECT_TRUE(alphaNumberSort("1", "01"));
   EXPECT_TRUE(alphaNumberSort("01", "001"));
   EXPECT_TRUE(alphaNumberSort("abc1", "abc01"));
}


TEST(StringUtilsTest, formatMessage)
{
   Vector<StringTableEntry> e;
   e.push_back("entry0");
   e.push_back("entry1");

   Vector<StringPtr> s;
   s.push_back("ptr0");

   Vector<S32> i;
   i.push_back(42);

   EXPECT_EQ("entry0 and entry1", formatMessage("%e0 and %e1", e, s, i));
   EXPECT_EQ("ptr0 is 42", formatMessage("%s0 is %i0", e, s, i));
   EXPECT_EQ("plain text", formatMessage("plain text", e, s, i));
   EXPECT_EQ("invalid %x9 tokens", formatMessage("invalid %x9 tokens", e, s, i));
   EXPECT_EQ("out of range ", formatMessage("out of range %e9", e, s, i));

   // Long string test to ensure no overflow
   std::string longStr(500, 'a');
   e.push_back(longStr.c_str());
   EXPECT_EQ(longStr, formatMessage("%e2", e, s, i));

   // Multiple placeholders and mixed types
   EXPECT_EQ("entry0 entry1 ptr0 42 entry0", formatMessage("%e0 %e1 %s0 %i0 %e0", e, s, i));

   // Boundary cases
   EXPECT_EQ("%e", formatMessage("%e", e, s, i));
   EXPECT_EQ("%eX", formatMessage("%eX", e, s, i));
   EXPECT_EQ("hello %", formatMessage("hello %", e, s, i));
   EXPECT_EQ("hello %e", formatMessage("hello %e", e, s, i));

   // Empty vectors
   EXPECT_EQ("test ", formatMessage("test %e0", Vector<StringTableEntry>(), s, i));

   // Large number of placeholders
   std::string manyPlaceholders;
   for(int j = 0; j < 100; ++j) manyPlaceholders += "%e0 ";
   std::string expectedMany;
   for(int j = 0; j < 100; ++j) expectedMany += "entry0 ";
   EXPECT_EQ(trim(expectedMany), trim(formatMessage(manyPlaceholders.c_str(), e, s, i)));
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
