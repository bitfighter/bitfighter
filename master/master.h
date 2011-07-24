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

#include "tnlNetInterface.h"
#include "tnlVector.h"



class GameConnectRequest;  // at the botton of master.h

namespace Zap {
   struct VersionedGameStats;  // gameStats.h
   struct GameStats;
}

class MasterServerConnection : public MasterServerInterface
{
private:
   typedef MasterServerInterface Parent;

protected:

   /// @name Linked List
   ///
   /// The server stores its connections on a linked list.
   ///
   /// @{

   ///
   MasterServerConnection *mNext;
   MasterServerConnection *mPrev;

   /// @}

   /// @name Globals
   /// @{

   ///
   static MasterServerConnection             gServerList;      // List of servers we know about
   static MasterServerConnection             gClientList;      // List of clients who are connected
public:
   static Vector<GameConnectRequest *>       gConnectList;
protected:

   /// @}


   /// @name Connection Info
   ///
   /// General information about this connection.
   ///
   /// @{

   ///
   bool             mIsGameServer;     ///< True if this is a game server.
   U32              mStrikeCount;      ///< Number of "strikes" this connection has... 3 strikes and you're out!
   U32              mLastQueryId;      ///< The last query id for info from this master.
   U32              mLastActivityTime; ///< The last time we got a request or an update from this host.

   /// A list of connection requests we're working on fulfilling for this connection.
   Vector< GameConnectRequest* > mConnectList;

   /// @}

   /// @name Server Info
   ///
   /// This info is filled in if this connection maps to a
   /// game server.
   ///
   /// @{

   U32              mRegionCode;        ///< The region code in which this server operates.
   StringTableEntry mVersionString;     ///< The unique version string for this server or client
                                        ///< Only used in version 0 protocol.  In subsequent versions, the Version vars
                                        ///< below will hold actual version numbers, and this var will hold a "+".

   U32              mCMProtocolVersion; ///< Version of the protocol we'll be using to converse with the client
   U32              mCSProtocolVersion; ///< Protocol version client will use to talk to server (client can only play with others 
                                        ///     using this same version)
   U32              mClientBuild;       ///< Build number of the client (different builds may use same protocols)

   // The following are mostly dummy values at the moment... we may use them later.
   U32              mCPUSpeed;          ///< The CPU speed of this server.
   U32              mInfoFlags;         ///< Info flags describing this server.
   U32              mPlayerCount;       ///< Current number of players on this server.
   U32              mMaxPlayers;        ///< Maximum number of players on this server.
   U32              mNumBots;           ///< Current number of bots on this server.

   StringTableEntry mLevelName;
   StringTableEntry mLevelType;
   StringTableEntry mPlayerOrServerName;       ///< Player's nickname, hopefully unique, but not enforced, or server's name
   Nonce mPlayerId;                            ///< (Hopefully) unique ID of this player

   bool mAuthenticated;                        ///< True if user was authenticated, false if not
   StringTableEntry mServerDescr;              ///< Server description
   bool isInGlobalChat;

   StringTableEntry mAutoDetectStr;             // Player's joystick autodetect string, for research purposes

   /// @}

public:

   /// Constructor initializes the linked list info with "safe" values
   /// so we don't explode if we destruct right away.
   MasterServerConnection();

   /// Destructor removes the connection from the doubly linked list of server connections
   ~MasterServerConnection();

   /// Adds this connection to the doubly linked list of servers
   void linkToServerList();


   void linkToClientList();

   enum PHPBB3AuthenticationStatus {
      Authenticated,
      CantConnect,
      UnknownUser,
      WrongPassword,
      InvalidUsername,
      Unsupported,
      UnknownStatus
   };

   // Check username & password against database
   PHPBB3AuthenticationStatus verifyCredentials(string &username, string password);

