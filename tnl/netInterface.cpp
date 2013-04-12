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

#include "tnl.h"
#include "tnlAsymmetricKey.h"
#include "tnlNetConnection.h"
#include "tnlNetInterface.h"
#include "tnlBitStream.h"
#include "tnlRandom.h"
#include "tnlNetObject.h"
#include "tnlClientPuzzle.h"
#include "tnlCertificate.h"
#include "../libtomcrypt/mycrypt.h"

namespace TNL {

//-----------------------------------------------------------------------------
// NetInterface initialization/destruction
//-----------------------------------------------------------------------------

NetInterface::NetInterface(const Address &bindAddress) : mSocket(bindAddress)
{
   NetClassRep::initialize(); // initialize the net class reps, if they haven't been initialized already.

   mLastTimeoutCheckTime = 0;
   mAllowConnections = true;
   mRequiresKeyExchange = false;

   Random::read(mRandomHashData, sizeof(mRandomHashData));

   mConnectionHashTable.resize(129);
   for(S32 i = 0; i < mConnectionHashTable.size(); i++)
      mConnectionHashTable[i] = NULL;
   mSendPacketList = NULL;
   mCurrentTime = Platform::getRealMilliseconds();
}

NetInterface::~NetInterface()
{
   // gracefully close all the connections on this NetInterface:
   while(mConnectionList.size())
   {
      NetConnection *c = mConnectionList[0];
      disconnect(c, NetConnection::ReasonShutdown, "");
   }
   while(mSendPacketList)
   {
      DelaySendPacket *next = mSendPacketList->nextPacket;
      mSendPacketList->~DelaySendPacket(); // properly free stuff like SafePtr
      free(mSendPacketList);
      mSendPacketList = next;
   }
}

Address NetInterface::getFirstBoundInterfaceAddress()
{
   Address theAddress = mSocket.getBoundAddress();

   if(theAddress.isEqualAddress(Address(IPProtocol, Address::Any, 0)))
   {
      Vector<Address> interfaceAddresses;
      Socket::getInterfaceAddresses(&interfaceAddresses);
      U16 savePort = theAddress.port;
      if(interfaceAddresses.size())
      {
         theAddress = interfaceAddresses[0];
         theAddress.port = savePort;
      }
   }
   return theAddress;
}

void NetInterface::setPrivateKey(AsymmetricKey *theKey)
{
   mPrivateKey = theKey;
}

//-----------------------------------------------------------------------------
// NetInterface packet sending functions
//-----------------------------------------------------------------------------

NetError NetInterface::sendto(const Address &address, BitStream *stream)
{
   return mSocket.sendto(address, stream->getBuffer(), stream->getBytePosition());
}

void NetInterface::sendtoDelayed(const Address *address, NetConnection *receiveTo, BitStream *stream, U32 millisecondDelay)
{
   U32 dataSize = stream->getBytePosition();

   // allocate the send packet, with the data size added on
   DelaySendPacket *thePacket = (DelaySendPacket *) malloc(sizeof(DelaySendPacket) + dataSize);
   new(thePacket) DelaySendPacket(); // Initalizes SafePtr
   if(thePacket->isReceive = (address == NULL))
      thePacket->receiveTo = receiveTo;
   else
      thePacket->remoteAddress = *address;
   thePacket->sendTime = getCurrentTime() + millisecondDelay;
   thePacket->packetSize = dataSize;
   memcpy(thePacket->packetData, stream->getBuffer(), dataSize);

   // insert it into the DelaySendPacket list, sorted by time
   DelaySendPacket **list;
   for(list = &mSendPacketList; *list && ((*list)->sendTime <= thePacket->sendTime); list = &((*list)->nextPacket))
      ;
   thePacket->nextPacket = *list;
   *list = thePacket;
}

//-----------------------------------------------------------------------------
// NetInterface utility functions
//-----------------------------------------------------------------------------

U32 NetInterface::computeClientIdentityToken(const Address &address, const Nonce &theNonce)
{
   hash_state hashState;
   U32 hash[8];

   sha256_init(&hashState);
   sha256_process(&hashState, (const U8 *) &address.port, sizeof(address.port));
   sha256_process(&hashState, (const U8 *) address.netNum, sizeof(address.netNum));
   sha256_process(&hashState, theNonce.data, Nonce::NonceSize);
   sha256_process(&hashState, mRandomHashData, sizeof(mRandomHashData));
   sha256_done(&hashState, (U8 *) hash);

   return hash[0];
}

//-----------------------------------------------------------------------------
// NetInterface pending connection list management
//-----------------------------------------------------------------------------

void NetInterface::addPendingConnection(NetConnection *connection)
{
   // Make sure we're not already connected to the host at the
   // connection's Address
   findAndRemovePendingConnection(connection->getNetAddress());
   NetConnection *temp = findConnection(connection->getNetAddress());

   if(temp)
      disconnect(temp, NetConnection::ReasonSelfDisconnect, "Reconnecting");

   // Hang on to the connection and add it to the pending connection list
   connection->incRef();
   mPendingConnections.push_back(connection);   // (only place where something is added to the mPendingConnections list)
}


// Search the pending connection list for the specified connection and remove it
void NetInterface::removePendingConnection(NetConnection *connection)
{
   for(S32 i = 0; i < mPendingConnections.size(); i++)
      if(mPendingConnections[i] == connection)
      {
         connection->decRef();
         mPendingConnections.erase(i);
         return;
      }
}


NetConnection *NetInterface::findPendingConnection(const Address &address)
{
   // Loop through all the pending connections and compare the NetAddresses
   for(S32 i = 0; i < mPendingConnections.size(); i++)
      if(address == mPendingConnections[i]->getNetAddress())
         return mPendingConnections[i];
   return NULL;
}

void NetInterface::findAndRemovePendingConnection(const Address &address)
{
   // Search through the list by Address and remove any connection
   // that matches.
   for(S32 i = 0; i < mPendingConnections.size(); i++)
      if(address == mPendingConnections[i]->getNetAddress())
      {
         mPendingConnections[i]->decRef();
         mPendingConnections.erase(i);
         return;
      }
}

//-----------------------------------------------------------------------------
// NetInterface connection list management
//-----------------------------------------------------------------------------

NetConnection *NetInterface::findConnection(const Address &addr)
{
   // The connection hash table is a single vector, with hash collisions
   // resolved to the next open space in the table.

   // Compute the hash index based on the network address
   U32 hashIndex = addr.hash() % mConnectionHashTable.size();

   // Search through the table for an address that matches the source
   // address.  If the connection pointer is NULL, we've found an
   // empty space and a connection with that address is not in the table
   while(mConnectionHashTable[hashIndex] != NULL)
   {
      if(addr == mConnectionHashTable[hashIndex]->getNetAddress())
         return mConnectionHashTable[hashIndex];
      hashIndex++;
      if(hashIndex >= (U32) mConnectionHashTable.size())
         hashIndex = 0;
   }
   return NULL;
}

void NetInterface::removeConnection(NetConnection *conn)
{
   for(S32 i = 0; i < mConnectionList.size(); i++)
   {
      if(mConnectionList[i] == conn)
      {
         mConnectionList.erase_fast(i);
         break;
      }
   }
   U32 index = conn->getNetAddress().hash() % mConnectionHashTable.size();
   U32 startIndex = index;

   while(mConnectionHashTable[index] != conn)
   {
      index++;
      if(index >= (U32) mConnectionHashTable.size())
         index = 0;
      TNLAssert(index != startIndex, "Attempting to remove a connection that is not in the table."); // not in the table
      if(index == startIndex)
         return;
   }
   mConnectionHashTable[index] = NULL;

   // rehash all subsequent entries until we find a NULL entry:
   for(;;)
   {
      index++;
      if(index >= (U32) mConnectionHashTable.size())
         index = 0;
      if(!mConnectionHashTable[index])
         break;
      NetConnection *rehashConn = mConnectionHashTable[index];
      mConnectionHashTable[index] = NULL;
      U32 realIndex = rehashConn->getNetAddress().hash() % mConnectionHashTable.size();
      while(mConnectionHashTable[realIndex] != NULL)
      {
         realIndex++;
         if(realIndex >= (U32) mConnectionHashTable.size())
            realIndex = 0;
      }
      mConnectionHashTable[realIndex] = rehashConn;
   }
   conn->decRef();
}

void NetInterface::addConnection(NetConnection *conn)
{
   conn->incRef();
   mConnectionList.push_back(conn);

   S32 numConnections = mConnectionList.size();

   if(numConnections > mConnectionHashTable.size() / 2)
   {
      mConnectionHashTable.resize(numConnections * 4 - 1);
      for(S32 i = 0; i < mConnectionHashTable.size(); i++)
         mConnectionHashTable[i] = NULL;

      for(S32 i = 0; i < numConnections; i++)
      {
         U32 index = mConnectionList[i]->getNetAddress().hash() % mConnectionHashTable.size();
         while(mConnectionHashTable[index] != NULL)
         {
            index++;
            if(index >= (U32) mConnectionHashTable.size())
               index = 0;
         }
         mConnectionHashTable[index] = mConnectionList[i];
      }
   }
   else
   {
      U32 index = mConnectionList[numConnections - 1]->getNetAddress().hash() % mConnectionHashTable.size();
      while(mConnectionHashTable[index] != NULL)
      {
         index++;
         if(index >= (U32) mConnectionHashTable.size())
            index = 0;
      }
      mConnectionHashTable[index] = mConnectionList[numConnections - 1];
   }
}

//-----------------------------------------------------------------------------
// NetInterface timeout and packet send processing
//-----------------------------------------------------------------------------

void NetInterface::processConnections()
{
   mCurrentTime = Platform::getRealMilliseconds();
   mPuzzleManager.tick(mCurrentTime);

   // first see if there are any delayed packets that need to be sent...
   while(mSendPacketList && S32(mSendPacketList->sendTime - getCurrentTime()) < 0)
   {
      DelaySendPacket *next = mSendPacketList->nextPacket;
      if(mSendPacketList->isReceive)
      {
         if(mSendPacketList->receiveTo.isValid())
         {
            BitStream b(mSendPacketList->packetData, mSendPacketList->packetSize);
            b.setMaxSizes(mSendPacketList->packetSize, 0);
            b.reset();
            RefPtr<NetConnection> conn = mSendPacketList->receiveTo.getPointer(); // if this packet causes a disconnection, keep the conn until this function exits
            conn->readRawPacket(&b);
         }
      }
      else
      {
         mSocket.sendto(mSendPacketList->remoteAddress,
            mSendPacketList->packetData, mSendPacketList->packetSize);
      }
      mSendPacketList->~DelaySendPacket(); // properly free stuff like SafePtr
      free(mSendPacketList);
      mSendPacketList = next;
   }

   NetObject::collapseDirtyList(); // collapse all the mask bits...
   for(S32 i = 0; i < mConnectionList.size(); i++)
      mConnectionList[i]->checkPacketSend(false, getCurrentTime());

   if(U32(getCurrentTime() - mLastTimeoutCheckTime) > TimeoutCheckInterval)
   {
      for(S32 i = 0; i < mPendingConnections.size();)
      {
         NetConnection *pending = mPendingConnections[i];

         if(pending->getConnectionState() == NetConnection::AwaitingChallengeResponse &&
            getCurrentTime() - pending->mConnectLastSendTime > ChallengeRetryTime)
         {
            if(pending->mConnectSendCount > ChallengeRetryCount)
            {
               pending->setConnectionState(NetConnection::ConnectTimedOut);
               pending->onConnectTerminated(NetConnection::ReasonTimedOut, "Timeout");
               removePendingConnection(pending);
               continue;
            }
            else
               sendConnectChallengeRequest(pending);
         }
         else if(pending->getConnectionState() == NetConnection::AwaitingConnectResponse &&
                                    getCurrentTime() - pending->mConnectLastSendTime > ConnectRetryTime)
         {
            if(pending->mConnectSendCount > ConnectRetryCount)
            {
               pending->setConnectionState(NetConnection::ConnectTimedOut);
               pending->onConnectTerminated(NetConnection::ReasonTimedOut, "Timeout");
               removePendingConnection(pending);
               continue;
            }
            else
            {
               if(pending->getConnectionParameters().mIsArranged)
                  sendArrangedConnectRequest(pending);
               else
                  sendConnectRequest(pending);
            }
         }
         else if(pending->getConnectionState() == NetConnection::SendingPunchPackets &&
            getCurrentTime() - pending->mConnectLastSendTime > PunchRetryTime)
         {
            if(pending->mConnectSendCount > PunchRetryCount)
            {
               pending->setConnectionState(NetConnection::ConnectTimedOut);
               pending->onConnectTerminated(NetConnection::ReasonTimedOut, "Timeout");
               removePendingConnection(pending);
               continue;
            }
            else
               sendPunchPackets(pending);
         }
         else if(pending->getConnectionState() == NetConnection::ComputingPuzzleSolution &&
            getCurrentTime() - pending->mConnectLastSendTime > PuzzleSolutionTimeout)
         {
            pending->setConnectionState(NetConnection::ConnectTimedOut);
            pending->onConnectTerminated(NetConnection::ReasonTimedOut, "Timeout");
            removePendingConnection(pending);
         }
         i++;
      }
      mLastTimeoutCheckTime = getCurrentTime();

      for(S32 i = 0; i < mConnectionList.size();)
      {
         if(mConnectionList[i]->checkTimeout(getCurrentTime()))
         {
            mConnectionList[i]->setConnectionState(NetConnection::TimedOut);
            mConnectionList[i]->onConnectionTerminated(NetConnection::ReasonTimedOut, "Timeout");
            removeConnection(mConnectionList[i]);
         }
         else
            i++;
      }
   }

   // check if we're trying to solve any client connection puzzles
   for(S32 i = 0; i < mPendingConnections.size(); i++)
   {
      if(mPendingConnections[i]->getConnectionState() == NetConnection::ComputingPuzzleSolution)
      {
         continuePuzzleSolution(mPendingConnections[i]);
         break;
      }
   }
}

//-----------------------------------------------------------------------------
// NetInterface incoming packet dispatch
//-----------------------------------------------------------------------------

void NetInterface::checkIncomingPackets()
{
   PacketStream stream;
   NetError error;
   Address sourceAddress;

   mCurrentTime = Platform::getRealMilliseconds();

   // read out all the available packets:
   while((error = stream.recvfrom(mSocket, &sourceAddress)) == NoError)
      processPacket(sourceAddress, &stream);
}

void NetInterface::processPacket(const Address &sourceAddress, BitStream *pStream)
{
   // Determine what to do with this packet:

   if(pStream->getBuffer()[0] & 0x80) // it's a protocol packet...
   {
      // if the LSB of the first byte is set, it's a game data packet
      // so pass it to the appropriate connection.

      // lookup the connection in the addressTable
      // if this packet causes a disconnection, keep the conn around until this function exits
      RefPtr<NetConnection> conn = findConnection(sourceAddress);
      if(conn)
      {
         if(conn->mSimulatedReceiveLatency)
         {
            pStream->setBitPosition(pStream->getMaxReadBitPosition() + 1);
            sendtoDelayed(NULL, conn, pStream, conn->mSimulatedReceiveLatency);
         }
         else
            conn->readRawPacket(pStream);
      }
   }
   else
   {
      // Otherwise, it's either a game info packet or a
      // connection handshake packet.

      U8 packetType;
      pStream->read(&packetType);

      if(packetType >= FirstValidInfoPacketId)
         handleInfoPacket(sourceAddress, packetType, pStream);
      else
      {
         // Check if there's a connection already:
         switch(packetType)
         {
            case ConnectChallengeRequest:
               handleConnectChallengeRequest(sourceAddress, pStream);
               break;
            case ConnectChallengeResponse:
               handleConnectChallengeResponse(sourceAddress, pStream);
               break;
            case ConnectRequest:
               handleConnectRequest(sourceAddress, pStream);
               break;
            case ConnectReject:
               handleConnectReject(sourceAddress, pStream);
               break;
            case ConnectAccept:
               handleConnectAccept(sourceAddress, pStream);
               break;
            case Disconnect:
               handleDisconnect(sourceAddress, pStream);
               break;
            case Punch:
               handlePunch(sourceAddress, pStream);
               break;
            case ArrangedConnectRequest:
               handleArrangedConnectRequest(sourceAddress, pStream);
               break;
         }
      }
   }
}

void NetInterface::handleInfoPacket(const Address &address, U8 packetType, BitStream *stream)
{
}

//-----------------------------------------------------------------------------
// NetInterface connection handshake initiaton and processing
//-----------------------------------------------------------------------------

void NetInterface::startConnection(NetConnection *conn)
{
   TNLAssert(conn->getConnectionState() == NetConnection::NotConnected, "Cannot start unless it is in the NotConnected state.");

   addPendingConnection(conn);
   conn->mConnectSendCount = 0;
   conn->setConnectionState(NetConnection::AwaitingChallengeResponse);
   sendConnectChallengeRequest(conn);
}

void NetInterface::sendConnectChallengeRequest(NetConnection *conn)
{
   logprintf(LogConsumer::LogNetInterface, "Sending Connect Challenge Request to %s", conn->getNetAddress().toString());
   PacketStream out;
   out.write(U8(ConnectChallengeRequest));
   ConnectionParameters &params = conn->getConnectionParameters();
   params.mNonce.write(&out);
   out.writeFlag(params.mRequestKeyExchange);
   out.writeFlag(params.mRequestCertificate);

   conn->mConnectSendCount++;
   conn->mConnectLastSendTime = getCurrentTime();
   out.sendto(mSocket, conn->getNetAddress());
}

void NetInterface::handleConnectChallengeRequest(const Address &addr, BitStream *stream)
{
   logprintf(LogConsumer::LogNetInterface, "Received Connect Challenge Request from %s", addr.toString());

   if(!mAllowConnections)
      return;

   Nonce clientNonce;
   clientNonce.read(stream);
   bool wantsKeyExchange = stream->readFlag();
   bool wantsCertificate = stream->readFlag();

   sendConnectChallengeResponse(addr, clientNonce, wantsKeyExchange, wantsCertificate);
}

void NetInterface::sendConnectChallengeResponse(const Address &addr, Nonce &clientNonce, bool wantsKeyExchange, bool wantsCertificate)
{
   PacketStream out;
   out.write(U8(ConnectChallengeResponse));
   clientNonce.write(&out);

   U32 identityToken = computeClientIdentityToken(addr, clientNonce);
   out.write(identityToken);

   // write out a client puzzle
   Nonce serverNonce = mPuzzleManager.getCurrentNonce();
   U32 difficulty = mPuzzleManager.getCurrentDifficulty();
   serverNonce.write(&out);
   out.write(difficulty);

   if(out.writeFlag(mRequiresKeyExchange || (wantsKeyExchange && !mPrivateKey.isNull())))
   {
      if(out.writeFlag(wantsCertificate && !mCertificate.isNull()))
         out.write(mCertificate);
      else
         out.write(mPrivateKey->getPublicKey());
   }
   logprintf(LogConsumer::LogNetInterface, "Sending Challenge Response: %8x", identityToken);

   out.sendto(mSocket, addr);
}

//-----------------------------------------------------------------------------

void NetInterface::handleConnectChallengeResponse(const Address &address, BitStream *stream)
{
   NetConnection *conn = findPendingConnection(address);
   if(!conn || conn->getConnectionState() != NetConnection::AwaitingChallengeResponse)    // Not expecting a connection, abort!
      return;

   Nonce theNonce;
   theNonce.read(stream);

   ConnectionParameters &theParams = conn->getConnectionParameters();

   if(theNonce != theParams.mNonce)    // Nonces don't match, abort!
      return;

   stream->read(&theParams.mClientIdentity);

   // See if the server wants us to solve a client puzzle
   theParams.mServerNonce.read(stream);
   stream->read(&theParams.mPuzzleDifficulty);

   if(theParams.mPuzzleDifficulty > ClientPuzzleManager::MaxPuzzleDifficulty)    // Puzzle too hard, abort!
      return;

   // See if the connection needs to be authenticated or uses key exchange
   if(stream->readFlag())
   {
      if(stream->readFlag())
      {
         theParams.mCertificate = new Certificate(stream);
         if(!theParams.mCertificate->isValid() || !conn->validateCertficate(theParams.mCertificate, true))  // Invalid cert, abort!
            return;

         theParams.mPublicKey = theParams.mCertificate->getPublicKey();
      }
      else
      {
         theParams.mPublicKey = new AsymmetricKey(stream);
         if(!theParams.mPublicKey->isValid() || !conn->validatePublicKey(theParams.mPublicKey, true))
            return;
      }
      if(mPrivateKey.isNull() || mPrivateKey->getKeySize() != theParams.mPublicKey->getKeySize())
      {
         // we don't have a private key, so generate one for this connection
         theParams.mPrivateKey = new AsymmetricKey(theParams.mPublicKey->getKeySize());
      }
      else
         theParams.mPrivateKey = mPrivateKey;
      theParams.mSharedSecret = theParams.mPrivateKey->computeSharedSecretKey(theParams.mPublicKey);
      //logprintf("shared secret (client) %s", theParams.mSharedSecret->encodeBase64()->getBuffer());
      Random::read(theParams.mSymmetricKey, SymmetricCipher::KeySize);
      theParams.mUsingCrypto = true;
   }

   logprintf(LogConsumer::LogNetInterface, "Received Challenge Response: %8x", theParams.mClientIdentity);

   conn->setConnectionState(NetConnection::ComputingPuzzleSolution);
   conn->mConnectSendCount = 0;

   theParams.mPuzzleSolution = 0;
   conn->mConnectLastSendTime = getCurrentTime();
   continuePuzzleSolution(conn);
}

void NetInterface::continuePuzzleSolution(NetConnection *conn)
{
   ConnectionParameters &theParams = conn->getConnectionParameters();
   bool solved = ClientPuzzleManager::solvePuzzle(&theParams.mPuzzleSolution, theParams.mNonce, theParams.mServerNonce,
                                                  theParams.mPuzzleDifficulty, theParams.mClientIdentity);

   if(solved)
   {
      logprintf(LogConsumer::LogNetInterface, "Client puzzle solved in %d ms.", 
                                                         Platform::getRealMilliseconds() - conn->mConnectLastSendTime);

      conn->setConnectionState(NetConnection::AwaitingConnectResponse);
      sendConnectRequest(conn);
   }
}

//-----------------------------------------------------------------------------
// NetInterface connect request
//-----------------------------------------------------------------------------

void NetInterface::sendConnectRequest(NetConnection *conn)
{
   PacketStream out;
   ConnectionParameters &theParams = conn->getConnectionParameters();

   const char *destDescr;
   if(theParams.mIsLocal)
      destDescr = "local in-process server";
   else
      destDescr = conn->getNetAddress().toString();

   logprintf(LogConsumer::LogNetInterface, "Sending connect request to %s", destDescr);

   out.write(U8(ConnectRequest));
   theParams.mNonce.write(&out);
   theParams.mServerNonce.write(&out);
   out.write(theParams.mClientIdentity);
   out.write(theParams.mPuzzleDifficulty);
   out.write(theParams.mPuzzleSolution);

   U32 encryptPos = 0;

   if(out.writeFlag(theParams.mUsingCrypto))
   {
      out.write(theParams.mPrivateKey->getPublicKey());
      encryptPos = out.getBytePosition();
      out.setBytePosition(encryptPos);
      out.write(SymmetricCipher::KeySize, theParams.mSymmetricKey);
   }
   out.writeFlag(theParams.mDebugObjectSizes);
   out.write(conn->getInitialSendSequence());
   out.writeString(conn->getClassName());
   conn->writeConnectRequest(&out);

   if(encryptPos)
   {
      // if we're using crypto on this connection,
      // then write a hash of everything we wrote into the packet
      // key.  Then we'll symmetrically encrypt the packet from
      // the end of the public key to the end of the signature.

      SymmetricCipher theCipher(theParams.mSharedSecret);
      out.hashAndEncrypt(NetConnection::MessageSignatureBytes, encryptPos, &theCipher);
   }

   conn->mConnectSendCount++;
   conn->mConnectLastSendTime = getCurrentTime();

   out.sendto(mSocket, conn->getNetAddress());
}

void NetInterface::handleConnectRequest(const Address &address, BitStream *stream)
{
   if(!mAllowConnections)
      return;

   ConnectionParameters theParams;
   theParams.mNonce.read(stream);
   theParams.mServerNonce.read(stream);
   stream->read(&theParams.mClientIdentity);

   if(theParams.mClientIdentity != computeClientIdentityToken(address, theParams.mNonce))
      return;

   stream->read(&theParams.mPuzzleDifficulty);
   stream->read(&theParams.mPuzzleSolution);

   // See if the connection is in the main connection table.
   // If the connection is in the connection table and it has
   // the same initiatorSequence, we'll just resend the connect
   // acceptance packet, assuming that the last time we sent it
   // it was dropped.
   NetConnection *connect = findConnection(address);
   if(connect)
   {
      ConnectionParameters &cp = connect->getConnectionParameters();
      if(cp.mNonce == theParams.mNonce && cp.mServerNonce == theParams.mServerNonce)
      {
         sendConnectAccept(connect);
         return;
      }
   }

   // Check the puzzle solution
   ClientPuzzleManager::ErrorCode result = mPuzzleManager.checkSolution(
      theParams.mPuzzleSolution, theParams.mNonce, theParams.mServerNonce,
      theParams.mPuzzleDifficulty, theParams.mClientIdentity);

   if(result != ClientPuzzleManager::Success)      // Wrong answer!
   {
      sendConnectReject(&theParams, address, NetConnection::ReasonPuzzle);
      return;
   }

   if(stream->readFlag())
   {
      if(mPrivateKey.isNull())
         return;

      theParams.mUsingCrypto = true;
      theParams.mPublicKey = new AsymmetricKey(stream);
      theParams.mPrivateKey = mPrivateKey;

      U32 decryptPos = stream->getBytePosition();

      stream->setBytePosition(decryptPos);
      theParams.mSharedSecret = theParams.mPrivateKey->computeSharedSecretKey(theParams.mPublicKey);

      SymmetricCipher theCipher(theParams.mSharedSecret);

      if(!stream->decryptAndCheckHash(NetConnection::MessageSignatureBytes, decryptPos, &theCipher))
         return;

      // Read the first part of the connection's symmetric key
      stream->read(SymmetricCipher::KeySize, theParams.mSymmetricKey);
      Random::read(theParams.mInitVector, SymmetricCipher::KeySize);
   }

   U32 connectSequence;
   theParams.mDebugObjectSizes = stream->readFlag();
   stream->read(&connectSequence);
   logprintf(LogConsumer::LogNetInterface, "Received Connect Request %8x", theParams.mClientIdentity);

   if(connect)
      disconnect(connect, NetConnection::ReasonSelfDisconnect, "NewConnection");

   char connectionClass[256];
   stream->readString(connectionClass);

   NetConnection *conn = NetConnectionRep::create(connectionClass);

   if(!conn)
      return;

   RefPtr<NetConnection> theConnection = conn;
   conn->getConnectionParameters() = theParams;

   conn->setNetAddress(address);
   conn->setInitialRecvSequence(connectSequence);
   conn->setInterface(this);

   if(theParams.mUsingCrypto)
      conn->setSymmetricCipher(new SymmetricCipher(theParams.mSymmetricKey, theParams.mInitVector));

   NetConnection::TerminationReason reason;
   if(!conn->readConnectRequest(stream, reason))
   {
      sendConnectReject(&theParams, address, reason);
      return;
   }
   addConnection(conn);
   conn->setConnectionState(NetConnection::Connected);
   conn->onConnectionEstablished();
   sendConnectAccept(conn);
}

//-----------------------------------------------------------------------------
// NetInterface connection acceptance and handling
//-----------------------------------------------------------------------------

void NetInterface::sendConnectAccept(NetConnection *conn)
{
   logprintf(LogConsumer::LogNetInterface, "Sending Connect Accept - connection established.");

   PacketStream out;
   out.write(U8(ConnectAccept));
   ConnectionParameters &theParams = conn->getConnectionParameters();

   theParams.mNonce.write(&out);
   theParams.mServerNonce.write(&out);
   U32 encryptPos = out.getBytePosition();
   out.setBytePosition(encryptPos);

   out.write(conn->getInitialSendSequence());
   conn->writeConnectAccept(&out);

   if(theParams.mUsingCrypto)
   {
      out.write(SymmetricCipher::KeySize, theParams.mInitVector);
      SymmetricCipher theCipher(theParams.mSharedSecret);
      out.hashAndEncrypt(NetConnection::MessageSignatureBytes, encryptPos, &theCipher);
   }

   out.sendto(mSocket, conn->getNetAddress());
}

void NetInterface::handleConnectAccept(const Address &address, BitStream *stream)
{
   Nonce nonce, serverNonce;

   nonce.read(stream);
   serverNonce.read(stream);

   U32 decryptPos = stream->getBytePosition();
   stream->setBytePosition(decryptPos);

   // Make sure we're actually waiting for a connection.  If not, then there's something wrong, and we bail.
   NetConnection *conn = findPendingConnection(address);
   if(!conn || conn->getConnectionState() != NetConnection::AwaitingConnectResponse)
      return;

   ConnectionParameters &theParams = conn->getConnectionParameters();

   if(theParams.mNonce != nonce || theParams.mServerNonce != serverNonce)
      return;

   if(theParams.mUsingCrypto)
   {
      SymmetricCipher theCipher(theParams.mSharedSecret);
      if(!stream->decryptAndCheckHash(NetConnection::MessageSignatureBytes, decryptPos, &theCipher))
         return;
   }
   U32 recvSequence;
   stream->read(&recvSequence);
   conn->setInitialRecvSequence(recvSequence);

   NetConnection::TerminationReason reason;
   if(!conn->readConnectAccept(stream, reason))
   {
      removePendingConnection(conn);
      return;
   }

   if(theParams.mUsingCrypto)
   {
      stream->read(SymmetricCipher::KeySize, theParams.mInitVector);
      conn->setSymmetricCipher(new SymmetricCipher(theParams.mSymmetricKey, theParams.mInitVector));
   }

   addConnection(conn);           // First, add it as a regular connection,
   removePendingConnection(conn); // then remove it from the pending connection list

   conn->setConnectionState(NetConnection::Connected);
   conn->onConnectionEstablished(); // notify the connection that it has been established
   logprintf(LogConsumer::LogNetInterface, "Received Connect Accept - connection established.");
}

//-----------------------------------------------------------------------------
// NetInterface connection rejection and handling
//-----------------------------------------------------------------------------

void NetInterface::sendConnectReject(ConnectionParameters *conn, const Address &theAddress, NetConnection::TerminationReason reason)
{
   //if(!reason)
   //   return; // if the stream is NULL, we reject silently

   PacketStream out;
   out.write(U8(ConnectReject));
   conn->mNonce.write(&out);
   conn->mServerNonce.write(&out);
   out.writeEnum(reason, NetConnection::TerminationReasons);
   out.writeString("");
   out.sendto(mSocket, theAddress);
}

void NetInterface::handleConnectReject(const Address &address, BitStream *stream)
{
   Nonce nonce;
   Nonce serverNonce;

   nonce.read(stream);
   serverNonce.read(stream);

   NetConnection *conn = findPendingConnection(address);
   if(!conn || (conn->getConnectionState() != NetConnection::AwaitingChallengeResponse &&
                conn->getConnectionState() != NetConnection::AwaitingConnectResponse))
      return;
   ConnectionParameters &p = conn->getConnectionParameters();
   if(p.mNonce != nonce || p.mServerNonce != serverNonce)
      return;


   NetConnection::TerminationReason reason = (NetConnection::TerminationReason) stream->readEnum(NetConnection::TerminationReasons);

   char reasonStr[256];
   stream->readString(reasonStr);

   logprintf(LogConsumer::LogNetInterface, "Received Connect Reject - reason code %d (%s)", reason, reasonStr);

   // If the reason is a bad puzzle solution, try once more with a new nonce
   if(reason == NetConnection::ReasonPuzzle && !p.mPuzzleRetried)
   {
      p.mPuzzleRetried = true;
      conn->setConnectionState(NetConnection::AwaitingChallengeResponse);
      conn->mConnectSendCount = 0;
      p.mNonce.getRandom();                  // Generate new nonce
      sendConnectChallengeRequest(conn);
      return;
   }

   conn->setConnectionState(NetConnection::ConnectRejected);
   conn->onConnectTerminated(reason, reasonStr);
   removePendingConnection(conn);
}

//-----------------------------------------------------------------------------
// NetInterface arranged connection process
//-----------------------------------------------------------------------------

void NetInterface::startArrangedConnection(NetConnection *conn)
{
   conn->setConnectionState(NetConnection::SendingPunchPackets);
   addPendingConnection(conn);
   conn->mConnectSendCount = 0;
   conn->mConnectLastSendTime = getCurrentTime();
   sendPunchPackets(conn);
}

void NetInterface::sendPunchPackets(NetConnection *conn)
{
   ConnectionParameters &theParams = conn->getConnectionParameters();
   PacketStream out;
   out.write(U8(Punch));

   if(theParams.mIsInitiator)
      theParams.mNonce.write(&out);
   else
      theParams.mServerNonce.write(&out);

   U32 encryptPos = out.getBytePosition();
   out.setBytePosition(encryptPos);

   if(theParams.mIsInitiator)
      theParams.mServerNonce.write(&out);
   else
   {
      theParams.mNonce.write(&out);
      if(out.writeFlag(mRequiresKeyExchange || (theParams.mRequestKeyExchange && !mPrivateKey.isNull())))
      {
         if(out.writeFlag(theParams.mRequestCertificate && !mCertificate.isNull()))
            out.write(mCertificate);
         else
            out.write(mPrivateKey->getPublicKey());
      }
   }
   SymmetricCipher theCipher(theParams.mArrangedSecret);
   out.hashAndEncrypt(NetConnection::MessageSignatureBytes, encryptPos, &theCipher);

   for(S32 i = 0; i < theParams.mPossibleAddresses.size(); i++)
   {
      out.sendto(mSocket, theParams.mPossibleAddresses[i]);

      logprintf(LogConsumer::LogNetInterface, "Sending punch packet (%s, %s) to %s",
         ByteBuffer(theParams.mNonce.data, Nonce::NonceSize).encodeBase64()->getBuffer(),
         ByteBuffer(theParams.mServerNonce.data, Nonce::NonceSize).encodeBase64()->getBuffer(),
         theParams.mPossibleAddresses[i].toString());
   }
   conn->mConnectSendCount++;
   conn->mConnectLastSendTime = getCurrentTime();
}

void NetInterface::handlePunch(const Address &theAddress, BitStream *stream)
{
   S32 i, j;
   NetConnection *conn = NULL;

   Nonce firstNonce;
   firstNonce.read(stream);

   ByteBuffer b(firstNonce.data, Nonce::NonceSize);

   logprintf(LogConsumer::LogNetInterface, "Received punch packet from %s - %s", theAddress.toString(), b.encodeBase64()->getBuffer());

   for(i = 0; i < mPendingConnections.size(); i++)
   {
      conn = mPendingConnections[i];
      ConnectionParameters &theParams = conn->getConnectionParameters();

      if(conn->getConnectionState() != NetConnection::SendingPunchPackets)
         continue;

      if((theParams.mIsInitiator && firstNonce != theParams.mServerNonce) ||
            (!theParams.mIsInitiator && firstNonce != theParams.mNonce))
         continue;

      // first see if the address is in the possible addresses list:

      for(j = 0; j < theParams.mPossibleAddresses.size(); j++)
         if(theAddress == theParams.mPossibleAddresses[j])
            break;

      // if there was an exact match, just exit the loop, or
      // continue on to the next pending if this is not an initiator:
      if(j != theParams.mPossibleAddresses.size())
      {
         if(theParams.mIsInitiator)
            break;
         else
            continue;
      }

      // if there was no exact match, we may have a funny NAT in the
      // middle.  But since a packet got through from the remote host
      // we'll want to send a punch to the address it came from, as long
      // as only the port is not an exact match:
      for(j = 0; j < theParams.mPossibleAddresses.size(); j++)
         if(theAddress.isEqualAddress(theParams.mPossibleAddresses[j]))
            break;

      // if the address wasn't even partially in the list, just exit out
      if(j == theParams.mPossibleAddresses.size())
         continue;

      // otherwise, as long as we don't have too many ping addresses,
      // add this one to the list:
      if(theParams.mPossibleAddresses.size() < 5)
         theParams.mPossibleAddresses.push_back(theAddress);

      // if this is the initiator of the arranged connection, then
      // process the punch packet from the remote host by issuing a
      // connection request.
      if(theParams.mIsInitiator)
         break;
   }
   if(i == mPendingConnections.size())
      return;

   ConnectionParameters &theParams = conn->getConnectionParameters();
   SymmetricCipher theCipher(theParams.mArrangedSecret);

   if(!stream->decryptAndCheckHash(NetConnection::MessageSignatureBytes, stream->getBytePosition(), &theCipher))
      return;

   Nonce nextNonce;
   nextNonce.read(stream);

   if(nextNonce != theParams.mNonce)
      return;

   // see if the connection needs to be authenticated or uses key exchange
   if(stream->readFlag())
   {
      if(stream->readFlag())
      {
         theParams.mCertificate = new Certificate(stream);
         if(!theParams.mCertificate->isValid() || !conn->validateCertficate(theParams.mCertificate, true))
            return;

         theParams.mPublicKey = theParams.mCertificate->getPublicKey();
      }
      else
      {
         theParams.mPublicKey = new AsymmetricKey(stream);
         if(!theParams.mPublicKey->isValid() || !conn->validatePublicKey(theParams.mPublicKey, true))
            return;
      }
      if(mPrivateKey.isNull() || mPrivateKey->getKeySize() != theParams.mPublicKey->getKeySize())
      {
         // we don't have a private key, so generate one for this connection
         theParams.mPrivateKey = new AsymmetricKey(theParams.mPublicKey->getKeySize());
      }
      else
         theParams.mPrivateKey = mPrivateKey;
      theParams.mSharedSecret = theParams.mPrivateKey->computeSharedSecretKey(theParams.mPublicKey);

      Random::read(theParams.mSymmetricKey, SymmetricCipher::KeySize);
      theParams.mUsingCrypto = true;
   }
   conn->setNetAddress(theAddress);
   logprintf(LogConsumer::LogNetInterface, "Punch from %s matched nonces - connecting...", theAddress.toString());

   conn->setConnectionState(NetConnection::AwaitingConnectResponse);
   conn->mConnectSendCount = 0;
   conn->mConnectLastSendTime = getCurrentTime();

   sendArrangedConnectRequest(conn);
}

// client
void NetInterface::sendArrangedConnectRequest(NetConnection *conn)
{
   logprintf(LogConsumer::LogNetInterface, "Sending Arranged Connect Request");
   PacketStream out;

   ConnectionParameters &theParams = conn->getConnectionParameters();

   out.write(U8(ArrangedConnectRequest));
   theParams.mNonce.write(&out);
   U32 encryptPos = out.getBytePosition();
   U32 innerEncryptPos = 0;

   out.setBytePosition(encryptPos);

   theParams.mServerNonce.write(&out);
   if(out.writeFlag(theParams.mUsingCrypto))
   {
      out.write(theParams.mPrivateKey->getPublicKey());
      innerEncryptPos = out.getBytePosition();
      out.setBytePosition(innerEncryptPos);
      out.write(SymmetricCipher::KeySize, theParams.mSymmetricKey);
   }
   out.writeFlag(theParams.mDebugObjectSizes);
   out.write(conn->getInitialSendSequence());
   conn->writeConnectRequest(&out);

   if(innerEncryptPos)
   {
      SymmetricCipher theCipher(theParams.mSharedSecret);
      out.hashAndEncrypt(NetConnection::MessageSignatureBytes, innerEncryptPos, &theCipher);
   }
   SymmetricCipher theCipher(theParams.mArrangedSecret);
   out.hashAndEncrypt(NetConnection::MessageSignatureBytes, encryptPos, &theCipher);

   conn->mConnectSendCount++;
   conn->mConnectLastSendTime = getCurrentTime();

   out.sendto(mSocket, conn->getNetAddress());
}

// server
void NetInterface::handleArrangedConnectRequest(const Address &theAddress, BitStream *stream)
{
   S32 i, j;
   NetConnection *conn = NULL;
   Nonce nonce, serverNonce;
   nonce.read(stream);

   // see if the connection is in the main connection table.
   // If the connection is in the connection table and it has
   // the same initiatorSequence, we'll just resend the connect
   // acceptance packet, assuming that the last time we sent it
   // it was dropped.
   NetConnection *oldConnection = findConnection(theAddress);
   if(oldConnection)
   {
      ConnectionParameters &cp = oldConnection->getConnectionParameters();
      if(cp.mNonce == nonce)
      {
         sendConnectAccept(oldConnection);
         return;
      }
   }

   for(i = 0; i < mPendingConnections.size(); i++)
   {
      conn = mPendingConnections[i];
      ConnectionParameters &theParams = conn->getConnectionParameters();

      if(conn->getConnectionState() != NetConnection::SendingPunchPackets || theParams.mIsInitiator)
         continue;

      if(nonce != theParams.mNonce)
         continue;

      for(j = 0; j < theParams.mPossibleAddresses.size(); j++)
         if(theAddress.isEqualAddress(theParams.mPossibleAddresses[j]))
            break;
      if(j != theParams.mPossibleAddresses.size())
         break;
   }
   if(i == mPendingConnections.size())
      return;

   ConnectionParameters &theParams = conn->getConnectionParameters();
   SymmetricCipher theCipher(theParams.mArrangedSecret);
   if(!stream->decryptAndCheckHash(NetConnection::MessageSignatureBytes, stream->getBytePosition(), &theCipher))
      return;

   stream->setBytePosition(stream->getBytePosition());

   serverNonce.read(stream);
   if(serverNonce != theParams.mServerNonce)
      return;

   if(stream->readFlag())
   {
      if(mPrivateKey.isNull())
         return;
      theParams.mUsingCrypto = true;
      theParams.mPublicKey = new AsymmetricKey(stream);
      theParams.mPrivateKey = mPrivateKey;

      U32 decryptPos = stream->getBytePosition();
      stream->setBytePosition(decryptPos);
      theParams.mSharedSecret = theParams.mPrivateKey->computeSharedSecretKey(theParams.mPublicKey);
      SymmetricCipher theCipher(theParams.mSharedSecret);

      if(!stream->decryptAndCheckHash(NetConnection::MessageSignatureBytes, decryptPos, &theCipher))
         return;

      // now read the first part of the connection's session (symmetric) key
      stream->read(SymmetricCipher::KeySize, theParams.mSymmetricKey);
      Random::read(theParams.mInitVector, SymmetricCipher::KeySize);
   }

   U32 connectSequence;
   theParams.mDebugObjectSizes = stream->readFlag();
   stream->read(&connectSequence);
   logprintf(LogConsumer::LogNetInterface, "Received Arranged Connect Request");

   if(oldConnection)
      disconnect(oldConnection, NetConnection::ReasonSelfDisconnect, "");

   conn->setNetAddress(theAddress);
   conn->setInitialRecvSequence(connectSequence);
   if(theParams.mUsingCrypto)
      conn->setSymmetricCipher(new SymmetricCipher(theParams.mSymmetricKey, theParams.mInitVector));

   NetConnection::TerminationReason reason;
   if(!conn->readConnectRequest(stream, reason))
   {
      sendConnectReject(&theParams, theAddress, reason);
      removePendingConnection(conn);
      return;
   }
   addConnection(conn);
   removePendingConnection(conn);
   conn->setConnectionState(NetConnection::Connected);
   conn->onConnectionEstablished();
   sendConnectAccept(conn);
}

//-----------------------------------------------------------------------------
// NetInterface disconnection and handling
//-----------------------------------------------------------------------------

void NetInterface::disconnect(NetConnection *conn, NetConnection::TerminationReason reason, const char *reasonString)
{
   // If we're still connecting...
   if(conn->getConnectionState() == NetConnection::NotConnected ||
      conn->getConnectionState() == NetConnection::AwaitingChallengeResponse ||
      conn->getConnectionState() == NetConnection::AwaitingConnectResponse ||
      conn->getConnectionState() == NetConnection::SendingPunchPackets  ||
      conn->getConnectionState() == NetConnection::ComputingPuzzleSolution)
   {
      conn->onConnectTerminated(reason, reasonString);
      removePendingConnection(conn);
   }

   // If we're already connected
   else if(conn->getConnectionState() == NetConnection::Connected)
   {
      conn->setConnectionState(NetConnection::Disconnected);
      conn->onConnectionTerminated(reason, reasonString);
      if(conn->getRemoteConnectionObject()) // Let other side of local connection know this connection have ended.
      {
         conn->getRemoteConnectionObject()->setRemoteConnectionObject(NULL);
         conn->getRemoteConnectionObject()->disconnect(reason, reasonString);
         conn->setRemoteConnectionObject(NULL);
      }
      if(conn->isNetworkConnection())
      {
         // send a disconnect packet...
         PacketStream out;
         out.write(U8(Disconnect));
         ConnectionParameters &theParams = conn->getConnectionParameters();
         theParams.mNonce.write(&out);
         theParams.mServerNonce.write(&out);
         U32 encryptPos = out.getBytePosition();
         out.setBytePosition(encryptPos);
         out.writeEnum(reason, NetConnection::TerminationReasons);
         out.writeString(reasonString);

         if(theParams.mUsingCrypto)
         {
            SymmetricCipher theCipher(theParams.mSharedSecret);
            out.hashAndEncrypt(NetConnection::MessageSignatureBytes, encryptPos, &theCipher);
         }
         out.sendto(mSocket, conn->getNetAddress());
      }
      removeConnection(conn);
   }
}

void NetInterface::handleDisconnect(const Address &address, BitStream *stream)
{
   NetConnection *conn = findConnection(address);
   if(!conn)
      return;

   ConnectionParameters &theParams = conn->getConnectionParameters();

   Nonce nonce, serverNonce;
   NetConnection::TerminationReason reason;
   char reasonStr[256];

   nonce.read(stream);
   serverNonce.read(stream);

   if(nonce != theParams.mNonce || serverNonce != theParams.mServerNonce)
      return;

   U32 decryptPos = stream->getBytePosition();
   stream->setBytePosition(decryptPos);

   if(theParams.mUsingCrypto)
   {
      SymmetricCipher theCipher(theParams.mSharedSecret);
      if(!stream->decryptAndCheckHash(NetConnection::MessageSignatureBytes, decryptPos, &theCipher))
         return;
   }

   reason = (NetConnection::TerminationReason) stream->readEnum(NetConnection::TerminationReasons);
   stream->readString(reasonStr);

   conn->setConnectionState(NetConnection::Disconnected);
   conn->onConnectionTerminated(reason, reasonStr);
   removeConnection(conn);
}

void NetInterface::handleConnectionError(NetConnection *theConnection, const char *errorString)
{
   disconnect(theConnection, NetConnection::ReasonError, errorString);
}

};
