//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlNonce.h"
#include "tnlBitStream.h"
#include "gtest/gtest.h"

using namespace TNL;

namespace Zap
{

TEST(NonceTest, ReadWriteSymmetry)
{
   Nonce src;
   src.getRandom();

   PacketStream stream;
   src.write(&stream);

   stream.setBitPosition(0);

   Nonce dest;
   dest.read(&stream);

   EXPECT_TRUE(dest.isValid());
   EXPECT_TRUE(src == dest);
}

} // namespace Zap
