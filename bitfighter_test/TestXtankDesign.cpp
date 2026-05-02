//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIXtankHelper.h"   // nextEnum, prevEnum
#include "XtankShape.h"      // XtankDesign, XtankBody, XtankWeapon, XtankArmor,
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

} // namespace Zap
