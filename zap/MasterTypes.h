//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _MASTER_TYPES_H_
#define _MASTER_TYPES_H_

#include "tnlTypes.h"

#include <utility>      // For pair


namespace Zap
{

typedef std::pair<IPAddress, S32> ServerAddr;

}


#endif