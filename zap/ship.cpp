//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ship.h"

#include "projectile.h"
#include "gameType.h"
#include "Zone.h"
#include "Colors.h"
#include "Teleporter.h"
#include "speedZone.h"
#include "safeZone.h"
#include "reloadZone.h"
#include "fuelZone.h"
#include "repairZone.h"
#include "VehicleDesign.h"    // For XtankBody enum, body_stat, xtankEngineInfos, xtankTreadInfos, etc.


#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "UIManager.h"
#endif

#include "gameObjectRender.h"

#include "stringUtils.h"   // For itos
#include "MathUtils.h"     // For radiansToDegrees
#include "GeomUtils.h"
#include "Timer.h"


#ifdef TNL_OS_WIN32
#  include <windows.h>     // For ARRAYSIZE
#endif

#define hypot _hypot    // Kill some warnings


namespace Zap
{
   static bool isXtankBodyMountAvailable(XtankBody bodyIndex, XtankMountLocation mount)
   {
	  if(mount >= XtankMountLocation::TURRET1 && mount <= XtankMountLocation::TURRET4)
	  {
		 S32 turretIdx = (S32)mount - (S32)XtankMountLocation::TURRET1;
		 return turretIdx < xtankTurretInfos[(S32)bodyIndex].count;
	  }

	  return mount == XtankMountLocation::FRONT || mount == XtankMountLocation::BACK || mount == XtankMountLocation::LEFT || mount == XtankMountLocation::RIGHT;
   }


   static bool isXtankWeaponMountCompatible(XtankBody bodyIndex, XtankWeapon weapon, XtankMountLocation mount)
   {
	  if(!isXtankBodyMountAvailable(bodyIndex, mount))
		 return false;

	  const XtankWeaponInfo &wi = xtankWeaponInfos[(S32)weapon];
	  return (wi.legalMounts & getXtankMountBit(mount)) != 0;
   }


   static Point getXtankMountPointBodySpace(XtankBody body, XtankMountLocation mount)
   {
	  if(body != XtankBody::BITFIGHTER_SHIP &&
		 mount >= XtankMountLocation::TURRET1 && mount <= XtankMountLocation::TURRET4)
	  {
		 S32 bodyIndex = (S32)body;
		 S32 turretIdx = (S32)mount - (S32)XtankMountLocation::TURRET1;
		 if(turretIdx < xtankTurretInfos[bodyIndex].count)
		 {
			const XtankTurret &turret = xtankTurretInfos[bodyIndex].turrets[turretIdx];
			return Point(turret.x, turret.y);
		 }
	  }

	  const F32 yFront = (F32)Ship::CollisionRadius * 0.85f;
	  const F32 yBack = (F32)Ship::CollisionRadius * -0.85f;
	  const F32 xSide = (F32)Ship::CollisionRadius * 0.75f;
	  const F32 yMid = 0;

	  switch(mount)
	  {
	  case XtankMountLocation::FRONT: return Point(0, yFront);
	  case XtankMountLocation::BACK:  return Point(0, yBack);
	  case XtankMountLocation::LEFT:  return Point(-xSide, yMid);
	  case XtankMountLocation::RIGHT: return Point(xSide, yMid);
	  default:          return Point(0, 0);
	  }
   }

   TNL_IMPLEMENT_NETOBJECT(Ship);

#ifdef _MSC_VER
#  pragma warning(disable:4355)
#endif


   // Constructor
   // Note that on client, we use all default values set in declaration; on the server, these values will be provided
   // Most of these values are set in the initial packet set from the server (see packUpdate() below)
   // Also, the following is also run by robot's constructor
   Ship::Ship(ClientInfo *clientInfo, S32 team, const Point &pos, bool isRobot) : MoveObject(pos, (F32)CollisionRadius), mSpawnPoint(pos)
   {
	  initialize(clientInfo, team, pos, isRobot);
   }


   // Combined Lua / C++ default constructor -- this is used by Lua and by TNL, so we need to programatically separate the two
   Ship::Ship(lua_State *L) : MoveObject(Point(0, 0), (F32)CollisionRadius)
   {
	  if(L)
	  {
		 luaL_error(L, "Currently cannot instantiate a Ship object from Lua.");
		 return;
	  }

	  initialize(NULL, TEAM_NEUTRAL, Point(0, 0), false);
   }


   // Destructor
   Ship::~Ship()
   {
	  dismountAll();

	  // It may be that mClientInfo has already been assigned a new ship; we only want to NULL it out
	  // in the event it hasn't.  At the end of a game, mClientInfo may disappear before the ship does.
	  if(mClientInfo && mClientInfo->getShip() == this)
		 mClientInfo->setShip(NULL);   // Don't leave a dangling pointer

	  LUAW_DESTRUCTOR_CLEANUP;
   }


   void Ship::initialize(ClientInfo *clientInfo, S32 team, const Point &pos, bool isRobot)
   {
	  // Note that clientInfo can be NULL if coming via Lua constructor
	  static const U32 ModuleSecondaryTimerDelay = 500;
	  static const U32 SpyBugPlacementTimerDelay = 800;
	  static const U32 IdleRechargeCycleTimerDelay = 2000;

	  mObjectTypeNumber = PlayerShipTypeNumber;
	  mFireTimer = 0;
	  mFastRecharging = false;
	  mLastProcessStateAngle = 0;

	  mEngineeredTeleporter = NULL;

	  // Set up module secondary delay timer
	  for(S32 i = 0; i < ModuleCount; i++)
		 mModuleSecondaryTimer[i].setPeriod(ModuleSecondaryTimerDelay);

	  mSpyBugPlacementTimer.setPeriod(SpyBugPlacementTimerDelay);
	  mSensorEquipZoomTimer.setPeriod(SensorZoomTime);
	  mFastRechargeTimer.reset(IdleRechargeCycleTimerDelay, IdleRechargeCycleTimerDelay);
	  mSendSpawnEffectTimer.setPeriod(300);

	  mNetFlags.set(Ghostable);

#ifndef ZAP_DEDICATED
	  for(U32 i = 0; i < TrailCount; i++)
		 mLastTrailPoint[i] = -1;   // Or something... doesn't really matter what
#endif

	  mClientInfo = clientInfo;     // Will be NULL if being created by TNL

	  if(mClientInfo)
		 mClientInfo->setShip(this);

	  setTeam(team);
	  mass = 1.0;            // Ship's mass, not used

	  // Name will be unique across all clients, but client and server may disagree on this name if the server has modified it to make it unique

	  mIsRobot = isRobot;

	  if(!isRobot)               // Robots will run this during their own initialization; no need to run it twice!
		 initialize(pos);
	  else
		 mHasExploded = false;    // Client needs this false for unpackUpdate

#ifndef ZAP_DEDICATED
	  mSparkElapsed = 0;
	  mShapeType = ShipShape::Normal;
#endif

	  // Tank physics state — present in all builds (server runs tank physics)
	  // Use the active (old) design for respawn, or fall back to on-deck for first spawn
	  if(clientInfo)
		 mVehicleDesign = *clientInfo->getOldDesign();
	  mTankHeadingAngle = -FloatHalfPi;  // Default: hull facing north (up on screen)
	  mDesiredHeading   = NO_HULL_ANGLE_REQUESTED;        // No pending turn request
	  mTankSpeed = 0;
	  mSpeedFraction = 0.0f;             // Start stopped
	  mSafety = false;                   // Safety off by default (matches XTank default)

	  // Xtank ammo/heat runtime state
	  mHeat = 0.0f;
	  mMoney = STARTING_MONEY;
	  mLoadout.set(DefaultLoadout);

	  // Save armor from the real design before reset() zeroes armorSides.
	  // mArmorSides uses a 100× internal tick scale to allow sub-point damage accumulation.
	  array<S32, VehicleSidesCount> savedArmorSides;
	  for(S32 i = 0; i < VehicleSidesCount; i++)
		 savedArmorSides[i] = mVehicleDesign.armorSides[i];

	  mVehicleDesign.reset();
	  initXtankWeaponStates();   // Must come after mVehicleDesign is fully settled

	  // Fuel: initialize from engine fcap
	  {
		 const S32 engineIdx = MAX(0, MIN((S32)mVehicleDesign.engine, XtankEngineCount - 1));
		 mMaxFuel = (F32)xtankEngineInfos[engineIdx].fcap;
		 if(mMaxFuel <= 0) mMaxFuel = 100.0f;  // safe fallback for default/unset design
		 mFuel = mMaxFuel;
	  }
	  mWasInFuelZone = false;

	  // Armor: initialize per-side HP (×100 ticks) from design maximums saved before reset().
	  // Also restore armorSides onto mVehicleDesign so pack/unpack max ranges are correct.
	  for(S32 i = 0; i < VehicleSidesCount; i++)
	  {
		 mVehicleDesign.armorSides[i] = savedArmorSides[i];
		 mArmorSides[i] = savedArmorSides[i] * 100;
	  }
	  mWasInRepairZone = false;
		  mRepairAccumMs = 0.0f;
		  if(clientInfo && clientInfo->getOldLoadout()->getModule(0) != ModuleNone)
		 mLoadout = *clientInfo->getOldLoadout();
	  // TODO: Probably need something for xtank here

	  LUAW_CONSTRUCTOR_INITIALIZATIONS;
   }


   Ship *Ship::clone() const
   {
	  return new Ship(*this);
   }


   // Compare a client copy of a ship to the server copy and see if they are "equal"
   // Only used for tests
   bool Ship::isServerCopyOf(const Ship &clientShip) const
   {
	  if(mLoadout != clientShip.mLoadout)
		 return false;

	  if(mHealth != clientShip.mHealth || mEnergy != clientShip.mEnergy || getTeam() != clientShip.getTeam())
		 return false;

	  // Server sends renderPos/vel, client stores those in actualPos/vel
	  if(getRenderPos() != clientShip.getActualPos() || getRenderVel() != clientShip.getRenderVel())
		 return false;

	  if(!mCurrentMove.isEqualMove(&clientShip.mCurrentMove))
		 return false;

	  if(mMountedItems.size() != clientShip.mMountedItems.size())
		 return false;

	  for(S32 i = 0; i < mMountedItems.size(); i++)
		 if(mMountedItems[i]->getObjectTypeNumber() != clientShip.mMountedItems[i]->getObjectTypeNumber())
			return false;

	  // Compare xtank state so we can catch heading/speed sync failures in tests
	  if(mVehicleDesign != clientShip.mVehicleDesign)
		 return false;

	  if(isXtankVehicle())
	  {
		 if(fabsf(mTankHeadingAngle - clientShip.mTankHeadingAngle) > 0.01f)
			return false;
		 if(fabsf(mTankSpeed - clientShip.mTankSpeed) > 1.0f)
			return false;
	  }

	  return true;
   }


   // Initialize some things that both ships and bots care about... this will get run during the ship's constructor
   // and also after a bot respawns and needs to reset itself
   void Ship::initialize(const Point &pos)
   {
	  // Does this ever evaluate to true?
	  if(getGame())
		 mSendSpawnEffectTimer.reset();

	  setPosVelAng(pos, Point(0, 0), 0);

	  updateExtentInDatabase();

	  mHealth = 1.0;       // Start at full health
	  mHasExploded = false; // Haven't exploded yet!

#ifndef ZAP_DEDICATED
	  for(S32 i = 0; i < TrailCount; i++)          // Clear any vehicle trails
		 mTrail[i].reset();
#endif

	  mEnergy = (S32)((F32)EnergyMax * .80);     // Start off with 80% energy

	  mCooldownNeeded = false;

	  // Start spawn shield timer
	  mSpawnShield.reset(SpawnShieldTime);
   }


   bool Ship::processArguments(S32 argc, const char **argv, Game *game)
   {
	  if(argc != 3)
		 return false;

	  Point pos;
	  pos.read(argv + 1);
	  pos *= game->getLegacyGridSize();
	  for(U32 i = 0; i < MoveStateCount; i++)
	  {
		 setPos(i, pos);
		 setAngle(i, 0);
	  }

	  return true;
   }


   string Ship::toLevelCode() const
   {
	  return string(getClassName()) + " " + itos(getTeam()) + " " + geomToLevelCode();
   }


   ClientInfo *Ship::getClientInfo() const
   {
	  return mClientInfo;
   }


   bool Ship::canAddToEditor() { return false; }      // No ships in the editor
   const char *Ship::getOnScreenName() { return "Ship"; }


   void Ship::setEngineeredTeleporter(Teleporter *teleporter)
   {
	  mEngineeredTeleporter = teleporter;
   }


   Teleporter *Ship::getEngineeredTeleporter()
   {
	  return mEngineeredTeleporter;
   }


   void Ship::onGhostRemove()
   {
	  Parent::onGhostRemove();
	  mLoadout.deactivateAllModules();
	  updateModuleSounds();
   }


   F32 Ship::getHealth() const
   {
	  return mHealth;
   }


   S32 Ship::getEnergy() const
   {
	  return mEnergy;
   }


   void Ship::setActualPos(const Point &p, bool warp)
   {
	  Parent::setActualPos(p);
	  Parent::setRenderPos(p);

	  if(warp)
		 setMaskBits(PositionMask | WarpPositionMask | TeleportMask);
	  else
		 setMaskBits(PositionMask);
   }


   // Process a move.  This will advance the position of the ship, as well as adjust its velocity and angle.
   F32 Ship::processMove(U32 stateIndex)
   {
	  mLastProcessStateAngle = getAngle(stateIndex);
	  setAngle(stateIndex, mCurrentMove.angle);


	  // Route to the appropriate physics model
	  if(isXtankVehicle())
		 return processTankMove(stateIndex);
	  else
		 return processShipMove(stateIndex);
   }

   
   // BF only
   F32 Ship::processShipMove(U32 stateIndex)
   {
	  static const F32 NORMAL_ACCEL_FACT = 1.0f;
	  static const F32 ARMOR_ACCEL_FACT = 0.35f;

	  static const F32 NORMAL_SPEED_FACT = 1.0f;
	  static const F32 ARMOR_SPEED_FACT = 1.0f;

	  // -----------------------------------------------------------------------
	  // Standard Bitfighter ship physics below
	  // -----------------------------------------------------------------------

	  // Nothing to do when ship is not moving, Continue to check for SpeedZones
	  if(mCurrentMove.x == 0 && mCurrentMove.y == 0 && getVel(stateIndex) == Point(0, 0))
	  {
		 if(!checkForSpeedzones(stateIndex))
			return 0;
	  }

	  F32 maxVel = (mLoadout.isModulePrimaryActive(ModuleBoost) ? BoostMaxVelocity : MaxVelocity) *
		 (hasModule(ModuleArmor) ? ARMOR_SPEED_FACT : NORMAL_SPEED_FACT);

	  F32 time = mCurrentMove.time * 0.001f;

	  static Point requestVel, accel;     // Reusable containers

	  // This is what the client requested -- basically requestVel.len() will range from 0 to 1; any higher will be clipped
	  requestVel.set(mCurrentMove.x, mCurrentMove.y);
	  requestVel *= maxVel;

	  static const S32 MAX_CONTROLLABLE_SPEED = 1000;

	  // If you are going too fast (i.e. > MAX_CONTROLLABLE_SPEED), you cannot move, and will automatically
	  // "hit the brakes" by requesting a speed of 0
	  if(getVel(stateIndex).lenSquared() > sq(MAX_CONTROLLABLE_SPEED))
		 requestVel.set(0, 0);

	  // Limit requestVel to maxVel (but can be lower)
	  if(requestVel.lenSquared() > sq(maxVel))
		 requestVel.normalize(maxVel);

	  // a  = requested vel - current vel
	  accel = requestVel - getVel(stateIndex);

	  // Increase acceleration when turbo-boost is active, reduce it when armor is present
	  F32 maxAccel = Acceleration * time *                                                // Standard accel, modified by:
		 (mLoadout.isModulePrimaryActive(ModuleBoost) ? BoostAccelFact : 1) * // Boost
		 (hasModule(ModuleArmor) ? ARMOR_ACCEL_FACT : NORMAL_ACCEL_FACT) *    // Armor
		 getGame()->getShipAccelModificationFactor(this);                     // Slip zones

	  // If you are requesting a lower accel than the max, you get it instantly... else you are limited to max
	  if(accel.lenSquared() <= sq(maxAccel))
		 setVel(stateIndex, requestVel);
	  else
	  {
		 accel.normalize(maxAccel);
		 setVel(stateIndex, getVel(stateIndex) + accel);
	  }

	  return move(time, stateIndex, false);
   }



