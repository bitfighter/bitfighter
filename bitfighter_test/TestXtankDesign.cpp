//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIXtankHelper.h"   // nextEnum, prevEnum
#include "VehicleDesign.h"   // XtankDesign, XtankBody, XtankWeapon, XtankArmor,
                             // XtankMountLocation, VehicleSides, MAX_ARMOR_PER_SIDE

#include "gtest/gtest.h"

namespace Zap
{

// =============================================================================
// nextEnum / prevEnum
// =============================================================================

TEST(NextPrevEnum, NextWrapsAroundAtCount)
{
   // The value just before COUNT should wrap back to 0.
   XtankArmor last = static_cast<XtankArmor>((S32)XtankArmor::COUNT - 1);
   EXPECT_EQ(static_cast<XtankArmor>(0), nextEnum(last));
}

TEST(NextPrevEnum, PrevWrapsAroundAtZero)
{
   XtankArmor first = static_cast<XtankArmor>(0);
   EXPECT_EQ(static_cast<XtankArmor>((S32)XtankArmor::COUNT - 1), prevEnum(first));
}

TEST(NextPrevEnum, NextStepsForward)
{
   EXPECT_EQ(XtankArmor::Kevlar,          nextEnum(XtankArmor::Steel));
   EXPECT_EQ(XtankArmor::Hardened_Steel,  nextEnum(XtankArmor::Kevlar));
}

TEST(NextPrevEnum, PrevStepsBackward)
{
   EXPECT_EQ(XtankArmor::Steel,  prevEnum(XtankArmor::Kevlar));
   EXPECT_EQ(XtankArmor::Kevlar, prevEnum(XtankArmor::Hardened_Steel));
}

TEST(NextPrevEnum, NextThenPrevIsIdentity)
{
   XtankArmor a = XtankArmor::Composite;
   EXPECT_EQ(a, prevEnum(nextEnum(a)));
}

TEST(NextPrevEnum, PrevThenNextIsIdentity)
{
   XtankArmor a = XtankArmor::Titanium;
   EXPECT_EQ(a, nextEnum(prevEnum(a)));
}

// Works with a different enum type (XtankTread).
TEST(NextPrevEnum, WorksWithXtankTread)
{
   EXPECT_EQ(XtankTread::NORMAL,   nextEnum(XtankTread::SMOOTH));
   EXPECT_EQ(XtankTread::SMOOTH,   prevEnum(XtankTread::NORMAL));

   // Wrap forward: HOVER is last valid value, should wrap to SMOOTH.
   XtankTread last = static_cast<XtankTread>((S32)XtankTread::COUNT - 1);
   EXPECT_EQ(XtankTread::SMOOTH, nextEnum(last));

   // Wrap backward: SMOOTH (0) should wrap to HOVER.
   EXPECT_EQ(last, prevEnum(XtankTread::SMOOTH));
}

// Works with XtankMountLocation.
TEST(NextPrevEnum, WorksWithXtankMountLocation)
{
   EXPECT_EQ(XtankMountLocation::TURRET2, nextEnum(XtankMountLocation::TURRET1));
   EXPECT_EQ(XtankMountLocation::TURRET1, prevEnum(XtankMountLocation::TURRET2));

   XtankMountLocation last = static_cast<XtankMountLocation>((S32)XtankMountLocation::COUNT - 1);
   EXPECT_EQ(static_cast<XtankMountLocation>(0), nextEnum(last));
   EXPECT_EQ(last, prevEnum(static_cast<XtankMountLocation>(0)));
}

// Full round-trip: cycling through all values and back.
TEST(NextPrevEnum, FullCycleForward)
{
   XtankArmor a = XtankArmor::Steel;
   for(S32 i = 0; i < (S32)XtankArmor::COUNT; i++)
      a = nextEnum(a);
   EXPECT_EQ(XtankArmor::Steel, a);  // should be back to start
}

TEST(NextPrevEnum, FullCycleBackward)
{
   XtankArmor a = XtankArmor::Steel;
   for(S32 i = 0; i < (S32)XtankArmor::COUNT; i++)
      a = prevEnum(a);
   EXPECT_EQ(XtankArmor::Steel, a);
}


// =============================================================================
// XtankDesign::nextArmor / previousArmor
// =============================================================================

TEST(XtankDesignArmor, NextArmorStepsForward)
{
   XtankDesign d;
   d.armor = XtankArmor::Steel;
   d.nextArmor();
   EXPECT_EQ(XtankArmor::Kevlar, d.armor);
}

TEST(XtankDesignArmor, PreviousArmorStepsBackward)
{
   XtankDesign d;
   d.armor = XtankArmor::Kevlar;
   d.previousArmor();
   EXPECT_EQ(XtankArmor::Steel, d.armor);
}

TEST(XtankDesignArmor, NextArmorWrapsAtEnd)
{
   XtankDesign d;
   d.armor = static_cast<XtankArmor>((S32)XtankArmor::COUNT - 1);
   d.nextArmor();
   EXPECT_EQ(static_cast<XtankArmor>(0), d.armor);
}

TEST(XtankDesignArmor, PreviousArmorWrapsAtStart)
{
   XtankDesign d;
   d.armor = static_cast<XtankArmor>(0);
   d.previousArmor();
   EXPECT_EQ(static_cast<XtankArmor>((S32)XtankArmor::COUNT - 1), d.armor);
}

TEST(XtankDesignArmor, FullCycleNextArmorReturnsToStart)
{
   XtankDesign d;
   XtankArmor start = d.armor;
   for(S32 i = 0; i < (S32)XtankArmor::COUNT; i++)
      d.nextArmor();
   EXPECT_EQ(start, d.armor);
}

TEST(XtankDesignArmor, FullCyclePreviousArmorReturnsToStart)
{
   XtankDesign d;
   XtankArmor start = d.armor;
   for(S32 i = 0; i < (S32)XtankArmor::COUNT; i++)
      d.previousArmor();
   EXPECT_EQ(start, d.armor);
}


// =============================================================================
// XtankDesign::increaseArmor / reduceArmor
// =============================================================================

TEST(XtankDesignArmorSides, IncreaseArmorBasic)
{
   XtankDesign d;
   d.increaseArmor(VehicleSides::SIDE_FRONT, 10);
   EXPECT_EQ(10, d.armorSides[(S32)VehicleSides::SIDE_FRONT]);
}

TEST(XtankDesignArmorSides, ReduceArmorBasic)
{
   XtankDesign d;
   d.increaseArmor(VehicleSides::SIDE_BACK, 20);
   d.reduceArmor(VehicleSides::SIDE_BACK, 5);
   EXPECT_EQ(15, d.armorSides[(S32)VehicleSides::SIDE_BACK]);
}

TEST(XtankDesignArmorSides, ReduceArmorDoesNotGoBelowZero)
{
   XtankDesign d;
   // armorSides starts at 0; reducing should clamp to 0, not go negative.
   d.reduceArmor(VehicleSides::SIDE_LEFT, 50);
   EXPECT_EQ(0, d.armorSides[(S32)VehicleSides::SIDE_LEFT]);
}

TEST(XtankDesignArmorSides, ReduceArmorExactlyToZero)
{
   XtankDesign d;
   d.increaseArmor(VehicleSides::SIDE_RIGHT, 10);
   d.reduceArmor(VehicleSides::SIDE_RIGHT, 10);
   EXPECT_EQ(0, d.armorSides[(S32)VehicleSides::SIDE_RIGHT]);
}

TEST(XtankDesignArmorSides, IncreaseArmorDoesNotExceedMax)
{
   XtankDesign d;
   d.increaseArmor(VehicleSides::SIDE_FRONT, MAX_ARMOR_PER_SIDE + 1);
   EXPECT_EQ(MAX_ARMOR_PER_SIDE, d.armorSides[(S32)VehicleSides::SIDE_FRONT]);
}

TEST(XtankDesignArmorSides, IncreaseArmorExactlyToMax)
{
   XtankDesign d;
   d.increaseArmor(VehicleSides::SIDE_FRONT, MAX_ARMOR_PER_SIDE);
   EXPECT_EQ(MAX_ARMOR_PER_SIDE, d.armorSides[(S32)VehicleSides::SIDE_FRONT]);
   // One more increase should still be clamped.
   d.increaseArmor(VehicleSides::SIDE_FRONT, 1);
   EXPECT_EQ(MAX_ARMOR_PER_SIDE, d.armorSides[(S32)VehicleSides::SIDE_FRONT]);
}

TEST(XtankDesignArmorSides, IncreaseArmorByDefaultAmount)
{
   XtankDesign d;
   d.increaseArmor(VehicleSides::SIDE_BACK);   // default amount = 1
   EXPECT_EQ(1, d.armorSides[(S32)VehicleSides::SIDE_BACK]);
}

TEST(XtankDesignArmorSides, ReduceArmorByDefaultAmount)
{
   XtankDesign d;
   d.increaseArmor(VehicleSides::SIDE_BACK, 5);
   d.reduceArmor(VehicleSides::SIDE_BACK);     // default amount = 1
   EXPECT_EQ(4, d.armorSides[(S32)VehicleSides::SIDE_BACK]);
}

TEST(XtankDesignArmorSides, SidesAreIndependent)
{
   XtankDesign d;
   d.increaseArmor(VehicleSides::SIDE_FRONT, 10);
   d.increaseArmor(VehicleSides::SIDE_BACK,  20);
   d.reduceArmor(VehicleSides::SIDE_FRONT, 3);

   EXPECT_EQ(7,  d.armorSides[(S32)VehicleSides::SIDE_FRONT]);
   EXPECT_EQ(20, d.armorSides[(S32)VehicleSides::SIDE_BACK]);
   EXPECT_EQ(0,  d.armorSides[(S32)VehicleSides::SIDE_LEFT]);
   EXPECT_EQ(0,  d.armorSides[(S32)VehicleSides::SIDE_RIGHT]);
}

TEST(XtankDesignArmorSides, AllSidesCanReachMax)
{
   XtankDesign d;
   for(S32 i = 0; i < VehicleSidesCount; i++)
   {
      VehicleSides side = static_cast<VehicleSides>(i);
      d.increaseArmor(side, MAX_ARMOR_PER_SIDE);
      EXPECT_EQ(MAX_ARMOR_PER_SIDE, d.armorSides[i]);
   }
}

TEST(XtankDesignArmorSides, AllSidesClampAtZero)
{
   XtankDesign d;
   for(S32 i = 0; i < VehicleSidesCount; i++)
   {
      VehicleSides side = static_cast<VehicleSides>(i);
      d.reduceArmor(side, 9999);
      EXPECT_EQ(0, d.armorSides[i]);
   }
}


// =============================================================================
// XtankDesign::nextMount / previousMount
// =============================================================================

// Lightcycle has 1 turret; the only valid mount for a turret weapon is TURRET1.
TEST(XtankDesignMount, SingleTurretBodyOnlyAllowsTurret1)
{
   XtankDesign d;
   d.body = XtankBody::Lightcycle;
   d.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);

