//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _STACK_TRACER_H_
#define _STACK_TRACER_H_

#include "tnlTypes.h"
#include "tnlPlatform.h"

#ifdef TNL_DEBUG
#define BF_NO_STACKTRACE
#endif

#ifndef BF_NO_STACKTRACE


using namespace TNL;
using namespace std;

namespace Zap
{


// Design for this class borrowed from http://oroboro.com/stack-trace-on-crash/
class StackTracer
{
public:
    StackTracer();
};


}

#endif // BF_NO_STACKTRACE


#endif // _STACK_TRACER_H_