   // Tank driving physics for xtank vehicle bodies.
   //
   // Implements the update_vector() algorithm from the original xtank game
   // (xtank-master/Src/update.c), adapted for Bitfighter's continuous-time
   // simulation.  Derived stats (max_speed, engine_acc, tread_acc, handling)
   // are computed from the original xtank component tables using the formulas
   // from xtank-master/Src/vdesign.c (compute_vdesc).
   //
   // Key behaviors:
   //   - Weight-dependent acceleration (engine power vs. total vehicle mass)
   //   - Traction-limited turning and acceleration (skidding on grip loss)
   //   - Roll/slide velocity decomposition (lateral drift when turning at speed)
   //   - Dynamic friction < static friction (0.7x when already sliding)
   //   - Engine-assisted braking
   //
   // Input mapping (same Move fields, new meaning):
   //   move.y: (BINDING_DOWN - BINDING_UP); W gives -1, S gives +1
   //           => throttle = -move.y   (W = forward = +1, S = backward = -1)
   //   move.x: (BINDING_RIGHT - BINDING_LEFT); D gives +1, A gives -1
   //           => steer    =  move.x   (D = turn right, A = turn left)
   //   move.angle: turret/aim direction (unchanged)
   //
   // The hull faces mTankHeadingAngle; the turret still tracks the aim angle.
   // XT only
   F32 Ship::processTankMove(U32 stateIndex)
   {
	  const S32 engineIdx = MAX(0, MIN((S32)mVehicleDesign.engine, XtankEngineCount - 1));
	  const S32 treadIdx = MAX(0, MIN((S32)mVehicleDesign.tread, XtankTreadCount - 1));
	  const S32 suspensionIdx = MAX(0, MIN((S32)mVehicleDesign.suspension, XtankSuspensionCount - 1));

	  const XtankBodyInfo2 &body = xTankBodyStats[(S32)mVehicleDesign.body];
	  const XtankEngineInfo &engine = xtankEngineInfos[engineIdx];
	  const XtankTreadInfo &tread = xtankTreadInfos[treadIdx];
	  const SuspensionStat &suspensionInfo = suspensionStat[suspensionIdx];

	  F32 dt = mCurrentMove.time * 0.001f;
	  if(dt <= 0)
		 return 0;

	  S32 totalWeight = mVehicleDesign.getWeight();

	  // --- Xtank derived physics (per-frame units, from vdesign.c compute_vdesc) ---
	  F32 power = (F32)engine.power;
	  F32 drag = body.drag;
	  F32 treadFric = tread.friction;

	  // max_speed = cbrt(power / drag) / friction   [xtank units/frame]
	  F32 xt_max_speed = powf(power / drag, 1.0f / 3.0f) / treadFric;
	  if(mVehicleDesign.isHover())
		 xt_max_speed *= 0.5f;

	  // engine_acc = 16 * power / weight   [xtank dv/frame from engine]
	  F32 xt_engine_acc = 16.0f * power / (F32)totalWeight;

	  // tread_acc = friction * MAX_ACCEL   [xtank dv/frame from traction]
	  // Weight drops out because traction is proportional to weight.
	  F32 xt_tread_acc = treadFric * (F32)MAX_ACCEL;

	  // max_turn_rate = handling / 8.0     [radians/frame, for snap-to-heading]
	  // In xtank, vehicles rotate toward a mouse-set desired_heading and stop.
	  // With BF's continuous WASD rotation we need a much lower rate.  We keep
	  // the xtank handling ratio (light=fast, heavy=slow) but scale to a range
	  // that feels right for hold-to-spin: ~3.5 rad/s for handling=8 (Lightcycle)
	  // down to ~1.3 rad/s for handling=3 (Rhino/Panzy).
	  static const F32 TURN_SCALE = 3.5f;  // target rad/s for handling/8 = 1.0
	  // suspensionInfo.friction is expected to stay in a small range [-1.0, 2.0].
	  TNLAssert(suspensionInfo.friction >= -1.0f && suspensionInfo.friction <= 2.0f,
		 "Unexpected suspension handling modifier");
	  static const F32 MIN_EFFECTIVE_HANDLING = 1.0f;

	  // body.handling values are currently 3..8; this lower bound keeps turn math stable
	  // even if future data pushes handling lower.
	  F32 bodyHandling = (F32)body.handling;
	  F32 effectiveHandling = MAX(MIN_EFFECTIVE_HANDLING, bodyHandling + suspensionInfo.friction);
	  F32 xt_max_turn = effectiveHandling / 8.0f;

	  // --- Convert xtank per-frame units to BF per-second units ---
	  // Xtank runs at ~20 fps.  BF_SCALE maps xtank distance to BF distance.
	  static const F32 XTANK_FPS = 20.0f;
	  static const F32 BF_SCALE = 1.5f;

	  F32 maxSpeed = xt_max_speed * XTANK_FPS * BF_SCALE;                     // BF units/sec
	  F32 engineAcc = xt_engine_acc * XTANK_FPS * XTANK_FPS * BF_SCALE;         // BF units/sec^2
	  F32 treadAcc = xt_tread_acc * XTANK_FPS * XTANK_FPS * BF_SCALE;         // BF units/sec^2
	  F32 maxTurnRate = xt_max_turn * TURN_SCALE;                                // radians/sec

	  // --- Input mapping ---
	  // mCurrentMove.speedFraction is the cruise-control throttle set by the speed keys (packed into Move).
	  // The vehicle drives at that fraction of max speed; WASD forward/back is ignored.
	  F32 throttle = mCurrentMove.speedFraction;

	  // --- Fuel: enforce empty-tank cutout ---
	  // When out of fuel the engine cuts out completely (spec §4).
	  if(mFuel <= 0.0f)
		 throttle = 0.0f;

	  F32 steer = mCurrentMove.x;   // +1 = clockwise, -1 = counter-clockwise

	  // When out of fuel, turning is also blocked (spec §4).
	  if(mFuel <= 0.0f)
		 steer = 0.0f;

	  // --- Get current velocity ---
	  Point vel = getVel(stateIndex);
	  F32 speed = vel.len();
	  F32 moveAngle = (speed > 0.1f) ? atan2f(vel.y, vel.x) : mTankHeadingAngle;

	  // --- Early exit when completely idle ---
	  // Also update mDesiredHeading and mSafety before the check.
	  if(mCurrentMove.hullAngle != NO_HULL_ANGLE_REQUESTED)
		 mDesiredHeading = mCurrentMove.hullAngle;

	  mSafety = mCurrentMove.safety;   // Server adopts client's safety toggle each tick

	  if(throttle == 0 && steer == 0 && speed < 0.1f && mDesiredHeading == NO_HULL_ANGLE_REQUESTED)
	  {
		 setVel(stateIndex, Point(0, 0));
		 mTankSpeed = 0;
		 if(!checkForSpeedzones(stateIndex))
			return 0;
	  }

	  // effectiveTurnRate: start at max, then clamp if safety is on and speed is high.
	  // xt_tread_acc is in xtank units/frame; convert current speed to same units for comparison.
	  F32 effectiveTurnRate = maxTurnRate;
	  if(mSafety && speed > 0.01f)
	  {
		 F32 xtSpeed = speed / (XTANK_FPS * BF_SCALE);   // BF units/sec → xtank units/frame
		 if(xt_tread_acc < xtSpeed)
		 {
			effectiveTurnRate = asinf(xt_tread_acc / xtSpeed) * TURN_SCALE;
			if(effectiveTurnRate > maxTurnRate)
			   effectiveTurnRate = maxTurnRate;
		 }
	  }

	  // --- Steering: rotate the hull ---
	  // Right-mouse sets mDesiredHeading; the hull steps toward it at effectiveTurnRate.
	  // Keyboard steer is used only when no desired heading is pending.
	  // (mDesiredHeading already updated above, before the early-exit check)

	  if(mDesiredHeading != NO_HULL_ANGLE_REQUESTED)
	  {
		 // Compute shortest-arc difference in (-pi, pi)
		 F32 diff = mDesiredHeading - mTankHeadingAngle;
		 while(diff > FloatPi)  
			diff -= Float2Pi;
		 while(diff < -FloatPi)  
			diff += Float2Pi;

		 F32 step = effectiveTurnRate * dt;
		 if(fabsf(diff) <= step)
		 {
			mTankHeadingAngle = mDesiredHeading;
			mDesiredHeading   = NO_HULL_ANGLE_REQUESTED;       // Arrived — clear the request
		 }
		 else
			mTankHeadingAngle += (diff > 0 ? step : -step);
	  }
	  else
	  {
		 // No desired heading — keyboard steer as usual
		 mTankHeadingAngle += steer * effectiveTurnRate * dt;
	  }

	  // Keep heading in [-pi, pi]
	  while(mTankHeadingAngle > FloatPi)  
		 mTankHeadingAngle -= Float2Pi;

	  while(mTankHeadingAngle < -FloatPi) 
		 mTankHeadingAngle += Float2Pi;

	  F32 heading = mTankHeadingAngle;

	  // --- Ground friction ---
	  // 1.0 on normal ground, reduced in BF SlipZones (mirrors xtank slip squares).
	  F32 groundFriction = getGame()->getShipAccelModificationFactor(this);

	  // --- Traction: maximum acceleration the treads can deliver ---
	  F32 traction = groundFriction * treadAcc;   // BF units/sec^2
	  if(mVehicleDesign.isHover())
	  {
		 // In xtank, hover treads get a fixed low traction of 1.0 per-frame,
		 // independent of ground friction.  This makes hovers slide much more.
		 traction = 1.0f * XTANK_FPS * XTANK_FPS * BF_SCALE;
	  }

	  // --- Decompose velocity into roll (along heading) and slide (perpendicular) ---
	  F32 headingDiff = moveAngle - heading;
	  F32 rollSpeed = cosf(headingDiff) * speed;   // forward component (BF units/sec)
	  F32 slideSpeed = sinf(headingDiff) * speed;   // sideways component (BF units/sec)

	  // --- Dynamic friction reduction when already sliding ---
	  // In xtank (safety=FALSE), when the vehicle is sliding sideways, dynamic
	  // friction is 70% of static friction.  This is what causes the characteristic
	  // xtank skid: once you start sliding, it's harder to regain grip.
	  if(fabsf(slideSpeed) > 0.1f && !mVehicleDesign.isHover())
		 traction *= 0.7f;

	  // --- Velocity-change limits for this timestep ---
	  F32 tractionDV = traction * dt;   // max dv from traction this step
	  F32 engineDV = engineAcc * dt;  // max dv from engine this step

	  // --- Desired forward speed ---
	  // Reverse is capped at 40% of forward max (xtank has no reverse; this gives
	  // BF's WASD controls a natural feel while keeping reverse slower).
	  F32 maxForward = maxSpeed;
	  F32 maxReverse = maxSpeed * 0.4f;
	  F32 desiredSpeed = (throttle >= 0) ? throttle * maxForward : throttle * maxReverse;

	  // How much the driver wants to change forward speed
	  F32 desiredDV = desiredSpeed - rollSpeed;

	  // Limit by engine power
	  F32 driveDV;
	  if(fabsf(desiredDV) > engineDV)
		 driveDV = engineDV * (desiredDV > 0 ? 1.0f : -1.0f);
	  else
		 driveDV = desiredDV;

	  // --- Xtank acceleration model (from update_vector in update.c) ---
	  F32 rollDV, slideDV;

	  if(driveDV * rollSpeed >= 0)
	  {
		 // Speeding up (or from rest):
		 // Correct lateral slide toward zero, limited by traction.
		 if(fabsf(slideSpeed) <= tractionDV)
			slideDV = -slideSpeed;                                       // Cancel slide completely
		 else
			slideDV = tractionDV * (slideSpeed > 0 ? -1.0f : 1.0f);    // Limit correction to grip

		 rollDV = driveDV;
	  }
	  else
	  {
		 // Braking: decelerate along both roll and slide, proportional to speed.
		 F32 scale = speed / tractionDV;
		 if(scale < 1.0f)
		 {
			// Traction sufficient to stop completely this step
			rollDV = -rollSpeed;
			slideDV = -slideSpeed;
		 }
		 else
		 {
			// Decelerate proportionally (preserving drift direction)
			rollDV = -rollSpeed / scale;
			slideDV = -slideSpeed / scale;
		 }

		 // Engine contributes to braking effort (fun, not realistic -- per xtank)
		 rollDV += driveDV;

		 // Don't over-compensate past the desired speed
		 if(fabsf(desiredDV) < fabsf(rollDV))
			rollDV = desiredDV;
	  }

	  // --- Clamp total acceleration to traction limit ---
	  F32 totalDV = sqrtf(rollDV * rollDV + slideDV * slideDV);
	  if(totalDV > tractionDV)
	  {
		 F32 scale = tractionDV / totalDV;
		 rollDV *= scale;
		 slideDV *= scale;
	  }

	  // --- Apply acceleration in world space ---
	  // roll direction = heading;  slide direction = heading + pi/2
	  F32 ch = cosf(heading);
	  F32 sh = sinf(heading);
	  F32 cp = -sh;   // cos(heading + pi/2)
	  F32 sp = ch;   // sin(heading + pi/2)

	  vel.x += ch * rollDV + cp * slideDV;
	  vel.y += sh * rollDV + sp * slideDV;

	  setVel(stateIndex, vel);

	  // --- Run collision / movement simulation ---
	  F32 result = move(dt, stateIndex, false);

	  // --- Update mTankSpeed for HUD display ---
	  // Project post-collision velocity onto heading to get forward speed.
	  // (Lateral velocity is preserved -- the vehicle can drift/skid.)
	  Point headingVec(ch, sh);
	  mTankSpeed = getVel(stateIndex).dot(headingVec);

	  // --- Fuel consumption (spec §3) ---
	  // Δfuel = FUEL_CONSUME × MAX_SPEED × (drive/max_speed)²
	  //       = 0.025 × throttle²   (per xtank frame)
	  // Scale from xtank frames to real seconds via dt.
	  if(!isGhost())   // server-authoritative only
	  {
		 const F32 FUEL_CONSUME = 0.001f;
		 const F32 XTANK_MAX_SPEED = 25.0f;
		 // drive = |throttle| * max_speed in xtank units → ratio = throttle
		 F32 driveRatio = fabsf(mCurrentMove.speedFraction);
		 F32 fuelDelta = FUEL_CONSUME * XTANK_MAX_SPEED * driveRatio * driveRatio;
		 // fuelDelta is per xtank frame; convert to per-second and scale by dt
		 fuelDelta *= XTANK_FPS * dt;
		 mFuel -= fuelDelta;
		 if(mFuel < 0.0f)
			mFuel = 0.0f;
		 setMaskBits(XtankFuelMask);
	  }

	  return result;
   }

   // Returns the zone in question if this ship is in any zone.
   // If ship is in multiple zones, an aribtrary one will be returned, and the level designer will be flogged.
   BfObject *Ship::isInAnyZone() const
   {
	  findObjectsUnderShip((TestFunc)isZoneType);  // Fills fillVector
	  return doIsInZone(fillVector);
   }


   // Returns the zone in question if this ship is in a zone of type zoneType.
   // If ship is in multiple zones of type zoneTypeNumber, an aribtrary one will be returned, and the level designer will be flogged.
   BfObject *Ship::isInZone(U8 zoneTypeNumber) const
   {
	  findObjectsUnderShip(zoneTypeNumber);        // Fills fillVector
	  return doIsInZone(fillVector);
   }


   // Private helper for isInZone() and isInAnyZone() -- these fill fillVector, and we operate on it below
   BfObject *Ship::doIsInZone(const Vector<DatabaseObject *> &objects) const
   {
	  if(objects.size() == 0)  // Ship isn't in extent of any objectType objects, can bail here
		 return NULL;

	  // Extents overlap...  now check for actual overlap

	  for(S32 i = 0; i < objects.size(); i++)
	  {
		 BfObject *zone = static_cast<BfObject *>(objects[i]);

		 // Get points that define the zone boundaries
		 const Vector<Point> *polyPoints = zone->getCollisionPoly();

		 if(polyPoints->size() != 0 && polygonContainsPoint(polyPoints->address(), polyPoints->size(), getActualPos()))
			return zone;
	  }
	  return NULL;
   }


   // Returns the object in question if this ship is on an object of type objectType
   DatabaseObject *Ship::isOnObject(U8 objectType, U32 stateIndex)
   {
	  findObjectsUnderShip(objectType);

	  if(fillVector.size() == 0)  // Ship isn't in extent of any objectType objects, can bail here
		 return NULL;

	  // Return first actually overlapping object on our candidate list
	  for(S32 i = 0; i < fillVector.size(); i++)
		 if(isOnObject(static_cast<BfObject *>(fillVector[i]), ActualState))
			return fillVector[i];

	  return NULL;
   }


   // Given an object, see if the ship is sitting on it (useful for figuring out if ship is on top of a regenerated repair item, z.B.)
   bool Ship::isOnObject(BfObject *object, U32 stateIndex)
   {
	  Point center;
	  float radius;
	  static Vector<Point> polyPoints;
	  polyPoints.clear();
	  Rect rect;

	  // Ships don't have collisionPolys, so this first check is utterly unneeded unless we change that
	  /*if(getCollisionPoly(polyPoints))
		 return object->collisionPolyPointIntersect(polyPoints);
	  else */
	  if(getCollisionCircle(ActualState, center, radius))
		 return object->collisionPolyPointIntersect(center, radius);
	  else
		 return false;
   }


   F32 Ship::getSensorZoomFraction() const
   {
	  return 1 - mSensorEquipZoomTimer.getFraction();
   }


   // Returns vector based on direction ship is facing
   // BF only
   Point Ship::getAimVector() const
   {
	  return Point(cos(getActualAngle()), sin(getActualAngle()));
   }


   // BF only
   void Ship::selectNextWeapon()
   {
	  setActiveWeapon(mLoadout.getActiveWeaponIndex() + 1);
   }


   // BF only
   void Ship::selectPrevWeapon()
   {
	  setActiveWeapon(mLoadout.getActiveWeaponIndex() - 1);
   }


   // I *think* this runs only on the server, and from tests
   // BF only
   void Ship::selectWeapon(S32 weaponIdx)
   {
	  while(weaponIdx < 0)
		 weaponIdx += ShipWeaponCount;

	  setActiveWeapon(weaponIdx % ShipWeaponCount);      // Advance index to selected weapon
   }


   // (Re)initialise per-weapon runtime state from the current mXtankDesign.
   // Weapons start fully loaded and ready to fire.
   // XT only
   void Ship::initXtankWeaponStates()
   {
	  for(S32 i = 0; i < WEAPON_SLOTS; i++)
	  {
		 XtankWeapon weapon = mVehicleDesign.weapons[i];
		 XtankWeaponState &ws = mWeaponStates[i];

		 if(weapon == XtankWeapon::NONE)
		 {
			ws.ammo = 0;
			ws.reloadTimer.clear();
			ws.refillTimer.clear();
			ws.mIsActive = false;
		 }
		 else
		 {
			const XtankWeaponInfo &wi = xtankWeaponInfos[(S32)weapon];
			ws.ammo = wi.max_ammo;
			ws.reloadTimer.clear();
			ws.refillTimer.reset(wi.refill_time);
			ws.mIsActive = true;
		 }
	  }
   }


   // Dissipate vehicle heat over deltaMs milliseconds.
   // Xtank rule: vehicles shed (heatSinks + 1) heat every 1/15 s.
   // XT only
   void Ship::coolHeat(U32 deltaMs)
   {
	  if(mHeat <= 0.0f)
		 return;

	  const F32 dissipationPerMs = (mVehicleDesign.heatSinks + 1) * XTANK_FPS / 1000.0f;
	  mHeat -= dissipationPerMs * (F32)deltaMs;
	  if(mHeat < 0.0f)
		 mHeat = 0.0f;
   }


   // Decrement per-weapon reload timers by deltaMs.
   // XT only
   void Ship::advanceXtankTimers(U32 deltaMs)
   {
	  for(S32 i = 0; i < WEAPON_SLOTS; i++)
	  {
		 XtankWeaponState &ws = mWeaponStates[i];
		 ws.reloadTimer.update(deltaMs);
	  }
   }


