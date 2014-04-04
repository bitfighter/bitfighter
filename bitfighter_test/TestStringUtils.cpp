//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "stringUtils.h"
#include "RenderUtils.h"
#include "FontManager.h"

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


TEST(StringUtilsTest, chopComment)
{
   EXPECT_EQ("This is a ",        chopComment("This is a #comment"));
   EXPECT_EQ("",                  chopComment("# This is a comment"));
   EXPECT_EQ("This is a comment", chopComment("This is a comment#"));
   EXPECT_EQ("This is a comment", chopComment("This is a comment"));
   EXPECT_EQ("",                  chopComment("#"));
   EXPECT_EQ("",                  chopComment(""));
}


static void testThatLinesAreNotLongerThanMax(const Vector<string> &lines, S32 maxLen, S32 fontSize)
{
   for(S32 i = 0; i < lines.size(); i++)    
   {
      if(getStringWidth(fontSize, lines[i].c_str()) > maxLen)
      {
         int x = getStringWidth(fontSize, lines[i].c_str());
         int y = 0;
      }
      EXPECT_GE(maxLen, getStringWidth(fontSize, lines[i].c_str())) << 
            "TestThatLinesAreNotLongerThanMax failed on iteration " << i <<"; fontSize=" << fontSize << 
                                                       "; strlen=" << getStringWidth(fontSize, lines[i].c_str()) <<
                                                       "; line=" << lines[i];

      // Make sure there is no leading or trailing whitespace
      EXPECT_NE(lines[i][0], ' ')                     << "Leading space on iter "  << i <<"; str = \"" << lines[i] << "\"";
      ASSERT_GT(lines[i].length(), 0);
      EXPECT_NE(lines[i][lines[i].length() - 1], ' ') << "Trailing space on iter " << i <<"; str = \"" << lines[i] << "\"";
   }
}


// Here make sure that just one more character would make the line too long
static void wouldOneMoreCharMakeTheLineTooLong(const Vector<string> &lines, S32 maxLen, S32 fontSize)
{
   for(S32 i = 0; i < lines.size() - 1; i++)    
   {
      string line = lines[i] + lines[i + 1][0];    // Add first char of next item

      EXPECT_GE(getStringWidth(fontSize, line.c_str()), maxLen) << 
            "WouldOneMoreCharMakeTheLineTooLong failed on iteration " << i <<"; len = " << getStringWidth(10, line.c_str());
   }
}


static void wouldOneMoreWordMakeTheLineTooLong(const Vector<string> &lines, S32 maxLen, S32 fontSize)
{
   Vector<string> words;
   for(S32 i = 0; i < lines.size() - 1; i++)
   {
      parseString(lines[i + 1], words);
      string line = lines[i] + ' ' + words[0];

      EXPECT_GE(getStringWidth(fontSize, line.c_str()), maxLen) <<
         "WouldOneMoreWordMakeTheLineTooLong failed on iteration " << i << "; len = " << getStringWidth(10, line.c_str());
   }
}


TEST(StringUtilsTest, ParseStringTests)
{
   Vector<string> words;
   parseString("   Superior, Michigan, Huron    , Erie,     Ontario", words, ',');

   // Parsed words should come out stripped of all extraneous whitespace
   ASSERT_EQ(5, words.size());
   EXPECT_EQ("Superior", words[0]);
   EXPECT_EQ("Michigan", words[1]);
   EXPECT_EQ("Huron",    words[2]);
   EXPECT_EQ("Erie",     words[3]);
   EXPECT_EQ("Ontario",  words[4]);
}


// Test wrapping based on character counts
TEST(StringUtilsTest, WrapStringsMaxChars)
{
   FontManager::initialize(NULL, false);

   string s = "hello there";
   Vector<string> lines = wrapString(s, 8);
   ASSERT_EQ(2, lines.size());
   EXPECT_EQ("hello", lines[0]);
   EXPECT_EQ("there", lines[1]);

   // Big line
   string longLine =
      "Now is the winter of our discontent "
      "Made glorious summer by this sun of York; "
      "And all the clouds that lour'd upon our house "
      "In the deep bosom of the ocean buried. "
      "Now are our brows bound with victorious wreaths; "
      "Our bruised arms hung up for monuments; "
      "Our stern alarums changed to merry meetings, "
      "Our dreadful marches to delightful measures.";

   lines = wrapString(longLine, 70);

   ASSERT_EQ(6, lines.size());
   EXPECT_EQ("Now is the winter of our discontent Made glorious summer by this sun", lines[0]);
   EXPECT_EQ("of York; And all the clouds that lour'd upon our house In the deep", lines[1]);
   EXPECT_EQ("bosom of the ocean buried. Now are our brows bound with victorious", lines[2]);
   EXPECT_EQ("wreaths; Our bruised arms hung up for monuments; Our stern alarums", lines[3]);
   EXPECT_EQ("changed to merry meetings, Our dreadful marches to delightful", lines[4]);
   EXPECT_EQ("measures.", lines[5]);

   // Test wrapping with length NO_AUTO_WRAP, which will disable length checking and only wrap on \ns.
   string taxLine1 = "The withdrawal of your contribution won't count as taxable income, but any earnings you take out do.";
   string taxline2 = "For example, if you contributed $3,000 and had to withdraw $3,000 plus $150 of earnings, that $150 of earnings counts as taxable income. In addition, if you're not 59 1/2 years old, those earnings are also hit with the 10 percent additional tax on early withdrawals.";
   lines = wrapString(taxLine1 + "\n" + taxline2, NO_AUTO_WRAP);
   ASSERT_EQ(2, lines.size());
   EXPECT_EQ(taxLine1, lines[0]);
   EXPECT_EQ(taxline2, lines[1]);
}