   // Calling nextMount should stay on TURRET1 (no other compatible mount).
   XtankMountLocation before = d.weaponMounts[0];
   d.nextMount(0);
   EXPECT_EQ(before, d.weaponMounts[0]);
}

// Tiger has 2 turrets; nextMount should cycle TURRET1 -> TURRET2 -> TURRET1.
TEST(XtankDesignMount, TwoTurretBodyCyclesTurrets)
{
   XtankDesign d;
   d.body = XtankBody::Tiger;
   d.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);

   d.nextMount(0);
   EXPECT_EQ(XtankMountLocation::TURRET2, d.weaponMounts[0]);

   d.nextMount(0);
   EXPECT_EQ(XtankMountLocation::TURRET1, d.weaponMounts[0]);
}

TEST(XtankDesignMount, TwoTurretBodyPreviousMountCycles)
{
   XtankDesign d;
   d.body = XtankBody::Tiger;
   d.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);

   d.previousMount(0);
   EXPECT_EQ(XtankMountLocation::TURRET2, d.weaponMounts[0]);

   d.previousMount(0);
   EXPECT_EQ(XtankMountLocation::TURRET1, d.weaponMounts[0]);
}

// Panzy has 4 turrets; verify full cycle.
TEST(XtankDesignMount, FourTurretBodyCyclesAllTurrets)
{
   XtankDesign d;
   d.body = XtankBody::Panzy;
   d.setWeapon(0, XtankWeapon::HEAVY_AUTOCANNON, XtankMountLocation::TURRET1);

   d.nextMount(0);
   EXPECT_EQ(XtankMountLocation::TURRET2, d.weaponMounts[0]);
   d.nextMount(0);
   EXPECT_EQ(XtankMountLocation::TURRET3, d.weaponMounts[0]);
   d.nextMount(0);
   EXPECT_EQ(XtankMountLocation::TURRET4, d.weaponMounts[0]);
   d.nextMount(0);
   EXPECT_EQ(XtankMountLocation::TURRET1, d.weaponMounts[0]);
}

