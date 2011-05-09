/*
 * SoundSystem.h
 *
 *  Created on: May 8, 2011
 *      Author: dbuck
 */

#ifndef SOUNDSYSTEM_H_
#define SOUNDSYSTEM_H_

#include "tnlTypes.h"
#include "Point.h"

// forward declarations
namespace TNL {
   template <class T> class RefPtr;
   class ByteBuffer;
   typedef RefPtr<ByteBuffer> ByteBufferPtr;
};

namespace Zap {

// forward declarations
class SoundEffect;
typedef TNL::RefPtr<SoundEffect> SFXHandle;

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

   SFXGrenadeProjectile,

   SFXMineDeploy,
   SFXMineArm,
   SFXMineExplode,

   SFXSpyBugDeploy,
   SFXSpyBugExplode,

   SFXAsteroidExplode,

   // Ship noises
   SFXShipExplode,
   SFXShipHeal,
   SFXShipBoost,
   SFXShipHit,    // Ship is hit by a projectile

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

   NumSFXBuffers     // Count of the number of SFX sounds we have
};

class SoundSystem
{

private:
   // Sound Effect functions
   static void playOnSource(SFXHandle& effect);
   static void updateGain(SFXHandle& effect);

public:
   static const TNL::S32 NumSamples = 16;

   SoundSystem();
   virtual ~SoundSystem();

   // General functions
   static void init();
   static void shutdown();
   static void setListenerParams(Point pos, Point velocity);

   // Sound Effect functions
   static void processSoundEffects();
   static SFXHandle playSoundEffect(TNL::U32 profileIndex, TNL::F32 gain = 1.0f);
   static SFXHandle playSoundEffect(TNL::U32 profileIndex, Point position, Point velocity, TNL::F32 gain = 1.0f);
   static void playSoundEffect(SFXHandle& effect);
   static SFXHandle playRecordedBuffer(TNL::ByteBufferPtr p, TNL::F32 gain);
   static void stopSoundEffect(SFXHandle& effect);
   static void setSoundEffectGain(SFXHandle& effect, TNL::F32 gain);
   static void updateSoundEffectGain(SFXHandle& effect);
   static void unqueueBuffers(TNL::S32 sourceIndex);
   static void setMovementParams(SFXHandle& effect, Point position, Point velocity);
   static void updateMovementParams(SFXHandle& effect);

   // Voice Chat functions
   static void queueVoiceChatBuffer(SFXHandle& effect, TNL::ByteBufferPtr p);
   static bool startRecording();
   static void captureSamples(TNL::ByteBufferPtr sampleBuffer);
   static void stopRecording();

};

}

#endif /* SOUNDSYSTEM_H_ */
