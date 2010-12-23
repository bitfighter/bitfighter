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

#include "tnlCertificate.h"

namespace TNL {

void Certificate::parse()
{
   BitStream aStream(getBuffer(), getBufferSize());

   mPayload = new ByteBuffer(0);
   aStream.read(mPayload);
   mPublicKey = new AsymmetricKey(&aStream);
   mSignature = new ByteBuffer(0);

   mSignatureByteSize = aStream.getBytePosition();

   // advance the bit stream to the next byte:
   aStream.setBytePosition(aStream.getBytePosition());

   aStream.read(mSignature);

   if(aStream.isValid() && getBufferSize() == aStream.getBytePosition() && mPublicKey->isValid())
      mIsValid = true;
}


bool Certificate::validate(RefPtr<AsymmetricKey> signatoryPublicKey)
{
   if(!mIsValid)
      return false;

   ByteBuffer signedBytes(getBuffer(), mSignatureByteSize);
   
   return signatoryPublicKey->verifySignature(signedBytes, *mSignature);
}

Certificate::Certificate(const ByteBuffer &payload, RefPtr<AsymmetricKey> publicKey, RefPtr<AsymmetricKey> theCAPrivateKey)
{
   mIsValid = false;
   mSignatureByteSize = 0;

   if(payload.getBufferSize() > MaxPayloadSize || !publicKey->isValid())
      return;

   ByteBufferPtr thePublicKey = publicKey->getPublicKey();
   PacketStream packet;

   packet.write(&payload);
   packet.write(thePublicKey);
   mSignatureByteSize = packet.getBytePosition();
   packet.setBytePosition(mSignatureByteSize);

   ByteBuffer theSignedBytes(packet.getBuffer(), packet.getBytePosition());

   mSignature = theCAPrivateKey->hashAndSign(theSignedBytes);
   packet.write(mSignature);

   setBuffer(packet.getBuffer(), packet.getBytePosition());
   takeOwnership();
}

};
