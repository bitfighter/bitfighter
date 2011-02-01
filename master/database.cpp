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
#include "database.h"
#include "tnlTypes.h"
#include "tnlLog.h"
#include "../zap/stringUtils.h"            // For replaceString()

#ifdef BF_WRITE_TO_MYSQL
#include "mysql++.h"
#endif

using namespace std;
using namespace mysqlpp;
using namespace TNL;


// Default constructor -- don't use this one!
DatabaseWriter::DatabaseWriter()
{
   mIsValid = false;
}


// Constructor
DatabaseWriter::DatabaseWriter(const char *server, const char *db, const char *user, const char *password)
{
   initialize(server, db, user, password);
}


// Constructor -- defaults to local machine for db server
DatabaseWriter::DatabaseWriter(const char *db, const char *user, const char *password)
{
   initialize("127.0.0.1", db, user, password);
}


void DatabaseWriter::initialize(const char *server, const char *db, const char *user, const char *password)
{
   mServer = server;
   mDb = db;
   mUser = user;
   mPassword = password;
   mIsValid = db[0] != 0;
}


static string sanitize(const string &value)     
{
   return replaceString(replaceString(value, "\\", "\\\\"), "'", "''");
}


#define btos(value) (value ? "1" : "0")


#ifndef BF_WRITE_TO_MYSQL
class Query{
};
class SimpleResult{
public:
   U64 insert_id() {return 0;};
};
#define Exception std::exception
#endif


// Create wrapper function to make logging easier
static SimpleResult runQuery(Query *query, const string &sql)      // TODO: Pass query as a Query *?
{
#ifdef BF_WRITE_TO_MYSQL
   return query->execute(sql);
#else
	throw std::exception();
#endif
}


static void insertStatsShots(Query *query, const string &playerId, const Vector<WeaponStats> weaponStats)
{
   for(S32 i = 0; i < weaponStats.size(); i++)
   {
      if(weaponStats[i].shots > 0)
      {
         string sql = "INSERT INTO stats_player_shots(stats_player_id, weapon, shots, shots_struck) "
                       "VALUES(" + playerId + ", '" + WeaponInfo::getWeaponName(weaponStats[i].weaponType) + "', " + 
                                  itos(weaponStats[i].shots) + ", " + itos(weaponStats[i].hits) + ");";
         
         if(query)
            itos(runQuery(query, sql).insert_id());
         else
            logprintf(LogConsumer::StatisticsFilter, sql.c_str());
      }
   }
}


// Inserts player and all associated weapon stats
static string insertStatsPlayer(Query *query, const PlayerStats *playerStats, const string gameId, const string &teamId)
{
   string sql = "INSERT INTO stats_player(stats_game_id, stats_team_id, player_name, "
                                               "is_authenticated, is_robot, "
                                               "result, points, kill_count, "
                                               "death_count, "
                                               "suicide_count, switched_team) "
                      "VALUES(" + gameId + ", " + teamId + ", '" + sanitize(playerStats->name) + "', " +
                                 btos(playerStats->isAuthenticated) + ", " + btos(playerStats->isRobot) + ", '" +
                                 playerStats->gameResult + "', " + itos(playerStats->points) + ", " + itos(playerStats->kills) + ", " + 
                                 itos(playerStats->deaths) + ", " +
                                 itos(playerStats->suicides) + ", " + btos(playerStats->switchedTeams) + ");";

   string playerId;

   if(query)
      playerId = itos(runQuery(query, sql).insert_id());
   else
   {
      playerId = "(select max(stats_player_id) from stats_player)";
      logprintf(LogConsumer::StatisticsFilter, sql.c_str());
   }

   insertStatsShots(query, playerId, playerStats->weaponStats);

   return playerId;
}


// Inserts stats of team and all players
static string insertStatsTeam(Query *query, const TeamStats *teamStats, const string &gameId)
{
   string sql = "INSERT INTO stats_team(stats_game_id, team_name, team_score, result, color_hex) "
            "VALUES(" + gameId + ", '" + sanitize(teamStats->name) + "', " + itos(teamStats->score) + " ,'" + 
                     teamStats->gameResult + "' ,'" + teamStats->color + "');";

   string teamId;

   if(query)
      teamId = itos(runQuery(query, sql).insert_id());
   else
   {
      teamId = "(select max(stats_team_id) from stats_team)";
      logprintf(LogConsumer::StatisticsFilter, sql.c_str());
   }

   for(S32 i = 0; i < teamStats->playerStats.size(); i++)
      insertStatsPlayer(query, &teamStats->playerStats[i], gameId, teamId);

   return teamId;
}


