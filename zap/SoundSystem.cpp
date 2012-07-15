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

#include "SoundSystem.h"
#include "SoundEffect.h"
#include "Point.h"
#include "tnlLog.h"
#include "tnlByteBuffer.h"

#if !defined (NO_AUDIO)

#include "SFXProfile.h"
#include "config.h"
#include "stringUtils.h"
#include "UI.h"
#include "UIManager.h"

#include "tnlNetBase.h"

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

 // Core-related noises
 {  "core_heartbeat.wav",        false, 1.0f,  false, 150, 1000 },
 {  "core_explode.wav",          false, 1.0f,  false, 300, 1000 },
 {  "core_panel_explode.wav",    false, 2.0f,  false, 300, 1000 },

 // Other
 {  "achievement_earned.wav",  true,  1.0f, false, 0,   0 },

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

 // Core-related noises
 {  "core_heartbeat.wav",        false, 1.0f,  false, 150, 1000 },
 {  "core_explode.wav",          false, 1.0f,  false, 300, 1000 },
 {  "core_panel_explode.wav",    false, 2.0f,  false, 300, 1000 },

 // Other
 {  "achievement_earned.wav",  true,  1.0f, false, 0,   0 },

 {  NULL, false, 0, false, 0, 0 },
};


// TODO clean up this rif-raff; maybe put in config.h?
static bool gSFXValid = false;
F32 mMaxDistance = 500;
Point mListenerPosition;
Point mListenerVelocity;

SFXProfile *gSFXProfiles;

static ALuint gSfxBuffers[NumSFXBuffers];
static Vector<ALuint> gFreeSources;
static Vector<ALuint> gVoiceFreeBuffers;
static Vector<SFXHandle> gPlayList;

// Music specific
MusicInfo musicInfos[MaxMusicTypes];

static ALfloat musicVolume = 0;
string SoundSystem::mMusicDir;
string SoundSystem::mMenuMusicFile = "menu.ogg";
bool SoundSystem::mMenuMusicValid = false;
bool SoundSystem::mGameMusicValid = false;

Vector<string> SoundSystem::mMusicList;
S32 SoundSystem::mCurrentlyPlayingIndex = 0;

// Constructor
SoundSystem::SoundSystem()
{
   // Do nothing
}


// Destructor
SoundSystem::~SoundSystem()
{
   // Do nothing
}


// Initialize the sound sub-system
// Use ALURE to ease the use of OpenAL
void SoundSystem::init(sfxSets sfxSet, const string &sfxDir, const string &musicDir, float musicVolLevel)
{
   // Initialize the sound device
   if(!alureInitDevice(NULL, NULL))
   {
      logprintf(LogConsumer::LogError, "Failed to open OpenAL device: %s\n", alureGetErrorString());
      return;
   }

   mMusicDir = musicDir;

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
   if(sfxSet == sfxClassicSet)
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
      string fileBuffer = joindir(sfxDir, gSFXProfiles[i].fileName);

      // Stick sound into a buffer
      if(alureBufferDataFromFile(fileBuffer.c_str(), gSfxBuffers[i]) == AL_FALSE)
      {
         logprintf(LogConsumer::LogError, "Failure (1) loading sound file '%s': Game will proceed without sound.", fileBuffer.c_str());
         return;
      }
   }

   // Set up music list for streaming later
   if(!getFilesFromFolder(mMusicDir, mMusicList))
      logprintf(LogConsumer::LogWarning, "Could not read music files from folder \"%s\".  Game will proceed without music", musicDir.c_str());
   else if(mMusicList.size() == 0)
      logprintf(LogConsumer::LogWarning, "No music files found in folder \"%s\".  Game will proceed without music", musicDir.c_str());
   else     // Got some music!
   {
      // Remove the menu music from the file list
      for(S32 i = 0; i < mMusicList.size(); i++)
         if(mMusicList[i] == mMenuMusicFile)
         {
            mMenuMusicValid = true;
            mMusicList.erase(i);
            break;
         }

      if(mMusicList.size() > 0)
         mGameMusicValid = true;

      if(musicSystemValid())
      {
         // Set static volume
         musicVolume = musicVolLevel;

         for(S32 i = 0; i < MaxMusicTypes; i++)
         {
            // Create dedicated music sources
            alGenSources(1, &(musicInfos[i].source));
            musicInfos[i].state = MusicStopped;
            musicInfos[i].type = (MusicInfoType)i;

            // Initialize source to the proper volume level
            alSourcef(musicInfos[i].source, AL_GAIN, musicVolume);
            alSourcei(musicInfos[i].source, AL_SOURCE_RELATIVE, true);
         }
      }
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
   if(musicSystemValid())
   {
      for(S32 i = 0; i < MaxMusicTypes; i++)
      {
         stopMusic(musicInfos[i]);

         alDeleteSources(1, &(musicInfos[i].source));
      }
   }

   // Clean up sources
   for (S32 i = 0; i < NumSamples; i++)
      alDeleteSources(1, &(gFreeSources[i]));

   gFreeSources.clear();

   // Clean up SoundEffect buffers
   alDeleteBuffers(NumSFXBuffers, gSfxBuffers);

   // Clean up voice buffers
   for (S32 i = 0; i < 32; i++)
      alDeleteBuffers(1, &(gVoiceFreeBuffers[i]));
   gVoiceFreeBuffers.clear();

   // Shutdown device cv9
   alureShutdownDevice();
}


