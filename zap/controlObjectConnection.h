//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _CONTROLOBJECTCONNECTION_H_
#define _CONTROLOBJECTCONNECTION_H_

#include "../tnl/tnl.h"
#include "../tnl/tnlGhostConnection.h"

#include "move.h"
#include "Point.h"
#include "BfObject.h" 

using namespace TNL;

namespace Zap
{

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

   Vector<Move> pendingMoves;
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

public:
   ControlObjectConnection();

   void setControlObject(BfObject *theObject);
   BfObject *getControlObject();
   U32 getControlCRC();

   void addPendingMove(Move *theMove);

   struct GamePacketNotify : public GhostConnection::GhostPacketNotify
   {
      S8 firstUnsentMoveIndex;
      Point lastControlObjectPosition;
      GamePacketNotify();
   };

   PacketNotify *allocNotify();

   void writePacket(BitStream *bstream, PacketNotify *notify);
   void readPacket(BitStream *bstream);

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