// Mine Layer can only go on BACK; nextMount should keep it there.
TEST(XtankDesignMount, BackOnlyWeaponStaysOnBack)
{
   XtankDesign d;
   d.body = XtankBody::Marauder;
   d.setWeapon(0, XtankWeapon::MINE_LAYER, XtankMountLocation::BACK);

   XtankMountLocation before = d.weaponMounts[0];
   d.nextMount(0);
   EXPECT_EQ(before, d.weaponMounts[0]);

   d.previousMount(0);
   EXPECT_EQ(before, d.weaponMounts[0]);
}

// nextMount and previousMount on different slots are independent.
TEST(XtankDesignMount, DifferentSlotsAreIndependent)
{
   XtankDesign d;
   d.body = XtankBody::Tiger;
   d.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);
   d.setWeapon(1, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);

   d.nextMount(0);   // slot 0: TURRET1 -> TURRET2
   EXPECT_EQ(XtankMountLocation::TURRET2, d.weaponMounts[0]);
   EXPECT_EQ(XtankMountLocation::TURRET1, d.weaponMounts[1]);  // unchanged
}

// preferredMount is updated when nextMount / previousMount is called.
TEST(XtankDesignMount, PreferredMountIsUpdated)
{
   XtankDesign d;
   d.body = XtankBody::Tiger;
   d.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);
   d.preferredMounts[0] = XtankMountLocation::TURRET1;

   d.nextMount(0);
   EXPECT_EQ(d.weaponMounts[0], d.preferredMounts[0]);
}


