/*
 * SFXProfile.h
 *
 *  Created on: May 9, 2011
 *      Author: dbuck
 */

#ifndef SFXPROFILE_H_
#define SFXPROFILE_H_

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

#endif /* SFXPROFILE_H_ */
