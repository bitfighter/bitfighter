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

#ifndef _TNL_BITSTREAM_H_
#define _TNL_BITSTREAM_H_

#ifndef _TNL_BYTEBUFFER_H_
#include "tnlByteBuffer.h"
#endif

#ifndef _TNL_CONNECTIONSTRINGTABLE_H_
#include "tnlConnectionStringTable.h"
#endif

#include "tnlHuffmanStringProcessor.h"    // For HuffmanStringProcessor::MAX_SENDABLE_LINE_LENGTH

#include "tnl.h"

namespace TNL {

class SymmetricCipher;

/// Point3F is used by BitStream for transmitting 3D points and vectors.
///
/// Users of the TNL should provide conversion operators to and from
/// this TNL-native type.
struct Point3F
{
   F32 x; ///< the X coordinate
   F32 y; ///< the Y coordinate
   F32 z; ///< the Z coordinate
};

/// Helper macro used in BitStream declaration.
///
/// @note DeclareTemplatizedReadWrite macro declares a read and write function
///       for the specified type. This is a macro because MSVC6 has seriously
///       broken template functionality.  Thanks MS!
#define DeclareTemplatizedReadWrite(T) \
inline bool write(T value) { T temp = convertHostToLEndian(value); return write(sizeof(T), &temp); } \
inline bool read(T *value) { T temp; bool success = read(sizeof(T), &temp); *value = convertLEndianToHost(temp); return success;}

/// BitStream provides a bit-level stream interface to a data buffer.
class BitStream : public ByteBuffer
{
protected:
   enum {
      ResizePad = 1500,
   };
   U32  bitNum;               ///< The current bit position for reading/writing in the bit stream.
   bool error;                ///< Flag set if a user operation attempts to read or write past the max read/write sizes.
   bool mCompressRelative;    ///< Flag set if the bit stream should compress points relative to a compression point.
   Point3F mCompressPoint;    ///< Reference point for relative point compression.
   U32  maxReadBitNum;        ///< Last valid read bit position.
   U32  maxWriteBitNum;       ///< Last valid write bit position.
   ConnectionStringTable *mStringTable; ///< String table used to compress StringTableEntries over the network.
   /// String buffer holds the last string written into the stream for substring compression.
   char mStringBuffer[256];

   bool resizeBits(U32 numBitsNeeded);
public:

   /// @name Constructors
   ///
   /// Note that the BitStream essentially wraps an existing buffer, so to use a bitstream you must
   /// have an existing buffer for it to work with!
   ///
   /// @{

   /// Default to maximum write size being the size of the buffer.
   BitStream(U8 *bufPtr, U32 bufSize) : ByteBuffer(bufPtr, bufSize) { setMaxSizes(bufSize, bufSize); reset(); }

   /// Optionally, specify a maximum write size.
   BitStream(U8 *bufPtr, U32 bufSize, U32 maxWriteSize) 
      : ByteBuffer(bufPtr, bufSize) { setMaxSizes(bufSize, maxWriteSize); reset(); }

   /// Creates a resizable BitStream
   BitStream() : ByteBuffer() { setMaxSizes( getBufferSize(), getBufferSize() ); reset(); }
   /// @}

   /// Sets the maximum read and write sizes for the BitStream.
   void setMaxSizes(U32 maxReadSize, U32 maxWriteSize = 0);

   /// Sets the maximum read and write bit sizes for the BitStream.
   void setMaxBitSizes(U32 maxReadBitSize, U32 maxWriteBitSize = 0);

   /// resets the read/write position to 0 and clears any error state.
   void reset();

   /// clears the string compression buffer.
   void clearStringBuffer() { mStringBuffer[0] = 0; }

   /// sets the ConnectionStringTable for compressing string table entries across the network
   void setStringTable(ConnectionStringTable *table) { mStringTable = table; }

   /// clears the error state from an attempted read or write overrun
   void clearError() { error = false; }

   /// Returns a pointer to the next byte in the BitStream from the current bit position
   U8*  getBytePtr();

