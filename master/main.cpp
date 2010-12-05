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
#include "../tnl/tnlNetInterface.h"
#include "../tnl/tnlVector.h"
#include "../tnl/tnlAsymmetricKey.h"
#include "../zap/SharedConstants.h"
//#include "../zap/stringUtils.h"   // For itos
#include "authenticator.h"   // For authenticating users against the PHPBB3 database
#include <stdio.h>
#include <string>
#include <stdarg.h>     // For va_args
#include <time.h>

using namespace TNL;
using namespace std;

NetInterface *gNetInterface = NULL;

Vector<string> MOTDTypeVecOld;
Vector<string> MOTDStringVecOld;

Vector<U32> MOTDVersionVec;
Vector<string> MOTDStringVec;
U32 gLatestReleasedCSProtocol = 0; // Will be updated with value from cfg file

string gMasterName;                // Name of the master server
string gJasonOutFile;              // File where JSON data gets dumped
bool gNeedToWriteStatus = true;    // Tracks whether we need to update our status file, for possible display on a website

// Variables for managing access to MySQL
string gMySqlAddress;
string gDbUsername;
string gDbPassword;

U32 gServerStartTime;

// Variables for verifying usernames/passwords in PHPBB3
string gPhpbb3Database;
string gPhpbb3TablePrefix;



class MasterServerConnection;

class GameConnectRequest
{
public:
   SafePtr<MasterServerConnection> initiator;
   SafePtr<MasterServerConnection> host;

   U32 initiatorQueryId;
   U32 hostQueryId;
   U32 requestTime;
};

// TODO: Get this from stringUtils... doesn't work for some reason.  Too tired to track it down at the moment...
std::string itos(S32 i)
{
   char outString[100];
   dSprintf(outString, sizeof(outString), "%d", i);
   return outString;
}


static bool isControlCharacter(char ch)
   {
      return ch > 0 && ch <= 0x1F;
   }

static bool containsControlCharacter( const char* str )
{
   while ( *str )
   {
      if ( isControlCharacter( *(str++) ) )
         return true;
   }
   return false;
}