bool SoundSystem::musicSystemValid()
{
   return mGameMusicValid || mMenuMusicValid;
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


SFXHandle SoundSystem::playSoundEffect(U32 profileIndex, Point position)
{
   return playSoundEffect(profileIndex, position, Point(0,0));
}


SFXHandle SoundSystem::playSoundEffect(U32 profileIndex, Point position, Point velocity, F32 gain)
{
   SFXHandle ret = new SoundEffect(profileIndex, NULL, gain, position, velocity);
   playSoundEffect(ret);
   return ret;
}


void SoundSystem::playSoundEffect(const SFXHandle &effect)
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


// Client version
void SoundSystem::processAudio(F32 sfxVol, F32 musicVol, F32 voiceVol)
{
   processSoundEffects(sfxVol, voiceVol);
   processMusic(musicVol);
   processVoiceChat();

   alureUpdate();
}


// Server version, with no music or voice
void SoundSystem::processAudio(F32 sfxVol)
{
   processSoundEffects(sfxVol, 0);

   alureUpdate();
}


void SoundSystem::processMusic(F32 newMusicVolLevel)
{
   // If music system failed to initialize, just return
   if(!musicSystemValid())
      return;

   // Adjust music volume only if changed
   if(S32(newMusicVolLevel * 10) != S32(musicVolume * 10)) 
   {
      musicVolume = newMusicVolLevel;

      for(S32 i = 0; i < MaxMusicTypes; i++)
         alSourcef(musicInfos[i].source, AL_GAIN, musicVolume);
   }

   // Determine if we are in-game playing (or elsewhere in other menus)
   bool inGame = UserInterface::current->getMenuID() == GameUI ||
         UserInterface::current->getUIManager()->cameFrom(GameUI);

   bool inEditor = UserInterface::current->getMenuID() == EditorUI ||
         UserInterface::current->getUIManager()->cameFrom(EditorUI);

   if(inGame)
   {
      // If menu music is playing, stop it
      if(musicInfos[MusicTypeMenu].state == MusicPlaying)
         stopMusic(musicInfos[MusicTypeMenu]);

      // No game music tracks found, just return
      if(!mGameMusicValid)
         return;

      // If game music is playing, pause it if volume is set to zero
      if(musicInfos[MusicTypeGame].state == MusicPlaying)
         if(S32(newMusicVolLevel * 10) == 0)
            pauseMusic(musicInfos[MusicTypeGame]);

      // If game music is stopped (end of last track?) continue playing
      if(musicInfos[MusicTypeGame].state == MusicStopped)
         playGameMusic();

      // If the music was paused, resume, as long as the volume isn't zero
      if(musicInfos[MusicTypeGame].state == MusicPaused)
         if(S32(newMusicVolLevel * 10) != 0)
            resumeMusic(musicInfos[MusicTypeGame]);
   }
   // If in editor, no music (for now)
   else if(inEditor)
   {
      if(musicInfos[MusicTypeMenu].state == MusicPlaying)
         stopMusic(musicInfos[MusicTypeMenu]);

      if(musicInfos[MusicTypeGame].state == MusicPlaying)
         stopMusic(musicInfos[MusicTypeGame]);
   }
   // Else, menu music!
   else
   {
      // If we were playing music in-game, just pause it
      if(musicInfos[MusicTypeGame].state == MusicPlaying)
         pauseMusic(musicInfos[MusicTypeGame]);

      // No menu music found, just return
      if(!mMenuMusicValid)
         return;

      // If menu music is playing, pause it if volume is set to zero
      if(musicInfos[MusicTypeMenu].state == MusicPlaying)
         if(S32(newMusicVolLevel * 10) == 0)
            pauseMusic(musicInfos[MusicTypeMenu]);

      // If menu music is stopped, play
      if(musicInfos[MusicTypeMenu].state == MusicStopped)
         playMenuMusic();

      // If menu music is paused, play if volume isn't zero
      if(musicInfos[MusicTypeMenu].state == MusicPaused)
         if(S32(newMusicVolLevel * 10) != 0)
            resumeMusic(musicInfos[MusicTypeMenu]);
   }

}


void SoundSystem::processVoiceChat()
{
   // TODO move voiceChat logic here
}


void SoundSystem::processSoundEffects(F32 sfxVol, F32 voiceVol)
{
   // If SFX system failed to initialize, just return
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
         playOnSource(s, sfxVol, voiceVol);
      }
      else
         updateGain(s, sfxVol, voiceVol);     // For other sources, check the distance and adjust the gain
   }
}


