//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "voiceCodec.h"
#include "gtest/gtest.h"
#include <vector>
#include <cmath>

using namespace TNL;
using namespace Zap;

namespace Zap
{

TEST(VoiceCodecTest, SpeexEncoderDecoderCycle)
{
   SpeexVoiceEncoder encoder;
   SpeexVoiceDecoder decoder;

   U32 samplesPerFrame = encoder.getSamplesPerFrame();
   if(samplesPerFrame == 0)
   {
      // BF_NO_VOICECHAT is likely defined
      return;
   }

   // Create a buffer with some dummy audio data (sine wave)
   U32 numFrames = 5;
   U32 totalSamples = samplesPerFrame * numFrames;
   ByteBufferPtr audioData = new ByteBuffer(totalSamples * sizeof(S16));
   S16 *samples = (S16 *) audioData->getBuffer();

   for(U32 i = 0; i < totalSamples; ++i)
   {
      samples[i] = (S16)(32767.0 * sin(i * 0.1));
   }

   // Compress
   ByteBufferPtr compressed = encoder.compressBuffer(audioData);
   ASSERT_NE(nullptr, compressed);
   ASSERT_GT(compressed->getBufferSize(), 0u);

   // Decompress
   ByteBufferPtr decompressed = decoder.decompressBuffer(compressed);
   ASSERT_NE(nullptr, decompressed);

   // Check size
   EXPECT_EQ(totalSamples * sizeof(S16), decompressed->getBufferSize());

   // Since Speex is lossy, we don't expect bit-for-bit equality,
   // but it should be non-empty and have the right length.
}

TEST(VoiceCodecTest, EmptyBuffer)
{
   SpeexVoiceEncoder encoder;

   U32 samplesPerFrame = encoder.getSamplesPerFrame();
   if(samplesPerFrame == 0) return;

   ByteBufferPtr emptyBuffer = new ByteBuffer(0);
   ByteBufferPtr compressed = encoder.compressBuffer(emptyBuffer);

   EXPECT_EQ(nullptr, compressed);
}

TEST(VoiceCodecTest, SmallBuffer)
{
   SpeexVoiceEncoder encoder;

   U32 samplesPerFrame = encoder.getSamplesPerFrame();
   if(samplesPerFrame == 0) return;

   // Less than one frame
   ByteBufferPtr smallBuffer = new ByteBuffer((samplesPerFrame - 1) * sizeof(S16));
   ByteBufferPtr compressed = encoder.compressBuffer(smallBuffer);

   EXPECT_EQ(nullptr, compressed);
   // The original buffer should still have the data
   EXPECT_EQ((samplesPerFrame - 1) * sizeof(S16), smallBuffer->getBufferSize());
}

} // namespace Zap
