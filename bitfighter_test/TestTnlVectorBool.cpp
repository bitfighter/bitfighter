//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlVector.h"
#include "gtest/gtest.h"

namespace Zap {

TEST(TnlVectorBoolTest, AddressPointerArithmetic)
{
   TNL::Vector<bool> v;
   v.push_back(true);
   v.push_back(false);
   v.push_back(true);

   ASSERT_EQ(3, v.size());

   bool *ptr = v.address();
   ASSERT_TRUE(ptr != NULL);

   // If sizeof(bool) is 1 and underlying storage is S32 (sizeof=4),
   // then ptr[1] will be the second byte of the first S32,
   // not the first byte of the second S32.

   EXPECT_EQ(v[0], ptr[0]);
   EXPECT_EQ(v[1], ptr[1]);
   EXPECT_EQ(v[2], ptr[2]);
}

TEST(TnlVectorBoolTest, OperatorBracketCasting)
{
   TNL::Vector<bool> v;
   v.push_back(true);
   v.push_back(false);

   // Verify that operator[] returns a reference that can be modified
   v[1] = true;
   EXPECT_EQ(true, v[1]);

   bool *ptr = v.address();
   EXPECT_EQ(true, ptr[0]);
}

TEST(TnlVectorBoolTest, PushPop)
{
   TNL::Vector<bool> v;
   v.push_back(true);
   v.push_back(false);
   EXPECT_EQ(2, v.size());
   EXPECT_EQ(true, v[0]);
   EXPECT_EQ(false, v[1]);
   v.pop_back();
   EXPECT_EQ(1, v.size());
   EXPECT_EQ(true, v[0]);

   v.push_front(false);
   EXPECT_EQ(2, v.size());
   EXPECT_EQ(false, v[0]);
   EXPECT_EQ(true, v[1]);
}

} // namespace Zap
