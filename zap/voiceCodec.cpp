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

#include "voiceCodec.h"
#include "lpc10.h"
#include "gsm.h"

namespace Zap
{

ByteBufferPtr VoiceEncoder::compressBuffer(ByteBufferPtr sampleBuffer)
{
   U32 sampleCount = sampleBuffer->getBufferSize() >> 1;
   U32 spf = getSamplesPerFrame();

   U32 framesUsed = U32(sampleCount / spf);
   if(framesUsed)
   {
      U32 samplesUsed = framesUsed * spf;
      U32 maxSize = getMaxCompressedFrameSize() * framesUsed;
      ByteBufferPtr ret = new ByteBuffer(maxSize);

      U8 *compressedPtr = ret->getBuffer();
      S16 *samplePtr = (S16 *) sampleBuffer->getBuffer();

      U32 len = 0;

      for(U32 i = 0; i < samplesUsed; i += spf)
         len += compressFrame(samplePtr + i, compressedPtr + len);

      ret->resize(len);

      U32 newSize = (sampleCount - samplesUsed) * sizeof(U16);
      memcpy(samplePtr, samplePtr + samplesUsed, newSize);

      sampleBuffer->resize(newSize);
      return ret;
   }
   return NULL;
}

ByteBufferPtr VoiceDecoder::decompressBuffer(ByteBufferRef compressedBuffer)
{
   U32 spf = getSamplesPerFrame();
   U32 avgCompressedSize = getAvgCompressedFrameSize();
   U32 compressedSize = compressedBuffer.getBufferSize();

   // guess the total number of frames:
   U32 guessFrameCount = (compressedSize / avgCompressedSize) + 1;

   ByteBufferPtr ret = new ByteBuffer(spf * sizeof(S16) * guessFrameCount);

   U32 p = 0;
   U8 *inputPtr = (U8 *) compressedBuffer.getBuffer();
   U32 frameCount = 0;
   S16 *samplePtr = (S16 *) ret->getBuffer();

   for(U32 i = 0; i < compressedSize; i += p)
   {
      if(frameCount == guessFrameCount)
      {
         guessFrameCount = frameCount + ( (compressedSize - i) / avgCompressedSize ) + 1;
         ret->resize(spf * sizeof(S16) * guessFrameCount);

         samplePtr = (S16 *) ret->getBuffer();
      }
      p = decompressFrame(samplePtr + frameCount * spf, inputPtr + i, compressedSize - i);
      frameCount++;
   }
   ret->resize(frameCount * spf * sizeof(S16));
   return ret;
}


LPC10VoiceEncoder::LPC10VoiceEncoder()
{
   encoderState = create_lpc10_encoder_state();
   init_lpc10_encoder_state((lpc10_encoder_state *) encoderState);
}

LPC10VoiceEncoder::~LPC10VoiceEncoder()
{
   destroy_lpc10_encoder_state((lpc10_encoder_state *) encoderState);
}

U32 LPC10VoiceEncoder::getSamplesPerFrame()
{
   return LPC10_SAMPLES_PER_FRAME;
}

U32 LPC10VoiceEncoder::getMaxCompressedFrameSize()
{
   return LPC10_ENCODED_FRAME_SIZE;
}

U32 LPC10VoiceEncoder::compressFrame(S16 *samplePtr, U8 *outputPtr)
{
   return vbr_lpc10_encode(samplePtr, outputPtr, (lpc10_encoder_state *) encoderState);
}

LPC10VoiceDecoder::LPC10VoiceDecoder()
{
   decoderState = create_lpc10_decoder_state();
   init_lpc10_decoder_state((lpc10_decoder_state *) decoderState);
}

LPC10VoiceDecoder::~LPC10VoiceDecoder()
{
   destroy_lpc10_decoder_state((lpc10_decoder_state *) decoderState);
}

U32 LPC10VoiceDecoder::getSamplesPerFrame()
{
   return LPC10_SAMPLES_PER_FRAME;
}

U32 LPC10VoiceDecoder::getAvgCompressedFrameSize()
{
   return (LPC10_ENCODED_FRAME_SIZE + 1) >> 1;
}

U32 LPC10VoiceDecoder::decompressFrame(S16 *framePtr, U8 *inputPtr, U32 inSize)
{
   int p;
   vbr_lpc10_decode(inputPtr, inSize, framePtr, (lpc10_decoder_state *) decoderState, &p);
   return (U32) p;
}

GSMVoiceEncoder::GSMVoiceEncoder()
{
   encoderState = gsm_create();
}

GSMVoiceEncoder::~GSMVoiceEncoder()
{
   gsm_destroy((struct gsm_state *) encoderState);
}

U32 GSMVoiceEncoder::getSamplesPerFrame()
{
   return GSM_SAMPLES_PER_FRAME;
}

U32 GSMVoiceEncoder::getMaxCompressedFrameSize()
{
   return GSM_ENCODED_FRAME_SIZE;
}

U32 GSMVoiceEncoder::compressFrame(S16 *samplePtr, U8 *outputPtr)
{
   return gsm_encode((struct gsm_state *) encoderState, samplePtr, outputPtr);
}

GSMVoiceDecoder::GSMVoiceDecoder()
{
   decoderState = gsm_create();
}

GSMVoiceDecoder::~GSMVoiceDecoder()
{
   gsm_destroy((struct gsm_state *) decoderState);
}

U32 GSMVoiceDecoder::getSamplesPerFrame()
{
   return GSM_SAMPLES_PER_FRAME;
}

U32 GSMVoiceDecoder::getAvgCompressedFrameSize()
{
   return GSM_ENCODED_FRAME_SIZE;
}

U32 GSMVoiceDecoder::decompressFrame(S16 *framePtr, U8 *inputPtr, U32 inSize)
{
   gsm_decode((struct gsm_state *) decoderState, inputPtr, framePtr);
   return GSM_ENCODED_FRAME_SIZE;
}

// Begin Speex

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


};


