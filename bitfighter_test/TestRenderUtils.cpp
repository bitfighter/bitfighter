//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "RenderUtils.h"
#include "stringUtils.h"
#include "FontManager.h"

#include "gtest/gtest.h"
#include "tnlVector.h"
#include <string>

namespace Zap {
using namespace TNL;
using namespace std;
	
static void testThatLinesAreNotLongerThanMax(const Vector<string> &lines, S32 maxLen, S32 fontSize)
{
   for(S32 i = 0; i < lines.size(); i++)    
   {
      if(getStringWidth(fontSize, lines[i].c_str()) > maxLen)
      {
         int x = getStringWidth(fontSize, lines[i].c_str());
         int y = 0;
      }
      EXPECT_TRUE(getStringWidth(fontSize, lines[i].c_str()) <= maxLen) << 
            "TestThatLinesAreNotLongerThanMax failed on iteration " << i <<"; strlen=" << getStringWidth(10, lines[i].c_str());

      // Make sure there is no leading or trailing whitespace
      EXPECT_NE(lines[i][0], ' ')                     << "Leading space on iter "  << i <<"; str = \"" << lines[i] << "\"";
      EXPECT_NE(lines[i][lines[i].length() - 1], ' ') << "Trailing space on iter " << i <<"; str = \"" << lines[i] << "\"";
   }
}


// Here make sure that just one more character would make the line too long
static void wouldOneMoreCharMakeTheLineTooLong(const Vector<string> &lines, S32 maxLen, S32 fontSize)
{
   for(S32 i = 0; i < lines.size() - 1; i++)    
   {
      string line = lines[i] + lines[i + 1][0];    // Add first char of next item

      EXPECT_TRUE(getStringWidth(fontSize, line.c_str()) > maxLen) << 
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

      EXPECT_TRUE(getStringWidth(fontSize, line.c_str()) > maxLen) <<
         "WouldOneMoreWordMakeTheLineTooLong failed on iteration " << i << "; len = " << getStringWidth(10, line.c_str());
   }
}

TEST(RenderUtilsTest, StringWrappingTests)
{
   Vector<string> lines, lines2;
   FontManager::initialize(NULL, false);

   lines = wrapString("", 200, 10);
   EXPECT_EQ(0, lines.size());

   lines = wrapString("A", 200, 10);
   EXPECT_EQ(1, lines.size());

   lines = wrapString("Short string", 200, 10);
   ASSERT_EQ(1, lines.size());
   EXPECT_TRUE(lines[0] == "Short string");

   // Make sure a terminal \n does not create a new line
   lines = wrapString("Short string\n", 200, 10);
   ASSERT_EQ(1, lines.size());
   EXPECT_TRUE(lines[0] == "Short string");

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
         lines = wrapString(longLine, 300, 14);

         SCOPED_TRACE("Pressure Test ==> width=" + itos(width) + ", size=" + itos(size));
         testThatLinesAreNotLongerThanMax(lines, 300, 14);
         wouldOneMoreWordMakeTheLineTooLong(lines, 300, 14);
      }
   }
}

};