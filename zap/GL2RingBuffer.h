//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GL2RINGBUFFER_H_
#define _GL2RINGBUFFER_H_

#include "tnlTypes.h"
#include <cstddef> // For size_t

using namespace TNL;

namespace Zap
{

class GL2RingBuffer
{
private:
   U32 mId;
   std::size_t mCurrentOffset;

public:
   GL2RingBuffer();
   ~GL2RingBuffer();

   void bind();
   std::size_t insertData(const void* data, U32 size);
};

}

#endif // _GL2RINGBUFFER_H_ 