   // Handle per-tick ammo refill when the ship is inside a friendly/neutral ReloadZone.
   // Only weapons that are turned off are eligible for refill.
   // When the ship is outside any ReloadZone the per-weapon refillTimers are reset.
   // XT only
   void Ship::processXtankRefill(U32 deltaMs)
   {
	  // Only Xtank vehicles refill ammo
	  if(!isXtankVehicle())
		 return;

	  BfObject *zoneObj = isInZone(ReloadZoneTypeNumber);


	  if(!zoneObj)
	  {
		 mWasInReloadZone = false;
		 return;
	  }

	  // We're in a ReloadZone!

	  // Reset refill timers on zone entry so the clock starts fresh each visit
	  if(!mWasInReloadZone)
	  {
		 for(S32 i = 0; i < WEAPON_SLOTS; i++)
		 {
			XtankWeapon weapon = mVehicleDesign.weapons[i];
			if(weapon == XtankWeapon::NONE)
			   continue;
			mWeaponStates[i].refillTimer.reset(xtankWeaponInfos[(S32)weapon].refill_time);
		 }
		 mWasInReloadZone = true;
	  }

	  ///// First, handle Instant-refill weapons (refill_time == 0)
	  // Gather eligible weapon slots and compute total ammo cost to fill them all.
	  S32 instantSlots[WEAPON_SLOTS];
	  S32 instantCount = 0;
	  S32 totalCostFull = 0;   // cost to fill every eligible instant-refill weapon

	  for(S32 i = 0; i < WEAPON_SLOTS; i++)
	  {
		 XtankWeapon weapon = mVehicleDesign.weapons[i];
		 if(weapon == XtankWeapon::NONE)
			continue;

		 XtankWeaponState &ws = mWeaponStates[i];
		 if(ws.isActive())
			continue;   // only turned-off weapons refill

		 const XtankWeaponInfo &wi = xtankWeaponInfos[(S32)weapon];
		 if(wi.refill_time != 0)
			continue;   // handled in the timed-refill section below

		 S32 needed = wi.max_ammo - ws.ammo;
		 if(needed <= 0)
			continue;

		 instantSlots[instantCount] = i;
		 instantCount++;
		 totalCostFull += needed * wi.ammo_cost;
	  }

	  if(instantCount > 0)
	  {
		 for(S32 s = 0; s < instantCount; s++)
		 {
			S32 i = instantSlots[s];
			XtankWeapon weapon = mVehicleDesign.weapons[i];
			const XtankWeaponInfo &wi = xtankWeaponInfos[(S32)weapon];
			XtankWeaponState &ws = mWeaponStates[i];
			S32 needed = wi.max_ammo - ws.ammo;

			S32 unitsToAdd;
			if(totalCostFull <= mMoney || wi.ammo_cost <= 0)
			{
			   // Enough money — fill completely
			   unitsToAdd = needed;
			}
			else
			{
			   // Proportional fill: give this slot its share of what we can afford.
			   // Proportion = (slotNeeded * ammo_cost) / totalCostFull applied to mMoney.
			   unitsToAdd = (wi.ammo_cost > 0) ? (mMoney * needed) / (totalCostFull) : needed;
			}

			// Clamp to what we can actually afford
			if(wi.ammo_cost > 0)
			   unitsToAdd = MIN(unitsToAdd, mMoney / wi.ammo_cost);

			unitsToAdd = MIN(unitsToAdd, needed);
			unitsToAdd = MAX(unitsToAdd, 0);

			ws.ammo += unitsToAdd;
			mMoney -= unitsToAdd * wi.ammo_cost;
		 }
	  }

	  ///// Timed-refill weapons (refill_time > 0)
	  for(S32 i = 0; i < WEAPON_SLOTS; i++)
	  {
		 XtankWeapon weapon = mVehicleDesign.weapons[i];
		 if(weapon == XtankWeapon::NONE)
			continue;

		 XtankWeaponState &ws = mWeaponStates[i];
		 if(ws.isActive())
			continue;

		 const XtankWeaponInfo &wi = xtankWeaponInfos[(S32)weapon];
		 if(wi.refill_time == 0)
			continue;   // already handled above
		 if(ws.ammo >= wi.max_ammo)
			continue;

		 if(ws.refillTimer.update(deltaMs))
		 {
			if(mMoney >= wi.ammo_cost)
			{
			   ws.ammo++;
			   mMoney -= wi.ammo_cost;
			   setMaskBits(XtankWeaponAmmoMask);
			}
			// Reset timer for next ammo unit; use clear() when full so mPeriod is preserved for next visit
			if(ws.ammo < wi.max_ammo)
			   ws.refillTimer.reset(wi.refill_time);
			else
			   ws.refillTimer.clear();
		 }
	  }
   }


   // Client variant
   // XT only
   void Ship::toggleWeapon(S32 slotIndex)
   {
	  TNLAssert(isXtankVehicle(), "toggleWeapon should only be called for xtank vehicles");
	  TNLAssert(slotIndex >= 0 && slotIndex < WEAPON_SLOTS, "Invalid slot!");

	  // Toggling a slot that's empty does nothing
	  XtankWeapon weapon = mVehicleDesign.weapons[slotIndex];
	  if(weapon == XtankWeapon::NONE)
		 return;

	  XtankWeaponState &ws = mWeaponStates[slotIndex];
	  ws.mIsActive = !ws.mIsActive;
   }


   // Server variant -- we sent the extra bit to ensure things don't get out of sync
   void Ship::toggleWeapon(S32 slotIndex, bool isActive)
   {
	  XtankWeaponState &ws = mWeaponStates[slotIndex];
	  ws.mIsActive = isActive;

   }


   void Ship::processWeaponFire()
   {
	  // Can only fire when mFireTimer <= 0
	  if(mFireTimer > 0)
		 mFireTimer -= S32(mCurrentMove.time);

	  if(!mCurrentMove.fire && mFireTimer < 0)
		 mFireTimer = 0;


	  mWeaponFireDecloakTimer.update(mCurrentMove.time);


	  if(isXtankVehicle())
		 processXtankWeaponFire();
	  else
		 processBfWeaponFire();


	  // If we've fired, Spawn Shield turns off
	  if(mSpawnShield.getCurrent() != 0)
	  {
		 setMaskBits(SpawnShieldMask);
		 mSpawnShield.clear();
	  }
   }


   // BF only
   void Ship::processBfWeaponFire()
   {
	  GameType *gameType = getGame()->getGameType();

	  WeaponType curWeapon = mLoadout.getActiveWeapon();

	  //             player is firing            player's ship is still largely functional
	  if(gameType && mCurrentMove.fire && (!getClientInfo() || !getClientInfo()->isShipSystemsDisabled()))
	  {
		 // In a while loop, to catch up the firing rate for low Frame Per Second
		 while(mFireTimer <= 0 && mEnergy >= WeaponInfo::getWeaponInfo(curWeapon).minEnergy)
		 {
			mEnergy -= WeaponInfo::getWeaponInfo(curWeapon).drainEnergy;      // Drain energy
#ifdef SHOW_SERVER_SITUATION
			// Make a noise when the client thinks we've shot -- ideally, there should be one boop per shot, delayed by about half
			// of whatever /lag is set to.
			if(isClient())
			   UserInterface::playBoop();
#endif
			mWeaponFireDecloakTimer.reset(WeaponFireDecloakTime);          // Uncloak ship

			if(getClientInfo())
			   getClientInfo()->getStatistics()->countShot(curWeapon);

			Point dir = getAimVector();

			if(isServer())
			{
			   // TODO: To fix skip fire effect on jittery server, need to replace the 0 with... something...
			   GameWeapon::createWeaponProjectiles(curWeapon, dir, getActualPos(), getActualVel(), 0, CollisionRadius - 2, this);
			}

			// Railgun gives a little kickback
			if(curWeapon == WeaponRailgun)
			{
			   static const F32 RAILGUN_KICKBACK_IMPULSE = 400.0f;

			   mImpulseVector -= dir * RAILGUN_KICKBACK_IMPULSE;
			}

			mFireTimer = (S32)WeaponInfo::getWeaponInfo(curWeapon).fireDelay;
		 }
	  }

   }


   // Handle fuel refill while inside a FuelZone
   // Rate: +1 unit/frame at XTANK_FPS (20 fps), i.e. +XTANK_FPS per second, costing fuel_cost money/unit.
   void Ship::processXtankFuel(U32 deltaMs)
   {
	  if(!isXtankVehicle())
		 return;

	  BfObject *zoneObj = isInZone(FuelZoneTypeNumber);

	  if(!zoneObj)
	  {
		 mWasInFuelZone = false;
		 return;
	  }

	  FuelZone *zone = dynamic_cast<FuelZone *>(zoneObj);
	  if(!zone || !zone->isActiveForShip(this))
	  {
		 mWasInFuelZone = false;
		 return;
	  }

	  // Must be stopped
	  if(fabsf(mTankSpeed) >= 5.0f)
		 return;

	  // isInZone() already confirms the ship is inside the zone polygon.
	  // No additional centroid-proximity check is needed.

	  if(mFuel >= mMaxFuel)
		 return;   // Tank already full

	  const S32 engineIdx = MAX(0, MIN((S32)mVehicleDesign.engine, XtankEngineCount - 1));
	  // XtankEngineInfo::fuel is the per-unit refuel cost (5, 8, 10...); ::cost is the engine purchase price.
	  const S32 fuelCost = xtankEngineInfos[engineIdx].fuel;

	  // Rate: +1 unit/frame at XTANK_FPS, scaled to real dt in seconds
	  const F32 XTANK_FPS = 20.0f;
	  F32 dt = deltaMs * 0.001f;
	  F32 unitsToAdd = 1.0f * XTANK_FPS * dt;

	  // Money check: if this costs money, limit to what we can afford
	  if(fuelCost > 0)
	  {
		 F32 maxAffordable = (F32)mMoney / (F32)fuelCost;
		 if(unitsToAdd > maxAffordable)
			 unitsToAdd = maxAffordable;
	  }

	  if(unitsToAdd <= 0)
		 return;   // Out of money

	  // Clamp to tank capacity
	  F32 needed = mMaxFuel - mFuel;
	  if(unitsToAdd > needed)
		 unitsToAdd = needed;

	  mFuel += unitsToAdd;
	  if(fuelCost > 0)
		 mMoney -= (S32)(unitsToAdd * (F32)fuelCost + 0.5f);

	  mWasInFuelZone = true;
	  setMaskBits(XtankFuelMask);
   }


   void Ship::processXtankRepair(U32 deltaMs)
   {
      if(!isXtankVehicle())
         return;

      BfObject *zoneObj = isInZone(RepairZoneTypeNumber);

      if(!zoneObj)
      {
         mWasInRepairZone = false;
         return;
      }

      RepairZone *zone = dynamic_cast<RepairZone *>(zoneObj);
      if(!zone || !zone->isActiveForShip(this))
      {
         mWasInRepairZone = false;
         return;
      }

      // Must be stopped
      if(fabsf(mTankSpeed) >= 5.0f)
         return;

      // Repair rate: +1 armor point/side every 3 xtank frames (150 ms), per spec §7
      static const F32 REPAIR_INTERVAL_MS = 150.0f;
      mRepairAccumMs += (F32)deltaMs;
      if(mRepairAccumMs < REPAIR_INTERVAL_MS)
         return;
      mRepairAccumMs -= REPAIR_INTERVAL_MS;

      const S32 armorIdx = MAX(0, MIN((S32)mVehicleDesign.armor, XtankArmorCount - 1));
      const S32 bodySize = mVehicleDesign.getBodySize();
      const S32 costPerPoint = xtankArmorInfos[armorIdx].cost * bodySize;

      bool anyRepaired = false;
      for(S32 i = 0; i < VehicleSidesCount; i++)
      {
         // HP is stored as integer ticks (100 per damage unit) to match the damage path.
         // Design max is in design armor points; convert to the same scale.
         S32 maxHP = mVehicleDesign.armorSides[i] * 100;
         if(mArmorSides[i] >= maxHP)
              continue;   // This side is full

         if(costPerPoint > 0 && mMoney < costPerPoint)
              continue;   // Can't afford this point

         mArmorSides[i] += 100;   // Repair 1 armor point (stored as 100 internal ticks)
         if(mArmorSides[i] > maxHP)
              mArmorSides[i] = maxHP;
         if(costPerPoint > 0)
              mMoney -= costPerPoint;
         anyRepaired = true;
      }

      if(anyRepaired)
      {
         mWasInRepairZone = true;
         setMaskBits(XtankArmorMask);
      }
   }


   void Ship::processXtankWeaponFire()
   {
	  GameType *gameType = getGame()->getGameType();

	  if(gameType && mCurrentMove.fire && (!getClientInfo() || !getClientInfo()->isShipSystemsDisabled()))
	  {
		 // Body-to-world rotation matrix (shared across all weapon slots this tick).
		 const F32 cosBody = cos(mTankHeadingAngle - FloatHalfPi);
		 const F32 sinBody = sin(mTankHeadingAngle - FloatHalfPi);
		 const Point bodyRight(cosBody, sinBody);
		 const Point bodyForward(-sinBody, cosBody);
		 const Point aimDir = getAimVector();
		 static const F32 BARREL_LENGTH = 12.0f;

		 for(S32 i = 0; i < WEAPON_SLOTS; i++)
		 {
			XtankWeapon weapon = mVehicleDesign.weapons[i];
			if(weapon == XtankWeapon::NONE)
			   continue;

			const XtankWeaponInfo &wi = xtankWeaponInfos[(S32)weapon];
			XtankWeaponState &ws = mWeaponStates[i];
			XtankMountLocation     mount = (XtankMountLocation)mVehicleDesign.weaponMounts[i];

			// Fire conditions (all must be met)
			if(ws.reloadTimer.getCurrent() > 0)   continue;   // RELOADING
			if(!ws.hasAmmo())                     continue;   // NO_AMMO
			if(!ws.isActive())                    continue;   // WEAPON_OFF
			if(mHeat > HeatLimit)                 continue;   // TOO_HOT

			// Fire!
			if(isServer())
			{
			   ws.ammo--;
			   setMaskBits(XtankWeaponAmmoMask);
			}
			ws.reloadTimer.reset(wi.reload_time);
			mHeat += (F32)wi.heat;

			mWeaponFireDecloakTimer.reset(WeaponFireDecloakTime);

			if(isServer())
			{
			   XtankMountLocation mountLoc = (XtankMountLocation)mVehicleDesign.weaponMounts[i];
			   Point mountBody = getXtankMountPointBodySpace(mVehicleDesign.body, mountLoc);
			   Point mountWorld(
				  getActualPos().x + cosBody * mountBody.x - sinBody * mountBody.y,
				  getActualPos().y + sinBody * mountBody.x + cosBody * mountBody.y
			   );

			   Point shotDir = aimDir;
			   if(mountLoc == XtankMountLocation::FRONT)
				  shotDir = bodyForward;
			   else if(mountLoc == XtankMountLocation::BACK)
				  shotDir = bodyForward * -1.0f;
			   else if(mountLoc == XtankMountLocation::LEFT)
				  shotDir = bodyRight * -1.0f;
			   else if(mountLoc == XtankMountLocation::RIGHT)
				  shotDir = bodyRight;

			   Point barrelTip = mountWorld + shotDir * BARREL_LENGTH;
			   GameWeapon::createXtankProjectile(weapon, shotDir, barrelTip, getActualVel(), 0, this);
			}
		 }
	  }
   }



   // Compute the delta between our current render position and the server position after
   // client-side prediction has been run
   void Ship::controlMoveReplayComplete()
   {
	  Point delta = getActualPos() - getRenderPos();
	  F32 deltaLenSq = delta.lenSquared();

	  // If the delta is either very small, or greater than the max interpolation threshold,
	  // just warp to the new position
	  if(deltaLenSq <= sq(0.5) || deltaLenSq > sq(MaxControlObjectInterpDistance))
	  {
#ifndef ZAP_DEDICATED
		 // If it's a large delta, get rid of the movement trails
		 if(deltaLenSq > sq(MaxControlObjectInterpDistance))
			for(S32 i = 0; i < TrailCount; i++)
			   mTrail[i].reset();
#endif

		 copyMoveState(ActualState, RenderState);
		 mInterpolating = false;
	  }
	  else
		 mInterpolating = true;
   }


   void Ship::idle(IdleCallPath path)
   {
	  // Don't idle exploded ships
	  if(mHasExploded)
		 return;

	  if(path == ServerProcessingUpdatesFromClient && getClientInfo())
		 getClientInfo()->getStatistics()->mPlayTime += mCurrentMove.time;

	  Parent::idle(path);

	  if(path == ServerIdleMainLoop && controllingClientIsValid())
	  {
		 // If this is a controlled object in the server's main
		 // idle loop, process the render state forward -- this
		 // is what projectiles will collide against.  This allows
		 // clients to properly lead other clients, instead of
		 // piecewise stepping only when packets arrive from the client.
		 processMove(RenderState);

		 if(getActualVel().lenSquared() != 0 || getActualPos() != getRenderPos() ||
			(isXtankVehicle() && (mCurrentMove.x != 0 || mCurrentMove.y != 0 || mTankSpeed != 0)))
			setMaskBits(PositionMask);

		 mSendSpawnEffectTimer.update(mCurrentMove.time);
	  }
	  else if(path != ClientIdlingLocalShip)   // <=== do we really want this loop running if    path == ServerIdleMainLoop && NOT controllingClientIsValid() ??  (Levels might have "Ship" that should idle)
	  {  // Won't move ship from ClientIdlingLocalShip, but do process some timers (that doesn't effect gameplay) not covered in ClientReplayingPendingMoves

		 //TNLAssert(path == ServerIdleMainLoop, "path == ServerIdleMainLoop is true when level have 'Ship'");

		 // Apply impulse vector and reset it
		 setActualVel(getActualVel() + mImpulseVector);
		 mImpulseVector.set(0, 0);

		 // For all other cases, advance the actual state of the object with the current move.
		 // Dist is the distance the ship moved this tick.
		 F32 dist = processMove(ActualState);

		 if(path == ServerProcessingUpdatesFromClient || path == ClientIdlingLocalShip)
			getClientInfo()->getStatistics()->accumulateDistance(dist);

		 if(path == ServerProcessingUpdatesFromClient ||
			path == ClientIdlingLocalShip ||
			path == ClientReplayingPendingMoves)
		 {
			// For different optimizer settings and different platforms the floating point calculations may come out slightly
			// differently in the lowest mantissa bits.  So normalize after each update the position and velocity, so that
			// the control state update will not differ from client to server.
			static const F32 ShipVarNormalizeMultiplier = 128;
			static const F32 ShipVarNormalizeFraction = 1.0 / ShipVarNormalizeMultiplier;

			static Point p;


			// This rounds the position and velocity to specific bit resolutions
			// log2(ShipVarNormalizeMultiplier). This gives better predictability
			// with floating point operations on pos/vel, and maybe allows better TNL
			// float compression
			p = getActualPos();
			p.scaleFloorDiv(ShipVarNormalizeMultiplier, ShipVarNormalizeFraction);
			Parent::setActualPos(p);

			p = getActualVel();
			p.scaleFloorDiv(ShipVarNormalizeMultiplier, ShipVarNormalizeFraction);
			Parent::setActualVel(p);
		 }

		 if(path == ServerIdleMainLoop || path == ServerProcessingUpdatesFromClient)
		 {
			// Update the render state on the server to match the actual updated state, and mark the object as having changed
			// Position state.  An optimization here would check the before and after positions so as to not update unmoving ships.
			if(getRenderAngle() != getActualAngle() || getRenderPos() != getActualPos() || getRenderVel() != getActualVel() ||
			   (isXtankVehicle() && (mCurrentMove.x != 0 || mCurrentMove.y != 0 || mTankSpeed != 0)))
			   setMaskBits(PositionMask);

			copyMoveState(ActualState, RenderState);
		 }

	  }

	  if(path == ServerProcessingUpdatesFromClient || path == ClientIdlingLocalShip ||
		 path == ClientIdlingNotLocalShip ||
		 (path == ServerIdleMainLoop && !controllingClientIsValid()))  // Level might have "Ship"
	  {
		 mSensorEquipZoomTimer.update(mCurrentMove.time);
		 mCloakTimer.update(mCurrentMove.time);

		 // Update spawn shield unless we move the ship - then it turns off .. server only
		 if(mSpawnShield.getCurrent() != 0)
		 {
			if(path == ServerProcessingUpdatesFromClient && (mCurrentMove.x != 0 || mCurrentMove.y != 0))
			{
			   mSpawnShield.clear();
			   setMaskBits(SpawnShieldMask);  // Tell clients spawn shield turned off due to moving
			}
			else
			   mSpawnShield.update(mCurrentMove.time);
		 }
	  }


	  // Update the object in the game's extents database
	  updateExtentInDatabase();

	  // If this is a move executing on the server and it's different from the last move,
	  // then mark the move to be updated to the ghosts
	  if(path == ServerProcessingUpdatesFromClient && !mCurrentMove.isEqualMove(&mPrevMove))
		 setMaskBits(MoveMask);

	  mPrevMove = mCurrentMove;

	  mRepairTargets.clear();

	  // Process weapons and modules on controlled objects; handles all the energy reductions as well
	  if(path == ServerProcessingUpdatesFromClient || path == ClientReplayingPendingMoves
#ifndef ZAP_DEDICATED
		 || (path == ClientIdlingNotLocalShip && ((ClientGame *)getGame())->getConnectionToServer()->mPackUnpackShipEnergyMeter)
#endif
		 )
	  {
		 // Handle Recharge timer

		 // Half the time if in a friendly/neutral loadout zone
		 BfObject *object = isInZone(LoadoutZoneTypeNumber);
		 S32 currentLoadoutZoneTeam = object ? object->getTeam() : NO_TEAM;
		 U32 updateTime = mCurrentMove.time;
		 if(currentLoadoutZoneTeam == TEAM_NEUTRAL || currentLoadoutZoneTeam == getTeam())
			updateTime *= 2;

		 mFastRechargeTimer.update(updateTime);
		 mFastRecharging = mFastRechargeTimer.getCurrent() == 0;

		 if(isXtankVehicle())
		 {
			advanceXtankTimers(mCurrentMove.time);
			coolHeat(mCurrentMove.time);
		 }

		 processWeaponFire();
		 processModules();
		 rechargeEnergy();

		 if(isXtankVehicle() && path == ServerProcessingUpdatesFromClient)
		 {
			processXtankRefill(mCurrentMove.time);
			processXtankFuel(mCurrentMove.time);
			processXtankRepair(mCurrentMove.time);
		 }
	  }
	  // Find any repair targets for rendering repair rays -- on other paths, this will be done in processModules
	  else if(path == ClientIdlingLocalShip || path == ClientIdlingNotLocalShip)
		 if(mLoadout.isModulePrimaryActive(ModuleRepair))
			findRepairTargets();

	  if(path == ServerProcessingUpdatesFromClient)
		 repairTargets();


	  if(path == ClientIdlingLocalShip || path == ClientIdlingNotLocalShip)
	  {
		 // On the client, update the interpolation of this object unless we are replaying control moves
		 mInterpolating = (getActualVel().lenSquared() < MoveObject::InterpMaxVelocity * MoveObject::InterpMaxVelocity);
		 updateInterpolation();

		 mWarpInTimer.update(mCurrentMove.time);

		 // Emit some particles, trail sections and update the turbo noise
		 emitMovementSparks();
		 updateTrails();
		 updateModuleSounds();

	  }
   }


