//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Tests that verify Ship::packUpdate() / Ship::unpackUpdate() round-trip
// correctly for xtank vehicles — specifically the xtank design block
// (XtankBodyMask) and the per-slot ammo/active state that lives inside the
// energy-meter block (mPackUnpackShipEnergyMeter).
//
// Coverage:
//   1. Xtank design (weapons, mounts, engine, etc.) survives a pack/unpack
//      round-trip.
//   2. Per-slot ammo counts survive the round-trip.
//   3. Per-slot mIsActive flags survive the round-trip.
//   4. Depleted ammo (< max) is transmitted faithfully.
//   5. Infinite-ammo weapons (max_ammo == S32_MAX) are not transmitted and
//      the client state is left at whatever initXtankWeaponStates() set.
//   6. Disabled (mIsActive == false) weapon state survives the round-trip.
//   7. A non-xtank (Bitfighter ship) round-trip does not corrupt weapon states.

#include "ship.h"
#include "VehicleDesign.h"
#include "gameConnection.h"
#include "tnlBitStream.h"

#include "gtest/gtest.h"

namespace Zap
{

using namespace TNL;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Perform a full pack/unpack round-trip between `server` and `client` using a
// GameConnection that has mPackUnpackShipEnergyMeter enabled (the flag that
// gates the energy / xtank ammo block).
//
// The mask uses XtankBodyMask | XtankWeaponStateMask to trigger both the
// design block and the per-slot ammo/active state block.  The energy-meter
// block is unconditional (not gated by any mask bit) so it always runs when
// mPackUnpackShipEnergyMeter is true.  Deliberately excludes PositionMask and
// MoveMask so that unpackUpdate never calls processMove(), which would
// otherwise overwrite the just-decoded mXtankDesign.body with the default
// mCurrentMove.bodyIndex = -1 (BITFIGHTER_SHIP).
static void xtankPackUnpack(Ship &server, Ship &client, U32 mask = Ship::XtankBodyMask | Ship::XtankWeaponAmmoMask)
{
   GameConnection conn;
   conn.mPackUnpackShipEnergyMeter = true;

   client.markAsGhost();

   BitStream stream;
   server.packUpdate(&conn, mask, &stream);
   stream.setBitPosition(0);
   client.unpackUpdate(&conn, &stream);
}

// Build an XtankDesign populated with two real weapons so all the interesting
// serialisation paths get exercised.
static VehicleDesign makeTestDesign()
{
   VehicleDesign d;
   d.body       = XtankBody::Lightcycle;
   d.engine     = XtankEngine::DEFAULT;
   d.tread      = XtankTread::DEFAULT;
   d.armor      = XtankArmor::DEFAULT;
   d.suspension = XtankSuspension::DEFAULT;
   d.bumper     = XtankBumper::DEFAULT;
   d.heatSinks  = 2;
   d.specials   = 0;

   // Slot 0: Light Machine Gun (finite ammo, max_ammo == 300)
   d.weapons[0]      = XtankWeapon::LIGHT_MACHINE_GUN;
   d.weaponMounts[0] = XtankMountLocation::TURRET1;

   // Slot 1: Machine Gun (finite ammo, max_ammo == 250)
   d.weapons[1]      = XtankWeapon::MACHINE_GUN;
   d.weaponMounts[1] = XtankMountLocation::TURRET2;

   // Remaining slots: NONE
   for(S32 i = 2; i < WEAPON_SLOTS; i++)
   {
      d.weapons[i]      = XtankWeapon::NONE;
      d.weaponMounts[i] = XtankMountLocation::NONE;
   }

   return d;
}

// Put `ship` into xtank mode with `design` and initialise weapon states from
// the design (full ammo, all active).
static void applyDesign(Ship &ship, const VehicleDesign &design)
{
   ship.mXtankDesign = design;
   ship.initXtankWeaponStates();
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// 1. The xtank design fields (weapons, mounts, engine, etc.) survive a full
//    pack/unpack round-trip through the XtankBodyMask block.
TEST(XtankPackUnpackTest, DesignSurvivesRoundTrip)
{
   Ship server, client;
   VehicleDesign design = makeTestDesign();
   applyDesign(server, design);

   xtankPackUnpack(server, client);

   const VehicleDesign &got = client.mXtankDesign;
   EXPECT_EQ(got.body,       design.body);
   EXPECT_EQ(got.engine,     design.engine);
   EXPECT_EQ(got.tread,      design.tread);
   EXPECT_EQ(got.heatSinks,  design.heatSinks);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      EXPECT_EQ(got.weapons[i],      design.weapons[i])      << "slot " << i;
      EXPECT_EQ(got.weaponMounts[i], design.weaponMounts[i]) << "slot " << i;
   }
}

// 2. Full ammo counts survive the round-trip.
TEST(XtankPackUnpackTest, FullAmmoSurvivesRoundTrip)
{
   Ship server, client;
   VehicleDesign design = makeTestDesign();
   applyDesign(server, design);

   xtankPackUnpack(server, client);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      XtankWeapon weapon = design.weapons[i];
      if(weapon == XtankWeapon::NONE)
         continue;

      const XtankWeaponInfo &wi  = xtankWeaponInfos[(S32)weapon];
      const XtankWeaponState &ws = client.getXtankWeaponState(i);

      EXPECT_EQ(ws.ammo, wi.max_ammo) << "slot " << i << " (" << wi.name << ") ammo mismatch";
   }
}

