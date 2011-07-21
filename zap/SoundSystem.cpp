/*
 * SoundSystem.cpp
 *
 *  Created on: May 8, 2011
 *      Author: dbuck
 */

#include "SoundSystem.h"
#include "SoundEffect.h"
#include "Point.h"
#include "tnlLog.h"
#include "tnlByteBuffer.h"

#if !defined (ZAP_DEDICATED) && !defined (TNL_OS_XBOX)

#include "alInclude.h"
#include "../alure/AL/alure.h"

#include "SFXProfile.h"
#include "config.h"
#include "stringUtils.h"

#include "tnlNetBase.h"

using namespace TNL;

namespace Zap {

//
// Must keep this aligned with SFXProfiles!!
//

//   fileName     isRelative gainScale isLooping  fullGainDistance  zeroGainDistance
static SFXProfile sfxProfilesModern[] = {
 // Utility sounds
 {  "phaser.wav",          true,  1.0f,  false, 0,   0 },      // SFXVoice -- "phaser.wav" is a dummy here
 {  "phaser.wav",          false, 0.45f, false, 150, 600 },    // SFXNone  -- as above

 // Players joining/leaving noises -- aren't really relative, but true allows them to play properly...
 {  "player_joined.wav",   true,  0.8f,  false, 150, 600 },
 {  "player_left.wav",     true,  0.8f,  false, 150, 600 },

 // Weapon noises
 {  "phaser.wav",          false, 0.45f, false, 150, 600 },
 {  "tink.wav",            false, 0.9f,  false, 150, 600 },
 {  "bounce.wav",          false, 0.45f, false, 150, 600 },
 {  "bounce_impact.wav",   false, 0.7f,  false, 150, 600 },
 {  "triple.wav",          false, 0.45f, false, 150, 600 },
 {  "triple_impact.wav",   false, 0.7f,  false, 150, 600 },
 {  "turret.wav",          false, 0.45f, false, 150, 600 },
 {  "turret_impact.wav",   false, 0.7f,  false, 150, 600 },

 {  "grenade.wav",         false, 0.9f,  false, 300, 600 },

 {  "mine_deploy.wav",     false, 0.4f,  false, 150, 600 },
 {  "mine_arm.wav",        false, 0.7f,  false, 400, 600 },
 {  "mine_explode.wav",    false, 0.8f,  false, 300, 800 },

 {  "spybug_deploy.wav",   false, 0.4f,  false, 150, 600 },
 {  "spybug_explode.wav",  false, 0.8f,  false, 300, 800 },

 { "asteroid_explode.wav", false, 0.80f, false,  150, 800 },

 // Ship noises
 {  "ship_explode.wav",    false, 1.0,   false, 300, 1000 },
 {  "ship_heal.wav",       false, 1.0,   false, 300, 1000 },
 {  "ship_turbo.wav",      false, 0.15f, true,  150, 500 },
 {  "ship_hit.wav",        false, 1.0,   false, 150, 600 },    // Ship is hit by a projectile

 {  "bounce_wall.wav",     false, 0.7f,  false, 150, 600 },
 {  "bounce_obj.wav",      false, 0.7f,  false, 150, 600 },
 {  "bounce_shield.wav",   false, 0.7f,  false, 150, 600 },

 {  "ship_shield.wav",     false, 0.15f, true,  150, 500 },
 {  "ship_sensor.wav",     false, 0.15f, true,  150, 500 },
 {  "ship_repair.wav",     false, 0.15f, true,  150, 500 },
 {  "ship_cloak.wav",      false, 0.35f, true,  150, 500 },

 // Flag noises
 {  "flag_capture.wav",    true,  0.45f, false, 0,   0 },
 {  "flag_drop.wav",       true,  0.45f, false, 0,   0 },
 {  "flag_return.wav",     true,  0.45f, false, 0,   0 },
 {  "flag_snatch.wav",     true,  0.45f, false, 0,   0 },

 // Teleport noises
 {  "teleport_in.wav",     false, 1.0,   false, 200, 500 },
 {  "teleport_out.wav",    false, 1.0,   false, 200, 500 },
 {  "gofast.wav",          false, 1.0,   false, 200, 500 },    // Heard outside the ship
 {  "gofast.wav",          true, 1.0,    false, 200, 500 },    // Heard inside the ship

 // Forcefield noises
 {  "forcefield_up.wav",   false,  0.7f,  false, 150, 600 },
 {  "forcefield_down.wav", false,  0.7f,  false, 150, 600 },

 // UI noises
 {  "boop.wav",            true,  0.4f,  false, 150, 600 },
 {  "comm_up.wav",         true,  0.4f,  false, 150, 600 },
 {  "comm_down.wav",       true,  0.4f,  false, 150, 600 },
 {  "boop.wav",            true,  0.25f, false, 150, 600 },

 {  NULL, false, 0, false, 0, 0 },
};


//   fileName     isRelative gainScale isLooping  fullGainDistance  zeroGainDistance

static SFXProfile sfxProfilesClassic[] = {
 // Utility sounds
 {  "phaser.wav",          true,  1.0f,  false, 0,   0 },
 {  "phaser.wav",          false, 0.45f, false, 150, 600 },

 // Players joining/leaving noises -- aren't really relative, but true allows them to play properly...
 {  "player_joined.wav",   true,  0.8f,  false, 150, 600 },
 {  "player_left.wav",     true,  0.8f,  false, 150, 600 },

 // Weapon noises
 {  "phaser.wav",          false, 0.45f, false, 150, 600 },
 {  "phaser_impact.wav",   false, 0.7f,  false, 150, 600 },
 {  "bounce.wav",          false, 0.45f, false, 150, 600 },
 {  "bounce_impact.wav",   false, 0.7f,  false, 150, 600 },
 {  "triple.wav",          false, 0.45f, false, 150, 600 },
 {  "triple_impact.wav",   false, 0.7f,  false, 150, 600 },
 {  "turret.wav",          false, 0.45f, false, 150, 600 },
 {  "turret_impact.wav",   false, 0.7f,  false, 150, 600 },

 {  "grenade.wav",         false, 0.9f,  false, 300, 600 },

 {  "mine_deploy.wav",     false, 0.4f,  false, 150, 600 },
 {  "mine_arm.wav",        false, 0.7f,  false, 400, 600 },
 {  "mine_explode.wav",    false, 0.8f,  false, 300, 800 },

 {  "spybug_deploy.wav",   false, 0.4f,  false, 150, 600 },
 {  "spybug_explode.wav",  false, 0.8f,  false, 300, 800 },

 { "asteroid_explode.wav", false, 0.8f, false,  150, 800 },

 // Ship noises
 {  "ship_explode.wav",    false, 1.0,   false, 300, 1000 },
 {  "ship_heal.wav",       false, 1.0,   false, 300, 1000 },
 {  "ship_turbo.wav",      false, 0.15f, true,  150, 500 },
 {  "ship_hit.wav",        false, 1.0,   false, 150, 600 },    // Ship is hit by a projectile

 {  "bounce_wall.wav",     false, 0.7f,  false, 150, 600 },
 {  "bounce_obj.wav",      false, 0.7f,  false, 150, 600 },
 {  "bounce_shield.wav",   false, 0.7f,  false, 150, 600 },

 {  "ship_shield.wav",     false, 0.15f, true,  150, 500 },
 {  "ship_sensor.wav",     false, 0.15f, true,  150, 500 },
 {  "ship_repair.wav",     false, 0.15f, true,  150, 500 },
 {  "ship_cloak.wav",      false, 0.35f, true,  150, 500 },

 // Flag noises
 {  "flag_capture.wav",    true,  0.45f, false, 0,   0 },
 {  "flag_drop.wav",       true,  0.45f, false, 0,   0 },
 {  "flag_return.wav",     true,  0.45f, false, 0,   0 },
 {  "flag_snatch.wav",     true,  0.45f, false, 0,   0 },

 // Teleport noises
 {  "teleport_in.wav",     false, 1.0,   false, 200, 500 },
 {  "teleport_out.wav",    false, 1.0,   false, 200, 500 },

 {  "gofast.wav",          false, 1.0,   false, 200, 500 },    // Heard outside the ship
 {  "gofast.wav",          true, 1.0,   false, 200, 500 },     // Heard inside the ship

 // Forcefield noises
 {  "forcefield_up.wav",   false,  0.7f,  false, 150, 600 },
 {  "forcefield_down.wav", false,  0.7f,  false, 150, 600 },

 // UI noises
 {  "boop.wav",            true,  0.4f,  false, 150, 600 },
 {  "comm_up.wav",         true,  0.4f,  false, 150, 600 },
 {  "comm_down.wav",       true,  0.4f,  false, 150, 600 },
 {  "boop.wav",            true,  0.25f, false, 150, 600 },

 {  NULL, false, 0, false, 0, 0 },
};


// TODO clean up this rif-raff; maybe put in config.h?
static bool gSFXValid = false;
static bool gMusicValid = false;
F32 mMaxDistance = 500;
Point mListenerPosition;
Point mListenerVelocity;

SFXProfile *gSFXProfiles;

static ALuint gSfxBuffers[NumSFXBuffers];
static Vector<ALuint> gFreeSources;
static Vector<ALuint> gVoiceFreeBuffers;

// Music specific
static Vector<SFXHandle> gPlayList;
static alureStream* musicStream;
static ALuint musicSource;  // dedicated source for Music
static MusicState musicState;
static ALfloat musicVolume = 0;

static Vector<string> musicList;
static S32 currentlyPlayingIndex;

extern ConfigDirectories gConfigDirs;
extern bool gDedicatedServer;

extern string joindir(const string &path, const string &filename);

SoundSystem::SoundSystem()
{
}

SoundSystem::~SoundSystem()
{
}

// Initialize the sound sub-system.
// Use ALURE to ease the use of OpenAL.
void SoundSystem::init()
{
   // Initialize the sound device
   if(!alureInitDevice(NULL, NULL))
   {
      logprintf(LogConsumer::LogError, "Failed to open OpenAL device: %s\n", alureGetErrorString());
      return;
   }

   // Create source pool for all game sounds
   gFreeSources.resize(NumSamples);
   alGenSources(NumSamples, gFreeSources.address());
   if(alGetError() != AL_NO_ERROR)
   {
      logprintf(LogConsumer::LogError, "Failed to create OpenAL sources!\n");
      return;
   }

   // Create sound buffers for the sound effect pool
   alGenBuffers(NumSFXBuffers, gSfxBuffers);
   if(alGetError() != AL_NO_ERROR)
   {
      logprintf(LogConsumer::LogError, "Failed to create OpenAL buffers!\n");
      return;
   }

   // OpenAL normally defaults the distance model to AL_INVERSE_DISTANCE
   // which is the "physically correct" model, however we'll use the simple
   // linear model
   alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
   if(alGetError() != AL_NO_ERROR)
      logprintf(LogConsumer::LogWarning, "Failed to set proper sound gain distance model!  Sounds will be off..\n");

   // Choose the sound set
   if(gIniSettings.sfxSet == sfxClassicSet)
      gSFXProfiles = sfxProfilesClassic;
   else
      gSFXProfiles = sfxProfilesModern;

   // Iterate through all sounds
   for(U32 i = 0; i < NumSFXBuffers; i++)
   {
      // End when we find a sound sans filename
      if(!gSFXProfiles[i].fileName)
         break;

      // Grab sound file location
      string fileBuffer = joindir(gConfigDirs.sfxDir, gSFXProfiles[i].fileName);

      // Stick sound into a buffer
      if(alureBufferDataFromFile(fileBuffer.c_str(), gSfxBuffers[i]) == AL_FALSE)
      {
         logprintf(LogConsumer::LogError, "Failure (1) loading sound file '%s': Game will proceed without sound.", fileBuffer.c_str());
         return;
      }
   }

   // Set up music list for streaming later
   if (!getFilesFromFolder(gConfigDirs.musicDir, musicList))
   {
      logprintf(LogConsumer::LogWarning, "Could not read music files from folder \"%s\".  Game will proceed without music", gConfigDirs.musicDir.c_str());
   }
   else
   {
      // Create dedicated music source
      alGenSources(1, &musicSource);

      // Set up other relevant data
      currentlyPlayingIndex = 0;
      gMusicValid = true;
      musicState = MusicStopped;

      // Initialize to the proper volume level
      musicVolume = gIniSettings.musicVolLevel;
      alSourcef(musicSource, AL_GAIN, musicVolume);
   }

   // Set up voice chat buffers
   gVoiceFreeBuffers.resize(NumVoiceChatBuffers);
   alGenBuffers(NumVoiceChatBuffers, gVoiceFreeBuffers.address());

   gSFXValid = true;
}

void SoundSystem::shutdown()
{
   if(!gSFXValid)
      return;

   // Stop and clean up music
   if(gMusicValid) {
   stopMusic();
   alureDestroyStream(musicStream, 0, NULL);
      alDeleteSources(1, &musicSource);
   }

   // Clean up SoundEffect buffers
   alDeleteBuffers(NumSFXBuffers, gSfxBuffers);

   // Clean up voice buffers
   for (S32 i = 0; i < 32; i++)
      alDeleteBuffers(1, &(gVoiceFreeBuffers[i]));
   gVoiceFreeBuffers.clear();

   // Clean up sources
   for (S32 i = 0; i < NumSamples; i++)
      alDeleteSources(1, &(gFreeSources[i]));
   gFreeSources.clear();

   // Shutdown device cv9
   alureShutdownDevice();
}

void SoundSystem::setListenerParams(Point pos, Point velocity)
{
   if(!gSFXValid)
      return;

   mListenerPosition = pos;
   mListenerVelocity = velocity;
   alListener3f(AL_POSITION, pos.x, pos.y, -mMaxDistance/2);
}

SFXHandle SoundSystem::playRecordedBuffer(ByteBufferPtr p, F32 gain)
{
   SFXHandle ret = new SoundEffect(0, p, gain, Point(), Point());
   playSoundEffect(ret);
   return ret;
}

SFXHandle SoundSystem::playSoundEffect(U32 profileIndex, F32 gain)
{
   SFXHandle ret = new SoundEffect(profileIndex, NULL, gain, Point(), Point());
   playSoundEffect(ret);
   return ret;
}

SFXHandle SoundSystem::playSoundEffect(U32 profileIndex, Point position, Point velocity, F32 gain)
{
   SFXHandle ret = new SoundEffect(profileIndex, NULL, gain, position, velocity);
   playSoundEffect(ret);
   return ret;
}


void SoundSystem::playSoundEffect(SFXHandle& effect)
{
   if(!gSFXValid)
      return;

   if(effect->mSourceIndex != -1)
      return;
   else
   {
      // See if it's aleady on the play list:
      for(S32 i = 0; i < gPlayList.size(); i++)
         if(effect == gPlayList[i].getPointer())
            return;
      if(gPlayList.size() < 100)  // limit sounds, as search takes too long when list is big
         gPlayList.push_back(effect);
   }
}


void SoundSystem::stopSoundEffect(SFXHandle& effect)
{
   if(!gSFXValid)
      return;

   // Remove from the play list, if this sound is playing
   if(effect->mSourceIndex != -1)
   {
      alSourceStop(gFreeSources[effect->mSourceIndex]);
      effect->mSourceIndex = -1;
   }
   for(S32 i = 0; i < gPlayList.size(); i++)
   {
      if(gPlayList[i].getPointer() == effect)
      {
         gPlayList.erase(i);
         return;
      }
   }
}


void SoundSystem::unqueueBuffers(S32 sourceIndex)
{
   // free up any played buffers from this source.
   if(sourceIndex != -1)
   {
      ALint processed;
      alGetError();

      alGetSourcei(gFreeSources[sourceIndex], AL_BUFFERS_PROCESSED, &processed);

      while(processed)
      {
         ALuint buffer;
         alSourceUnqueueBuffers(gFreeSources[sourceIndex], 1, &buffer);
         if(alGetError() != AL_NO_ERROR)
            return;

         processed--;

         // ok, this is a lame solution - but the way OpenAL should work is...
         // you should only be able to unqueue buffers that you queued - duh!
         // otherwise it's a bitch to manage sources that can either be streamed
         // or already loaded.
         U32 i;
         for(i = 0 ; i < NumSFXBuffers; i++)
            if(buffer == gSfxBuffers[i])
               break;
         if(i == NumSFXBuffers)
            gVoiceFreeBuffers.push_back(buffer);
      }
   }
}

void SoundSystem::setMovementParams(SFXHandle& effect, Point position, Point velocity)
{
   if(!gSFXValid)
      return;

   effect->mPosition = position;
   effect->mVelocity = velocity;
   if(effect->mSourceIndex != -1)
      updateMovementParams(effect);
}

void SoundSystem::updateMovementParams(SFXHandle& effect)
{
   ALuint source = gFreeSources[effect->mSourceIndex];
   if(effect->mProfile->isRelative)
   {
      alSourcei(source, AL_SOURCE_RELATIVE, true);
      alSource3f(source, AL_POSITION, 0, 0, 0);
      //alSource3f(source, AL_VELOCITY, 0, 0, 0);
   }
   else
   {
      alSourcei(source, AL_SOURCE_RELATIVE, false);
      alSource3f(source, AL_POSITION, effect->mPosition.x, effect->mPosition.y, 0);
      //alSource3f(source, AL_VELOCITY, mVelocity.x, mVelocity.y, 0);
   }
}


void SoundSystem::processAudio()
{
   processSoundEffects();
   processMusic();
   processVoiceChat();

   alureUpdate();
}


void SoundSystem::processMusic()
{
   if (!gMusicValid)
      return;

   // Adjust music volume only if changed
   if (S32(gIniSettings.musicVolLevel * 10) != S32(musicVolume * 10)) {
      musicVolume = gIniSettings.musicVolLevel;
      alSourcef(musicSource, AL_GAIN, musicVolume);
   }

   // Once currentlyPlayingIndex is greater than list size, we are done with the list
   // We probably don't want this behaviour in the end
   if(musicState == MusicStopped && currentlyPlayingIndex < musicList.size())
      playMusicList();
}


void SoundSystem::processVoiceChat()
{
   // TODO move voiceChat logic here
}


void SoundSystem::processSoundEffects()
{
   if(!gSFXValid)
      return;

   // Ok, so we have a list of currently "playing" sounds, which is
   // unbounded in length, but only the top NumSources actually have sources
   // associtated with them.  Sounds are prioritized on a 0-1 scale
   // based on type and distance.
   //
   // Each time through the main loop, newly played sounds are placed
   // on the process list.  When SFXProcess is called, any finished sounds
   // are retired from the list, and then it prioritizes and sorts all
   // the remaining sounds.  For any sounds from 0 to NumSources that don't
   // have a current source, they grab one of the sources not used by the other
   // top sounds.  At this point, any sound that is not looping, and is
   // not in the active top list is retired.

   // Now, look through all the currently playing sources and see which
   // ones need to be retired:

   bool sourceActive[NumSamples];
   for(S32 i = 0; i < NumSamples; i++)
   {
      ALint state;
      unqueueBuffers(i);
      alGetSourcei(gFreeSources[i], AL_SOURCE_STATE, &state);
      sourceActive[i] = state != AL_STOPPED && state != AL_INITIAL;
   }
   for(S32 i = 0; i < gPlayList.size(); )
   {
      SFXHandle &s = gPlayList[i];

      if(s->mSourceIndex != -1 && !sourceActive[s->mSourceIndex])
      {
         // this sound was playing; now it is stopped,
         // so remove it from the list.
         s->mSourceIndex = -1;
         gPlayList.erase_fast(i);
      }
      else
      {
         // compute a priority for this sound.
         if(!s->mProfile->isRelative)
            s->mPriority = (500 - (s->mPosition - mListenerPosition).len()) / 500.0f;
         else
            s->mPriority = 1.0;
         i++;
      }
   }
   // Now, bubble sort all the sounds up the list:
   // we choose bubble sort, because the list should
   // have a lot of frame-to-frame coherency, making the
   // sort most often O(n)
   for(S32 i = 1; i < gPlayList.size(); i++)
   {
      F32 priority = gPlayList[i]->mPriority;
      for(S32 j = i - 1; j >= 0; j--)
      {
         if(priority > gPlayList[j]->mPriority)
         {
            SFXHandle temp = gPlayList[j];
            gPlayList[j] = gPlayList[j+1];
            gPlayList[j+1] = temp;
         }
      }
   }
   // Last, release any sources and get rid of non-looping sounds
   // outside our max sound limit
   for(S32 i = NumSamples; i < gPlayList.size(); )
   {
      SFXHandle &s = gPlayList[i];
      if(s->mSourceIndex != -1)
      {
         sourceActive[s->mSourceIndex] = false;
         s->mSourceIndex = -1;
      }
      if(!s->mProfile->isLooping)
         gPlayList.erase_fast(i);
      else
         i++;
   }
   // Assign sources to all sounds that need them
   S32 firstFree = 0;
   S32 max = NumSamples;
   if(max > gPlayList.size())
      max = gPlayList.size();

   for(S32 i = 0; i < max; i++)
   {
      SFXHandle &s = gPlayList[i];
      if(s->mSourceIndex == -1)
      {
         while(firstFree < NumSamples-1 && sourceActive[firstFree])
            firstFree++;
         s->mSourceIndex = firstFree;
         sourceActive[firstFree] = true;
         playOnSource(s);
      }
      else
         updateGain(s);     // For other sources, check the distance and adjust the gain
   }
}

void SoundSystem::playOnSource(SFXHandle& effect)
{
   ALuint source = gFreeSources[effect->mSourceIndex];
   alSourceStop(source);
   unqueueBuffers(effect->mSourceIndex);

   if(effect->mInitialBuffer.isValid())
   {
      if(!gVoiceFreeBuffers.size())
         return;

      ALuint buffer = gVoiceFreeBuffers.first();
      gVoiceFreeBuffers.pop_front();

      alSourcei(source, AL_BUFFER, 0);  // clear old buffers

      alBufferData(buffer, AL_FORMAT_MONO16, effect->mInitialBuffer->getBuffer(),
            effect->mInitialBuffer->getBufferSize(), 8000);
      alSourceQueueBuffers(source, 1, &buffer);
   }
   else
      alSourcei(source, AL_BUFFER, gSfxBuffers[effect->mSFXIndex]);

   alSourcei(source, AL_LOOPING, effect->mProfile->isLooping);
   alSourcef(source, AL_REFERENCE_DISTANCE, effect->mProfile->fullGainDistance);
   alSourcef(source, AL_MAX_DISTANCE, effect->mProfile->zeroGainDistance);
   alSourcef(source, AL_ROLLOFF_FACTOR, 1);

   updateMovementParams(effect);
   updateGain(effect);

   alSourcePlay(source);
}


// Recalculate distance, and reset gain as necessary
void SoundSystem::updateGain(SFXHandle& effect)
{
   ALuint source = gFreeSources[effect.getPointer()->mSourceIndex];

   // First check if it's a voice chat... voice volume is handled separately.
   if(effect.getPointer()->mSFXIndex == SFXVoice)
   {
      alSourcef(source, AL_GAIN, gIniSettings.voiceChatVolLevel);
      return;
   }

   alSourcef(source, AL_GAIN, effect.getPointer()->mGain * effect.getPointer()->mProfile->gainScale * (gDedicatedServer ? gIniSettings.alertsVolLevel : gIniSettings.sfxVolLevel));
}


// This only called when playing an incoming voice chat message
void SoundSystem::queueVoiceChatBuffer(SFXHandle& effect, ByteBufferPtr p)
{
   if(!gSFXValid)
      return;

   if(!p->getBufferSize())
      return;

   effect->mInitialBuffer = p;
   if(effect->mSourceIndex != -1)
   {
      if(!gVoiceFreeBuffers.size())
         return;

      ALuint source = gFreeSources[effect->mSourceIndex];

      ALuint buffer = gVoiceFreeBuffers.first();
      gVoiceFreeBuffers.pop_front();

      S16 max = 0;
      S16 *ptr = (S16 *) p->getBuffer();
      U32 count = p->getBufferSize() / 2;
      while(count--)
      {
         if(max < *ptr)
            max = *ptr;
         ptr++;
      }

      alBufferData(buffer, AL_FORMAT_MONO16, effect->mInitialBuffer->getBuffer(),
            effect->mInitialBuffer->getBufferSize(), 8000);
      alSourceQueueBuffers(source, 1, &buffer);

      ALint state;
      alGetSourcei(source, AL_SOURCE_STATE, &state);
      if(state == AL_STOPPED)
         alSourcePlay(source);
   }
   else
      playSoundEffect(effect);
}



void SoundSystem::music_end_callback(void* userdata, ALuint source)
{
   logprintf("finished playing: %s", musicList[currentlyPlayingIndex].c_str());

   musicState = MusicStopped;
   currentlyPlayingIndex++;
}

void SoundSystem::playMusicList()
{
   playMusic(currentlyPlayingIndex);
}

void SoundSystem::playMusic(S32 listIndex)
{
   musicState = MusicPlaying;

   string musicFile = joindir(gConfigDirs.musicDir, musicList[listIndex]);
   musicStream = alureCreateStreamFromFile(musicFile.c_str(), MusicChunkSize, 0, NULL);

   if(!musicStream)
   {
      logprintf(LogConsumer::LogError, "Failed to create music stream for: %s", musicList[listIndex].c_str());
   }

   if(!alurePlaySourceStream(musicSource, musicStream, NumMusicStreamBuffers, 0, music_end_callback, NULL))
   {
      logprintf(LogConsumer::LogError, "Failed to play music file: %s", musicList[listIndex].c_str());
   }
}

void SoundSystem::stopMusic()
{
   alureStopSource(musicSource, AL_FALSE);
}

}