// Test wrapping based on rendered string width
TEST(StringUtilsTest, WrapStringsLineWidth)
{
   Vector<string> lines, lines2;
   FontManager::initialize(NULL, false);

   lines = wrapString("", 200, 10);
   EXPECT_EQ(0, lines.size());

   lines = wrapString("A", 200, 10);
   EXPECT_EQ(1, lines.size());

   lines = wrapString("Short string", 200, 10);
   ASSERT_EQ(1, lines.size());
   EXPECT_EQ("Short string", lines[0]);

   // Make sure a terminal \n does not create a new line
   lines = wrapString("Short string\n", 200, 10);
   ASSERT_EQ(1, lines.size());
   EXPECT_EQ("Short string", lines[0]);

   lines = wrapString("Three\nShort\nLines", 200, 10);
   ASSERT_EQ(3, lines.size());
   EXPECT_TRUE(lines[0] == "Three" && lines[1] == "Short" && lines[2] == "Lines");

   lines = wrapString("Three\n\nShort\n\nDouble spaced lines", 200, 10);
   ASSERT_EQ(5, lines.size());
   EXPECT_TRUE(lines[0] == "Three" && lines[1] == "" && lines[2] == "Short" && lines[3] == "" && lines[4] == "Double spaced lines");

   lines = wrapString("One longer line that will require wrapping due to short length", 50, 10);
   EXPECT_EQ(9, lines.size());
   {
      SCOPED_TRACE("Scenario 1");
      testThatLinesAreNotLongerThanMax(lines, 50, 10);
   }

   // Forcing the linebreaks where they would normally fall should produce same result
   lines2 = wrapString("One\nlonger\nline that\nwill\nrequire\nwrapping\ndue to\nshort\nlength", 50, 10);
   EXPECT_EQ(lines.size(), lines2.size());
   for(S32 i = 0; i < lines.size(); i++)
      EXPECT_EQ(lines[i], lines2[i]) << "Failed on iter " << i << "; \"" << lines[i] << "\" != \"" << lines2[i] << "\"!";
   {
      SCOPED_TRACE("Scenario 1A");
      testThatLinesAreNotLongerThanMax(lines2, 50, 10);
   }

   // Check wrapping where there are no spaces or newlines
   string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
   lines = wrapString(alphabet, 50, 10);

   string line;
   for(S32 i = 0; i < lines.size(); i++)
      line += lines[i];

   EXPECT_EQ(line, alphabet) << "Merge failed... expected " << alphabet << " got " << line;
   {
      SCOPED_TRACE("Scenario 2");
      testThatLinesAreNotLongerThanMax(lines, 50, 10);
      wouldOneMoreCharMakeTheLineTooLong(lines, 50, 10);
   }

   // Ok, one big pressure test to ensure everything is copacetic
   string longLine =
      "Now is the winter of our discontent "
      "Made glorious summer by this sun of York; "
      "And all the clouds that lour'd upon our house "
      "In the deep bosom of the ocean buried. "
      "Now are our brows bound with victorious wreaths; "
      "Our bruised arms hung up for monuments; "
      "Our stern alarums changed to merry meetings, "
      "Our dreadful marches to delightful measures.";

   for(S32 i = 0; i < 50; i++)
   {
      S32 width = 150 + i * 10;
      for(S32 j = 0; j < 5; j++)
      {
         S32 size = 10 + 2 * j;
         lines = wrapString(longLine, width, size);

         SCOPED_TRACE("Pressure Test ==> width=" + itos(width) + ", size=" + itos(size));
         testThatLinesAreNotLongerThanMax(lines, width, size);
         wouldOneMoreWordMakeTheLineTooLong(lines, width, size);
      }
   }
}


};
