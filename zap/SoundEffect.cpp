/*
 * SoundEffect.cpp
 *
 *  Created on: May 8, 2011
 *      Author: dbuck
 */

#include "SoundEffect.h"

#if !defined (ZAP_DEDICATED) && !defined (TNL_OS_XBOX)

namespace Zap {

extern SFXProfile *gSFXProfiles;

SoundEffect::SoundEffect(U32 profileIndex, ByteBufferPtr ib, F32 gain, Point position, Point velocity)
{
   mSFXIndex = profileIndex;
   mProfile = gSFXProfiles + profileIndex;
   mGain = gain;
   mPosition = position;
   mVelocity = velocity;
   mSourceIndex = -1;
   mPriority = 0;
   mInitialBuffer = ib;
}

// Destructor
SoundEffect::~SoundEffect()
{
   // Do nothing
}



}

#elif defined (ZAP_DEDICATED)

using namespace TNL;

namespace Zap
{
SoundEffect::SoundEffect(U32 profileIndex, ByteBufferPtr ib, F32 gain, Point position, Point velocity)
{
}

SoundEffect::~SoundEffect()
{
}

};

#endif
