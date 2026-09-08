//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlBitSet.h"
#include "gtest/gtest.h"

namespace Zap {

using namespace TNL;

TEST(BitSetTest, Construction)
{
   BitSet32 b1;
   EXPECT_EQ(0u, b1.getMask());

   BitSet32 b2(0xDEADBEEF);
   EXPECT_EQ(0xDEADBEEFUL, b2.getMask());

   BitSet32 b3(b2);
   EXPECT_EQ(0xDEADBEEFUL, b3.getMask());

   BitSet32 b4 = 0x12345678;
   EXPECT_EQ(0x12345678UL, b4.getMask());
}

TEST(BitSetTest, SetMethods)
{
   BitSet32 b;
   b.set();
   EXPECT_EQ(0xFFFFFFFFUL, b.getMask());

   b.clear();
   EXPECT_EQ(0u, b.getMask());

   b.set(0x1);
   EXPECT_EQ(0x1u, b.getMask());

   b.set(0x2);
   EXPECT_EQ(0x3u, b.getMask());

   b.clear(0x1);
   EXPECT_EQ(0x2u, b.getMask());

   b.toggle(0x3); // Toggle bits 1 and 2. Bit 2 is currently set, so it becomes clear. Bit 1 is clear, so it becomes set.
   EXPECT_EQ(0x1u, b.getMask());
}

TEST(BitSetTest, BooleanSet)
{
   BitSet32 b(0xF0);

   // set(BitSet32 s, bool b)
   // For each bit set in s, sets or clears that bit in this, depending on b.

   b.set(BitSet32(0x0F), true);
   EXPECT_EQ(0xFFu, b.getMask());

   b.set(BitSet32(0xAA), false);
   // 0xFF & ~0xAA = 0x55
   EXPECT_EQ(0x55u, b.getMask());
}

TEST(BitSetTest, TestingMethods)
{
   BitSet32 b(0x0000000F);

   EXPECT_TRUE(b.test(0x01));
   EXPECT_TRUE(b.test(0x08));
   EXPECT_FALSE(b.test(0x10));
   EXPECT_TRUE(b.test(0x03)); // Tests if ANY of these bits are set

   EXPECT_TRUE(b.testStrict(0x0F));
   EXPECT_TRUE(b.testStrict(0x01));
   EXPECT_FALSE(b.testStrict(0x1F));
}

TEST(BitSetTest, Operators)
{
   BitSet32 b(0x0F);

   EXPECT_EQ(0x1Fu, (b | 0x10u).getMask());
   EXPECT_EQ(0x0Eu, (b & 0x0Eu).getMask());
   EXPECT_EQ(0xF0u, (b ^ 0xFFu).getMask());

   b |= 0x10;
   EXPECT_EQ(0x1Fu, b.getMask());

   b &= 0x01;
   EXPECT_EQ(0x01u, b.getMask());

   b ^= 0x11;
   EXPECT_EQ(0x10u, b.getMask());
}

} // namespace Zap