   // Client has contacted us and requested a list of active servers
   // that match their criteria.
   //
   // The query server method builds a piecewise list of servers
   // that match the client's particular filter criteria and
   // sends it to the client, followed by a QueryServersDone RPC.
   TNL_DECLARE_RPC_OVERRIDE(c2mQueryServers,
                (U32 queryId, U32 regionMask, U32 minPlayers, U32 maxPlayers,
                 U32 infoFlags, U32 maxBots, U32 minCPUSpeed,
                 StringTableEntry gameType, StringTableEntry missionType)
   );

   /// checkActivityTime validates that this particular connection is
   /// not issuing too many requests at once in an attempt to DOS
   /// by flooding either the master server or any other server
   /// connected to it.  A client whose last activity time falls
   /// within the specified delta gets a strike... 3 strikes and
   /// you're out!  Strikes go away after being good for a while.
   bool checkActivityTime(U32 timeDeltaMinimum);

   void removeConnectRequest(GameConnectRequest *gcr);

   GameConnectRequest *findAndRemoveRequest(U32 requestId);


   MasterServerConnection *findClient(Nonce &clientId);   // Should be const, but that won't compile for reasons not yet determined!!


   // Write a current count of clients/servers for display on a website, using JSON format
   // This gets updated whenver we gain or lose a server, at most every REWRITE_TIME ms
   static void writeClientServerList_JSON();

   bool isAuthenticated() { return mAuthenticated; }

   // This is called when a client wishes to arrange a connection with a server
   TNL_DECLARE_RPC_OVERRIDE(c2mRequestArrangedConnection, (U32 requestId, IPAddress remoteAddress, IPAddress internalAddress,
                                                           ByteBufferPtr connectionParameters) );

   /// s2mAcceptArrangedConnection is sent by a server to notify the master that it will accept the connection
   /// request from a client.  The requestId parameter sent by the MasterServer in m2sClientRequestedArrangedConnection
   /// should be sent back as the requestId field.  The internalAddress is the server's self-determined IP address.

   // Called to indicate a connect request is being accepted.
   TNL_DECLARE_RPC_OVERRIDE(s2mAcceptArrangedConnection, (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData));

   // TODO: Delete after 014
   TNL_DECLARE_RPC_OVERRIDE(c2mAcceptArrangedConnection, (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData))
      { s2mAcceptArrangedConnection_remote(requestId, internalAddress, connectionData); } // for now, redirect to proper function


   // TODO: Delete after 014 -- replaced with identical s2mRejectArrangedConnection
   TNL_DECLARE_RPC_OVERRIDE(c2mRejectArrangedConnection, (U32 requestId, ByteBufferPtr rejectData))
      { s2mRejectArrangedConnection_remote(requestId, rejectData); }

   // s2mRejectArrangedConnection notifies the Master Server that the server is rejecting the arranged connection
   // request specified by the requestId.  The rejectData will be passed along to the requesting client.
   // Called to indicate a connect request is being rejected.
   TNL_DECLARE_RPC_OVERRIDE(s2mRejectArrangedConnection, (U32 requestId, ByteBufferPtr rejectData));

   // TODO: Delete after 014 -- replaced with identical s2mUpdateServerStatus
   TNL_DECLARE_RPC_OVERRIDE(c2mUpdateServerStatus, (
      StringTableEntry levelName, StringTableEntry levelType,
      U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags))
      { s2mUpdateServerStatus_remote(levelName, levelType, botCount, playerCount, maxPlayers, infoFlags); }

   // s2mUpdateServerStatus updates the status of a server to the Master Server, specifying the current game
   // and mission types, any player counts and the current info flags.
   // Updates the master with the current status of a game server.
   TNL_DECLARE_RPC_OVERRIDE(s2mUpdateServerStatus, (StringTableEntry levelName, StringTableEntry levelType,
                                                    U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags));


   // Send player statistics to the master server     ==> deprecated
   TNL_DECLARE_RPC_OVERRIDE(s2mSendPlayerStatistics, (StringTableEntry playerName, 
                                                      U16 kills, U16 deaths, U16 suicides, Vector<U16> shots, Vector<U16> hits));

