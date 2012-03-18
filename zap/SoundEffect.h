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
