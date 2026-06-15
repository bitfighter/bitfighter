//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Tests for the xtank ammo, heat, and reload-timer subsystems added in
// ship.cpp (initXtankWeaponStates, coolHeat, advanceXtankTimers) and the
// XtankWeaponState struct helpers (hasAmmo, isOn).

#include "ship.h"
#include "VehicleDesign.h"

#include "gtest/gtest.h"

namespace Zap
{

// ---------------------------------------------------------------------------
// XtankWeaponState struct helpers
// ---------------------------------------------------------------------------

TEST(XtankWeaponStateTest, HasAmmo_TrueWhenAmmoPositive)
{
   XtankWeaponState ws;
   ws.ammo   = 5;
   ws.mIsActive = true;
   EXPECT_TRUE(ws.hasAmmo());
}

TEST(XtankWeaponStateTest, HasAmmo_FalseWhenAmmoZero)
{
   XtankWeaponState ws;
   ws.ammo   = 0;
   ws.mIsActive = true;
   EXPECT_FALSE(ws.hasAmmo());
}

TEST(XtankWeaponStateTest, IsOn_TrueWhenFlagSet)
{
   XtankWeaponState ws;
   ws.mIsActive = true;
   EXPECT_TRUE(ws.isActive());
}


// ---------------------------------------------------------------------------
// Ship::initXtankWeaponStates
// ---------------------------------------------------------------------------

TEST(XtankAmmoTest, InitWeaponStates_AllNone_GivesZeroAmmo)
{
   // Default Ship has all weapon slots set to XtankWeapon::NONE.
   Ship ship;
   ship.initXtankWeaponStates();

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      const XtankWeaponState &ws = ship.mWeaponStates[i];
      EXPECT_EQ(ws.ammo,        0)   << "slot " << i;
      EXPECT_EQ(ws.reloadTimer.getCurrent(), 0)   << "slot " << i;
      EXPECT_EQ(ws.refillTimer.getCurrent(), 0)   << "slot " << i;
      EXPECT_EQ(ws.mIsActive,      (U8)0) << "slot " << i;
   }
}

TEST(XtankAmmoTest, InitWeaponStates_ValidWeapon_StartsFullyLoaded)
{
   // Pick the first real weapon (LIGHT_MACHINE_GUN = 0) and assign it to slot 0.
   Ship ship;
   ship.mXtankDesign.weapons[0] = XtankWeapon::LIGHT_MACHINE_GUN;

   ship.initXtankWeaponStates();

   const XtankWeaponInfo &wi = xtankWeaponInfos[(S32)XtankWeapon::LIGHT_MACHINE_GUN];
   const XtankWeaponState &ws = ship.mWeaponStates[0];

   EXPECT_EQ(ws.ammo,        wi.max_ammo)    << "weapon should start fully loaded";
   EXPECT_EQ(ws.reloadTimer.getCurrent(), 0)              << "weapon should be ready to fire";
   EXPECT_EQ(ws.refillTimer.getCurrent(), wi.refill_time) << "refill timer should match weapon stat";
   EXPECT_TRUE(ws.isActive())                    << "weapon should start enabled";
   EXPECT_TRUE(ws.hasAmmo())                 << "weapon should report having ammo";
}

TEST(XtankAmmoTest, InitWeaponStates_RemainingSlots_StayNone)
{
   Ship ship;
   ship.mXtankDesign.weapons[0] = XtankWeapon::LIGHT_MACHINE_GUN;
   ship.initXtankWeaponStates();

   // Slots 1–5 were left as NONE and should remain zeroed.
   for(S32 i = 1; i < WEAPON_SLOTS; i++)
   {
      EXPECT_EQ(ship.mWeaponStates[i].ammo,   0) << "slot " << i;
      EXPECT_EQ(ship.mWeaponStates[i].mIsActive, (U8)0) << "slot " << i;
   }
}

// ---------------------------------------------------------------------------
// Ship::coolHeat
// ---------------------------------------------------------------------------

