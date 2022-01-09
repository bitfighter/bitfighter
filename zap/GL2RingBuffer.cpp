//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Ring buffer for avoid collisions when OpenGL tries to render our data
// while we are writing new data.
// Data should be 4-byte aligned.
// For more information: https://www.khronos.org/opengl/wiki/Buffer_Object_Streaming

#ifndef BF_USE_LEGACY_GL

#include "GL2RingBuffer.h"
#include "glad/glad.h"

// A larger ring buffer will allow us to reallocate buffer memory less often
static const U32 RING_BUFFER_SIZE = 5 * 1000000U;

namespace Zap
{

GL2RingBuffer::GL2RingBuffer()
   : mId(0)
   , mCurrentOffset(0)
{
   // Generate buffer and allocate initial memory
   glGenBuffers(1, &mId);
   glBindBuffer(GL_ARRAY_BUFFER, mId);
   glBufferData(GL_ARRAY_BUFFER, RING_BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW);
}

GL2RingBuffer::~GL2RingBuffer()
{
   glDeleteBuffers(1, &mId);
}

void GL2RingBuffer::bind()
{
   glBindBuffer(GL_ARRAY_BUFFER, mId);
}

// Returns the offset of inserted data within the buffer
std::size_t GL2RingBuffer::insertData(const void *data, U32 size)
{
   if(mCurrentOffset + size >= RING_BUFFER_SIZE)
   {
      // Orphan current data and allocate new memory. Any old data still being used by OpenGL will
      // continue to exist until it doesn't need it anymore.
      glBufferData(GL_ARRAY_BUFFER, RING_BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW);
      mCurrentOffset = 0;
   }

   glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mCurrentOffset), size, data);

   std::size_t oldPosition = mCurrentOffset;
   mCurrentOffset += size + (4 - size%4); // Make sure we are 4-byte aligned
   return oldPosition;
}

}

#endif // BF_USE_LEGACY_GL