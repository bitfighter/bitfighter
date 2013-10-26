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

#include "../zap/SharedConstants.h"

#include "master.h"
#include "masterInterface.h"
#include "authenticator.h"       // For authenticating users against the PHPBB3 database
#include "database.h"            // For writing to the database

#include "../zap/stringUtils.h"  // For itos, replaceString
#include "../zap/version.h"      // for MASTER_PROTOCOL_VERSION - in case we ever forget to update master...
#include "../zap/IniFile.h"      // For INI reading/writing

#include "tnlVector.h"
#include "tnlAsymmetricKey.h"
#include "tnlAssert.h"

#include <stdio.h>
#include <string>
#include <stdarg.h>     
#include <time.h>
#include <map>


using namespace TNL;
using namespace std;
using namespace Zap;

namespace Master 
{

CIniFile gMasterINI("dummy");  //  --> move to settings struct


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


////////////////////////////////////////
////////////////////////////////////////

// Create a test database and write some records to it.  Return exit code.
S32 testDb(const char *dbName)
{
   DbWriter::DatabaseWriter databaseWriter(dbName);
   databaseWriter.setDumpSql(true);

   databaseWriter.insertAchievement(1, "ChumpChange", "Achievement Server", "99.99.99.99:9999");
   databaseWriter.insertLevelInfo("9aa6e5f2256c17d2d430b100032b997c", "Clown Car", "Jenkins!", "Core", false, 2, 20, 600);

   GameStats gameStats;
   gameStats.build_version = 100;
   gameStats.cs_protocol_version = 101;
   gameStats.duration = 999;
   gameStats.gameType = "Frogger";
   gameStats.isOfficial = false;
   gameStats.isTeamGame = true;
   gameStats.levelName = "LouLou";
   gameStats.playerCount = 2;
   gameStats.serverIP = "999.999.999.999";
   gameStats.serverName = "Crazy IP Server";

   TeamStats teamStats;
   teamStats.gameResult = 'W';
   teamStats.hexColor = "#FF0000";
   teamStats.name = "Red Dudes";
   teamStats.score = 2;

   PlayerStats playerStats;
   playerStats.changedLoadout = 1;
   playerStats.crashedIntoAsteroid = 2;      
   playerStats.deaths = 3;
   playerStats.flagDrop = 4;  
   playerStats.flagPickup = 5;
   playerStats.flagReturn = 6;
   playerStats.flagScore = 7;
   playerStats.fratricides = 8;
   playerStats.gameResult = 'W';
   playerStats.isAdmin = false;
   playerStats.isAuthenticated = true;
   playerStats.isHosting = true;
   playerStats.isLevelChanger = true;
   playerStats.isRobot = false;
   playerStats.kills = 9;
   playerStats.name = "Player 1";
   playerStats.playTime = 99;
   playerStats.points = 10;
   playerStats.suicides = 11;
   playerStats.switchedTeamCount = 12;  
   playerStats.teleport = 13; 
   teamStats.playerStats.push_back(playerStats);

   playerStats.changedLoadout = 101;
   playerStats.crashedIntoAsteroid = 102;
   playerStats.deaths = 103;
   playerStats.flagDrop = 104;
   playerStats.flagPickup = 105;
   playerStats.flagReturn = 106;
   playerStats.flagScore = 107;
   playerStats.fratricides = 108;
   playerStats.gameResult = 'W';
   playerStats.isAdmin = false;
   playerStats.isAuthenticated = true;
   playerStats.isHosting = true;
   playerStats.isLevelChanger = true;
   playerStats.isRobot = false;
   playerStats.kills = 109;
   playerStats.name = "Player 1";
   playerStats.playTime = 1099;
   playerStats.points = 1010;
   playerStats.suicides = 1011;
   playerStats.switchedTeamCount = 1012;
   playerStats.teleport = 1013;
   teamStats.playerStats.push_back(playerStats);

   gameStats.teamStats.push_back(teamStats);

   databaseWriter.insertStats(gameStats);

   printf("Created database %s", dbName);

   return 0;
}


////////////////////////////////////////
////////////////////////////////////////

extern void readConfigFile(CIniFile *ini, MasterSettings *settings);

// Constructor
MasterSettings::MasterSettings()     
{
   // Note that on the master, our settings are read-only, so there is no need to specify a comment
   //                      Data type  Setting name                       Default value         INI Key                              INI Section                                  
   mSettings.add(new Setting<string>("ServerName",                 "Bitfighter Master Server", "name",                                 "host"));
   mSettings.add(new Setting<string>("JsonOutfile",                      "server.json",        "json_file",                            "host"));
   mSettings.add(new Setting<U32>   ("Port",                                 25955,            "port",                                 "host"));
   mSettings.add(new Setting<U32>   ("LatestReleasedCSProtocol",               0,              "latest_released_cs_protocol",          "host"));
   mSettings.add(new Setting<U32>   ("LatestReleasedBuildVersion",             0,              "latest_released_client_build_version", "host"));
                                                                                               
   // Variables for managing access to MySQL                                                   
   mSettings.add(new Setting<string>("MySqlAddress",                           "",             "phpbb_database_address",               "phpbb"));
   mSettings.add(new Setting<string>("DbUsername",                             "",             "phpbb3_database_username",             "phpbb"));
   mSettings.add(new Setting<string>("DbPassword",                             "",             "phpbb3_database_password",             "phpbb"));
                                                                                               
   // Variables for verifying usernames/passwords in PHPBB3                                    
   mSettings.add(new Setting<string>("Phpbb3Database",                         "",             "phpbb3_database_name",                 "phpbb"));
   mSettings.add(new Setting<string>("Phpbb3TablePrefix",                      "",             "phpbb3_table_prefix",                  "phpbb"));
                                                                                               
   // Stats database credentials                                                               
   mSettings.add(new Setting<YesNo> ("WriteStatsToMySql",                      No,             "write_stats_to_mysql",                 "stats"));
   mSettings.add(new Setting<string>("StatsDatabaseAddress",                   "",             "stats_database_addr",                  "stats"));
   mSettings.add(new Setting<string>("StatsDatabaseName",                      "",             "stats_database_name",                  "stats"));
   mSettings.add(new Setting<string>("StatsDatabaseUsername",                  "",             "stats_database_username",              "stats"));
   mSettings.add(new Setting<string>("StatsDatabasePassword",                  "",             "stats_database_password",              "stats"));
}                                                                                              


// Constructor
MasterServer::MasterServer(MasterSettings *settings)
{
   mSettings = settings;

   mStartTime = Platform::getRealMilliseconds();

   // Initialize our net interface so we can accept connections...  mNetInterface is deleted in destructor
   mNetInterface = createNetInterface();

   mCleanupTimer.reset(TEN_MINUTES);
   mReadConfigTimer.reset(FIVE_SECONDS);     // Reread the config file every 5 seconds... excessive?
   mJsonWriteTimer.reset(0, FIVE_SECONDS);   // Max frequency for writing JSON files -- set current to 0 so we'll write immediately
   mJsonWritingSuspended = false;
   
   mDatabaseAccessThread.start();            // Start a thread to handle database interaction

   MasterServerConnection::setMasterServer(this);
}


// Destructor
MasterServer::~MasterServer()
{
   delete mNetInterface;
}


NetInterface *MasterServer::createNetInterface() const
{
   U32 port = mSettings->getVal<U32>("Port");
   NetInterface *netInterface = new NetInterface(Address(IPProtocol, Address::Any, port));

   // Log a welcome message in the main log and to the console
   logprintf("[%s] Master Server %s started - listening on port %d", getTimeStamp().c_str(),
                                                                     getSetting<string>("ServerName").c_str(),
                                                                     port);
   return netInterface;
}


U32 MasterServer::getStartTime() const
{
   return mStartTime;
}


const MasterSettings *MasterServer::getSettings() const
{
   return mSettings;
}


// Will trigger a JSON rewrite after timer has run its full cycle
void MasterServer::writeJsonDelayed()
{
   mJsonWriteTimer.reset();
   mJsonWritingSuspended = false;
}


// Indicates we want to write JSON as soon as possible... but never more 
// frequently than allowed by mJsonWriteTimer, which we don't reset here
void MasterServer::writeJsonNow()
{
   mJsonWritingSuspended = false;
}


const Vector<MasterServerConnection *> *MasterServer::getServerList() const
{
   return &mServerList;
}


const Vector<MasterServerConnection *> *MasterServer::getClientList() const
{
   return &mClientList;
}


void MasterServer::addServer(MasterServerConnection *server)
{
   mServerList.push_back(server);
}


void MasterServer::addClient(MasterServerConnection *client)
{
   mClientList.push_back(client);
}


void MasterServer::removeServer(S32 index)
{
   TNLAssert(index >= 0 && index < mServerList.size(), "Index out of range!");
   mServerList.erase_fast(index);
}


void MasterServer::removeClient(S32 index)
{
   TNLAssert(index >= 0 && index < mClientList.size(), "Index out of range!");
   mClientList.erase_fast(index);
}


NetInterface *MasterServer::getNetInterface() const
{
   return mNetInterface;
}


void MasterServer::idle(U32 timeDelta)
{
   mNetInterface->checkIncomingPackets();
   mNetInterface->processConnections();

   // Reread config file
   if(mReadConfigTimer.update(timeDelta))
   {
      readConfigFile(&gMasterINI, mSettings);
      mReadConfigTimer.reset();
   }

   // Cleanup, cleanup, everybody cleanup!
   if(mCleanupTimer.update(timeDelta))
   {
      MasterServerConnection::removeOldEntriesFromRatingsCache();    //<== need non-static access
      mCleanupTimer.reset();
   }


   // Handle writing our JSON file
   mJsonWriteTimer.update(timeDelta);

   if(!mJsonWritingSuspended && mJsonWriteTimer.getCurrent() == 0)
   {
      MasterServerConnection::writeClientServerList_JSON();

      mJsonWritingSuspended = true;    // No more writes until this is cleared
      mJsonWriteTimer.reset();         // But reset the timer so it start ticking down even if we aren't writing
   }


   // Process connections -- cycle through them and check if any have timed out
   U32 currentTime = Platform::getRealMilliseconds();

   for(S32 i = MasterServerConnection::gConnectList.size() - 1; i >= 0; i--)     //< Get rid of global here
   {
      GameConnectRequest *request = MasterServerConnection::gConnectList[i];     //< Get rid of global here

      if(currentTime - request->requestTime > FIVE_SECONDS)  
      {
         if(request->initiator.isValid())
         {
            ByteBufferPtr ptr = new ByteBuffer((U8 *)MasterRequestTimedOut, strlen(MasterRequestTimedOut) + 1);

            request->initiator->m2cArrangedConnectionRejected(request->initiatorQueryId, ptr);   // 0 = ReasonTimedOut
            request->initiator->removeConnectRequest(request);
         }

         if(request->host.isValid())
            request->host->removeConnectRequest(request);

         MasterServerConnection::gConnectList.erase_fast(i);
         delete request;
      }
   }

   // Process any delayed disconnects; we use this to avoid repeating and flooding join / leave messages
   for(S32 i = MasterServerConnection::gLeaveChatTimerList.size() - 1; i >= 0; i--)
   {
      MasterServerConnection *c = MasterServerConnection::gLeaveChatTimerList[i];      //< Get rid of global here

      if(!c || c->mLeaveGlobalChatTimer == 0)
         MasterServerConnection::gLeaveChatTimerList.erase(i);                         //< Get rid of global here
      else
      {
         if(currentTime - c->mLeaveGlobalChatTimer > ONE_SECOND)
         {
            c->isInGlobalChat = false;

            const Vector<MasterServerConnection *> *serverList = getServerList();

            for(S32 j = 0; j < serverList->size(); j++)
               if(serverList->get(j) != c && serverList->get(j)->isInGlobalChat)
                  serverList->get(j)->m2cPlayerLeftGlobalChat(c->mPlayerOrServerName);

            MasterServerConnection::gLeaveChatTimerList.erase(i);
         }
      }
   }

   mDatabaseAccessThread.idle();
}


DatabaseAccessThread *MasterServer::getDatabaseAccessThread()
{
   return &mDatabaseAccessThread;
}

}  // namespace


