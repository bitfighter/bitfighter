//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _STACK_TRACER_H_
#define _STACK_TRACER_H_

#ifndef BF_NO_STACKTRACE

#include "tnlTypes.h"
#include "tnlPlatform.h"


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
