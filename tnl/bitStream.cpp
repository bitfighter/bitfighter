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

#include "tnlBitStream.h"
#include "tnlVector.h"
#include "tnlNetBase.h"
#include "tnlHuffmanStringProcessor.h"
#include "tnlSymmetricCipher.h"
#include "../libtomcrypt/mycrypt.h"

#include <math.h>

namespace TNL {

void BitStream::setMaxSizes(U32 maxReadSize, U32 maxWriteSize)
{
   maxReadBitNum = maxReadSize << 3;
   maxWriteBitNum = maxWriteSize << 3;
}

void BitStream::setMaxBitSizes(U32 maxReadSize, U32 maxWriteSize)
{
   maxReadBitNum = maxReadSize;
   maxWriteBitNum = maxWriteSize;
}

void BitStream::reset()
{
   bitNum = 0;
   error = false;
   mCompressRelative = false;
   mStringBuffer[0] = 0;
   mStringTable = NULL;
}

U8 *BitStream::getBytePtr()
{
   return getBuffer() + getBytePosition();
}

void BitStream::writeClassId(U32 classId, U32 classType, U32 classGroup)
{
   TNLAssert(classType < NetClassTypeCount, "Out of range class type.");
   TNLAssert(classId < NetClassRep::getNetClassCount(classGroup, classType), "Out of range class id.");
   writeInt(classId, NetClassRep::getNetClassBitSize(classGroup, classType));
}

U32 BitStream::readClassId(U32 classType, U32 classGroup)
{
   TNLAssert(classType < NetClassTypeCount, "Out of range class type.");
   U32 ret = readInt(NetClassRep::getNetClassBitSize(classGroup, classType));
   if(ret >= NetClassRep::getNetClassCount(classGroup, classType))
      return 0xFFFFFFFF;
   return ret;
}

bool BitStream::resizeBits(U32 newBits)
{
   U32 newSize = ((maxWriteBitNum + newBits + 7) >> 3) + ResizePad;
   if(!resize(newSize))
   {
      error = true;
      return false;
   }
   maxReadBitNum = newSize << 3;
   maxWriteBitNum = newSize << 3;
   return true;
}

bool BitStream::writeBits(U32 bitCount, const void *bitPtr)
{
   if(!bitCount)
      return true;

   if(bitCount + bitNum > maxWriteBitNum)
      if(!resizeBits(bitCount + bitNum - maxWriteBitNum))
         return false;

   U32 upShift  = bitNum & 0x7;
   U32 downShift= 8 - upShift;

   const U8 *sourcePtr = (U8 *) bitPtr;
   U8 *destPtr = getBuffer() + (bitNum >> 3);

   // if this write is for <= 1 byte, and it will all fit in the
   // first dest byte, then do some special masking.
   if(downShift >= bitCount)
   {
      U8 mask = ((1 << bitCount) - 1) << upShift;
      *destPtr = (*destPtr & ~mask) | ((*sourcePtr << upShift) & mask);
      bitNum += bitCount;
      return true;
   }

   // check for byte aligned writes -- this will be
   // much faster than the shifting writes.
   if(!upShift)
   {
      bitNum += bitCount;
      for(; bitCount >= 8; bitCount -= 8)
         *destPtr++ = *sourcePtr++;
      if(bitCount)
      {
         U8 mask = (1 << bitCount) - 1;
         *destPtr = (*sourcePtr & mask) | (*destPtr & ~mask);
      }
      return true;
   }

   // the write destination is not byte aligned.
   U8 sourceByte;
   U8 destByte = *destPtr & (0xFF >> downShift);
   U8 lastMask  = 0xFF >> (7 - ((bitNum + bitCount - 1) & 0x7));

   bitNum += bitCount;

   for(/* empty */;bitCount >= 8; bitCount -= 8)
   {
      sourceByte = *sourcePtr++;
      *destPtr++ = destByte | (sourceByte << upShift);
      destByte = sourceByte >> downShift;
   }
   if(bitCount == 0)
   {
      *destPtr = (*destPtr & ~lastMask) | (destByte & lastMask);
      return true;
   }
   if(bitCount <= downShift)
   {
      *destPtr = (*destPtr & ~lastMask) | ((destByte | (*sourcePtr << upShift)) & lastMask);
      return true;
   }
   sourceByte = *sourcePtr;

   *destPtr++ = destByte | (sourceByte << upShift);
   *destPtr = (*destPtr & ~lastMask) | ((sourceByte >> downShift) & lastMask);
   return true;
}

bool BitStream::readBits(U32 bitCount, void *bitPtr)
{
   if(!bitCount)
      return true;
   if(bitCount + bitNum > maxReadBitNum)
   {
      error = true;
      return false;
   }

   U8 *sourcePtr = getBuffer() + (bitNum >> 3);
   U32 byteCount = (bitCount + 7) >> 3;

   U8 *destPtr = (U8 *) bitPtr;

   U32 downShift = bitNum & 0x7;
   U32 upShift = 8 - downShift;

   if(!downShift)
   {
      while(byteCount--)
         *destPtr++ = *sourcePtr++;
      bitNum += bitCount;
      return true;
   }

   U8 sourceByte = *sourcePtr >> downShift;
   bitNum += bitCount;

   for(; bitCount >= 8; bitCount -= 8)
   {
      U8 nextByte = *++sourcePtr;
      *destPtr++ = sourceByte | (nextByte << upShift);
      sourceByte = nextByte >> downShift;
   }
   if(bitCount)
   {
      if(bitCount <= upShift)
      {
         *destPtr = sourceByte;
         return true;
      }
      *destPtr = sourceByte | ( (*++sourcePtr) << upShift);
   }
   return true;
}

bool BitStream::setBit(U32 bitCount, bool set)
{
   if(bitCount >= maxWriteBitNum)
      if(!resizeBits(bitCount - maxWriteBitNum + 1))
         return false;
   if(set)
      *(getBuffer() + (bitCount >> 3)) |= (1 << (bitCount & 0x7));
   else
      *(getBuffer() + (bitCount >> 3)) &= ~(1 << (bitCount & 0x7));
   return true;
}

bool BitStream::testBit(U32 bitCount)
{
   return (*(getBuffer() + (bitCount >> 3)) & (1 << (bitCount & 0x7))) != 0;
}

bool BitStream::writeFlag(bool val)
{
   if(bitNum + 1 > maxWriteBitNum)
      if(!resizeBits(1))
         return false;
   if(val)
      *(getBuffer() + (bitNum >> 3)) |= (1 << (bitNum & 0x7));
   else
      *(getBuffer() + (bitNum >> 3)) &= ~(1 << (bitNum & 0x7));
   bitNum++;
   return (val);
}

bool BitStream::write(const ByteBuffer *theBuffer)
{
   U32 size = theBuffer->getBufferSize();
   if(size > 1023)
      return false;

   writeInt(size, 10);
   return write(size, theBuffer->getBuffer());
}

bool BitStream::read(ByteBuffer *theBuffer)
{
   U32 size = readInt(10);    // Max size = 1024
   theBuffer->takeOwnership();
   theBuffer->resize(size);
   return read(size, theBuffer->getBuffer());
}

U32 BitStream::readInt(U8 bitCount)
{
   TNLAssert(bitCount <= 32, "bitCount must be less then 32, for 64 bit, use readInt64");
   U32 ret = 0;
   readBits(bitCount, &ret);
   ret = convertLEndianToHost(ret);

   // Clear bits that we didn't read.
   if(bitCount == 32)
      return ret;
   else
      ret &= (1 << bitCount) - 1;

   return ret;
}

U64 BitStream::readInt64(U8 bitCount)
{
   U64 ret = 0;
   readBits(bitCount, &ret);
   ret = convertLEndianToHost(ret);

   // Clear bits that we didn't read.
   if(bitCount == 64)
      return ret;
   else
      ret &= (U64(1) << bitCount) - 1;

   return ret;
}


void BitStream::writeInt(U32 val, U8 bitCount)
{
   TNLAssert(bitCount <= 32, "bitCount must be less then 32, for 64 bit, use writeInt64");
   val = convertHostToLEndian(val);
   writeBits(bitCount, &val);
}

void BitStream::writeInt64(U64 val, U8 bitCount)
{
   val = convertHostToLEndian(val);
   writeBits(bitCount, &val);
}

void BitStream::writeFloat(F32 f, U8 bitCount)
{
   TNLAssert(f >= 0 && f <= 1, "writeFloat Must be between 0.0 and 1.0");
   writeInt(U32(f * ((1 << bitCount) - 1) + 0.5f), bitCount);  // note that float to int will remove the decimal numbers.
}

F32 BitStream::readFloat(U8 bitCount)
{
   return readInt(bitCount) / F32((1 << bitCount) - 1);
}

void BitStream::writeSignedFloat(F32 f, U8 bitCount)
{
   writeSignedInt(S32(f * ((1 << (bitCount - 1)) - 1)), bitCount);
}

F32 BitStream::readSignedFloat(U8 bitCount)
{
   return readSignedInt(bitCount) / F32((1 << (bitCount - 1)) - 1);
}

void BitStream::writeSignedInt(S32 value, U8 bitCount)
{
   writeInt(value, bitCount);
}

#if ((-1) >> 1 != -1)
#error "Signed right shift error, your compiler doesn't support signed right shift?"
#endif

S32 BitStream::readSignedInt(U8 bitCount)
{
   return S32(readInt(bitCount)) << (32 - bitCount) >> (32 - bitCount);
}

void BitStream::writeNormalVector(const Point3F& vec, U8 bitCount)
{
   F32 phi   = F32(atan2(vec.x, vec.y) * FloatInversePi );
   F32 theta = F32(atan2(vec.z, sqrt(vec.x*vec.x + vec.y*vec.y)) * Float2InversePi);

   writeSignedFloat(phi, bitCount+1);
   writeSignedFloat(theta, bitCount);
}

void BitStream::readNormalVector(Point3F *vec, U8 bitCount)
{
   F32 phi   = readSignedFloat(bitCount+1) * FloatPi;
   F32 theta = readSignedFloat(bitCount) * FloatHalfPi;

   vec->x = sin(phi)*cos(theta);
   vec->y = cos(phi)*cos(theta);
   vec->z = sin(theta);
}

Point3F BitStream::dumbDownNormal(const Point3F& vec, U8 bitCount)
{
   U8 buffer[128];
   BitStream temp(buffer, 128);

   temp.writeNormalVector(vec, bitCount);
   temp.setBitPosition(0);

   Point3F ret;
   temp.readNormalVector(&ret, bitCount);
   return ret;
}

void BitStream::writeNormalVector(const Point3F& vec, U8 angleBitCount, U8 zBitCount)
{
   // if this is a z up or down vector, just write out a couple of bits
   // if z is -1 or 1, x and y must both be 0

   // FIXME: this should test if the vec.z is within zBitCount precision of 1
   // BJG - fixed, but may be inefficient. Lookup table?
   if(writeFlag(fabs(vec.z) >= (1.0f-(1.0f/zBitCount))))
      writeFlag(vec.z < 0);
   else
   {
      // write out the z value and the angle that x and y make around the z axis
      writeSignedFloat( vec.z, zBitCount );
      writeSignedFloat( atan2(vec.x,vec.y) * FloatInverse2Pi, angleBitCount );
   }
}

void BitStream::readNormalVector(Point3F * vec, U8 angleBitCount, U8 zBitCount)
{
   if(readFlag())
   {
      vec->z = readFlag() ? -1.0f : 1.0f;
      vec->x = 0;
      vec->y = 0;
   }
   else
   {
      vec->z = readSignedFloat(zBitCount);

      F32 angle = Float2Pi * readSignedFloat(angleBitCount);

      F32 mult = (F32) sqrt(1.0f - vec->z * vec->z);
      vec->x = mult * cos(angle);
      vec->y = mult * sin(angle);
   }
}

//----------------------------------------------------------------------------

void BitStream::clearPointCompression()
{
   mCompressRelative = false;
}

void BitStream::setPointCompression(const Point3F& p)
{
   mCompressRelative = true;
   mCompressPoint = p;
}

static U32 gBitCounts[4] = {
   16, 18, 20, 32
};

void BitStream::writePointCompressed(const Point3F& p,F32 scale)
{
   // Same # of bits for all axis
   Point3F vec;
   F32 invScale = 1 / scale;
   U32 type;
   if(mCompressRelative)
   {
     vec.x = p.x - mCompressPoint.x;
     vec.y = p.y - mCompressPoint.y;
     vec.z = p.z - mCompressPoint.z;

      F32 dist = (F32) sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z) * invScale;
      if(dist < (1 << 15))
         type = 0;
      else if(dist < (1 << 17))
         type = 1;
      else if(dist < (1 << 19))
         type = 2;
      else
         type = 3;
   }
   else
      type = 3;

