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
#include "../zap/ChatCheck.h"


class GameConnectRequest;  // at the botton of master.h
class DatabaseWriter;

namespace Zap {
   struct VersionedGameStats;  // gameStats.h
   struct GameStats;
}

class MasterServerConnection;
struct HighScores
{
    Vector<StringTableEntry> groupNames;
    Vector<string> names;
    Vector<string> scores;
    S32 scoresPerGroup;

    bool isValid;
    bool isBuzy;  // for multithreading
    U32 lastClock; // High scores can get old
    Vector<SafePtr<MasterServerConnection> > waitingClients;

    HighScores() { isValid = false; }
};


class MasterServerConnection : public MasterServerInterface, public Zap::ChatCheck
{
private:
   typedef MasterServerInterface Parent;

	string mLoggingStatus;

public:
   static HighScores highScores;    // Cached high scores

private:
   Int<BADGE_COUNT> mBadges;
   Int<BADGE_COUNT> getBadges();

   void sendMotd();

   MasterConnectionType mConnectionType;

public:
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
public:
   static MasterServerConnection             gServerList;      // List of servers we know about
   static MasterServerConnection             gClientList;      // List of clients who are connected
   static Vector<GameConnectRequest *>       gConnectList;

   /// @}


   /// @name Connection Info
   ///
   /// General information about this connection.
   ///
   /// @{

   ///
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

   U32              mCMProtocolVersion; ///< Version of the protocol we'll be using to converse with the client
   U32              mCSProtocolVersion; ///< Protocol version client will use to talk to server (client can only play with others 
                                        ///     using this same version)
   U32              mClientBuild;       ///< Build number of the client (different builds may use same protocols)

   U32              mInfoFlags;         ///< Info flags describing this server.
   U32              mPlayerCount;       ///< Current number of players on this server.
   U32              mMaxPlayers;        ///< Maximum number of players on this server.
   U32              mNumBots;           ///< Current number of bots on this server.

   StringTableEntry mLevelName;        ///<<=== TODO: Change to const char *
   StringTableEntry mLevelType;
   StringTableEntry mPlayerOrServerName;       ///< Player's nickname, hopefully unique, but not enforced, or server's name
   Nonce mPlayerId;                            ///< (Hopefully) unique ID of this player

   bool mAuthenticated;                        ///< True if user was authenticated, false if not
   bool mIsDebugClient;                        ///< True if client is running from a debug build

   StringTableEntry mServerDescr;              ///< Server description
   bool isInGlobalChat;


   bool mIsMasterAdmin;

   bool mIsIgnoredFromList;

   StringTableEntry mAutoDetectStr;             // Player's joystick autodetect string, for research purposes

   /// @}

public:
   static Vector<SafePtr<MasterServerConnection> >       gLeaveChatTimerList;
   U32 mLeaveGlobalChatTimer;
   bool mChatTooFast;

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
   static PHPBB3AuthenticationStatus verifyCredentials(string &username, string password);

   PHPBB3AuthenticationStatus checkAuthentication(const char *password, bool doNotDelay = false);
   void processAutentication(StringTableEntry newName, PHPBB3AuthenticationStatus status, TNL::Int<32> badges);

   // Client has contacted us and requested a list of active servers
   // that match their criteria.
   //
   // The query server method builds a piecewise list of servers
   // that match the client's particular filter criteria and
   // sends it to the client, followed by a QueryServersDone RPC.
   TNL_DECLARE_RPC_OVERRIDE(c2mQueryServers, (U32 queryId) );

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


   // s2mRejectArrangedConnection notifies the Master Server that the server is rejecting the arranged connection
   // request specified by the requestId.  The rejectData will be passed along to the requesting client.
   // Called to indicate a connect request is being rejected.
   TNL_DECLARE_RPC_OVERRIDE(s2mRejectArrangedConnection, (U32 requestId, ByteBufferPtr rejectData));

   // s2mUpdateServerStatus updates the status of a server to the Master Server, specifying the current game
   // and mission types, any player counts and the current info flags.
   // Updates the master with the current status of a game server.
   TNL_DECLARE_RPC_OVERRIDE(s2mUpdateServerStatus, (StringTableEntry levelName, StringTableEntry levelType,
                                                    U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags));

   void processIsAuthenticated(Zap::GameStats *gameStats);
   void writeStatisticsToDb(Zap::VersionedGameStats &stats);
   void writeAchievementToDb(U8 achievementId, const StringTableEntry &playerNick);
   void writeLevelInfoToDb(const string &hash, const string &levelName, const string &creator, 
                           const StringTableEntry &gameType, bool hasLevelGen, U8 teamCount, S32 winningScore, S32 gameDurationInSeconds);


   HighScores *getHighScores(S32 scoresPerGroup);

   TNL_DECLARE_RPC_OVERRIDE(s2mSendStatistics, (Zap::VersionedGameStats stats));
   TNL_DECLARE_RPC_OVERRIDE(s2mAcheivementAchieved, (U8 achievementId, StringTableEntry playerNick));
   TNL_DECLARE_RPC_OVERRIDE(s2mSendLevelInfo, (string hash, string levelName, string creator, 
                                               StringTableEntry gametype, bool hasLevelGen, U8 teamCount, S32 winningScore, S32 gameDurationInSeconds));

   // Send message-of-the-day
   TNL_DECLARE_RPC_OVERRIDE(c2mRequestMOTD, ());

   // Send high scores stats to client
   TNL_DECLARE_RPC_OVERRIDE(c2mRequestHighScores, ());

   // Game server wants to know if user name has been verified
   TNL_DECLARE_RPC_OVERRIDE(s2mRequestAuthentication, (Vector<U8> id, StringTableEntry name));

   static string cleanName(string name);
   bool readConnectRequest(BitStream *bstream, NetConnection::TerminationReason &reason);

   TNL_DECLARE_RPC_OVERRIDE(c2mJoinGlobalChat, ());
   TNL_DECLARE_RPC_OVERRIDE(c2mLeaveGlobalChat, ());

   TNL_DECLARE_RPC_OVERRIDE(c2mSendChat, (StringPtr message));

   // Got out-of-game chat message from client, need to relay it to others
   TNL_DECLARE_RPC_OVERRIDE(s2mChangeName, (StringTableEntry name));
   TNL_DECLARE_RPC_OVERRIDE(s2mServerDescription, (StringTableEntry descr));

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


