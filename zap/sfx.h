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

#ifndef _SFX_H_
#define _SFX_H_

#include "../tnl/tnl.h"
#include "../tnl/tnlNetBase.h"
#include "../tnl/tnlByteBuffer.h"

#include "point.h"


using namespace TNL;

namespace Zap
{

   // Must keep this aligned with sfxProfilesModern[] and sfxProfilesClassic[]
enum SFXProfiles
{
   // Utility sounds
   SFXVoice,
   SFXNone,

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

   // Forcefield noises
   SFXForceFieldUp,
   SFXForceFieldDown,

   SFXPlayerJoined,
   SFXPlayerLeft,

   // UI noises
   SFXUIBoop,
   SFXUICommUp,
   SFXUICommDown,
   SFXIncomingMessage,

   NumSFXBuffers     // Count of the number of SFX sounds we have
};

struct SFXProfile
{
   const char *fileName;
   bool        isRelative;
   F32         gainScale;
   bool        isLooping;
   F32         fullGainDistance;
   F32         zeroGainDistance;
};

class SFXObject : public Object
{
   static Point mListenerPosition;
   static Point mListenerVelocity;
   static F32 mMaxDistance;
   static S32 mCaptureGain;

   U32 mSFXIndex;
   Point mPosition;
   Point mVelocity;
   ByteBufferPtr mInitialBuffer;
   SFXProfile *mProfile;
   F32 mGain;
   S32 mSourceIndex;
   F32 mPriority;
   void playOnSource();
   void updateGain();
   void updateMovementParams();
public:
   SFXObject(U32 profileIndex, ByteBufferPtr samples, F32 gain, Point position, Point velocity);
   ~SFXObject();

   void setGain(F32 gain);
   void play();
   void stop();
   void setMovementParams(Point position, Point velocity);
   void queueBuffer(ByteBufferPtr b);
   bool isPlaying() { return mSourceIndex != -1; }

   static bool startRecording();
   static void captureSamples(ByteBufferPtr sampleBuffer);
   static void stopRecording();

   static void setMaxDistance(F32 maxDistance);
   static void init();
   static void shutdown();
   static void process();
   static void setListenerParams(Point position, Point velocity);
   static RefPtr<SFXObject> play(U32 profileIndex, F32 gain = 1.0f);
   static RefPtr<SFXObject> play(U32 profileIndex, Point position, Point velocity, F32 gain = 1.0f);
   static RefPtr<SFXObject> playRecordedBuffer(ByteBufferPtr b, F32 gain = 1.0f);
};

typedef RefPtr<SFXObject> SFXHandle;

extern void SFXInit();
extern void SFXShutdown();
extern void SFXProcess();
extern void SFXSetListenerParams(Point position, Point velocity);

};

#endif


