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


bool SoundEffect::isPlaying()
{
   return mSourceIndex != -1;
}


}

#endif // BF_NO_AUDIO
