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
#include <stdio.h>
#include <string>
#include <time.h>

using namespace TNL;
using namespace std;

NetInterface *gNetInterface = NULL;

Vector<string> MOTDTypeVecOld;
Vector<string> MOTDStringVecOld;

Vector<U32> MOTDVersionVec;
Vector<string> MOTDStringVec;
U32 gLatestReleasedCSProtocol = 0;

const char *gMasterName;           // Name of the master server
string gJasonOutFile;              // File where JSON data gets dumped
bool gNeedToWriteStatus = true;    // Tracks whether we need to update our status file, for possible display on a website

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
   U32              mCSProtocolVersion; ///< Protocol version client will use to talk to server (client can only play with others using this same version)
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
   StringTableEntry mServerDescr;              ///< Server description
   bool isInGlobalChat;

   StringTableEntry mAutoDetectStr;             // Player's joystick autodetect string, for research purposes

   /// @}

public:

   static S32 gServerListCount;
   static S32 gClientListCount;

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
   }

   /// Destructor removes the connection from the doubly linked list of
   /// server connections.
   ~MasterServerConnection()
   {
      // unlink it if it's in the list
      mPrev->mNext = mNext;
      mNext->mPrev = mPrev;
      logprintf("[%s] %s disconnected", getTimeStamp().c_str(), getNetAddress().toString());

      if(mIsGameServer)
      {
         gServerListCount--;
         logprintf("[%s] Server disconnected", getTimeStamp().c_str());
      }
      else
      {
         gClientListCount--;
         logprintf("[%s] Client disconnected", getTimeStamp().c_str());
      }

      if(isInGlobalChat)
         for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
            if (walk->isInGlobalChat)
               walk->m2cPlayerLeftGlobalChat(mPlayerOrServerName);

      gNeedToWriteStatus = true;
   }

   /// Adds this connection to the doubly linked list of servers.
   void linkToServerList()
   {
      mNext = gServerList.mNext;
      mPrev = gServerList.mNext->mPrev;
      mNext->mPrev = this;
      mPrev->mNext = this;
      gServerListCount++;

      gNeedToWriteStatus = true;
      logprintf("[%s] Server connected [%s]", getTimeStamp().c_str(), mPlayerOrServerName.getString());
      logprintf("-> %s", mServerDescr.getString());
   }


   void linkToClientList()
   {
      mNext = gClientList.mNext;
      mPrev = gClientList.mNext->mPrev;
      mNext->mPrev = this;
      mPrev->mNext = this;
      gClientListCount++;

      gNeedToWriteStatus = true;
      logprintf("[%s] Client connected [%s]", getTimeStamp().c_str(), mPlayerOrServerName.getString());
   }


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
   // This gets updated whenver we gain or lose a server, at most every RewriteTime ms
   static void writeClientServerList_JSON()
   {
      if(gJasonOutFile == "")
         return;

      bool first = true;
      FILE *f = fopen(gJasonOutFile.c_str(), "w");
      if(f)
      {
         // First the servers
         fprintf(f, "{\n\t\"servers\": [");

            for(MasterServerConnection *walk = gServerList.mNext; walk != &gServerList; walk = walk->mNext)
            {
               fprintf(f, "%s\n\t\t{\n\t\t\t\"serverName\": \"%s\",\n\t\t\t\"protocolVersion\": %d,\n\t\t\t\"currentLevelName\": \"%s\",\n\t\t\t\"currentLevelType\": \"%s\",\n\t\t\t\"playerCount\": %d\n\t\t}",
                          first ? "":", ", walk->mPlayerOrServerName.getString(), walk->mCSProtocolVersion, walk->mLevelName.getString(), walk->mLevelType.getString(), walk->mPlayerCount);
               first = false;
            }

         // Next the player names
         fprintf(f, "\n\t],\n\t\"players\": [");

            first = true;
            for(MasterServerConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
            {
               fprintf(f, "%s\"%s\"", first ? "":", ", walk->mPlayerOrServerName.getString());
               first = false;
            }

         // Finally, the player and server counts
            fprintf(f, "],\n\t\"serverCount\": %d,\n\t\"playerCount\": %d\n}\n", MasterServerConnection::gServerListCount, MasterServerConnection::gClientListCount);

         fflush(f);
         fclose(f);
      }
      else
         logprintf("Could not write to JSON file %s...", gJasonOutFile.c_str());
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
   TNL_DECLARE_RPC_OVERRIDE(c2mRequestArrangedConnection, (U32 requestId,
      IPAddress remoteAddress, IPAddress internalAddress,
      ByteBufferPtr connectionParameters))
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

 
      logprintf("Client: %s requested connection to %s",
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

      logprintf("[%s] Server: %s accept connection request from %s", getTimeStamp().c_str(), buffer,
         req->initiator.isValid() ? req->initiator->getNetAddress().toString() : "Unknown");

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

      logprintf("[%s] Server: %s accept connection request from %s", getTimeStamp().c_str(), buffer,
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

      logprintf("[%s] Server: %s reject connection request from %s",
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

      logprintf("[%s] Server: %s reject connection request from %s",
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
   TNL_DECLARE_RPC_OVERRIDE(s2mUpdateServerStatus, (
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


   // Send player statistics to the master server
   TNL_DECLARE_RPC_OVERRIDE(s2mSendPlayerStatistics, (StringTableEntry playerName, Vector<U16> shots, Vector<U16> hits))
   {
      S32 totalShots = 0, totalHits = 0;

      for(S32 i = 0; i < shots.size(); i++)
      {
         totalShots += shots[i];
         totalHits += hits[i];
      }

      // Name | shots | hits
      logprintf("PLAYER: %s\t%d\t%d", playerName.getString(), totalShots, totalHits);
   }


   // TODO: Get this to be the same as UI::itos()
   // Convert int to string 
   string itos(S32 i)
   {
      char outString[100];
      dSprintf(outString, sizeof(outString), "%d", i);
      return outString;
   }


   // Send game statistics to the master server
   TNL_DECLARE_RPC_OVERRIDE(s2mSendGameStatistics, (StringTableEntry gameType, StringTableEntry levelName, 
                                                    RangedU32<0,MAX_PLAYERS> players, S16 timeInSecs))
   {
      string timestr = itos(timeInSecs / 60) + ":";
      timestr += ((timeInSecs % 60 < 10) ? "0" : "") + itos(timeInSecs % 60);

      // GameType | Time | Level name | players | time
      logprintf("GAME\t%s\t%s\t%s\t%d\t%s", getTimeStamp().c_str(), gameType.getString(), levelName.getString(), players, timestr.c_str() );
   }


   bool readConnectRequest(BitStream *bstream, const char **errorString)
   {
      if(!Parent::readConnectRequest(bstream, errorString))
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
         mPlayerOrServerName = readstr;

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
         mPlayerOrServerName = readstr;

         linkToClientList();
      }

      // Figure out which MOTD to send to client, based on game version (stored in mVersionString)
      //U32 matchLen = 0;
      string motdString = "Welcome to Bitfighter!";  // Default msg

      if(mCMProtocolVersion == 0)
         for(S32 i = 0; i < MOTDTypeVecOld.size(); i++)
         {
            //U32 len;
            //const char *type = MOTDTypeVecOld[i];
            //for(len = 0; type[len] == mVersionString.getString()[len] && type[len] != 0; len++)
            //   ;     // Do nothing... the action is in the for statement!

            //if(len > matchLen)
            //{
            //   matchLen = len;
            //   motdString = MOTDStringVecOld[i];
            //}

            if(!strcmp(mVersionString.getString(), MOTDTypeVecOld[i].c_str()))
            {
               motdString = MOTDStringVecOld[i];
               break;
            }
         }
      else     // mCMProtocolVersion >= 1
         for(S32 i = 0; i < MOTDVersionVec.size(); i++)
            if(mClientBuild == MOTDVersionVec[i])
            {
               motdString = MOTDStringVec[i];
               break;
            }

      m2cSetMOTD(gMasterName, motdString.c_str());

      if(mCMProtocolVersion >= 1)
         m2cSendUpdgradeStatus(gLatestReleasedCSProtocol > mCSProtocolVersion);   // Version 0 clients will disconnect if we try this

      if(mCMProtocolVersion == 0)
         logprintf("[%s] %s connected with protocol 0, version %s, from %s", getTimeStamp().c_str(), mIsGameServer ? "Server" : "Client", 
            mVersionString.getString(), getNetAddress().toString());
      else  // mCMProtocolVersion >= 1
         logprintf("[%s] %s connected with protocol %d, build %d, from %s, using %s",
                   getTimeStamp().c_str(),
                   mIsGameServer ? "Server":"Client", mCMProtocolVersion, mClientBuild, getNetAddress().toString(),
                   strcmp(mAutoDetectStr.getString(), "") ? mAutoDetectStr.getString():"<None>");
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
      char privateTo[MAX_SHORT_TEXT_LEN + 1];
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
                  if (j < MAX_SHORT_TEXT_LEN) privateTo[j++] = c;
            }
            else
               if (k < MAX_CHAT_MSG_LENGTH) strippedMessage[k++] = c;
         }

         if(buildingTo)       // If we're still in buildingTo mode by the time we get here, it means...
         {
            logprintf("malformed private message: %s", message.getString());
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
         logprintf("Relayed private msg from %s to %s: %s", mPlayerOrServerName.getString(), privateTo, strippedMessage);
      else
         logprintf("Relayed chat msg from %s: %s", mPlayerOrServerName.getString(), message.getString());
   }

   TNL_DECLARE_NETCONNECTION(MasterServerConnection);

};

TNL_IMPLEMENT_NETCONNECTION(MasterServerConnection, NetClassGroupMaster, true);

Vector< GameConnectRequest* > MasterServerConnection::gConnectList;

MasterServerConnection MasterServerConnection::gServerList;
MasterServerConnection MasterServerConnection::gClientList;

S32 MasterServerConnection::gServerListCount = 0;
S32 MasterServerConnection::gClientListCount = 0;


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

#include <stdio.h>

class StdoutLogConsumer : public LogConsumer
{
public:
   void logString(const char *string)
   {
      printf("%s", string);
   }
} gStdoutLogConsumer;


class FileLogConsumer : public LogConsumer    // Dumps logs to file
{
private:
   FILE *f;
public:
   FileLogConsumer(const char* logFile="bitfighter_master.log")
   {
      f = fopen(logFile, "a");
      logString("------ Bitfighter Master Server Log File ------");
   }

   ~FileLogConsumer()
   {
      if(f)
         fclose(f);
   }

   void logString(const char *string)
   {
      if(f)
      {
         fprintf(f, "%s", string);
         fflush(f);
      }
   }
} gFileLogConsumer;



enum {
   DefaultMasterPort = 25955,
};


U32 gMasterPort = DefaultMasterPort;

extern void readConfigFile();

int main(int argc, const char **argv)
{
   gMasterName = "Bitfighter Master Server";    // Default, can be overridden in cfg file

   // Parse command line parameters...
   readConfigFile();

   // Initialize our net interface so we can accept connections...
   gNetInterface = new NetInterface(Address(IPProtocol, Address::Any, gMasterPort));

   //for the master server alone, we don't need a key exchange - that would be a waste
   //gNetInterface->setRequiresKeyExchange(true);
   //gNetInterface->setPrivateKey(new AsymmetricKey(20));

   logprintf("[%s] Master Server %s started - listening on port %d", getTimeStamp().c_str(), gMasterName, gMasterPort);


   enum {
      RewriteTime = 5000,     // Rewrite status file at most this often (in ms)
      RereadTime = 5000,      // How often to we re-read our config file? (in ms)
   };

   U32 lastConfigReadTime = Platform::getRealMilliseconds();
   U32 lastWroteStatusTime = Platform::getRealMilliseconds() - RewriteTime;    // So we can do a write right off the bat

    // And until infinity, process whatever comes our way.
  for(;;)     // To infinity and beyond!!
   {
      U32 currentTime = Platform::getRealMilliseconds();
      gNetInterface->checkIncomingPackets();
      gNetInterface->processConnections();

      if(currentTime - lastConfigReadTime > RereadTime)     // Reread the config file every 5000ms
      {
         lastConfigReadTime = currentTime;
         readConfigFile();
      }

      if(gNeedToWriteStatus && currentTime - lastWroteStatusTime > RewriteTime)     // Write status file as need, at most every RewriteTime ms
      {
         lastWroteStatusTime = currentTime;
         MasterServerConnection::writeClientServerList_JSON();
         gNeedToWriteStatus = false;
      }

      Platform::sleep(1);
   }
   return 0;
}
