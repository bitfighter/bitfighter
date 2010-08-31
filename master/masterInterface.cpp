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

#include "masterInterface.h"

// Since this is an interface, we implement a bunch of stubs.  These will be overridden on the
// client, server, or master end as needed.  This interface will be compiled into both the
// master and the individual clients.

//
//   IMPORTANT NOTE!!!
//
//   When adding new RPCs that did not exist in previous versions of the game, make sure to give them a new
//   rpcVersion number, otherwise clients will not be able to connect!
//

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mQueryServers,
   (U32 queryId, U32 regionMask, U32 minPlayers, U32 maxPlayers, U32 infoFlags,
   U32 maxBots, U32 minCPUSpeed, StringTableEntry gameType, StringTableEntry missionType),
   (queryId, regionMask, minPlayers, maxPlayers, infoFlags, maxBots, minCPUSpeed, gameType, missionType),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cQueryServersResponse,
   (U32 queryId, Vector<IPAddress> ipList), (queryId, ipList),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mRequestArrangedConnection, (U32 requestId,
   IPAddress remoteAddress, IPAddress internalAddress, ByteBufferPtr connectionParameters),
   (requestId, remoteAddress, internalAddress, connectionParameters),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2sClientRequestedArrangedConnection,
   (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionParameters),
   (requestId, possibleAddresses, connectionParameters),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 3) {}

      // TODO: Delete after 014 -- replaced with identical m2sClientRequestedArrangedConnection above
      TNL_IMPLEMENT_RPC(MasterServerInterface, m2cClientRequestedArrangedConnection,
         (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionParameters),
         (requestId, possibleAddresses, connectionParameters),
         NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mAcceptArrangedConnection,
   (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData),
   (requestId, internalAddress, connectionData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 3) {}

      // TODO: Delete after 014 -- replaced with identical s2mAcceptArrangedConnection above
      TNL_IMPLEMENT_RPC(MasterServerInterface, c2mAcceptArrangedConnection,
         (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData),
         (requestId, internalAddress, connectionData),
         NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

      // TODO: Delete after 014 -- replaced with identical s2mRejectArrangedConnection below
      TNL_IMPLEMENT_RPC(MasterServerInterface, c2mRejectArrangedConnection,
         (U32 requestId, ByteBufferPtr rejectData),
         (requestId, rejectData),
         NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, s2mRejectArrangedConnection,
   (U32 requestId, ByteBufferPtr rejectData),
   (requestId, rejectData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 3) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cArrangedConnectionAccepted,
   (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData),
   (requestId, possibleAddresses, connectionData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cArrangedConnectionRejected,
   (U32 requestId, ByteBufferPtr rejectData),
   (requestId, rejectData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mUpdateServerStatus, (
   StringTableEntry levelName, StringTableEntry levelType,
   U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags),
   (levelName, levelType, botCount, playerCount, maxPlayers, infoFlags),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 3) {}

      // TODO: Delete after 014 -- replaced with identical s2mUpdateServerStatus above
      TNL_IMPLEMENT_RPC(MasterServerInterface, c2mUpdateServerStatus, (
         StringTableEntry levelName, StringTableEntry levelType,
         U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags),
         (levelName, levelType, botCount, playerCount, maxPlayers, infoFlags),
         NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSetMOTD, (TNL::StringPtr masterName, TNL::StringPtr motdString), (masterName, motdString),
                  NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}


// Out-of-game chat relay functions:
TNL_IMPLEMENT_RPC(MasterServerInterface, c2mSendChat, (TNL::StringPtr message), (message),
                  NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSendChat, (StringTableEntry playerNick, bool isPrivate, TNL::StringPtr message), (playerNick, isPrivate, message),
                  NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}


// For managing list of players in global chat
TNL_IMPLEMENT_RPC(MasterServerInterface, c2mJoinGlobalChat, (), (),
                  NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 2) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mLeaveGlobalChat, (), (),
                  NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 2) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, m2cPlayerJoinedGlobalChat, (StringTableEntry playerNick), (playerNick),
                  NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 2) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cPlayersInGlobalChat, (Vector<StringTableEntry> playerNicks), (playerNicks),
                  NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 2) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cPlayerLeftGlobalChat, (StringTableEntry playerNick), (playerNick),
                  NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 2) {}


// Implement a need-to-updrade verification service, without breaking older clients, by updgrading the version 1
// All clients that implement only 0-verison events will ignore this.  In theory.
TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSendUpdgradeStatus, (bool needToUpgrade), (needToUpgrade),
                  NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1) {}


// Send player statistics to the master server
TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendPlayerStatistics, 
   (StringTableEntry playerName, U16 kills, U16 deaths, U16 suicides, Vector<U16> shots, Vector<U16> hits),
   (playerName, kills, deaths, suicides, shots, hits),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 3) {}


// Send game statistics to the master server
TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendGameStatistics, (StringTableEntry gameType, StringTableEntry levelName, 
                                                                 RangedU32<0,MAX_PLAYERS> players, S16 time),
   (gameType, levelName, players, time),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 3) {}



TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendPlayerStatistics_2, 
   (StringTableEntry playerName, StringTableEntry teamName, U16 kills, U16 deaths, U16 suicides, Vector<U16> shots, Vector<U16> hits),
   (playerName, teamName, kills, deaths, suicides, shots, hits),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 4) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendGameStatistics_2, (StringTableEntry gameType, StringTableEntry levelName,
                                                                   Vector<StringTableEntry> teams, Vector<S32> teamScores,
                                                                   RangedU32<0,MAX_PLAYERS> players, S16 time),
   (gameType, levelName, teams, teamScores, players, time),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 4) {}

