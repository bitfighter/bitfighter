//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _TEAM_CONSTANTS_H_
#define _TEAM_CONSTANTS_H_

#include "tnl.h"

using namespace TNL;

namespace Zap
{

// Special team constants
static const S32 TEAM_NEUTRAL = -1;
static const S32 TEAM_HOSTILE = -2;
static const S32 NO_TEAM = -3;      // Not exposed to lua, not used in level files, only used internally

static const S32 NONE = -1;         // Used for a variety of purposes

};

#endif