   /// Returns the current position in the stream rounded up to the next byte.
   U32 getBytePosition() const;
   /// Returns the current bit position in the stream
   U32 getBitPosition() const;
   /// Sets the position in the stream to the first bit of byte newPosition.
   void setBytePosition(const U32 newPosition);
   /// Sets the position in the stream to newBitPosition.
   void setBitPosition(const U32 newBitPosition);
   /// Advances the position in the stream by numBits.
   void advanceBitPosition(const S32 numBits);

   /// Returns the maximum readable bit position
   U32 getMaxReadBitPosition() const { return maxReadBitNum; }

   /// Returns the number of bits that can be written into the BitStream without resizing
   U32 getBitSpaceAvailable() const { return maxWriteBitNum - bitNum; }

   /// Pads the bits up to the next byte boundary with 0's.
   void zeroToByteBoundary();

   /// Writes an unsigned integer value between 0 and 2^(bitCount - 1) into the stream.
   void writeInt(U32 value, U8 bitCount);
   /// Writes an unsigned integer value between 0 and 2^(bitCount - 1) into the stream.
   void writeInt64(U64 value, U8 bitCount);
   /// Reads an unsigned integer value between 0 and 2^(bitCount - 1) from the stream.
   U32  readInt(U8 bitCount);
   /// Reads 64-bit unsigned integer value between 0 and 2^(bitCount - 1) from the stream.
   U64  readInt64(U8 bitCount);

   /// Writes an unsigned integer value between 0 and 2^(bitCount -1) into the stream at the specified position, without changing the current write position.
   void writeIntAt(U32 value, U8 bitCount, U32 bitPosition);

   /// Writes a signed integer value between -2^(bitCount-1) and 2^(bitCount-1) - 1.
   void writeSignedInt(S32 value, U8 bitCount);
   /// Reads a signed integer value between -2^(bitCount-1) and 2^(bitCount-1) - 1.
   S32  readSignedInt(U8 bitCount);

   /// Writes an unsigned integer value in the range rangeStart to rangeEnd inclusive.
   void writeRangedU32(U32 value, U32 rangeStart, U32 rangeEnd);
   /// Reads an unsigned integer value in the range rangeStart to rangeEnd inclusive.
   U32  readRangedU32(U32 rangeStart, U32 rangeEnd);

   /// Writes an enumeration value in the range of 0 ... enumRange - 1.
   void writeEnum(U32 enumValue, U32 enumRange);
   /// Reads an enumeration value in the range 0 ... enumRange - 1.
   U32 readEnum(U32 enumRange);

   /// Writes a float from 0 to 1 inclusive, using bitCount bits of precision.
   void writeFloat(F32 f, U8 bitCount);
   /// Reads a float from 0 to 1 inclusive, using bitCount bits of precision.
   F32  readFloat(U8 bitCount);

   /// Writes a signed float from -1 to 1 inclusive, using bitCount bits of precision.
   void writeSignedFloat(F32 f, U8 bitCount);
   /// Reads a signed float from -1 to 1 inclusive, using bitCount bits of precision.
   F32  readSignedFloat(U8 bitCount);

   /// Writes an object's class ID, given its class type and class group.
   void writeClassId(U32 classId, U32 classType, U32 classGroup);
   /// Reads a class ID for an object, given a class type and class group.  Returns -1 if the class type is out of range
   U32 readClassId(U32 classType, U32 classGroup);

   /// Writes a normalized vector into the stream, using bitCount bits for the precision of angles phi and theta.
   void writeNormalVector(const Point3F &vec, U8 bitCount);
   /// Reads a normalized vector from the stream, using bitCount bits for the precision of angles phi and theta.
   void readNormalVector(Point3F *vec, U8 bitCount);

   /// Uses the same method as in writeNormalVector to reduce the precision of a normal vector
   /// to determine what will be read from the stream.
   static Point3F dumbDownNormal(const Point3F& vec, U8 bitCount);

