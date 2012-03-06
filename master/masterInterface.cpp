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
#include "../zap/gameStats.h"

//struct VersionedGameStats;

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
   (U32 queryId), (queryId),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cQueryServersResponse,
   (U32 queryId, Vector<IPAddress> ipList), (queryId, ipList),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mRequestArrangedConnection, 
   (U32 requestId, IPAddress remoteAddress, IPAddress internalAddress, ByteBufferPtr connectionParameters),
   (requestId, remoteAddress, internalAddress, connectionParameters),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2sClientRequestedArrangedConnection,
   (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionParameters),
   (requestId, possibleAddresses, connectionParameters),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mAcceptArrangedConnection,
   (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData),
   (requestId, internalAddress, connectionData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, s2mRejectArrangedConnection,
   (U32 requestId, ByteBufferPtr rejectData),
   (requestId, rejectData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cArrangedConnectionAccepted,
   (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData),
   (requestId, possibleAddresses, connectionData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cArrangedConnectionRejected,
   (U32 requestId, ByteBufferPtr rejectData),
   (requestId, rejectData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mUpdateServerStatus, 
   (StringTableEntry levelName, StringTableEntry levelType, U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags),
   (levelName, levelType, botCount, playerCount, maxPlayers, infoFlags),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSetMOTD, 
   (TNL::StringPtr masterName, TNL::StringPtr motdString), 
   (masterName, motdString),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}


// Out-of-game chat relay functions:
TNL_IMPLEMENT_RPC(MasterServerInterface, c2mSendChat, 
   (TNL::StringPtr message), 
   (message),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSendChat, 
   (StringTableEntry playerNick, bool isPrivate, TNL::StringPtr message), 
   (playerNick, isPrivate, message),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}


// For managing list of players in global chat
TNL_IMPLEMENT_RPC(MasterServerInterface, c2mJoinGlobalChat, 
   (), 
   (),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mLeaveGlobalChat, 
   (), 
   (),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cPlayerJoinedGlobalChat, 
   (StringTableEntry playerNick), 
   (playerNick),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cPlayersInGlobalChat, 
   (Vector<StringTableEntry> playerNicks), 
   (playerNicks),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cPlayerLeftGlobalChat, 
   (StringTableEntry playerNick), 
   (playerNick),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}


// Implement a need-to-updrade verification service, without breaking older clients, by updgrading the version 1
// All clients that implement only 0-verison events will ignore this.  In theory.
TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSendUpdgradeStatus, 
   (bool needToUpgrade), 
   (needToUpgrade),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}


////////////////////////////////////
// Statistics related


// With the use of RPCGuaranteedOrderedBigData for 016, a much bigger data can be sent
TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendStatistics, 
   (Zap::VersionedGameStats stats), 
   (stats),
   NetClassGroupMasterMask, RPCGuaranteedOrderedBigData, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mAcheivementAchieved, 
   (U8 achievementId, StringTableEntry playerNick), 
   (achievementId, playerNick),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirClientToServer, 1) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendLevelInfo, 
   (string hash, string levelName, string creator, 
    StringTableEntry gametype, bool hasLevelGen, U8 teamCount, S32 winningScore, S32 gameDurationInSeconds), 
   (hash, levelName, creator, gametype, hasLevelGen, teamCount, winningScore, gameDurationInSeconds),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirClientToServer, 1) {}


// Send high score stats to client
TNL_IMPLEMENT_RPC(MasterServerInterface, c2mRequestHighScores, 
   (), 
   (),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirClientToServer, 1) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSendHighScores, 
   (Vector<StringTableEntry> groupNames, Vector<string> names, Vector<string> scores),
   (groupNames, names, scores),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, 1) {}


////////////////////////////////////
// Authentication RPCs

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSetAuthenticated, 
   (RangedU32<0, AuthenticationStatusCount> authStatus, Int<BADGE_COUNT> badges, StringPtr correctedName), 
   (authStatus, badges, correctedName),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mRequestAuthentication, 
   (Vector<U8> id, StringTableEntry name), 
   (id, name),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2sSetAuthenticated, 
   (Vector<U8> id, StringTableEntry name, RangedU32<0,AuthenticationStatusCount> status, Int<BADGE_COUNT> badges), 
   (id, name, status, badges),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mChangeName, 
   (StringTableEntry name), 
   (name),
   NetClassGroupMasterMask, RPCGuaranteedOrderedBigData, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mServerDescription,
   (StringTableEntry descr), 
   (descr),
   NetClassGroupMasterMask, RPCGuaranteedOrderedBigData, RPCDirClientToServer, 0) {}
