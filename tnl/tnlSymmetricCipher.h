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

#ifndef _TNL_SYMMETRICCIPHER_H_
#define _TNL_SYMMETRICCIPHER_H_

#ifndef _TNL_NETBASE_H_
#include "tnlNetBase.h"
#endif

#include "../libtomcrypt/mycrypt.h"
#undef MD5 // mycrypt_custom.h defines this, clashing with the MD5 class

namespace TNL
{

class ByteBuffer;

/// Class for symmetric encryption of data across a connection.  Internally it uses
/// the libtomcrypt AES algorithm to encrypt the data.
class SymmetricCipher : public Object
{
public:
   enum {
      BlockSize = 16,
      KeySize = 16,
   };
private:
   U32 mCounter[BlockSize >> 2];
   U32 mInitVector[BlockSize];
   U8 mPad[BlockSize];
   symmetric_key mSymmetricKey;
   U32 mPadLen;
public:
   SymmetricCipher(const U8 symmetricKey[KeySize], const U8 initVector[BlockSize]);
   SymmetricCipher(const ByteBuffer *theByteBuffer);

   void setupCounter(U32 counterValue1, U32 counterValue2, U32 counterValue3, U32 counterValue4);
   void encrypt(const U8 *plainText, U8 *cipherText, U32 len);
   void decrypt(const U8 *cipherText, U8 *plainText, U32 len);
};

};

#endif