using namespace Master;

int main(int argc, const char **argv)
{
   // Handle cmd line params
   if(argc == 2 && strcmp(argv[1], "-testdb") == 0)
      exit(testDb("test_db"));

   // Configure logging
   S32 events = LogConsumer::AllErrorTypes | LogConsumer::LogConnection | LogConsumer::LogConnectionManager | LogConsumer::LogChat;

   FileLogConsumer fileLogConsumer;             // Primary logfile
   fileLogConsumer.init("bitfighter_master.log", "a");
   fileLogConsumer.setMsgTypes(events);                               
   fileLogConsumer.logprintf("------ Bitfighter Master Server Log File ------");
      
   StdoutLogConsumer stdoutLogConsumer;         // stdout, so we can monitor stuff from the cmd line
   stdoutLogConsumer.setMsgTypes(events);                             

   FileLogConsumer statisticsLogConsumer;       // Statistics file
   statisticsLogConsumer.init("bitfighter_player_stats.log", "a");
   statisticsLogConsumer.setMsgTypes(LogConsumer::StatisticsFilter);  

   // Set INI location
   MasterSettings settings;
   gMasterINI.SetPath("master.ini");
   readConfigFile(&gMasterINI, &settings);

   seedRandomNumberGenerator();

   MasterServer masterServer(&settings);


   U32 lastTime = Platform::getRealMilliseconds();

   while(true)
   {
      U32 currentTime = Platform::getRealMilliseconds();

      U32 timeDelta = currentTime - lastTime;
      lastTime = currentTime;

      // Sanity check
      if(timeDelta < -500 || timeDelta > 5000)
         timeDelta = 10;

      masterServer.idle(timeDelta);
      Platform::sleep(5);
   }

   return 0;
}
