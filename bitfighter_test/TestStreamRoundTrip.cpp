//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Round-trip tests for LoadoutTracker::writeToStream/readFromStream and
// VehicleDesign::writeToStream/readFromStream.

#include "LoadoutTracker.h"
#include "XtankShape.h"
#include "shipItems.h"

#include "tnlBitStream.h"

#ifdef TNL_OS_WIN32
#  include <windows.h>   // For ARRAYSIZE
#endif

#include "gtest/gtest.h"

namespace Zap
{

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Write src into a fresh BitStream, then read it back into dst.
static void roundTrip(LoadoutTracker &src, LoadoutTracker &dst)
{
   TNL::BitStream stream;
   src.writeToStream(&stream);
   stream.setBitPosition(0);
   dst.readFromStream(&stream);
}

static void roundTrip(VehicleDesign &src, VehicleDesign &dst)
{
   TNL::BitStream stream;
   src.writeToStream(&stream);
   stream.setBitPosition(0);
   dst.readFromStream(&stream);
}


// ---------------------------------------------------------------------------
// LoadoutTracker round-trip tests
// ---------------------------------------------------------------------------

struct LoadoutTrackerStreamTest : public ::testing::Test
{
   LoadoutTracker src;
   LoadoutTracker dst;
};


// A default-constructed tracker (all ModuleNone / WeaponNone) must survive
// a round trip without corruption.
TEST_F(LoadoutTrackerStreamTest, DefaultTracker_RoundTrip)
{
   roundTrip(src, dst);
   EXPECT_EQ(src, dst);
}


// A fully-populated tracker survives a round trip.
TEST_F(LoadoutTrackerStreamTest, FullyLoaded_RoundTrip)
{
   src = LoadoutTracker("Sensor,Armor,Bouncer,Phaser,Burst");
   roundTrip(src, dst);
   EXPECT_EQ(src, dst);

   // Verify the individual slots came through correctly.
   EXPECT_EQ(dst.getModule(0), ModuleSensor);
   EXPECT_EQ(dst.getModule(1), ModuleArmor);
   EXPECT_EQ(dst.getWeapon(0), WeaponBounce);
   EXPECT_EQ(dst.getWeapon(1), WeaponPhaser);
   EXPECT_EQ(dst.getWeapon(2), WeaponBurst);
}


// A tracker with Shield + Engineer and a different weapon combination.
TEST_F(LoadoutTrackerStreamTest, AlternateLoadout_RoundTrip)
{
   src = LoadoutTracker("Shield,Engineer,Triple,Phaser,Burst");
   roundTrip(src, dst);
   EXPECT_EQ(src, dst);

   EXPECT_EQ(dst.getModule(0), ModuleShield);
   EXPECT_EQ(dst.getModule(1), ModuleEngineer);
   EXPECT_EQ(dst.getWeapon(0), WeaponTriple);
   EXPECT_EQ(dst.getWeapon(1), WeaponPhaser);
   EXPECT_EQ(dst.getWeapon(2), WeaponBurst);
}


// Writing a tracker overwrites any prior content in the destination.
TEST_F(LoadoutTrackerStreamTest, Overwrite_PreExistingDst_RoundTrip)
{
   dst = LoadoutTracker("Shield,Engineer,Triple,Phaser,Burst");
   src = LoadoutTracker("Sensor,Armor,Bouncer,Phaser,Burst");

   roundTrip(src, dst);
   EXPECT_EQ(src, dst);
   EXPECT_EQ(dst.getModule(0), ModuleSensor);
}


// Multiple sequential round-trips through the same BitStream object must
// produce independent results with no cross-contamination.
TEST_F(LoadoutTrackerStreamTest, MultipleRoundTrips_Independent)
{
   LoadoutTracker a("Sensor,Armor,Bouncer,Phaser,Burst");
   LoadoutTracker b("Shield,Engineer,Triple,Phaser,Burst");
   LoadoutTracker ra, rb;

   roundTrip(a, ra);
   roundTrip(b, rb);

   EXPECT_EQ(a, ra);
   EXPECT_EQ(b, rb);
   EXPECT_NE(ra, rb);
}


// Write two different trackers end-to-end in a single stream and read them
// back in the same order; each must match its original.
TEST_F(LoadoutTrackerStreamTest, TwoTrackersSequentialInOneStream)
{
   LoadoutTracker a("Sensor,Armor,Bouncer,Phaser,Burst");
   LoadoutTracker b("Shield,Engineer,Triple,Phaser,Burst");
   LoadoutTracker ra, rb;

   TNL::BitStream stream;
   a.writeToStream(&stream);
   b.writeToStream(&stream);

   stream.setBitPosition(0);
   ra.readFromStream(&stream);
   rb.readFromStream(&stream);

   EXPECT_EQ(a, ra);
   EXPECT_EQ(b, rb);
}


// Pack/unpack (Vector<U8>) and stream round-trips must be consistent.
TEST_F(LoadoutTrackerStreamTest, StreamAndPackConsistent)
{
   src = LoadoutTracker("Sensor,Armor,Bouncer,Phaser,Burst");

   roundTrip(src, dst);

   Vector<U8> srcPack = src.pack();
   Vector<U8> dstPack = dst.pack();
   ASSERT_EQ(srcPack.size(), dstPack.size());
   for(S32 i = 0; i < srcPack.size(); i++)
      EXPECT_EQ(srcPack[i], dstPack[i]) << "pack byte " << i;
}


// ---------------------------------------------------------------------------
// VehicleDesign round-trip tests
// ---------------------------------------------------------------------------

struct VehicleDesignStreamTest : public ::testing::Test
{
   VehicleDesign src;
   VehicleDesign dst;
};


// A default VehicleDesign (from VehicleDesign::reset) survives a round trip.
TEST_F(VehicleDesignStreamTest, DefaultDesign_RoundTrip)
{
   roundTrip(src, dst);
   EXPECT_EQ(src, dst);
}


// A fully-customised design with every field set to non-default values
// survives a round trip.
TEST_F(VehicleDesignStreamTest, FullyCustomised_RoundTrip)
{
   src.body       = XtankBody::Tiger;
   src.engine     = XtankEngine::Fusion;
   src.tread      = XtankTread::HOVER;
   src.armor      = XtankArmor::Titanium;
   src.suspension = XtankSuspension::HEAVY;
   src.bumper     = XtankBumper::RUBBER;
   src.heatSinks  = 7;
   src.specials   = (1u << SPECIAL_RADAR) | (1u << SPECIAL_STEALTH) | (1u << SPECIAL_REPAIR);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      src.weapons[i]      = (XtankWeapon)((S32)XtankWeapon::MACHINE_GUN + i);
      src.weaponMounts[i] = (XtankMountLocation)(i % (S32)XtankMountLocation::COUNT);
   }