   writeInt(type, 2);

   if (type != 3)
   {
      type = gBitCounts[type];
      writeSignedInt(S32(vec.x * invScale),type);
      writeSignedInt(S32(vec.y * invScale),type);
      writeSignedInt(S32(vec.z * invScale),type);
   }
   else
   {
      write(p.x);
      write(p.y);
      write(p.z);
   }
}


void BitStream::readPointCompressed(Point3F* p,F32 scale)
{
   // Same # of bits for all axis
   U32 type = readInt(2);

   if(type == 3)
   {
      read(&p->x);
      read(&p->y);
      read(&p->z);
   }
   else
   {
      type = gBitCounts[type];
      p->x = (F32) readSignedInt(type);
      p->y = (F32) readSignedInt(type);
      p->z = (F32) readSignedInt(type);

      p->x = mCompressPoint.x + p->x * scale;
      p->y = mCompressPoint.y + p->y * scale;
      p->z = mCompressPoint.z + p->z * scale;
   }
}


void BitStream::readString(char buf[256])
{
   if(readFlag())
   {
      S32 offset = readInt(8);
      HuffmanStringProcessor::readHuffBuffer(this, mStringBuffer + offset);
      strcpy(buf, mStringBuffer);
   }
   else
   {
      HuffmanStringProcessor::readHuffBuffer(this, buf);
      strcpy(mStringBuffer, buf);
   }
}

