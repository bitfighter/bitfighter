//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlString.h"
#include "gtest/gtest.h"

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

} // namespace Zap