   bool Ship::checkForSpeedzones(U32 stateIndex)
   {
	  SpeedZone *speedZone = static_cast<SpeedZone *>(isOnObject(SpeedZoneTypeNumber, stateIndex));

	  if(speedZone && speedZone->collide(this))
		 return speedZone->collided(this, stateIndex);
	  return false;
   }


   static Vector<DatabaseObject *> foundObjects;      // Reusable container

   void Ship::findRepairTargets()
   {
	  // We use the render position in findRepairTargets so that
	  // ships that are moving can repair each other (server) and
	  // so that ships don't render funny repair lines to interpolating
	  // ships (client)

	  Point pos = getRenderPos();
	  Rect r(pos, (RepairRadius + CollisionRadius));

	  foundObjects.clear();
	  findObjects((TestFunc)isWithHealthType, foundObjects, r);   // All isWithHealthType objects are items

	  for(S32 i = 0; i < foundObjects.size(); i++)
	  {
		 BfObject *item = static_cast<BfObject *>(foundObjects[i]);

		 // Don't repair dead or fully healed objects...
		 if(item->isDestroyed() || item->getHealth() >= 1)
			continue;

		 // ...or ones not on our team or neutral
		 if(item->getTeam() != TEAM_NEUTRAL && item->getTeam() != getTeam())
			continue;

		 // ...or same team in non-team game, except self
		 if(!getGame()->getGameType()->isTeamGame() && item->getTeam() == getTeam() && item != this)
			continue;

		 // Find the radius of the repairable.  Handle teleporter special case
		 F32 itemRadius = 0;
		 if(item->getObjectTypeNumber() == TeleporterTypeNumber)
			itemRadius = Teleporter::TELEPORTER_RADIUS;
		 else
		 {
			TNLAssert(dynamic_cast<Item *>(item), "Expected to find an item!");
			itemRadius = static_cast<Item *>(item)->getRadius();
		 }

		 // Only repair items within a circle around the ship since we did an object search with a rectangle
		 if((item->getPos() - pos).lenSquared() > sq(RepairRadius + CollisionRadius + itemRadius))
			continue;

		 // In case of CoreItem, don't repair if no repair locations are returned
		 if(item->getRepairLocations(pos).size() == 0)
			continue;

		 mRepairTargets.push_back(item);
	  }
   }


   // Repairs ALL repair targets found above; server only
   void Ship::repairTargets()
   {
	  if(mRepairTargets.size() == 0)
		 return;

	  F32 totalRepair = RepairHundredthsPerSecond * 0.01f * mCurrentMove.time * 0.001f;

	  // totalRepair /= mRepairTargets.size();      // Divide repair amongst repair targets... makes repair too weak

	  DamageInfo di;
	  di.damageAmount = -totalRepair;
	  di.damagingObject = this;
	  di.damageType = DamageTypePoint;

	  for(S32 i = 0; i < mRepairTargets.size(); i++)
		 mRepairTargets[i]->damageObject(&di);
   }


   // BF only
   void Ship::processModules()
   {
	  if(isXtankVehicle())
		 return;

	  // Update some timers
	  for(S32 i = 0; i < ModuleCount; i++)
		 mModuleSecondaryTimer[i].update(mCurrentMove.time);

	  mSpyBugPlacementTimer.update(mCurrentMove.time);

	  // Save the previous module primary/secondary component states; reset them - to be set later
	  bool wasModulePrimaryActive[ModuleCount];
	  bool wasModuleSecondaryActive[ModuleCount];

	  for(S32 i = 0; i < ModuleCount; i++)
	  {
		 wasModulePrimaryActive[i] = mLoadout.isModulePrimaryActive(ShipModule(i));
		 wasModuleSecondaryActive[i] = mLoadout.isModuleSecondaryActive(ShipModule(i));
	  }

	  mLoadout.deactivateAllModules();

	  // Go through our loaded modules and see if they are currently turned on
	  // Are these checked on the server side?
	  for(S32 i = 0; i < ShipModuleCount; i++)
	  {
		 // If you have passive module, it's always active, no restrictions, but is off for energy consumption purposes
		 if(ModuleInfo::getModuleInfo(mLoadout.getModule(i))->getPrimaryUseType() == ModulePrimaryUsePassive)
			mLoadout.setModuleIndexPrimary(i, true);         // needs to be true to allow stats counting

		 // Set loaded module states to 'on' if detected as so, unless modules are disabled,
		 // we need to cooldown, or we are in xtank (tank) mode where modules are suppressed.
		 if(isBitfighterShip() && !mCooldownNeeded &&
			(!getClientInfo() || (getClientInfo() && !getClientInfo()->isShipSystemsDisabled())))
		 {
			if(mCurrentMove.modulePrimary[i])
			   mLoadout.setModuleIndexPrimary(i, true);

			if(mCurrentMove.moduleSecondary[i])
			   mLoadout.setModuleIndexSecondary(i, true);
		 }
	  }

	  // No Turbo or Pulse if we're not moving
	  if(mLoadout.isModulePrimaryActive(ModuleBoost) && mCurrentMove.x == 0 && mCurrentMove.y == 0)
	  {
		 mLoadout.setModulePrimary(ModuleBoost, false);
		 mLoadout.setModuleSecondary(ModuleBoost, false);
	  }

	  if(mLoadout.isModulePrimaryActive(ModuleRepair))
	  {
		 findRepairTargets();
		 // If there are no repair targets, turn off repair
		 if(mRepairTargets.size() == 0)
			mLoadout.setModulePrimary(ModuleRepair, false);
	  }

	  // No cloak with nearby sensored people
	  if(mLoadout.isModulePrimaryActive(ModuleCloak))
	  {
		 if(mWeaponFireDecloakTimer.getCurrent() != 0)
			mLoadout.setModulePrimary(ModuleCloak, false);
		 //else
		 //{
		 //   Rect cloakCheck(getActualPos(), getActualPos());
		 //   cloakCheck.expand(Point(CloakCheckRadius, CloakCheckRadius));

		 //   fillVector.clear();
		 //   findObjects(ShipType | RobotType, fillVector, cloakCheck);

		 //   if(fillVector.size() > 0)
		 //   {
		 //      for(S32 i=0; i<fillVector.size(); i++)
		 //      {
		 //         Ship *s = dynamic_cast<Ship *>(fillVector[i]);

		 //         if(!s) continue;

		 //         if(s->getTeam() != getTeam() && s->isModuleActive(ModuleSensor))
		 //         {
		 //            mModuleActive[ModuleCloak] = false;
		 //            break;
		 //         }
		 //      }
		 //   }
		 //}
	  }

	  U32 timeInMilliSeconds = mCurrentMove.time;

	  // Modules with active primary components
	  S32 primaryActivationCount = 0;

	  // Update things based on available energy...
	  for(S32 i = 0; i < ModuleCount; i++)
	  {
		 if(mLoadout.isModulePrimaryActive(ShipModule(i)))
		 {
			const ModuleInfo *moduleInfo = ModuleInfo::getModuleInfo((ShipModule)i);
			S32 energyUsed = moduleInfo->getPrimaryEnergyDrain() * timeInMilliSeconds;
			mEnergy -= energyUsed;

			// Exclude passive modules
			if(energyUsed != 0)
			   primaryActivationCount += 1;

			if(getClientInfo())
			   getClientInfo()->getStatistics()->addModuleUsed(ShipModule(i), mCurrentMove.time);


			// Sensor module needs to place a spybug
			if(i == ModuleSensor &&
			   mSpyBugPlacementTimer.getCurrent() == 0 &&        // Prevent placement too fast
			   mEnergy > moduleInfo->getPrimaryPerUseCost())     // Have enough energy
			{

			   if(isClient())
			   {
				  mEnergy -= moduleInfo->getPrimaryPerUseCost();
				  mSpyBugPlacementTimer.reset();
			   }
			   else
				  deploySpybug();

			}
		 }

		 // Fire the module secondary component if it is active and the delay timer has run out
		 if(mLoadout.isModuleSecondaryActive(ShipModule(i)) && mModuleSecondaryTimer[i].getCurrent() == 0)
		 {
			S32 energyCost = gModuleInfo[i].getSecondaryPerUseCost();
			// If we have enough energy, fire the module
			if(mEnergy >= energyCost)
			{
			   // Reduce energy
			   mEnergy -= energyCost;

			   // Pulse uses up all energy and applies an impulse vector
			   if(i == ModuleBoost)
			   {
				  // The impulse should be in the same direction you're already going
				  mImpulseVector = getActualVel();

				  // Change to Pulse speed based on current energy
				  mImpulseVector.normalize((((F32)mEnergy / (F32)EnergyMax) * (PulseMaxVelocity - PulseMinVelocity)) + PulseMinVelocity);

				  mEnergy = 0;
			   }
			}
		 }
	  }

	  // Only toggle cooldown if no primary components are active
	  if(primaryActivationCount == 0)
		 mCooldownNeeded = mEnergy <= EnergyCooldownThreshold;

	  // Offset recharge bonus when using modules in a friendly zone
	  //if (primaryActivationCount > 0)
	  //{
	  //   // This assumes the neutral and friendly bonuses are equal
	  //   BfObject *object = isInZone(LoadoutZoneTypeNumber);
	  //   S32 currentZoneTeam = object ? object->getTeam() : NO_TEAM;
	  //   if (currentZoneTeam == TEAM_NEUTRAL || currentZoneTeam == getTeam())
	  //      mEnergy -= EnergyRechargeRateInFriendlyLoadoutZoneModifier * timeInMilliSeconds;
	  //}

	  // Reduce total energy consumption when more than one module is used
	  if(primaryActivationCount > 1)
		 mEnergy += EnergyRechargeRate * timeInMilliSeconds;

	  // Do logic triggered when module primary component state changes
	  for(S32 i = 0; i < ModuleCount; i++)
	  {
		 if(mLoadout.isModulePrimaryActive(ShipModule(i)) != wasModulePrimaryActive[i])
		 {
			if(i == ModuleCloak)
			   mCloakTimer.reset(CloakFadeTime - mCloakTimer.getCurrent(), CloakFadeTime);

			setMaskBits(ModulePrimaryMask);
		 }
	  }

	  // Do logic triggered when module secondary component state changes
	  for(U32 i = 0; i < ModuleCount; i++)
	  {
		 if(mLoadout.isModuleSecondaryActive(ShipModule(i)) != wasModuleSecondaryActive[i])
		 {
			// If current state is active, reset the delay timer if it has run out
			if(mLoadout.isModuleSecondaryActive(ShipModule(i)))
			   if(mModuleSecondaryTimer[i].getCurrent() == 0)
				  mModuleSecondaryTimer[i].reset();

				 setMaskBits(ModuleSecondaryMask);
				 }
			   }
			}


			// Runs on server only, at the request of c2sDeploySpybug
   void Ship::deploySpybug()
   {
	  const ModuleInfo *moduleInfo = ModuleInfo::getModuleInfo(ModuleSensor);     // Spybug is attached to this module

	  S32 deploymentEnergy = moduleInfo->getPrimaryPerUseCost();

	  // Double check the requirements... we don't want no monkey business
	  if(mEnergy < deploymentEnergy || mSpyBugPlacementTimer.getCurrent() > 0)
	  {
		 // Problem! -- send message to client to recredit their energy.  This is a very rare circumstance.
		 GameConnection *cc = getControllingClient();

		 if(cc)
			cc->s2cCreditEnergy(deploymentEnergy);

		 return;
	  }

	  mEnergy -= deploymentEnergy;
	  mSpyBugPlacementTimer.reset();

	  Point direction = getAimVector();
	  GameWeapon::createWeaponProjectiles(WeaponSpyBug, direction, getActualPos(),
		 getActualVel(), 0, CollisionRadius - 2, this);

	  if(getClientInfo())
		 getClientInfo()->getStatistics()->countShot(WeaponSpyBug);
   }


   // Energy can be negative!
   void Ship::creditEnergy(S32 deltaEnergy)
   {
	  mEnergy = MAX(0, MIN(EnergyMax, mEnergy + deltaEnergy));
   }


   // Runs on client and server
   void Ship::rechargeEnergy()
   {
	  U32 timeInMilliSeconds = mCurrentMove.time;

	  // Energy will not recharge if spawn shield is up
	  if(mSpawnShield.getCurrent() != 0)
		 mFastRechargeTimer.reset();    // Fast recharge timer doesn't really get going until after spawn shield is down
	  else
	  {
		 // Base recharge rate
		 mEnergy += EnergyRechargeRate * timeInMilliSeconds;

		 //// Apply energy recharge modifier for the zone the player is in
		 //BfObject *object = isInZone(LoadoutZoneTypeNumber);
		 //S32 currentLoadoutZoneTeam = object ? object->getTeam() : NO_TEAM;

		 //if(currentLoadoutZoneTeam == TEAM_HOSTILE)
		 //   mEnergy += EnergyRechargeRateInHostileLoadoutZoneModifier * timeInMilliSeconds;

		 //else if(currentLoadoutZoneTeam == TEAM_NEUTRAL)
		 //   mEnergy += EnergyRechargeRateInNeutralLoadoutZoneModifier * timeInMilliSeconds;

		 //else if(currentLoadoutZoneTeam == getTeam())
		 //   mEnergy += EnergyRechargeRateInFriendlyLoadoutZoneModifier * timeInMilliSeconds;

		 //else if(currentLoadoutZoneTeam != NO_TEAM)
		 //   mEnergy += EnergyRechargeRateInEnemyLoadoutZoneModifier * timeInMilliSeconds;

		 // Recharge energy very fast if we're completely idle for a given amount of time
		 if(mCurrentMove.x != 0 || mCurrentMove.y != 0 || mCurrentMove.fire || mCurrentMove.isAnyModActive())
		 {
			resetFastRecharge();
		 }

		 if(mFastRecharging)
			mEnergy += EnergyRechargeRateIdleRechargeCycle * timeInMilliSeconds;
	  }

	  // Movement penalty
	  //if (mCurrentMove.x != 0 || mCurrentMove.y != 0)
	  //   mEnergy += EnergyRechargeRateMovementModifier * timeInMilliSeconds;

	  // Handle energy falling below 0
	  if(mEnergy <= 0)
	  {
		 mEnergy = 0;
		 mCooldownNeeded = true;
	  }

	  // Cap energy at max
	  else if(mEnergy >= EnergyMax)
		 mEnergy = EnergyMax;
   }


   void Ship::resetFastRecharge()
   {
	  mFastRechargeTimer.reset();
	  mFastRecharging = false;
   }


   // Returns the armor-side index hit by a projectile traveling in impulseVelocity.
   // impulseVelocity is the projectile's velocity vector (shooter→ship).
   // headingAngle uses standard trig convention (angle from +X axis).
   VehicleSides Ship::getHitSideFromImpulse(const Point &impulseVelocity, F32 headingAngle)
   {
      // Negate so hitDir points FROM the shooter side, i.e. the face that was struck.
      Point hitDir = impulseVelocity * -1.0f;
      hitDir.normalize();

      Point nose(cosf(headingAngle), sinf(headingAngle));
      Point right(sinf(headingAngle), -cosf(headingAngle));  // 90° CW

      F32 dotFront = hitDir.dot(nose);
      F32 dotRight = hitDir.dot(right);

      if(fabsf(dotFront) >= fabsf(dotRight))
         return (dotFront >= 0) ? VehicleSides::SIDE_FRONT : VehicleSides::SIDE_BACK;  
      else
         return (dotRight >= 0) ? VehicleSides::SIDE_LEFT : VehicleSides::SIDE_RIGHT; 
   }
   

