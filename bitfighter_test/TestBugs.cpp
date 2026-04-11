//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlVector.h"
#include "gtest/gtest.h"
#include <string>

namespace TNL {

TEST(VectorBugTest, PopBackDanglingReference)
{
   Vector<std::string> v;
   // Use a string long enough to avoid Short String Optimization (SSO)
   std::string longStr = "This is a long string that should definitely require heap allocation to store its content.";
   v.push_back(longStr);

   // pop_back returns T&, but the element is removed before return.
   // This should trigger a Use-After-Free.
   std::string s = v.pop_back();

   EXPECT_EQ(longStr, s);
}

TEST(VectorBugTest, PopFrontDanglingReference)
{
   Vector<std::string> v;
   std::string longStr = "Another long string for testing pop_front to ensure we catch Use-After-Free bugs.";
   v.push_back(longStr);

   // pop_front returns T&, but the element is removed before return.
   // This should trigger a Use-After-Free.
   std::string s = v.pop_front();

   EXPECT_EQ(longStr, s);
}

} // namespace TNL