// =============================================================================
// XtankDesign::same()
// =============================================================================

TEST(XtankDesignSame, DefaultDesignsAreEqual)
{
   XtankDesign a, b;
   EXPECT_TRUE(a.same(b));
}

TEST(XtankDesignSame, DifferentBodyIsNotEqual)
{
   XtankDesign a, b;
   b.body = XtankBody::Tiger;
   EXPECT_FALSE(a.same(b));
}

TEST(XtankDesignSame, DifferentEngineIsNotEqual)
{
   XtankDesign a, b;
   b.engine = XtankEngine::Fusion;
   EXPECT_FALSE(a.same(b));
}

TEST(XtankDesignSame, DifferentTreadIsNotEqual)
{
   XtankDesign a, b;
   b.tread = XtankTread::HOVER;
   EXPECT_FALSE(a.same(b));
}

TEST(XtankDesignSame, DifferentArmorIsNotEqual)
{
   XtankDesign a, b;
   b.armor = XtankArmor::Tungsten;
   EXPECT_FALSE(a.same(b));
}

TEST(XtankDesignSame, DifferentSuspensionIsNotEqual)
{
   XtankDesign a, b;
   b.suspension = XtankSuspension::ACTIVE;
   EXPECT_FALSE(a.same(b));
}

TEST(XtankDesignSame, DifferentBumperIsNotEqual)
{
   XtankDesign a, b;
   b.bumper = XtankBumper::RETRO;
   EXPECT_FALSE(a.same(b));
}

TEST(XtankDesignSame, DifferentHeatSinksIsNotEqual)
{
   XtankDesign a, b;
   b.heatSinks = 3;
   EXPECT_FALSE(a.same(b));
}

TEST(XtankDesignSame, DifferentSpecialsIsNotEqual)
{
   XtankDesign a, b;
   b.specials = toggleSpecial(b.specials, SPECIAL_RADAR);
   EXPECT_FALSE(a.same(b));
}

TEST(XtankDesignSame, DifferentArmorSideIsNotEqual)
{
   XtankDesign a, b;
   b.armorSides[(S32)VehicleSides::SIDE_FRONT] = 10;
   EXPECT_FALSE(a.same(b));
}

// Core requirement: same weapons in a different slot order == same design.
TEST(XtankDesignSame, WeaponsInDifferentOrderAreEqual)
{
   XtankDesign a, b;
   a.body = b.body = XtankBody::Tiger;

   a.setWeapon(0, XtankWeapon::MACHINE_GUN,  XtankMountLocation::TURRET1);
   a.setWeapon(1, XtankWeapon::PULSE_LASER,  XtankMountLocation::TURRET2);

   // Same weapons, swapped slots.
   b.setWeapon(0, XtankWeapon::PULSE_LASER,  XtankMountLocation::TURRET2);
   b.setWeapon(1, XtankWeapon::MACHINE_GUN,  XtankMountLocation::TURRET1);

   EXPECT_TRUE(a.same(b));
}

// Same weapon but different mount point => not the same.
TEST(XtankDesignSame, SameWeaponDifferentMountIsNotEqual)
{
   XtankDesign a, b;
   a.body = b.body = XtankBody::Tiger;

   a.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);
   b.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET2);

   EXPECT_FALSE(a.same(b));
}

// One design has a weapon the other doesn't.
TEST(XtankDesignSame, ExtraWeaponIsNotEqual)
{
   XtankDesign a, b;
   a.body = b.body = XtankBody::Tiger;

   a.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);
   // b leaves slot 0 as NONE.

   EXPECT_FALSE(a.same(b));
}

// Duplicate weapon entries in different slots that match each other.
TEST(XtankDesignSame, DuplicateWeaponsSameOrder)
{
   XtankDesign a, b;
   a.body = b.body = XtankBody::Tiger;

   a.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);
   a.setWeapon(1, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET2);

   b.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);
   b.setWeapon(1, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET2);

   EXPECT_TRUE(a.same(b));
}

// Two of the same weapon in swapped slots.
TEST(XtankDesignSame, DuplicateWeaponsSwappedSlotsAreEqual)
{
   XtankDesign a, b;
   a.body = b.body = XtankBody::Tiger;

   a.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);
   a.setWeapon(1, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET2);

   b.setWeapon(0, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET2);
   b.setWeapon(1, XtankWeapon::MACHINE_GUN, XtankMountLocation::TURRET1);

   EXPECT_TRUE(a.same(b));
}

