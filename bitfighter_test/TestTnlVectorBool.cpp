//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlVector.h"
#include "gtest/gtest.h"

namespace Zap {

TEST(TnlVectorBoolTest, BasicOperations)
{
   TNL::Vector<bool> v;
   v.push_back(true);
   v.push_back(false);
   v.push_back(true);

   ASSERT_EQ(3, v.size());
   EXPECT_EQ(true, v[0]);
   EXPECT_EQ(false, v[1]);
   EXPECT_EQ(true, v[2]);
}

TEST(TnlVectorBoolTest, AddressIndexing)
{
   TNL::Vector<bool> v;
   v.push_back(true);
   v.push_back(false);
   v.push_back(true);

   bool* ptr = v.address();
   ASSERT_TRUE(ptr != NULL);

   // If Vector<bool> uses std::vector<S32> internally, address()[1] will
   // likely point to the second byte of the first S32, not the second bool.
   // On little-endian, the first S32 (true) is 0x00000001.
   // ptr[0] is 0x01 (true)
   // ptr[1] is 0x00 (false)
   // ptr[2] is 0x00 (false)
   // ptr[3] is 0x00 (false)
   // ptr[4] is 0x00 (false) - this is the first byte of the second S32 (which is 0/false)

   // We expect ptr[1] to be the second element we pushed, which was false.
   // But we also expect ptr[2] to be the third element, which was true.

   EXPECT_EQ(v[0], ptr[0]);
   EXPECT_EQ(v[1], ptr[1]);
   EXPECT_EQ(v[2], ptr[2]);  // This will likely fail if the bug exists
}

TEST(TnlVectorBoolTest, WriteAccess)
{
   TNL::Vector<bool> v;
   v.push_back(false);
   v.push_back(false);

   v[0] = true;
   EXPECT_EQ(true, v[0]);

   v[1] = true;
   EXPECT_EQ(true, v[1]);

   // Ensure writing to one didn't affect the other
   EXPECT_EQ(true, v[0]);
}

} // namespace Zap
