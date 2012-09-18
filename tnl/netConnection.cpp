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
#include "tnlNetBase.h"
#include "tnlNetConnection.h"
#include "tnlNetInterface.h"
#include "tnlLog.h"
#include "tnlRandom.h"
#include "tnlSymmetricCipher.h"
#include "tnlAsymmetricKey.h"
#include "tnlConnectionStringTable.h"

#include <stdarg.h>


namespace TNL {

NetConnectionRep *NetConnectionRep::mLinkedList = NULL;

NetConnection *NetConnectionRep::create(const char *name)
{
   for(NetConnectionRep *walk = mLinkedList; walk; walk = walk->mNext)
      if(walk->mCanRemoteCreate && !strcmp(name, walk->mClassRep->getClassName()))
      {
         Object *obj = walk->mClassRep->create();
         NetConnection *ret = dynamic_cast<NetConnection *>(obj);
         TNLAssert(ret != NULL, "Invalid TNL_IMPLEMENT_NETCONNECTION");
         if(!ret)
            delete obj;
         return ret;
      }

   return NULL;
}

static const char *packetTypeNames[] = 
{
   "DataPacket",
   "PingPacket",
   "AckPacket",
};

//-----------------------------------------------------------------

NetConnection::NetConnection()
{
   mInitialSendSeq = Random::readI();
   mConnectionParameters.mNonce.getRandom();

   mSimulatedSendLatency = 0;
   mSimulatedReceiveLatency = 0;
   mSimulatedSendPacketLoss = 0;
   mSimulatedReceivePacketLoss = 0;

   mLastPacketRecvTime = 0;
   mLastUpdateTime = 0;
   mRoundTripTime = 0;
   mSendDelayCredit = 0;
   mConnectionState = NotConnected;
   
   mNotifyQueueHead = NULL;
   mNotifyQueueTail = NULL;
   
   mLocalRate.maxRecvBandwidth = DefaultFixedBandwidth;
   mLocalRate.maxSendBandwidth = DefaultFixedBandwidth;
   mLocalRate.minPacketRecvPeriod = DefaultFixedSendPeriod;
   mLocalRate.minPacketSendPeriod = DefaultFixedSendPeriod;

   mRemoteRate = mLocalRate;
   mLocalRateChanged = true;
   computeNegotiatedRate();

   mPingSendCount = 0;
   mLastPingSendTime = 0;

   mLastSeqRecvd = 0;

   mHighestAckedSeq = mInitialSendSeq;
   mLastSendSeq = mInitialSendSeq; // start sending at mInitialSendSeq + 1
   mAckMask[0] = 0;
   mLastRecvAckAck = 0;

   // Adaptive
   cwnd = 2;
   ssthresh = 30;
   mLastSeqRecvdAck = 0;
   mLastAckTime = 0;

   mPingTimeout = DefaultPingTimeout;
   mPingRetryCount = DefaultPingRetryCount;
   mStringTable = NULL;
}

void NetConnection::setInitialRecvSequence(U32 sequence)
{ 
   mInitialRecvSeq = mLastSeqRecvd = mLastRecvAckAck = sequence;
}

void NetConnection::clearAllPacketNotifies()
{
   while(mNotifyQueueHead)
      handleNotify(0, false);
}

NetConnection::~NetConnection()
{
   clearAllPacketNotifies();
   delete mStringTable;

   TNLAssert(mNotifyQueueHead == NULL, "Uncleared notifies remain.");
}

//--------------------------------------------------------------------
void NetConnection::setNetAddress(const Address &addr)
{
   mNetAddress = addr;
}

const Address &NetConnection::getNetAddress()
{
   return mNetAddress;
}

NetConnection::PacketNotify::PacketNotify()
{
   rateChanged = false;
   sendTime = 0;
}

bool NetConnection::checkTimeout(U32 time)
{
   if(!mLastPingSendTime)
      mLastPingSendTime = time;

   U32 timeout = mPingTimeout;
   U32 timeoutCount = mPingRetryCount;

   if(isAdaptive())
   {
      if(hasUnackedSentPackets())
      {
         timeout = AdaptiveUnackedSentPingTimeout;
      }
      else
      {
         timeoutCount = AdaptivePingRetryCount;
         if(!mPingSendCount)
            timeout = AdaptiveInitialPingTimeout;
      }
   }
   if((time - mLastPingSendTime) > timeout)
   {
      if(isNetworkConnection()) // Make local connection never time out, but send ping in case of lag / packet loss simulation
         if(mPingSendCount >= timeoutCount)
            return true;
      mLastPingSendTime = time;
      mPingSendCount++;
      sendPingPacket();
   }
   return false;
}

void NetConnection::keepAlive()
{
   mLastPingSendTime = 0;
   mPingSendCount = 0;
}

//--------------------------------------------------------------------

char NetConnection::mErrorBuffer[256];

void NetConnection::setLastError(const char *fmt, ...)
{
   va_list argptr;
   va_start(argptr, fmt);
   vsnprintf(mErrorBuffer, sizeof(mErrorBuffer), fmt, argptr);
   // setLastErrors assert in debug builds
   
   TNLAssert(0, mErrorBuffer);
   va_end(argptr);
}

//--------------------------------------------------------------------

void NetConnection::writeRawPacket(BitStream *bstream, NetPacketType packetType)
{
   writePacketHeader(bstream, packetType);
   if(packetType == DataPacket)
   {
      PacketNotify *note = allocNotify();
      if(!mNotifyQueueHead)
         mNotifyQueueHead = note;
      else
         mNotifyQueueTail->nextPacket = note;
      mNotifyQueueTail = note;
      note->nextPacket = NULL;
      note->sendTime = mInterface->getCurrentTime();

      writePacketRateInfo(bstream, note);
      S32 start = bstream->getBitPosition();
      bstream->setStringTable(mStringTable);

      logprintf(LogConsumer::LogNetConnection, "NetConnection %s: START %s", mNetAddress.toString(), getClassName());
      writePacket(bstream, note);
      logprintf(LogConsumer::LogNetConnection, "NetConnection %s: END %s - %d bits", mNetAddress.toString(), getClassName(), bstream->getBitPosition() - start);
   }
   if(!mSymmetricCipher.isNull())
   {
      mSymmetricCipher->setupCounter(mLastSendSeq, mLastSeqRecvd, packetType, 0);
      bstream->hashAndEncrypt(MessageSignatureBytes, PacketHeaderByteSize, mSymmetricCipher);
   }
}

void NetConnection::readRawPacket(BitStream *bstream)
{
   if(mSimulatedReceivePacketLoss && Random::readF() < mSimulatedReceivePacketLoss)
   {
      logprintf(LogConsumer::LogNetConnection, "NetConnection %s: RECVDROP - %d", mNetAddress.toString(), getLastSendSequence());
      return;
   }
   logprintf(LogConsumer::LogNetConnection, "NetConnection %s: RECV- %d bytes", mNetAddress.toString(), bstream->getMaxReadBitPosition() >> 3);

   mErrorBuffer[0] = 0;

   if(readPacketHeader(bstream))
   {
      mLastPacketRecvTime = mInterface->getCurrentTime();

      readPacketRateInfo(bstream);
      bstream->setStringTable(mStringTable);
      readPacket(bstream);

      if(!bstream->isValid() && !mErrorBuffer[0])
         NetConnection::setLastError("Invalid Packet -- broken bstream");

      if(mErrorBuffer[0])
         getInterface()->handleConnectionError(this, mErrorBuffer);

      mErrorBuffer[0] = 0;
   }
}

//--------------------------------------------------------------------

void NetConnection::writePacketHeader(BitStream *stream, NetPacketType packetType)
{
   if(windowFull() && packetType == DataPacket)
      TNL_DEBUGBREAK();

   S32 ackByteCount = ((mLastSeqRecvd - mLastRecvAckAck + 7) >> 3);
   TNLAssert(ackByteCount <= MaxAckByteCount, "ackByteCount exceeds MaxAckByteCount!");
   
   if(packetType == DataPacket)
      mLastSendSeq++;
      
   stream->writeInt(packetType, 2);
   stream->writeInt(mLastSendSeq, 5); // write the first 5 bits of the send sequence
   stream->writeFlag(true); // high bit of first byte indicates this is a data packet.
   stream->writeInt(mLastSendSeq >> 5, SequenceNumberBitSize - 5); // write the rest of the send sequence
   stream->writeInt(mLastSeqRecvd, AckSequenceNumberBitSize);
   stream->writeInt(0, PacketHeaderPadBits);

   stream->writeRangedU32(ackByteCount, 0, MaxAckByteCount);

   U32 wordCount = (ackByteCount + 3) >> 2;

   for(U32 i = 0; i < wordCount; i++)
      stream->writeInt(mAckMask[i], i == wordCount - 1 ?
         (ackByteCount - (i * 4)) * 8 : 32);
   
   U32 sendDelay = getTimeSinceLastPacketReceived();
   if(sendDelay > 2047)
      sendDelay = 2047;
   stream->writeInt(sendDelay >> 3, 8);

   // if we're resending this header, we can't advance the
   // sequence recieved (in case this packet drops and the prev one
   // goes through) 

   if(packetType == DataPacket)
      mLastSeqRecvdAtSend[mLastSendSeq & PacketWindowMask] = mLastSeqRecvd;

   //if(isNetworkConnection())
   //{
   //   logprintf(LogBlah, ("SND: mLSQ: %08x  pkLS: %08x  pt: %d abc: %d",
   //      mLastSendSeq, mLastSeqRecvd, packetType, ackByteCount));
   //}

   logprintf(LogConsumer::LogConnectionProtocol, "build hdr %d %d", mLastSendSeq, packetType);
}

bool NetConnection::readPacketHeader(BitStream *pstream)
{
   // read in the packet header:
   //
   //   2 bits packet type
   //   low 5 bits of the packet sequence number
   //   1 bit game packet
   //   SequenceNumberBitSize-5 bits (packet seq number >> 5)
   //   AckSequenceNumberBitSize bits ackstart seq number
   //   PacketHeaderPadBits = 0 - padding to byte boundary
   //   after this point, if this is an encrypted packet, all the rest of the data will be encrypted

   //   rangedU32 - 0...MaxAckByteCount
   //
   // type is:
   //    00 data packet
   //    01 ping packet
   //    02 ack packet
   
   // next 0...ackByteCount bytes are ack flags
   //
   // return value is true if this is a valid data packet
   // or false if there is nothing more that should be read

   U32 pkPacketType     = pstream->readInt(2);
   U32 pkSequenceNumber = pstream->readInt(5);
   bool USED_EXTERNAL pkDataPacketFlg = pstream->readFlag();
   pkSequenceNumber = pkSequenceNumber | (pstream->readInt(SequenceNumberBitSize - 5) << 5);

   U32 pkHighestAck     = pstream->readInt(AckSequenceNumberBitSize);
   U32 pkPadBits        = pstream->readInt(PacketHeaderPadBits);

   if(pkPadBits != 0)
      return false;

   TNLAssert(pkDataPacketFlg, "Invalid packet header in NetConnection::readPacketHeader!");

   // verify packet ordering and acking and stuff
   // check if the 9-bit sequence is within the packet window
   // (within 31 packets of the last received sequence number).

   pkSequenceNumber |= (mLastSeqRecvd & SequenceNumberMask);
   // account for wrap around
   if(pkSequenceNumber < mLastSeqRecvd)
      pkSequenceNumber += SequenceNumberWindowSize;
   
   // in the following test, account for wrap around from 0
   if(pkSequenceNumber - mLastSeqRecvd > (MaxPacketWindowSize - 1))
   {
      // the sequence number is outside the window... must be out of order
      // discard.
      return false;
   }

   pkHighestAck |= (mHighestAckedSeq & AckSequenceNumberMask);
   // account for wrap around
   
   if(pkHighestAck < mHighestAckedSeq)
      pkHighestAck += AckSequenceNumberWindowSize;
   
   if(pkHighestAck > mLastSendSeq)
   {
      // the ack number is outside the window... must be an out of order
      // packet, discard.
      return false;
   }
   
   if(!mSymmetricCipher.isNull())
   {
      mSymmetricCipher->setupCounter(pkSequenceNumber, pkHighestAck, pkPacketType, 0);
      if(!pstream->decryptAndCheckHash(MessageSignatureBytes, PacketHeaderByteSize, mSymmetricCipher))
      {
         logprintf(LogConsumer::LogNetConnection, "Packet failed crypto");
         return false;
      }
   }

   U32 pkAckByteCount   = pstream->readRangedU32(0, MaxAckByteCount);
   if(pkAckByteCount > MaxAckByteCount || pkPacketType >= InvalidPacketType)
      return false;
         
   U32 pkAckMask[MaxAckMaskSize];
   U32 pkAckWordCount = (pkAckByteCount + 3) >> 2;

   for(U32 i = 0; i < pkAckWordCount; i++)
      pkAckMask[i] = pstream->readInt(i == pkAckWordCount - 1 ? 
            (pkAckByteCount - (i * 4)) * 8 : 32);

   //if(isNetworkConnection())
   //{
   //   logprintf(LogBlah, "RCV: mHA: %08x  pkHA: %08x  mLSQ: %08x  pkSN: %08x  pkLS: %08x  pkAM: %08x",
   //      mHighestAckedSeq, pkHighestAck, mLastSendSeq, pkSequenceNumber, mLastSeqRecvd, pkAckMask[0]);
   //}

   U32 pkSendDelay = (pstream->readInt(8) << 3) + 4;

   for(U32 i = mLastSeqRecvd+1; i < pkSequenceNumber; i++)
      logprintf(LogConsumer::LogConnectionProtocol, "Not recv %d", i);

   logprintf(LogConsumer::LogConnectionProtocol, "Recv %d %s", pkSequenceNumber, packetTypeNames[pkPacketType]);

   // shift up the ack mask by the packet difference
   // this essentially nacks all the packets dropped

   U32 ackMaskShift = pkSequenceNumber - mLastSeqRecvd;

   // if we've missed more than a full word of packets, shift up by words
   while(ackMaskShift > 32)
   {
      for(S32 i = MaxAckMaskSize - 1; i > 0; i--)
         mAckMask[i] = mAckMask[i-1];
      mAckMask[0] = 0;
      ackMaskShift -= 32;
   }

   // the first word upshifts all NACKs, except for the low bit, which is a
   // 1 if this is a data packet (i.e. not a ping packet or an ack packet)
   U32 upShifted = (pkPacketType == DataPacket) ? 1 : 0; 

   for(U32 i = 0; i < MaxAckMaskSize; i++)
   {
      U32 nextShift = mAckMask[i] >> (32 - ackMaskShift);
      mAckMask[i] = (mAckMask[i] << ackMaskShift) | upShifted;
      upShifted = nextShift;
   }

   // do all the notifies...
   U32 notifyCount = pkHighestAck - mHighestAckedSeq;
   for(U32 i = 0; i < notifyCount; i++) 
   {
      U32 notifyIndex = mHighestAckedSeq + i + 1;

      U32 ackMaskBit = (pkHighestAck - notifyIndex) & 0x1F;
      U32 ackMaskWord = (pkHighestAck - notifyIndex) >> 5;

      bool packetTransmitSuccess = (pkAckMask[ackMaskWord] & (1 << ackMaskBit)) != 0;
      logprintf(LogConsumer::LogConnectionProtocol, "Ack %d %d", notifyIndex, packetTransmitSuccess);

      mHighestAckedSendTime = 0;
      handleNotify(notifyIndex, packetTransmitSuccess);

      // Running average of roundTrip time
      if(mHighestAckedSendTime)
      {
         S32 roundTripDelta = mInterface->getCurrentTime() - (mHighestAckedSendTime + pkSendDelay);
         mRoundTripTime = mRoundTripTime * 0.9f + roundTripDelta * 0.1f;
         if(mRoundTripTime < 0)
            mRoundTripTime = 0;
      }      
      if(packetTransmitSuccess)
         mLastRecvAckAck = mLastSeqRecvdAtSend[notifyIndex & PacketWindowMask];
   }
   // the other side knows more about its window than we do.
   if(pkSequenceNumber - mLastRecvAckAck > MaxPacketWindowSize)
      mLastRecvAckAck = pkSequenceNumber - MaxPacketWindowSize;
   
   mHighestAckedSeq = pkHighestAck;

   // first things first...
   // ackback any pings or half-full windows

   keepAlive(); // notification that the connection is ok

   U32 prevLastSequence = mLastSeqRecvd;
   mLastSeqRecvd = pkSequenceNumber;

   if(pkPacketType == PingPacket)
      //   || (pkSequenceNumber - mLastRecvAckAck > (MaxPacketWindowSize >> 1)))  this line appears to cause severe problems with really high lag
   {
      // send an ack to the other side
      // the ack will have the same packet sequence as our last sent packet
      // if the last packet we sent was the connection accepted packet
      // we must resend that packet
      sendAckPacket();
   }
   return prevLastSequence != pkSequenceNumber && pkPacketType == DataPacket;
}

//--------------------------------------------------------------------

void NetConnection::writePacketRateInfo(BitStream *bstream, PacketNotify *note)
{
   note->rateChanged = mLocalRateChanged;
   mLocalRateChanged = false;
   if(bstream->writeFlag(note->rateChanged))
   {
      if(!bstream->writeFlag(mTypeFlags.test(ConnectionAdaptive)))
      {
         bstream->writeRangedU32(mLocalRate.maxRecvBandwidth, 0, MaxFixedBandwidth);
         bstream->writeRangedU32(mLocalRate.maxSendBandwidth, 0, MaxFixedBandwidth);
         bstream->writeRangedU32(mLocalRate.minPacketRecvPeriod, 1, MaxFixedSendPeriod);
         bstream->writeRangedU32(mLocalRate.minPacketSendPeriod, 1, MaxFixedSendPeriod);
      }
   }
}

void NetConnection::readPacketRateInfo(BitStream *bstream)
{
   if(bstream->readFlag())
   {
      if(bstream->readFlag())
         mTypeFlags.set(ConnectionRemoteAdaptive);
      else
      {
         mRemoteRate.maxRecvBandwidth = bstream->readRangedU32(0, MaxFixedBandwidth);
         mRemoteRate.maxSendBandwidth = bstream->readRangedU32(0, MaxFixedBandwidth);
         mRemoteRate.minPacketRecvPeriod = bstream->readRangedU32(1, MaxFixedSendPeriod);
         mRemoteRate.minPacketSendPeriod = bstream->readRangedU32(1, MaxFixedSendPeriod);
         computeNegotiatedRate();
      }
   }
}

void NetConnection::computeNegotiatedRate()
{
   mCurrentPacketSendPeriod = getMax(mLocalRate.minPacketSendPeriod, mRemoteRate.minPacketRecvPeriod);

   U32 maxBandwidth = getMin(mLocalRate.maxSendBandwidth, mRemoteRate.maxRecvBandwidth);
   mCurrentPacketSendSize = U32(maxBandwidth * mCurrentPacketSendPeriod * 0.001f);

   // make sure we don't try to overwrite the maximum packet size
   if(mCurrentPacketSendSize > MaxPacketDataSize)
      mCurrentPacketSendSize = MaxPacketDataSize;
}

void NetConnection::setIsAdaptive()
{
   mTypeFlags.set(ConnectionAdaptive);
   mLocalRateChanged = true;
}

void NetConnection::setFixedRateParameters(U32 minPacketSendPeriod, U32 minPacketRecvPeriod, U32 maxSendBandwidth, U32 maxRecvBandwidth)
{
   mTypeFlags.clear(ConnectionAdaptive);

   mLocalRate.maxRecvBandwidth = maxRecvBandwidth;
   mLocalRate.maxSendBandwidth = maxSendBandwidth;
   mLocalRate.minPacketRecvPeriod = minPacketRecvPeriod;
   mLocalRate.minPacketSendPeriod = minPacketSendPeriod;
   mLocalRateChanged = true;
   computeNegotiatedRate();
}

//--------------------------------------------------------------------

void NetConnection::sendPingPacket()
{
   PacketStream ps;
   writeRawPacket(&ps, PingPacket);
   logprintf(LogConsumer::LogConnectionProtocol, "send ping %d", mLastSendSeq);

   sendPacket(&ps);
}

void NetConnection::sendAckPacket()
{
   PacketStream ps;
   writeRawPacket(&ps, AckPacket);
   logprintf(LogConsumer::LogConnectionProtocol, "send ack %d", mLastSendSeq);

   sendPacket(&ps);
}

//--------------------------------------------------------------------

void NetConnection::handleNotify(U32 sequence, bool recvd)
{
   logprintf(LogConsumer::LogNetConnection, "NetConnection %s: NOTIFY %d %s", mNetAddress.toString(), sequence, recvd ? "RECVD" : "DROPPED");

   PacketNotify *note = mNotifyQueueHead;
   TNLAssert(note != NULL, "Error: got a notify with a null notify head.");
   mNotifyQueueHead = mNotifyQueueHead->nextPacket;

   if(note->rateChanged && !recvd)
      mLocalRateChanged = true;

   if(recvd)
   {
      mHighestAckedSendTime = note->sendTime;

      if(isAdaptive())
      {
         // Deal with updating our cwnd and ssthresh...
         if(cwnd < ssthresh)
         {
            // Slow start strategy
            cwnd++;
            logprintf(LogConsumer::LogNetConnection, "PKT SSOK - ssthresh = %f     cwnd=%f", ssthresh, cwnd);

         } else {
            // We are in normal state..
            if(cwnd < MaxPacketWindowSize-2)
               cwnd += 1/cwnd;

            logprintf(LogConsumer::LogNetConnection, "PKT   OK - ssthresh = %f     cwnd=%f", ssthresh, cwnd);

         }
      }

      packetReceived(note);
   }
   else
   {
      if(isAdaptive())
      {
         // Deal with updating our cwnd and ssthresh...
         ssthresh = (0.5f * ssthresh < 2) ? 2 : (0.5f * ssthresh);
         cwnd -= 1;
         if(cwnd < 2) cwnd = 2;

/*         logprintf(LogConsumer::LogNetConnection, "  * ack=%f   pktDt=%d    time=%f (%d)     seq=%d %d %d %d",
                        ack,
                        ackDelta,
                        deltaT / 1000.0f,
                        deltaT,
                        mLastSeqRecvd,
                        mLastRecvAckAck,
                        mLastSeqRecvdAck,
                        mHighestAckedSeq
         ); */

      }

      packetDropped(note);
   }
   delete note;
}

//--------------------------------------------------------------------

void NetConnection::checkPacketSend(bool force, U32 curTime)
{
   U32 delay = mCurrentPacketSendPeriod;

   if(!force)
   {
      if(!isAdaptive())
      {
         if(curTime - mLastUpdateTime + mSendDelayCredit < delay)
            return;
      
         mSendDelayCredit = curTime - (mLastUpdateTime + delay - mSendDelayCredit);
         if(mSendDelayCredit > 1000)
            mSendDelayCredit = 1000;
      }
   }
   prepareWritePacket();
   if(windowFull() || !isDataToTransmit())
   {
      // there is nothing to transmit, or the window is full
      if(isAdaptive())
      {
         // Still, on an adaptive connection, we may need to send an ack here...

         // Check if we should ack. We use a heuristic to do this. (fuzzy logic!)
         S32 ackDelta = (mLastSeqRecvd - mLastSeqRecvdAck);
         F32 ack = ackDelta / 4.0f;

         // Multiply by the time since we've acked...
         // If we're much below 200, we don't want to ack; if we're much over we do.
         U32 deltaT = (curTime - mLastAckTime);
         ack = ack *  deltaT / 200.0f;

         if((ack > 1.0f || (ackDelta > (0.75*MaxPacketWindowSize))) && (mLastSeqRecvdAck != mLastSeqRecvd))
         {         
            mLastSeqRecvdAck = mLastSeqRecvd;
            mLastAckTime = curTime;
            sendAckPacket();
         }
      }
      return;
   }
   PacketStream stream(mCurrentPacketSendSize);
   mLastUpdateTime = curTime;

   writeRawPacket(&stream, DataPacket);   

   sendPacket(&stream);
}

bool NetConnection::windowFull()
{
   if(mLastSendSeq - mHighestAckedSeq >= (MaxPacketWindowSize - 2))
      return true;
   if(isAdaptive())
      return mLastSendSeq - mHighestAckedSeq >= cwnd;
   return false;
}

NetError NetConnection::sendPacket(BitStream *stream)
{
   if(mSimulatedSendPacketLoss && Random::readF() < mSimulatedSendPacketLoss)
   {
      logprintf(LogConsumer::LogNetConnection, "NetConnection %s: SENDDROP - %d", mNetAddress.toString(), getLastSendSequence());
      return NoError;
   }

   logprintf(LogConsumer::LogNetConnection, "NetConnection %s: SEND - %d bytes", mNetAddress.toString(), stream->getBytePosition());

   // do nothing on send if this is a demo replay.
   if(isLocalConnection())
   {
      // short circuit connection to the other side.
      // handle the packet, then force a notify.
      if(mSimulatedSendLatency + mRemoteConnection->mSimulatedReceiveLatency != 0)
      {
         mInterface->sendtoDelayed(NULL, mRemoteConnection, stream, mSimulatedSendLatency + mRemoteConnection->mSimulatedReceiveLatency);
      }
      else
      {
         U32 size = stream->getBytePosition();

         stream->reset();
         stream->setMaxSizes(size, 0);
      
         mRemoteConnection->readRawPacket(stream);
      }
      return NoError;
   }
   else
   {
      if(mSimulatedSendLatency)
      {
         mInterface->sendtoDelayed(&getNetAddress(), NULL, stream, mSimulatedSendLatency);
         return NoError;
      }
      else
         return mInterface->sendto(getNetAddress(), stream);
   }
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------

// these are the virtual function defs for Connection -
// if your subclass has additional data to read / write / notify, add it in these functions.

void NetConnection::readPacket(BitStream *bstream)
{
}

void NetConnection::prepareWritePacket()
{
}

void NetConnection::writePacket(BitStream *bstream, PacketNotify *note)
{
}

void NetConnection::packetReceived(PacketNotify *note)
{
   if(mStringTable)
      mStringTable->packetReceived(&note->stringList);
}

void NetConnection::packetDropped(PacketNotify *note)
{
   if(mStringTable)
      mStringTable->packetDropped(&note->stringList);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NetConnection::setTranslatesStrings()
{
   if(!mStringTable) 
      mStringTable = new ConnectionStringTable(this);
}


void NetConnection::setInterface(NetInterface *myInterface)
{
   mInterface = myInterface;
}


NetInterface *NetConnection::getInterface()
{
   return mInterface;
}


U32 NetConnection::getTimeSinceLastPacketReceived() 
{ 
   return mInterface->getCurrentTime() - mLastPacketRecvTime; 
}


void NetConnection::setSymmetricCipher(SymmetricCipher *theCipher)
{
   mSymmetricCipher = theCipher;
}


void NetConnection::connect(NetInterface *theInterface, const Address &address, bool requestKeyExchange, bool requestCertificate)
{
   mConnectionParameters.mRequestKeyExchange = requestKeyExchange;
   mConnectionParameters.mRequestCertificate = requestCertificate;
   mConnectionParameters.mIsInitiator = true;

   setNetAddress(address);
   setInterface(theInterface);
   mInterface->startConnection(this);
}


void NetConnection::connectArranged(NetInterface *connectionInterface, const Vector<Address> &possibleAddresses, 
                                    Nonce &nonce, Nonce &serverNonce, ByteBufferPtr sharedSecret, bool isInitiator, 
                                    bool requestsKeyExchange, bool requestsCertificate)
{
   mConnectionParameters.mRequestKeyExchange = requestsKeyExchange;
   mConnectionParameters.mRequestCertificate = requestsCertificate;
   mConnectionParameters.mPossibleAddresses = possibleAddresses;
   mConnectionParameters.mIsInitiator = isInitiator;
   mConnectionParameters.mIsArranged = true;
   mConnectionParameters.mNonce = nonce;
   mConnectionParameters.mServerNonce = serverNonce;
   mConnectionParameters.mArrangedSecret = sharedSecret;
   mConnectionParameters.mArrangedSecret->takeOwnership();

   setInterface(connectionInterface);
   mInterface->startArrangedConnection(this);   
}

void NetConnection::disconnect(TerminationReason tr, const char *reason)
{
   mInterface->disconnect(this, tr, reason);
}

void NetConnection::onConnectionEstablished()
{
   if(isInitiator())
      setIsConnectionToServer();
   else
      setIsConnectionToClient();
}


void NetConnection::onConnectionTerminated(TerminationReason, const char *)
{
   // Do nothing
}


void NetConnection::onConnectTerminated(TerminationReason, const char *)
{
   // Do nothing
}

void NetConnection::writeConnectRequest(BitStream *stream)
{
   stream->write(U32(getNetClassGroup()));
   stream->write(U32(NetClassRep::getClassGroupCRC(getNetClassGroup())));
}

bool NetConnection::readConnectRequest(BitStream *stream, NetConnection::TerminationReason &reason)
{
   U32 classGroup, classCRC;
   stream->read(&classGroup);
   stream->read(&classCRC);

   if((NetClassGroup)classGroup == getNetClassGroup() && classCRC == NetClassRep::getClassGroupCRC(getNetClassGroup()))
      return true;

   reason = ReasonInvalidCRC;
   return false;
}

void NetConnection::writeConnectAccept(BitStream *stream)
{
   // Do nothing
}

bool NetConnection::readConnectAccept(BitStream *stream, TerminationReason &reason)
{
   return true;
}

bool NetConnection::connectLocal(NetInterface *connectionInterface, NetInterface *serverInterface)
{
   Object *co = Object::create(getClassName());
   NetConnection *client = this;
   NetConnection *server = dynamic_cast<NetConnection *>(co);
   NetConnection::TerminationReason reason;
   PacketStream stream;

   if(!server)
      goto errorOut;

   client->setInterface(connectionInterface);
   client->getConnectionParameters().mIsInitiator = true;
   client->getConnectionParameters().mIsLocal = true;
   server->getConnectionParameters().mIsLocal = true;

   server->setInterface(serverInterface);

   server->setInitialRecvSequence(client->getInitialSendSequence());
   client->setInitialRecvSequence(server->getInitialSendSequence());
   client->setRemoteConnectionObject(server);
   server->setRemoteConnectionObject(client);

   stream.setBytePosition(0);
   client->writeConnectRequest(&stream);
   stream.setBytePosition(0);
   if(!server->readConnectRequest(&stream, reason))
      goto errorOut;

   stream.setBytePosition(0);
   server->writeConnectAccept(&stream);
   stream.setBytePosition(0);

   if(!client->readConnectAccept(&stream, reason))
      goto errorOut;

   client->setConnectionState(NetConnection::Connected);
   server->setConnectionState(NetConnection::Connected);

   client->onConnectionEstablished();
   server->onConnectionEstablished();
   connectionInterface->addConnection(client);
   serverInterface->addConnection(server);
   return true;
errorOut:
   delete server;
   return false;
}

};