   /// Writes a normalized vector by writing a z value and theta angle.
   void writeNormalVector(const Point3F& vec, U8 angleBitCount, U8 zBitCount);
   /// Reads a normalized vector by reading a z value and theta angle.
   void readNormalVector(Point3F *vec, U8 angleBitCount, U8 zBitCount);

   /// Sets a reference point for subsequent compressed point writing.
   void setPointCompression(const Point3F &p);
   /// Disables compression of point.
   void clearPointCompression();
   /// Writes a point into the stream, to a precision denoted by scale.
   void writePointCompressed(const Point3F &p, F32 scale);
   /// Reads a compressed point from the stream, to a precision denoted by scale.
   void readPointCompressed(Point3F *p, F32 scale);


   /// Writes bitCount bits into the stream from bitPtr.
   bool writeBits(U32 bitCount, const void *bitPtr);
   /// Reads bitCount bits from the stream into bitPtr.
   bool readBits(U32 bitCount, void *bitPtr);

   /// Writes a ByteBuffer into the stream.  The ByteBuffer can be no larger than 1024 bytes in size.
   bool write(const ByteBuffer *theBuffer);

   /// Reads a ByteBuffer in from the stream.
   bool read(ByteBuffer *theBuffer);

   /// Writes a single boolean flag (bit) into the stream, and returns the boolean that was written.
   ///
   /// This is set up so you can do...
   ///
   /// @code
   ///   if(stream->writeFlag(foo == bar))
   ///   {
   ///     ... write other stuff ...
   ///   }
   /// @endcode
   bool writeFlag(bool val);

   /// Reads a single bit from the stream.
   ///
   /// This is set up so you can do...
   ///
   /// @code
   ///   if(stream->readFlag())
   ///   {
   ///     ... read other stuff ...
   ///   }
   /// @endcode
   bool readFlag();

   bool write(bool value) { writeFlag(value); return !error; }
   bool read(bool *value) { *value = readFlag(); return !error; }

   /// Writes a huffman compressed string into the stream.
   void writeString(const char *stringBuf, U8 maxLen=HuffmanStringProcessor::MAX_SENDABLE_LINE_LENGTH);
   /// Reads a huffman compressed string from the stream.
   void readString(char stringBuf[256]);

   /// Writes a string table entry into the stream
   void writeStringTableEntry(const StringTableEntry &ste);

   /// Reads a string table entry from the stream
   void readStringTableEntry(StringTableEntry *ste);

   /// Writes byte data into the stream.
   bool write(const U32 in_numBytes, const void* in_pBuffer);
   /// Reads byte data from the stream.
   bool read(const U32 in_numBytes,  void* out_pBuffer);

   /// @name Various types that the BitStream can read and write...
   /// @{

   ///
   DeclareTemplatizedReadWrite(U8);
   DeclareTemplatizedReadWrite(S8);
   DeclareTemplatizedReadWrite(U16);
   DeclareTemplatizedReadWrite(S16);
   DeclareTemplatizedReadWrite(U32);
   DeclareTemplatizedReadWrite(S32);
   DeclareTemplatizedReadWrite(S64);
   DeclareTemplatizedReadWrite(U64);
   DeclareTemplatizedReadWrite(F32);
   DeclareTemplatizedReadWrite(F64);

   /// @}

   /// Sets the bit at position bitCount to the value of set
   bool setBit(U32 bitCount, bool set);
   /// Tests the value of the bit at position bitCount.
   bool testBit(U32 bitCount);

   /// Returns whether the BitStream writing has exceeded the write target size.
   bool isFull() { return bitNum > (getBufferSize() << 3); }
   /// Returns whether the stream has generated an error condition due to reading or writing past the end of the buffer.
   bool isValid() { return !error; }

   /// Hashes the BitStream, writing the hash digest into the end of the buffer, and then encrypts with the given cipher
   void hashAndEncrypt(U32 hashDigestSize, U32 encryptStartOffset, SymmetricCipher *theCipher);

