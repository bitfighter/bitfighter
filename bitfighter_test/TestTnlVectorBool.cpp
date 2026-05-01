//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlVector.h"
#include "gtest/gtest.h"

namespace Zap {

TEST(TnlVectorBoolTest, AddressIndexing)
{
   TNL::Vector<bool> v;
   v.push_back(true);
   v.push_back(false);
   v.push_back(true);

   ASSERT_EQ(3, v.size());

   bool *addr = v.address();
   ASSERT_NE(nullptr, addr);

   // addr[0] should be the first bool
   EXPECT_EQ(true, addr[0]);

   // addr[1] should be the second bool
   EXPECT_EQ(false, addr[1]);

   // addr[2] should be the third bool
   EXPECT_EQ(true, addr[2]);

   // The addresses should match exactly
   EXPECT_EQ((void*)&v[0], (void*)&addr[0]);
   EXPECT_EQ((void*)&v[1], (void*)&addr[1]);
   EXPECT_EQ((void*)&v[2], (void*)&addr[2]);
}

TEST(TnlVectorBoolTest, BasicOperations)
{
   TNL::Vector<bool> v;
   v.push_back(true);
   v.push_back(false);

   EXPECT_EQ(true, v[0]);
   EXPECT_EQ(false, v[1]);

   v[1] = true;
   EXPECT_EQ(true, v[1]);

   v.push_front(false);
   EXPECT_EQ(false, v[0]);
   EXPECT_EQ(true, v[1]);
   EXPECT_EQ(true, v[2]);
}

} // namespace Zap
