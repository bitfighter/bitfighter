//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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

#ifdef GAME_JOLT
#  include <signal.h>
#endif

using namespace TNL;
using namespace std;
using namespace Zap;
using namespace DbWriter;


namespace Master
{

// Create a test database and write some records to it.  Return exit code.
S32 testDb(const char *dbName)
{
   DatabaseWriter databaseWriter(dbName);
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

}

using namespace Master;

//#ifdef _MSC_VER
//#  pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
//#endif

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
   MasterSettings settings("master.ini");
   settings.readConfigFile();

   seedRandomNumberGenerator();

   MasterServer masterServer(&settings);

#ifdef GAME_JOLT
   signal(SIGCHLD, SIG_IGN);     // Allow zombie children to die quietly
#endif


   U32 lastTime = Platform::getRealMilliseconds();

   while(true)
   {
      U32 currentTime = Platform::getRealMilliseconds();

      U32 timeDelta = currentTime - lastTime;
      lastTime = currentTime;

      // Sane sanity check
      if(timeDelta > 5000)
         timeDelta = 10;

      masterServer.idle(timeDelta);
      Platform::sleep(5);
   }

   return 0;
}