// Sanitize strings before inclusion into JSON
static const char *sanitizeForJson(const char *value)
{
   unsigned maxsize = strlen(value)*2 + 3; // allescaped+quotes+NULL
   std::string result;
   result.reserve(maxsize);  // memory management

   // Return if no escaping needed
   if (strpbrk(value, "\"\\\b\f\n\r\t<>&") == NULL && !containsControlCharacter(value))
   {
      return value;
   }

   // If any of the above exist then do some escaping
   for (const char* c=value; *c != 0; ++c)
   {
      switch(*c)
      {
      // For JSON
      case '\"':
         result += "\\\"";
         break;
      case '\\':
         result += "\\\\";
         break;
      case '\b':
         result += "\\b";
         break;
      case '\f':
         result += "\\f";
         break;
      case '\n':
         result += "\\n";
         break;
      case '\r':
         result += "\\r";
         break;
      case '\t':
         result += "\\t";
         break;

      // For html markup entities
      case '&':
         result += "&amp;";
         break;
      case '<':
         result += "&lt;";
         break;
      case '>':
         result += "&gt;";
         break;
      default:
         if ( isControlCharacter( *c ) )
         {
            // Do nothing for the moment -- there shouldn't be any control chars here, and if there are we don't really care.
            // However, some day we might want to support this, so we'll leave the code in place.
            //std::ostringstream oss;
            //oss << "\\u" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << static_cast<int>(*c);
            //result += oss.str();
         }
         else
         {
            result += *c;
         }
         break;
      }
   }

   return result.c_str();
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

   static Vector<GameConnectRequest *>       gConnectList;

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

   /// Constructor initializes the linked list info with
   /// "safe" values so we don't explode if we destruct
   /// right away.
   MasterServerConnection()
   {
      mStrikeCount = 0; 
      mLastActivityTime = 0;
      mNext = this;
      mPrev = this;
      setIsConnectionToClient();
      setIsAdaptive();
      isInGlobalChat = false;
      mAuthenticated = false;
   }

   /// Destructor removes the connection from the doubly linked list of
   /// server connections.
   ~MasterServerConnection()
   {
      // unlink it if it's in the list
      mPrev->mNext = mNext;
      mNext->mPrev = mPrev;

      if(mIsGameServer)
      {
         // SERVER_DISCONNECT | timestamp | player/server name
         logprintf(LogConsumer::LogConnection, "SERVER_DISCONNECT\t%s\t%s", getTimeStamp().c_str(), mPlayerOrServerName.getString());
      }
      else
      {
         // CLIENT_DISCONNECT | timestamp | player name
         logprintf(LogConsumer::LogConnection, "CLIENT_DISCONNECT\t%s\t%s", getTimeStamp().c_str(), mPlayerOrServerName.getString());
      }

      if(isInGlobalChat)
         for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
            if(walk->isInGlobalChat)
               walk->m2cPlayerLeftGlobalChat(mPlayerOrServerName);

      gNeedToWriteStatus = true;
   }

   /// Adds this connection to the doubly linked list of servers
   void linkToServerList()
   {
      mNext = gServerList.mNext;
      mPrev = gServerList.mNext->mPrev;
      mNext->mPrev = this;
      mPrev->mNext = this;

      gNeedToWriteStatus = true;

      // SERVER_CONNECT | timestamp | server name | server description
      logprintf(LogConsumer::LogConnection, "SERVER_CONNECT\t%s\t%s\t%s", getTimeStamp().c_str(), mPlayerOrServerName.getString(), mServerDescr.getString());
   }


   void linkToClientList()
   {
      mNext = gClientList.mNext;
      mPrev = gClientList.mNext->mPrev;
      mNext->mPrev = this;
      mPrev->mNext = this;

      gNeedToWriteStatus = true;

      // CLIENT_CONNECT | timestamp | player name
      logprintf(LogConsumer::LogConnection, "CLIENT_CONNECT\t%s\t%s", getTimeStamp().c_str(), mPlayerOrServerName.getString());
   }

   enum PHPBB3AuthenticationStatus {
      Authenticated,
      CantConnect,
      UnknownUser,
      WrongPassword,
      InvalidUsername,
      Unsupported,
      UnknownStatus
   };

#ifdef VERIFY_PHPBB3    // Defined in Linux Makefile, not in VC++ project

   // Check username & password against database
   PHPBB3AuthenticationStatus verifyCredentials(string &username, string password)
   {
      Authenticator authenticator;

      // Security levels: 0 = no security (no checking for sql-injection attempts, not recommended unless you add your own security)
      //		    1 = basic security (prevents the use of any of these characters in the username: "(\"*^';&></) " including the space)
      //        1 = very basic security (prevents the use of double quote character)
      //		    2 = alphanumeric (only allows alphanumeric characters in the username)
      //
      // We'll use level 1 for now, so users can put special characters in their username
      authenticator.initialize(gMySqlAddress, gDbUsername, gDbPassword, gPhpbb3Database, gPhpbb3TablePrefix, 1);

      S32 errorcode;
      if(authenticator.authenticate(username, password, errorcode))   // returns true if the username was found and the password is correct
         return Authenticated;
      else
      {
         switch(errorcode)
         {
            case 0:  return CantConnect;
            case 1:  return UnknownUser;
            case 2:  return WrongPassword;
            case 3:  return InvalidUsername;
            default: return UnknownStatus;
         }
      }
   }
#else
      // Check name & pw against database
   PHPBB3AuthenticationStatus verifyCredentials(string name, string password)
   {
      return Unsupported;
   }
#endif


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
   )
   {
      Vector<IPAddress> theVector(IP_MESSAGE_ADDRESS_COUNT);
      theVector.reserve(IP_MESSAGE_ADDRESS_COUNT);

      for(MasterServerConnection *walk = gServerList.mNext; walk != &gServerList; walk = walk->mNext)
      {
         // First check the version -- we only want to match potential players that agree on which protocol to use
         if(mCMProtocolVersion == 0)      // CM Protocol 0 uses strings for version numbers
         {     // braces required
            if(walk->mVersionString != mVersionString)
               continue;
         }
         else // Version 1 or higher protocol uses a numerical versioning system
            if(walk->mCSProtocolVersion != mCSProtocolVersion)     // Incomptible protocols
               continue;

         // Ok, so the protocol version is correct, however...
         if(!(walk->mRegionCode & regionMask))       // ...wrong region
            continue;
         if(walk->mPlayerCount > maxPlayers || walk->mPlayerCount < minPlayers)   // ...too few or too many players
            continue;
         if(infoFlags & ~walk->mInfoFlags)           // ...wrong info flags
            continue;
         if(maxBots < walk->mNumBots)                // ...too many bots
            continue;
         if(minCPUSpeed > walk->mCPUSpeed)           // ...CPU speed insufficient
            continue;
         if(gameType.isNotNull() && (gameType != walk->mLevelName))          // ...wrong level name
            continue;
         if(missionType.isNotNull() && (missionType != walk->mLevelType))    // ...wrong level type
            continue;

         // Somehow, despite it all, we matched.  Add us to the results list.
         theVector.push_back(walk->getNetAddress().toIPAddress());

         // If we get a packet's worth, send it to the client and empty our buffer...
         if(theVector.size() == IP_MESSAGE_ADDRESS_COUNT)
         {
            m2cQueryServersResponse(queryId, theVector);
            theVector.clear();
         }
      }

      // Send the final packet
      m2cQueryServersResponse(queryId, theVector);
      // If we sent any with the previous message, send another list with no servers.
      if(theVector.size())
      {
         theVector.clear();
         m2cQueryServersResponse(queryId, theVector);
      }
   }

   /// checkActivityTime validates that this particular connection is
   /// not issuing too many requests at once in an attempt to DOS
   /// by flooding either the master server or any other server
   /// connected to it.  A client whose last activity time falls
   /// within the specified delta gets a strike... 3 strikes and
   /// you're out!  Strikes go away after being good for a while.
   void checkActivityTime(U32 timeDeltaMinimum)
   {
      U32 currentTime = Platform::getRealMilliseconds();
      if(currentTime - mLastActivityTime < timeDeltaMinimum)
      {
         mStrikeCount++;
         if(mStrikeCount == 3)
            disconnect(ReasonFloodControl, "");
      }
      else if(mStrikeCount > 0)
         mStrikeCount--;
   }

   void removeConnectRequest(GameConnectRequest *gcr)
   {
      for(S32 j = 0; j < mConnectList.size(); j++)
      {
         if(gcr == mConnectList[j])
         {
            mConnectList.erase_fast(j);
            break;
         }
      }
   }

   GameConnectRequest *findAndRemoveRequest(U32 requestId)
   {
      GameConnectRequest *req = NULL;
      for(S32 j = 0; j < mConnectList.size(); j++)
      {
         if(mConnectList[j]->hostQueryId == requestId)
         {
            req = mConnectList[j];
            mConnectList.erase_fast(j);
            break;
         }
      }
      if(!req)
         return NULL;

      if(req->initiator.isValid())
         req->initiator->removeConnectRequest(req);
      for(S32 j = 0; j < gConnectList.size(); j++)
      {
         if(gConnectList[j] == req)
         {
            gConnectList.erase_fast(j);
            break;
         }
      }
      return req;
   }


   // Write a current count of clients/servers for display on a website, using JSON format
   // This gets updated whenver we gain or lose a server, at most every REWRITE_TIME ms
   static void writeClientServerList_JSON()
   {
      if(gJasonOutFile == "")
         return;

      bool first = true;
      S32 playerCount = 0;
      S32 serverCount = 0;

      FILE *f = fopen(gJasonOutFile.c_str(), "w");
      if(f)
      {
         // First the servers
         fprintf(f, "{\n\t\"servers\": [");

            for(MasterServerConnection *walk = gServerList.mNext; walk != &gServerList; walk = walk->mNext)
            {
               fprintf(f, "%s\n\t\t{\n\t\t\t\"serverName\": \"%s\",\n\t\t\t\"protocolVersion\": %d,\n\t\t\t\"currentLevelName\": \"%s\",\n\t\t\t\"currentLevelType\": \"%s\",\n\t\t\t\"playerCount\": %d\n\t\t}",
                          first ? "" : ", ", sanitizeForJson(walk->mPlayerOrServerName.getString()), 
                          walk->mCSProtocolVersion, walk->mLevelName.getString(), walk->mLevelType.getString(), walk->mPlayerCount);
               playerCount +=  walk->mPlayerCount;
               serverCount++;
               first = false;
            }

         // Next the player names
         fprintf(f, "\n\t],\n\t\"players\": [");

            first = true;
            for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
            {
               fprintf(f, "%s\"%s\"", first ? "":", ", sanitizeForJson(walk->mPlayerOrServerName.getString()));
               first = false;
            }

            // Finally, the player and server counts
            fprintf(f, "],\n\t\"serverCount\": %d,\n\t\"playerCount\": %d\n}\n", serverCount, playerCount);

         fflush(f);
         fclose(f);
      }
      else
         logprintf(LogConsumer::LogError, "Could not write to JSON file \"%s\"", gJasonOutFile.c_str());
   }

   /*  Resulting JSON data should look like this:
   {
      "servers": [
         {
            "serverName": "Spang",
            "protocolVersion": 23,
            "currentLevelName": "Triple Threat",
            "currentLevelType": "CTF",
            "playerCount": 2
         },
         {
            "serverName": "Ramst",
            "protocolVersion": 23,
            "currentLevelName": "Kaptor",
            "currentLevelType": "Hunters",
            "playerCount": 3
         }
      ],
      "players": [ "chris", "colin", "fred", "george", "Peter99" ],
      "serverCount": 2,
      "playerCount": 5
   }
   */


   // This is called when a client wishes to arrange a connection with a server
   TNL_DECLARE_RPC_OVERRIDE(c2mRequestArrangedConnection, (U32 requestId, IPAddress remoteAddress, IPAddress internalAddress,
                                                           ByteBufferPtr connectionParameters) )
   {
      // First, make sure that we're connected with the server that they're requesting a connection with
      MasterServerConnection *conn = (MasterServerConnection *) gNetInterface->findConnection(remoteAddress);
      if(!conn)
      {
         ByteBufferPtr ptr = new ByteBuffer((U8 *) MasterNoSuchHost, (U32) strlen(MasterNoSuchHost) + 1);
            s2mRejectArrangedConnection(requestId, ptr);
         return;
      }

      // Record the request...
      GameConnectRequest *req = new GameConnectRequest;
      req->initiator = this;
      req->host = conn;
      req->initiatorQueryId = requestId;
      req->hostQueryId = mLastQueryId++;
      req->requestTime = Platform::getRealMilliseconds();

 
      logprintf(LogConsumer::LogConnectionManager, "Client: %s requested connection to %s", 
                                                        getNetAddress().toString(), conn->getNetAddress().toString());

      // Add the request to the relevant lists (the global list, this connection's list,
      // and the other connection's list).
      mConnectList.push_back(req);
      conn->mConnectList.push_back(req);
      gConnectList.push_back(req);

      // Do some DOS checking...
      checkActivityTime(2000);      // 2 secs

      // Get our address...
      Address theAddress = getNetAddress();

      // Record some different addresses to try...
      Vector<IPAddress> possibleAddresses;

      // The address, but port+1
      theAddress.port++;
      possibleAddresses.push_back(theAddress.toIPAddress());

      // The address, with the original port
      theAddress.port--;
      possibleAddresses.push_back(theAddress.toIPAddress());

      // Or the address the port thinks it's talking to.
      // (That is, if it's not the any address.)
      Address theInternalAddress(internalAddress);
      Address anyAddress;
      if(!theInternalAddress.isEqualAddress(anyAddress) && theInternalAddress != theAddress)
         possibleAddresses.push_back(internalAddress);

      // And inform the other part of the request
      conn->m2sClientRequestedArrangedConnection(req->hostQueryId, possibleAddresses, connectionParameters);
   }


   /// s2mAcceptArrangedConnection is sent by a server to notify the master that it will accept the connection
   /// request from a client.  The requestId parameter sent by the MasterServer in m2sClientRequestedArrangedConnection
   /// should be sent back as the requestId field.  The internalAddress is the server's self-determined IP address.

   // Called to indicate a connect request is being accepted.
   TNL_DECLARE_RPC_OVERRIDE(s2mAcceptArrangedConnection, (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData))
   {
      GameConnectRequest *req = findAndRemoveRequest(requestId);
      if(!req)
         return;

      Address theAddress = getNetAddress();
      Vector<IPAddress> possibleAddresses;

      theAddress.port++;
      possibleAddresses.push_back(theAddress.toIPAddress());

      theAddress.port--;
      possibleAddresses.push_back(theAddress.toIPAddress());

      Address theInternalAddress(internalAddress);
      Address anyAddress;
      if(!theInternalAddress.isEqualAddress(anyAddress) && theInternalAddress != theAddress)
         possibleAddresses.push_back(internalAddress);


      char buffer[256];
      strcpy(buffer, getNetAddress().toString());

      logprintf(LogConsumer::LogConnectionManager, "[%s] Server: %s accepted connection request from %s", getTimeStamp().c_str(), buffer, 
                                                        req->initiator.isValid() ? req->initiator->getNetAddress().toString() :        
                                                                                   "Unknown");

      // If we still know about the requestor, tell him his connection was accepted...
      if(req->initiator.isValid())
         req->initiator->m2cArrangedConnectionAccepted(req->initiatorQueryId, possibleAddresses, connectionData);

      delete req;
   }


   // TODO: Delete after 014
   TNL_DECLARE_RPC_OVERRIDE(c2mAcceptArrangedConnection, (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData))
   {
      GameConnectRequest *req = findAndRemoveRequest(requestId);
      if(!req)
         return;

      Address theAddress = getNetAddress();
      Vector<IPAddress> possibleAddresses;

      theAddress.port++;
      possibleAddresses.push_back(theAddress.toIPAddress());

      theAddress.port--;
      possibleAddresses.push_back(theAddress.toIPAddress());

      Address theInternalAddress(internalAddress);
      Address anyAddress;
      if(!theInternalAddress.isEqualAddress(anyAddress) && theInternalAddress != theAddress)
         possibleAddresses.push_back(internalAddress);


      char buffer[256];
      strcpy(buffer, getNetAddress().toString());

      logprintf(LogConsumer::LogConnectionManager, "[%s] Server: %s accept connection request from %s", getTimeStamp().c_str(), buffer,
                                                        req->initiator.isValid() ? req->initiator->getNetAddress().toString() : "Unknown");

      // If we still know about the requestor, tell him his connection was accepted...
      if(req->initiator.isValid())
         req->initiator->m2cArrangedConnectionAccepted(req->initiatorQueryId, possibleAddresses, connectionData);

      delete req;
   }



   // TODO: Delete after 014 -- replaced with identical s2mRejectArrangedConnection
   TNL_DECLARE_RPC_OVERRIDE(c2mRejectArrangedConnection, (U32 requestId, ByteBufferPtr rejectData))
   {
      GameConnectRequest *req = findAndRemoveRequest(requestId);
      if(!req)
         return;

      logprintf(LogConsumer::LogConnectionManager, "[%s] Server: %s reject connection request from %s",
                                                        getTimeStamp().c_str(), getNetAddress().toString(),
                                                        req->initiator.isValid() ? req->initiator->getNetAddress().toString() : "Unknown");    
      if(req->initiator.isValid())
         req->initiator->m2cArrangedConnectionRejected(req->initiatorQueryId, rejectData);

      delete req;
   }


   // s2mRejectArrangedConnection notifies the Master Server that the server is rejecting the arranged connection
   // request specified by the requestId.  The rejectData will be passed along to the requesting client.
   // Called to indicate a connect request is being rejected.
   TNL_DECLARE_RPC_OVERRIDE(s2mRejectArrangedConnection, (U32 requestId, ByteBufferPtr rejectData))
   {
      GameConnectRequest *req = findAndRemoveRequest(requestId);
      if(!req)
         return;

      logprintf(LogConsumer::LogConnectionManager, "[%s] Server: %s reject connection request from %s",
                                                        getTimeStamp().c_str(), getNetAddress().toString(),
                                                        req->initiator.isValid() ? req->initiator->getNetAddress().toString() : "Unknown");

      if(req->initiator.isValid())
         req->initiator->m2cArrangedConnectionRejected(req->initiatorQueryId, rejectData);

      delete req;
   }


   // TODO: Delete after 014 -- replaced with identical s2mUpdateServerStatus
   TNL_DECLARE_RPC_OVERRIDE(c2mUpdateServerStatus, (
      StringTableEntry levelName, StringTableEntry levelType,
      U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags))
   {
      // If we didn't know we were a game server, don't accept updates
      if(!mIsGameServer)
         return;

      mLevelName = levelName;
      mLevelType = levelType;

      mNumBots = botCount;
      mPlayerCount = playerCount;
      mMaxPlayers = maxPlayers;
      mInfoFlags = infoFlags;

      // Check to ensure we're not getting flooded with these requests
      checkActivityTime(15000);      // 15 secs

      gNeedToWriteStatus = true;
   }


   // s2mUpdateServerStatus updates the status of a server to the Master Server, specifying the current game
   // and mission types, any player counts and the current info flags.
   // Updates the master with the current status of a game server.
   TNL_DECLARE_RPC_OVERRIDE(s2mUpdateServerStatus, (StringTableEntry levelName, StringTableEntry levelType,
                                                    U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags))
   {
      // If we didn't know we were a game server, don't accept updates
      if(!mIsGameServer)
         return;

      mLevelName = levelName;
      mLevelType = levelType;

      mNumBots = botCount;
      mPlayerCount = playerCount;
      mMaxPlayers = maxPlayers;
      mInfoFlags = infoFlags;

      // Check to ensure we're not getting flooded with these requests
      checkActivityTime(15000);      // 15 secs

      gNeedToWriteStatus = true;
   }


   // Send player statistics to the master server
   TNL_DECLARE_RPC_OVERRIDE(s2mSendPlayerStatistics, (StringTableEntry playerName, 
                                                      U16 kills, U16 deaths, U16 suicides, Vector<U16> shots, Vector<U16> hits))
   {
      S32 totalShots = 0, totalHits = 0;

      for(S32 i = 0; i < shots.size(); i++)
      {
         totalShots += shots[i];
         totalHits += hits[i];
      }

      // PLAYER | name | kills | deaths | suicides | shots | hits 
      logprintf(LogConsumer::StatisticsFilter, "PLAYER\t%s\t%d\t%d\t%d\t%d\t%d", playerName.getString(), kills, deaths, suicides, totalShots, totalHits);
   }

      // Send player statistics to the master server
   TNL_DECLARE_RPC_OVERRIDE(s2mSendPlayerStatistics_2, (StringTableEntry playerName, StringTableEntry teamName, 
                                                        U16 kills, U16 deaths, U16 suicides, Vector<U16> shots, Vector<U16> hits))
   {
      S32 totalShots = 0, totalHits = 0;

      for(S32 i = 0; i < shots.size(); i++)
      {
         totalShots += shots[i];
         totalHits += hits[i];
      }

      // PLAYER | stats version | name | team | kills | deaths | suicides | shots | hits 
      logprintf(LogConsumer::StatisticsFilter, "PLAYER\t2\t%s\t%s\t%d\t%d\t%d\t%d\t%d", playerName.getString(), teamName.getString(), kills, deaths, suicides, totalShots, totalHits);
   }

   // Send game statistics to the master server
   TNL_DECLARE_RPC_OVERRIDE(s2mSendGameStatistics, (StringTableEntry gameType, StringTableEntry levelName, 
                                                    RangedU32<0,MAX_PLAYERS> players, S16 timeInSecs))
   {
      string timestr = itos(timeInSecs / 60) + ":";
      timestr += ((timeInSecs % 60 < 10) ? "0" : "") + itos(timeInSecs % 60);

      // GAME | GameType | Time | Level name | players | time
      logprintf(LogConsumer::StatisticsFilter, "GAME\t%s\t%s\t%s\t%d\t%s", 
                    getTimeStamp().c_str(), gameType.getString(), levelName.getString(), players.value, timestr.c_str() );
   }


      // Send game statistics to the master server
   TNL_DECLARE_RPC_OVERRIDE(s2mSendGameStatistics_2, (StringTableEntry gameType, StringTableEntry levelName, 
                                                      Vector<StringTableEntry> teams, Vector<S32> teamScores,
                                                      Vector<RangedU32<0,256> > colorR, Vector<RangedU32<0,256> > colorG, Vector<RangedU32<0,256> > colorB, 
                                                      RangedU32<0,MAX_PLAYERS> players, S16 timeInSecs))
   {
      string timestr = itos(timeInSecs / 60) + ":";
      timestr += ((timeInSecs % 60 < 10) ? "0" : "") + itos(timeInSecs % 60);

      // GAME | stats version | GameType | time | level name | teams | players | time
      logprintf(LogConsumer::StatisticsFilter, "GAME\t2\t%s\t%s\t%s\t%d\t%d\t%s", 
                    getTimeStamp().c_str(), gameType.getString(), levelName.getString(), teams.size(), players.value, timestr.c_str() );

      // TEAM | stats version | team name | score | R | G |B
      for(S32 i = 0; i < teams.size(); i++)
         logprintf("TEAM\t2\t%s\t%d\t%d\t%d\t%d", teams[i].getString(), teamScores[i], (U32)colorR[i], (U32)colorG[i], (U32)colorB[i]);
   }


   // Game server wants to know if user name has been verified
   TNL_DECLARE_RPC_OVERRIDE(s2mRequestAuthentication, (Vector<U8> id, StringTableEntry name))
   {
      Nonce clientId(id);     // Reconstitute our id

      for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
         if(walk->mPlayerId == clientId)
         {
            AuthenticationStatus status;

            if(!stricmp(name.getString(), walk->mPlayerOrServerName.getString()))      // Need case insensitive comparison here
               status = AuthenticationStatusAuthenticatedName;

            // If server just restarted, clients will need to reauthenticate, and that may take some time.
            // We'll give them 90 seconds.
            else if(Platform::getRealMilliseconds() - gServerStartTime < 90000)      
               status = AuthenticationStatusTryAgainLater;             
            else
               status = AuthenticationStatusUnauthenticatedName;

            m2sSetAuthenticated(id, walk->mPlayerOrServerName, status);
            break;
         }
   }


   // From http://www.codeproject.com/KB/stl/stdstringtrim.aspx
   void trim(string &str)
   {
      string::size_type pos = str.find_last_not_of(' ');
      if(pos != string::npos) 
      {
         str.erase(pos + 1);
         pos = str.find_first_not_of(' ');
         if(pos != string::npos) str.erase(0, pos);
      }
      else str.erase(str.begin(), str.end());
   }


   string cleanName(string name)
   {
      trim(name);
      if(name == "")
         name = "ChumpChange";

      return name;
   }


   bool readConnectRequest(BitStream *bstream, NetConnection::TerminationReason &reason)
   {
      if(!Parent::readConnectRequest(bstream, reason))
         return false;

      // We first read a string in -- in protocol version 0, the version was sent as a string.  In susbsequent
      // protocols, a "+" char is sent for the version, followed by an int version number.  This should provide
      // more flexibility to alter the cm communication protocol in the future while retaining backwards compatibility
      // At some point, when no more version 0 clients exist in the wild, the version 0 stuff can be removed.  However,
      // we will still need to keep the dummy string because there is no way to remove it without breaking some
      // version of the protocol or another.
      // Also note that if player is hosting a game interactively (i.e. with host option on main menu), they
      // will create two connections here, one for the host, and one for the client instance.
      char readstr[256];
      bstream->readString(readstr);
      mVersionString = readstr;

      if(!strcmp(mVersionString.getString(), "+"))          // This is a version 1 or above protocol
      {
         bstream->read(&mCMProtocolVersion);    // Version of protocol we'll use with the client
         bstream->read(&mCSProtocolVersion);    // Protocol this client uses for C-S communication
         bstream->read(&mClientBuild);          // Client's build number
         bstream->readString(readstr);          // Client's joystick autodetect string
         mAutoDetectStr = readstr;
      }
      else                                      // Otherwise, set some defaults  --->> this stuff below here is old, and 
      {                                         // only applies to very old versions of Bitfighter.... probably safe to delete.
         mCMProtocolVersion = 0;                // This is a version 0 protocol
         mCSProtocolVersion = 0;
         mClientBuild = 0;
         mAutoDetectStr = "N/A";
      }

      // If it's a game server, read status info...
      if((mIsGameServer = bstream->readFlag()) == true)
      {
         bstream->read(&mCPUSpeed);
         bstream->read(&mRegionCode);
         bstream->read(&mNumBots);
         bstream->read(&mPlayerCount);
         bstream->read(&mMaxPlayers);
         bstream->read(&mInfoFlags);

         bstream->readString(readstr);    // Current level name
         mLevelName = readstr;

         bstream->readString(readstr);    // Current level type (hunters, ctf, soccer, etc.)
         mLevelType = readstr;

         bstream->readString(readstr);
         mPlayerOrServerName = cleanName(readstr).c_str();

         bstream->readString(readstr);

         if(mCMProtocolVersion >= 2)
            mServerDescr = readstr;
         else
            mServerDescr = "";

         linkToServerList();
      }
      else     // Not a server? Must be a client
      {
         bstream->readString(readstr);
         mPlayerOrServerName = cleanName(readstr).c_str();

         if(mCMProtocolVersion <= 2)
            linkToClientList();

         else     // Level 3 or above: Read a password & id and check them out
         {
            bstream->readString(readstr);
            string password = readstr;
            
            mPlayerId.read(bstream);

            // Probably redundant, but let's cycle through all our clients and make sure the playerId is unique.  With 2^64
            // possibilities, it most likely will be.
            for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
               if(walk != this && walk->mCMProtocolVersion >= 3 && walk->mPlayerId == mPlayerId)
               {
                  logprintf(LogConsumer::LogConnection, "User %s provided duplicate id to %s", mPlayerOrServerName.getString(), 
                                                                                               walk->mPlayerOrServerName.getString());
                  disconnect(ReasonDuplicateId, "");
                  reason = ReasonDuplicateId;
                  return false;
               }

            // Verify name and password against our PHPBB3 database.  Name will be set to the correct case if it is authenticated.
            string name = mPlayerOrServerName.getString();
            PHPBB3AuthenticationStatus stat = verifyCredentials(name, password);

            mPlayerOrServerName.set(name.c_str());

            if(stat == WrongPassword)
            {
               logprintf(LogConsumer::LogConnection, "User %s provided the wrong password", mPlayerOrServerName.getString());
               disconnect(ReasonBadLogin, "");
               reason = ReasonBadLogin;
               return false;
            }
            else if(stat == InvalidUsername)
            {
               logprintf(LogConsumer::LogConnection, "User name %s contains illegal characters", mPlayerOrServerName.getString());
               // Send message back to client to request new username/password
               disconnect(ReasonInvalidUsername, "");
               reason = ReasonInvalidUsername;
               return false;
            }
            linkToClientList();
            if(stat == Authenticated)
            {
               logprintf(LogConsumer::LogConnection, "Authenticated user %s", mPlayerOrServerName.getString());
               mAuthenticated = true;

               m2cSetAuthenticated((U32)AuthenticationStatusAuthenticatedName, name.c_str());
            }

            else if(stat == UnknownUser || stat == Unsupported)
               m2cSetAuthenticated(AuthenticationStatusUnauthenticatedName, "");

            else  // stat == CantConnect || stat == UnknownStatus
               m2cSetAuthenticated(AuthenticationStatusFailed, "");
         }
      }

      // Figure out which MOTD to send to client, based on game version (stored in mVersionString)
      string motdString = "Welcome to Bitfighter!";  // Default msg

      if(mCMProtocolVersion >= 1)    // Don't even bother with level 0 clients -- none of these clients are out there anymore!
      {
         for(S32 i = 0; i < MOTDVersionVec.size(); i++)
            if(mClientBuild == MOTDVersionVec[i])
            {
               motdString = MOTDStringVec[i];
               break;
            }

         m2cSendUpdgradeStatus(gLatestReleasedCSProtocol > mCSProtocolVersion);   // Version 0 clients will disconnect if we try this

         // CLIENT/SERVER_INFO | timestamp | protocol version | build number | address | controller
         logprintf(LogConsumer::LogConnection, "%s\t%s\t%d\t%d\t%s\t%s",
                                                    mIsGameServer ? "SERVER_INFO" : "CLIENT_INFO", getTimeStamp().c_str(),
                                                    mCMProtocolVersion, mClientBuild, getNetAddress().toString(),
                                                    strcmp(mAutoDetectStr.getString(), "") ? mAutoDetectStr.getString():"<None>");
      }

      m2cSetMOTD(gMasterName.c_str(), motdString.c_str());     // Even level 0 clients can handle this

      return true;
   }

   // Appears unused!
   //static void checkConnectTimeouts()
   //{
   //   const S32 ConnectRequestTimeout = 30000;      // 30 secs
   //   U32 currentTime = Platform::getRealMilliseconds();

   //   // Expire any connect requests that have grown old...
   //   for(S32 i = 0; i < gConnectList.size(); )    // third param deliberately blank
   //   {
   //      GameConnectRequest *gcr = gConnectList[i];
   //      if(currentTime - gcr->requestTime > ConnectRequestTimeout)
   //      {
   //         // It's old, so remove it from the initiator's list...
   //         if(gcr->initiator.isValid())
   //         {
   //            gcr->initiator->removeConnectRequest(gcr);
   //            ByteBufferPtr reqTimeoutBuffer = new ByteBuffer((U8 *) MasterRequestTimedOut, (U32) strlen(MasterRequestTimedOut) + 1);
   //            
   //            // For older clients (009 and below)
   //            if(mCMProtocolVersion <= 1)
   //               gcr->initiator->s2mRejectArrangedConnection(gcr->initiatorQueryId, reqTimeoutBuffer);
   //            // For clients 010 and above
   //            else
   //               gcr->initiator->s2mRejectArrangedConnection(gcr->initiatorQueryId, reqTimeoutBuffer, ConnectionAttemptTimedOut);
   //         }

   //         // And the host's lists..
   //         if(gcr->host.isValid())
   //            gcr->host->removeConnectRequest(gcr);

   //         // Delete it...
   //         delete gcr;

   //         // And remove it from our list, too.
   //         gConnectList.erase_fast(i);
   //         continue;
   //      }
   //      i++;
   //   }
   //}


   TNL_DECLARE_RPC_OVERRIDE(c2mJoinGlobalChat, ())
   { 
      isInGlobalChat = true;
      
      Vector<StringTableEntry> names;

      for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
         if (walk != this && walk->isInGlobalChat)
         {
            walk->m2cPlayerJoinedGlobalChat(mPlayerOrServerName);
            names.push_back(walk->mPlayerOrServerName);
         }

      if(names.size() > 0)
         m2cPlayersInGlobalChat(names);
   }


   TNL_DECLARE_RPC_OVERRIDE(c2mLeaveGlobalChat, ())
   {
      isInGlobalChat = false;

      for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
         if (walk != this && walk->isInGlobalChat)
            walk->m2cPlayerLeftGlobalChat(mPlayerOrServerName);
   }


   // Got out-of-game chat message from client, need to relay it to others
   TNL_DECLARE_RPC_OVERRIDE(c2mSendChat, (StringPtr message))
   {
      bool isPrivate = false;
      char privateTo[MAX_PLAYER_NAME_LENGTH + 1];
      char strippedMessage[MAX_CHAT_MSG_LENGTH + 1];

      // Check if the message is private.  If so, we need to parse it.
      if(message.getString()[0] == '/')      // Format: /ToNick Message goes here
      {
         isPrivate = true;

         bool buildingTo = true;
         S32 j = 0, k = 0;

         for(S32 i = 1; message.getString()[i]; i++)    // Start at 1 to lose the leading '/'
         {
            char c = message.getString()[i];

            if(buildingTo)
            {     // (braces required)
               if (c == ' ' || c == '\t')
                  buildingTo = false;
               else
                  if (j < MAX_PLAYER_NAME_LENGTH) privateTo[j++] = c;
            }
            else
               if (k < MAX_CHAT_MSG_LENGTH) strippedMessage[k++] = c;
         }

         if(buildingTo)       // If we're still in buildingTo mode by the time we get here, it means...
         {
            logprintf(LogConsumer::LogError, "Malformed private message: %s", message.getString());
            return;           // ...that there was no body of the message.  In which case, we're done.
         }

         privateTo[j] = 0;          // Make sure our strings are
         strippedMessage[k] = 0;    // properly terminated
      }


      for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
      {
         if(isPrivate)
         {
            if (!stricmp(walk->mPlayerOrServerName.getString(), privateTo))     // Only send to player(s) with the specified nick
               walk->m2cSendChat(mPlayerOrServerName, isPrivate, strippedMessage);
         }
         else                    // Send to everyone...
            if (walk != this)    // ...except self!
               walk->m2cSendChat(mPlayerOrServerName, isPrivate, message);
      }

      // Log F5 chat messages
      if(isPrivate)
      {
         logprintf(LogConsumer::LogChat, "Relayed private msg from %s to %s: %s", mPlayerOrServerName.getString(), privateTo, strippedMessage);
      }
      else
      {
         logprintf(LogConsumer::LogChat, "Relayed chat msg from %s: %s", mPlayerOrServerName.getString(), message.getString());
         }
   }

   TNL_DECLARE_NETCONNECTION(MasterServerConnection);

};

