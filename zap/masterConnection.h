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

#ifndef _MASTERCONNECTION_H_
#define _MASTERCONNECTION_H_

#include "../master/masterInterface.h"

#include "gameType.h"

#include "../tnl/tnlNetObject.h"

#include "gameConnection.h"
#include <string>

using namespace std;

namespace Zap
{

class MasterServerConnection : public MasterServerInterface
{
   typedef MasterServerInterface Parent;

   
   U32 mCurrentQueryId;    // ID of our current query

   bool mIsGameServer;
   U32 mInfoFlags;
   string mMasterName;

public:
   MasterServerConnection(bool isGameServer = false, U32 infoFlags = 0)    // Constructor
   {
      mIsGameServer = isGameServer;
      mInfoFlags = infoFlags;
      mCurrentQueryId = 0;
      setIsConnectionToServer();
      setIsAdaptive();
   }

   void setMasterName(string name);
   string getMasterName();

   void startServerQuery();
   Vector<IPAddress> mServerList;

   void cancelArrangedConnectionAttempt() { mCurrentQueryId++; }
   void requestArrangedConnection(const Address &remoteAddress);
   void updateServerStatus(StringTableEntry levelName, StringTableEntry levelType, U32 botCount, 
                           U32 playerCount, U32 maxPlayers, U32 infoFlags)
   {
      s2mUpdateServerStatus(levelName, levelType, botCount, playerCount, maxPlayers, infoFlags);
   }

   TNL_DECLARE_RPC_OVERRIDE(m2cQueryServersResponse, (U32 queryId, Vector<IPAddress> ipList));

   TNL_DECLARE_RPC_OVERRIDE(m2sClientRequestedArrangedConnection, (U32 requestId, Vector<IPAddress> possibleAddresses,
      ByteBufferPtr connectionParameters));

         // TODO: Delete after 014 -- replaced with identical m2sClientRequestedArrangedConnection above
         TNL_DECLARE_RPC_OVERRIDE(m2cClientRequestedArrangedConnection, 
               (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionParameters));

   TNL_DECLARE_RPC_OVERRIDE(m2cArrangedConnectionAccepted, 
               (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData));
   TNL_DECLARE_RPC_OVERRIDE(m2cArrangedConnectionRejected, (U32 requestId, ByteBufferPtr rejectData));
   TNL_DECLARE_RPC_OVERRIDE(m2cSetAuthenticated, (RangedU32<0, AuthenticationStatusCount> authStatus, StringPtr correctedName));

   TNL_DECLARE_RPC_OVERRIDE(m2cSetMOTD, (StringPtr masterName, StringPtr motdString));
   TNL_DECLARE_RPC_OVERRIDE(m2cSendUpdgradeStatus, (bool needToUpgrade));

   // Incoming out-of-game chat message from master
   TNL_DECLARE_RPC_OVERRIDE(m2cSendChat, (StringTableEntry clientName, bool isPrivate, StringPtr message));      
   
   // For managing list of players in global chat
   TNL_DECLARE_RPC_OVERRIDE(m2cPlayerJoinedGlobalChat, (StringTableEntry playerNick));
   TNL_DECLARE_RPC_OVERRIDE(m2cPlayerLeftGlobalChat, (StringTableEntry playerNick));
   TNL_DECLARE_RPC_OVERRIDE(m2cPlayersInGlobalChat, (Vector<StringTableEntry> playerNicks));

   void requestAuthentication(StringTableEntry mClientName, Nonce mClientId);

   TNL_DECLARE_RPC_OVERRIDE(m2sSetAuthenticated, (Vector<U8> id, StringTableEntry name, RangedU32<0,AuthenticationStatusCount> status));

   void writeConnectRequest(BitStream *bstream);
   void onConnectionEstablished();
   //void onConnectionTerminated(blah);                                  // An existing connection has been terminated
   void onConnectTerminated(TerminationReason r, const char *string);    // A still-being-established connection has been terminated

   TNL_DECLARE_NETCONNECTION(MasterServerConnection);
};

};

#endif

