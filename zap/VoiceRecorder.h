//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _VOICE_RECORDER_H_
#define _VOICE_RECORDER_H_

#include "Timer.h"
#include "SoundEffect.h"
#include "voiceCodec.h"
#include "tnlByteBuffer.h"

namespace Zap
{

class ClientGame;

class VoiceRecorder
{
private:
   ClientGame *mGame;

public:
   enum {
      FirstVoiceAudioSampleTime = 250,
      VoiceAudioSampleTime      = 100,
      MaxDetectionThreshold     = 2048,
   };

private:
   Timer mVoiceAudioTimer;
   RefPtr<SoundEffect> mVoiceSfx;
   RefPtr<VoiceEncoder> mVoiceEncoder;
   bool mRecordingAudio;
   U8 mWantToStopRecordingAudio;
   S32 mMaxAudioSample;
   S32 mMaxForGain;
   ByteBufferPtr mUnusedAudio;

public:
   explicit VoiceRecorder(ClientGame *game);
   ~VoiceRecorder();

   void idle(U32 timeDelta);
   void render() const;
   bool isRecording() const;

   void start();
   void stop();
   void stopNow();

private:
   void process();
};

}

#endif