// Fully-loaded Panzy design: 4 weapons in all 4 turrets; shuffled order.
TEST(XtankDesignSame, FullLoadoutShuffledIsEqual)
{
   XtankDesign a, b;
   a.body = b.body = XtankBody::Panzy;
   a.engine = b.engine = XtankEngine::Fusion;
   a.tread  = b.tread  = XtankTread::HOVER;
   a.armor  = b.armor  = XtankArmor::Tungsten;

   // a: slots 0-3 in natural order.
   a.setWeapon(0, XtankWeapon::HEAVY_AUTOCANNON, XtankMountLocation::TURRET1);
   a.setWeapon(1, XtankWeapon::HEAVY_AUTOCANNON, XtankMountLocation::TURRET2);
   a.setWeapon(2, XtankWeapon::HEAT_SEEKER,      XtankMountLocation::TURRET3);
   a.setWeapon(3, XtankWeapon::PULSE_LASER,      XtankMountLocation::TURRET4);

   // b: same weapons but slots shuffled.
   b.setWeapon(0, XtankWeapon::HEAT_SEEKER,      XtankMountLocation::TURRET3);
   b.setWeapon(1, XtankWeapon::HEAVY_AUTOCANNON, XtankMountLocation::TURRET2);
   b.setWeapon(2, XtankWeapon::PULSE_LASER,      XtankMountLocation::TURRET4);
   b.setWeapon(3, XtankWeapon::HEAVY_AUTOCANNON, XtankMountLocation::TURRET1);

   EXPECT_TRUE(a.same(b));
}

// same() is reflexive: a design equals itself.
TEST(XtankDesignSame, ReflexiveEquality)
{
   XtankDesign a;
   a.body = XtankBody::Medusa;
   a.setWeapon(0, XtankWeapon::HEAT_SEEKER, XtankMountLocation::TURRET1);
   a.setWeapon(1, XtankWeapon::HEAT_SEEKER, XtankMountLocation::TURRET2);
   EXPECT_TRUE(a.same(a));
}

// same() is symmetric: a.same(b) == b.same(a).
TEST(XtankDesignSame, SymmetricEquality)
{
   XtankDesign a, b;
   a.body = b.body = XtankBody::Rhino;
   a.setWeapon(0, XtankWeapon::BLAST_CANNON, XtankMountLocation::TURRET1);
   b.setWeapon(0, XtankWeapon::BLAST_CANNON, XtankMountLocation::TURRET1);

   EXPECT_EQ(a.same(b), b.same(a));
}

TEST(XtankDesignSame, SymmetricInequality)
{
   XtankDesign a, b;
   a.body = XtankBody::Rhino;
   b.body = XtankBody::Tiger;

   EXPECT_EQ(a.same(b), b.same(a));
   EXPECT_FALSE(a.same(b));
}


// =============================================================================
// VehicleDesign::pack() / VehicleDesign::unpack()
//
// These tests verify round-trip fidelity: packing a design and then unpacking
// it must reproduce exactly the same field values.
//
// Fixed serialized layout (35 bytes total):
//   body(1) engine(1) tread(1) bumper(1) suspension(1) heatSinks(1) armor(1)
//   armorSides[6] × 2 bytes each = 12
//   weapons[6] + weaponMounts[6]  × 1 byte each  = 12
//   specials                                       = 4
// =============================================================================

namespace
{

// Round-trip helper: pack `original` into a byte vector and unpack into a fresh
// VehicleDesign.
static VehicleDesign packUnpackVD(const VehicleDesign &original)
{
   Vector<U8> bytes;
   original.pack(bytes);
   VehicleDesign result;
   result.unpack(bytes);
   return result;
}

} // anonymous namespace

// The fixed serialized size must match the known layout.
TEST(VehicleDesignPackUnpack, SerializedSizeIsCorrect)
{
   VehicleDesign d;
   Vector<U8> bytes;
   d.pack(bytes);
   // body(1) + engine(1) + tread(1) + bumper(1) + suspension(1) + heatSinks(1) + armor(1)
   // + armorSides(6*2) + weapons+mounts(6*2) + specials(4) = 35
   EXPECT_EQ(35, bytes.size());
}

// pack() calls design.clear() before writing; any prior vector content is discarded.
TEST(VehicleDesignPackUnpack, PackClearsVectorBeforeWriting)
{
   VehicleDesign d;
   d.body = XtankBody::Tiger;

   Vector<U8> bytes;
   bytes.push_back(0xAB);  // pre-existing garbage
   bytes.push_back(0xCD);
   d.pack(bytes);

   EXPECT_EQ(35, bytes.size());  // 35, not 37
}

