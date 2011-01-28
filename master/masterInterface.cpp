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
//namespace Types
//{
//   // GameStatistics3 is in gameStats.h
//   const U8 GameStatistics3_CurrentVersion = 1;    // keep this the same U8 size.
//   /// Reads objects from a BitStream.
//   void read(TNL::BitStream &s, GameStatistics3 *val)
//   {
//      U8 version;
//      read(s, &version);    // version number, in the future, we add more things to read/write and want to keep compatible to old version 
//      //if(version > GameStatistics4_CurrentVersion) return; // can't read future version and unsupported version.
//
//      read(s, &val->gameType);
//      read(s, &val->teamGame);
//      read(s, &val->levelName);
//      read(s, &val->teams);
//      read(s, &val->teamScores);
//      read(s, &val->color);
//      read(s, &val->timeInSecs);
//      read(s, &val->playerNames);
//      read(s, &val->playerIDs);
//      read(s, &val->isBot);
//      read(s, &val->lastOnTeam);
//      read(s, &val->playerScores);
//      read(s, &val->playerKills);
//      read(s, &val->playerDeaths);
//      read(s, &val->playerSuicides);
//      read(s, &val->shots);
//      read(s, &val->hits);
//      if(version >= 1)
//         read(s, &val->playerSwitchedTeamCount);
//      else
//         val->playerSwitchedTeamCount.setSize(val->playerNames.size());
//   }
//
//
//   /// Writes objects into a BitStream. Server write and send to master.
//   void write(TNL::BitStream &s, GameStatistics3 &val)
//   {
//      write(s, GameStatistics3_CurrentVersion);       // send current version
//      write(s, val.gameType);
//      write(s, val.teamGame);
//      write(s, val.levelName);
//      write(s, val.teams);
//      write(s, val.teamScores);
//      write(s, val.color);
//      write(s, val.timeInSecs);
//      write(s, val.playerNames);
//      write(s, val.playerIDs);
//      write(s, val.isBot);
//      write(s, val.lastOnTeam);
//      write(s, val.playerScores);
//      write(s, val.playerKills);
//      write(s, val.playerDeaths);
//      write(s, val.playerSuicides);
//      write(s, val.shots);
//      write(s, val.hits);
//
//      if(GameStatistics3_CurrentVersion >= 1)
//         write(s, val.playerSwitchedTeamCount);
//   }
//}


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


TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendPlayerStatistics_2, 
   (StringTableEntry playerName, StringTableEntry teamName, U16 kills, U16 deaths, U16 suicides, Vector<U16> shots, Vector<U16> hits),
   (playerName, teamName, kills, deaths, suicides, shots, hits),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 4) {}


//TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendPlayerStatistics_3, 
//   (StringTableEntry playerName, Vector<U8> id, bool isBot, StringTableEntry teamName, S32 score, U16 kills, U16 deaths, U16 suicides, 
//         Vector<U16> shots, Vector<U16> hits),
//   (playerName, id, isBot, teamName, score, kills, deaths, suicides, shots, hits),
//   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 6) {}


// Send game statistics to the master server
TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendGameStatistics, (StringTableEntry gameType, StringTableEntry levelName, 
                                                                 RangedU32<0,128> players, S16 time),
   (gameType, levelName, players, time),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 3) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendGameStatistics_2, (StringTableEntry gameType, StringTableEntry levelName,
                                                                   Vector<StringTableEntry> teams, Vector<S32> teamScores,
                                                                   Vector<RangedU32<0,256> > colorR, Vector<RangedU32<0,256> > colorG, 
                                                                   Vector<RangedU32<0,256> > colorB, 
                                                                   RangedU32<0,128> players, S16 time),
   (gameType, levelName, teams, teamScores, colorR, colorG, colorB, players, time),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 4) {}


TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendGameStatistics_3, (StringTableEntry gameType, bool teamGame, 
                                                                   StringTableEntry levelName,
                                                                   Vector<StringTableEntry> teams, 
                                                                   Vector<S32> teamScores, Vector<RangedU32<0,0xFFFFFF> > color, 
                                                                   U16 time, Vector<StringTableEntry> playerNames, Vector<Vector<U8> > playerIDs,
                                                                   Vector<bool> isBot, Vector<bool> lastOnTeam, Vector<S32> playerScores, 
                                                                   Vector<U16> playerKills, Vector<U16> playerDeaths, Vector<U16> playerSuicides,
                                                                   Vector<U16> teamSwitchCount, Vector<Vector<U16> > shots, Vector<Vector<U16> > hits),
   (gameType, teamGame, levelName, teams, teamScores, color, time, playerNames, playerIDs, isBot, lastOnTeam, 
    playerScores, playerKills, playerDeaths, playerSuicides, teamSwitchCount, shots, hits),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 6) {}

//TNL_IMPLEMENT_RPC(MasterServerInterface, s2mSendGameStatistics_3, (GameStatistics3 gameStats),
//   (gameStats), NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 6) {}



TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSetAuthenticated, (RangedU32<0, AuthenticationStatusCount> authStatus, 
                                                               StringPtr correctedName), 
                                                               (authStatus, correctedName),
                  NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, 5) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, s2mRequestAuthentication, (Vector<U8> id, StringTableEntry name), (id, name),
                  NetClassGroupMasterMask, RPCGuaranteed, RPCDirClientToServer, 5) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2sSetAuthenticated, (Vector<U8> id, StringTableEntry name,
                              RangedU32<0,AuthenticationStatusCount> status ), (id, name, status ),
                  NetClassGroupMasterMask, RPCGuaranteed, RPCDirServerToClient, 5) {}

