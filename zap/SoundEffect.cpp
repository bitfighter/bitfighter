//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "SoundEffect.h"

#ifdef ZAP_DEDICATED
#  define BF_NO_AUDIO
#endif


#ifdef BF_NO_AUDIO

using namespace TNL;

namespace Zap
{

SoundEffect::SoundEffect(U32 profileIndex, ByteBufferPtr ib, F32 gain, Point position, Point velocity)
{
   // Do nothing
}

SoundEffect::~SoundEffect()
{
   // Do nothing
}

bool SoundEffect::isPlaying()
{
   return false;
}

};

#else // BF_NO_AUDIO

namespace Zap {

extern SFXProfile *gSFXProfiles;

SoundEffect::SoundEffect(U32 profileIndex, ByteBufferPtr ib, F32 gain, Point position, Point velocity) :
   mPosition(position),
   mVelocity(velocity)
{
   mSFXIndex = profileIndex;
   mProfile = gSFXProfiles + profileIndex;
   mGain = gain;
   mSourceIndex = -1;
   mPriority = 0;
   mInitialBuffer = ib;
}

// Destructor
SoundEffect::~SoundEffect()
{
   // Do nothing
}


bool SoundEffect::isPlaying()
{
   return mSourceIndex != -1;
}


}

#endif // BF_NO_AUDIO
