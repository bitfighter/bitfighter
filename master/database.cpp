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

#include "../sqlite/sqlite3.h"

#ifdef BF_WRITE_TO_MYSQL
#include "mysql++.h"
#endif

using namespace std;
using namespace mysqlpp;
using namespace TNL;



// Default constructor -- don't use this one!
DatabaseWriter::DatabaseWriter()
{
   mMySql = false;
   mSqlite = false;
}


// MySQL Constructor
DatabaseWriter::DatabaseWriter(const char *server, const char *db, const char *user, const char *password)
{
   initialize(server, db, user, password);
}


// MySQL Constructor -- defaults to local machine for db server
DatabaseWriter::DatabaseWriter(const char *db, const char *user, const char *password)
{
   initialize("127.0.0.1", db, user, password);
}


// Sqlite Constructor
DatabaseWriter::DatabaseWriter(const char *db)
{
   strncpy(mDb, db, sizeof(mDb) - 1);

   if(fileExists(mDb))
      mSqlite = true;
   else
   {
      logprintf("Could not find database file %s", mDb);
      mSqlite = false;
   }
}


void DatabaseWriter::initialize(const char *server, const char *db, const char *user, const char *password)
{
   strncpy(mServer, server, sizeof(mServer) - 1);   // was const char *, but problems when data in pointer dies.
   strncpy(mDb, db, sizeof(mDb) - 1);
   strncpy(mUser, user, sizeof(mUser) - 1);
   strncpy(mPassword, password, sizeof(mPassword) - 1);
   mMySql = (db[0] != 0);                          // Not valid if db is empty string
}


static string sanitize(const string &value)     
{
   return replaceString(replaceString(value, "\\", "\\\\"), "'", "''");
}


#define btos(value) (value ? "1" : "0")


#ifndef BF_WRITE_TO_MYSQL     // Stats not going to mySQL
class Query {
   // Empty class
};

class SimpleResult{
public:
   U64 insert_id() { return 0; }
};

#define Exception std::exception
#endif


// Create wrapper functions to make logging easier -- every stats related query comes through these

// Run a mysql query -- should never get here unless mysql has been compiled in
static SimpleResult runQuery(Query *query, const string &sql)
{
#ifdef BF_WRITE_TO_MYSQL
   return query->execute(sql);
#else
	throw std::exception();    // Should be impossible
#endif
}


// Run a sqlite query
static void runQuery(sqlite3 *sqliteDb, const string &sql)
{
   char *err = 0;
   sqlite3_exec(sqliteDb, sql.c_str(), NULL, 0, &err);

   if(err)
      logprintf("Database error accessing sqlite databse: %s", err);
}


static string runQuery(Query *query, sqlite3 *sqliteDb, const string &sql)
{
   if(query)
      return itos(runQuery(query, sql).insert_id());     // MySQL query

   else if(sqliteDb)
   {
      runQuery(sqliteDb, sql);                           // Sqlite query
      return itos(sqlite3_last_insert_rowid(sqliteDb));  
   }

   else 
      return "";
}


static void insertStatsShots(Query *query, sqlite3 *sqliteDb, const string &playerId, const Vector<WeaponStats> weaponStats)
{
   for(S32 i = 0; i < weaponStats.size(); i++)
   {
      if(weaponStats[i].shots > 0)
      {
         string sql = "INSERT INTO stats_player_shots(stats_player_id, weapon, shots, shots_struck) "
                       "VALUES(" + playerId + ", '" + WeaponInfo::getWeaponName(weaponStats[i].weaponType) + "', " + 
                                  itos(weaponStats[i].shots) + ", " + itos(weaponStats[i].hits) + ");";
         
         runQuery(query, sqliteDb, sql);
      }
   }
}


// Inserts player and all associated weapon stats
static string insertStatsPlayer(Query *query, sqlite3 *sqliteDb, const PlayerStats *playerStats, const string gameId, const string &teamId)
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

   string playerId = runQuery(query, sqliteDb, sql);

   insertStatsShots(query, sqliteDb, playerId, playerStats->weaponStats);

   return playerId;
}


// Inserts stats of team and all players
static string insertStatsTeam(Query *query, sqlite3 *sqliteDb, const TeamStats *teamStats, const string &gameId)
{
   string sql = "INSERT INTO stats_team(stats_game_id, team_name, team_score, result, color_hex) "
                "VALUES(" + gameId + ", '" + sanitize(teamStats->name) + "', " + itos(teamStats->score) + " ,'" + 
                            teamStats->gameResult + "' ,'" + teamStats->hexColor + "');";

   string teamId = runQuery(query, sqliteDb, sql);

   for(S32 i = 0; i < teamStats->playerStats.size(); i++)
      insertStatsPlayer(query, sqliteDb, &teamStats->playerStats[i], gameId, teamId);

   return teamId;
}


