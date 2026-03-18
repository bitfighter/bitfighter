//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "BfObject.h"

namespace Zap
{

// Derived Object Type conditional methods
bool isEngineeredType(U8 x)
{
   return
         x == TurretTypeNumber || x == ForceFieldProjectorTypeNumber;
}


bool isShipType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber;
}


bool isProjectileType(U8 x)
{
   return
         x == MineTypeNumber  || x == SpyBugTypeNumber      || x == BulletTypeNumber ||
         x == BurstTypeNumber || x == SeekerTypeNumber;
}


bool isGrenadeType(U8 x)
{
   return
         x == MineTypeNumber || x == SpyBugTypeNumber || x == BurstTypeNumber;
}


// Ship::findRepairTargets uses this and expects everything to be a sub-class of Item (except for teleporter)
// This is used to determine if bursts should explode on impact or not.
bool isWithHealthType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber           ||
         x == TurretTypeNumber     || x == ForceFieldProjectorTypeNumber ||
         x == CoreTypeNumber       || x == TeleporterTypeNumber;
}


bool isForceFieldDeactivatingType(U8 x)
{
   return
         x == MineTypeNumber         || x == SpyBugTypeNumber         ||
         x == FlagTypeNumber         || x == SoccerBallItemTypeNumber ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber       ||
         x == EnergyItemTypeNumber   || x == RepairItemTypeNumber     ||
         x == PlayerShipTypeNumber   || x == RobotShipTypeNumber      ||
         x == AsteroidTypeNumber;
}


bool isRadiusDamageAffectableType(U8 x)
{
   return
         x == PlayerShipTypeNumber   || x == RobotShipTypeNumber           || x == BurstTypeNumber      ||
         x == BulletTypeNumber       || x == MineTypeNumber                || x == SpyBugTypeNumber     ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber            || x == AsteroidTypeNumber   ||
         x == TurretTypeNumber       || x == ForceFieldProjectorTypeNumber || x == CoreTypeNumber       ||
         x == FlagTypeNumber         || x == SoccerBallItemTypeNumber      || x == TeleporterTypeNumber ||
         x == SeekerTypeNumber;
}


bool isMotionTriggerType(U8 x)
{
   return
         x == PlayerShipTypeNumber   || x == RobotShipTypeNumber || x == SoccerBallItemTypeNumber ||
         x == ResourceItemTypeNumber || x == TestItemTypeNumber  ||
         x == AsteroidTypeNumber     || x == MineTypeNumber;
}


bool isTurretTargetType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber       || x == ResourceItemTypeNumber ||
         x == TestItemTypeNumber   || x == SoccerBallItemTypeNumber;
}


bool isCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber   ||
         x == TurretTypeNumber  || x == ForceFieldTypeNumber ||
         x == CoreTypeNumber    || x == ForceFieldProjectorTypeNumber;
}


bool isForceFieldCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber || x == PolyWallTypeNumber ||
         x == TurretTypeNumber  || x == ForceFieldProjectorTypeNumber;
}


bool isWallType(U8 x)
{
   return
         x == BarrierTypeNumber  || x == PolyWallTypeNumber ||
         x == WallItemTypeNumber || x == WallEdgeTypeNumber || x == WallSegmentTypeNumber;
}


bool isWallOrForcefieldType(U8 x)
{
   return
      isWallType(x) || x == ForceFieldTypeNumber;
}


bool isWallItemType(U8 x)
{
   return x == WallItemTypeNumber;
}


bool isLineItemType(U8 x)
{
   return
         x == BarrierTypeNumber || x == WallItemTypeNumber || x == LineTypeNumber;
}


