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

#include "tnlAsymmetricKey.h"
#include "tnlRandom.h"
#include "../libtomcrypt/mycrypt.h"
#include "tnlLog.h"

#define crypto_key            ecc_key
#define crypto_make_key       ecc_make_key
#define crypto_free           ecc_free
#define crypto_import         ecc_import
#define crypto_export         ecc_export
#define crypto_shared_secret  ecc_shared_secret

namespace TNL
{

enum {
   StaticCryptoBufferSize = 2048,
};

static U8 staticCryptoBuffer[StaticCryptoBufferSize];

AsymmetricKey::AsymmetricKey(U32 keySize)
{
   mIsValid = false;

   int descriptorIndex = register_prng ( &yarrow_desc );
   crypto_key *theKey = (crypto_key *) malloc(sizeof(crypto_key));

   if( crypto_make_key((prng_state *) Random::getState(), descriptorIndex,
      keySize, theKey) != CRYPT_OK )
      return;

   mKeyData = theKey;
   mKeySize = keySize;

   unsigned long bufferLen = sizeof(staticCryptoBuffer) - sizeof(U32) - 1;

   staticCryptoBuffer[0] = KeyTypePrivate;
   writeU32ToBuffer(mKeySize, staticCryptoBuffer + 1);

   crypto_export(staticCryptoBuffer + sizeof(U32) + 1, &bufferLen, PK_PRIVATE, theKey);
   bufferLen += sizeof(U32) + 1;

   mPrivateKey = new ByteBuffer(staticCryptoBuffer, bufferLen);
   mPrivateKey->takeOwnership();

   bufferLen = sizeof(staticCryptoBuffer) - sizeof(U32) - 1;

   staticCryptoBuffer[0] = KeyTypePublic;
   writeU32ToBuffer(mKeySize, staticCryptoBuffer + 1);

   crypto_export(staticCryptoBuffer + sizeof(U32) + 1, &bufferLen, PK_PUBLIC, theKey);
   bufferLen += sizeof(U32) + 1;

   mPublicKey = new ByteBuffer(staticCryptoBuffer, bufferLen);
   mPublicKey->takeOwnership();

   mHasPrivateKey = true;
   mIsValid = true;
}

AsymmetricKey::~AsymmetricKey()
{
   if(mKeyData)
   {
      crypto_free((crypto_key *) mKeyData);
      free(mKeyData);
   }
}

void AsymmetricKey::load(const ByteBuffer &theBuffer)
{
   mIsValid = false;

   crypto_key *theKey = (crypto_key *) malloc(sizeof(crypto_key));
   const U8 *bufferPtr = theBuffer.getBuffer();

   mHasPrivateKey = bufferPtr[0] == KeyTypePrivate;

   U32 bufferSize = theBuffer.getBufferSize();
   if(bufferSize < sizeof(U32) + 1)
      return;

   mKeySize = readU32FromBuffer(bufferPtr + 1);

   if( crypto_import(bufferPtr + sizeof(U32) + 1, bufferSize - sizeof(U32) - 1, theKey)
         != CRYPT_OK)
      return;

   mKeyData = theKey;

   if(mHasPrivateKey)
   {
      unsigned long bufferLen = sizeof(staticCryptoBuffer) - sizeof(U32) - 1;
      staticCryptoBuffer[0] = KeyTypePublic;

      writeU32ToBuffer(mKeySize, staticCryptoBuffer);

      if( crypto_export(staticCryptoBuffer + sizeof(U32) + 1, &bufferLen, PK_PUBLIC, theKey)
            != CRYPT_OK )
         return;

      bufferLen += sizeof(U32) + 1;

      mPublicKey = new ByteBuffer(staticCryptoBuffer, bufferLen);
      mPublicKey->takeOwnership();
      mPrivateKey = new ByteBuffer(theBuffer);
      mPrivateKey->takeOwnership();
   }
   else
   {
      mPublicKey = new ByteBuffer(theBuffer);
      mPublicKey->takeOwnership();
   }
   mIsValid = true;
}

ByteBufferPtr AsymmetricKey::computeSharedSecretKey(AsymmetricKey *publicKey)
{
   if(publicKey->getKeySize() != getKeySize() || !mHasPrivateKey)
      return NULL;

   U8 hash[32];
   unsigned long outLen = sizeof(staticCryptoBuffer);

   TIME_BLOCK(secretSubKeyGen,
   crypto_shared_secret((crypto_key *) mKeyData, (crypto_key *) publicKey->mKeyData,
      staticCryptoBuffer, &outLen);
   )
   hash_state hashState;
   sha256_init(&hashState);
   sha256_process(&hashState, staticCryptoBuffer, outLen);
   sha256_done(&hashState, hash);
   ByteBuffer *ret = new ByteBuffer(hash, 32);
   ret->takeOwnership();
   return ret;
}

ByteBufferPtr AsymmetricKey::hashAndSign(const ByteBuffer &theByteBuffer)
{
   int descriptorIndex = register_prng ( &yarrow_desc );

   U8 hash[32];
   hash_state hashState;

   sha256_init(&hashState);
   sha256_process(&hashState, theByteBuffer.getBuffer(), theByteBuffer.getBufferSize());
   sha256_done(&hashState, hash);

   unsigned long outlen = sizeof(staticCryptoBuffer);

   ecc_sign_hash(hash, 32,
      staticCryptoBuffer, &outlen,
      (prng_state *) Random::getState(), descriptorIndex, (crypto_key *) mKeyData);

   return new ByteBuffer(staticCryptoBuffer, (U32) outlen);
}

bool AsymmetricKey::verifySignature(const ByteBuffer &theByteBuffer, const ByteBuffer &theSignature)
{
   U8 hash[32];
   hash_state hashState;

   sha256_init(&hashState);
   sha256_process(&hashState, theByteBuffer.getBuffer(), theByteBuffer.getBufferSize());
   sha256_done(&hashState, hash);

   int stat;

   ecc_verify_hash(theSignature.getBuffer(), theSignature.getBufferSize(), hash, 32, &stat, (crypto_key *) mKeyData);
   return stat != 0;
}

};
