//------------------------------------------------------------------------------------
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


#include "MasterServerConnection.h"

#include "master.h"
#include "database.h"
#include "DatabaseAccessThread.h"
#include "authenticator.h"

#include "../zap/version.h"
#include "../zap/stringUtils.h"

#include "../boost/boost/shared_ptr.hpp"

using namespace DbWriter;

namespace Master 
{


// Define some statics
HighScores MasterServerConnection::highScores;
MasterServer *MasterServerConnection::mMaster = NULL;


/// Constructor initializes the linked list info with "safe" values
/// so we don't explode if we destruct right away.
MasterServerConnection::MasterServerConnection()
{
   mStrikeCount = 0; 
   mLastActivityTime = 0;
   setIsConnectionToClient();
   setIsAdaptive();
   isInGlobalChat = false;
   mAuthenticated = false;
   mIsDebugClient = false;
   mIsIgnoredFromList = false;
   mIsMasterAdmin = false;
   mLoggingStatus = "Not_Connected";
   mConnectionType = MasterConnectionTypeNone;
}


/// Destructor removes the connection from the doubly linked list of server connections
MasterServerConnection::~MasterServerConnection()
{
   // If we're in global chat, announce to anyone else in global chat that we are leaving
   if(isInGlobalChat)
   {
      const Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

      for(S32 i = 0; i < clientList->size(); i++)
         if(clientList->get(i)->isInGlobalChat && clientList->get(i) != this)
            clientList->get(i)->m2cPlayerLeftGlobalChat(mPlayerOrServerName);
   }


   // Remove this from the client/server lists
   if(mConnectionType == MasterConnectionTypeClient)
   {
      S32 index = mMaster->getClientList()->getIndex(this);
      if(index > -1)
         mMaster->removeClient(index);

   }
   else if(mConnectionType == MasterConnectionTypeServer)
   {
      S32 index = mMaster->getServerList()->getIndex(this);
      if(index > -1)
         mMaster->removeServer (index);
   }

   if(mLoggingStatus != "")
   {
      // CONNECT_FAILED | timestamp | address | logging status
      logprintf(LogConsumer::LogConnection, "CONNECT_FAILED\t%s\t%s\t%s", getTimeStamp().c_str(), 
                                                                           getNetAddress().toString(), 
                                                                           mLoggingStatus.c_str());
   }
   else if(mConnectionType == MasterConnectionTypeServer)
   {
      // SERVER_DISCONNECT | timestamp | server name
      logprintf(LogConsumer::LogConnection, "SERVER_DISCONNECT\t%s\t%s", getTimeStamp().c_str(), 
                                                                           mPlayerOrServerName.getString());
   }
   else if(mConnectionType == MasterConnectionTypeClient)
   {
      // CLIENT_DISCONNECT | timestamp | player name
      logprintf(LogConsumer::LogConnection, "CLIENT_DISCONNECT\t%s\t%s", getTimeStamp().c_str(), 
                                                                           mPlayerOrServerName.getString());
   }

   mMaster->writeJsonNow();
}


// Check username & password against database
MasterServerConnection::PHPBB3AuthenticationStatus MasterServerConnection::verifyCredentials(string &username, string password)

#ifdef VERIFY_PHPBB3    // Defined in Linux Makefile, not in VC++ project
{
   Authenticator authenticator;

   // Security levels: 0 = no security (no checking for sql-injection attempts, not recommended unless you add your own security)
   //          1 = basic security (prevents the use of any of these characters in the username: "(\"*^';&></) " including the space)
   //        1 = very basic security (prevents the use of double quote character)  <=== also level 1????   I think this can be deleted --> see comments in authenticator.initialize
   //          2 = alphanumeric (only allows alphanumeric characters in the username)
   //
   // We'll use level 1 for now, so users can put special characters in their username
   authenticator.initialize(mMaster->getSetting<string>("MySqlAddress"), 
                              mMaster->getSetting<string>("DbUsername"), 
                              mMaster->getSetting<string>("DbPassword"), 
                              mMaster->getSetting<string>("Phpbb3Database"), 
                              mMaster->getSetting<string>("Phpbb3TablePrefix"), 
                              1);

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
#else // verifyCredentials
{
   return Unsupported;
}
#endif


// TODO: Should we be reusing these?
DatabaseWriter getDatabaseWriter(const MasterSettings *settings)
{
   if(settings->getVal<YesNo>("WriteStatsToMySql"))
      return DatabaseWriter(settings->getVal<string>("StatsDatabaseAddress").c_str(), settings->getVal<string>("StatsDatabaseName").c_str(),
      settings->getVal<string>("StatsDatabaseUsername").c_str(), settings->getVal<string>("StatsDatabasePassword").c_str());
   else
      return DatabaseWriter("stats.db");
}


struct Auth_Stats : public DatabaseAccessThread::BasicEntry
{
   SafePtr<MasterServerConnection> client;
   Int<BADGE_COUNT> badges;
   U16 gamesPlayed;
   MasterServerConnection::PHPBB3AuthenticationStatus stat;
   string playerName;
   char password[256];


   Auth_Stats(const MasterSettings *settings): DatabaseAccessThread::BasicEntry(settings) {}    // Quickie constructor

   void run()
   {
      stat = MasterServerConnection::verifyCredentials(playerName, password);
      if(stat == MasterServerConnection::Authenticated)
      {
         DatabaseWriter databaseWriter = getDatabaseWriter(mSettings);
         badges = databaseWriter.getAchievements(playerName.c_str());
         gamesPlayed = databaseWriter.getGamesPlayed(playerName.c_str());
      }
      else
      {
         badges = NO_BADGES;
         gamesPlayed = 0;
      }
   }
   void finish()
   {
      StringTableEntry playerNameSTE(playerName.c_str());
      if(client) // Check for NULL, Sometimes, a client disconnects very fast
         client->processAutentication(playerNameSTE, stat, badges, gamesPlayed);
   }
};


MasterServerConnection::PHPBB3AuthenticationStatus MasterServerConnection::checkAuthentication(const char *password, bool doNotDelay)
{
   // Don't let username start with spaces or be zero length.
   if(mPlayerOrServerName.getString()[0] == ' ' || mPlayerOrServerName.getString()[0] == 0)
      return InvalidUsername;

   RefPtr<Auth_Stats> auth = new Auth_Stats(mMaster->getSettings());
   auth->client = this;
   auth->playerName = mPlayerOrServerName.getString();
   strncpy(auth->password, password, sizeof(auth->password));
   auth->stat = UnknownStatus;
   mMaster->getDatabaseAccessThread()->addEntry(auth);
   if(doNotDelay)  // Wait up to 1000 milliseconds so we can return some value, for clients version 017 and older
   {
      U32 timer = Platform::getRealMilliseconds();
      while(Platform::getRealMilliseconds() - timer < 1000 && auth->stat == UnknownStatus)
      {
         Platform::sleep(5);
      }
      return auth->stat;
   }
   return UnknownStatus;
}

Vector<string> master_admins;  // --> move to settings struct


void MasterServerConnection::processAutentication(StringTableEntry newName, PHPBB3AuthenticationStatus stat, 
                                                   TNL::Int<32> badges, U16 gamesPlayed)
{
   mBadges = NO_BADGES;
   if(stat == WrongPassword)
   {
      logprintf(LogConsumer::LogConnection, "User %s provided the wrong password", mPlayerOrServerName.getString());
      disconnect(ReasonBadLogin, "");
   }
   else if(stat == InvalidUsername)
   {
      logprintf(LogConsumer::LogConnection, "User name %s contains illegal characters", mPlayerOrServerName.getString());
      // Send message back to client to request new username/password
      disconnect(ReasonInvalidUsername, "");
   }
   else if(stat == Authenticated)
   {
      mIsIgnoredFromList = false;  // just for authenticating..
      logprintf(LogConsumer::LogConnection, "Authenticated user %s", mPlayerOrServerName.getString());
      mAuthenticated = true;
      mMaster->writeJsonNow();  // Make sure JSON shows authenticated state

      if(mPlayerOrServerName != newName)
      {
         if(isInGlobalChat)  // Need to tell clients new name, in case of delayed authentication
         {
            const Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

            for(S32 i = 0; i < clientList->size(); i++)
            {
               MasterServerConnection *client = clientList->get(i);

               if(client != this && client->isInGlobalChat)
               {
                  client->m2cPlayerLeftGlobalChat(mPlayerOrServerName);
                  client->m2cPlayerJoinedGlobalChat(newName);
               }
            }
         }
         mPlayerOrServerName = newName;
      }

      mBadges = badges;
      mGamesPlayed = gamesPlayed;

      if(mCMProtocolVersion <= 6)      // 018a ==> 6, 019 ==> 7
         m2cSetAuthenticated((U32)AuthenticationStatusAuthenticatedName, badges, newName.getString());
      else
         m2cSetAuthenticated_019((U32)AuthenticationStatusAuthenticatedName, badges, gamesPlayed, newName.getString());

      for(S32 i = 0; i < master_admins.size(); i++)  // check for master admin
         if(newName == master_admins[i])
         {
            mIsMasterAdmin = true;
            break;
         }
   }
   else if(stat == UnknownUser || stat == Unsupported)
   {
      if(mCMProtocolVersion <= 6)
         m2cSetAuthenticated(AuthenticationStatusUnauthenticatedName, NO_BADGES, "");
      else
         m2cSetAuthenticated_019(AuthenticationStatusUnauthenticatedName, NO_BADGES, 0, "");
   }

   else  // stat == CantConnect || stat == UnknownStatus
   {
      if(mCMProtocolVersion <= 6)
         m2cSetAuthenticated(AuthenticationStatusFailed, NO_BADGES, "");
      else
         m2cSetAuthenticated_019(AuthenticationStatusFailed, NO_BADGES, 0, "");
   }
}


// Client has contacted us and requested a list of active servers
// that match their criteria.
//
// The query server method builds a piecewise list of servers
// that match the client's particular filter criteria and
// sends it to the client, followed by a QueryServersDone RPC.
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, c2mQueryServers, (U32 queryId))
{
   Vector<IPAddress> theVector(IP_MESSAGE_ADDRESS_COUNT);
   theVector.reserve(IP_MESSAGE_ADDRESS_COUNT);

   const Vector<MasterServerConnection *> *serverList = mMaster->getServerList();

   for(S32 i = 0; i < serverList->size(); i++)
   {
      if(serverList->get(i)->mIsIgnoredFromList)  // hide hidden servers...
         continue;

      // First check the version -- we only want to match potential players that agree on which protocol to use
      if(serverList->get(i)->mCSProtocolVersion != mCSProtocolVersion)     // Incomptible protocols
         continue;

      // Somehow, despite it all, we matched.  Add us to the results list.
      theVector.push_back(serverList->get(i)->getNetAddress().toIPAddress());

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


void MasterServerConnection::setMasterServer(MasterServer *master)
{
   mMaster = master;
}


/// checkActivityTime validates that this particular connection is
/// not issuing too many requests at once in an attempt to DOS
/// by flooding either the master server or any other server
/// connected to it.  A client whose last activity time falls
/// within the specified delta gets a strike... 3 strikes and
/// you're out!  Strikes go away after being good for a while.
bool MasterServerConnection::checkActivityTime(U32 timeDeltaMinimum)
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
         return false;
      }
   }
   else if(mStrikeCount > 0)
      mStrikeCount--;

