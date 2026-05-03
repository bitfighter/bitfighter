//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlVector.h"
#include "gtest/gtest.h"

using namespace TNL;

namespace Zap {

TEST(TnlVectorBoolTest, AddressCompatibility)
{
   Vector<bool> v;
   v.push_back(true);
   v.push_back(false);
   v.push_back(true);
   v.push_back(false);

   EXPECT_EQ(4, v.size());

   bool *ptr = v.address();
   ASSERT_NE(nullptr, ptr);

   // This is expected to fail with the current implementation because
   // address() returns a bool* into a std::vector<S32>, so ptr[1]
   // is actually the second byte of the first S32, not the first byte
   // of the second S32 (which is where v[1] is stored).
   EXPECT_EQ(v[0], ptr[0]);
   EXPECT_EQ(v[1], ptr[1]);
   EXPECT_EQ(v[2], ptr[2]);
   EXPECT_EQ(v[3], ptr[3]);
}

TEST(TnlVectorBoolTest, OperatorAndAddressConsistency)
{
    Vector<bool> v;
    for(int i = 0; i < 10; ++i)
        v.push_back((i % 2) == 0);

    bool *ptr = v.address();
    for(int i = 0; i < 10; ++i)
    {
        // v[i] accesses innerVector[i], which is at offset i * sizeof(S32)
        // ptr[i] accesses *(ptr + i), which is at offset i * sizeof(bool)
        EXPECT_EQ(v[i], ptr[i]) << "Mismatch at index " << i;
    }
}

} // namespace Zap
