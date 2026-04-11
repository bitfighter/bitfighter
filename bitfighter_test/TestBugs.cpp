//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlVector.h"
#include "stringUtils.h"
#include "gtest/gtest.h"
#include <string>

namespace TNL {

TEST(VectorBugTest, PopBackWorks)
{
   Vector<std::string> v;
   v.push_back("test");
   EXPECT_EQ(1, v.size());

   v.pop_back();
   EXPECT_EQ(0, v.size());
}

TEST(VectorBugTest, PopFrontWorks)
{
   Vector<std::string> v;
   v.push_back("test");
   EXPECT_EQ(1, v.size());

   v.pop_front();
   EXPECT_EQ(0, v.size());
}

} // namespace TNL

namespace Zap {

TEST(StringUtilsBugTest, ParseStringEmptyInput)
{
   Vector<string> words;
   parseString("", words, ',');
   EXPECT_EQ(0, words.size());

   parseString(NULL, words, ',');
   EXPECT_EQ(0, words.size());
}

} // namespace Zap