// A default-constructed design round-trips correctly.
TEST(VehicleDesignPackUnpack, DefaultDesignRoundTrips)
{
   VehicleDesign original;
   VehicleDesign result = packUnpackVD(original);
   EXPECT_TRUE(original.same(result));
}

// VehicleDesign(bytes) constructor produces the same design as default + unpack.
TEST(VehicleDesignPackUnpack, ConstructorFromSerializedMatchesUnpack)
{
   VehicleDesign original;
   original.body       = XtankBody::Tiger;
   original.engine     = XtankEngine::Fusion;
   original.heatSinks  = 5;

   Vector<U8> bytes;
   original.pack(bytes);

   VehicleDesign viaConstructor(bytes);
   VehicleDesign viaUnpack;
   viaUnpack.unpack(bytes);

   EXPECT_TRUE(viaConstructor.same(viaUnpack));
}

// Every valid xtank body (0..COUNT-1) survives the round-trip.
TEST(VehicleDesignPackUnpack, AllNormalBodiesRoundTrip)
{
   for(S32 i = 0; i < (S32)XtankBody::COUNT; i++)
   {
      VehicleDesign d;
      d.body = static_cast<XtankBody>(i);
      VehicleDesign result = packUnpackVD(d);
      EXPECT_EQ(d.body, result.body) << "body index " << i;
   }
}

// Every engine variant round-trips.
TEST(VehicleDesignPackUnpack, AllEnginesRoundTrip)
{
   for(S32 i = 0; i < XtankEngineCount; i++)
   {
      VehicleDesign d;
      d.engine = static_cast<XtankEngine>(i);
      VehicleDesign result = packUnpackVD(d);
      EXPECT_EQ(d.engine, result.engine) << "engine index " << i;
   }
}

// Every tread variant round-trips.
TEST(VehicleDesignPackUnpack, AllTreadsRoundTrip)
{
   for(S32 i = 0; i < XtankTreadCount; i++)
   {
      VehicleDesign d;
      d.tread = static_cast<XtankTread>(i);
      VehicleDesign result = packUnpackVD(d);
      EXPECT_EQ(d.tread, result.tread) << "tread index " << i;
   }
}

// Every armor type round-trips.
TEST(VehicleDesignPackUnpack, AllArmorTypesRoundTrip)
{
   for(S32 i = 0; i < XtankArmorCount; i++)
   {
      VehicleDesign d;
      d.armor = static_cast<XtankArmor>(i);
      VehicleDesign result = packUnpackVD(d);
      EXPECT_EQ(d.armor, result.armor) << "armor index " << i;
   }
}

// Every suspension variant round-trips.
TEST(VehicleDesignPackUnpack, AllSuspensionsRoundTrip)
{
   for(S32 i = 0; i < XtankSuspensionCount; i++)
   {
      VehicleDesign d;
      d.suspension = static_cast<XtankSuspension>(i);
      VehicleDesign result = packUnpackVD(d);
      EXPECT_EQ(d.suspension, result.suspension) << "suspension index " << i;
   }
}

// Every bumper variant round-trips.
TEST(VehicleDesignPackUnpack, AllBumpersRoundTrip)
{
   for(S32 i = 0; i < XtankBumperCount; i++)
   {
      VehicleDesign d;
      d.bumper = static_cast<XtankBumper>(i);
      VehicleDesign result = packUnpackVD(d);
      EXPECT_EQ(d.bumper, result.bumper) << "bumper index " << i;
   }
}

// Various heat-sink counts survive the round-trip.
TEST(VehicleDesignPackUnpack, HeatSinksRoundTrip)
{
   for(S32 n : {0, 1, 5, 10, MAX_HEAT_SINKS})
   {
      VehicleDesign d;
      d.heatSinks = n;
      VehicleDesign result = packUnpackVD(d);
      EXPECT_EQ(n, result.heatSinks) << "heatSinks = " << n;
   }
}

// Weapons and mount locations are preserved in their original slots.
TEST(VehicleDesignPackUnpack, WeaponsAndMountsPreserveSlotOrder)
{
   VehicleDesign d;
   d.body             = XtankBody::Panzy;  // 4 turrets
   d.weapons[0]       = XtankWeapon::LIGHT_MACHINE_GUN;
   d.weaponMounts[0]  = XtankMountLocation::TURRET1;
   d.weapons[1]       = XtankWeapon::PULSE_LASER;
   d.weaponMounts[1]  = XtankMountLocation::TURRET2;
   d.weapons[2]       = XtankWeapon::MINE_LAYER;
   d.weaponMounts[2]  = XtankMountLocation::BACK;
   // Slots 3-5 remain NONE / NONE from clear().

   VehicleDesign result = packUnpackVD(d);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      EXPECT_EQ(d.weapons[i],      result.weapons[i])      << "weapons["      << i << "]";
      EXPECT_EQ(d.weaponMounts[i], result.weaponMounts[i]) << "weaponMounts[" << i << "]";
   }
}

