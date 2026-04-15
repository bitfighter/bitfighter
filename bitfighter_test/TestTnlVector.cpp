//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlVector.h"
#include "gtest/gtest.h"

namespace Zap {

TEST(TnlVectorTest, PopMethods)
{
   TNL::Vector<int> v;
   v.push_back(1);
   v.push_back(2);
   v.push_back(3);

   ASSERT_EQ(3, v.size());
   EXPECT_EQ(3, v.last());

   v.pop_back();
   ASSERT_EQ(2, v.size());
   EXPECT_EQ(2, v.last());

   v.pop_front();
   ASSERT_EQ(1, v.size());
   EXPECT_EQ(2, v.first());
}

} // namespace Zap