   // Send player statistics to the master server     ==> deprecated as of 015
   TNL_DECLARE_RPC_OVERRIDE(s2mSendPlayerStatistics_2, (StringTableEntry playerName, StringTableEntry teamName, 
                                                        U16 kills, U16 deaths, U16 suicides, Vector<U16> shots, Vector<U16> hits));


   //// Send player statistics to the master server
   //TNL_DECLARE_RPC_OVERRIDE(s2mSendPlayerStatistics_3, (StringTableEntry playerName, Vector<U8> id, bool isBot, 
   //                                                     StringTableEntry teamName, S32 score,
   //                                                     U16 kills, U16 deaths, U16 suicides, Vector<U16> shots, Vector<U16> hits));

   // Send game statistics to the master server  ==> Deprecated
   TNL_DECLARE_RPC_OVERRIDE(s2mSendGameStatistics, (StringTableEntry gameType, StringTableEntry levelName, 
                                                    RangedU32<0,128> players, S16 timeInSecs));


   // Send game statistics to the master server   ==> Deprecated starting in 015
   TNL_DECLARE_RPC_OVERRIDE(s2mSendGameStatistics_2, (StringTableEntry gameType, StringTableEntry levelName, 
                                                      Vector<StringTableEntry> teams, Vector<S32> teamScores,
                                                      Vector<RangedU32<0,256> > colorR, Vector<RangedU32<0,256> > colorG, Vector<RangedU32<0,256> > colorB, 
                                                      RangedU32<0,128> players, S16 timeInSecs));

   /////////////////////////////////////////////////////////////////////////////////////




   // Send game statistics to the master server   ==> Current as of 015
   // Note that teams are sent in descending order, most players to fewest  
 //  TNL_DECLARE_RPC_OVERRIDE(s2mSendGameStatistics_3, (StringTableEntry gameType, bool teamGame, StringTableEntry levelName, 
 //                                                     Vector<StringTableEntry> teams, Vector<S32> teamScores,
 //                                                     Vector<RangedU32<0,0xFFFFFF> > color, 
 //                                                     U16 timeInSecs, Vector<StringTableEntry> playerNames, Vector<Vector<U8> > playerIds,
 //                                                     Vector<bool> isBot, Vector<bool> lastOnTeam, Vector<S32> playerScores, 
 //                                                     Vector<U16> playerKills, Vector<U16> playerDeaths, Vector<U16> playerSuicides, 
 //                                                     Vector<U16> teamSwitchCount, Vector<Vector<U16> > shots, Vector<Vector<U16> > hits));

   void processIsAuthenticated(Zap::GameStats *gameStats);
   void SaveStatistics(Zap::VersionedGameStats &stats);

   TNL_DECLARE_RPC_OVERRIDE(s2mSendStatistics, (Zap::VersionedGameStats stats));

   // Game server wants to know if user name has been verified
   TNL_DECLARE_RPC_OVERRIDE(s2mRequestAuthentication, (Vector<U8> id, StringTableEntry name));

   static string cleanName(string name);
   bool readConnectRequest(BitStream *bstream, NetConnection::TerminationReason &reason);

   TNL_DECLARE_RPC_OVERRIDE(c2mJoinGlobalChat, ());
   TNL_DECLARE_RPC_OVERRIDE(c2mLeaveGlobalChat, ());

   // Got out-of-game chat message from client, need to relay it to others
   TNL_DECLARE_RPC_OVERRIDE(c2mSendChat, (StringPtr message));
   TNL_DECLARE_NETCONNECTION(MasterServerConnection);

};

class GameConnectRequest
{
public:
   SafePtr<MasterServerConnection> initiator;
   SafePtr<MasterServerConnection> host;

   U32 initiatorQueryId;
   U32 hostQueryId;
   U32 requestTime;
};