void SoundSystem::playOnSource(SFXHandle& effect, F32 sfxVol, F32 voiceVol)
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
   updateGain(effect, sfxVol, voiceVol);

   alSourcePlay(source);
}


// Recalculate distance, and reset gain as necessary
void SoundSystem::updateGain(SFXHandle& effect, F32 sfxVol, F32 voiceVol)
{
   ALuint source = gFreeSources[effect.getPointer()->mSourceIndex];

   // First check if it's a voice chat... voice volume is handled separately
   if(effect.getPointer()->mSFXIndex == SFXVoice)
      alSourcef(source, AL_GAIN, voiceVol);
   else
      alSourcef(source, AL_GAIN, effect.getPointer()->mGain * effect.getPointer()->mProfile->gainScale * sfxVol);
}


// This only called when playing an incoming voice chat message
void SoundSystem::queueVoiceChatBuffer(const SFXHandle &effect, ByteBufferPtr p)
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


// This method is called after a music track finishes playing in-game
void SoundSystem::game_music_end_callback(void* userdata, ALuint source)
{
//   logprintf("finished playing: %s", musicList[currentlyPlayingIndex].c_str());

   // Set the state to stopped
   musicInfos[MusicTypeGame].state = MusicStopped;

   // Clean up the stream
   alureDestroyStream(musicInfos[MusicTypeGame].stream, 0, NULL);

   // Go to the next track, loop if at the end
   mCurrentlyPlayingIndex = (mCurrentlyPlayingIndex + 1) % mMusicList.size();
}


// This method is called if menu music is stopped
void SoundSystem::menu_music_end_callback(void* userdata, ALuint source)
{
   // Set the state to stopped
   musicInfos[MusicTypeMenu].state = MusicStopped;

   // Clean up the stream
   alureDestroyStream(musicInfos[MusicTypeMenu].stream, 0, NULL);
}


void SoundSystem::playGameMusic()
{
   musicInfos[MusicTypeGame].state = MusicPlaying;

   string musicFile = joindir(mMusicDir, mMusicList[mCurrentlyPlayingIndex]);
   musicInfos[MusicTypeGame].stream = alureCreateStreamFromFile(musicFile.c_str(), MusicChunkSize, 0, NULL);

   if(!musicInfos[MusicTypeGame].stream)
      logprintf(LogConsumer::LogError, "Failed to create music stream for: %s", mMusicList[mCurrentlyPlayingIndex].c_str());

   if(!alurePlaySourceStream(musicInfos[MusicTypeGame].source, musicInfos[MusicTypeGame].stream, NumMusicStreamBuffers, 0, game_music_end_callback, NULL))
      logprintf(LogConsumer::LogError, "Failed to play music file: %s", mMusicList[mCurrentlyPlayingIndex].c_str());
}


