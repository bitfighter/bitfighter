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

#ifndef _TNL_CERTIFICATE_H_
#define _TNL_CERTIFICATE_H_

#ifndef _TNL_BYTEBUFFER_H_
#include "tnlByteBuffer.h"
#endif

#ifndef _TNL_ASYMMETRICKEY_H_
#include "tnlAsymmetricKey.h"
#endif

namespace TNL
{

/// The Certificate class manages a digitally signed certificate.
/// Certificates consist of an application-defined payload, a public
/// key, and a signature.  It is up to the application to determine
/// from the payload what, if any, certificate authority (CA) signed
/// the certificate.  The validate() method can be used to check the
/// certificate's authenticity against the known public key of the
/// signing Certificate Authority.
///
/// The payload could include such items as:
///  - The valid date range for the certificate
///  - The domain of the certificate holder
///  - The identifying name of the Certificate Authority
///  - A player's name and user id for a multiplayer game
///
/// 
class Certificate : public ByteBuffer
{
protected:
   RefPtr<AsymmetricKey> mPublicKey;      ///< The public key for the holder of this certificate
   ByteBufferPtr mPayload;                ///< The certificate payload, including the identity of the holder and the Certificate Authority
   ByteBufferPtr mSignature;              ///< The digital signature of this certificate by the signatory
   bool mIsValid;                         ///< flag to signify whether this certificate has a valid form
   U32 mSignatureByteSize;                ///< Number of bytes of the ByteBuffer signed by the CA
public:
   enum {
      MaxPayloadSize = 512,
   };
   /// Certificate constructor
   Certificate(U8 *dataPtr, U32 dataSize) : ByteBuffer(dataPtr, dataSize)
   {
      mSignatureByteSize = 0;
      mIsValid = false;
      takeOwnership();
      parse();
   }

   Certificate(const ByteBuffer &buffer) : ByteBuffer(buffer)
   {
      mSignatureByteSize = 0;
      mIsValid = false;
      takeOwnership();
      parse();
   }
   Certificate(BitStream *stream)
   {
      mSignatureByteSize = 0;
      mIsValid = false;
      stream->read(this);
      parse();
   }
   Certificate(const ByteBuffer &payload, RefPtr<AsymmetricKey> publicKey, RefPtr<AsymmetricKey> theCAPrivateKey);

   void parse(); ///< Parses this certificate into the payload, public key, identiy, certificate authority and signature

   /// returns the validity of the certificate's formation
   bool isValid()
   {
      return mIsValid;
   }
   /// returns true if this certificate was signed by the private key corresponding to the passed public key.
   bool validate(RefPtr<AsymmetricKey> signatoryPublicKey);

   /// Returns the public key from the certificate
   RefPtr<AsymmetricKey> getPublicKey() { return mPublicKey; }

   /// Returns the certificate payload
   ByteBufferPtr getPayload() { return mPayload; }
};

};

#endif
