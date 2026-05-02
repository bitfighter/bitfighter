//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlVector.h"
#include "gtest/gtest.h"
#include <vector>

namespace Zap {

TEST(TnlVectorBoolTest, AddressAndIndexing)
{
   TNL::Vector<bool> v;
   v.push_back(true);
   v.push_back(false);
   v.push_back(true);

   ASSERT_EQ(3, v.size());

   // Current implementation uses std::vector<S32> internally for Vector<bool>
   // This test should demonstrate the issue if sizeof(bool) != sizeof(S32)
   bool *addr = v.address();

   // If the bug exists, addr[1] will not correctly point to the second bool
   // because it's actually an array of S32, and address() casts S32* to bool*

   // We expect these to match the values we pushed
   EXPECT_EQ(true, v[0]);
   EXPECT_EQ(false, v[1]);
   EXPECT_EQ(true, v[2]);

   // Using address() to access elements should also work if the memory layout is correct
   // (i.e., if sizeof(bool) == sizeof(element_in_internal_vector))
   if (addr)
   {
      EXPECT_EQ(true, addr[0]);
      // This is expected to FAIL with the current bug where internal is S32 (4 bytes) and bool is 1 byte
      EXPECT_EQ(false, addr[1]);
      EXPECT_EQ(true, addr[2]);
   }
}

} // namespace Zap
