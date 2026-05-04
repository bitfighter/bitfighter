#include "tnlVector.h"
#include "gtest/gtest.h"

namespace Zap {

TEST(TnlVectorBoolTest, AddressBehavior)
{
   TNL::Vector<bool> v;
   v.push_back(true);
   v.push_back(false);
   v.push_back(true);

   ASSERT_EQ(3, v.size());

   bool* ptr = v.address();
   ASSERT_NE(nullptr, ptr);

   // v[0] should be at ptr[0]
   // v[1] should be at ptr[1]
   // v[2] should be at ptr[2]

   EXPECT_EQ(true, ptr[0]);

   // In the buggy version, ptr[1] points to the second byte of the first S32.
   // Since v[0] was true (1), the S32 is 0x00000001 (on little endian).
   // So ptr[1] will be 0, which is false.
   // But v[1] was set to false, so this might accidentally pass if we're not careful.

   v.clear();
   v.push_back(true);
   v.push_back(true);
   v.push_back(true);

   ptr = v.address();
   EXPECT_EQ(true, ptr[0]);
   EXPECT_EQ(true, ptr[1]); // This will FAIL in buggy version because ptr[1] is the 2nd byte of v[0]'s S32
   EXPECT_EQ(true, ptr[2]); // This will FAIL too
}

} // namespace Zap
