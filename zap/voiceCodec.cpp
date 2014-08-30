//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "voiceCodec.h"

namespace Zap
{

// Destructor
VoiceEncoder::VoiceEncoder()
{
   // Do nothing
}

// Destructor
VoiceEncoder::~VoiceEncoder()
{
   // Do nothing
}

ByteBufferPtr VoiceEncoder::compressBuffer(const ByteBufferPtr &sampleBuffer)
{
   // Number of 16 bit samples in the input buffer
   U32 inputSampleCount = sampleBuffer->getBufferSize() / 2;  // Integer division OK

   // Samples per frame of the encoding codec
   U32 samplesPerFrame = getSamplesPerFrame();

   // How many frames will be used in this round (rounded down)
   U32 framesToCompress = U32(inputSampleCount / samplesPerFrame);

   // If we use any frames
   if(framesToCompress != 0)
   {
      // How many samples will we compress
      U32 samplesToCompress = framesToCompress * samplesPerFrame;
      U32 maxSize = getMaxCompressedFrameSize() * framesToCompress;

      // Create our compressed return buffer
      ByteBufferPtr compressedBuffer = new ByteBuffer(maxSize);

      U8 *compressedBufferPtr = compressedBuffer->getBuffer();
      S16 *sampleBufferPtr = (S16 *) sampleBuffer->getBuffer();

      // Fill our compressed buffer, frame by frame
      U32 len = 0;
      for(U32 i = 0; i < samplesToCompress; i += samplesPerFrame)
         len += compressFrame(sampleBufferPtr + i, compressedBufferPtr + len);

      compressedBuffer->resize(len);

      // This handles any trailing data not compressed in the input buffer and saves it back
      // to the input buffer to be handled next round
      U32 newSize = (inputSampleCount - samplesToCompress) * sizeof(U16);
      memcpy(sampleBufferPtr, sampleBufferPtr + samplesToCompress, newSize);
      sampleBuffer->resize(newSize);

      // Finally return our compressed data for sending on the network
      return compressedBuffer;
   }

   return NULL;
}

// Destructor
VoiceDecoder::VoiceDecoder()
{
   // Do nothing
}

// Destructor
VoiceDecoder::~VoiceDecoder()
{
   // Do nothing
}

ByteBufferPtr VoiceDecoder::decompressBuffer(const ByteBufferPtr &compressedBuffer)
{
   U32 samplesPerFrame = getSamplesPerFrame();
   U32 avgCompressedFrameSize = getAvgCompressedFrameSize();
   U32 compressedSize = compressedBuffer->getBufferSize();

   // Guess the total number of frames:
   U32 guessedFrameCount = (compressedSize / avgCompressedFrameSize) + 1;

   // Create a buffer for for the decompressed data, giving it a guessed starting size
   ByteBufferPtr decodedBuffer = new ByteBuffer(samplesPerFrame * sizeof(S16) * guessedFrameCount);

   // Pointers to raw buffer data
   U8 *compressedBufferPtr = compressedBuffer->getBuffer();
   S16 *decodedBufferPtr = (S16 *) decodedBuffer->getBuffer();

   U32 frameCount = 0;
   U32 bufferOffset = 0;

   // Loop until we've reached the end of our compressed input buffer
   for(U32 i = 0; i < compressedSize; i += bufferOffset)
   {
      // If we've hit our guessed frame count, that means we've guessed too low; resize the buffer
      // a little bit more to handle more data
      // FIXME why is this needed?  Why not just start with a larger buffer?
      if(frameCount == guessedFrameCount)
      {
         guessedFrameCount = frameCount + ( (compressedSize - i) / avgCompressedFrameSize ) + 1;
         decodedBuffer->resize(samplesPerFrame * sizeof(S16) * guessedFrameCount);

         decodedBufferPtr = (S16 *) decodedBuffer->getBuffer();
      }

      // Decode a frame and advance our buffer offset
      bufferOffset = decompressFrame(decodedBufferPtr + frameCount * samplesPerFrame, compressedBufferPtr + i, compressedSize - i);

      frameCount++;
   }

   // Finally resize the buffer to correspond to how many frames we've decompressed
   decodedBuffer->resize(frameCount * samplesPerFrame * sizeof(S16));

   return decodedBuffer;
}



#ifdef BF_NO_VOICECHAT
SpeexVoiceEncoder::SpeexVoiceEncoder() { /* Do nothing */ }
SpeexVoiceEncoder::~SpeexVoiceEncoder() { /* Do nothing */  }
U32 SpeexVoiceEncoder::getSamplesPerFrame() { return 0; }
U32 SpeexVoiceEncoder::getMaxCompressedFrameSize() { return 0; }
U32 SpeexVoiceEncoder::compressFrame(S16 *samplePtr, U8 *outputPtr) { return 0; }

SpeexVoiceDecoder::SpeexVoiceDecoder() { /* Do nothing */  }
SpeexVoiceDecoder::~SpeexVoiceDecoder() { /* Do nothing */  }
U32 SpeexVoiceDecoder::getSamplesPerFrame() { return 0; }
U32 SpeexVoiceDecoder::getAvgCompressedFrameSize() { return 0; }
U32 SpeexVoiceDecoder::decompressFrame(S16 *framePtr, U8 *inputPtr, U32 inSize) { return 0; }

// Begin Speex
#else // BF_NO_VOICECHAT

SpeexVoiceEncoder::SpeexVoiceEncoder()
{
   speex_bits_init(&speexBits);
   encoderState = speex_encoder_init(speex_lib_get_mode(SPEEX_MODEID_NB));  // narrow-band coding
}

SpeexVoiceEncoder::~SpeexVoiceEncoder()
{
   speex_bits_destroy(&speexBits);
   speex_encoder_destroy(encoderState);
}

U32 SpeexVoiceEncoder::getSamplesPerFrame()
{
   U32 samples;
   speex_encoder_ctl(encoderState, SPEEX_GET_FRAME_SIZE, &samples);

   return samples;  // 160 in narrow-band coding
}

U32 SpeexVoiceEncoder::getMaxCompressedFrameSize()
{
   return maxFrameByteSize;
}

U32 SpeexVoiceEncoder::compressFrame(S16 *samplePtr, U8 *outputPtr)
{
   speex_bits_reset(&speexBits);
   speex_encode_int(encoderState, samplePtr, &speexBits);
   speex_bits_write(&speexBits, (char*)outputPtr, maxFrameByteSize);

   return maxFrameByteSize;
}

SpeexVoiceDecoder::SpeexVoiceDecoder()
{
   speex_bits_init(&speexBits);
   decoderState = speex_decoder_init(speex_lib_get_mode(SPEEX_MODEID_NB));  // narrow-band coding
}

SpeexVoiceDecoder::~SpeexVoiceDecoder()
{
   speex_bits_destroy(&speexBits);
   speex_decoder_destroy(decoderState);
}

U32 SpeexVoiceDecoder::getSamplesPerFrame()
{
   U32 samples;
   speex_decoder_ctl(decoderState, SPEEX_GET_FRAME_SIZE, &samples);

   return samples;  // 160 in narrow-band coding
}

U32 SpeexVoiceDecoder::getAvgCompressedFrameSize()
{
   return maxFrameByteSize;
}

U32 SpeexVoiceDecoder::decompressFrame(S16 *framePtr, U8 *inputPtr, U32 inSize)
{
   speex_bits_read_from(&speexBits, (char*)inputPtr, inSize);
   speex_decode_int(decoderState, &speexBits, framePtr);

   return maxFrameByteSize;
}
#endif // BF_NO_VOICECHAT


};