// All weapon slots empty (NONE weapon, NONE mount) round-trips without corruption.
TEST(VehicleDesignPackUnpack, AllWeaponSlotsEmptyRoundTrip)
{
   VehicleDesign d;  // clear() sets all slots to NONE

   VehicleDesign result = packUnpackVD(d);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      EXPECT_EQ(XtankWeapon::NONE,        result.weapons[i])      << "slot " << i;
      EXPECT_EQ(XtankMountLocation::NONE, result.weaponMounts[i]) << "slot " << i;
   }
}

// All six weapon slots filled with a real weapon round-trip correctly.
TEST(VehicleDesignPackUnpack, AllWeaponSlotsFilledRoundTrip)
{
   VehicleDesign d;
   d.body = XtankBody::Panzy;  // 4 turrets
   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      d.weapons[i]      = XtankWeapon::MACHINE_GUN;
      d.weaponMounts[i] = XtankMountLocation::TURRET1;
   }

   VehicleDesign result = packUnpackVD(d);

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      EXPECT_EQ(XtankWeapon::MACHINE_GUN,    result.weapons[i])      << "slot " << i;
      EXPECT_EQ(XtankMountLocation::TURRET1, result.weaponMounts[i]) << "slot " << i;
   }
}

// Zero armor on every side round-trips to zero.
TEST(VehicleDesignPackUnpack, ArmorSidesAllZeroRoundTrip)
{
   VehicleDesign d;  // armorSides initialised to 0 by clear()

   VehicleDesign result = packUnpackVD(d);

   for(S32 i = 0; i < VehicleSidesCount; i++)
      EXPECT_EQ(0, result.armorSides[i]) << "side " << i;
}

// MAX_ARMOR_PER_SIDE (999) requires two bytes per side; the hi/lo split must
// reconstruct the original value exactly.
TEST(VehicleDesignPackUnpack, ArmorSidesAtMaximumRoundTrip)
{
   VehicleDesign d;
   for(S32 i = 0; i < VehicleSidesCount; i++)
      d.armorSides[i] = MAX_ARMOR_PER_SIDE;

   VehicleDesign result = packUnpackVD(d);

   for(S32 i = 0; i < VehicleSidesCount; i++)
      EXPECT_EQ(MAX_ARMOR_PER_SIDE, result.armorSides[i]) << "side " << i;
}

// Different values on each side are stored and retrieved independently.
TEST(VehicleDesignPackUnpack, ArmorSidesVariedRoundTrip)
{
   VehicleDesign d;
   d.armorSides[(S32)VehicleSides::SIDE_FRONT]  = 10;
   d.armorSides[(S32)VehicleSides::SIDE_BACK]   = 200;
   d.armorSides[(S32)VehicleSides::SIDE_LEFT]   = 0;
   d.armorSides[(S32)VehicleSides::SIDE_RIGHT]  = 999;
   d.armorSides[(S32)VehicleSides::SIDE_TOP]    = 512;
   d.armorSides[(S32)VehicleSides::SIDE_BOTTOM] = 1;

   VehicleDesign result = packUnpackVD(d);

   EXPECT_EQ(10,  result.armorSides[(S32)VehicleSides::SIDE_FRONT]);
   EXPECT_EQ(200, result.armorSides[(S32)VehicleSides::SIDE_BACK]);
   EXPECT_EQ(0,   result.armorSides[(S32)VehicleSides::SIDE_LEFT]);
   EXPECT_EQ(999, result.armorSides[(S32)VehicleSides::SIDE_RIGHT]);
   EXPECT_EQ(512, result.armorSides[(S32)VehicleSides::SIDE_TOP]);
   EXPECT_EQ(1,   result.armorSides[(S32)VehicleSides::SIDE_BOTTOM]);
}

// specials = 0 (no special equipment) round-trips to 0.
TEST(VehicleDesignPackUnpack, SpecialsNoneRoundTrip)
{
   VehicleDesign d;
   d.specials = 0;

   VehicleDesign result = packUnpackVD(d);
   EXPECT_EQ(0u, result.specials);
}

// All 12 defined XtankSpecial bits set round-trip correctly.
TEST(VehicleDesignPackUnpack, SpecialsAllDefinedBitsRoundTrip)
{
   U32 allDefined = 0;
   for(S32 i = 0; i < XtankSpecialCount; i++)
      allDefined |= (1u << i);

   VehicleDesign d;
   d.specials = allDefined;

   VehicleDesign result = packUnpackVD(d);
   EXPECT_EQ(allDefined, result.specials);
}

// specials uses a full U32 (4 bytes); all 32 bits must survive the round-trip.
TEST(VehicleDesignPackUnpack, SpecialsAllBitsSetRoundTrip)
{
   VehicleDesign d;
   d.specials = 0xFFFFFFFFu;

   VehicleDesign result = packUnpackVD(d);
   EXPECT_EQ(0xFFFFFFFFu, result.specials);
}