   void Ship::damageObject(DamageInfo *theInfo)
   {
	  TNLAssert(mHasExploded || mHealth > 0, "One must be true!  If this never fires, remove mHealth == 0 from if below.");  // Added 22-Sep-2013 by Watusimoto
	  if(mHealth == 0 || mHasExploded)  // Stop multi-kill problem;  might stop robots from becoming invincible
		 return;

	  bool hasArmor = hasModule(ModuleArmor);

	  // Deal with grenades and other explody things, even if they cause no damage
	  if(theInfo->damageType == DamageTypeArea || theInfo->damageType == DamageTypeVector)
	  {
		 static const F32 ARMOR_IMPULSE_ABSORBTION_FACTOR = 0.25f;

		 // Armor pads impulses.  Here comes the tank!
		 if(hasArmor)
			mImpulseVector += (theInfo->impulseVector * ARMOR_IMPULSE_ABSORBTION_FACTOR);
		 else
			mImpulseVector += theInfo->impulseVector;
	  }

	  F32 damageAmount = theInfo->damageAmount;

	  SafeZone *safeZone = static_cast<SafeZone *>(isInZone(SafeZoneTypeNumber));  // Type-filtered query
	  if(damageAmount > 0 && safeZone && safeZone->protectsShip(this))
	  {
		 damageAmount = 0;
		 theInfo->damageAmount = 0;
	  }

	  // No damage?  just return
	  if(damageAmount == 0)
		 return;

	  // Having armor reduces damage
	  static const F32 ARMOR_DAMAGE_REDUCTION_FACTOR = 0.4f;

	  // We have damage
	  if(damageAmount > 0)
	  {
		 if(!getGame()->objectCanDamageObject(theInfo->damagingObject, this))
			return;

		 // Factor in shields
		 if(mLoadout.isModulePrimaryActive(ModuleShield)) // && mEnergy >= EnergyShieldHitDrain)     // Commented code will cause
		 {                                                                                           // shields to drain when they
			//mEnergy -= EnergyShieldHitDrain;                                                       // have been hit.
			return;
		 }

		 // No damage done if spawn shield is active
		 if(mSpawnShield.getCurrent() != 0)
			return;

		 // Having armor reduces damage
		 if(hasArmor)
			damageAmount *= ARMOR_DAMAGE_REDUCTION_FACTOR;           // Any other damage, including asteroids

		 // Xtank directional armor: apply per-side HP tracking per spec.
		 // Armor absorbs all damage; health is NOT used for xtank vehicles.
		 if(isXtankVehicle())
		 {
			// Determine which side of the ship was hit; default to front if no direction available.
			VehicleSides hitSide = (theInfo->impulseVector.lenSquared() > 0)
			   ? getHitSideFromImpulse(theInfo->impulseVector, mTankHeadingAngle)
			   : VehicleSides::SIDE_FRONT;

			// Flat defense absorption: subtract material's defense value from damage.
			S32 armorIdx = MAX(0, MIN((S32)mVehicleDesign.armor, XtankArmorCount - 1));
			S32 defense = xtankArmorInfos[armorIdx].defense;
			F32 reducedDamage = MAX(0.0f, damageAmount - (F32)defense / 10.0f);

			// Deduct from this side's HP pool (stored as integer ticks, 100 per point).
			S32 &sideHP = mArmorSides[(S32)hitSide];
			S32 dmgTicks = (S32)(reducedDamage * 100.0f);
			sideHP -= dmgTicks;
			if(sideHP < 0)
			   sideHP = 0;
			setMaskBits(XtankArmorMask);

			if(sideHP == 0)
			{
			   // Side depleted — vehicle destroyed immediately (xtank spec §4).
			   killAndScore(theInfo);
			}
			// Armor absorbed the hit; do NOT touch mHealth.
			return;
		 }

		 // TODO: RAMPLATE special increases collision damage dealt; implement when
		 // ramming damage is applied from the attacker side.
	  }

	  // Healing (damageAmount < 0)
	  else
	  {
		 // Set healing rate to the same as the damage reduction rate.  Might be
		 // too slow at healing now?
		 static const F32 ARMOR_HEALING_FACTOR = ARMOR_DAMAGE_REDUCTION_FACTOR;

		 if(hasArmor)
			damageAmount *= ARMOR_HEALING_FACTOR;
	  }

	  ClientInfo *damagerOwner = theInfo->damagingObject ? theInfo->damagingObject->getOwner() : NULL;
	  ClientInfo *victimOwner = this->getOwner();

	  bool damageWasSelfInflicted = victimOwner && damagerOwner == victimOwner;

	  // Healing things do negative damage, thus adding to health
	  // The multiplier here is the damageSelfMultiplier from the WeaponInfo
	  mHealth -= damageAmount * (damageWasSelfInflicted ? theInfo->damageSelfMultiplier : 1);
	  setMaskBits(HealthMask);

	  if(mHealth <= 0)
	  {
		 mHealth = 0;
		 killAndScore(theInfo);
	  }
	  else if(mHealth > 1)
		 mHealth = 1;


	  // Do some stats related work
	  if(getClientInfo() && theInfo->damagingObject) // getClientInfo() could be NULL <== could it?... damagingObject could be NULL in testing
	  {
		 Projectile *projectile = NULL;
		 if(theInfo->damagingObject->getObjectTypeNumber() == BulletTypeNumber)
			projectile = static_cast<Projectile *>(theInfo->damagingObject);

		 if(projectile)
			getClientInfo()->getStatistics()->countHitBy(projectile->mWeaponType);

		 else if(mHealth == 0 && theInfo->damagingObject->getObjectTypeNumber() == AsteroidTypeNumber)
			getClientInfo()->getStatistics()->mCrashedIntoAsteroid++;
	  }
   }


   // Returns true if ship represents local player -- client only
   bool Ship::isLocalPlayerShip(Game *game) const
   {
	  if(game == NULL)
		 return false;
	  return getClientInfo() == game->getLocalRemoteClientInfo();
   }


   // Runs when ship spawns -- runs on client and server
   // Gets run on client every time ship spawns, gets run on server once per level
   void Ship::onAddedToGame(Game *game)
   {
	  Parent::onAddedToGame(game);
#ifndef ZAP_DEDICATED
	  if(isClient())       // Client
	  {
		 if(isLocalPlayerShip(game))
			static_cast<ClientGame *>(game)->undelaySpawn();    // Server tells us we're undelayed by spawning our ship
	  }

	  else                 // Server
#endif
	  {
		 mSendSpawnEffectTimer.reset();
	  }
   }


   void Ship::updateModuleSounds()
   {
#ifndef ZAP_DEDICATED
	  TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Need ClientGame for updateModuleSounds");
	  ClientGame *clientGame = static_cast<ClientGame *>(getGame());

	  const S32 moduleSFXs[ModuleCount] =
	  {
		 SFXShieldActive,
		 SFXShipBoost,
		 SFXNone,       // No more sensor
		 SFXRepairActive,
		 SFXUIBoop,     // Need better sound...
		 SFXCloakActive,
		 SFXNone,       // Armor... tough, but he don't say much
	  };

	  for(U32 i = 0; i < ModuleCount; i++)
	  {
		 if(mLoadout.isModulePrimaryActive(ShipModule(i)) && moduleSFXs[i] != SFXNone)
		 {
			if(mModuleSound[i].isValid())
			   clientGame->getUIManager()->setMovementParams(mModuleSound[i], getRenderPos(), getRenderVel());
			else
			   mModuleSound[i] = clientGame->playSoundEffect(moduleSFXs[i], getRenderPos(), getRenderVel());
		 }
		 else
		 {
			if(mModuleSound[i].isValid())
			{
			   clientGame->getUIManager()->stopSoundEffect(mModuleSound[i]);
			   mModuleSound[i] = NULL;
			}
		 }
	  }
#endif
   }


   static U32 MaxFireDelay = 0;

   // static method, only run during init on both client and server
   void Ship::computeMaxFireDelay()
   {
	  for(S32 i = 0; i < WeaponCount; i++)
		 if(WeaponInfo::getWeaponInfo(WeaponType(i)).fireDelay > MaxFireDelay)
			MaxFireDelay = WeaponInfo::getWeaponInfo(WeaponType(i)).fireDelay;
   }


   const U32 negativeFireDelay = 123;  // how far into negative we are allowed to send.
   // MaxFireDelay + negativeFireDelay, 900 + 123 = 1023, so writeRangedU32 are sending full range of 10 bits of information.

   // Only used on client for prediction in replay moves
   void Ship::setState(ControlObjectData *state)
   {
	  setPos(ActualState, state->mPos);
	  setVel(ActualState, state->mVel);
	  mImpulseVector = state->mImpulseVector;
	  mEnergy = state->mEnergy;
	  mFireTimer = state->mFireTimer;
	  mFastRechargeTimer.setCurrent(state->mFastRechargeTimer);
	  mSpyBugPlacementTimer.setCurrent(state->mSpyBugPlacementTimer);
	  mModuleSecondaryTimer[ModuleBoost].setCurrent(state->mPulseTimer);
	  mCooldownNeeded = state->mCooldownNeeded;
	  mFastRecharging = state->mFastRecharging;


	  // Probably needed, it is because ship don't update modules from Move until after moving the ship.
	  mLoadout.setModulePrimary(ModuleBoost, state->mBoostActive);
   }


   void Ship::getState(ControlObjectData *state) const
   {
	  state->mPos = getActualPos();
	  state->mVel = getActualVel();
	  state->mImpulseVector = mImpulseVector;
	  state->mEnergy = mEnergy;
	  state->mFireTimer = mFireTimer;
	  state->mFastRechargeTimer = mFastRechargeTimer.getCurrent();
	  state->mSpyBugPlacementTimer = mSpyBugPlacementTimer.getCurrent();
	  state->mPulseTimer = mModuleSecondaryTimer[ModuleBoost].getCurrent();
	  state->mCooldownNeeded = mCooldownNeeded;
	  state->mFastRecharging = mFastRecharging;
	  state->mBoostActive = mLoadout.isModulePrimaryActive(ModuleBoost);
   }


   void Ship::writeControlState(BitStream *stream)
   {
	  stream->write(getActualPos().x);
	  stream->write(getActualPos().y);
	  stream->write(getActualVel().x);
	  stream->write(getActualVel().y);

	  //stream->writeRangedU32(mEnergy, 0, EnergyMax);
	  //stream->writeFlag(mFastRecharging);
	  stream->writeFlag(mCooldownNeeded);

	  stream->writeRangedU32(mLoadout.getActiveWeaponIndex(), 0, ShipWeaponCount);

	  // Tank physics state: send body index, hull heading and current speed so
	  // the client can correct its prediction without jitter.
	  if(stream->writeFlag(isXtankVehicle()))
	  {
		 stream->write(mTankHeadingAngle);
		 stream->write(mTankSpeed);
	  }
   }


   void Ship::readControlState(BitStream *stream)
   {
	  F32 x, y;

	  stream->read(&x);
	  stream->read(&y);
	  Parent::setActualPos(Point(x, y));

	  stream->read(&x);
	  stream->read(&y);
	  Parent::setActualVel(Point(x, y));

	  //int serverReportedEnergy = stream->readRangedU32(0, EnergyMax);
	  //bool rrrmFastRecharging = stream->readFlag();

	  mCooldownNeeded = stream->readFlag();

	  setActiveWeapon(stream->readRangedU32(0, ShipWeaponCount));

	  // Tank physics state
	  if(stream->readFlag())
	  {
		 stream->read(&mTankHeadingAngle);
		 stream->read(&mTankSpeed);
	  }
   }


   // Only used by tests
   void Ship::setMove(const Move &move)
   {
	  mCurrentMove = move;
   }


   // Transmit ship status from server to client
   // Any changes here need to be reflected in Ship::unpackUpdate
   U32 Ship::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
   {
	  GameConnection *gameConnection = (GameConnection *)connection;

	  if(isInitialUpdate())      // This stuff gets sent only once per ship
	  {
		 // We'll need the name (or some other identifier) to match the ship to its clientInfo on the client side
		 stream->writeStringTableEntry(getClientInfo() ? getClientInfo()->getName() : StringTableEntry());

		 // Now write all the mounts:
		 for(S32 i = 0; i < mMountedItems.size(); i++)
		 {
			if(mMountedItems[i].isValid())
			{
			   S32 index = connection->getGhostIndex(mMountedItems[i]);
			   if(index != -1)      // This will skip any items that haven't yet been created on the client
			   {
				  stream->writeFlag(true);
				  stream->writeInt(index, GhostConnection::GhostIdBitSize);
			   }
			}
		 }
		 stream->writeFlag(false);
	  }  // End initial update

	  if(stream->writeFlag(updateMask & TeamMask))    // A player with admin can change robots teams
		 writeThisTeam(stream);

	  if(stream->writeFlag(updateMask & LoadoutMask))       // Loadout configuration
	  {
		 if(isXtankVehicle())
			mVehicleDesign.writeToStream(stream);
		 else
			mLoadout.writeToStream(stream);
	  }

	  // If this is the intial time through and we have an xtank vehicle, we need to send the design.
	  if(stream->writeFlag(updateMask & InitialMask && isXtankVehicle()))
		 mVehicleDesign.writeToStream(stream);

	  if(!stream->writeFlag(mHasExploded))
	  {
		 // Note that RespawnMask is only used by Robots -- can this be refactored out of Ship.cpp?
		 if(stream->writeFlag(updateMask & (RespawnMask | SpawnShieldMask)))
		 {
			stream->writeFlag((updateMask & RespawnMask) != 0 && mSendSpawnEffectTimer.getCurrent() > 0);  // If true, ship will appear to spawn on client
			U32 sendNumber = (mSpawnShield.getCurrent() + (SpawnShieldTime / 16 / 2)) * 16 / SpawnShieldTime; // rounding
			if(stream->writeFlag(sendNumber != 0))
			   stream->writeInt(sendNumber - 1, 4);
		 }

		 if(stream->writeFlag(updateMask & HealthMask))     // Health
			stream->writeFloat(mHealth, 6);
	  }

	  stream->writeFlag((updateMask & WarpPositionMask) && updateMask != 0xFFFFFFFF);

	  // Don't show warp effect when all mask flags are set, as happens when ship comes into scope
	  stream->writeFlag((updateMask & TeleportMask) && !(updateMask & InitialMask));

	  // Send position if this is our intial update or this ship does not represent the client that owns this ship
	  bool shouldWritePosition = (updateMask & InitialMask) || gameConnection->getControlObject() != this;

	  if(!shouldWritePosition)
	  {
		 // The number of writeFlags here *must* match the same number in the else statement
		 stream->writeFlag(false);
		 stream->writeFlag(false);
		 stream->writeFlag(false);
		 stream->writeFlag(false);
	  }
	  else     // Write mCurrentMove data...
	  {
		 if(stream->writeFlag(updateMask & PositionMask))         // <=== ONE
		 {
			// Send position and speed  ==> use renderPos because that is the server's best guess of where a client-controlled
			//                              ship is at any given moment, even if the server hasn't heard from the client for
			//                              dseveral frames due to network delays.
			gameConnection->writeCompressedPoint(getRenderPos(), stream);
			writeCompressedVelocity(getRenderVel(), BoostMaxVelocity + 1, stream);

			// For xtank vehicles also send the hull heading angle and speed so observer clients
			// stay in sync during steering and coasting (these are NOT recoverable from pos/vel alone).
			if(isXtankVehicle())
			{
			   stream->write(mTankHeadingAngle);
			   stream->write(mTankSpeed);
			}
		 }
		 if(stream->writeFlag(updateMask & MoveMask))             // <=== TWO
			mCurrentMove.pack(stream, NULL, false);               // Send current move

		 // If a module primary component is detected as on, pack it
		 if(stream->writeFlag(updateMask & ModulePrimaryMask))    // <=== THREE
			for(S32 i = 0; i < ModuleCount; i++)                  // Send info about which modules are active (primary)
			   stream->writeFlag(mLoadout.isModulePrimaryActive(ShipModule(i)));

		 // If a module secondary component is detected as on, pack it
		 if(stream->writeFlag(updateMask & ModuleSecondaryMask))  // <=== FOUR
			for(S32 i = 0; i < ModuleCount; i++)                  // Send info about which modules are active (secondary)
			   stream->writeFlag(mLoadout.isModuleSecondaryActive(ShipModule(i)));
	  }

	  if(gameConnection->mPackUnpackShipEnergyMeter)
	  {
		 stream->writeRangedU32(mEnergy >> 5, 0, EnergyMax >> 5);
		 stream->writeInt(mFastRechargeTimer.getCurrent() >> 4, 9);
		 stream->writeFlag(mCooldownNeeded);
		 if(stream->writeFlag(mFireTimer != 0))
			stream->writeInt(mFireTimer >> 4, 8);
		 stream->writeRangedU32(mLoadout.getActiveWeaponIndex(), 0, ShipWeaponCount);
		 // Xtank per-slot weapon state: active flag + finite ammo count.
		 // Both sides derive each slot's max_ammo from the already-synced mXtankDesign,
		 // so no extra range descriptor is needed in the stream.
	  }

	  if(isXtankVehicle())
	  {
		 if(stream->writeFlag(updateMask & XtankWeaponAmmoMask))
			 for(S32 i = 0; i < WEAPON_SLOTS; i++)
				if(mVehicleDesign.weapons[i] != XtankWeapon::NONE)
				{
				  const XtankWeaponState &ws = mWeaponStates[i];
				  XtankWeapon weapon = mVehicleDesign.weapons[i];
				  const XtankWeaponInfo &wi = xtankWeaponInfos[(S32)weapon];
				  if(wi.max_ammo != S32_MAX)
					 stream->writeRangedU32((U32)MAX(0, ws.ammo), 0, (U32)wi.max_ammo);
				}

		 // Pack fuel as a ranged U32 with 0..fcap range.
		 if(stream->writeFlag(updateMask & XtankFuelMask))
		 {
			 const S32 engineIdx = MAX(0, MIN((S32)mVehicleDesign.engine, XtankEngineCount - 1));
			 U32 fcap = (U32)MAX(1, xtankEngineInfos[engineIdx].fcap);
			 stream->writeRangedU32((U32)MAX(0.0f, mFuel), 0, fcap);
		 }

		 // Pack per-side armor HP.
		 if(stream->writeFlag(updateMask & XtankArmorMask))
		 {
			 for(S32 i = 0; i < VehicleSidesCount; i++)
			 {
				// Design max stored as armor points; HP is 100x that scale.
				S32 maxHP = mVehicleDesign.armorSides[i] * 100;
				if(maxHP <= 0) maxHP = 1;
				stream->writeRangedU32((U32)MAX(0, mArmorSides[i]), 0, (U32)maxHP);
			 }
		 }
	  }

	  return 0;
   }


   void Ship::findClientInfoFromName()
   {
	  if(mClientInfo.isValid())
		 return;

	  // ClientInfo can be NULL if the ship has been added via level file, for example
	  mClientInfo = getGame()->findClientInfo(mPlayerName);
	  if(mClientInfo)
		 mClientInfo->setShip(this);
   }

