//-----------------------------------------------------------------------------------
//
//   Torque Network Library
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _TNL_BYTEBUFFER_H_
#define _TNL_BYTEBUFFER_H_

#ifndef _TNL_H_
#include "tnl.h"
#endif

#ifndef _TNL_NETBASE_H_
#include "tnlNetBase.h"
#endif

namespace TNL {

class ByteBuffer : public Object
{
   friend struct MethodArgList;
protected:
   /// Pointer to our data buffer.
   U8 *mDataPtr;

   /// Length of buffer.
   U32  mBufSize;

   /// Do we own the memory we are using? (If so, we should free it.)
   bool mOwnsMemory;
public:
   // Act 1, Scene 1
   // Enter: enums, constructors, with divers alarums

   // Prologue:
   enum {
      DefaultBufferSize = 1500, ///< Starting size of the buffer, varlot.
   };

   // Chorus:

   /// Create a ByteBuffer from a chunk of memory.
   ByteBuffer(U8 *dataPtr, U32 bufferSize)
   {
      mBufSize = bufferSize;
      mDataPtr = dataPtr;
      mOwnsMemory = false;
   }

   /// Create a ByteBuffer of the specified size.
   ByteBuffer(U32 bufferSize = DefaultBufferSize)
   {
      mBufSize = bufferSize;
      mDataPtr = (U8 *) malloc(bufferSize);
      mOwnsMemory = true;
   }

   /// Copy constructor!
   ByteBuffer(const ByteBuffer &theBuffer)
   {
      mBufSize = theBuffer.mBufSize;
      mDataPtr = theBuffer.mDataPtr;
      mOwnsMemory = false;
   }

   ~ByteBuffer()
   {
      if(mOwnsMemory)
         free(mDataPtr);
   }

   /// Set the ByteBuffer to point to a new chunk of memory.
   void setBuffer(U8 *dataPtr, U32 bufferSize)
   {
      if(mOwnsMemory)
      {
         mOwnsMemory = false;
         free(mDataPtr);
      }
      mDataPtr = dataPtr;
      mBufSize = bufferSize;
   }

   /// Set the ByteBuffer to point to a new chunk of memory, indicating whether
   /// it should own the chunk or not.
   void setBuffer(U8 *dataPtr, U32 bufferSize, bool newOwnsMemory)
   {
      setBuffer(dataPtr, bufferSize);
      mOwnsMemory = newOwnsMemory;
   }

   /// Attempts to resize the buffer.
   ///
   /// @returns True if it owns its own memory, false otherwise.
   bool resize(U32 newBufferSize)
   {
      if(mBufSize >= newBufferSize)
         mBufSize = newBufferSize;
      else if(mOwnsMemory)
      {
         mBufSize = newBufferSize;
         mDataPtr = (U8 *) realloc(mDataPtr, newBufferSize);
         return true;
      }
      return false;
   }

   /// Appends the specified buffer to the end of the byte buffer.
   /// returns false if it does not own its own memory.
   bool appendBuffer(const U8 *dataBuffer, U32 bufferSize)
   {
      U32 start = mBufSize;
      if(!resize(mBufSize + bufferSize))
         return false;
      memcpy(mDataPtr + start, dataBuffer, bufferSize);
      return true;
   }

   /// Appends the specified ByteBuffer to the end of this byte buffer.
   /// returns fals if it does not own its own memory.
   bool appendBuffer(const ByteBuffer &theBuffer)
   {
      return appendBuffer(theBuffer.getBuffer(), theBuffer.getBufferSize());
   }

   /// Copies the current buffer into a newly allocated buffer that the ByteBuffer owns.
   void takeOwnership()
   {
      if(!mOwnsMemory)
      {
         U8 *memPtr = (U8 *) malloc(mBufSize);
         memcpy(memPtr, mDataPtr, mBufSize);
         mDataPtr = memPtr;
         mOwnsMemory = true;
      }
   }

   /// Does this ByteBuffer own its own memory?
   bool ownsMemory()
   {
      return mOwnsMemory;
   }

   U32 getBufferSize() const
   {
      return mBufSize;
   }

   U8 *getBuffer()
   {
      return mDataPtr;
   }

   const U8 *getBuffer() const
   {
      return mDataPtr;
   }

   /// Clear the buffer.
   void clear()
   {
      memset(mDataPtr, 0, mBufSize);
   }

   /// Encode the buffer to base 64, returning the encoded buffer.
   RefPtr<ByteBuffer> encodeBase64() const;
   /// Decode the buffer from base 64, returning the decoded buffer.
   RefPtr<ByteBuffer> decodeBase64() const;

   /// Computes an MD5 hash and returns it in a ByteBuffer
   RefPtr<ByteBuffer> computeMD5Hash(U32 len = 0) const;

   /// Converts to ascii-hex, returning the encoded buffer.
   RefPtr<ByteBuffer> encodeBase16() const;
   /// Decodes the buffer from base 16, returning the decoded buffer.
   RefPtr<ByteBuffer> decodeBase16() const;

   /// Returns a 32 bit CRC for the buffer.
   U32 calculateCRC(U32 start = 0, U32 end = 0xFFFFFFFF, U32 crcVal = 0xFFFFFFFF) const;
};

typedef RefPtr<ByteBuffer> ByteBufferPtr;
typedef const ByteBuffer &ByteBufferRef;

};

#endif