void BitStream::writeString(const char *string, U8 maxLen)     // maxlen defaults to 255
{
   if(!string)
      string = "";
   U8 j;
   for(j = 0; j < maxLen && mStringBuffer[j] == string[j] && string[j];j++)
      ;  // do nothing
   strncpy(mStringBuffer + j, string + j, maxLen - j);
   mStringBuffer[maxLen] = 0;

   if(writeFlag(j > 2))
   {
      writeInt(j, 8);
      HuffmanStringProcessor::writeHuffBuffer(this, string + j, maxLen - j);
   }
   else
      HuffmanStringProcessor::writeHuffBuffer(this, string, maxLen);
}


//--------------------------------------------------------------------
//--------------------------------------------------------------------

//----------------------------------------------------------------------------

void BitStream::readStringTableEntry(StringTableEntry *ste)
{
   if(mStringTable)
      *ste = mStringTable->readStringTableEntry(this);
   else
   {
      char buf[256];
      readString(buf);
      ste->set(buf);
   }
}

void BitStream::writeStringTableEntry(const StringTableEntry &ste)
{
   if(mStringTable)
      mStringTable->writeStringTableEntry(this, ste);
   else
      writeString(ste.getString());
}
//------------------------------------------------------------------------------

void BitStream::hashAndEncrypt(U32 hashDigestSize, U32 encryptStartOffset, SymmetricCipher *theCipher)
{
   U32 digestStart = getBytePosition();
   setBytePosition(digestStart);
   hash_state hashState;

   U8 hash[32];

   // do a sha256 hash of the BitStream:
   sha256_init(&hashState);
   sha256_process(&hashState, getBuffer(), digestStart);
   sha256_done(&hashState, hash);

   // write the hash into the BitStream:
   write(hashDigestSize, hash);

   theCipher->encrypt(getBuffer() + encryptStartOffset,
                      getBuffer() + encryptStartOffset,
                      getBytePosition() - encryptStartOffset);
}