   for(S32 i = 0; i < VehicleSidesCount; i++)
      src.armorSides[i] = (i + 1) * 50;   // 50,100,150,200,250,300

   roundTrip(src, dst);
   EXPECT_EQ(src, dst);

   // Spot-check individual fields.
   EXPECT_EQ(dst.body,       XtankBody::Tiger);
   EXPECT_EQ(dst.engine,     XtankEngine::Fusion);
   EXPECT_EQ(dst.tread,      XtankTread::HOVER);
   EXPECT_EQ(dst.armor,      XtankArmor::Titanium);
   EXPECT_EQ(dst.suspension, XtankSuspension::HEAVY);
   EXPECT_EQ(dst.bumper,     XtankBumper::RUBBER);
   EXPECT_EQ(dst.heatSinks,  7);
   EXPECT_TRUE(dst.hasSpecial(SPECIAL_RADAR));
   EXPECT_TRUE(dst.hasSpecial(SPECIAL_STEALTH));
   EXPECT_TRUE(dst.hasSpecial(SPECIAL_REPAIR));
   EXPECT_FALSE(dst.hasSpecial(SPECIAL_HUD));
}


// Boundary values: maximum heat-sink count, all specials set, maximum
// per-side armor, all weapon slots set to NONE / NONE mount.
TEST_F(VehicleDesignStreamTest, BoundaryValues_RoundTrip)
{
   src.heatSinks = MAX_HEAT_SINKS;
   src.specials  = (U32)(MAX_SPECIALS - 1);   // all 12 special bits set

   for(S32 i = 0; i < VehicleSidesCount; i++)
      src.armorSides[i] = MAX_ARMOR_PER_SIDE;

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      src.weapons[i]      = XtankWeapon::NONE;
      src.weaponMounts[i] = XtankMountLocation::NONE;
   }

   roundTrip(src, dst);
   EXPECT_EQ(src, dst);

   EXPECT_EQ(dst.heatSinks, MAX_HEAT_SINKS);
   EXPECT_EQ(dst.specials,  (U32)(MAX_SPECIALS - 1));

   for(S32 i = 0; i < VehicleSidesCount; i++)
      EXPECT_EQ(dst.armorSides[i], MAX_ARMOR_PER_SIDE) << "armorSides[" << i << "]";
}