// 3. mIsActive (weapon on/off) survives the round-trip.
TEST(XtankPackUnpackTest, ActiveFlagSurvivesRoundTrip)
{
   Ship server, client;
   VehicleDesign design = makeTestDesign();
   applyDesign(server, design);

   // initXtankWeaponStates sets all weapons active; verify that is preserved.
   xtankPackUnpack(server, client);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      bool serverActive = server.getXtankWeaponState(i).mIsActive;
      bool clientActive = client.getXtankWeaponState(i).mIsActive;
      EXPECT_EQ(serverActive, clientActive) << "slot " << i << " mIsActive mismatch";
   }
}

// 4. Depleted (partial) ammo is transmitted faithfully.
TEST(XtankPackUnpackTest, PartialAmmoSurvivesRoundTrip)
{
   Ship server, client;
   VehicleDesign design = makeTestDesign();
   applyDesign(server, design);

   // Burn some ammo on slot 0 and slot 1.
   server.mWeaponStates[0].ammo = 42;
   server.mWeaponStates[1].ammo = 7;

   xtankPackUnpack(server, client);

   EXPECT_EQ(client.getXtankWeaponState(0).ammo, 42) << "slot 0 partial ammo";
   EXPECT_EQ(client.getXtankWeaponState(1).ammo,  7) << "slot 1 partial ammo";
}

// 5. Infinite-ammo weapons (max_ammo == S32_MAX) are not serialised; the
//    client value comes from initXtankWeaponStates() (i.e. also S32_MAX).
TEST(XtankPackUnpackTest, InfiniteAmmoWeaponNotSerialized)
{
   // Find a weapon with infinite ammo.
   XtankWeapon infiniteWeapon = XtankWeapon::NONE;
   for(S32 w = 0; w < XtankWeaponCount; w++)
   {
      if(xtankWeaponInfos[w].max_ammo == S32_MAX)
      {
         infiniteWeapon = (XtankWeapon)w;
         break;
      }
   }
   if(infiniteWeapon == XtankWeapon::NONE)
   {
      // No infinite-ammo weapon exists in this build — skip quietly.
      GTEST_SKIP() << "No infinite-ammo weapon found; skipping test.";
   }

   VehicleDesign design;
   design.body          = XtankBody::Lightcycle;
   design.weapons[0]    = infiniteWeapon;
   design.weaponMounts[0] = XtankMountLocation::TURRET1;
   for(S32 i = 1; i < WEAPON_SLOTS; i++)
   {
      design.weapons[i]      = XtankWeapon::NONE;
      design.weaponMounts[i] = XtankMountLocation::NONE;
   }

   Ship server, client;
   applyDesign(server, design);

   xtankPackUnpack(server, client);

   // After unpack the client runs initXtankWeaponStates which sets ammo to
   // max_ammo — verify it is S32_MAX.
   EXPECT_EQ(client.getXtankWeaponState(0).ammo, S32_MAX)
      << "Infinite-ammo weapon slot should remain at S32_MAX after round-trip";
}

// 6. A weapon that is turned off (mIsActive == false) has its flag
//    transmitted and remains off on the client.
TEST(XtankPackUnpackTest, DisabledWeaponFlagSurvivesRoundTrip)
{
   Ship server, client;
   VehicleDesign design = makeTestDesign();
   applyDesign(server, design);

   // Turn off slot 1.
   server.mWeaponStates[1].mIsActive = false;

   xtankPackUnpack(server, client);

   EXPECT_TRUE(client.getXtankWeaponState(0).mIsActive)  << "slot 0 should still be on";
   EXPECT_FALSE(client.getXtankWeaponState(1).mIsActive) << "slot 1 should be off";
}

// 7. A non-xtank (Bitfighter ship) round-trip does not write or read any
//    xtank ammo data; the client's weapon states stay at their default
//    (all NONE, ammo == 0).
TEST(XtankPackUnpackTest, BitfighterShipRoundTripDoesNotCorruptWeaponStates)
{
   Ship server, client;
   // Default Ship is BITFIGHTER_SHIP — no xtank fields written.

   xtankPackUnpack(server, client);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      EXPECT_EQ(client.getXtankWeaponState(i).ammo,      0)     << "slot " << i;
      EXPECT_FALSE(client.getXtankWeaponState(i).mIsActive)     << "slot " << i;
   }
}

};