static string insertStatsGame(Query *query, sqlite3 *sqliteDb, const GameStats *gameStats, U64 serverId)
{
   string sql = "INSERT INTO stats_game(server_id, game_type, is_official, player_count, "
                                       "duration_seconds, level_name, is_team_game, team_count) "
                "VALUES( " + itos(serverId) + ", '" + gameStats->gameType + "', " + btos(gameStats->isOfficial) + ", " + itos(gameStats->playerCount) + ", " +
                             itos(gameStats->duration) + ", '" + sanitize(gameStats->levelName) + "', " + btos(gameStats->isTeamGame) + ", " + 
                             itos(gameStats->teamCount)  + ");";

   string gameId = runQuery(query, sqliteDb, sql);

   for(S32 i = 0; i < gameStats->teamStats.size(); i++)
      insertStatsTeam(query, sqliteDb, &gameStats->teamStats[i], gameId);

   return gameId;
}


static U64 insertStatsServer(Query *query, sqlite3 *sqliteDb, const GameStats &gameStats)
{
   string sql = "INSERT INTO server(server_name, ip_address) "
                "VALUES('" + sanitize(gameStats.serverName) + "', '" + gameStats.serverIP + "');";

   if(query)
     return runQuery(query, sql).insert_id();

   else if(sqliteDb)
   {
      runQuery(sqliteDb, sql);
      return sqlite3_last_insert_rowid(sqliteDb);
   }

   return U64_MAX;
}


// Looks in database to find server mathcing the one in gameStats... returns server_id, or U64_MAX if no match was found
static U64 getServerFromDatabase(Query *query, sqlite3 *sqliteDb, const GameStats &gameStats)
{
   // Find server in database
   string sql = "SELECT server_id FROM server AS server "
                "WHERE server_name = '" + sanitize(gameStats.serverName) + "' AND ip_address = '" + gameStats.serverIP + "';";
                //"WHERE server_name = 'Bitfighter sam686' AND ip_address = '96.2.123.136';";

#ifdef BF_WRITE_TO_MYSQL
   if(query)
   {
      U64 serverId_int = U64_MAX;
      StoreQueryResult results = query->store(sql.c_str(), sql.length());

      if(results.num_rows() >= 1)
         serverId_int = results[0][0];

      return serverId_int;
   }
   else
#endif
   if(sqliteDb)
   {
      char *err = 0;
      char **results;
      int rows, cols;

      sqlite3_get_table(sqliteDb, sql.c_str(), &results, &rows, &cols, &err);

      if(rows >= 1)
      {
         string result = results[1];      // results[0] will contain the col header, or "server_id"
         return atoi(result.c_str());
      }

      sqlite3_free_table(results);
   }

   return U64_MAX;
}


void DatabaseWriter::addToServerCache(U64 id, const GameStats &gameStats)
{
   // Limit cache growth
   static const S32 SERVER_CACHE_SIZE = 20;

   if(cachedServers.size() >= SERVER_CACHE_SIZE) 
      cachedServers.erase(0);

   cachedServers.push_back(ServerInfo(id, gameStats.serverName, gameStats.serverIP));
}


U64 DatabaseWriter::getServerFromCache(const GameStats &gameStats)
{
   // Check cache for server info
   for(S32 i = cachedServers.size() - 1; i >= 0; i--)    // Counting backwards visits newest servers first
      if(cachedServers[i].ip == gameStats.serverIP && cachedServers[i].name == gameStats.serverName )
         return  cachedServers[i].id;

   return U64_MAX;
}


// Return false if failed, true if written to database
void DatabaseWriter::insertStats(const GameStats &gameStats) 
{
   Query *query = NULL;
   sqlite3 *sqliteDb = NULL;

   try
   {

#ifdef BF_WRITE_TO_MYSQL
      if(mMySql)
      {
         Connection conn;                                 // Connect to the database
         conn.connect(mDb, mServer, mUser, mPassword);    // Will throw error if it fails
         getServerFromCache(gameStats);
         query = new Query(&conn);
      }
      else
#endif
      if(mSqlite)
      {
         if(sqlite3_open(mDb, &sqliteDb))    // Returns true if an error occurred
         {
            logprintf("ERROR: Can't open stats database %s: %s", mDb, sqlite3_errmsg(sqliteDb));
            sqlite3_close(sqliteDb);
            return;
         }
      }

      else
      {
         //logprintf("Invalid DatabaseWriter!");
         return;
      }


      U64 serverId = getServerFromCache(gameStats);

      if(serverId == U64_MAX)      // Not found in cache, check database
      {
         serverId = getServerFromDatabase(query, sqliteDb, gameStats);

         if(serverId == U64_MAX)   // Not found in database, add to database
            serverId = insertStatsServer(query, sqliteDb, gameStats);

         addToServerCache(serverId, gameStats);
	   }

      insertStatsGame(query, sqliteDb, &gameStats, serverId);
   }
   catch (const Exception &ex) 
   {
      logprintf("Failure writing stats to database: %s", ex.what());
   }

   // Cleanup!
   if(query)
      delete query;

   if(sqliteDb)
      sqlite3_close(sqliteDb);
}

