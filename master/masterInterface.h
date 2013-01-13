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


#ifndef _MASTERINTERFACE_H_
#define _MASTERINTERFACE_H_

#include "../zap/gameStats.h"    // Must be included before tnlRPC.h to compile in gcc
#include "../zap/SharedConstants.h"
#include "tnlEventConnection.h"
#include "tnlRPC.h"

using namespace TNL;


// Useful string constants...
static const char USED_EXTERNAL *MasterNoSuchHost = "No Such Host";
static const char USED_EXTERNAL *MasterRequestTimedOut = "Timed Out";

const S32 IP_MESSAGE_ADDRESS_COUNT = 30;

/// The MasterServerInterface is the RPC interface to the TNL example Master Server.
/// The default Master Server tracks a list of public servers and allows clients
/// to query for them based on different filter criteria, including maximum number of players,
/// region codes, game or mission type, and others.
///
/// When a client wants to initiate a connection with a server listed by the master, it
/// can ask the Master Server to arranage a connection.  The masterclient example that
/// ships with TNL demonstrates a client/server console application that uses the Master
/// Server to arrange connections between a client and a server instance.
///
/// Client/Server programs using the Master Server for server listing should create
/// a subclass of MasterServerInterface named "MasterServerConnection", and override
/// all of the RPC methods that begin with m2c, as they signify master to client messages.
/// RPC methods can be overridden with the TNL_DECLARE_RPC_OVERRIDE and TNL_IMPLEMENT_RPC_OVERRIDE methods.

// Notes from CE: To create a new master-client RPC, first declare that event in
// masterInterface.h, and create a stub in masterInterface.cpp.  Since these files
// are compiled into both the master server and individual clients, this will create
// a consistent function declaration in each program.
//
// If the RPC is to be executed on the master, then use the TNL_DECLARE_RPC_OVERRIDE
// macro in main.cpp in the master folder to override the stub and provide the
// actual code to be run.  Do not use TNL_IMPLEMENT_RPC_OVERRIDE.
//
// If the RPC will be executed on the client, then declare the
// TNL_DECLARE_RPC_OVERRIDE macro to declare the function in masterConnection.h, and
// use the TNL_IMPLEMENT_RPC_OVERRIDE macro to write the actual code in
// masterConnection.cpp.



//struct VersionedGameStats;

class MasterServerInterface : public EventConnection
{
public:
   // Version 0 RPCs

   /// c2mQueryServers is sent by the client to the master server to request a list of
   /// servers that match the specified filter criteria (most of which were removed for 016).  A c2mQueryServers request will
   /// result in one or more m2cQueryServersResponse RPCs, with the final call having an empty
   /// Vector of servers.
   TNL_DECLARE_RPC(c2mQueryServers, (U32 queryId));

   /// m2cQueryServersResponse is sent by the master server in response to a c2mQueryServers RPC, to
   /// return a partial list of the servers that matched the specified filter criteria.  Because packets
   /// are limited in size, the response server list is broken up into lists of at most IP_MESSAGE_ADDRESS_COUNT IP addresses
   /// per message.  The Master Server will always send a final, empty m2cQueryServersResponse to signify that the list
   /// is complete.
   TNL_DECLARE_RPC(m2cQueryServersResponse, (U32 queryId, Vector<IPAddress> ipList));

   /// c2mRequestArrangedConnection is an RPC sent from the client to the master to request an arranged
   /// connection with the specified server address.  The internalAddress should be the client's own self-reported
   /// IP address.  The connectionParameters buffer will be sent without modification to the specified
   /// server.
   TNL_DECLARE_RPC(c2mRequestArrangedConnection, (U32 requestId,
      IPAddress remoteAddress, IPAddress internalAddress,
      ByteBufferPtr connectionParameters));

   /// m2sClientRequestedArranged connection is sent from the master to a server to notify it that
   /// a client has requested a connection.  The possibleAddresses vector is a list of possible IP addresses
   /// that the server should attempt to connect to for that client if it accepts the connection request.
   TNL_DECLARE_RPC(m2sClientRequestedArrangedConnection, (U32 requestId, Vector<IPAddress> possibleAddresses,
      ByteBufferPtr connectionParameters));
            
   /// s2mAcceptArrangedConnection is sent by a server to notify the master that it will accept the connection
   /// request from a client.  The requestId parameter sent by the MasterServer in m2sClientRequestedArrangedConnection
   /// should be sent back as the requestId field.  The internalAddress is the server's self-determined IP address.
   TNL_DECLARE_RPC(s2mAcceptArrangedConnection, (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData));

