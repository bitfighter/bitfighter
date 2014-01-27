//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "masterInterface.h"
#include "../zap/gameStats.h"

using namespace TNL;

namespace Master 
{

// Since this is an interface, we implement a bunch of stubs.  These will be overridden on the
// client, server, or master end as needed.  This interface will be compiled into both the
// master and the individual clients.

//
//   IMPORTANT NOTE!!!
//
//   When adding new RPCs that did not exist in previous versions of the game, make sure to give them a new
//   rpcVersion number, otherwise clients will not be able to connect!
//

// Note that these may not be the historically correct version numbers... feel free to correct them if you 
// want to do the archaeology!
static const S32 M_RPC_PRE_017 = 0;
static const S32 M_RPC_017  = 1;
static const S32 M_RPC_018  = 2;
static const S32 M_RPC_019  = 3;
static const S32 M_RPC_019a = 4;

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mQueryServers,
   (U32 queryId), (queryId),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cQueryServersResponse,
   (U32 queryId, Vector<IPAddress> ipList), (queryId, ipList),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cQueryServersResponse_019a,
   (U32 queryId, Vector<IPAddress> ipList, Vector<S32> serverIdList), (queryId, ipList, serverIdList),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, M_RPC_019a) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mRequestArrangedConnection, 
   (U32 requestId, IPAddress remoteAddress, IPAddress internalAddress, ByteBufferPtr connectionParameters),
   (requestId, remoteAddress, internalAddress, connectionParameters),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2sClientRequestedArrangedConnection,
   (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionParameters),
   (requestId, possibleAddresses, connectionParameters),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mAcceptArrangedConnection,
   (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData),
   (requestId, internalAddress, connectionData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, M_RPC_PRE_017) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, s2mRejectArrangedConnection,
   (U32 requestId, ByteBufferPtr rejectData),
   (requestId, rejectData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cArrangedConnectionAccepted,
   (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData),
   (requestId, possibleAddresses, connectionData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cArrangedConnectionRejected,
   (U32 requestId, ByteBufferPtr rejectData),
   (requestId, rejectData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mUpdateServerStatus, 
   (StringTableEntry levelName, StringTableEntry levelType, U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags),
   (levelName, levelType, botCount, playerCount, maxPlayers, infoFlags),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, M_RPC_PRE_017) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, c2mRequestMOTD,
      (),
      (),
      NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, M_RPC_018) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSetMOTD, 
   (TNL::StringPtr masterName, TNL::StringPtr motdString), 
   (masterName, motdString),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, M_RPC_PRE_017) {}


// Out-of-game chat relay functions:
TNL_IMPLEMENT_RPC(MasterServerInterface, c2mSendChat, 
   (TNL::StringPtr message), 
   (message),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSendChat, 
   (StringTableEntry playerNick, bool isPrivate, TNL::StringPtr message), 
   (playerNick, isPrivate, message),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, M_RPC_PRE_017) {}


// For managing list of players in global chat
TNL_IMPLEMENT_RPC(MasterServerInterface, c2mJoinGlobalChat, 
   (), 
   (),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mLeaveGlobalChat, 
   (), 
   (),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cPlayerJoinedGlobalChat, 
   (StringTableEntry playerNick), 
   (playerNick),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cPlayersInGlobalChat, 
   (Vector<StringTableEntry> playerNicks), 
   (playerNicks),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cPlayerLeftGlobalChat, 
   (StringTableEntry playerNick), 
   (playerNick),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, M_RPC_PRE_017) {}


// Implement a need-to-updrade verification service, without breaking older clients, by updgrading the version 1
// All clients that implement only 0-verison events will ignore this.  In theory.
TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSendUpdgradeStatus, 
   (bool needToUpgrade), 
   (needToUpgrade),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, M_RPC_PRE_017) {}


////////////////////////////////////
// Statistics related