// Individual special bits in each byte position of the U32 survive independently.
TEST(VehicleDesignPackUnpack, SpecialsByteBoundariesRoundTrip)
{
   const U32 kTestValues[] = { 0x00000001u, 0x00000080u,  // byte 0
                               0x00000100u, 0x00008000u,  // byte 1
                               0x00010000u, 0x00800000u,  // byte 2
                               0x01000000u, 0x80000000u }; // byte 3
   for(U32 mask : kTestValues)
   {
      VehicleDesign d;
      d.specials = mask;
      VehicleDesign result = packUnpackVD(d);
      EXPECT_EQ(mask, result.specials) << "mask 0x" << std::hex << mask;
   }
}

// A fully-configured design with every field set to a non-default value
// round-trips intact, checked field-by-field.
TEST(VehicleDesignPackUnpack, FullyConfiguredDesignRoundTrips)
{
   VehicleDesign d;
   d.body       = XtankBody::Panzy;
   d.engine     = XtankEngine::Fusion;
   d.tread      = XtankTread::HOVER;
   d.armor      = XtankArmor::Tungsten;
   d.suspension = XtankSuspension::ACTIVE;
   d.bumper     = XtankBumper::RETRO;
   d.heatSinks  = 7;
   d.specials   = 0x00000FFFu;  // all 12 defined specials

   for(S32 i = 0; i < VehicleSidesCount; i++)
      d.armorSides[i] = (i + 1) * 100;  // 100, 200, 300, 400, 500, 600

   d.weapons[0]      = XtankWeapon::LIGHT_MACHINE_GUN;  d.weaponMounts[0] = XtankMountLocation::TURRET1;
   d.weapons[1]      = XtankWeapon::HEAVY_AUTOCANNON;   d.weaponMounts[1] = XtankMountLocation::TURRET2;
   d.weapons[2]      = XtankWeapon::MINE_LAYER;         d.weaponMounts[2] = XtankMountLocation::BACK;
   d.weapons[3]      = XtankWeapon::PULSE_LASER;        d.weaponMounts[3] = XtankMountLocation::TURRET3;
   d.weapons[4]      = XtankWeapon::HEAT_SEEKER;        d.weaponMounts[4] = XtankMountLocation::TURRET4;
   d.weapons[5]      = XtankWeapon::MACHINE_GUN;        d.weaponMounts[5] = XtankMountLocation::FRONT;

   VehicleDesign result = packUnpackVD(d);

   EXPECT_EQ(d.body,       result.body);
   EXPECT_EQ(d.engine,     result.engine);
   EXPECT_EQ(d.tread,      result.tread);
   EXPECT_EQ(d.armor,      result.armor);
   EXPECT_EQ(d.suspension, result.suspension);
   EXPECT_EQ(d.bumper,     result.bumper);
   EXPECT_EQ(d.heatSinks,  result.heatSinks);
   EXPECT_EQ(d.specials,   result.specials);

   for(S32 i = 0; i < VehicleSidesCount; i++)
      EXPECT_EQ(d.armorSides[i], result.armorSides[i]) << "armorSides[" << i << "]";

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      EXPECT_EQ(d.weapons[i],      result.weapons[i])      << "weapons["      << i << "]";
      EXPECT_EQ(d.weaponMounts[i], result.weaponMounts[i]) << "weaponMounts[" << i << "]";
   }
}

// same() returns true after a round-trip (integration with same()).
TEST(VehicleDesignPackUnpack, SameReturnsTrueAfterRoundTrip)
{
   VehicleDesign d;
   d.body      = XtankBody::Medusa;
   d.engine    = XtankEngine::Fission;
   d.tread     = XtankTread::CHAINED;
   d.heatSinks = 3;
   d.weapons[0]     = XtankWeapon::BLAST_CANNON;
   d.weaponMounts[0] = XtankMountLocation::TURRET1;

   VehicleDesign result = packUnpackVD(d);
   EXPECT_TRUE(d.same(result));
}

// Packing the same design twice produces identical byte vectors.
TEST(VehicleDesignPackUnpack, PackIsDeterministic)
{
   VehicleDesign d;
   d.body    = XtankBody::Tiger;
   d.engine  = XtankEngine::Large_Turbine;
   d.specials = 0xDEADBEEFu;

   Vector<U8> bytes1, bytes2;
   d.pack(bytes1);
   d.pack(bytes2);

   ASSERT_EQ(bytes1.size(), bytes2.size());
   for(S32 i = 0; i < bytes1.size(); i++)
      EXPECT_EQ(bytes1[i], bytes2[i]) << "byte " << i;
}

} // namespace Zap