   /// s2mRejectArrangedConnection notifies the Master Server that the server is rejecting the arranged connection
   /// request specified by the requestId.  The rejectData will be passed along to the requesting client.
   TNL_DECLARE_RPC(s2mRejectArrangedConnection, (U32 requestId, ByteBufferPtr rejectData));


   /// m2cArrangedConnectionAccepted is sent to a client that has previously requested a connection to a listed server
   /// via c2mRequestArrangedConnection if the server accepted the connection.  The possibleAddresses vector is the list
   /// of IP addresses the client should attempt to connect to, and the connectionData buffer is the buffer sent by the
   /// server upon accepting the connection.
   TNL_DECLARE_RPC(m2cArrangedConnectionAccepted, (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData));

   /// m2cArrangedConnectionRejected is sent to a client when an arranged connection request is rejected by the
   /// server, or when the request times out because the server never responded.
   TNL_DECLARE_RPC(m2cArrangedConnectionRejected, (U32 requestId, ByteBufferPtr rejectData));

   /// s2mUpdateServerStatus updates the status of a server to the Master Server, specifying the current game
   /// and mission types, any player counts and the current info flags.
   TNL_DECLARE_RPC(s2mUpdateServerStatus, (
      StringTableEntry levelName, StringTableEntry levelType,
      U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags));

   /// Request the Message-of-the-Day
   TNL_DECLARE_RPC(c2mRequestMOTD, ());

   /// m2cSetMOTD is sent to a client when the connection is established.  The
   /// client's BUILD_VERSION is used to pick which MOTD will be sent.
   TNL_DECLARE_RPC(m2cSetMOTD, (TNL::StringPtr masterName, TNL::StringPtr motdString));

   // If the user has a verified name, send this message back so they know the "correct spelling" of the name
   TNL_DECLARE_RPC(m2cSetAuthenticated, (RangedU32<0, AuthenticationStatusCount> authStatus, Int<BADGE_COUNT> badges, StringPtr correctedName));


   // c2mSendChat sends an out-of-game chat message from the client to the master server
   // where it is relayed to other "out-of-game" clients
   TNL_DECLARE_RPC(c2mSendChat, (TNL::StringPtr message));

   // m2cSendChat relays a chat message back to any clients connected to master
   TNL_DECLARE_RPC(m2cSendChat, (StringTableEntry playerNick, bool isPrivate, TNL::StringPtr message));



   /// m2cSendUpdgradeStatus is sent to a client when the connection is established.  If
   /// needToUpgrade is true, an alert will be displayed to the user that they should get new version.
   /// Implemented as separate method from m2cSetMOTD in order to retain backwards compatibility
   TNL_DECLARE_RPC(m2cSendUpdgradeStatus, (bool needToUpgrade));

   // For managing players who are chatting on global chat
   TNL_DECLARE_RPC(c2mJoinGlobalChat, () );
   TNL_DECLARE_RPC(c2mLeaveGlobalChat, () );

   TNL_DECLARE_RPC(m2cPlayerJoinedGlobalChat, (StringTableEntry playerNick) );
   TNL_DECLARE_RPC(m2cPlayerLeftGlobalChat, (StringTableEntry playerNick) );
   TNL_DECLARE_RPC(m2cPlayersInGlobalChat, (Vector<StringTableEntry> playerNicks));


   TNL_DECLARE_RPC(s2mRequestAuthentication, (Vector<U8> id, StringTableEntry name));
   TNL_DECLARE_RPC(m2sSetAuthenticated, (Vector<U8> id, StringTableEntry name, 
                                         RangedU32<0,AuthenticationStatusCount> status, Int<BADGE_COUNT> badges));

   TNL_DECLARE_RPC(s2mSendStatistics, (Zap::VersionedGameStats stats));
   TNL_DECLARE_RPC(s2mAcheivementAchieved, (U8 achievementId, StringTableEntry playerNick));
   TNL_DECLARE_RPC(s2mSendLevelInfo, (string hash, string levelName, string creator, 
                                      StringTableEntry gametype, bool hasLevelGen, U8 teamCount, S32 winningScore, S32 gameDurationInSeconds));


   TNL_DECLARE_RPC(c2mRequestHighScores, ());
   TNL_DECLARE_RPC(m2cSendHighScores, (Vector<StringTableEntry> groupNames, Vector<string> names, Vector<string> scores));


   TNL_DECLARE_RPC(s2mChangeName, (StringTableEntry name));         // when server changes name using /setservname
   TNL_DECLARE_RPC(s2mServerDescription, (StringTableEntry descr)); // when server changes using /setservdescr

};


#endif
