//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _INTERVALS_H
#define _INTERVALS_H

#include "tnlTypes.h"

using namespace TNL;

static const S32 ONE_SECOND    = 1000;    // milliseconds
static const S32 ONE_MINUTE    = 60 * ONE_SECOND;
static const S32 ONE_HOUR      = 60 * ONE_MINUTE;
                               
static const S32 TWO_SECONDS   =  2 * ONE_SECOND; 
static const S32 THREE_SECONDS =  3 * ONE_SECOND;
static const S32 FOUR_SECONDS  =  4 * ONE_SECOND;
static const S32 FIVE_SECONDS  =  5 * ONE_SECOND;
static const S32 SIX_SECONDS   =  6 * ONE_SECOND; 
static const S32 TEN_SECONDS   = 10 * ONE_SECOND;

static const S32 FIFTEEN_SECONDS = 15 * ONE_SECOND;
static const S32 TWENTY_SECONDS  = 20 * ONE_SECOND;
static const S32 THIRTY_SECONDS  = 30 * ONE_SECOND;

static const S32 TEN_MINUTES   = 10 * ONE_MINUTE;
                               
static const S32 TWO_HOURS     =  2 * ONE_HOUR;

// Conversion factors
static const F32 MS_TO_SECONDS = .001f;

#endif