   // Any changes here need to be reflected in Ship::packUpdate
   void Ship::unpackUpdate(GhostConnection *connection, BitStream *stream)
   {
#ifndef ZAP_DEDICATED
	  bool positionChanged = false;    // True when position changes a little -- ship position will be interpolated
	  bool shipwarped = false;         // True when position changes a lot -- ship will be warped to new location

	  bool playSpawnEffect = false;

	  TNLAssert(isClient(), "We are expecting a ClientGame here!");

	  if(isInitialUpdate())
	  {
		 // During the initial update, we need to assign a ClientInfo object to this ship.  We'll
		 // identify the proper ClientInfo by the player's name.
		 //
		 // Read the name and use it to find the clientInfo that should be waiting for us... hopefully
		 stream->readStringTableEntry(&mPlayerName);

		 findClientInfoFromName();

		 // Read mounted items:
		 while(stream->readFlag())
		 {
			S32 index = stream->readInt(GhostConnection::GhostIdBitSize);
			MountableItem *item = static_cast<MountableItem *>(connection->resolveGhost(index));
			if(item)                      // Could be NULL if server hasn't yet sent mounted item to us
			   item->mountToShip(this);
		 }

	  }  // initial update

	  if(stream->readFlag())        // Team changed (TeamMask)
		 readThisTeam(stream);

	  if(stream->readFlag())        // New loadout configuration (LoadoutMask)
	  {
		 if(isXtankVehicle())
			unpackDesign(stream);
		 else
			unpackLoadout(stream);
	  }

	  // InitialMask && isXtankVehicle()
	  if(stream->readFlag())
	  {
		 unpackDesign(stream);

		 mTankHeadingAngle = -FloatHalfPi;
		 mTankSpeed = 0;
#ifndef ZAP_DEDICATED
		 // Notify the HUD so the xtank panel stays current for the local player.
		 if(isLocalPlayerShip(getGame()))
			static_cast<ClientGame *>(getGame())->xtankDesignUpdated(mVehicleDesign);
#endif
	  }

	  if(stream->readFlag()) {   // mHasExploded
		 mHealth = 0;
		 if(!mHasExploded)
		 {
			mHasExploded = true;
			disableCollision();

			if(!isInitialUpdate())
			   emitExplosion();     // Boom!
		 }

		 if(getGame() && isLocalPlayerShip(getGame()))   // If this ship is ours, quit engineer menu
			getGame()->quitEngineerHelper();
	  }
	  else
	  {
		 if(stream->readFlag())     // Respawn
		 {
			if(mHasExploded)
			   enableCollision();
			mHasExploded = false;
			playSpawnEffect = stream->readFlag();    // Prevent spawn effect every time the robot goes into scope
			shipwarped = true;

			if(stream->readFlag())
			   mSpawnShield.reset((stream->readInt(4) + 1) * SpawnShieldTime / 16);
			else
			   mSpawnShield.reset(0);
		 }

		 if(stream->readFlag())     // Health
			mHealth = stream->readFloat(6);
	  }

	  if(stream->readFlag())        // Ship made a large change in position
		 shipwarped = true;

	  if(stream->readFlag())        // Ship just teleported
	  {
		 shipwarped = true;
		 mWarpInTimer.reset(WarpFadeInTime);    // Make ship all spinny (sfx, spiral bg are done by the teleporter itself)
	  }

	  if(stream->readFlag())     // UpdateMask
	  {
		 Point p;
		 ((GameConnection *)connection)->readCompressedPoint(p, stream);
		 Parent::setActualPos(p);

		 readCompressedVelocity(p, BoostMaxVelocity + 1, stream);
		 Parent::setActualVel(p);

		 // For xtank vehicles, also read the hull heading angle and speed that were
		 // packed alongside the position.  mXtankDesign.body is already current because
		 // the XtankDesignMask block is processed earlier in this same unpackUpdate call.
		 if(isXtankVehicle())
		 {
			stream->read(&mTankHeadingAngle);
			stream->read(&mTankSpeed);
		 }

		 positionChanged = true;
	  }

	  if(stream->readFlag())     // MoveMask
	  {
		 mCurrentMove.initialize();             // Reset mCurrentMove to its factory defaults
		 mCurrentMove.unpack(stream, false);    // And populate it with data from stream
	  }

	  if(stream->readFlag())     // ModulePrimaryMask
	  {
		 bool wasPrimaryActive[ModuleCount];
		 for(S32 i = 0; i < ModuleCount; i++)
		 {
			wasPrimaryActive[i] = mLoadout.isModulePrimaryActive(ShipModule(i));
			mLoadout.setModulePrimary(ShipModule(i), stream->readFlag());

			// Module activity toggled
			if(wasPrimaryActive[i] != mLoadout.isModulePrimaryActive(ShipModule(i)))
			{
			   if(i == ModuleCloak)
				  mCloakTimer.reset(CloakFadeTime - mCloakTimer.getCurrent(), CloakFadeTime);
			}
		 }
	  }

	  if(stream->readFlag())     // ModuleSecondaryMask
		 for(S32 i = 0; i < ModuleCount; i++)
			mLoadout.setModuleSecondary(ShipModule(i), stream->readFlag());

	  setActualAngle(mCurrentMove.angle);


	  if(positionChanged && !isRobot())
	  {
		 mCurrentMove.time = (U32)connection->getOneWayTime();
		 processMove(ActualState);
	  }

	  if(shipwarped)
	  {
		 mInterpolating = false;
		 copyMoveState(ActualState, RenderState);

		 for(S32 i = 0; i < TrailCount; i++)
			mTrail[i].reset();
	  }
	  else
		 mInterpolating = true;


	  if(playSpawnEffect)
	  {
		 mWarpInTimer.reset(WarpFadeInTime);    // Make ship all spinny

		 static_cast<ClientGame *>(getGame())->emitTeleportInEffect(getActualPos(), 1);

		 getGame()->playSoundEffect(SFXTeleportIn, getActualPos());
	  }

	  if(positionChanged)
		 updateExtentInDatabase();

	  if(((GameConnection *)connection)->mPackUnpackShipEnergyMeter)
	  {
		 mEnergy = stream->readRangedU32(0, EnergyMax >> 5) << 5;
		 mFastRechargeTimer.reset(stream->readInt(9) << 4, mFastRechargeTimer.getPeriod());
		 mCooldownNeeded = stream->readFlag();
		 if(stream->readFlag())
			mFireTimer = (stream->readInt(8) << 4) + 10;
		 else
			mFireTimer = 0;
		 // Claude 4.6 Explaantion of the << 4 + 10:
		 // The pack and unpack are asymmetric in an intentional way.
		 // The >> 4 on write quantizes the timer to 16 ms steps to fit in 8 bits (0–4080 ms range). That
		 // truncation always floors the value — you lose 0–15 ms. On read, << 4 reconstructs the floor of
		 // the bucket.
		 //
		 // The + 10 is a deliberate midpoint bias: instead of always reconstructing at the bottom of the 16
		 // ms bucket (up to 15 ms low), adding half the bucket size (8 ms) centers the reconstruction error
		 // around zero. The value chosen is 10 rather than 8, which is probably a small empirical tweak —
		 // since the packet spends time in transit while the timer is running down, the true value at the
		 // moment of receive is already somewhat less than what was sent, so biasing slightly above the
		 // midpoint compensates for that one-way latency drift.
		 setActiveWeapon(stream->readRangedU32(0, ShipWeaponCount));
	  }
	  // Xtank per-slot weapon state: active flag + finite ammo count.
	  // Uses the same mXtankDesign already updated earlier in this unpackUpdate.
	  if(isXtankVehicle())
	  {
		 if(stream->readFlag())     // XtankWeaponStateMask
			 for(S32 i = 0; i < WEAPON_SLOTS; i++)
				if(mVehicleDesign.weapons[i] != XtankWeapon::NONE)
				{
				  XtankWeaponState &ws = mWeaponStates[i];
				  XtankWeapon weapon = mVehicleDesign.weapons[i];
				  const XtankWeaponInfo &wi = xtankWeaponInfos[(S32)weapon];
				  if(wi.max_ammo != S32_MAX)
					 ws.ammo = (S32)stream->readRangedU32(0, (U32)wi.max_ammo);		// Read ammo for each slot
				}

		 // Fuel level
		 if(stream->readFlag())     // XtankFuelMask
		 {
			 const S32 engineIdx = MAX(0, MIN((S32)mVehicleDesign.engine, XtankEngineCount - 1));
			 U32 fcap = (U32)MAX(1, xtankEngineInfos[engineIdx].fcap);
			 mFuel = (F32)stream->readRangedU32(0, fcap);
			 mMaxFuel = (F32)fcap;
		 }

		 // Per-side armor HP
		 if(stream->readFlag())     // XtankArmorMask
		 {
			 for(S32 i = 0; i < VehicleSidesCount; i++)
			 {
				S32 maxHP = mVehicleDesign.armorSides[i] * 100;
				if(maxHP <= 0) maxHP = 1;
				mArmorSides[i] = (S32)stream->readRangedU32(0, (U32)maxHP);
			 }
		 }
	  }

#endif
   }  // unpackUpdate



   // Ship sent an updated loadout as part of their packet.  Handle that here.
   // Needs to remain symmetrical with unpackLoadout() above.	  
   // BF only
   void Ship::unpackLoadout(BitStream *stream)
   {
	  bool hadSensorThen = mLoadout.hasModule(ModuleSensor);

	  mLoadout.readFromStream(stream);

	  bool hasSensorNow = mLoadout.hasModule(ModuleSensor);
	  bool hasEngineerNow = mLoadout.hasModule(ModuleEngineer);

	  // Set sensor zoom timer if sensor carrying status has switched
	  if(hadSensorThen != hasSensorNow && !isInitialUpdate())  // ! isInitialUpdate(), don't do zoom out effect of ship spawn
		 mSensorEquipZoomTimer.reset();


	  // Notify the user interface (via the ClientGame object) about some things that may have changed.
	  // Note that during testing, we might not have a game object, so we'll need to check for NULL here.
	  Game *game = getGame();

	  if(!game)
		 return;

	  if(!hasEngineerNow)			   // Can't engineer without this module
	  {
		 if(isLocalPlayerShip(game))   // If this ship is ours, quit engineer menu (does nothing if menu is not shown)
			game->quitEngineerHelper();
	  }

	  // Alert the UI that a new loadout has arrived (ClientGame->GameUI->LoadoutIndicator)
#ifndef ZAP_DEDICATED
	  if(isLocalPlayerShip(game))
		 static_cast<ClientGame *>(game)->newLoadoutHasArrived(mLoadout);
#endif

	  // Looks like the user has successfully updated their loadout... we want to show a congratulations message.  However,
	  // we don't want to do this if it has changed because the level reset, nor if it changed due to a respawn.  If the
	  // level has a loadout zone, it means that the loadout was not changed due to a spawn event.
	  if(!isInitialUpdate() && game->levelHasLoadoutZone())
		 game->addInlineHelpItem(LoadoutFinishedItem);
   }


   // XT only
   void Ship::unpackDesign(BitStream *stream)
   {
	  mVehicleDesign.readFromStream(stream);

	  // Prime mArmorSides to full for the new design.  Any in-progress damage will be
	  // corrected by a following XtankArmorMask packet; on a fresh spawn this gives the
	  // client the correct starting values immediately without waiting for a damage event.
	  for(S32 i = 0; i < VehicleSidesCount; i++)
		 mArmorSides[i] = mVehicleDesign.armorSides[i] * 100;
   }


   F32 Ship::getUpdatePriority(GhostConnection *connection, U32 updateMask, S32 updateSkips)
   {
	  F32 value = Parent::getUpdatePriority(connection, updateMask, updateSkips);

	  if(getControllingClient())
		 value += 2.3f;
	  else
		 value -= 2.3f;

	  return value;
   }


   void Ship::updateInterpolation()
   {
	  Parent::updateInterpolation();

	  // Update position of any mounted items
	  for(S32 i = 0; i < mMountedItems.size(); i++)
		 if(mMountedItems[i].isValid())
			mMountedItems[i]->setRenderPos(getRenderPos());
   }


   bool Ship::hasModule(ShipModule mod) const
   {
	  return mLoadout.hasModule(mod);
   }


   bool Ship::isDestroyed()
   {
	  return mHasExploded;
   }


   bool Ship::isVisible(bool viewerHasSensor)
   {
	  if(viewerHasSensor || !mLoadout.isModulePrimaryActive(ModuleCloak))
		 return true;

	  for(S32 i = 0; i < mMountedItems.size(); i++)
		 if(mMountedItems[i].isValid() && mMountedItems[i]->isItemThatMakesYouVisibleWhileCloaked())
			return true;

	  return false;
   }


   // Returns index of first flag mounted on ship, or NO_FLAG if there aren't any
   S32 Ship::getFlagIndex()
   {
	  for(S32 i = 0; i < mMountedItems.size(); i++)
		 if(mMountedItems[i].isValid() && (mMountedItems[i]->getObjectTypeNumber() == FlagTypeNumber))
			return i;
	  return GameType::NO_FLAG;
   }


   S32 Ship::getFlagCount()
   {
	  S32 count = 0;
	  for(S32 i = 0; i < mMountedItems.size(); i++)
		 if(mMountedItems[i].isValid() && (mMountedItems[i]->getObjectTypeNumber() == FlagTypeNumber))
		 {
			FlagItem *flag = static_cast<FlagItem *>(mMountedItems[i].getPointer());
			count += flag->getFlagCount();      // Nexus flag have multiple flags as one item
		 }
	  return count;
   }


   bool Ship::isCarryingItem(U8 objectType) const
   {
	  for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
		 if(mMountedItems[i].isValid() && mMountedItems[i]->getObjectTypeNumber() == objectType)
			return true;

	  return false;
   }


   // Dismounts first object found of specified type, and returns the object.  If no objects of specified type found, will return NULL.
   MountableItem *Ship::dismountFirst(U8 objectType)
   {
	  for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
		 if(mMountedItems[i]->getObjectTypeNumber() == objectType)
		 {
			MountableItem *item = mMountedItems[i];
			item->dismount(DISMOUNT_NORMAL);
			return item;
		 }

	  return NULL;
   }


   // Dismount all objects of any type -- runs on client and server.  Only runs when carrier was killed.
   void Ship::dismountAll()
   {
	  // Count down here because as items are dismounted, they will be removed from the mMountedItems vector
	  for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
		 if(mMountedItems[i].isValid())               // Can be NULL when quitting the server
			mMountedItems[i]->dismount(DISMOUNT_MOUNT_WAS_KILLED);
   }


   // Dismount all objects of specified type.  Currently only used when loadout no longer includes engineer and ship drops all ResourceItems.
   void Ship::dismountAll(U8 objectType)
   {
	  for(S32 i = mMountedItems.size() - 1; i >= 0; i--)
		 if(mMountedItems[i]->getObjectTypeNumber() == objectType)
			mMountedItems[i]->dismount(DISMOUNT_NORMAL);
   }


   bool Ship::isLoadoutSameAsCurrent(const LoadoutTracker &loadout)
   {
	  return loadout == mLoadout;      // Yay for operator overloading!
   }


   void Ship::onNewLoadoutAccpeted()
   {
	  setMaskBits(LoadoutMask);
   }


   const LoadoutTracker *Ship::getLoadout() const
   {
	  return &mLoadout;
   }


   // This actualizes the requested loadout... when, for example the user enters a loadout zone
   // To set the "on-deck" loadout, use GameType->setClientShipLoadout()
   // Returns true if loadout has changed
   // silent param only true when a ship is spawning and there is a loadout zone in the level; we need to update
   // the ship's loadout to be what they had before they died, but we don't want to send a message to the client
   // Server only; BF only
   bool Ship::setLoadout(const LoadoutTracker &newLoadout, bool silent)
   {
	  // Check to see if the new configuration is the same as the old.  If so, we have nothing to do.
	  if(newLoadout == mLoadout)      // Don't bother if ship config hasn't changed
		 return false;

	  if(getClientInfo())
		 getClientInfo()->getStatistics()->mChangedLoadout++;

	  mLoadout = newLoadout;

	  setMaskBits(LoadoutMask);

	  // Set our current weapon to the first one, for consistency
	  selectWeapon(0);

	  if(!hasModule(ModuleEngineer))         // We don't have engineer, so drop any resources we may be carrying
	  {
		 dismountAll(ResourceItemTypeNumber);

		 if(getClientInfo())
		 {
			destroyPartiallyDeployedTeleporter();
			getClientInfo()->sTeleporterCleanup();
		 }
	  }

	  if(!silent)
	  {
		 // Notifiy user
		 GameConnection *cc = getControllingClient();

		 if(cc)
		 {
			static StringTableEntry msg("Ship loadout configuration updated.");
			cc->s2cDisplayMessage(GameConnection::ColorInfo, SFXUIBoop, msg);
		 }
	  }

	  return true;
   }


   // Server only; XT only
   bool Ship::setDesign(const VehicleDesign &newDesign, bool silent)
   {
	  // Check to see if the new configuration is the same as the old.  If so, we have nothing to do.
	  if(newDesign == mVehicleDesign)      // Don't bother if ship config hasn't changed
		 return false;

	  if(getClientInfo())
		 getClientInfo()->getStatistics()->mChangedLoadout++;

	  mVehicleDesign = newDesign;
	  initXtankWeaponStates();

	  // Reset armor HP to new design maximums (×100 ticks to match internal damage scale).
	  for(S32 i = 0; i < VehicleSidesCount; i++)
		 mArmorSides[i] = mVehicleDesign.armorSides[i] * 100;

	  setMaskBits(LoadoutMask | XtankArmorMask);

	  if(!silent)
	  {
		 // Notifiy user
		 GameConnection *cc = getControllingClient();

		 if(cc)
		 {
			static StringTableEntry msg("Ship design updated.");
			cc->s2cDisplayMessage(GameConnection::ColorInfo, SFXUIBoop, msg);
		 }
	  }

	  return true;
   }

   // Runs on client and server
   void Ship::setActiveWeapon(U32 weaponIndex)
   {
	  mLoadout.setActiveWeapon(weaponIndex);

#ifndef ZAP_DEDICATED
	  // Notify the UI that the weapon has changed (mGame might be NULL when testing)
	  if(mGame && !mGame->isServer() && static_cast<ClientGame *>(mGame)->getConnectionToServer() && static_cast<ClientGame *>(mGame)->getConnectionToServer()->getControlObject() == this)
		 static_cast<ClientGame *>(mGame)->setActiveWeapon(weaponIndex);
#endif
   }


   // Used by tests
   WeaponType Ship::getActiveWeapon() const
   {
	  return mLoadout.getActiveWeapon();
   }


   // Used by tests
   string Ship::getLoadoutString() const
   {
	  return mLoadout.toString(true);
   }


   bool Ship::isModulePrimaryActive(ShipModule mod)
   {
	  return mLoadout.isModulePrimaryActive(mod);
   }


   ShipModule Ship::getModule(U32 modIndex)
   {
	  return mLoadout.getModule(modIndex);
   }


   void Ship::onEnteredZone(Zone *zone)
   {
	  EventManager::get()->fireEvent(EventManager::ShipEnteredZoneEvent, this, zone);
   }


   void Ship::onLeftZone(Zone *zone)
   {
	  EventManager::get()->fireEvent(EventManager::ShipLeftZoneEvent, this, zone);
   }


   void Ship::killAndScore(DamageInfo *theInfo)
   {
	  if(isClient())     // Server only, please...
		 return;

	  GameType *gt = getGame()->getGameType();
	  if(gt)
		 gt->controlObjectForClientKilled(getClientInfo(), this, theInfo->damagingObject);

	  BfObject *shooter = WeaponInfo::getWeaponShooterFromObject(theInfo->damagingObject);

	  // Fire ShipKilled event
	  EventManager::get()->fireEvent(EventManager::ShipKilledEvent,
		 this, theInfo->damagingObject, shooter);

	  kill();
   }


   // Ship was killed.
   void Ship::kill()
   {
	  if(isServer())    // Server only block
	  {
		 if(getOwner())
		 {
			getOwner()->saveActiveLoadout(mLoadout);      // Save current loadout in getOwner()->mActiveLoadout
			getOwner()->saveActiveDesign(mVehicleDesign);   // Save current vehicle design in getOwner()->mActiveDesign
		 }

		 // Fire the ShipLeftZoneEvent for every zone the ship is in
		 Vector<SafePtr<Zone> > zoneList;   // Reuse our reusable container

		 getZonesObjectIsIn(zoneList);

		 for(S32 i = 0; i < zoneList.size(); i++)
			EventManager::get()->fireEvent(EventManager::ShipLeftZoneEvent, this, static_cast<Zone *>(zoneList[i].getPointer()));
	  }

	  // Client and server
	  deleteObject(KillDeleteDelay);
	  mHasExploded = true;
	  setMaskBits(ExplodedMask);
	  disableCollision();

	  // Jettison any mounted items
	  dismountAll();

	  // Handle if in the middle of building a teleport
	  if(isServer())   // Server only
	  {
		 destroyPartiallyDeployedTeleporter();
		 if(getClientInfo())
			getClientInfo()->sTeleporterCleanup();
	  }
   }