TEST(XtankAmmoTest, CoolHeat_ReducesHeatByExpectedAmount)
{
   Ship ship;
   ship.setXtankBodyForTest(XtankBody::Lightcycle);
   ship.mXtankDesign.heatSinks = 0;    // 1 dissipation unit per 1/15 s
   ship.mHeat = 100.0f;

   // With 0 heat sinks: dissipation = (0+1)*15/1000 = 0.015 heat/ms
   // After 1000 ms: heat should drop by 15.0
   ship.coolHeat(1000);

   EXPECT_NEAR(ship.getHeat(), 85.0f, 0.001f);
}

TEST(XtankAmmoTest, CoolHeat_WithMoreHeatSinks_CoolsFaster)
{
   Ship ship;
   ship.setXtankBodyForTest(XtankBody::Lightcycle);
   ship.mXtankDesign.heatSinks = 4;   // 5 dissipation units per 1/15 s
   ship.mHeat = 100.0f;

   // Dissipation = (4+1)*15/1000 = 0.075 heat/ms
   // After 1000 ms: drop by 75.0
   ship.coolHeat(1000);

   EXPECT_NEAR(ship.getHeat(), 25.0f, 0.001f);
}

TEST(XtankAmmoTest, CoolHeat_ClampsAtZero)
{
   Ship ship;
   ship.setXtankBodyForTest(XtankBody::Lightcycle);
   ship.mXtankDesign.heatSinks = 0;
   ship.mHeat = 5.0f;

   // More than enough cooling to reach zero
   ship.coolHeat(10000);

   EXPECT_EQ(ship.getHeat(), 0.0f);
}

TEST(XtankAmmoTest, CoolHeat_NoOpWhenAlreadyCool)
{
   Ship ship;
   ship.mHeat = 0.0f;
   ship.coolHeat(500);
   EXPECT_EQ(ship.getHeat(), 0.0f);
}

// ---------------------------------------------------------------------------
// Ship::advanceXtankTimers
// ---------------------------------------------------------------------------

TEST(XtankAmmoTest, AdvanceTimers_DecrementsReloadTimer)
{
   Ship ship;
   ship.mWeaponStates[0].reloadTimer.reset(500);

   ship.advanceXtankTimers(200);

   EXPECT_EQ(ship.mWeaponStates[0].reloadTimer.getCurrent(), 300);
}

TEST(XtankAmmoTest, AdvanceTimers_ClampsAtZero)
{
   Ship ship;
   ship.mWeaponStates[0].reloadTimer.reset(100);

   ship.advanceXtankTimers(500);   // More than the timer value

   EXPECT_EQ(ship.mWeaponStates[0].reloadTimer.getCurrent(), 0);
}

TEST(XtankAmmoTest, AdvanceTimers_DoesNotAffectZeroTimer)
{
   Ship ship;
   ship.mWeaponStates[0].reloadTimer.reset(0);

   ship.advanceXtankTimers(300);

   EXPECT_EQ(ship.mWeaponStates[0].reloadTimer.getCurrent(), 0);
}

TEST(XtankAmmoTest, AdvanceTimers_AdvancesAllSlots)
{
   Ship ship;
   for(S32 i = 0; i < WEAPON_SLOTS; i++)
      ship.mWeaponStates[i].reloadTimer.reset(100 * (i + 1));

   ship.advanceXtankTimers(50);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
      EXPECT_EQ(ship.mWeaponStates[i].reloadTimer.getCurrent(), 100 * (i + 1) - 50) << "slot " << i;
}

// ---------------------------------------------------------------------------
// Starting state checks
// ---------------------------------------------------------------------------

TEST(XtankAmmoTest, DefaultShip_MoneyIsStartingMoney)
{
   Ship ship;
   EXPECT_EQ(ship.getMoney(), STARTING_MONEY);
}

TEST(XtankAmmoTest, DefaultShip_HeatIsZero)
{
   Ship ship;
   EXPECT_EQ(ship.getHeat(), 0.0f);
}

};