static string insertStatsGame(Query *query, const GameStats *gameStats, const string &serverId)
{
   string sql = "INSERT INTO stats_game(server_id, game_type, is_official, player_count, "
                                       "duration_seconds, level_name, is_team_game, team_count) "
         "VALUES( " + serverId + ", '" + gameStats->gameType + "', " + btos(gameStats->isOfficial) + ", " + itos(gameStats->playerCount) + ", " +
                     itos(gameStats->duration) + ", '" + sanitize(gameStats->levelName) + "', " + btos(gameStats->isTeamGame) + ", " + 
                     itos(gameStats->teamCount)  + ");";

   string gameId;

   if(query)
      gameId = itos(runQuery(query, sql).insert_id());
   else
   {
      gameId = "(select max(stats_game_id) from stats_game)";
      logprintf(LogConsumer::StatisticsFilter, sql.c_str());
   }

   for(S32 i = 0; i < gameStats->teamStats.size(); i++)
      insertStatsTeam(query, &gameStats->teamStats[i], gameId);

   return gameId;
}


static string insertStatsServer(Query *query, const GameStats &gameStats)
{
   string sql = "INSERT INTO server(server_name, ip_address) "
                "VALUES('" + sanitize(gameStats.serverName) + "', '" + gameStats.serverIP + "');";

   string serverId;

   if(query)
      serverId = itos(runQuery(query, sql).insert_id());
   else
   {
      serverId = "(select max(server_id) from server)";
      logprintf(LogConsumer::StatisticsFilter, sql.c_str());
   }

   return serverId;
}


// Return false if failed to write to database
bool DatabaseWriter::insertStats(const GameStats &gameStats, bool writeToDatabase) 
{
   Query *query = NULL;
   bool success = true;

   try
   {
      string serverId;

      if(writeToDatabase)
         serverId = insertStatsServerWithCache();     // calls insertStatsServer
      else
         serverId = insertStatsServer(NULL, gameStats);
        
      if(serverId == "")      // Will only happen if writeToDatabase && ! BF_WRITE_TO_MYSQL -- an illogical combination
         return false;

      insertStatsGame(query, &gameStats, serverId);

      if(query)      
         delete query;
   }
   catch (const Exception &ex) 
   {
      logprintf("Failure writing stats to database: %s", ex.what());
      success = false;
   }

   if(query)
      delete query;

   return success;

}


#ifdef BF_WRITE_TO_MYSQL

string DatabaseWriter::insertStatsServerWithCache()
{
   if(!mIsValid)
   {
      //logprintf("Invalid DatabaseWriter!");
      return false;
   }
   Connection conn;                                 // Connect to the database
   SimpleResult result;
      
   conn.connect(mDb, mServer, mUser, mPassword);    // Will throw error if it fails
      
   query = new Query(&conn);

   U64 serverId_int = U64_MAX;

   // Check cache first
   for(S32 i = cachedServers.size() - 1; i >= 0; i--)
      if(cachedServers[i].ip == gameStats.serverIP && cachedServers[i].name == gameStats.serverName )
      {
         serverId_int = cachedServers[i].id;
         break;
      }

   if(serverId_int == U64_MAX)  // Not in cache
   {
      // Find server in database
      string sql = "SELECT server_id FROM server AS server "
                     "WHERE server_name = '" + sanitize(gameStats.serverName) + "' AND ip_address = '" + gameStats.serverIP + "'";
      StoreQueryResult results = query->store(sql.c_str(), sql.length());

      if(results.num_rows() >= 1)
         serverId_int = results[0][0];

      if(serverId_int == U64_MAX)      // Not in database
         serverId = insertStatsServer(query, gameStats);
      else
         serverId = itos(serverId_int);

      // Limit cache growth
      static const S32 SERVER_CACHE_SIZE = 20;
      if(cachedServers.size() > SERVER_CACHE_SIZE) 
         cachedServers.erase(0);

      cachedServers.push_back(ServerInformation(serverId_int, gameStats.serverName, gameStats.serverIP));

      return serverId;
	}
}

#else

string DatabaseWriter::insertStatsServerWithCache()
{
   return "";
}

#endif