   mLastActivityTime = currentTime;
   return true;
}


void MasterServerConnection::removeConnectRequest(GameConnectRequest *gcr)
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


GameConnectRequest *MasterServerConnection::findAndRemoveRequest(U32 requestId)
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


MasterServerConnection *MasterServerConnection::findClient(Nonce &clientId)   // Should be const, but that won't compile for reasons not yet determined!!
{
   if(!clientId.isValid())
      return NULL;

   const Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

   for(S32 i = 0; i < clientList->size(); i++)
      if(clientList->get(i)->mPlayerId == clientId)
         return clientList->get(i);

   return NULL;
}


static bool listClient(MasterServerConnection *client)
{
   if(client->mIsIgnoredFromList)     // Skip clients with ignored flag set
      return false;

   if(client->mIsDebugClient)         // Skip clients running in debug mode
      return false;

   return true;
}


// Write a current count of clients/servers for display on a website, using JSON format
// This gets updated whenever we gain or lose a server, at most every 5 seconds (currently)
void MasterServerConnection::writeClientServerList_JSON()
{
   string jsonfile = mMaster->getSetting<string>("JsonOutfile");

   // Don't write if we don't have a file
   if(jsonfile == "")
      return;

   bool first = true;
   S32 playerCount = 0;
   S32 serverCount = 0;

   FILE *f = fopen(jsonfile.c_str(), "w");
   if(f)
   {
      // First the servers
      fprintf(f, "{\n\t\"servers\": [");

      const Vector<MasterServerConnection *> *serverList = mMaster->getServerList();

      for(S32 i = 0; i < serverList->size(); i++)
      {
         MasterServerConnection *server = serverList->get(i);

         if(server->mIsIgnoredFromList)
            continue;

         fprintf(f, "%s\n\t\t{\n\t\t\t\"serverName\": \"%s\",\n\t\t\t\"protocolVersion\": %d,\n\t\t\t\"currentLevelName\": \"%s\",\n\t\t\t\"currentLevelType\": \"%s\",\n\t\t\t\"playerCount\": %d\n\t\t}",
                     first ? "" : ", ", sanitizeForJson(server->mPlayerOrServerName.getString()),
                     server->mCSProtocolVersion, server->mLevelName.getString(), server->mLevelType.getString(), server->mPlayerCount);
         playerCount += server->mPlayerCount;
         serverCount++;
         first = false;
      }

      // Next the player names      // "players": [ "chris", "colin", "fred", "george", "Peter99" ],
      fprintf(f, "\n\t],\n\t\"players\": [");
      first = true;

      const Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

      for(S32 i = 0; i < clientList->size(); i++)
      {
         if(listClient(clientList->get(i)))
         {
            fprintf(f, "%s\"%s\"", first ? "" : ", ", sanitizeForJson(clientList->get(i)->mPlayerOrServerName.getString()));
            first = false;
         }
      }

      // Authentication status      // "authenticated": [ true, false, false, true, true ],
      fprintf(f, "],\n\t\"authenticated\": [");
      first = true;

      for(S32 i = 0; i < clientList->size(); i++)
      {
         if(listClient(clientList->get(i)))
         {
            fprintf(f, "%s%s", first ? "" : ", ", clientList->get(i)->mAuthenticated ? "true" : "false");
            first = false;
         }
      }

      // Finally, the player and server counts
      fprintf(f, "],\n\t\"serverCount\": %d,\n\t\"playerCount\": %d\n}\n", serverCount, playerCount);

      fflush(f);
      fclose(f);
   }
   else
      logprintf(LogConsumer::LogError, "Could not write to JSON file \"%s\"", jsonfile.c_str());
}

