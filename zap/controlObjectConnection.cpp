//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "controlObjectConnection.h"
#include "BfObject.h"
#include "game.h"

#include "ship.h"

#include <math.h>

namespace Zap
{

ControlObjectConnection::ControlObjectConnection()
{
   highSendIndex[0] = 0;
   highSendIndex[1] = 0;
   highSendIndex[2] = 0;
   mLastClientControlCRC = 0;
   firstMoveIndex = 1;
   mMoveTimeCredit = 0;

   mTimeSinceLastMove = 0;
   mPrevAngle = 0;

   mCompressPointsRelative = false;

   mObjectMovedThisGame = false;
   mIsBusy = false;
}


// Destructor
ControlObjectConnection::~ControlObjectConnection()
{
   // Do nothing
}


// Only gets run on the server
void ControlObjectConnection::setControlObject(BfObject *theObject)
{
   if(controlObject.isValid())
      controlObject->setControllingClient(NULL);

   controlObject = theObject;

   if(theObject)
      theObject->setControllingClient((GameConnection *) this);
}


void ControlObjectConnection::packetReceived(PacketNotify *notify)
{
   if(isConnectionToServer())  // only client needs to handle this
   {
      for(/* empty */; S8(firstMoveIndex - ((Zap::ControlObjectConnection::GamePacketNotify *) notify)->firstUnsentMoveIndex) < 0; firstMoveIndex++)
         pendingMoves.erase(U32(0));
   }
   mServerPosition = ((GamePacketNotify *) notify)->lastControlObjectPosition;
   Parent::packetReceived(notify);
}


BfObject *ControlObjectConnection::getControlObject() const
{
   return controlObject;
}


U32 ControlObjectConnection::getControlCRC()
{
   PacketStream stream;
   BfObject *co = getControlObject();

   if(!co)
      return 0;

   stream.writeInt(getGhostIndex(co), GhostConnection::GhostIdBitSize);
   co->writeControlState(&stream);
   stream.zeroToByteBoundary();
   return stream.calculateCRC(0, stream.getBytePosition());
}


void ControlObjectConnection::addPendingMove(Move *theMove)
{
   if(controlObject.isNull())
      return;

   if(pendingMoves.size() != 0 &&
      (theMove->time + pendingMoves.last().time < 50 ||   // Send less often when almost full.
      (theMove->time + pendingMoves.last().time < 8 && pendingMoves.size() < MaxPendingMoves-10)) &&
      U8(highSendIndex[2] - firstMoveIndex) != pendingMoves.size())
   {
      ControlObjectData *m = &pendingMoves.last();
      ((Ship*)controlObject.getPointer())->setState(m);
      theMove->time += m->time;
      *(Move*)m = *theMove;
   }
   else if(pendingMoves.size() < MaxPendingMoves)
   {
      ControlObjectData data;
      *((Move*)(&data)) = *theMove;
      if(controlObject.isValid() && controlObject->getObjectTypeNumber() == PlayerShipTypeNumber)
         ((Ship*)controlObject.getPointer())->getState(&data);
      pendingMoves.push_back(data);
   }
   else
      return;  // Has an effect of not moving your ship, usually when losing connection.

   controlObject->setCurrentMove(*theMove);
   controlObject->idle(BfObject::ClientReplayingPendingMoves);
}



ControlObjectConnection::PacketNotify *ControlObjectConnection::allocNotify()
{
   return new GamePacketNotify;
}

static U8 CLIENTCONTROLBITS = 16;

void ControlObjectConnection::writePacket(BitStream *bstream, PacketNotify *notify)
{
   if(isConnectionToServer())
   {
      S8 firstSendIndex = highSendIndex[0];
      
      // Cast to S8 appears to be needed here, even though they are both already S8, 
      // somehow the compiler converts them to 32 bit, screwing up the S8 overflow
      if(S8(firstSendIndex - firstMoveIndex) < 0)   
         firstSendIndex = firstMoveIndex;

      bstream->writeInt(getControlCRC(), CLIENTCONTROLBITS);

      bstream->write(firstSendIndex);
      U32 skipCount = (U32) S8(firstSendIndex - firstMoveIndex);
      U32 moveCount = pendingMoves.size() - skipCount;

      bstream->writeRangedU32(moveCount, 0, MaxPendingMoves);
      Move empty;                // Create a new, empty move, representing just standing still, doing nothing.
      Move *lastMove = &empty;

      // The fn pack() compares the current move to the previous one, and only packs if they are different.
      // The first time through this loop, we'll be comparing the move to be packed with our empty move, and
      // so pack() only really has an effect when our move is actually doing something, either moving,
      // firing, or activating a module.  See the Move constructor and the pack() fn for more details.
      for(S32 i = skipCount; i < pendingMoves.size(); i++)
      {
         pendingMoves[i].pack(bstream, lastMove, true);
         lastMove = &pendingMoves[i];
      }
      ((GamePacketNotify *) notify)->firstUnsentMoveIndex = firstMoveIndex + S8(pendingMoves.size());
      if(controlObject.isValid())
         ((GamePacketNotify *) notify)->lastControlObjectPosition = controlObject->getPos();

      highSendIndex[0] = highSendIndex[1];
      highSendIndex[1] = highSendIndex[2];
      highSendIndex[2] = ((GamePacketNotify *) notify)->firstUnsentMoveIndex;
   }
   else     // We're on the server, sending packet to client.  I think...
   {
      S32 ghostIndex = -1;
      if(controlObject.isValid())
      {
         ghostIndex = getGhostIndex(controlObject);
         mServerPosition = controlObject->getPos();
      }

      // We only compress points relative if we know that the
      // remote side has a copy of the control object already
      mCompressPointsRelative = bstream->writeFlag(ghostIndex != -1);

      if(bstream->writeFlag( (getControlCRC() & (0xFFFFFFFF >> CLIENTCONTROLBITS)) != mLastClientControlCRC ))
      {
         if(ghostIndex != -1)
         {
            bstream->writeInt(ghostIndex, GhostConnection::GhostIdBitSize);
            controlObject->writeControlState(bstream);
         }
      }
   }
   Parent::writePacket(bstream, notify);
}


void ControlObjectConnection::readPacket(BitStream *bstream)
{
   // We only replay control object moves if we got an update
   // from the server.  However, we wait until after the super
   // class connection objects have a chance to process so
   // that ghosts of other objects can be updated first.
   // This way the control object won't get ahead of objects
   // it is pushing in a laggy situation.

   bool replayControlObjectMoves = false;

   if(isConnectionToClient())    // Is server ??
   {
      mLastClientControlCRC = bstream->readInt(CLIENTCONTROLBITS);

      S8 firstMove;
      bstream->read(&firstMove);
      U32 count = bstream->readRangedU32(0, MaxPendingMoves);

      Move theMove;

      for(/* empty */; S8(firstMove - firstMoveIndex) < 0 && count > 0; firstMove++)
      {
         // Looks like we'll be ignoring these moves
         count--;
         theMove.unpack(bstream, true);
      }
      for(/* empty */; count > 0; count--)
      {
         theMove.unpack(bstream, true);
         // Process the move, including crediting time to the client and all that joy.
         // The time crediting prevents clients from hacking speed cheats
         // that feed more moves to the server than are allowed.
         if(mMoveTimeCredit >= theMove.time && controlObject.isValid() && !(controlObject->isDeleted()))
         {
            mMoveTimeCredit -= theMove.time;
            controlObject->setCurrentMove(theMove);
            controlObject->idle(BfObject::ServerProcessingUpdatesFromClient);
            onGotNewMove(theMove);
         }
         else
            mMoveTimeCredit = mMoveTimeCredit;
         firstMoveIndex++;
      }
   }
   else     // Is connection to server (i.e. we're on the client, I think)
   {
      bool controlObjectValid = bstream->readFlag();     

      mCompressPointsRelative = controlObjectValid;
      //mGameUserInterface.receivedControlUpdate(false);

      //onGotNewMove();

      // CRC mismatch...
      if(bstream->readFlag())
      {
         if(controlObjectValid)
         {
            U32 ghostIndex = bstream->readInt(GhostConnection::GhostIdBitSize);
            BfObject *prevControlObject = controlObject.getPointer();
            controlObject = (BfObject *) resolveGhost(ghostIndex);
            TNLAssert(controlObject, "controlObject is NULL, bstream will be broken");
            if(controlObject)
            {
               if(controlObject.isValid() &&
                  controlObject.getPointer() == prevControlObject &&
                  controlObject->getObjectTypeNumber() == PlayerShipTypeNumber &&
                  pendingMoves.size() != 0)
                     ((Ship*)controlObject.getPointer())->setState(&pendingMoves[0]);
               controlObject->readControlState(bstream);
            }
            mServerPosition = controlObject->getPos();
            replayControlObjectMoves = true;
         }
         else
            controlObject = NULL;
      }
   }
   Parent::readPacket(bstream);


   if(replayControlObjectMoves && controlObject.isValid())
   {
      for(S32 i = 0; i < pendingMoves.size(); i++)
      {
         if(controlObject.isValid() && controlObject->getObjectTypeNumber() == PlayerShipTypeNumber)
            ((Ship*)controlObject.getPointer())->getState(&pendingMoves[i]);
         Move theMove = pendingMoves[i];
         theMove.prepare();
         controlObject->setCurrentMove(theMove);
         controlObject->idle(BfObject::ClientReplayingPendingMoves);
      }
      controlObject->controlMoveReplayComplete();
   }
}


// A new move has arrived
void ControlObjectConnection::onGotNewMove(const Move &move)
{
   // See if the player actually moved, or if this is just an empty "do nothing" move
   if(move.x != 0 || move.y != 0 || move.fire || move.isAnyModActive() || move.angle != mPrevAngle)
   {
      setObjectMovedThisGame(true);

      // Only reset time if this connection isn't 'busy' (chatting or in menus)
      if(!mIsBusy)
         resetTimeSinceLastMove();
   }

   mPrevAngle = move.angle;
}


U32 ControlObjectConnection::getTimeSinceLastMove()
{
   return mTimeSinceLastMove;
}


void ControlObjectConnection::addTimeSinceLastMove(U32 time)
{
   mTimeSinceLastMove += time;
}


void ControlObjectConnection::resetTimeSinceLastMove()
{
   mTimeSinceLastMove = 0;
}


void ControlObjectConnection::setObjectMovedThisGame(bool moved)
{
   mObjectMovedThisGame = moved;
}


bool ControlObjectConnection::getObjectMovedThisGame()
{
   return mObjectMovedThisGame;
}


void ControlObjectConnection::writeCompressedPoint(const Point &p, BitStream *stream)
{
   if(!mCompressPointsRelative)
   {
      stream->write(p.x);
      stream->write(p.y);
      return;
   }

   Point delta = p - mServerPosition;
    // floor(number + 0.5) fix rounding problems (was 5 = (U32)5.95)
   S32 dx = (S32) floor((delta.x + Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL + Game::PLAYER_SCOPE_MARGIN) + 0.5f);
   S32 dy = (S32) floor((delta.y + Game::PLAYER_VISUAL_DISTANCE_VERTICAL + Game::PLAYER_SCOPE_MARGIN) + 0.5f);

   S32 maxx = (Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL + Game::PLAYER_SCOPE_MARGIN) * 2;
   S32 maxy = (Game::PLAYER_VISUAL_DISTANCE_VERTICAL + Game::PLAYER_SCOPE_MARGIN) * 2;

   if(stream->writeFlag(dx >= 0 && dx <= maxx && dy >= 0 && dy <= maxy))
   {
      stream->writeRangedU32(dx, 0, maxx);
      stream->writeRangedU32(dy, 0, maxy);
   }
   else
   {
      stream->write(p.x);
      stream->write(p.y);
   }
}

void ControlObjectConnection::readCompressedPoint(Point &p, BitStream *stream)
{
   if(!mCompressPointsRelative)
   {
      stream->read(&p.x);
      stream->read(&p.y);
      return;
   }
   if(stream->readFlag())
   {
      U32 maxx = (Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL + Game::PLAYER_SCOPE_MARGIN) * 2;
      U32 maxy = (Game::PLAYER_VISUAL_DISTANCE_VERTICAL + Game::PLAYER_SCOPE_MARGIN) * 2;

      F32 dx = F32(stream->readRangedU32(0, maxx)) - (Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL + Game::PLAYER_SCOPE_MARGIN);
      F32 dy = F32(stream->readRangedU32(0, maxy)) - (Game::PLAYER_VISUAL_DISTANCE_VERTICAL + Game::PLAYER_SCOPE_MARGIN);

      Point delta(dx, dy);
      p = mServerPosition + delta;
   }
   else
   {
      stream->read(&p.x);
      stream->read(&p.y);
   }
}


void ControlObjectConnection::addToTimeCredit(U32 timeAmount)
{
   mMoveTimeCredit += timeAmount;
   
   if(mMoveTimeCredit > MaxMoveTimeCredit)
   {
      // Prevent unlimited shield when client is freezing, slow, or lagging (or some cheater has triggered a breakpoint in rabbit!)
      if(controlObject.isValid())
      {
         Move move = controlObject->getCurrentMove();
         move.time = mMoveTimeCredit - MaxMoveTimeCredit;
         controlObject->setCurrentMove(move);
         controlObject->idle(BfObject::ServerProcessingUpdatesFromClient);
      }

      mMoveTimeCredit = MaxMoveTimeCredit;
   }
}


bool ControlObjectConnection::isDataToTransmit()
{
   return true;
}



ControlObjectConnection::GamePacketNotify::GamePacketNotify()
{
   firstUnsentMoveIndex =  0;
}



};


