/*
 * SoundEffect.h
 *
 *  Created on: May 8, 2011
 *      Author: dbuck
 */

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

namespace Zap {

class SoundEffect : public TNL::Object
{
public:
   static Point mListenerPosition;
   static Point mListenerVelocity;
   static TNL::F32 mMaxDistance;
   static TNL::S32 mCaptureGain;

   TNL::U32 mSFXIndex;
   Point mPosition;
   Point mVelocity;
   TNL::ByteBufferPtr mInitialBuffer;
   SFXProfile *mProfile;
   TNL::F32 mGain;
   TNL::S32 mSourceIndex;
   TNL::F32 mPriority;

   SoundEffect(TNL::U32 profileIndex, TNL::ByteBufferPtr ib, TNL::F32 gain, Point position, Point velocity);
   virtual ~SoundEffect();

   bool isPlaying() { return mSourceIndex != -1; }
};

typedef TNL::RefPtr<SoundEffect> SFXHandle;

}

#endif /* SOUNDEFFECT_H_ */