// Zero heat-sinks, no specials, and zero per-side armor must round-trip
// cleanly.
TEST_F(VehicleDesignStreamTest, ZeroValues_RoundTrip)
{
   src.heatSinks = 0;
   src.specials  = 0;

   for(S32 i = 0; i < VehicleSidesCount; i++)
      src.armorSides[i] = 0;

   roundTrip(src, dst);
   EXPECT_EQ(src, dst);

   EXPECT_EQ(dst.heatSinks, 0);
   EXPECT_EQ(dst.specials,  0u);

   for(S32 i = 0; i < VehicleSidesCount; i++)
      EXPECT_EQ(dst.armorSides[i], 0) << "armorSides[" << i << "]";
}


// Writing src overwrites any non-default content that was already in dst.
TEST_F(VehicleDesignStreamTest, Overwrite_PreExistingDst_RoundTrip)
{
   dst.body      = XtankBody::Rhino;
   dst.engine    = XtankEngine::Fusion;
   dst.heatSinks = 50;

   src.body      = XtankBody::Lightcycle;
   src.engine    = XtankEngine::Small_Electric;
   src.heatSinks = 1;

   roundTrip(src, dst);
   EXPECT_EQ(src, dst);
   EXPECT_EQ(dst.body,      XtankBody::Lightcycle);
   EXPECT_EQ(dst.engine,    XtankEngine::Small_Electric);
   EXPECT_EQ(dst.heatSinks, 1);
}


// Two different designs written end-to-end in one stream, read back in
// order, must each equal their original.
TEST_F(VehicleDesignStreamTest, TwoDesignsSequentialInOneStream)
{
   VehicleDesign a, b, ra, rb;

   a.body    = XtankBody::Psycho;
   a.engine  = XtankEngine::Fuel_Cell;
   a.tread   = XtankTread::SPIKED;
   a.heatSinks = 3;

   b.body    = XtankBody::Malice;
   b.engine  = XtankEngine::Breeder_Fission;
   b.tread   = XtankTread::CHAINED;
   b.heatSinks = 12;

   TNL::BitStream stream;
   a.writeToStream(&stream);
   b.writeToStream(&stream);

   stream.setBitPosition(0);
   ra.readFromStream(&stream);
   rb.readFromStream(&stream);

   EXPECT_EQ(a, ra);
   EXPECT_EQ(b, rb);
   EXPECT_NE(ra, rb);
}


// All weapon slots at extreme valid values survive a round trip.
TEST_F(VehicleDesignStreamTest, AllWeaponSlotsExtreme_RoundTrip)
{
   // Use highest-indexed valid weapon and mount in every slot.
   XtankWeapon      lastWeapon = (XtankWeapon)((S32)XtankWeapon::COUNT - 1);
   XtankMountLocation lastMount = (XtankMountLocation)((S32)XtankMountLocation::COUNT - 1);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      src.weapons[i]      = lastWeapon;
      src.weaponMounts[i] = lastMount;
   }

   roundTrip(src, dst);
   EXPECT_EQ(src, dst);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      EXPECT_EQ(dst.weapons[i],      lastWeapon) << "weapons[" << i << "]";
      EXPECT_EQ(dst.weaponMounts[i], lastMount)  << "weaponMounts[" << i << "]";
   }
}


// Pack/unpack (Vector<U8>) and stream round-trips must be consistent.
TEST_F(VehicleDesignStreamTest, StreamAndPackConsistent)
{
   src.body      = XtankBody::Medusa;
   src.engine    = XtankEngine::Large_Turbine;
   src.heatSinks = 5;
   src.armorSides[0] = 200;
   src.specials      = (1u << SPECIAL_CONSOLE) | (1u << SPECIAL_TACLINK);

   roundTrip(src, dst);

   Vector<U8> srcPack = src.pack();
   Vector<U8> dstPack = dst.pack();
   ASSERT_EQ(srcPack.size(), dstPack.size());
   for(S32 i = 0; i < srcPack.size(); i++)
      EXPECT_EQ(srcPack[i], dstPack[i]) << "pack byte " << i;
}


} // namespace Zap