#elif defined (ZAP_DEDICATED)

using namespace TNL;

namespace Zap
{


void SoundSystem::updateGain(SFXHandle& effect)
{
}

void SoundSystem::updateMovementParams(SFXHandle& effect)
{
}

void SoundSystem::playOnSource(SFXHandle& effect)
{
}

void SoundSystem::setMovementParams(SFXHandle& effect, Point position, Point velocity)
{
}

SFXHandle SoundSystem::playSoundEffect(U32 profileIndex, F32 gain)
{
   return new SoundEffect(0,NULL,0,Point(0,0), Point(0,0));
}

SFXHandle SoundSystem::playSoundEffect(U32 profileIndex, Point position, Point velocity, F32 gain)
{
   return new SoundEffect(0,NULL,0,Point(0,0), Point(0,0));
}

SFXHandle SoundSystem::playRecordedBuffer(ByteBufferPtr p, F32 gain)
{
   return new SoundEffect(0,NULL,0,Point(0,0), Point(0,0));
}

void SoundSystem::playSoundEffect(SFXHandle& effect)
{
}

void SoundSystem::stopSoundEffect(SFXHandle& effect)
{
}

void SoundSystem::queueVoiceChatBuffer(SFXHandle& effect, ByteBufferPtr p)
{
}

void SoundSystem::init()
{
   logprintf(LogConsumer::LogError, "No OpenAL support on this platform.");
}

void SoundSystem::processAudio()
{
}

void SoundSystem::setListenerParams(Point pos, Point velocity)
{
}

void SoundSystem::shutdown()
{
};

};

