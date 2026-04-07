//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "statistics.h"
#include "gtest/gtest.h"

namespace Zap
{

TEST(StatisticsTest, GetHitRateNoShotsDoesNotDivideByZero)
{
   Statistics stats;

   // With zero shots, getHitRate() must return 0.0 without crashing
   EXPECT_FLOAT_EQ(0.0f, stats.getHitRate());
}


TEST(StatisticsTest, GetHitRatePerWeaponNoShotsDoesNotDivideByZero)
{
   Statistics stats;

   // Per-weapon overload: also safe when no shots have been fired with that weapon
   EXPECT_FLOAT_EQ(0.0f, stats.getHitRate(WeaponPhaser));
   EXPECT_FLOAT_EQ(0.0f, stats.getHitRate(WeaponBounce));
   EXPECT_FLOAT_EQ(0.0f, stats.getHitRate(WeaponBurst));
}


TEST(StatisticsTest, GetHitRateCalculatedCorrectly)
{
   Statistics stats;

   // Fire 4 shots, land 2 hits -> 50% hit rate
   stats.countShot(WeaponPhaser);
   stats.countShot(WeaponPhaser);
   stats.countShot(WeaponPhaser);
   stats.countShot(WeaponPhaser);
   stats.countHit(WeaponPhaser);
   stats.countHit(WeaponPhaser);

   EXPECT_FLOAT_EQ(0.5f, stats.getHitRate());
   EXPECT_FLOAT_EQ(0.5f, stats.getHitRate(WeaponPhaser));
}


TEST(StatisticsTest, GetHitRatePerWeaponZeroShotsDifferentWeapon)
{
   Statistics stats;

   // Only fire Phaser; Bounce should still return 0.0 without crashing
   stats.countShot(WeaponPhaser);
   stats.countHit(WeaponPhaser);

   EXPECT_FLOAT_EQ(1.0f, stats.getHitRate(WeaponPhaser));
   EXPECT_FLOAT_EQ(0.0f, stats.getHitRate(WeaponBounce));  // no shots with Bounce
}


TEST(StatisticsTest, GetHitRateResetClearsStats)
{
   Statistics stats;

   stats.countShot(WeaponPhaser);
   stats.countHit(WeaponPhaser);
   EXPECT_FLOAT_EQ(1.0f, stats.getHitRate());

   stats.resetStatistics();

   // After reset, back to zero shots -> must return 0.0 safely
   EXPECT_FLOAT_EQ(0.0f, stats.getHitRate());
   EXPECT_FLOAT_EQ(0.0f, stats.getHitRate(WeaponPhaser));
}

} // namespace Zap
