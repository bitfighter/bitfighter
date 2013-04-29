// Bitfighter Tests

#include "gtest/gtest.h"

#include "Point.h"
#include <string>

#include <stdio.h>

using namespace Zap;

namespace {

class BfTest : public ::testing::Test
{



};

TEST_F(BfTest, testOne) 
{
   Point p(3,4);
   EXPECT_EQ(p.distanceTo(Point(0,0)), 5);
}


}     // namespace

int main(int argc, char **argv) 
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


