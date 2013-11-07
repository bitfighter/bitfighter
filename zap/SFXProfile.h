//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SFX_PROFILE_H_
#define _SFX_PROFILE_H_

#include "tnlTypes.h"

namespace Zap {

struct SFXProfile
{
   const char *fileName;
   bool        isRelative;
   TNL::F32    gainScale;
   bool        isLooping;
   TNL::F32    fullGainDistance;
   TNL::F32    zeroGainDistance;
};

}

#endif
