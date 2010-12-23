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

#ifndef _TNL_ASYMMETRICKEY_H_
#define _TNL_ASYMMETRICKEY_H_

#ifndef _TNL_BITSTREAM_H_
#include "tnlBitStream.h"
#endif


namespace TNL {

class AsymmetricKey : public Object
{
   /// Raw key data.
   ///
   /// The specific format of this buffer varies by cryptosystem, so look
   /// at the subclass to see how it's laid out. In general this is an
   /// opaque buffer.
   void *mKeyData;

   /// Size of the key at construct time.
   U32 mKeySize;

   /// Do we have the private key for this?
   ///
   /// We may only have access to the public key (for instance, when validating
   /// a message signed by someone else).
   bool mHasPrivateKey;

   /// Buffer containing the public half of this keypair.
   ByteBufferPtr mPublicKey;

   /// Buffer containing the private half of this keypair.
   ByteBufferPtr mPrivateKey;
   bool mIsValid;

   /// Load keypair from a buffer.
   void load(const ByteBuffer &theBuffer);

   /// Enum used to indicate the portion of the key we are working with.
   enum KeyType {
      KeyTypePrivate,
      KeyTypePublic,
   };
public:

   /// Constructs an AsymmetricKey from the specified data pointer.
   AsymmetricKey(U8 *dataPtr, U32 bufferSize) : mKeyData(NULL)
   {
      load(ByteBuffer(dataPtr, bufferSize));
   }

   /// Constructs an AsymmetricKey from a ByteBuffer.
   AsymmetricKey(const ByteBuffer &theBuffer) : mKeyData(NULL)
   {
      load(theBuffer);
   }

   /// Constructs an AsymmetricKey by reading it from a BitStream.
   AsymmetricKey(BitStream *theStream) : mKeyData(NULL)
   {
      ByteBuffer theBuffer;
      theStream->read(&theBuffer);
      load(theBuffer);
   }

   /// Generates a new asymmetric key of keySize bytes
   AsymmetricKey(U32 keySize);
   
   /// Destructor for the AsymmetricKey.
   ~AsymmetricKey();

   /// Returns a ByteBuffer containing an encoding of the public key.
   ByteBufferPtr getPublicKey() { return mPublicKey; }

   /// Returns a ByteBuffer containing an encoding of the private key.
   ByteBufferPtr getPrivateKey() { return mPrivateKey; }

   /// Returns true if this AsymmetricKey is a key pair.
   bool hasPrivateKey() { return mHasPrivateKey; }

   /// Returns true if this is a valid key.
   bool isValid() { return mIsValid; }
   /// Compute a key we can share with the specified AsymmetricKey
   /// for a symmetric crypto.
   ByteBufferPtr computeSharedSecretKey(AsymmetricKey *publicKey);

   /// Returns the strength of the AsymmetricKey in byte size.
   U32 getKeySize() { return mKeySize; }

   /// Constructs a digital signature for the specified buffer of bits.  This
   /// method only works for private keys.  A public key only Asymmetric key
   /// will generate a signature of 0 bytes in length.
   ByteBufferPtr hashAndSign(const ByteBuffer &theByteBuffer);

   /// Returns true if the private key associated with this AsymmetricKey 
   /// signed theByteBuffer with theSignature.
   bool verifySignature(const ByteBuffer &theByteBuffer, const ByteBuffer &theSignature);
};

typedef RefPtr<AsymmetricKey> AsymmetricKeyPtr;

};

#endif
