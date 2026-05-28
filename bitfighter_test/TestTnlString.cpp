//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlString.h"
#include "gtest/gtest.h"
#include "tnlTypes.h"
#include "tnlAssert.h"

#include <string>
#include <string.h>
#include <stdlib.h>

using namespace TNL;

namespace Zap {

TEST(TnlStringTest, StringPtrAssignment)
{
   TNL::StringPtr s1("test");
   TNL::StringPtr s2;

   // Test assigning empty StringPtr to another
   EXPECT_NO_THROW(s1 = s2);
   EXPECT_STREQ("", s1.getString());

   // Test self-assignment
   TNL::StringPtr s3("self");
   EXPECT_NO_THROW(s3 = s3);
   EXPECT_STREQ("self", s3.getString());

   // Test assigning NULL char pointer
   TNL::StringPtr s4("before");
   EXPECT_NO_THROW(s4 = (const char*)NULL);
   EXPECT_STREQ("", s4.getString());
}

TEST(TnlStringTest, StringPtrConstructors)
{
   // NULL constructor
   TNL::StringPtr s1((const char*)NULL);
   EXPECT_STREQ("", s1.getString());

   // Empty string constructor
   TNL::StringPtr s2("");
   EXPECT_STREQ("", s2.getString());

   // Normal constructor
   TNL::StringPtr s3("hello");
   EXPECT_STREQ("hello", s3.getString());

   // Copy constructor with NULL
   TNL::StringPtr sNull;
   TNL::StringPtr s4(sNull);
   EXPECT_STREQ("", s4.getString());
}


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

TEST(TnlStringTest, StringPtrSelfAssignmentFromBuffer)
{
    TNL::StringPtr s("hello");
    const char* ptr = s.getString();
    s = ptr;
    EXPECT_STREQ("hello", s.getString());

    // Test with substring
    s = "hello world";
    ptr = s.getString() + 6;
    s = ptr;
    EXPECT_STREQ("world", s.getString());
}

TEST(TnlStringTest, StdStringConstructor)
{
   std::string std_s = "hello";
   StringPtr s(std_s);
   EXPECT_STREQ("hello", s.getString());
}

} // namespace Zap
