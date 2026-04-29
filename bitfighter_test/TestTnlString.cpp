//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlTypes.h"
#include "tnlAssert.h"
#include "tnlString.h"
#include "gtest/gtest.h"
#include <string>
#include <string.h>
#include <stdlib.h>

namespace Zap
{

using namespace TNL;

TEST(TnlStringTest, DefaultConstructor)
{
   StringPtr s;
   EXPECT_STREQ("", s.getString());
   EXPECT_STREQ("", (const char *)s);
}

TEST(TnlStringTest, CStringConstructor)
{
   StringPtr s("hello");
   EXPECT_STREQ("hello", s.getString());
}

TEST(TnlStringTest, NullCStringConstructor)
{
   // This is expected to crash due to strlen(NULL)
   StringPtr s((const char *)NULL);
   EXPECT_STREQ("", s.getString());
}

TEST(TnlStringTest, CopyConstructor)
{
   StringPtr s1("hello");
   StringPtr s2(s1);
   EXPECT_STREQ("hello", s2.getString());
}

TEST(TnlStringTest, AssignmentOperator)
{
   StringPtr s1("hello");
   StringPtr s2;
   s2 = s1;
   EXPECT_STREQ("hello", s2.getString());
}

TEST(TnlStringTest, SelfAssignment)
{
   // This is expected to crash or cause use-after-free
   StringPtr s("hello");
   s = s;
   EXPECT_STREQ("hello", s.getString());
}

TEST(TnlStringTest, NullAssignment)
{
   // Assigning from a StringPtr that holds NULL
   StringPtr s1;
   StringPtr s2("hello");
   s2 = s1;
   EXPECT_STREQ("", s2.getString());
}

TEST(TnlStringTest, NullCStringAssignment)
{
   // This is expected to crash due to strlen(NULL)
   StringPtr s("hello");
   s = (const char *)NULL;
   EXPECT_STREQ("", s.getString());
}

TEST(TnlStringTest, StdStringConstructor)
{
   std::string std_s = "hello";
   StringPtr s(std_s);
   EXPECT_STREQ("hello", s.getString());
}

} // namespace Zap
