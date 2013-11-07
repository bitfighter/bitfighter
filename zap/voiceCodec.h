//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _VOICECODEC_H_
#define _VOICECODEC_H_

#include "tnlByteBuffer.h"

#if defined(TNL_OS_MOBILE) || defined(BF_NO_AUDIO)
#  define BF_NO_VOICECHAT
#endif

#ifndef BF_NO_VOICECHAT
#  include "speex/speex.h"
#else
   typedef struct SpeexBits {} SpeexBits;
#endif

using namespace TNL;

namespace Zap
{

/// The VoiceEncoder class is an abstract base class for the various
/// voice compression algorithms supported by Zap - initially just the
/// HawkVoice variable bitrate LPC10 codec.  It takes an arbitrary buffer
/// of 16 bit samples at 8KHz and returns a compressed buffer.  The
/// original buffer is modified to contain any samples that were not used
/// due to unfilled frame sizing.
class VoiceEncoder : public Object
{
   virtual U32 getSamplesPerFrame() = 0;
   virtual U32 getMaxCompressedFrameSize() = 0;
   virtual U32 compressFrame(S16 *samplePtr, U8 *outputPtr) = 0;
public:
   VoiceEncoder();
   virtual ~VoiceEncoder();

   ByteBufferPtr compressBuffer(const ByteBufferPtr &sampleBuffer);
};

/// The VoiceDecoder class is an abstract base class for the various
/// voice decompression algorithms supported by Zap - initially just the
/// HawkVoice variable bitrate LPC10 codec.  It takes a buffer compressed
/// with the appropriate VoiceEncoder analogue and returns the decompressed
/// 16 bit sample buffer.
class VoiceDecoder : public Object
{
   virtual U32 getSamplesPerFrame() = 0;
   virtual U32 getAvgCompressedFrameSize() = 0;

   virtual U32 decompressFrame(S16 *framePtr, U8 *inputPtr, U32 inSize) = 0;
public:
   VoiceDecoder();
   virtual ~VoiceDecoder();

   ByteBufferPtr decompressBuffer(const ByteBufferPtr &compressedBuffer);
};

/// The SpeexVoiceEncoder class implements the Speex codec
/// compressor.
class SpeexVoiceEncoder : public VoiceEncoder
{
   SpeexBits speexBits;
   static const U32 maxFrameByteSize = 33;  // don't go to high or ByteBuffer will break

   void *encoderState;
   U32 getSamplesPerFrame();
   U32 getMaxCompressedFrameSize();
   U32 compressFrame(S16 *samplePtr, U8 *outputPtr);
public:
   SpeexVoiceEncoder();
   virtual ~SpeexVoiceEncoder();
};

/// The SpeexVoiceDecoder class implements the Speex codec
/// decompressor.
class SpeexVoiceDecoder : public VoiceDecoder
{
   SpeexBits speexBits;
   static const U32 maxFrameByteSize = 33;

   void *decoderState;
   U32 getSamplesPerFrame();
   U32 getAvgCompressedFrameSize();

   U32 decompressFrame(S16 *framePtr, U8 *inputPtr, U32 inSize);
public:
   SpeexVoiceDecoder();
   virtual ~SpeexVoiceDecoder();
};

};

#endif