bool isWeaponCollideableType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber      || x == BurstTypeNumber               ||
         x == SpyBugTypeNumber     || x == MineTypeNumber           || x == BulletTypeNumber              ||
         x == FlagTypeNumber       || x == SoccerBallItemTypeNumber || x == ForceFieldProjectorTypeNumber ||
         x == AsteroidTypeNumber   || x == TestItemTypeNumber       || x == ResourceItemTypeNumber        ||
         x == TurretTypeNumber     || x == CoreTypeNumber           || x == BarrierTypeNumber             ||
         x == PolyWallTypeNumber   || x == ForceFieldTypeNumber     || x == TeleporterTypeNumber          ||
         x == SeekerTypeNumber;
}


bool isAsteroidCollideableType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber           ||
         x == TestItemTypeNumber   || x == ResourceItemTypeNumber        ||
         x == TurretTypeNumber     || x == ForceFieldProjectorTypeNumber ||
         x == BarrierTypeNumber    || x == PolyWallTypeNumber            ||
         x == ForceFieldTypeNumber || x == CoreTypeNumber;
}


bool isFlagCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber   || x == ForceFieldProjectorTypeNumber || x == ForceFieldTypeNumber || x == PolyWallTypeNumber;
}


bool isFlagOrShipCollideableType(U8 x)
{
   return
         x == BarrierTypeNumber    || x == PolyWallTypeNumber    || x == ForceFieldTypeNumber ||
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber;
}


bool isVisibleOnCmdrsMapType(U8 x)
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber      || x == CoreTypeNumber                ||
         x == BarrierTypeNumber    || x == PolyWallTypeNumber       || x == TextItemTypeNumber            ||
         x == TurretTypeNumber     || x == ForceFieldTypeNumber     || x == ForceFieldProjectorTypeNumber ||
         x == FlagTypeNumber       || x == SoccerBallItemTypeNumber || x == LineTypeNumber                ||
         x == GoalZoneTypeNumber   || x == NexusTypeNumber          || x == LoadoutZoneTypeNumber         ||
         x == SpeedZoneTypeNumber  || x == TeleporterTypeNumber     || x == SlipZoneTypeNumber            ||
         x == AsteroidTypeNumber   || x == TestItemTypeNumber       || x == ResourceItemTypeNumber        ||
         x == EnergyItemTypeNumber || x == RepairItemTypeNumber;
}


bool isVisibleOnCmdrsMapWithSensorType(U8 x)     // Weapons visible on commander's map for sensor
{
   return
         x == PlayerShipTypeNumber || x == RobotShipTypeNumber      || x == ResourceItemTypeNumber        ||
         x == BarrierTypeNumber    || x == PolyWallTypeNumber       || x == LoadoutZoneTypeNumber         ||
         x == TurretTypeNumber     || x == ForceFieldTypeNumber     || x == ForceFieldProjectorTypeNumber ||
         x == FlagTypeNumber       || x == SoccerBallItemTypeNumber || x == SlipZoneTypeNumber            ||
         x == GoalZoneTypeNumber   || x == NexusTypeNumber          || x == CoreTypeNumber                ||
         x == SpeedZoneTypeNumber  || x == TeleporterTypeNumber     || x == BurstTypeNumber               ||
         x == LineTypeNumber       || x == TextItemTypeNumber       || x == RepairItemTypeNumber          ||
         x == AsteroidTypeNumber   || x == TestItemTypeNumber       || x == EnergyItemTypeNumber          ||
         x == BulletTypeNumber     || x == MineTypeNumber           || x == SeekerTypeNumber;
}


bool isZoneType(U8 x)      // Zones a ship could be in
{
   return
         x == LoadoutZoneTypeNumber || x == GoalZoneTypeNumber   || x == NexusTypeNumber  ||
         x == ZoneTypeNumber        || x == SlipZoneTypeNumber;
}


bool isSeekerTarget(U8 x)
{
   return isShipType(x);
}


bool isMountableItemType(U8 x)
{
   return
         x == ResourceItemTypeNumber || x == FlagTypeNumber;
}


bool isAnyObjectType(U8 x)
{
   return true;
}

}