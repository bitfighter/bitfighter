//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef SOUNDSYSTEM_ENUMS_H_
#define SOUNDSYSTEM_ENUMS_H_

namespace Zap {


// TODO: Make this an X-MACRO!  
// Must keep this aligned with sfxProfilesModern[] and sfxProfilesClassic[]
enum SFXProfiles
{
   // Utility sounds
   SFXVoice,
   SFXNone,

   SFXPlayerJoined,
   SFXPlayerLeft,

   // Weapon noises
   SFXPhaserProjectile,
   SFXPhaserImpact,
   SFXBounceProjectile,
   SFXBounceImpact,
   SFXTripleProjectile,
   SFXTripleImpact,
   SFXTurretProjectile,
   SFXTurretImpact,

   SFXBurst, 

   SFXMineDeploy,
   SFXMineArm,
   SFXMineExplode,

   SFXSpyBugDeploy,
   SFXSpyBugExplode,

   SFXAsteroidSmallExplode,
   SFXAsteroidMediumExplode,
   SFXAsteroidLargeExplode,

   // Ship noises
   SFXShipExplode,
   SFXShipHeal,      // Ship picked up a health pack or energy item
   SFXShipBoost,
   SFXShipHit,       // Ship is hit by a projectile

   SFXBounceWall,
   SFXBounceObject,
   SFXBounceShield,

   SFXShieldActive,
   SFXSensorActive,
   SFXRepairActive,
   SFXCloakActive,

   // Flag noises
   SFXFlagCapture,
   SFXFlagDrop,
   SFXFlagReturn,
   SFXFlagSnatch,

   // Teleport noises
   SFXTeleportIn,
   SFXTeleportOut,
   SFXTeleportExploding,

   // GoFast noises
   SFXGoFastOutside,
   SFXGoFastInside,

   // Forcefield noises
   SFXForceFieldUp,
   SFXForceFieldDown,

   // UI noises
   SFXUIBoop,
   SFXUICommUp,
   SFXUICommDown,
   SFXIncomingMessage,

   // Core-related noises
   SFXCoreHeartbeat,
   SFXCoreExplode,
   SFXCorePanelExplode,

   // Seekers
   SFXSeekerFire,

   // Other noises
   SFXAchievementEarned,

   NumSFXBuffers     // Count of the number of SFX sounds we have
};


enum MusicLocation {
   MusicLocationNone,
   MusicLocationMenus,
   MusicLocationGame,
   MusicLocationCredits,
   MusicLocationEditor
};


}


#endif
