//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "game.h"
#include "gtest/gtest.h"

namespace Zap
{

TEST(GameTest, AspectRatio)
{
   // Aspect ratio of visible area should be the same in normal and sensor modes
   EXPECT_FLOAT_EQ(Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL / Game::PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_HORIZONTAL, 
                   Game::PLAYER_VISUAL_DISTANCE_VERTICAL   / Game::PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_VERTICAL);
}

};
