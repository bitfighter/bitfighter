//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

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



}


#endif