   // Server only -- ship has been killed, or player changed loadout in middle of engineering
   void Ship::destroyPartiallyDeployedTeleporter()
   {
	  if(mEngineeredTeleporter)
	  {
		 Teleporter *t = mEngineeredTeleporter;
		 mEngineeredTeleporter = NULL;          // Set to NULL first to avoid "Your teleporter got destroyed" message
		 getGame()->teleporterDestroyed(t);
	  }
   }


   void Ship::setChangeTeamMask()
   {
	  setMaskBits(TeamMask);
   }


   // Client only
   void Ship::emitExplosion()
   {
#ifndef ZAP_DEDICATED
	  TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");
	  ClientGame *game = static_cast<ClientGame *>(getGame());

	  game->emitShipExplosion(getRenderPos());
#endif
   }


#ifndef ZAP_DEDICATED
   // Returns the ShipShapeInfo to use for this ship's rendering and spark emission.
   // When an xtank body is active (mXtankDesign.body >= 0) the xtank shape is used;
   // otherwise the standard BF shape determined by mShapeType is returned.
   const ShipShapeInfo *Ship::getActiveShipShapeInfo() const
   {
	  if(isXtankVehicle())
		 return &xtankBodyInfos[(S32)mVehicleDesign.body];

	  return &ShipShape::shipShapeInfos[mShapeType];
   }
#endif


   // Toggle between the Lightcycle xtank body and the regular Bitfighter ship.
   // Pressing once enters the Lightcycle; pressing again returns to the normal ship.
   // The body change is propagated to the server via Move::bodyIndex (set by
   // UIGame.cpp after calling this function).
   void Ship::cycleXtankBody()
   {
	  if(mVehicleDesign.body == XtankBody::BITFIGHTER_SHIP)
	  {
		 // Switch to xTank vehicle
		 mVehicleDesign = VehicleDesign();
		 //mXtankDesign.initForBody(mXtankDesign.body);  // Load default weapons
	  }
	  else
	  {
		 // Return to regular ship
		 mVehicleDesign = VehicleDesign();  // Reset when returning to BF ship
		 mVehicleDesign.body = XtankBody::BITFIGHTER_SHIP;
	  }
   }


   // Override getCollisionCircle to return the hull-based radius for xtank bodies
   // so that large vehicles (e.g. Panzy, Rhino) have a proper hitbox instead of
   // the default CollisionRadius of 24.
   bool Ship::getCollisionCircle(U32 stateIndex, Point &point, F32 &radius) const
   {
	  point = getPos(stateIndex);
	  if(isXtankVehicle())
		 radius = xtankBodyCollisionRadius[(S32)mVehicleDesign.body];
	  else
		 radius = (F32)CollisionRadius;
	  return true;
   }


   // Client only
   void Ship::emitMovementSparks()
   {
#ifndef ZAP_DEDICATED
	  static const F32 TOO_SLOW_FOR_SPARKS = 0.1f;
	  if(mHasExploded || getActualVel().lenSquared() < sq(TOO_SLOW_FOR_SPARKS))
		 return;

	  const ShipShapeInfo *shipShapeInfo = getActiveShipShapeInfo();
	  S32 cornerCount = shipShapeInfo->cornerCount;

	  Vector<Point> corners;
	  corners.resize(cornerCount);

	  Vector<Point> shipDirs;
	  shipDirs.resize(cornerCount);

	  for(S32 i = 0; i < cornerCount; i++)
		 corners[i].set(shipShapeInfo->cornerPoints[i * 2], shipShapeInfo->cornerPoints[i * 2 + 1]);

	  // For xtank vehicles the hull rotates by mTankHeadingAngle; aim angle drives
	  // only the turret.  Using the aim angle here would make the wake trail fan
	  // out in the wrong direction when the turret points sideways.
	  const F32 bodyAngle = isXtankVehicle() ? mTankHeadingAngle : getRenderAngle();

	  F32 th = FloatHalfPi - bodyAngle;

	  F32 sinTh = sin(th);
	  F32 cosTh = cos(th);
	  F32 warpInScale = (WarpFadeInTime - mWarpInTimer.getCurrent()) / F32(WarpFadeInTime);

	  for(S32 i = 0; i < cornerCount; i++)
	  {
		 shipDirs[i].x = corners[i].x * cosTh + corners[i].y * sinTh;
		 shipDirs[i].y = corners[i].y * cosTh - corners[i].x * sinTh;
		 shipDirs[i] *= warpInScale;
	  }

	  Point leftVec(getActualVel().y, -getActualVel().x);
	  Point rightVec(-getActualVel().y, getActualVel().x);

	  leftVec.normalize();
	  rightVec.normalize();

	  S32 bestId = 0, leftId, rightId;
	  F32 bestDot = leftVec.dot(shipDirs[0]);

	  // Find the left-wards match
	  for(S32 i = 1; i < cornerCount; i++)
	  {
		 F32 d = leftVec.dot(shipDirs[i]);
		 if(d >= bestDot)
		 {
			bestDot = d;
			bestId = i;
		 }
	  }

	  leftId = bestId;
	  Point leftPt = getRenderPos() + shipDirs[bestId];

	  // Find the right-wards match
	  bestId = 0;
	  bestDot = rightVec.dot(shipDirs[0]);

	  for(S32 i = 1; i < cornerCount; i++)
	  {
		 F32 d = rightVec.dot(shipDirs[i]);
		 if(d >= bestDot)
		 {
			bestDot = d;
			bestId = i;
		 }
	  }

	  rightId = bestId;
	  Point rightPt = getRenderPos() + shipDirs[bestId];

	  bool boostActive = mLoadout.isModulePrimaryActive(ModuleBoost);
	  bool cloakActive = mLoadout.isModulePrimaryActive(ModuleCloak);

	  // Select profile
	  UI::TrailProfile profile;

	  if(cloakActive)
		 profile = UI::CloakedShipProfile;
	  else if(boostActive)
		 profile = UI::TurboShipProfile;
	  else
		 profile = UI::ShipProfile;

	  // Stitch things up if we must...
	  if(leftId == mLastTrailPoint[0] && rightId == mLastTrailPoint[1])
	  {
		 mTrail[0].update(leftPt, profile);
		 mTrail[1].update(rightPt, profile);
		 mLastTrailPoint[0] = leftId;
		 mLastTrailPoint[1] = rightId;
	  }
	  else if(leftId == mLastTrailPoint[1] && rightId == mLastTrailPoint[0])
	  {
		 mTrail[1].update(leftPt, profile);
		 mTrail[0].update(rightPt, profile);
		 mLastTrailPoint[1] = leftId;
		 mLastTrailPoint[0] = rightId;
	  }
	  else
	  {
		 mTrail[0].update(leftPt, profile);
		 mTrail[1].update(rightPt, profile);
		 mLastTrailPoint[0] = leftId;
		 mLastTrailPoint[1] = rightId;
	  }

	  if(cloakActive)
		 return;

	  // -------------------------------------------------------------------------
	  // Particle exhaust
	  // -------------------------------------------------------------------------

	  if(isXtankVehicle())
	  {
		 // Xtank tank exhaust: emit from the rear of the hull, directed backward.
		 // Characteristics vary by engine type:
		 //   Light    – 1 dim red-brown point spark, short life
		 //   Standard – 2 orange-red point sparks
		 //   Heavy    – 3 bright orange/yellow line sparks, long life (most visible)

		 static const F32 MinExhaustSpeed = 30.0f;   // below this, no visible exhaust
		 static const F32 ExhaustSpeedNorm = 600.0f;  // max speed for speedFrac normalisation
		 static const F32 ExhaustDistRatio = 0.65f;   // fraction of collision radius to exhaust exit
		 static const F32 SparkProbMultiplier = 1.4f;    // scales emit probability (>1 = emit more at full speed)

		 const F32 speed = mTankSpeed;
		 if(fabsf(speed) < MinExhaustSpeed)
			return;  // barely moving -- skip exhaust

		 const F32 speedFrac = MIN(fabsf(speed) / ExhaustSpeedNorm, 1.0f);

		 // heading unit vector; exhaust exits the REAR (opposite to heading when
		 // moving forward, but keep it consistent: always from the geometric rear)
		 const Point headingDir(cos(mTankHeadingAngle), sin(mTankHeadingAngle));
		 const F32   exhaustDist = xtankBodyCollisionRadius[(S32)mVehicleDesign.body] * ExhaustDistRatio;
		 // When moving forward the rear is behind (−heading); reversing → front
		 const F32   rearSign = (speed >= 0.0f) ? -1.0f : 1.0f;
		 const Point exhaustPos = getRenderPos() + headingDir * (exhaustDist * rearSign);

		 // Exhaust velocity direction (sparks fly backward)
		 const Point exhaustDir = headingDir * rearSign;

		 S32         sparkCount;
		 Color       colorDim, colorBright;
		 S32         maxTTL;
		 UI::SparkType sType;

		 switch(mVehicleDesign.engine)
		 {
		 case XtankEngine::Small_Electric:
		 case XtankEngine::Small_Combustion:
		 case XtankEngine::Small_Turbine:
			sparkCount = 1;
			colorDim = Color(0.50f, 0.10f, 0.00f);  // dark red-brown
			colorBright = Color(0.80f, 0.30f, 0.05f);  // dim orange-red
			maxTTL = 450;
			sType = UI::SparkTypePoint;
			break;

		 case XtankEngine::Medium_Electric:
		 case XtankEngine::Medium_Combustion:
		 case XtankEngine::Medium_Turbine:
		 case XtankEngine::Fuel_Cell:
			sparkCount = 2;
			colorDim = Color(0.80f, 0.20f, 0.00f);  // red
			colorBright = Color(1.00f, 0.60f, 0.10f);  // orange
			maxTTL = 650;
			sType = UI::SparkTypePoint;
			break;

		 default:
			sparkCount = 3;
			colorDim = Color(1.00f, 0.45f, 0.00f);  // orange
			colorBright = Color(1.00f, 0.90f, 0.25f);  // bright yellow
			maxTTL = 900;
			sType = UI::SparkTypeLine;   // line sparks = distinct look
			break;
		 }

		 TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");
		 ClientGame *cg = static_cast<ClientGame *>(getGame());

		 for(S32 s = 0; s < sparkCount; s++)
		 {
			// Probabilistically skip sparks at lower speeds
			if(TNL::Random::readF() > speedFrac * SparkProbMultiplier)
			   continue;

			Point jitter(TNL::Random::readF() * 8.0f - 4.0f,
			   TNL::Random::readF() * 8.0f - 4.0f);
			const F32 sparkSpeed = 50.0f + speedFrac * 100.0f;
			const Point sparkVel = exhaustDir * sparkSpeed + jitter * 15.0f;

			Color sparkColor;
			sparkColor.interp(TNL::Random::readF(), colorDim, colorBright);

			cg->emitSpark(exhaustPos + jitter * 0.4f, sparkVel, sparkColor,
			   TNL::Random::readI(0, maxTTL), sType);
		 }
	  }
	  else
	  {
		 // Standard BF ship: emit sparks in the direction matching each of the
		 // four compass points relative to the ship's heading.
		 Point velDir(mCurrentMove.x, mCurrentMove.y);
		 F32 len = velDir.len();

		 if(len > 0)
		 {
			if(len > 1)
			   velDir *= 1 / len;

			static Point sd[4];
			sd[0].set(cos(getRenderAngle()), sin(getRenderAngle()));
			sd[1].set(-sd[0]);
			sd[2].set(sd[0].y, -sd[0].x);
			sd[3].set(-sd[0].y, sd[0].x);

			for(U32 i = 0; i < 4; i++)
			{
			   F32 dot = sd[i].dot(velDir);

			   if(dot > 0.1)
			   {
				  // shoot some sparks...
				  if(dot >= 0.2 * velDir.len())
				  {
					 Point chaos(TNL::Random::readF(), TNL::Random::readF());
					 chaos *= 5;

					 // interp give us some nice enginey colors...
					 Color dim(Colors::red);
					 Color light(1, 1, boostActive ? 1.f : 0.f);
					 Color thrust;

					 F32 t = TNL::Random::readF();
					 thrust.interp(t, light, dim);

					 TNLAssert(dynamic_cast<ClientGame *>(getGame()) != NULL, "Not a ClientGame");

					 static_cast<ClientGame *>(getGame())->emitSpark(getRenderPos() - sd[i] * 13,
						-sd[i] * 100 + chaos, thrust, TNL::Random::readI(0, 1500), UI::SparkTypePoint);
				  }
			   }
			}
		 }
	  }
#endif
   }


   // Client only
   void Ship::updateTrails()
   {
#ifndef ZAP_DEDICATED
	  for(U32 i = 0; i < TrailCount; i++)
		 mTrail[i].idle(mCurrentMove.time);
#endif
   }


   void Ship::renderLayer(S32 layerIndex)
   {
	  TNLAssert(getGame()->getGameType(), "gameType should always be valid here");

#ifndef ZAP_DEDICATED
	  if(layerIndex == 0)  // Only render on layers -1 and 1
		 return;

	  if(!shouldRender())      // Don't render an exploded ship!
		 return;

	  ClientGame *clientGame = static_cast<ClientGame *>(getGame());
	  GameConnection *conn = clientGame->getConnectionToServer();

	  ClientInfo *clientInfo = getClientInfo();    // Could be NULL

	  // This is the local player's ship -- could be NULL
	  Ship *localShip = dynamic_cast<Ship *>(conn->getControlObject());

	  const bool isLocalShip = !(conn && conn->getControlObject() != this);    // i.e. the ship belongs to the player viewing the rendering
	  const bool isAuthenticated = clientInfo ? clientInfo->isAuthenticated() : false;

	  const bool boostActive = mLoadout.isModulePrimaryActive(ModuleBoost);
	  const bool shieldActive = mLoadout.isModulePrimaryActive(ModuleShield);
	  const bool repairActive = mLoadout.isModulePrimaryActive(ModuleRepair) && mHealth < 1;
	  const bool sensorActive = doesShipActivateSensor(localShip);
	  const bool hasArmor = hasModule(ModuleArmor);

	  const Point vel(mCurrentMove.x, mCurrentMove.y);

	  // If the local player is cloaked, and is close enough to this ship, it will activate a sensor module,
	  // and we'll need to draw it.  Here, we determine if that has happened.

	  const bool isBusy = clientInfo ? clientInfo->isBusy() : false;
	  const bool engineeringTeleport = clientInfo ? clientInfo->isEngineeringTeleporter() : false;
	  const bool showCoordinates = clientGame->isShowingDebugShipCoords();

	  // Caclulate rotAmount to add the spinny effect you see when a ship spawns or comes through a teleport
	  F32 warpInScale = (WarpFadeInTime - mWarpInTimer.getCurrent()) / F32(WarpFadeInTime);

	  const string shipName = clientInfo ? clientInfo->getName().getString() : "";
	  const U32 killStreak = clientInfo ? clientInfo->getKillStreak() : 0;
	  const U32 gamesPlayed = clientInfo ? clientInfo->getGamesPlayed() : 0;

	  const Color *color = getGame()->getGameType()->getTeamColor(this);
	  const Color &healthBarColor = getGame()->getGameType()->getTeamHealthBarColor(this);
	  F32 alpha = getShipVisibility(localShip);

	  F32 angle = getRenderAngle();   // aim angle: where the mouse/reticle points

	  // For xtank vehicle bodies the hull faces mTankHeadingAngle (the authoritative
	  // physics heading) while the aim angle controls the turret(s).  For the
	  // standard BF ship body and aim angle are the same.
	  const F32 aimAngle = angle;
	  const F32 bodyAngle = isXtankVehicle() ? mTankHeadingAngle : angle;

	  F32 deltaAngle = getAngleDiff(mLastProcessStateAngle, bodyAngle);

	  renderShip(layerIndex, getRenderPos(), getActualPos(), vel, bodyAngle, deltaAngle,
		 getActiveShipShapeInfo(), color, healthBarColor, alpha, clientGame->getCurrentTime(), shipName, warpInScale,
		 isLocalShip, isBusy, isAuthenticated, showCoordinates, mHealth, mRadius, getTeam(),
		 boostActive, shieldActive, repairActive, sensorActive, hasArmor, engineeringTeleport, killStreak,
		 gamesPlayed);

	  // Draw xtank turrets on top of the hull, pointing at the aim direction.
	  // Only layer 1 is the visible pass; layer -1 is the cloaking shadow pass.
	  if(isXtankVehicle() && layerIndex == 1)
	  {
		 renderXtankTurrets(getRenderPos(), bodyAngle, aimAngle, alpha,
			xtankTurretInfos[(S32)mVehicleDesign.body], color, warpInScale);

		 // Draw heat-sink and engine-type bling overlaid on the hull.
		 renderXtankVehicleOverlay(getRenderPos(), bodyAngle, alpha, mVehicleDesign.body, mVehicleDesign.heatSinks, mVehicleDesign.engine, warpInScale);
	  }

	  if(mSpawnShield.getCurrent() != 0)  // Add spawn shield -- has a period of being on solidly, then blinks yellow
		 renderSpawnShield(getRenderPos(), mSpawnShield.getCurrent(), clientGame->getCurrentTime());

	  if(mLoadout.isModulePrimaryActive(ModuleRepair) && alpha != 0)     // Don't bother when completely transparent
		 renderShipRepairRays(getRenderPos(), this, mRepairTargets, alpha);

	  // Render mounted items
	  for(S32 i = 0; i < mMountedItems.size(); i++)
		 if(mMountedItems[i].isValid())
			mMountedItems[i]->renderItemAlpha(getRenderPos(), alpha);
#endif
   }


   bool Ship::shouldRender() const
   {
	  return !mHasExploded;
   }


   // Determine if the specified ship will activate our sensor.  Ship can be NULL.
   bool Ship::doesShipActivateSensor(const Ship *ship)
   {
	  if(!ship)
		 return false;

	  // If ship is cloaking, and we have sensor...
	  if(ship->mLoadout.isModulePrimaryActive(ModuleCloak) && hasModule(ModuleSensor))
	  {
		 // ...then check the distance
		 F32 distanceSquared = (ship->getActualPos() - getActualPos()).lenSquared();

		 return (distanceSquared < sq(ModuleInfo::SensorCloakOuterDetectionDistance));
	  }

	  return false;
   }


   // Set to true to allow players to see their cloaked teammates,
   // false if cloaked teammates should be invisible
   static const bool SHOW_CLOAKED_TEAMMATES = true;

