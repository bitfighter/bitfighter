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

#ifndef SOUNDSYSTEM_H_
#define SOUNDSYSTEM_H_

#include "ConfigEnum.h"     // For sfxSets
#include "Timer.h"

#ifdef ZAP_DEDICATED
#  define BF_NO_AUDIO
#endif

#ifndef BF_NO_AUDIO
#include "../openal/alInclude.h"
#include "../alure/AL/alure.h"
#else
class alureStream;
#endif

#include "tnlTypes.h"
#include "tnlVector.h"

#include <string>

// forward declarations
typedef unsigned int ALuint;

namespace TNL {
   template <class T> class RefPtr;
   class ByteBuffer;
   typedef RefPtr<ByteBuffer> ByteBufferPtr;
};

using namespace TNL;
using namespace std;

namespace Zap {

// Forward declarations
class Point;
class SoundEffect;
class UIManager;
typedef RefPtr<SoundEffect> SFXHandle;

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

   SFXBurstProjectile, 

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

   // Core-related noises
   SFXCoreHeartbeat,
   SFXCoreExplode,
   SFXCorePanelExplode,

   // Other noises
   SFXAchievementEarned,

   NumSFXBuffers     // Count of the number of SFX sounds we have
};

enum MusicCommand {
   MusicCommandNone,
   MusicCommandStop,     // Instantly stop music
   MusicCommandPlay,     // Play/resume music (no fading)
   MusicCommandPause,    // Pause music
   MusicCommandFadeIn,   // Start and fade in music
   MusicCommandFadeOut,  // Fade out and stop music
};

enum MusicState {
   MusicStateNone,
   MusicStateFadingIn,
   MusicStateFadingOut,
   MusicStatePlaying,
   MusicStateStopped,
   MusicStatePaused,
   // Interim states
   MusicStateLoading,
   MusicStateStopping,
   MusicStatePausing,
   MusicStateResuming,
};

enum MusicLocation {
   MusicLocationNone,
   MusicLocationMenus,
   MusicLocationGame,
   MusicLocationCredits,
   MusicLocationEditor,
};

struct MusicData
{
   MusicLocation currentLocation;   // Music location (in menus, in game, etc..)
   MusicLocation previousLocation;
   MusicCommand command;            // Command to target a different music state
   MusicState state;                // Current music state
   ALfloat volume;
   ALuint source;
   alureStream* stream;
};


class SoundSystem
{
private:
   static const S32 NumMusicStreamBuffers = 3;
   static const S32 MusicChunkSize = 250000;
   static const S32 NumVoiceChatBuffers = 32;
   static const S32 NumSamples = 16;

   // Sound Effect functions
   static void updateGain(SFXHandle& effect, F32 sfxVolLevel, F32 voiceVolLevel);
   static void playOnSource(SFXHandle& effect, F32 sfxVol, F32 voiceVol);

   static void music_end_callback(void* userData, ALuint source);
   static void menu_music_end_callback(void* userData, ALuint source);

   static MusicData mMusicData;

   static string mMusicDir;
   static string mMenuMusicFile;
   static string mCreditsMusicFile;

   static bool mMenuMusicValid;
   static bool mGameMusicValid;
   static bool mCreditsMusicValid;

   static Vector<string> mGameMusicList;
   static S32 mCurrentlyPlayingIndex;

   static Timer mMusicFadeTimer;
   static const U32 MusicFadeOutDelay = 500;
   static const U32 MusicFadeInDelay = 1000;

   static bool musicSystemValid();

public:
   SoundSystem();
   virtual ~SoundSystem();

   // General functions
   static void init(sfxSets sfxSet, const string &sfxDir, const string &musicDir, float musicVol);
   static void shutdown();
   static void setListenerParams(Point pos, Point velocity);
   static void processAudio(U32 timeDelta, F32 sfxVol, F32 musicVol, F32 voiceVol, UIManager *uiManager);  // Client version
   static void processAudio(F32 sfxVol);                                                                   // Server version

   // Sound Effect functions
   static void processSoundEffects(F32 sfxVol, F32 voiceVol);
   static SFXHandle playSoundEffect(U32 profileIndex, F32 gain = 1.0f);
   static SFXHandle playSoundEffect(U32 profileIndex, Point position);
   static SFXHandle playSoundEffect(U32 profileIndex, Point position, Point velocity, F32 gain = 1.0f);
   static void playSoundEffect(const SFXHandle &effect);
   static SFXHandle playRecordedBuffer(ByteBufferPtr p, F32 gain);
   static void stopSoundEffect(SFXHandle &effect);
   static void unqueueBuffers(S32 sourceIndex);
   static void setMovementParams(SFXHandle& effect, Point position, Point velocity);
   static void updateMovementParams(SFXHandle& effect);

   // Voice Chat functions
   static void processVoiceChat();
   static void queueVoiceChatBuffer(const SFXHandle &effect, ByteBufferPtr p);
   static bool startRecording();
   static void captureSamples(ByteBufferPtr sampleBuffer);
   static void stopRecording();

   // Music functions
   static void processMusic(U32 timeDelta, F32 musicVol, MusicLocation musicLocation);
   static void playMusic();
   static void stopMusic();
   static void pauseMusic();
   static void resumeMusic();
   static void fadeInMusic();
   static void fadeOutMusic();
   static void playNextTrack();
   static void playPrevTrack();

   static bool isMusicPlaying();
};

}

#endif /* SOUNDSYSTEM_H_ */
