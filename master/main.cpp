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

#include "../zap/SharedConstants.h"

#include "masterInterface.h"
#include "authenticator.h"    // For authenticating users against the PHPBB3 database
#include "database.h"         // For writing to the database

#include "tnlNetInterface.h"
#include "tnlVector.h"
#include "tnlAsymmetricKey.h"

#include <stdio.h>
#include <string>
#include <stdarg.h>     // For va_args
#include <time.h>

#include "../zap/stringUtils.h"     // For itos, replaceString


using namespace TNL;
using namespace std;
using namespace Zap;

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

// Variables for writing stats
string gStatsDatabaseAddress;
string gStatsDatabaseName;
string gStatsDatabaseUsername;
string gStatsDatabasePassword;

bool gWriteStatsToMySql;


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

//// TODO: Get this from stringUtils... doesn't work for some reason.  Too tired to track it down at the moment...
//std::string itos(S32 i)
//{
//   char outString[100];
//   dSprintf(outString, sizeof(outString), "%d", i);
//   return outString;
//}


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

   /// Destructor removes the connection from the doubly linked list of server connections
   ~MasterServerConnection()
   {
      // Unlink it if it's in the list
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
      //          1 = basic security (prevents the use of any of these characters in the username: "(\"*^';&></) " including the space)
      //        1 = very basic security (prevents the use of double quote character)
      //          2 = alphanumeric (only allows alphanumeric characters in the username)
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
         {
            logprintf(LogConsumer::LogConnection, "User %s Disconnect due to flood control set at %i milliseconds", 
                                                  mPlayerOrServerName.getString(), timeDeltaMinimum);
            disconnect(ReasonFloodControl, "");
         }
      }
      else if(mStrikeCount > 0)
         mStrikeCount--;

      mLastActivityTime = currentTime;
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


   MasterServerConnection *findClient(Nonce &clientId)   // Should be const, but that won't compile for reasons not yet determined!!
   {
      if(!clientId.isValid())
         return NULL;

      for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
         if(walk->mPlayerId == clientId)
            return walk;

      return NULL;
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

         // Next the player names      // "players": [ "chris", "colin", "fred", "george", "Peter99" ],
         fprintf(f, "\n\t],\n\t\"players\": [");
         first = true;
         for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
         {
            fprintf(f, "%s\"%s\"", first ? "":", ", sanitizeForJson(walk->mPlayerOrServerName.getString()));
            first = false;
         }

         // Authentication status      // "authenticated": [ true, false, false, true, true ],
         fprintf(f, "],\n\t\"authenticated\": [");
         first = true;
         for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
         {
            fprintf(f, "%s%s", first ? "":", ", walk->mAuthenticated ? "true" : "false");
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
      "authenticated": [ true, false, false, true, true ],
      "serverCount": 2,
      "playerCount": 5
   }
   */

   bool isAuthenticated() { return mAuthenticated; }

   // This is called when a client wishes to arrange a connection with a server
   TNL_DECLARE_RPC_OVERRIDE(c2mRequestArrangedConnection, (U32 requestId, IPAddress remoteAddress, IPAddress internalAddress,
                                                           ByteBufferPtr connectionParameters) )
   {
      // First, make sure that we're connected with the server that they're requesting a connection with
      MasterServerConnection *conn = (MasterServerConnection *) gNetInterface->findConnection(remoteAddress);
      if(!conn)
      {
         ByteBufferPtr ptr = new ByteBuffer((U8 *) MasterNoSuchHost, (U32) strlen(MasterNoSuchHost) + 1);
             (requestId, ptr);
            //s2mRejectArrangedConnection(requestId, ptr);  // client randomly get "Invalid Packet -- event direction wrong"...
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
      // version 013? and earlier use m2c, don't want to break compatibility.
      if(conn->mCSProtocolVersion >= 31)
         conn->m2sClientRequestedArrangedConnection(req->hostQueryId, possibleAddresses, connectionParameters);
      else
         conn->m2cClientRequestedArrangedConnection(req->hostQueryId, possibleAddresses, connectionParameters);
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

      //Update only if anything is different
      if(mLevelName   != levelName   || mLevelType  != levelType  || mNumBots   != botCount ||
         mPlayerCount != playerCount || mMaxPlayers != maxPlayers || mInfoFlags != infoFlags )
      {
         mLevelName = levelName;
         mLevelType = levelType;

         mNumBots = botCount;
         mPlayerCount = playerCount;
         mMaxPlayers = maxPlayers;
         mInfoFlags = infoFlags;

         // Check to ensure we're not getting flooded with these requests
         checkActivityTime(4000);      // 4 secs     version 014 send status every 5 seconds

         gNeedToWriteStatus = true;
      }
   }


   // s2mUpdateServerStatus updates the status of a server to the Master Server, specifying the current game
   // and mission types, any player counts and the current info flags.
   // Updates the master with the current status of a game server.
   TNL_DECLARE_RPC_OVERRIDE(s2mUpdateServerStatus, (StringTableEntry levelName, StringTableEntry levelType,
                                                    U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags))
   {
      // Only accept updates from game servers
      if(!mIsGameServer)
         return;

      // Update only if anything is different
      if(mLevelName   != levelName   || mLevelType  != levelType  || mNumBots   != botCount   || 
         mPlayerCount != playerCount || mMaxPlayers != maxPlayers || mInfoFlags != infoFlags )
      {
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
   }


   // Send player statistics to the master server     ==> deprecated
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

   // Send player statistics to the master server     ==> deprecated as of 015
   TNL_DECLARE_RPC_OVERRIDE(s2mSendPlayerStatistics_2, (StringTableEntry playerName, StringTableEntry teamName, 
                                                        U16 kills, U16 deaths, U16 suicides, Vector<U16> shots, Vector<U16> hits))
   {
      S32 totalShots = 0, totalHits = 0;

      for(S32 i = 0; i < shots.size(); i++)
      {
         totalShots += shots[i];
         totalHits += hits[i];
      }

      // PLAYER | stats version (2) | name | team | kills | deaths | suicides | shots | hits 
      logprintf(LogConsumer::StatisticsFilter, "PLAYER\t2\t%s\t%s\t%d\t%d\t%d\t%d\t%d", 
                                                playerName.getString(), teamName.getString(), kills, deaths, suicides, totalShots, totalHits);
   }


   //// Send player statistics to the master server
   //TNL_DECLARE_RPC_OVERRIDE(s2mSendPlayerStatistics_3, (StringTableEntry playerName, Vector<U8> id, bool isBot, 
   //                                                     StringTableEntry teamName, S32 score,
   //                                                     U16 kills, U16 deaths, U16 suicides, Vector<U16> shots, Vector<U16> hits))
   //{
   //   Nonce clientId(id);

   //   MasterServerConnection *client = findClient(clientId);

   //   bool authenticated = (client && client->isAuthenticated());

   //   S32 totalShots = 0;
   //   S32 totalHits = 0;

   //   for(S32 i = 0; i < shots.size(); i++)
   //   {
   //      totalShots += shots[i];
   //      totalHits += hits[i];
   //   }

   //   // PLAYER | stats version (3) | name | authenticated | isBot | team | score | kills | deaths | suicides | shots | hits 
   //   logprintf(LogConsumer::StatisticsFilter, "PLAYER\t3\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d", 
   //            playerName.getString(), 
   //            authenticated ? "true" : "false", 
   //            isBot ? "true" : "false", 
   //            teamName.getString(), 
   //            score, kills, deaths, suicides, 
   //            totalShots, totalHits);
   //}


   // Send game statistics to the master server  ==> Deprecated
   TNL_DECLARE_RPC_OVERRIDE(s2mSendGameStatistics, (StringTableEntry gameType, StringTableEntry levelName, 
                                                    RangedU32<0,128> players, S16 timeInSecs))
   {
      string timestr = itos(timeInSecs / 60) + ":";
      timestr += ((timeInSecs % 60 < 10) ? "0" : "") + itos(timeInSecs % 60);

      // GAME | GameType | Time | Level name | players | time
      logprintf(LogConsumer::StatisticsFilter, "GAME\t%s\t%s\t%s\t%d\t%s", 
                    getTimeStamp().c_str(), gameType.getString(), levelName.getString(), players.value, timestr.c_str() );
   }


   // Send game statistics to the master server   ==> Deprecated starting in 015
   TNL_DECLARE_RPC_OVERRIDE(s2mSendGameStatistics_2, (StringTableEntry gameType, StringTableEntry levelName, 
                                                      Vector<StringTableEntry> teams, Vector<S32> teamScores,
                                                      Vector<RangedU32<0,256> > colorR, Vector<RangedU32<0,256> > colorG, Vector<RangedU32<0,256> > colorB, 
                                                      RangedU32<0,128> players, S16 timeInSecs))
   {
      string timestr = itos(timeInSecs / 60) + ":";
      timestr += ((timeInSecs % 60 < 10) ? "0" : "") + itos(timeInSecs % 60);

      // GAME | stats version (2) | GameType | time | level name | teams | players | time
      logprintf(LogConsumer::StatisticsFilter, "GAME\t2\t%s\t%s\t%s\t%d\t%d\t%s", 
                     getTimeStamp().c_str(), gameType.getString(), levelName.getString(), teams.size(), players.value, timestr.c_str() );

      // TEAM | stats version (2) | team name | score | R | G |B
      for(S32 i = 0; i < teams.size(); i++)
         logprintf(LogConsumer::StatisticsFilter, "TEAM\t2\t%s\t%d\t%d\t%d\t%d", 
                     teams[i].getString(), teamScores[i], (U32)colorR[i], (U32)colorG[i], (U32)colorB[i]);
   }

   /////////////////////////////////////////////////////////////////////////////////////




   // Send game statistics to the master server   ==> Current as of 015
   // Note that teams are sent in descending order, most players to fewest  
 //  TNL_DECLARE_RPC_OVERRIDE(s2mSendGameStatistics_3, (StringTableEntry gameType, bool teamGame, StringTableEntry levelName, 
 //                                                     Vector<StringTableEntry> teams, Vector<S32> teamScores,
 //                                                     Vector<RangedU32<0,0xFFFFFF> > color, 
 //                                                     U16 timeInSecs, Vector<StringTableEntry> playerNames, Vector<Vector<U8> > playerIds,
 //                                                     Vector<bool> isBot, Vector<bool> lastOnTeam, Vector<S32> playerScores, 
 //                                                     Vector<U16> playerKills, Vector<U16> playerDeaths, Vector<U16> playerSuicides, 
 //                                                     Vector<U16> teamSwitchCount, Vector<Vector<U16> > shots, Vector<Vector<U16> > hits))
 //  {
   //
 ////  TNL_DECLARE_RPC_OVERRIDE(s2mSendGameStatistics_3, (GameStatistics3 gameStat))
   ////{
 ////     StringTableEntry gameType = gameStat.gameType;
 ////     bool teamGame = gameStat.teamGame;
 ////     StringTableEntry levelName = gameStat.levelName;
 ////     Vector<StringTableEntry> teams = gameStat.teams;
 ////     Vector<S32> teamScores = gameStat.teamScores;
 ////     Vector<RangedU32<0,0xFFFFFF> > color = gameStat.color;
 ////     U16 timeInSecs = gameStat.timeInSecs;
 ////     Vector<StringTableEntry> playerNames = gameStat.playerNames;
 ////     Vector<Vector<U8> > playerIds = gameStat.playerIDs;
 ////     Vector<bool> isBot = gameStat.isBot;
 ////     Vector<bool> lastOnTeam = gameStat.lastOnTeam;
 ////     Vector<S32> playerScores = gameStat.playerScores;
 ////     Vector<U16> playerKills = gameStat.playerKills;
 ////     Vector<U16> playerDeaths = gameStat.playerDeaths;
 ////     Vector<U16> playerSuicides = gameStat.playerSuicides;
 ////     Vector<Vector<U16> > shots = gameStat.shots;
 ////     Vector<Vector<U16> > hits = gameStat.hits;


 //     if(mInfoFlags & TestModeFlag)       // Ignore stats from server in test mode
 //        return;  

 //     // Some integrity checks to protect agains bad data
 //     // TODO: Expand
 //     bool error = false;

 //     if(shots.size() != hits.size())
 //     {
 //        error = true;
 //     }
 //     if(!error)
 //     {
 //        for(S32 i = 0; i < shots.size(); i++)
 //           if(shots[i].size() != hits[i].size())
 //           {
 //              error = true;
 //              break;
 //           }
 //     }

 //     if(error)
 //     {
 //        // TODO: Log the error, and the client that sent it
 //        return;
 //     }

   //   string timestr = itos(timeInSecs / 60) + ":";
 //     timestr += ((timeInSecs % 60 < 10) ? "0" : "") + itos(timeInSecs % 60);

 //     S32 players = 0, bots = 0;
 //     for(S32 i = 0; i < isBot.size(); i++)
 //     {
 //        if(isBot[i])
 //           bots++;
 //        else
 //           players++;
 //     }

 //     // GAME | stats version (3) | GameVersion | timestamp | GameType | teamGame (true/false) | level name | teams | players | bots | time
 //     logprintf(LogConsumer::StatisticsFilter, "GAME\t3\t%d\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%s", 
 //                    mClientBuild, getTimeStamp().c_str(), gameType.getString(), teamGame ? "true" : "false", levelName.getString(), 
 //                    teams.size(), players, bots, timestr.c_str() );

 //     // TEAM | stats version (3) | team name | players | bots | score | hexColor
 //     for(S32 i = 0; i < teams.size(); i++)
 //     {
 //        Color teamColor(color[i]);
 //        logprintf(LogConsumer::StatisticsFilter, "TEAM\t3\t%s\t%s", 
 //                 teams[i].getString(), teamColor.toHexString().c_str());
 //     }

 //     
 //     // TODO: Make this work, integrate with statistics being logged on server
 //      //   // PLAYER | stats version (3) | name | authenticated | isBot | team | score | kills | deaths | suicides | shots | hits 
 //  //   logprintf(LogConsumer::StatisticsFilter, "PLAYER\t3\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d", 
 //  //            playerName.getString(), 
 //  //            authenticated ? "true" : "false", 
 //  //            isBot ? "true" : "false", 
 //  //            teamName.getString(), 
 //  //            score, kills, deaths, suicides, 
 //  //            totalShots, totalHits);


 //     S32 lastClient = 0;     // Track last player who's info was written

 //     // TODO: Make put this in a constructor
 //     GameStats gameStats;

 //     gameStats.duration = timeInSecs;
 //     gameStats.gameType = gameType.getString();
 //     gameStats.isOfficial = false;
 //     gameStats.isTeamGame = teamGame;
 //     gameStats.levelName = levelName.getString();
 //     gameStats.playerCount = playerNames.size();     // Humans + bots
 //     gameStats.serverIP = getNetAddressString();
 //     gameStats.serverName = mPlayerOrServerName.getString();
 //     gameStats.teamCount = teams.size();

 //     //gameStats.teamStats.setSize(teams.size());

 //     S32 nextPlayerToProcess = 0;

 //     for(S32 i = 0; i < teams.size(); i++)
 //     {
 //        TeamStats teamStats;
 //        Color teamColor(color[i]);

 //        teamStats.name = teams[i].getString();
 //        teamStats.color = teamColor.toHexString();

 //        teamStats.score = teamScores[i];

 //        for(S32 j = nextPlayerToProcess; j < playerNames.size(); j++)
 //        {
 //           PlayerStats playerStats;

 //           // TODO: Put these into a constuctor
 //           playerStats.deaths = playerDeaths[j];

 //           // Note: Have to wait until team result is computed to set it on the players

 //           Nonce playerId(playerIds[j]);
 //           MasterServerConnection *client = findClient(playerId);
 //           playerStats.isAuthenticated = (client && client->isAuthenticated());

 //           playerStats.isRobot = isBot[j];
 //           playerStats.kills = playerKills[j];
 //           playerStats.name = playerNames[j].getString();
 //           playerStats.points = playerScores[j];
 //           playerStats.suicides = playerSuicides[j];
 //           playerStats.switchedTeams = teamSwitchCount[j];    

 //           
 //           for(S32 k = 0; k < shots[j].size(); k++)
 //           {
 //              WeaponStats weaponStats;
 //              //TODO: Put the following into a constructor
 //              weaponStats.hits = hits[j][k];
 //              weaponStats.shots = shots[j][k];
 //              weaponStats.weaponType = (WeaponType) k;
 //              playerStats.weaponStats.push_back(weaponStats);
 //           }

 //           teamStats.playerStats.push_back(playerStats);

 //           if(lastOnTeam[j])
 //           {
 //              nextPlayerToProcess = j + 1;
 //              break;
 //           }
 //        }

 //        gameStats.teamStats.push_back(teamStats);
 //     }

 //     gameStats.teamStats.sort(teamScoreSort);

 //     // Compute win/loss/tie for teams
 //     for(S32 i = 0; i < gameStats.teamStats.size(); i++)
 //        gameStats.teamStats[i].gameResult = 
 //                    getResult(gameStats.teamStats.size(), teamScores[0], gameStats.teamStats.size() == 1 ? 0 : teamScores[1], teamScores[i], i == 0);

 //     // Now that we have the win/loss/tie results for the teams, we can also assign those same results to the players.
 //     if(gameStats.isTeamGame)
 //     {
 //        for(S32 i = 0; i < gameStats.teamStats.size(); i++)
 //           for(S32 j = 0; j < gameStats.teamStats[i].playerStats.size(); j++)
 //              gameStats.teamStats[i].playerStats[j].gameResult = gameStats.teamStats[i].gameResult;
 //     }
 //     else
 //     {
 //        // Non-team games: compute winning player(s) based on score; but must sort first
 //        if(!teamGame)
 //        {
 //           for(S32 i = 0; i < gameStats.teamStats.size(); i++)
 //           {
 //              gameStats.teamStats[i].playerStats.sort(playerScoreSort);

 //              for(S32 j = 0; j < gameStats.teamStats[i].playerStats.size(); j++)
 //              {
 //                 gameStats.teamStats[i].playerStats[j].gameResult = 
 //                    getResult(gameStats.teamStats[i].playerStats.size(), 
 //                              gameStats.teamStats[i].playerStats[0].points, 
 //                              gameStats.teamStats[i].playerStats.size() == 1 ? 0 : gameStats.teamStats[i].playerStats[1].points, 
 //                              gameStats.teamStats[i].playerStats[j].points, 
 //                              j == 0);
 //              }
 //           }
 //        }
 //     }

 //     DatabaseWriter dbWriter(gStatsDatabaseAddress.c_str(), gStatsDatabaseName.c_str(), 
 //                             gStatsDatabaseUsername.c_str(), gStatsDatabasePassword.c_str());  
 //     dbWriter.insertStats(gameStats);
 //  }

   void processIsAuthenticated(GameStats *gameStats)
   {
      for(S32 i = 0; i < gameStats->teamStats.size(); i++)
      {
         Vector<PlayerStats> *playerStats = &gameStats->teamStats[i].playerStats;

         for(S32 j=0; j < playerStats->size(); j++)
         {
            Nonce playerId = playerStats->get(j).nonce;
            MasterServerConnection *client = findClient(playerId);
            playerStats->get(j).isAuthenticated = (client && client->isAuthenticated());
         }
      }
   }


   TNL_DECLARE_RPC_OVERRIDE(s2mSendStatistics, (VersionedGameStats stats))
   {
      if(!stats.valid)
      {
         logprintf(LogConsumer::LogWarning, "Invalid stats %d %s %s", stats.version, getNetAddressString(), mPlayerOrServerName.getString());
         return;
      }

      GameStats *gameStats = &stats.gameStats;

      gameStats->serverIP = getNetAddressString();
      gameStats->serverName = mPlayerOrServerName.getString();
      gameStats->cs_protocol_version = mCSProtocolVersion;

      processIsAuthenticated(gameStats);
      processStatsResults(gameStats);

      DatabaseWriter databaseWriter;

      if(gWriteStatsToMySql)
      {
         databaseWriter = DatabaseWriter(gStatsDatabaseAddress.c_str(), gStatsDatabaseName.c_str(), 
                                         gStatsDatabaseUsername.c_str(), gStatsDatabasePassword.c_str());
      }
      else
      {
         databaseWriter = DatabaseWriter("stats.db");
      }


      // Will fail if compiled without database support and gWriteStatsToDatabase is true
      databaseWriter.insertStats(*gameStats);

   }


   // Game server wants to know if user name has been verified
   TNL_DECLARE_RPC_OVERRIDE(s2mRequestAuthentication, (Vector<U8> id, StringTableEntry name))
   {
      Nonce clientId(id);     // Reconstitute our id

      for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
         if(walk->mPlayerId == clientId)
         {
            AuthenticationStatus status;

            if(!stricmp(name.getString(), walk->mPlayerOrServerName.getString()) && walk->isAuthenticated())      // Need case insensitive comparison here
               status = AuthenticationStatusAuthenticatedName;

            // If server just restarted, clients will need to reauthenticate, and that may take some time.
            // We'll give them 90 seconds.
            else if(Platform::getRealMilliseconds() - gServerStartTime < 90 * 1000)      
               status = AuthenticationStatusTryAgainLater;             
            else
               status = AuthenticationStatusUnauthenticatedName;

            m2sSetAuthenticated(id, walk->mPlayerOrServerName, status);
            break;
         }
   }


   string cleanName(string name)
   {
      trim(name);
      if(name == "")
         return "ChumpChange";

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
         logprintf(LogConsumer::LogChat, "Relayed private msg from %s to %s: %s", mPlayerOrServerName.getString(), privateTo, strippedMessage);
      else
         logprintf(LogConsumer::LogChat, "Relayed chat msg from %s: %s", mPlayerOrServerName.getString(), message.getString());
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

   const U32 REWRITE_TIME = 5000;        // Rewrite status file at most this often (in ms)
   const U32 REREAD_TIME = 5000;         // How often to we re-read our config file? (in ms)

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

      for(S32 i = MasterServerConnection::gConnectList.size()-1; i >= 0; i--)
      {
         GameConnectRequest *request = MasterServerConnection::gConnectList[i];
         if(currentTime - request->requestTime > 5000) // 5 seconds
         {
            if(request->initiator.isValid())
            {
               ByteBufferPtr ptr = new ByteBuffer((U8 *) MasterRequestTimedOut, (U32) strlen(MasterRequestTimedOut) + 1);
                  //s2mRejectArrangedConnection(requestId, ptr);
               request->initiator->m2cArrangedConnectionRejected(request->initiatorQueryId, ptr);   // 0 = ReasonTimedOut
               request->initiator->removeConnectRequest(request);
            }
            if(request->host.isValid())
               request->host->removeConnectRequest(request);
            MasterServerConnection::gConnectList.erase_fast(i);
            delete request;
         }
      }

      Platform::sleep(1);
   }
   return 0;
}