bool BitStream::decryptAndCheckHash(U32 hashDigestSize, U32 decryptStartOffset, SymmetricCipher *theCipher)
{
   U32 bufferSize = getBufferSize();
   U8 *buffer = getBuffer();

   if(bufferSize < decryptStartOffset + hashDigestSize)
      return false;

   theCipher->decrypt(buffer + decryptStartOffset,
                      buffer + decryptStartOffset,
                      bufferSize - decryptStartOffset);

   hash_state hashState;
   U8 hash[32];

   sha256_init(&hashState);
   sha256_process(&hashState, buffer, bufferSize - hashDigestSize);
   sha256_done(&hashState, hash);

   bool ret = !memcmp(buffer + bufferSize - hashDigestSize, hash, hashDigestSize);
   if(ret)
      resize(bufferSize - hashDigestSize);
   return ret;
}

//------------------------------------------------------------------------------

NetError PacketStream::sendto(Socket &outgoingSocket, const Address &addr)
{
   return outgoingSocket.sendto(addr, buffer, getBytePosition());
}

NetError PacketStream::recvfrom(Socket &incomingSocket, Address *recvAddress)
{
   NetError error;
   S32 dataSize;
   error = incomingSocket.recvfrom(recvAddress, buffer, sizeof(buffer), &dataSize);
   setBuffer(buffer, dataSize);
   setMaxSizes(dataSize, 0);
   reset();
   return error;
}

};