#endif


#if defined(ZAP_DEDICATED)
namespace Zap
{
bool SoundSystem::startRecording()
{
   return false;
}

void SoundSystem::captureSamples(ByteBufferPtr buffer)
{
}

void SoundSystem::stopRecording()
{
}

};

#else

namespace Zap
{

// requires OpenAL version 1.1 (OpenAL-Soft might be required)

static ALCdevice *captureDevice;

bool SoundSystem::startRecording()
{
   captureDevice = alcCaptureOpenDevice(NULL, 8000, AL_FORMAT_MONO16, 2048);
   if(alGetError() != AL_NO_ERROR || captureDevice == NULL)
      return false;

   alcCaptureStart(captureDevice);
   return true;
}

void SoundSystem::captureSamples(ByteBufferPtr buffer)
{
   U32 startSize = buffer->getBufferSize();
   ALint sample;

   alcGetIntegerv(captureDevice, ALC_CAPTURE_SAMPLES, 1, &sample);

   U32 endSize = startSize + sample*2;  // convert number of samples (2 byte mono) to number of bytes, but keep sample size unchanged for alcCaptureSamples

   buffer->resize(endSize);
   alcCaptureSamples(captureDevice, (ALCvoid *) &buffer->getBuffer()[startSize], sample);
}

void SoundSystem::stopRecording()
{
   alcCaptureStop(captureDevice);
   alcCaptureCloseDevice(captureDevice);
   captureDevice = NULL;
}

};

#endif
