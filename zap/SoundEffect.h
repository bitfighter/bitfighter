//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef SOUNDEFFECT_H_
#define SOUNDEFFECT_H_

#include "Point.h"
#include "SFXProfile.h"
#include "tnlTypes.h"
#include "tnlByteBuffer.h"

// forward declarations

namespace TNL {
   template <class T> class RefPtr;
   class Object;
};

using namespace TNL;

namespace Zap {

class SoundEffect : public TNL::Object
{
public:
   static Point mListenerPosition;
   static Point mListenerVelocity;
   static F32 mMaxDistance;
   static S32 mCaptureGain;

   U32 mSFXIndex;
   Point mPosition;
   Point mVelocity;
   ByteBufferPtr mInitialBuffer;
   SFXProfile *mProfile;
   F32 mGain;
   S32 mSourceIndex;
   F32 mPriority;

   SoundEffect(U32 profileIndex, ByteBufferPtr ib, F32 gain, Point position, Point velocity);
   virtual ~SoundEffect();

   bool isPlaying();
};

typedef RefPtr<SoundEffect> SFXHandle;

}

#endif /* SOUNDEFFECT_H_ */