   // Determine ship's visibility, 0 = invisible, 1 = normal visibility.
   // Value will be used as alpha when rendering ship.
   F32 Ship::getShipVisibility(const Ship *localShip)
   {
	  bool isLocalShip = (localShip == this);

	  const bool cloakActive = mLoadout.isModulePrimaryActive(ModuleCloak);
	  const F32 cloakFraction = mCloakTimer.getFraction();

	  // If ship is using cloak module, we'll reduce its visibility
	  F32 alpha = cloakActive ? cloakFraction : 1 - cloakFraction;

	  S32 localTeam = localShip ? localShip->getTeam() : NO_TEAM;
	  bool teamGame = mGame->getGameType()->isTeamGame();
	  bool isFriendly = (getTeam() == localTeam) && teamGame;

	  // Don't completely hide local player or ships on same team (in a team game)
	  if(isLocalShip || (SHOW_CLOAKED_TEAMMATES && isFriendly))
		 return max(alpha, 0.25f);     // Make sure we have at least .25 alpha

	  // Apply rules to cloaked players not on your team

	  static const F32 MAX_ALPHA = 0.4f;

	  // If we have sensor equipped and this non-local ship is cloaked
	  if(localShip && localShip->hasModule(ModuleSensor) && cloakActive)
	  {
		 // Do a distance check - cloaked ships are detected at a reduced distance
		 F32 distanceSquared = (localShip->getActualPos() - getActualPos()).lenSquared();

		 // Ship is within outer detection radius
		 if(distanceSquared < sq(ModuleInfo::SensorCloakOuterDetectionDistance))
		 {
			// Inside inner radius? De-cloak a maximum of MAX_ALPHA
			if(distanceSquared < sq(ModuleInfo::SensorCloakInnerDetectionDistance))
			   return MAX_ALPHA;

			// Otherwise de-cloak proportionally to the distance between inner and outer detection radii
			F32 ratio = (sq(ModuleInfo::SensorCloakOuterDetectionDistance) - distanceSquared) /
			   (sq(ModuleInfo::SensorCloakOuterDetectionDistance) - sq(ModuleInfo::SensorCloakInnerDetectionDistance));

			return sq(ratio) * MAX_ALPHA;  // Non-linear
		 }
	  }

	  return alpha;
   }


   S32 Ship::getMountedItemCount() const
   {
	  return mMountedItems.size();
   }


   MountableItem *Ship::getMountedItem(S32 index) const
   {
	  if(index < 0 || index >= mMountedItems.size())
		 return NULL;

	  return mMountedItems[index];
   }


   void Ship::addMountedItem(MountableItem *item)
   {
	  TNLAssert(item->getMount() == this, "Mounting to wrong ship!  Maybe try item.mountToShip(&ship);");
	  mMountedItems.push_back(item);
   }


   // Supposes mountedItems are not repeated, and list is unordered
   void Ship::removeMountedItem(MountableItem *item)
   {
	  for(S32 i = 0; i < mMountedItems.size(); i++)
		 if(mMountedItems[i] == item)
		 {
			mMountedItems.erase_fast(i);
			return;
		 }
   }


   bool Ship::isRobot()
   {
	  return mIsRobot;
   }


   //// Lua methods

   //               Fn name           Param profiles  Profile count
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, isAlive,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getPlayerInfo,   ARRAYDEF({{ END }}), 1 ) \
                                                           \
   METHOD(CLASS, isModActive,     ARRAYDEF({{ MOD_ENUM, END }}), 1 ) \
   METHOD(CLASS, getEnergy,       ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, setEnergy,       ARRAYDEF({{ NUM, END }}), 1 ) \
   METHOD(CLASS, getHealth,       ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, setHealth,       ARRAYDEF({{ NUM, END }}), 1 ) \
   METHOD(CLASS, hasFlag,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getFlagCount,    ARRAYDEF({{ END }}), 1 ) \
                                                           \
   METHOD(CLASS, getAngle,        ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getActiveWeapon, ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getMountedItems, ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getLoadout,      ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, setLoadout,      ARRAYDEF({{ TABLE, END }, { INT, INT, INT, INT, INT, END }}), 2 ) \
   METHOD(CLASS, setLoadoutNow,   ARRAYDEF({{ TABLE, END }, { INT, INT, INT, INT, INT, END }}), 2 ) \


   GENERATE_LUA_FUNARGS_TABLE(Ship, LUA_METHODS);
   GENERATE_LUA_METHODS_TABLE(Ship, LUA_METHODS);

#undef LUA_METHODS


   const char *Ship::luaClassName = "Ship";
   REGISTER_LUA_SUBCLASS(Ship, MoveObject);


   // The following block injects a special item into the docs where documentation about the constructor would go
   /**
	* @luafuncsheader Ship
	*
	* \warning There is no Lua constructor for Ships; they cannot be created from a script.  Sorry!
	* You might be able to do what you want by spawning a Robot.
	*/



	// Note: All of these methods will return nil if the ship in question has been deleted.

	/**
	 * @luafunc bool Ship::isAlive()
	 *
	 * @brief Check if this Ship is alive.
	 *
	 * @return `true` if the Ship is present in the game world and still alive,
	 * `false` otherwise.
	 */
   S32 Ship::lua_isAlive(lua_State *L) { return returnBool(L, !isDestroyed()); }

   /**
	* @luafunc bool Ship::hasFlag()
	*
	* @brief Check if this Ship is carrying a flag.
	*
	* @return `true` if the Ship is carrying at least one flag, `false` otherwise.
	*/
   S32 Ship::lua_hasFlag(lua_State *L) { return returnBool(L, getFlagCount() > 0); }

   /**
	* @luafunc int Ship::getFlagCount()
	*
	* @brief Get the number of flags carried by this Ship.
	*
	* @return The number of flags carried by this Ship.
	*/
   S32 Ship::lua_getFlagCount(lua_State *L) { return returnInt(L, getFlagCount()); }


   /**
	* @luafunc PlayerInfo Ship::getPlayerInfo()
	*
	* @brief Get the PlayerInfo for this Ship.
	*
	* @return The PlayerInfo for this Ship.
	*/
   S32 Ship::lua_getPlayerInfo(lua_State *L) { return returnPlayerInfo(L, this); }

   /**
	* @luafunc num Ship::getAngle()
	*
	* @brief Get the angle of the Ship.
	*
	* @return The Ship's angle in radians.
	*/
   S32 Ship::lua_getAngle(lua_State *L) { return returnFloat(L, getCurrentMove().angle); }  // Get angle ship is pointing at


   /**
	* @luafunc Weapon Ship::getActiveWeapon()
	*
	* @brief Checks if the given module is active.
	*
	* @descr This will return an item of the \ref WeaponEnum enum, e.g.
	* `Weapon.Phaser`.
	*
	* @return The \ref WeaponEnum that is currently active on this ship.
	*/
   S32 Ship::lua_getActiveWeapon(lua_State *L)
   {
	  return returnWeaponType(L, mLoadout.getActiveWeapon());
   }


   /**
	* @luafunc bool Ship::isModActive(Module module)
	*
	* @brief Checks if the given module is active.
	*
	* @descr This method takes a \ref ModuleEnum item as a parameter, e.g.
	* `Module.Shield`
	*
	* @param module The \ref ModuleEnum to check.
	*
	* @return `true` if the given \ref ModuleEnum is in active use, false
	* otherwise.
	*/
   S32 Ship::lua_isModActive(lua_State *L) {
	  checkArgList(L, functionArgs, luaClassName, "isModActive");

	  ShipModule module = getShipModule(L, 1);

	  return returnBool(L, mLoadout.isModulePrimaryActive(module) || mLoadout.isModuleSecondaryActive(module));
   }


   /**
	* @luafunc num Ship::getEnergy()
	*
	* @brief Gets the enegy of this ship.
	*
	* @descr Energy is specified as a number between 0 and 1 where 0 means no
	* energy and 1 means full energy.
	*
	* @return Returns a value between 0 and 1 indicating the energy of the item.
	*/
   S32 Ship::lua_getEnergy(lua_State *L)
   {
	  // Return ship's energy as a fraction between 0 and 1
	  return returnFloat(L, (F32)mEnergy / (F32)EnergyMax);
   }


   /**
	* @luafunc Ship::setEnergy(num energy)
	*
	* @brief Set the current energy of this ship.
	*
	* @descr Energy is specified as a number between 0 and 1 where 0 means no
	* energy and 1 means full energy.  Values outside this range will be clamped to
	* the valid range.
	*
	* @param energy A value between 0 and 1.
	*/
   S32 Ship::lua_setEnergy(lua_State *L)
   {
	  checkArgList(L, functionArgs, "Ship", "setEnergy");

	  F32 param = getFloat(L, 1);
	  S32 newEnergy = S32(CLAMP(param, 0.0f, 1.0f) * (F32)EnergyMax);

	  // Determine our credit to be sent to the client, and send it
	  S32 credit = CLAMP(newEnergy - mEnergy, -Ship::EnergyMax, Ship::EnergyMax);

	  GameConnection *cc = getControllingClient();
	  if(cc)
		 cc->s2cCreditEnergy(credit);

	  // Now set the server-side energy to the new one
	  mEnergy = newEnergy;

	  return 0;
   }


   /**
	* @luafunc num Ship::getHealth()
	*
	* @brief Returns the health of this ship.
	*
	* @descr Health is specified as a number between 0 and 1 where 0 is completely
	* dead and 1 is full health.
	*
	* @return Returns a value between 0 and 1 indicating the health of the item.
	*/
   S32 Ship::lua_getHealth(lua_State *L)
   {
	  // Return ship's health as a fraction between 0 and 1
	  return returnFloat(L, getHealth());
   }


   /**
	* @luafunc Ship::setHealth(num health)
	*
	* @brief Set the current health of this ship.
	*
	* @descr Health is specified as a number between 0 and 1 where 0 is completely
	* dead and 1 is full health. Values outside this range will be clamped to the
	* valid range.
	*
	* @param health A value between 0 and 1.
	*
	* @note A setting of 0 will kill the ship instantly.
	*/
   S32 Ship::lua_setHealth(lua_State *L)
   {
	  checkArgList(L, functionArgs, "Ship", "setHealth");

	  F32 param = getFloat(L, 1);

	  mHealth = CLAMP(param, 0.0f, 1.0f);

	  // Transmit with next packet
	  setMaskBits(HealthMask);

	  if(mHealth <= 0)
	  {
		 lua_pop(L, 1);  // Remove from stack as event gets triggered immediately

		 DamageInfo di;
		 di.damagingObject = NULL;
		 killAndScore(&di);
	  }

	  return 0;
   }


   /**
	* @luafunc table Ship::getMountedItems()
	*
	* @brief Get all Items carried by this Ship.
	*
	* @return A table of all Items mounted on ship (e.g. ResourceItems and Flags)
	*/
   S32 Ship::lua_getMountedItems(lua_State *L)
   {
	  bool hasArgs = lua_isnumber(L, 1);
	  Vector<BfObject *> tempVector;

	  // Loop through all the mounted items
	  for(S32 i = 0; i < mMountedItems.size(); i++)
	  {
		 // Add every item to the list if no arguments were specified
		 if(!hasArgs)
			tempVector.push_back(dynamic_cast<BfObject *>(mMountedItems[i].getPointer()));

		 // Else, compare against argument type and add to the list if matched
		 else
		 {
			S32 index = 1;
			while(lua_isnumber(L, index))
			{
			   U8 objectType = (U8)lua_tointeger(L, index);

			   if(mMountedItems[i]->getObjectTypeNumber() == objectType)
			   {
				  tempVector.push_back(dynamic_cast<BfObject *>(mMountedItems[i].getPointer()));
				  break;
			   }

			   index++;
			}
		 }
	  }

	  clearStack(L);

	  lua_createtable(L, mMountedItems.size(), 0);    // Create a table, with enough slots pre-allocated for our data

	  // Now push all found items back to LUA
	  S32 pushed = 0;      // Count of items actually pushed onto the stack

	  for(S32 i = 0; i < tempVector.size(); i++)
	  {
		 tempVector[i]->push(L);
		 pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
		 lua_rawseti(L, 1, pushed);
	  }

	  return 1;
   }


   /**
	* @luafunc table Ship::getLoadout()
	*
	* @brief Get the current loadout
	*
	* @descr This method will return a table with the loadout in the following
	* order:
	*
	* `Module 1, Module 2, Weapon 1, Weapon 2, Weapon 3`
	*
	* @return A table with the current loadout, as described above.
	*/
   S32 Ship::lua_getLoadout(lua_State *L)
   {
	  // Create our loadout table
	  lua_createtable(L, ShipModuleCount + ShipWeaponCount, 0);

	  // Add current modules and weapons to the table
	  for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
	  {
		 // Modules
		 if(i < ShipModuleCount)
			lua_pushinteger(L, (S32)mLoadout.getModule(i));

		 // Weapons are offset by total module type count
		 else
			lua_pushinteger(L, (S32)mLoadout.getWeapon(i - ShipModuleCount) + ModuleCount);

		 // Lua uses 1-based arrays
		 lua_rawseti(L, 1, i + 1);
	  }

	  // Return our table
	  return 1;
   }


   LoadoutTracker Ship::checkAndBuildLoadout(lua_State *L, S32 profile)
   {
	  S32 expectedSize = ShipModuleCount + ShipWeaponCount;

	  Vector<S32> loadoutValues(expectedSize);
	  LoadoutTracker loadout;

	  // A table
	  if(profile == 0)
	  {
		 lua_pushnil(L);                             // table, nil
		 while(lua_next(L, 1) != 0)                  // table, key, value
		 {
			loadoutValues.push_back(getInt2<S32>(L, -1));
			lua_pop(L, 1);                           // table, key
		 }
	  }
	  // 5 parameters all integers, the argument list check guarantees 5 params here
	  else
	  {
		 for(S32 i = 0; i < expectedSize; i++)
			loadoutValues.push_back(getInt2<S32>(L, i + 1));
	  }

	  // Make sure we have the appropriate number of loadout values
	  if(loadoutValues.size() != expectedSize)
		 THROW_LUA_EXCEPTION(L, string("The loadout given must contain " + itos(expectedSize) + " elements").c_str());


	  // Now we verify and build up our loadout
	  S32 moduleCount = 0;
	  S32 weaponCount = 0;
	  for(S32 i = 0; i < expectedSize; i++)
	  {
		 S32 value = loadoutValues[i];

		 // Test if a weapon - the integer will be greater than ModuleCount
		 if(value >= (S32)ModuleCount)
		 {
			if(weaponCount >= ShipWeaponCount)
			   THROW_LUA_EXCEPTION(L, string("Too many weapons!  You must provide exactly " + itos(ShipWeaponCount) + " weapons.").c_str());

			loadout.setWeapon(weaponCount, WeaponType(value - ModuleCount));
			weaponCount++;
		 }
		 else
		 {
			if(moduleCount >= ShipWeaponCount)
			   THROW_LUA_EXCEPTION(L, string("Too many modules!  You must provide exactly " + itos(ShipModuleCount) + " modules.").c_str());

			loadout.setModule(moduleCount, ShipModule(value));
			moduleCount++;
		 }
	  }

	  // If we made it here without throwing an exception, then we have a loadout
	  // with proper number of weapons/modules!
	  return loadout;
   }


   bool Ship::isBitfighterShip() const
   {
	  return mVehicleDesign.body == XtankBody::BITFIGHTER_SHIP;
   }


   bool Ship::isXtankVehicle() const
   {
	  return mVehicleDesign.body != XtankBody::BITFIGHTER_SHIP;
   }


   const VehicleDesign *Ship::getXtankDesign() const
   {

	  return &mVehicleDesign;
   }


   const XtankWeaponState &Ship::getXtankWeaponState(S32 slot) const
   {
	  return mWeaponStates[slot];
   }


   /**
	* @luafunc Ship::setLoadout(Weapon w1, Weapon w2, Weapon w3, Module m1, Module m2)
	* @brief Convenience alias for setLoadout(table)
	*
	* @param w1 The new \ref WeaponEnum for slot 1.
	* @param w2 The new \ref WeaponEnum for slot 2.
	* @param w3 The new \ref WeaponEnum for slot 3.
	* @param m1 The new \ref ModuleEnum for slot 1.
	* @param m2 The new \ref ModuleEnum for slot 2.
	*
	* @luafunc Ship::setLoadout(table loadout)
	*
	* @brief Sets the requested loadout for the ship.
	*
	* @descr When setting the loadout, normal rules apply for updating the
	* loadout, e.g. moving over a loadout zone.
	*
	* This method will take a table with 5 entries in any order comprised of
	* 3 weapons and 2 modules.
	*
	* @note This method will also take 5 parameters as a new loadout, instead
	* of a table. See setLoadout(Weapon, Weapon, Weapon, Module, Module)
	*
	* @param loadout The new loadout to request.
	*
	* @see setLoadoutNow()
	*/
   S32 Ship::lua_setLoadout(lua_State *L)
   {
	  S32 profile = checkArgList(L, functionArgs, luaClassName, "setLoadout");

	  LoadoutTracker loadout = checkAndBuildLoadout(L, profile);

	  getOwner()->requestDesign(&loadout);

	  return 0;
   }


   /**
	* @luafunc Ship::setLoadoutNow(table loadout)
	*
	* @brief Immediately sets the loadout for the ship.
	*
	* @descr This method does not require that you follow normal loadout-switching
	* rules.
	*
	* The parameters for this method follow the same rules as Ship::setLoadout().
	*
	* @param loadout The new loadout to set.
	*
	* @see setLoadout(loadout)
	*/
   S32 Ship::lua_setLoadoutNow(lua_State *L)
   {
	  S32 profile = checkArgList(L, functionArgs, luaClassName, "setLoadoutNow");

	  LoadoutTracker loadout = checkAndBuildLoadout(L, profile);

	  if(loadout.isValidForLevel(getGame()->getGameType()->isEngineerEnabled()))
	  {
		 // Set requested loadout so we don't revert if going to a loadout zone
		 // (this may set the loadout now if the ship is in a loadout zone)
		 getClientInfo()->requestDesign(&loadout);

		 // Set current loadout
		 setLoadout(loadout);
	  }
	  else
		 THROW_LUA_EXCEPTION(L, "The loadout given is invalid");

	  return 0;
   }

   // LuaItem methods that override one in a parent class
   S32 Ship::lua_setPos(lua_State *L)
   {
	  S32 r = Parent::lua_setPos(L);
	  Teleporter::checkAllTeleporters(this);

	  // If the object isn't in a game yet (mGame == NULL), two problems arise:
	  // 1. No clients to update — there's no network context, so the call is meaningless.
	  // 2. Dirty list leak — the object gets added to mDirtyList but processDeleteList / game idle
	  //    will never drain it. If the object is then deleted before ever being added to a game, the
	  //    dirty list holds a dangling pointer, which corrupts the next idle cycle.
	  if(getGame())
		 setMaskBits(PositionMask);  // Update clients

	  return r;
   }

   // This completely overrides the BfObject::lua_setTeam() method
   S32 Ship::lua_setTeam(lua_State *L)
   {
	  checkArgList(L, functionArgs, "BfObject", "setTeam");
	  S32 newTeam = getTeamIndex(L, 1);

	  if(!getGame())    // Not yet in a game; just set team directly
	  {
		 setTeam(newTeam);
		 return 0;
	  }

	  // Update team on server and clients, with appropriate logic
	  GameType *gameType = getGame()->getGameType();
	  gameType->changeClientTeam(mClientInfo, newTeam);

	  return 0;
   }

}  // namespace Zap