   /// Decrypts the BitStream, then checks the hash digest at the end of the buffer to validate the contents
   bool decryptAndCheckHash(U32 hashDigestSize, U32 decryptStartOffset, SymmetricCipher *theCipher);
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


inline U32 BitStream::getBytePosition() const
{
   return (bitNum + 7) >> 3;
}

inline U32 BitStream::getBitPosition() const
{
   return bitNum;
}

inline void BitStream::setBytePosition(const U32 newPosition)
{
   bitNum = newPosition << 3;
}

inline void BitStream::setBitPosition(const U32 newBitPosition)
{
   bitNum = newBitPosition;
}

inline void BitStream::advanceBitPosition(const S32 numBits)
{
   setBitPosition(getBitPosition() + numBits);
}

inline void BitStream::zeroToByteBoundary()
{
   if(bitNum & 0x7)
      writeInt(0, 8 - (bitNum & 0x7));
}

inline bool BitStream::write(const U32 in_numBytes, const void* in_pBuffer)
{
   return writeBits(in_numBytes << 3, in_pBuffer);
}

inline bool BitStream::read(const U32 in_numBytes,  void* out_pBuffer)
{
   return readBits(in_numBytes << 3, out_pBuffer);
}

inline bool BitStream::readFlag()
{
   if(bitNum > maxReadBitNum)
   {
      error = true;
      TNLAssert(false, "Out of range read");
      return false;
   }
   S32 mask = 1 << (bitNum & 0x7);
   bool ret = (*(getBuffer() + (bitNum >> 3)) & mask) != 0;
   bitNum++;
   return ret;
}
//extern void logprintf(const char *format, ...);

inline void BitStream::writeIntAt(U32 value, U8 bitCount, U32 bitPosition)
{
   U32 curPos = getBitPosition();
   setBitPosition(bitPosition);
   writeInt(value, bitCount);
   setBitPosition(curPos);
}

// #include <stdio.h>
inline void BitStream::writeRangedU32(U32 value, U32 rangeStart, U32 rangeEnd)
{
   TNLAssertV(value >= rangeStart && value <= rangeEnd, ("Out of bounds value! [val=%d, start=%d, end=%d]", value, rangeStart, rangeEnd));
   TNLAssert(rangeEnd >= rangeStart, "error, end of range less than start in writeRangedU32()");

   U32 rangeSize = rangeEnd - rangeStart + 1;
   U32 rangeBits = getNextBinLog2(rangeSize);

   writeInt(S32(value - rangeStart), S32(rangeBits));
}

inline U32 BitStream::readRangedU32(U32 rangeStart, U32 rangeEnd)
{
   TNLAssert(rangeEnd >= rangeStart, "error, end of range less than start in readRangedU32()");

   U32 rangeSize = rangeEnd - rangeStart + 1;
   U32 rangeBits = getNextBinLog2(rangeSize);

   U32 val = U32(readInt(S32(rangeBits)));
   return val + rangeStart;
}

inline void BitStream::writeEnum(U32 enumValue, U32 enumRange)
{
   writeInt(enumValue, getNextBinLog2(enumRange));
}

inline U32 BitStream::readEnum(U32 enumRange)
{
   return U32(readInt(getNextBinLog2(enumRange)));
}

/// PacketStream provides a network interface to the BitStream for easy construction of data packets.
class PacketStream : public BitStream
{
   typedef BitStream Parent;
   U8 buffer[MaxPacketDataSize]; ///< internal buffer for packet data, sized to the maximum UDP packet size.
public:
   /// Constructor assigns the internal buffer to the BitStream.
   PacketStream(U32 targetPacketSize = MaxPacketDataSize) : BitStream(buffer, targetPacketSize, MaxPacketDataSize) { buffer[0] = 0; }
   /// Sends this packet to the specified address through the specified socket.
   NetError sendto(Socket &outgoingSocket, const Address &theAddress);
   /// Reads a packet into the stream from the specified socket.
   NetError recvfrom(Socket &incomingSocket, Address *recvAddress);
};


};
#endif //_TNL_BITSTREAM_H_