void SoundSystem::playMenuMusic()
{
   musicInfos[MusicTypeMenu].state = MusicPlaying;

   string musicFileFullPath = joindir(mMusicDir, mMenuMusicFile);
   musicInfos[MusicTypeMenu].stream = alureCreateStreamFromFile(musicFileFullPath.c_str(), MusicChunkSize, 0, NULL);

   if(!musicInfos[MusicTypeMenu].stream)
      logprintf(LogConsumer::LogError, "Failed to create music stream for: %s", mMenuMusicFile.c_str());

   // Loop forever (parameter -1)
   if(!alurePlaySourceStream(musicInfos[MusicTypeMenu].source, musicInfos[MusicTypeMenu].stream, NumMusicStreamBuffers, -1, menu_music_end_callback, NULL))
      logprintf(LogConsumer::LogError, "Failed to play music file: %s", mMenuMusicFile.c_str());
}


void SoundSystem::stopMusic(MusicInfo &musicInfo)
{
   alureStopSource(musicInfo.source, AL_FALSE);

   // Clean up the stream here since we aren't calling the callback
   if(musicInfo.stream)
      alureDestroyStream(musicInfo.stream, 0, NULL);

   musicInfo.state = MusicStopped;
}


void SoundSystem::pauseMusic(MusicInfo &musicInfo)
{
   alurePauseSource(musicInfo.source);
   musicInfo.state = MusicPaused;
}


void SoundSystem::resumeMusic(MusicInfo &musicInfo)
{
   alureResumeSource(musicInfo.source);
   musicInfo.state = MusicPlaying;
}


void SoundSystem::playNextTrack()
{
   stopMusic(musicInfos[MusicTypeGame]);

   // Increment the playing index
   mCurrentlyPlayingIndex = (mCurrentlyPlayingIndex + 1) % mMusicList.size();

   playGameMusic();
}


void SoundSystem::playPrevTrack()
{
   stopMusic(musicInfos[MusicTypeGame]);

   // Decrement the playing index
   mCurrentlyPlayingIndex = (mCurrentlyPlayingIndex + 1) % mMusicList.size();
   if(mCurrentlyPlayingIndex < 0)
      mCurrentlyPlayingIndex += mMusicList.size();

   playGameMusic();
}


};

#elif defined (NO_AUDIO)

using namespace TNL;

namespace Zap
{


void SoundSystem::updateGain(SFXHandle& effect, F32 volLevel, F32 voiceVolLevel)
{
   // Do nothing
}

void SoundSystem::updateMovementParams(SFXHandle& effect)
{
   // Do nothing
}

void SoundSystem::playOnSource(SFXHandle& effect, F32 sfxVol, F32 voiceVol)
{
   // Do nothing
}

void SoundSystem::setMovementParams(SFXHandle& effect, Point position, Point velocity)
{
   // Do nothing
}

SFXHandle SoundSystem::playSoundEffect(U32 profileIndex, F32 gain)
{
   return new SoundEffect(0,NULL,0,Point(0,0), Point(0,0));
}

SFXHandle SoundSystem::playSoundEffect(U32 profileIndex, Point position)
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

void SoundSystem::playSoundEffect(const SFXHandle& effect)
{
   // Do nothing
}

void SoundSystem::stopSoundEffect(SFXHandle& effect)
{
   // Do nothing
}

void SoundSystem::queueVoiceChatBuffer(const SFXHandle &effect, ByteBufferPtr p)
{
   // Do nothing
}

void SoundSystem::init(sfxSets sfxSet, const string &sfxDir, const string &musicDir, float musicVol)
{
   logprintf(LogConsumer::LogError, "No OpenAL support on this platform.");
}

void SoundSystem::processAudio(F32 sfxVol, F32 musicVol, F32 voiceVol)
{
   // Do nothing
}

void SoundSystem::processAudio(F32 sfxVol)
{
   // Do nothing
}

void SoundSystem::setListenerParams(Point pos, Point velocity)
{
   // Do nothing
}

void SoundSystem::shutdown()
{
   // Do nothing
}

void SoundSystem::playNextTrack()
{
   // Do nothing
}

void SoundSystem::playPrevTrack()
{
   // Do nothing
}


};

#endif      // #ifdef defined NO_AUDIO


#if defined(NO_AUDIO)
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
