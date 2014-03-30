//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CONTROLOBJECTCONNECTION_H_
#define _CONTROLOBJECTCONNECTION_H_

#include "move.h"
#include "Point.h"
#include "BfObject.h" 

#include "tnl.h"
#include "tnlGhostConnection.h"

using namespace TNL;

namespace Zap
{

struct ControlObjectData : public Move
{
   Point mPos;
   Point mVel;
   Point mImpulseVector;
   S32 mEnergy;
   S32 mFireTimer;
   U32 mFastRechargeTimer;
   U32 mSpyBugPlacementTimer;
   U32 mPulseTimer;
   bool mCooldownNeeded;
   bool mFastRecharging;
   bool mBoostActive;
};

class BfObject;

class ControlObjectConnection: public GhostConnection    // only child class is GameConnection...
{
private:
   typedef GhostConnection Parent;

   // Move management
   enum {
      MaxPendingMoves = 63,
      MaxMoveTimeCredit = 512,
   };


   Vector<ControlObjectData> pendingMoves;
   SafePtr<BfObject> controlObject;

   U32 mLastClientControlCRC;
   Point mServerPosition;
   bool mCompressPointsRelative;

   S8 firstMoveIndex;
   S8 highSendIndex[3];
   U32 mMoveTimeCredit;

   U32 mTimeSinceLastMove; 
   F32 mPrevAngle;

   bool mObjectMovedThisGame;

   void onGotNewMove(const Move &move);

protected:
   bool mIsBusy;
   bool mNeedReplayMoves;

public:
   ControlObjectConnection();
   virtual ~ControlObjectConnection();

   void setControlObject(BfObject *theObject);
   BfObject *getControlObject() const;
   U32 getControlCRC();

   virtual void addPendingMove(Move *theMove);
   bool ControlObjectConnection::isMovesFull();

   struct GamePacketNotify : public GhostConnection::GhostPacketNotify
   {
      S8 firstUnsentMoveIndex;
      Point lastControlObjectPosition;
      GamePacketNotify();
   };

   PacketNotify *allocNotify();

   void writePacket(BitStream *bstream, PacketNotify *notify);
   void readPacket(BitStream *bstream);

	void prepareReplay();

   void packetReceived(PacketNotify *notify);
   void addToTimeCredit(U32 timeAmount);

   bool isDataToTransmit();

   void writeCompressedPoint(const Point &p, BitStream *stream);
   void readCompressedPoint(Point &p, BitStream *stream);

   void addTimeSinceLastMove(U32 time);
   U32 getTimeSinceLastMove();
   void resetTimeSinceLastMove();

   void setObjectMovedThisGame(bool moved);
   bool getObjectMovedThisGame();
};


};

#endif