TNL_IMPLEMENT_NETCONNECTION(MasterServerConnection, NetClassGroupMaster, true);

Vector< GameConnectRequest* > MasterServerConnection::gConnectList;

MasterServerConnection MasterServerConnection::gServerList;
MasterServerConnection MasterServerConnection::gClientList;


void seedRandomNumberGenerator()
{
   U32 time = Platform::getRealMilliseconds();

   U8 buf[16];

   buf[0] = U8(time);
   buf[1] = U8(time >> 8);
   buf[2] = U8(time >> 16);
   buf[3] = U8(time >> 24);

   // Need at least 16 bytes to make anything happen.  We'll provide 4 sort of good ones, and 12 bytes of uninitialized crap.
   Random::addEntropy(buf, 16);
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
////////////////////////////////////////
////////////////////////////////////////

#include <stdio.h>

////////////////////////////////////////
////////////////////////////////////////

FileLogConsumer gFileLogConsumer;
FileLogConsumer gStatisticsLogConsumer;
StdoutLogConsumer gStdoutLogConsumer;

////////////////////////////////////////
////////////////////////////////////////


U32 gMasterPort = 25955;      // <== Default, can be overwritten in cfg file

extern void readConfigFile();


int main(int argc, const char **argv)
{
   gServerStartTime = Platform::getRealMilliseconds();

   gMasterName = "Bitfighter Master Server";    // Default, can be overridden in cfg file

   seedRandomNumberGenerator();

   // Configure logging
   S32 events = LogConsumer::AllErrorTypes | LogConsumer::LogConnection | LogConsumer::LogConnectionManager | LogConsumer::LogChat;

   gFileLogConsumer.init("bitfighter_master.log", "a");
   gFileLogConsumer.setMsgTypes(events);                               // Primary logfile
   gFileLogConsumer.logprintf("------ Bitfighter Master Server Log File ------");

   gStatisticsLogConsumer.init("bitfighter_player_stats.log", "a");
   gStatisticsLogConsumer.setMsgTypes(LogConsumer::StatisticsFilter);  // Statistics file

   gStdoutLogConsumer.setMsgTypes(events);                             // stdout


   // Parse command line parameters...
   readConfigFile();

   // Initialize our net interface so we can accept connections...
   gNetInterface = new NetInterface(Address(IPProtocol, Address::Any, gMasterPort));

   //for the master server alone, we don't need a key exchange - that would be a waste
   //gNetInterface->setRequiresKeyExchange(true);
   //gNetInterface->setPrivateKey(new AsymmetricKey(20));

   // Log a welcome message in the main log and to the console
   gFileLogConsumer.logprintf("[%s] Master Server %s started - listening on port %d", getTimeStamp().c_str(), gMasterName.c_str(), gMasterPort);
   gStdoutLogConsumer.logprintf("Master Server %s started - listening on port %d", gMasterName.c_str(), gMasterPort);

   const S32 REWRITE_TIME = 5000;        // Rewrite status file at most this often (in ms)
   const S32 REREAD_TIME = 5000;         // How often to we re-read our config file? (in ms)

   U32 lastConfigReadTime = Platform::getRealMilliseconds();
   U32 lastWroteStatusTime = lastConfigReadTime - REWRITE_TIME;    // So we can do a write right off the bat

    // And until infinity, process whatever comes our way.
  for(;;)     // To infinity and beyond!!
   {
      U32 currentTime = Platform::getRealMilliseconds();
      gNetInterface->checkIncomingPackets();
      gNetInterface->processConnections();

      if(currentTime - lastConfigReadTime > REREAD_TIME)     // Reread the config file every 5000ms
      {
         lastConfigReadTime = currentTime;
         readConfigFile();
      }

      // Write status file as need, at most every REWRITE_TIME ms
      if(gNeedToWriteStatus && currentTime - lastWroteStatusTime > REWRITE_TIME)  
      {
         lastWroteStatusTime = currentTime;
         MasterServerConnection::writeClientServerList_JSON();
         gNeedToWriteStatus = false;
      }

      Platform::sleep(1);
   }
   return 0;
}