/*  Resulting JSON data should look like this:
{
   "servers": [
      {
         "serverName": "Spang",
         "protocolVersion": 23,
         "currentLevelName": "Triple Threat",
         "currentLevelType": "CTF",
         "playerCount": 2w
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

// This is called when a client wishes to arrange a connection with a server
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, c2mRequestArrangedConnection, 
                           (U32 requestId, IPAddress remoteAddress, IPAddress internalAddress, ByteBufferPtr connectionParameters) )
{
   // First, make sure that we're connected with the server that they're requesting a connection with
   MasterServerConnection *conn = (MasterServerConnection *) mMaster->getNetInterface()->findConnection(remoteAddress);
   if(!conn)
   {
      ByteBufferPtr ptr = new ByteBuffer((U8 *) MasterNoSuchHost, (U32) strlen(MasterNoSuchHost) + 1);
      m2cArrangedConnectionRejected(requestId, ptr);
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
   checkActivityTime(TWO_SECONDS);      

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

   conn->m2sClientRequestedArrangedConnection(req->hostQueryId, possibleAddresses, connectionParameters);
}


/// s2mAcceptArrangedConnection is sent by a server to notify the master that it will accept the connection
/// request from a client.  The requestId parameter sent by the MasterServer in m2sClientRequestedArrangedConnection
/// should be sent back as the requestId field.  The internalAddress is the server's self-determined IP address.

// Called to indicate a connect request is being accepted.
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, s2mAcceptArrangedConnection, (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData))
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
                                                      req->initiator.isValid() ? req->initiator->getNetAddress().toString() : "Unknown");

   // If we still know about the requestor, tell him his connection was accepted...
   if(req->initiator.isValid())
      req->initiator->m2cArrangedConnectionAccepted(req->initiatorQueryId, possibleAddresses, connectionData);

   delete req;
}



// s2mRejectArrangedConnection notifies the Master Server that the server is rejecting the arranged connection
// request specified by the requestId.  The rejectData will be passed along to the requesting client.
// Called to indicate a connect request is being rejected.
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, s2mRejectArrangedConnection, (U32 requestId, ByteBufferPtr rejectData))
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


// s2mUpdateServerStatus updates the status of a server to the Master Server, specifying the current game
// and mission types, any player counts and the current info flags.
// Updates the master with the current status of a game server.
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, s2mUpdateServerStatus, (StringTableEntry levelName, StringTableEntry levelType,
                                                   U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags))
{
   // Only accept updates from game servers
   if(mConnectionType != MasterConnectionTypeServer)
      return;

   // Only update if anything has changed
   if(mLevelName   != levelName   || mLevelType  != levelType  || mNumBots   != botCount   || 
      mPlayerCount != playerCount || mMaxPlayers != maxPlayers || mInfoFlags != infoFlags )
   {
      mLevelName = levelName;
      mLevelType = levelType;

      mNumBots     = botCount;
      mPlayerCount = playerCount;
      mMaxPlayers  = maxPlayers;
      mInfoFlags   = infoFlags;

      // Check to ensure we're not getting flooded with these requests
      checkActivityTime(FOUR_SECONDS);

      mMaster->writeJsonNow();
   }
}


////////////////////////////////////////
////////////////////////////////////////

Int<BADGE_COUNT> MasterServerConnection::getBadges()
{
   if(mCSProtocolVersion <= 35) // bitfighter 017 limited to display only some badges
      return mBadges & (BIT(DEVELOPER_BADGE) | BIT(FIRST_VICTORY) | BIT(BADGE_TWENTY_FIVE_FLAGS));

   return mBadges;
}


U16 MasterServerConnection::getGamesPlayed()
{
   return mGamesPlayed;
}


void MasterServerConnection::processIsAuthenticated(GameStats *gameStats)
{
   for(S32 i = 0; i < gameStats->teamStats.size(); i++)
   {
      Vector<PlayerStats> *playerStats = &gameStats->teamStats[i].playerStats;

      for(S32 j = 0; j < playerStats->size(); j++)
      {
         Nonce playerId = playerStats->get(j).nonce;
         MasterServerConnection *client = findClient(playerId);
         playerStats->get(j).isAuthenticated = (client && client->isAuthenticated());
      }
   }
}



struct AddGameReport : public DatabaseAccessThread::BasicEntry
{
   GameStats mStats;

   AddGameReport(const MasterSettings *settings) : DatabaseAccessThread::BasicEntry(settings) { }    // Quickie constructor

   void run()
   {
      DatabaseWriter databaseWriter = getDatabaseWriter(mSettings);
      // Will fail if compiled without database support and gWriteStatsToDatabase is true
      databaseWriter.insertStats(mStats);
   }
};

void MasterServerConnection::writeStatisticsToDb(VersionedGameStats &stats)
{
   if(!checkActivityTime(SIX_SECONDS))
      return;

   if(!stats.valid)
   {
      logprintf(LogConsumer::LogWarning, "Invalid stats: version %d, %s %s", stats.version, getNetAddressString(), mPlayerOrServerName.getString());
      return;
   }

   GameStats *gameStats = &stats.gameStats;

   gameStats->serverIP = getNetAddressString();
   gameStats->serverName = mPlayerOrServerName.getString();
   gameStats->cs_protocol_version = mCSProtocolVersion;

   processIsAuthenticated(gameStats);
   processStatsResults(gameStats);

   RefPtr<AddGameReport> gameReport = new AddGameReport(mMaster->getSettings());
   gameReport->mStats = *gameStats;  // copy so we keep data during a thread
   mMaster->getDatabaseAccessThread()->addEntry(gameReport);
}

   
struct AchievementWriter : public DatabaseAccessThread::BasicEntry
{
   U8 achievementId;
   StringTableEntry playerNick;
   StringTableEntry mPlayerOrServerName;
   string addressString;

   AchievementWriter(const MasterSettings *settings) : DatabaseAccessThread::BasicEntry(settings) { }    // Quickie constructor

   void run()
   {
      DatabaseWriter databaseWriter = getDatabaseWriter(mSettings);
      // Will fail if compiled without database support and gWriteStatsToDatabase is true
      databaseWriter.insertAchievement(achievementId, playerNick.getString(), mPlayerOrServerName.getString(), addressString);
   }
};


void MasterServerConnection::writeAchievementToDb(U8 achievementId, const StringTableEntry &playerNick)
{
   if(!checkActivityTime(6 * 1000))  // 6 seconds
      return;

   // Basic sanity check
   if(playerNick == "")
      return;

   RefPtr<AchievementWriter> a_writer = new AchievementWriter(mMaster->getSettings());
   a_writer->achievementId = achievementId;
   a_writer->playerNick = playerNick;
   a_writer->mPlayerOrServerName = mPlayerOrServerName;
   a_writer->addressString = getNetAddressString();
   mMaster->getDatabaseAccessThread()->addEntry(a_writer);
}


struct LevelInfoWriter : public DatabaseAccessThread::BasicEntry
{
   string hash;
   string levelName;
   string creator;
   StringTableEntry gameType;
   bool hasLevelGen;
   U8 teamCount;
   S32 winningScore;
   S32 gameDurationInSeconds;

   LevelInfoWriter(const MasterSettings *settings) : DatabaseAccessThread::BasicEntry(settings) { }    // Quickie constructor

   void run()
   {
      DatabaseWriter databaseWriter = getDatabaseWriter(mSettings);
      // Will fail if compiled without database support and gWriteStatsToDatabase is true
      databaseWriter.insertLevelInfo(hash, levelName, creator, gameType.getString(), hasLevelGen, teamCount, winningScore, gameDurationInSeconds);
   }
};


void MasterServerConnection::writeLevelInfoToDb(const string &hash, const string &levelName, const string &creator, 
                                                const StringTableEntry &gameType, bool hasLevelGen, U8 teamCount, S32 winningScore, 
                                                S32 gameDurationInSeconds)
{
   if(!checkActivityTime(SIX_SECONDS))
      return;

   // Basic sanity check
   if(hash == "" || gameType == "")
      return;

   RefPtr<LevelInfoWriter> l_writer = new LevelInfoWriter(mMaster->getSettings());
   l_writer->hash = hash;
   l_writer->levelName = levelName;
   l_writer->creator = creator;
   l_writer->gameType = gameType;
   l_writer->hasLevelGen = hasLevelGen;
   l_writer->teamCount = teamCount;
   l_writer->winningScore = winningScore;
   l_writer->gameDurationInSeconds = gameDurationInSeconds;
   mMaster->getDatabaseAccessThread()->addEntry(l_writer);
}


struct HighScoresReader : public DatabaseAccessThread::BasicEntry
{
   S32 scoresPerGroup;

   // Constructor
   HighScoresReader(const MasterSettings *settings, S32 scoresPerGroup) : DatabaseAccessThread::BasicEntry(settings)        
   {
      this->scoresPerGroup = scoresPerGroup;
   }

   void run()
   {
      DatabaseWriter databaseWriter = getDatabaseWriter(mSettings);

      // Client will display these in two columns, row by row

      MasterServerConnection::highScores.groupNames.clear();
      MasterServerConnection::highScores.names.clear();
      MasterServerConnection::highScores.scores.clear();

      MasterServerConnection::highScores.groupNames.push_back("Official Wins Last Week");
      databaseWriter.getTopPlayers("v_last_week_top_player_official_wins", "win_count",  
                                    scoresPerGroup, MasterServerConnection::highScores.names, 
                                    MasterServerConnection::highScores.scores);

      MasterServerConnection::highScores.groupNames.push_back("Official Wins This Week, So Far");
      databaseWriter.getTopPlayers("v_current_week_top_player_official_wins", "win_count",  
                                    scoresPerGroup, MasterServerConnection::highScores.names, 
                                    MasterServerConnection::highScores.scores);

      MasterServerConnection::highScores.groupNames.push_back("Games Played Last Week");
      databaseWriter.getTopPlayers("v_last_week_top_player_games", "game_count", 
                                    scoresPerGroup, MasterServerConnection::highScores.names, 
                                    MasterServerConnection::highScores.scores);

      MasterServerConnection::highScores.groupNames.push_back("Games Played This Week, So Far");
      databaseWriter.getTopPlayers("v_current_week_top_player_games", "game_count", 
                                    scoresPerGroup, MasterServerConnection::highScores.names,
                                    MasterServerConnection::highScores.scores);

      MasterServerConnection::highScores.groupNames.push_back("Latest BBB Winners");
      databaseWriter.getTopPlayers("v_latest_bbb_winners", "rank", 
                                    scoresPerGroup, MasterServerConnection::highScores.names, 
                                    MasterServerConnection::highScores.scores);

      MasterServerConnection::highScores.scoresPerGroup = scoresPerGroup;
   }


   void finish()
   {
      MasterServerConnection::highScores.isBusy = false;

      for(S32 i = 0; i < MasterServerConnection::highScores.waitingClients.size(); i++)
         if(MasterServerConnection::highScores.waitingClients[i])
            MasterServerConnection::highScores.waitingClients[i]->m2cSendHighScores(MasterServerConnection::highScores.groupNames, 
                                                                                    MasterServerConnection::highScores.names, 
                                                                                    MasterServerConnection::highScores.scores);
      MasterServerConnection::highScores.waitingClients.clear();
   }
};


////////////////////////////////////////
////////////////////////////////////////

typedef map<U32, boost::shared_ptr<TotalLevelRating> > TotalLevelRatingsMap;
static TotalLevelRatingsMap totalLevelRatingsCache;

   
struct TotalLevelRatingsReader : public DatabaseAccessThread::BasicEntry
{
   U32 dbId;
   S16 rating;

   TotalLevelRatingsReader(const MasterSettings *settings, U32 databaseId) : DatabaseAccessThread::BasicEntry(settings)    // Constructor
   {
      dbId = databaseId;
   }

   void run()
   {
      rating = getDatabaseWriter(mSettings).getLevelRating(dbId);
   }


   void finish()
   {
      TotalLevelRating *totalRating = totalLevelRatingsCache[dbId].get();
      totalRating->rating = rating;
      totalRating->isBusy = false;

      for(S32 i = 0; i < totalRating->waitingClients.size(); i++)
         if(totalRating->waitingClients[i])
            totalRating->waitingClients[i]->m2cSendTotalLevelRating(dbId, rating);

         totalRating->waitingClients.clear();
         
   }
};


////////////////////////////////////////
////////////////////////////////////////

typedef pair<U32, StringTableEntry> DbIdPlayerNamePair;
typedef map<DbIdPlayerNamePair, boost::shared_ptr<PlayerLevelRating> > PlayerLevelRatingsMap;
static PlayerLevelRatingsMap playerLevelRatingsCache;


struct PlayerLevelRatingsReader : public DatabaseAccessThread::BasicEntry
{
   U32 dbId;
   StringTableEntry playerName;
   S32 rating;

   // Constructor
   PlayerLevelRatingsReader(const MasterSettings *settings, U32 databaseId, const StringTableEntry &playerName) : 
         DatabaseAccessThread::BasicEntry(settings)   
   {
      dbId = databaseId;
      this->playerName = playerName;
   }

   void run()
   {
      rating = getDatabaseWriter(mSettings).getLevelRating(dbId, playerName);
   }

   void finish()
   {
      PlayerLevelRating *playerRating = playerLevelRatingsCache[DbIdPlayerNamePair(dbId, playerName)].get();
      playerRating->rating = rating;
      playerRating->isBusy = false;

      for(S32 i = 0; i < playerRating->waitingClients.size(); i++)
         if(playerRating->waitingClients[i])
            playerRating->waitingClients[i]->m2cSendPlayerLevelRating(dbId, rating);

      playerRating->waitingClients.clear();
   }
};


////////////////////////////////////////
////////////////////////////////////////

HighScores *MasterServerConnection::getHighScores(S32 scoresPerGroup)
{
   // Remember... highScores is static!
   if(!highScores.isValid || highScores.isExpired() || scoresPerGroup != highScores.scoresPerGroup)
      if(!highScores.isBusy)
      {
         highScores.isBusy = true;
         highScores.isValid = true;
         highScores.lastClock = Platform::getRealMilliseconds();

         RefPtr<HighScoresReader> highScoreReader = new HighScoresReader(mMaster->getSettings(), scoresPerGroup);
         mMaster->getDatabaseAccessThread()->addEntry(highScoreReader);
      }
      
   return &highScores;
}


// Note: Will return NULL if databaseId == NOT_IN_DATABASE.  Otherwise, will not.
TotalLevelRating *MasterServerConnection::getLevelRating(U32 databaseId)
{
   if(databaseId == NOT_IN_DATABASE)
      return NULL;

   // Note that while map[xxx] will create an entry if it does not exist, in this case, it will create a boost::shared_ptr
   // wrapping a NULL object.  So rating, which points to that object, will also be NULL.  Viva la confusion!
   TotalLevelRating *rating = totalLevelRatingsCache[databaseId].get();

   if(!rating)
   {
      boost::shared_ptr<TotalLevelRating> newRating = boost::shared_ptr<TotalLevelRating>(new TotalLevelRating());
      totalLevelRatingsCache[databaseId] = newRating;
      rating = newRating.get();
   }

   if(!rating->isValid || rating->isExpired())
      if(!rating->isBusy)
      {
         rating->isBusy = true;
         rating->isValid = true;
         rating->lastClock = Platform::getRealMilliseconds();

         RefPtr<TotalLevelRatingsReader> totalLevelRatingsReader = new TotalLevelRatingsReader(mMaster->getSettings(), databaseId);
         mMaster->getDatabaseAccessThread()->addEntry(totalLevelRatingsReader);
      }

   return rating;
}


// Send this connection the level rating for the specified player
// Note: Will return NULL if databaseId == NOT_IN_DATABASE.  Otherwise, will not.
PlayerLevelRating *MasterServerConnection::getLevelRating(U32 databaseId, const StringTableEntry &playerName)
{
   if(databaseId == NOT_IN_DATABASE)
      return NULL;

   // Note that while map[xxx] will create an entry if it does not exist, in this case, it will create a boost::shared_ptr
   // wrapping a NULL object.  So rating, which points to that object, will also be NULL.  Viva la confusion!
   PlayerLevelRating *rating = playerLevelRatingsCache[DbIdPlayerNamePair(databaseId, playerName)].get();

   if(!rating)
   {
      boost::shared_ptr<PlayerLevelRating> newRating = boost::shared_ptr<PlayerLevelRating>(new PlayerLevelRating());
      playerLevelRatingsCache[DbIdPlayerNamePair(databaseId, playerName)] = newRating;
      rating = newRating.get();
   }

   if(!rating->isValid || rating->isExpired())
      if(!rating->isBusy)
      {
         rating->isBusy = true;
         rating->isValid = true;
         rating->lastClock = Platform::getRealMilliseconds();

         RefPtr<TotalLevelRatingsReader> totalLevelRatingsReader = new TotalLevelRatingsReader(mMaster->getSettings(), databaseId);
         mMaster->getDatabaseAccessThread()->addEntry(totalLevelRatingsReader);
      }

   return rating;
}



// Cycle through and remove expired cache entries -- static method
void MasterServerConnection::removeOldEntriesFromRatingsCache()
{
   {
      TotalLevelRatingsMap::iterator it;

      for(TotalLevelRatingsMap::iterator it = totalLevelRatingsCache.begin(); it != totalLevelRatingsCache.end(); it++)
         if(it->second->isValid && !it->second->isBusy && it->second->isExpired())
            totalLevelRatingsCache.erase(it);
   }

   {
   PlayerLevelRatingsMap::iterator it;

   for(PlayerLevelRatingsMap::iterator it = playerLevelRatingsCache.begin(); it != playerLevelRatingsCache.end(); it++)
      if(it->second->isValid && !it->second->isBusy && it->second->isExpired())
         playerLevelRatingsCache.erase(it);
   }
}


////////////////////////////////////////
////////////////////////////////////////


TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, s2mSendStatistics, (VersionedGameStats stats))
{
   writeStatisticsToDb(stats);
   highScores.isValid = false;
}


TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, s2mAcheivementAchieved, (U8 achievementId, StringTableEntry playerNick))
{
   if(achievementId > BADGE_COUNT)  // Check for out of range badges
      return;

   writeAchievementToDb(achievementId, playerNick);

   const Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

   for(S32 i = 0; i < clientList->size(); i++)
      if(clientList->get(i)->mPlayerOrServerName == playerNick)
      {
         clientList->get(i)->mBadges = mBadges | BIT(achievementId); // Add to local variable without needing to reload from database
         break;
      }
}


TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, s2mSendLevelInfo, 
                           (string hash, string levelName, string creator, 
                              StringTableEntry gameType, bool hasLevelGen, U8 teamCount, S32 winningScore, S32 gameDurationInSeconds))
{
   writeLevelInfoToDb(hash, levelName, creator, gameType, hasLevelGen, teamCount, winningScore, gameDurationInSeconds);
}


TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, c2mRequestHighScores, ())
{

   if(!highScores.isBusy)     // Not busy, so value must be cached.  Send the scores now.
   {
      HighScores *highScoreGroup = getHighScores(3);
      m2cSendHighScores(highScoreGroup->groupNames, highScoreGroup->names, highScoreGroup->scores);
   }

   else                       // In the process of retrieving... highScores will send later when retrieval is complete
   {
      bool alreadyOnList = false;
      for(S32 i = 0; i < highScores.waitingClients.size(); i++)
         if(highScores.waitingClients[i] == this)
         {
            alreadyOnList = true;
            break;
         }

         if(!alreadyOnList)
            highScores.waitingClients.push_back(this);
   }
}


// We got a request to send the server's rating for the current level.  We will send two messages in response, one with the
// individual rating, on with the overall average rating.
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, c2mRequestLevelRating, (U32 databaseId))
{
   // Do nothing if client sent us an invalid id
   if(databaseId == NOT_IN_DATABASE)
      return;

   TotalLevelRating *totalRating = getLevelRating(databaseId);

   TNLAssert(totalRating, "totalRating should not be NULL!");

   if(!totalRating->isBusy)     // Not busy, so value must be cached.  Send it now.
   {
      logprintf("Using cached value of total rating!");
      m2cSendTotalLevelRating(databaseId, totalRating->rating);
   }

   else                    // Otherwise, add it to the list to send when the database request is complete
   {
      bool alreadyOnList = false;
      for(S32 i = 0; i < totalRating->waitingClients.size(); i++)
         if(totalRating->waitingClients[i] == this)
            {
               alreadyOnList = true;
               break;
            }

      if(!alreadyOnList)
         totalRating->waitingClients.push_back(this);
   }


   PlayerLevelRating *playerRating = getLevelRating(databaseId, mPlayerOrServerName);

   TNLAssert(playerRating, "plyrRating should not be NULL!");

   if(!playerRating->isBusy)
   {
      sendPlayerLevelRating(databaseId, playerRating->rating);
   }

   else
   {
      bool alreadyOnList = false;

      for(S32 i = 0; i < playerRating->waitingClients.size(); i++)
      if(playerRating->waitingClients[i] == this)
         {
            alreadyOnList = true;
            break;
         }

      if(!alreadyOnList)
         playerRating->waitingClients.push_back(this);
   }
}


void MasterServerConnection::sendPlayerLevelRating(U32 databaseId, S32 rating)
{
   // Make sure ranges are ok... none of these conditions should ever occur.  But this is Bitfighter...
   if(rating < -1)      rating = -1;
   else if(rating > 1)  rating = 1;

   // Normalize to the datatype needed for sending
   RangedU32<0, 2>   normalizedRating = rating + 1;

   m2cSendPlayerLevelRating(databaseId, normalizedRating);
}


TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, c2mRequestMOTD, ())
{
   sendMotd();
}


// Game server wants to know if user name has been verified
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, s2mRequestAuthentication, (Vector<U8> id, StringTableEntry name))
{
   Nonce clientId(id);     // Reconstitute our id

   const Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

   for(S32 i = 0; i < clientList->size(); i++)
   {
      MasterServerConnection *client = clientList->get(i);
      if(client->mPlayerId == clientId)
      {
         AuthenticationStatus status;

         // Need case insensitive comparison here
         if(!stricmp(name.getString(), client->mPlayerOrServerName.getString()) && client->isAuthenticated())
            status = AuthenticationStatusAuthenticatedName;

         // If server just restarted, clients will need to reauthenticate, and that may take some time.
         // We'll give them 90 seconds.
         else if(Platform::getRealMilliseconds() - mMaster->getStartTime() < 90 * 1000)
            status = AuthenticationStatusTryAgainLater;
         else
            status = AuthenticationStatusUnauthenticatedName;

         if(mCMProtocolVersion <= 6)      // 018a ==> 6, 019 ==> 7
            m2sSetAuthenticated(id, client->mPlayerOrServerName, status, client->getBadges());
         else
            m2sSetAuthenticated_019(id, client->mPlayerOrServerName, status, client->getBadges(), client->getGamesPlayed());
         break;
      }
   }
}


// Remove leading/trailing spaces, provide default if name is empty
string MasterServerConnection::cleanName(string name)    // Makes copy of name that we can alter
{
   trim(name);
   if(name == "")
      return "ChumpChange";

   return name;
}

map<U32, string> gMOTDClientMap;

void MasterServerConnection::sendMotd()
{
   // Figure out which MOTD to send to client, based on game version (stored in mVersionString)
   string motdString = "Welcome to Bitfighter!";  // Default msg

   if(gMOTDClientMap[mClientBuild] != "")
      motdString = gMOTDClientMap[mClientBuild];

   m2cSetMOTD(mMaster->getSetting<string>("ServerName"), motdString.c_str());     // Even level 0 clients can handle this
}


Vector<Address> gListAddressHide;  // --> move to settings struct


// Must match MasterServerConnection::writeConnectRequest()!!
bool MasterServerConnection::readConnectRequest(BitStream *bstream, NetConnection::TerminationReason &reason)
{
   mLoggingStatus = "Something failed in readConnectRequest";
   if(!Parent::readConnectRequest(bstream, reason))
   {
      mLoggingStatus = "Parent::readConnectRequest failed";
      return false;
   }

   // Note that if player is hosting a game interactively (i.e. with host option on main menu), they
   // will create two connections here, one for the host, and one for the client instance.
   char readstr[256]; // to hold the string being read

   bstream->read(&mCMProtocolVersion);    // Version of protocol we'll use with the client
   if(mCMProtocolVersion < 4 || mCMProtocolVersion > MASTER_PROTOCOL_VERSION) // check for unsupported version
   {
      mLoggingStatus = "Bad version";
      return false;
   }

   bstream->read(&mCSProtocolVersion);    // Protocol this client uses for C-S communication
   bstream->read(&mClientBuild);          // Client's build number

   // In protocol 6 we've added the concept of a connection type
   if(mCMProtocolVersion >= 6)
      mConnectionType = (MasterConnectionType) bstream->readEnum(MasterConnectionTypeCount);

   // Protocol 5 and earlier only sent 1 bit to determine if it was a game server
   else
   {
      if(bstream->readFlag())
         mConnectionType = MasterConnectionTypeServer;
      else
         mConnectionType = MasterConnectionTypeClient;
   }

   // Now read the bitstream and do other logic according to the connection type
   switch(mConnectionType)
   {
      // If it's a game server, read status info...
      case MasterConnectionTypeServer:
      {
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
         mServerDescr = readstr;

         mMaster->addServer(this);
         mMaster->writeJsonNow();

         // SERVER_CONNECT | timestamp | server name | server description
         logprintf(LogConsumer::LogConnection, "SERVER_CONNECT\t%s\t%s\t%s", getTimeStamp().c_str(), mPlayerOrServerName.getString(), mServerDescr.getString());

      }
      break;

      case MasterConnectionTypeClient:
      {
         bstream->readString(readstr);          // Client's joystick autodetect string
         mAutoDetectStr = readstr;

         bstream->readString(readstr);
         mPlayerOrServerName = cleanName(readstr).c_str();

         bstream->readString(readstr); // the last "readstr" for "password" in startAuthentication

         // Read client flags -- only the first is currently used
         if(mCMProtocolVersion >= 6)
         {
            U8 flags = bstream->readInt(8);
            mIsDebugClient = flags & ClientDebugModeFlag;
         }

         mPlayerId.read(bstream);

         // Probably redundant, but let's cycle through all our clients and make sure the playerId is unique.  With 2^64
         // possibilities, it most likely will be.
         const Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

         for(S32 i = 0; i < clientList->size(); i++)
            if(clientList->get(i) != this && clientList->get(i)->mPlayerId == mPlayerId)
            {
               logprintf(LogConsumer::LogConnection, "User %s provided duplicate id to %s", mPlayerOrServerName.getString(),
                                                                                             clientList->get(i)->mPlayerOrServerName.getString());
               disconnect(ReasonDuplicateId, "");
               reason = ReasonDuplicateId;
               return false;
            }

         // Start the authentication by reading database on seperate thread
         // On clients 017 and older, they completely ignore any disconnect reason once fully connected,
         // so we pause waiting for database instead of fully connecting yet.

         for(S32 i = 0; i < gListAddressHide.size(); i++)
            if(getNetAddress().isEqualAddress(gListAddressHide[i]))
               mIsIgnoredFromList = true;

         switch(checkAuthentication(readstr, mCSProtocolVersion <= 35)) // readstr is password
         {
            case WrongPassword:   
               reason = ReasonBadLogin;        
               return false;

            case InvalidUsername: 
               reason = ReasonInvalidUsername; 
               return false;

            case UnknownStatus: 
               mMaster->addClient(this);

               // CLIENT_CONNECT | timestamp | player name
               logprintf(LogConsumer::LogConnection, "CLIENT_CONNECT\t%s\t%s", getTimeStamp().c_str(), mPlayerOrServerName.getString());

               mMaster->writeJsonDelayed();  // Delay writing JSON to reduce chances of incorrectly showing new player as unauthenticated
               break;

            case Authenticated: 
               mMaster->addClient(this);

               // CLIENT_CONNECT | timestamp | player name
               logprintf(LogConsumer::LogConnection, "CLIENT_CONNECT\t%s\t%s", getTimeStamp().c_str(), mPlayerOrServerName.getString());

               mMaster->writeJsonNow();      // Write immediately  
               break;

            case CantConnect:
            case UnknownUser:
            case Unsupported:
               // Do nothing
               break;
         }

         // If client needs to upgrade, tell them
         m2cSendUpdgradeStatus(mMaster->getSetting<U32>("LatestReleasedCSProtocol")   > mCSProtocolVersion || 
                                 mMaster->getSetting<U32>("LatestReleasedBuildVersion") > mClientBuild);

         // Send message of the day
         sendMotd();
      }
      break;

      case MasterConnectionTypeAnonymous:
      default:
         // Do nothing!
         break;
   }


   // Log our connection if from client or server
   if(mConnectionType == MasterConnectionTypeServer || mConnectionType == MasterConnectionTypeClient)
   {
      // CLIENT/SERVER_INFO | timestamp | protocol version | build number | address | controller
      logprintf(LogConsumer::LogConnection, "%s\t%s\t%d\t%d\t%s\t%s",
            mConnectionType == MasterConnectionTypeServer ? "SERVER_INFO" : "CLIENT_INFO",
            getTimeStamp().c_str(), mCMProtocolVersion, mClientBuild, getNetAddress().toString(),
            strcmp(mAutoDetectStr.getString(), "") ? mAutoDetectStr.getString():"<None>");
   }

   mLoggingStatus = "";  // No issues!

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


TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, c2mJoinGlobalChat, ())
{
   Vector<StringTableEntry> names;

   const Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

   for(S32 i = 0; i < clientList->size(); i++)
      if(clientList->get(i) != this && clientList->get(i)->isInGlobalChat)
         names.push_back(clientList->get(i)->mPlayerOrServerName);

   if(names.size() > 0)
      m2cPlayersInGlobalChat(names);   // Send to this client, to avoid blank name list of quickly leave/join

   if(mIsIgnoredFromList)              // don't list name in lobby, too.
      return;

   mLeaveGlobalChatTimer = 0;          // don't continue with delayed chat leave.
   if(isInGlobalChat)                  // Already in Global Chat
      return;

   isInGlobalChat = true;
      
   for(S32 i = 0; i < clientList->size(); i++)
      if(clientList->get(i) != this && clientList->get(i)->isInGlobalChat)
         clientList->get(i)->m2cPlayerJoinedGlobalChat(mPlayerOrServerName);

}


TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, c2mLeaveGlobalChat, ())
{
   if(!isInGlobalChat)  // was not in Global Chat
      return;

   // using delayed leave, to avoid quickly join / leave problem.
   mLeaveGlobalChatTimer = Platform::getRealMilliseconds() - 20; // "-20" make up for inaccurate getRealMilliseconds going backwards by 1 or 2 milliseconds.
   gLeaveChatTimerList.push_back(this);

   //isInGlobalChat = false;
   //Vector<MasterServerConnection *> *clientList = mMaster->getClientList();
   //for(S32 i = 0; i < clientList->size(); i++)
   //   if (clientList->get(i) != this && clientList->get(i)->isInGlobalChat)
   //      clientList->get(i)->m2cPlayerLeftGlobalChat(mPlayerOrServerName);
}


// Got out-of-game chat message from client, need to relay it to others
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, c2mSendChat, (StringPtr message))
{
   // Variables needed for private message handling
   bool isPrivate = false;
   string pmRecipient;
   const char *strippedMessage = NULL;

   bool badCommand = false;

   // Incoming command!
   if(message.getString()[0] == '/')
   {
      Vector<string> words = parseStringAndStripLeadingSlash(message.getString());

      string command = lcase(words[0]);

      // Master Admin-only commands
      if(mIsMasterAdmin)
      {
         bool adminCommand = true;  // Assume admin command

         if(command == "dropserver")
         {
            bool droppedServer = false;
            Address addr(words[1].c_str());

            const Vector<MasterServerConnection *> *serverList = mMaster->getServerList();

            for(S32 i = 0; i < serverList->size(); i++)
            {
               MasterServerConnection *server = serverList->get(i);

               if(server->getNetAddress().isEqualAddress(addr) && (addr.port == 0 || addr.port == server->getNetAddress().port))
               {
                  server->mIsIgnoredFromList = true;
                  m2cSendChat(server->mPlayerOrServerName, true, "dropped");
                  droppedServer = true;
               }
            }

            if(!droppedServer)
               m2cSendChat(mPlayerOrServerName, true, "dropserver: address not found");
         }
         else if(command == "restoreservers")
         {
            bool broughtBackServer = false;

            const Vector<MasterServerConnection *> *serverList = mMaster->getServerList();

            for(S32 i = 0; i < serverList->size(); i++)
               if(serverList->get(i)->mIsIgnoredFromList)
               {
                  broughtBackServer = true;
                  serverList->get(i)->mIsIgnoredFromList = false;
                  m2cSendChat(serverList->get(i)->mPlayerOrServerName, true, "servers restored");
               }
            if(!broughtBackServer)
               m2cSendChat(mPlayerOrServerName, true, "No server was hidden");
         }
         else if(command == "hideplayer")
         {
            bool found = false;
            const Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

            for(S32 i = 0; i < clientList->size(); i++)
            {
               MasterServerConnection *client = clientList->get(i);
               if(strcmp(words[1].c_str(), client->mPlayerOrServerName.getString()) == 0)
               {
                  client->mIsIgnoredFromList = !client->mIsIgnoredFromList;
                  m2cSendChat(client->mPlayerOrServerName, true, client->mIsIgnoredFromList ? "player hidden" : "player not hidden anymore");
                  found = true;
               }
            }

            if(!found)
               m2cSendChat(mPlayerOrServerName, true, "player not found");
         }
         else if(command == "hideip")
         {
            Address addr(words[1].c_str());
            bool found = false;
            const Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

            for(S32 i = 0; i < clientList->size(); i++)
            {
               MasterServerConnection *client = clientList->get(i);

               if(addr.isEqualAddress(client->getNetAddress()))
               {
                  client->mIsIgnoredFromList = true;
                  m2cSendChat(client->mPlayerOrServerName, true, "player now hidden");
                  c2mLeaveGlobalChat_remote();  // Also mute and delist the player
                  found = true;
               }
            }
            gListAddressHide.push_back(addr);
            if(found)
               m2cSendChat(mPlayerOrServerName, true, "player found, and is in IP hidden list");
            if(!found)
               m2cSendChat(mPlayerOrServerName, true, "player not found, but is in IP hidden list");
         }
         else if(command ==  "unhideips")
         {
            gListAddressHide.clear();
            m2cSendChat(mPlayerOrServerName, true, "cleared IP hidden list");
         }
         else
            adminCommand = false;  // Wasn't an admin command after all

         // We've done our job if a command was run.  Let's leave
         if(adminCommand)
            return;
      }

      // If we made it here then an admin command wasn't sent
      if(command == "pm")
      {
         if(words.size() < 3)  // No message?
         {
            logprintf(LogConsumer::LogError, "Malformed private message: %s", message.getString());
            m2cSendChat(mPlayerOrServerName, true, "Malformed private message");
            return;
         }

         isPrivate = true;

         pmRecipient = words[1];

         S32 argCount = 2 + countCharInString(words[1], ' ');  // Set pointer after 2 args + number of spaces in player name
         strippedMessage = findPointerOfArg(message, argCount);

         // Now relay the message and only send to client with the specified nick
         const Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

         for(S32 i = 0; i < clientList->size(); i++)
         {
            if(stricmp(clientList->get(i)->mPlayerOrServerName.getString(), pmRecipient.c_str()) == 0)
            {
               clientList->get(i)->m2cSendChat(mPlayerOrServerName, isPrivate, strippedMessage);
               break;
            }
         }
      }
      else
         badCommand = true;  // Don't relay bad commands as chat messages
   }


   // If player is being ignored or chatting too fast
   if(mIsIgnoredFromList || !checkMessage(message, isPrivate ? 2 : 0))
   {
      if(!mChatTooFast)
         logprintf(LogConsumer::LogChat, mIsIgnoredFromList ? "Tried to chat but muted and hidden: %s" : "This player may be chatting to fast: %s ", mPlayerOrServerName.getString());

      mChatTooFast = true;
      static const StringTableEntry msg("< You are chatting too fast, your message didn't make it through.");
      m2cSendChat(msg, false, StringPtr(" "));

      return;  // Bail
   }
   mChatTooFast = false;


   // Now relay the chat to all connected clients
   if(!badCommand && !isPrivate)
   {
      const  Vector<MasterServerConnection *> *clientList = mMaster->getClientList();

      for(S32 i = 0; i < clientList->size(); i++)
      {
         if(clientList->get(i) != this)    // ...except self!
            clientList->get(i)->m2cSendChat(mPlayerOrServerName, isPrivate, message);
      }
   }


   // Log F5 chat messages
   if(isPrivate)
      logprintf(LogConsumer::LogChat, "Relayed private msg from %s to %s: %s", mPlayerOrServerName.getString(), pmRecipient.c_str(), strippedMessage);
   else
      logprintf(LogConsumer::LogChat, "Relayed chat msg from %s: %s", mPlayerOrServerName.getString(), message.getString());
}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, s2mChangeName, (StringTableEntry name))
{
   if(mConnectionType == MasterConnectionTypeServer)  // server only, don't want clients to rename yet (client names need to authenticate)
   {
      mPlayerOrServerName = name;
      mMaster->writeJsonNow();  // update server name in ".json"
   }
}
TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, s2mServerDescription, (StringTableEntry descr))
{
   mServerDescr = descr;
}

TNL_IMPLEMENT_NETCONNECTION(MasterServerConnection, NetClassGroupMaster, true);

Vector< GameConnectRequest* > MasterServerConnection::gConnectList;
Vector<SafePtr<MasterServerConnection> > MasterServerConnection::gLeaveChatTimerList;


}