// With the use of RPCGuaranteedOrderedBigData for 016, a much bigger data can be sent
TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendStatistics, 
   (Zap::VersionedGameStats stats), 
   (stats),
   NetClassGroupMasterMask, RPCGuaranteedOrderedBigData, RPCDirClientToServer, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mAcheivementAchieved, 
   (U8 achievementId, StringTableEntry playerNick), 
   (achievementId, playerNick),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirClientToServer, M_RPC_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendLevelInfo, 
   (string hash, string levelName, string creator, 
    StringTableEntry gametype, bool hasLevelGen, U8 teamCount, S32 winningScore, S32 gameDurationInSeconds), 
   (hash, levelName, creator, gametype, hasLevelGen, teamCount, winningScore, gameDurationInSeconds),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirClientToServer, M_RPC_017) {}


// Send high score stats to client
TNL_IMPLEMENT_RPC(MasterServerInterface, c2mRequestHighScores, 
                  (), 
                  (),
                  NetClassGroupMasterMask, RPCGuaranteed, RPCDirClientToServer, M_RPC_017) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSendHighScores, 
                  (Vector<StringTableEntry> groupNames, Vector<string> names, Vector<string> scores),
                  (groupNames, names, scores),
                  NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, M_RPC_017) {}


// Level rating related
TNL_IMPLEMENT_RPC(MasterServerInterface, c2mSetLevelRating,
                  (U32 databaseId, RangedU32<0, 2> rating),
                  (databaseId, rating),
                  NetClassGroupMasterMask, RPCGuaranteed, RPCDirClientToServer, M_RPC_019) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mRequestLevelRating,
                  (U32 databaseId), 
                  (databaseId),
                  NetClassGroupMasterMask, RPCGuaranteed, RPCDirClientToServer, M_RPC_019) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSendPlayerLevelRating,
                  (U32 databaseId, RangedU32<0, 2> rating),
                  (databaseId, rating),
                  NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, M_RPC_019) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSendTotalLevelRating,
                  (U32 databaseId, S16 rating),
                  (databaseId, rating),
                  NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, M_RPC_019) {}


////////////////////////////////////
// Authentication RPCs

// 018a version
TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSetAuthenticated, 
   (RangedU32<0, AuthenticationStatusCount> authStatus, Int<BADGE_COUNT> badges, StringPtr correctedName), 
   (authStatus, badges, correctedName),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, M_RPC_PRE_017) {}

// 019 version
TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSetAuthenticated_019, 
   (RangedU32<0, AuthenticationStatusCount> authStatus, Int<BADGE_COUNT> badges, U16 gamesPlayed, StringPtr correctedName), 
   (authStatus, badges, gamesPlayed, correctedName),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, M_RPC_019) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mRequestAuthentication, 
   (Vector<U8> id, StringTableEntry name), 
   (id, name),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirClientToServer, M_RPC_PRE_017) {}

// 018a version
TNL_IMPLEMENT_RPC(MasterServerInterface, m2sSetAuthenticated, 
   (Vector<U8> id, StringTableEntry name, RangedU32<0,AuthenticationStatusCount> status, Int<BADGE_COUNT> badges), 
   (id, name, status, badges),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, M_RPC_PRE_017) {}

// 019 version
TNL_IMPLEMENT_RPC(MasterServerInterface, m2sSetAuthenticated_019, 
   (Vector<U8> id, StringTableEntry name, RangedU32<0,AuthenticationStatusCount> status, Int<BADGE_COUNT> badges, U16 gamesPlayed), 
   (id, name, status, badges, gamesPlayed),
   NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, M_RPC_019) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mChangeName, 
   (StringTableEntry name), 
   (name),
   NetClassGroupMasterMask, RPCGuaranteedOrderedBigData, RPCDirClientToServer, M_RPC_PRE_017) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mServerDescription,
   (StringTableEntry descr), 
   (descr),
   NetClassGroupMasterMask, RPCGuaranteedOrderedBigData, RPCDirClientToServer, M_RPC_PRE_017) {}


}