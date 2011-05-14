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

#include "tnlByteBuffer.h"
#include "../libtomcrypt/mycrypt.h"

namespace TNL {

void ByteBuffer::newByteBuffer(U32 bufferSize)
{
   if(mOwnsMemory)
   {
      mOwnsMemory = false;
      free(mDataPtr);
      mOwnsMemory = true;
   }
   mBufSize = bufferSize;
   mDataPtr = (U8 *) malloc(bufferSize);
}

bool ByteBuffer::resize(U32 newBufferSize)
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


bool ByteBuffer::appendBuffer(const U8 *dataBuffer, U32 bufferSize)
{
   U32 start = mBufSize;
   if(!resize(mBufSize + bufferSize))
      return false;
   memcpy(mDataPtr + start, dataBuffer, bufferSize);
   return true;
}


void ByteBuffer::takeOwnership()
{
   if(!mOwnsMemory)
   {
      U8 *memPtr = (U8 *) malloc(mBufSize);
      memcpy(memPtr, mDataPtr, mBufSize);
      mDataPtr = memPtr;
      mOwnsMemory = true;
   }
}


RefPtr<ByteBuffer> ByteBuffer::encodeBase64() const
{
   unsigned long outLen = ((getBufferSize() / 3) + 1) * 4 + 4 + 1;
   ByteBuffer *ret = new ByteBuffer(outLen);
   base64_encode(getBuffer(), getBufferSize(), ret->getBuffer(), &outLen);
   ret->resize(outLen+1);
   ret->getBuffer()[outLen] = 0;
   return ret;
}

RefPtr<ByteBuffer> ByteBuffer::decodeBase64() const
{
   unsigned long outLen = getBufferSize();
   ByteBuffer *ret = new ByteBuffer(outLen);
   base64_decode(getBuffer(), getBufferSize(), ret->getBuffer(), &outLen);
   ret->resize(outLen);
   return ret;
}

RefPtr<ByteBuffer> ByteBuffer::encodeBase16() const
{
   U32 outLen = getBufferSize() * 2 + 1;
   ByteBuffer *ret = new ByteBuffer(outLen);
   const U8 *buffer = getBuffer();
   U8 *outBuffer = ret->getBuffer();

   S32 size = getBufferSize();
   for(S32 i = 0; i < size; i++)
   {
      U8 b = *buffer++;
      U32 nib1 = b >> 4;
      U32 nib2 = b & 0xF;
      if(nib1 > 9)
         *outBuffer++ = 'a' + nib1 - 10;
      else
         *outBuffer++ = '0' + nib1;
      if(nib2 > 9)
         *outBuffer++ = 'a' + nib2 - 10;
      else
         *outBuffer++ = '0' + nib2;
   }
   *outBuffer = 0;
   return ret;
}

RefPtr<ByteBuffer> ByteBuffer::decodeBase16() const
{
   U32 outLen = getBufferSize() >> 1;
   ByteBuffer *ret = new ByteBuffer(outLen);
   const U8 *src = getBuffer();
   U8 *dst = ret->getBuffer();
   for(U32 i = 0; i < outLen; i++)
   {
      U8 out = 0;
      U8 nib1 = *src++;
      U8 nib2 = *src++;
      if(nib1 >= '0' && nib1 <= '9')
         out = (nib1 - '0') << 4;
      else if(nib1 >= 'a' && nib1 <= 'f')
         out = (nib1 - 'a' + 10) << 4;
      else if(nib1 >= 'A' && nib1 <= 'A')
         out = (nib1 - 'A' + 10) << 4;
      if(nib2 >= '0' && nib2 <= '9')
         out |= nib2 - '0';
      else if(nib2 >= 'a' && nib2 <= 'f')
         out |= nib2 - 'a' + 10;
      else if(nib2 >= 'A' && nib2 <= 'A')
         out |= nib2 - 'A' + 10;
      *dst++ = out;
   }
   return ret;
}

RefPtr<ByteBuffer> ByteBuffer::computeMD5Hash(U32 len) const
{
   if(!len)
      len = getBufferSize();

   ByteBuffer *ret = new ByteBuffer(16);
   hash_state md;
   md5_init(&md);
   md5_process(&md, (unsigned char *) getBuffer(), len);
   md5_done(&md, ret->getBuffer());
   return ret;
}

U32 ByteBuffer::calculateCRC(U32 start, U32 end, U32 crcVal) const
{
   static U32 crcTable[256];
   static bool crcTableValid = false;

   if(!crcTableValid)
   {
      U32 val;

      for(S32 i = 0; i < 256; i++)
      {
         val = i;
         for(S32 j = 0; j < 8; j++)
         {
            if(val & 0x01)
               val = 0xedb88320 ^ (val >> 1);
            else
               val = val >> 1;
         }
         crcTable[i] = val;
      }
      crcTableValid = true;
   }
   
   if(start >= mBufSize)
      return 0;
   if(end > mBufSize)
      end = mBufSize;

   // now calculate the crc
   const U8 * buf = getBuffer();
   for(U32 i = start; i < end; i++)
      crcVal = crcTable[(crcVal ^ buf[i]) & 0xff] ^ (crcVal >> 8);
   return(crcVal);
}

};
