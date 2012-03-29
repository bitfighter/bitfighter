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
#include <fstream>
#include "database.h"
#include "tnlTypes.h"
#include "tnlLog.h"
#include "../zap/stringUtils.h"            // For replaceString() and itos()
#include "../zap/WeaponInfo.h"

#ifdef BF_WRITE_TO_MYSQL
#include "mysql++.h"
using namespace mysqlpp;
#endif

using namespace std;
using namespace TNL;



// Default constructor -- don't use this one!
DatabaseWriter::DatabaseWriter()
{
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

   if(!fileExists(mDb))
      createStatsDatabase();
}


void DatabaseWriter::initialize(const char *server, const char *db, const char *user, const char *password)
{
   strncpy(mServer, server, sizeof(mServer) - 1);   // was const char *, but problems when data in pointer dies.
   strncpy(mDb, db, sizeof(mDb) - 1);
   strncpy(mUser, user, sizeof(mUser) - 1);
   strncpy(mPassword, password, sizeof(mPassword) - 1);
}


static string sanitize(const string &value)     
{
   return replaceString(replaceString(value, "\\", "\\\\"), "'", "''");
}


#define btos(value) (value ? "1" : "0")


#ifndef BF_WRITE_TO_MYSQL     // Stats not going to mySQL
class SimpleResult{
public:
   S32 insert_id() { return 0; }
};

#define Exception std::exception
#endif


static void insertStatsLoadout(const DbQuery &query, U64 playerId, const Vector<LoadoutStats> loadoutStats)
{
   for(S32 i = 0; i < loadoutStats.size(); i++)
   {
         string sql = "INSERT INTO stats_player_loadout(stats_player_id, loadout) "
                       "VALUES(" + itos(playerId) + ", " + itos(loadoutStats[i].loadoutHash) + ");";

         query.runQuery(sql);
   }
}


static void insertStatsShots(const DbQuery &query, U64 playerId, const Vector<WeaponStats> weaponStats)
{
   for(S32 i = 0; i < weaponStats.size(); i++)
   {
      if(weaponStats[i].shots > 0)
      {
         string sql = "INSERT INTO stats_player_shots(stats_player_id, weapon, shots, shots_struck) "
                       "VALUES(" + itos(playerId) + ", '" + WeaponInfo::getWeaponName(weaponStats[i].weaponType) + "', " + 
                                  itos(weaponStats[i].shots) + ", " + itos(weaponStats[i].hits) + ");";
         
         query.runQuery(sql);
      }
   }
}


// Inserts player and all associated weapon stats
static U64 insertStatsPlayer(const DbQuery &query, const PlayerStats *playerStats, U64 gameId, const string &teamId)
{
   string sql = "INSERT INTO stats_player(stats_game_id, stats_team_id, player_name, "
                                               "is_authenticated, is_robot, "
                                               "result, points, kill_count, "
                                               "death_count, "
                                               "suicide_count, switched_team_count, "
                                               "asteroid_crashes, flag_drops, "
                                               "flag_pickups, flag_returns, flag_scores, "
                                               "teleport_uses) "
                      "VALUES(" + itos(gameId) + ", " + teamId + ", '" + sanitize(playerStats->name) + "', " +
                                 btos(playerStats->isAuthenticated) + ", " + btos(playerStats->isRobot) + ", '" +
                                 playerStats->gameResult + "', " + itos(playerStats->points) + ", " + itos(playerStats->kills) + ", " + 
                                 itos(playerStats->deaths) + ", " +
                                 itos(playerStats->suicides) + ", " + itos(playerStats->switchedTeamCount) + ", " +
                                 itos(playerStats->crashedIntoAsteroid) + ", " + itos(playerStats->flagDrop) + ", " + 
                                 itos(playerStats->flagPickup) + ", " + itos(playerStats->flagReturn) + ", " + itos(playerStats->flagScore) + ", " + 
                                 itos(playerStats->teleport) + ");";

   U64 playerId = query.runQuery(sql);

   insertStatsShots(query, playerId, playerStats->weaponStats);
   insertStatsLoadout(query, playerId, playerStats->loadoutStats);

   return playerId;
}


// Inserts stats of team and all players
static U64 insertStatsTeam(const DbQuery &query, const TeamStats *teamStats, U64 &gameId)
{
   string sql = "INSERT INTO stats_team(stats_game_id, team_name, team_score, result, color_hex) "
                "VALUES(" + itos(gameId) + ", '" + sanitize(teamStats->name) + "', " + itos(teamStats->score) + " ,'" + 
                ctos(teamStats->gameResult) + "' ,'" + teamStats->hexColor + "');";

   U64 teamId = query.runQuery(sql);

   for(S32 i = 0; i < teamStats->playerStats.size(); i++)
      insertStatsPlayer(query, &teamStats->playerStats[i], gameId, itos(teamId));

   return teamId;
}


static U64 insertStatsGame(const DbQuery &query, const GameStats *gameStats, U64 serverId)
{
   string sql = "INSERT INTO stats_game(server_id, game_type, is_official, player_count, "
                                       "duration_seconds, level_name, is_team_game, team_count) "
                "VALUES( " + itos(serverId) + ", '" + gameStats->gameType + "', " + btos(gameStats->isOfficial) + ", " + 
                             itos(gameStats->playerCount) + ", " + itos(gameStats->duration) + ", '" + 
                             sanitize(gameStats->levelName) + "', " + btos(gameStats->isTeamGame) + ", " + 
                             itos(gameStats->teamStats.size())  + ");";

   U64 gameId = query.runQuery(sql);

   for(S32 i = 0; i < gameStats->teamStats.size(); i++)
      insertStatsTeam(query, &gameStats->teamStats[i], gameId);

   return gameId;
}


static U64 insertStatsServer(const DbQuery &query, const string &serverName, const string &serverIP)
{
   string sql = "INSERT INTO server(server_name, ip_address) "
                "VALUES('" + sanitize(serverName) + "', '" + serverIP + "');";

   if(query.query)
     return query.runQuery(sql);

   if(query.sqliteDb)
   {
      query.runQuery(sql);
      return sqlite3_last_insert_rowid(query.sqliteDb);
   }

   return U64_MAX;
}


// Looks in database to find server mathcing the one in gameStats... returns server_id, or -1 if no match was found
S32 DatabaseWriter::getServerIdFromDatabase(const DbQuery &query, const string &serverName, const string &serverIP)
{
   // Find server in database
   string sql = "SELECT server_id FROM server AS server "
                "WHERE server_name = '" + sanitize(serverName) + "' AND ip_address = '" + serverIP + "' LIMIT 1;";
                //"WHERE server_name = 'Bitfighter sam686' AND ip_address = '96.2.123.136';";

   Vector<Vector<string> > results;
   selectHandler(sql, 1, results);

   if(results.size() == 1 && results[0].size() == 1)
      return atoi(results[0][0].c_str());

   return -1;
}


void DatabaseWriter::addToServerCache(U64 id, const string &serverName, const string &serverIP)
{
   // Limit cache growth
   static const S32 SERVER_CACHE_SIZE = 20;

   if(cachedServers.size() >= SERVER_CACHE_SIZE) 
      cachedServers.erase(0);

   cachedServers.push_back(ServerInfo(id, serverName, serverIP));
}


// Get the serverID given its name and IP.  First we'll check our cache to see if this is a known server; if we can't find
// it there, we'll go to the database to retrieve it.
U64 DatabaseWriter::getServerID(const DbQuery &query, const string &serverName, const string &serverIP)
{
   U64 serverId = getServerIDFromCache(serverName, serverIP);

   if(serverId == U64_MAX)      // Not found in cache, check database
   {
      serverId = getServerIdFromDatabase(query, serverName, serverIP);

      if(serverId == U64_MAX)   // Not found in database, add to database
         serverId = insertStatsServer(query, serverName, serverIP);

      // Save server info to cache for future use
      addToServerCache(serverId, serverName, serverIP);     
   }

   return serverId;
}


// We can save a little wear-and-tear on the database by caching recent server IDs rather than retrieving them from
// the database each time we need to find one.  Server IDs should be unique for a given pair of server name and IP address.
U64 DatabaseWriter::getServerIDFromCache(const string &serverName, const string &serverIP)
{
   for(S32 i = cachedServers.size() - 1; i >= 0; i--)    // Counting backwards to visit newest servers first
      if(cachedServers[i].ip == serverIP && cachedServers[i].name == serverName)
         return cachedServers[i].id;

   return U64_MAX;
}


void DatabaseWriter::insertStats(const GameStats &gameStats) 
{
   DbQuery query(mDb, mServer, mUser, mPassword);

   try
   {
      if(query.isValid)
      {
         U64 serverId = getServerID(query, gameStats.serverName, gameStats.serverIP);
         insertStatsGame(query, &gameStats, serverId);
      }
   }
   catch (const Exception &ex) 
   {
      logprintf("Failure writing stats to database: %s", ex.what());
   }
}


void DatabaseWriter::insertAchievement(U8 achievementId, const StringTableEntry &playerNick, const string &serverName, const string &serverIP) 
{
   DbQuery query(mDb, mServer, mUser, mPassword);

   try
   {
      if(query.isValid)
      {
         U64 serverId = getServerID(query, serverName, serverIP);

         string sql = "INSERT INTO player_achievements(player_name, achievement_id, server_id) "
                      "VALUES( '" + sanitize(string(playerNick.getString())) + "', '" + itos(achievementId) + "', " + itos(serverId) + ");";

         query.runQuery(sql);
      }
   }
   catch (const Exception &ex) 
   {
      logprintf("Failure writing achievement to database: %s", ex.what());
   }
}


void DatabaseWriter::insertLevelInfo(const string &hash, const string &levelName, const string &creator, 
                                     const string &gameType, bool hasLevelGen, U8 teamCount, S32 winningScore, S32 gameDurationInSeconds)
{
   DbQuery query(mDb, mServer, mUser, mPassword);

   try
   {
      if(!query.isValid)
         return;

      // Sanity check
      if(hash.length() != 32)
         return;

      string sql;

      // We only want to insert a record of this server if the hash does not yet exist
      sql = "SELECT hash FROM stats_level WHERE hash = '" + sanitize(hash) + "' LIMIT 1;";

      Vector<Vector<string> > results;
      selectHandler(sql, 1, results);

      bool found = (results.size() == 1 && results[0].size() == 1);

      if(!found) 
      {
         sql = "INSERT INTO stats_level(hash, level_name, creator, game_type, has_levelgen, team_count, winning_score, game_duration) "
               "VALUES('" + sanitize(hash)      + "', '" + sanitize(levelName)         + "', " + 
                      "'" + sanitize(creator)   + "', '" + sanitize(gameType)          + "', " +
                      "'" + btos(hasLevelGen)   + "', '" + itos(teamCount)             + "', " + 
                      "'" + itos(winningScore)  + "', '" + itos(gameDurationInSeconds) + "');";
         query.runQuery(sql);
      }
   }
   catch (const Exception &ex) 
   {
      logprintf("Failure writing level info to database: %s", ex.what());
   }
}


void DatabaseWriter::getTopPlayers(const string &table, const string &col2, S32 count, Vector<string> &names, Vector<string> &scores)
{
   // Find server in database
   string sql = "SELECT player_name, " + col2 + " FROM " + table + " " +
                "LIMIT " + itos(count) + ";";

   Vector<Vector<string> > results;

   selectHandler(sql, 2, results);

   for(S32 i = 0; i < results.size(); i++)
   {
      names.push_back(results[i][0]);
      scores.push_back(results[i][1]);
   }

   // Make sure we have the correct number of responses, even if table doesn't have enough records
   for(S32 i = results.size(); i < count; i++)
   {
      names.push_back("");
      scores.push_back("");
   }
}


Int<BADGE_COUNT> DatabaseWriter::getAchievements(const StringTableEntry &name)
{
   string sql = "SELECT achievement_id FROM player_achievements WHERE player_name = '" + string(name.getString()) + "';";

   Vector<Vector<string> > results;

   selectHandler(sql, 1, results);

   S32 badges = 0;

   for(S32 i = 0; i < results.size(); i++)
      badges |= BIT(atoi(results[i][0].c_str()));

   return (Int<BADGE_COUNT>)badges;
}


void DatabaseWriter::selectHandler(const string &sql, S32 cols, Vector<Vector<string> > &values)
{
   DbQuery query(mDb, mServer, mUser, mPassword);

#ifdef BF_WRITE_TO_MYSQL
   if(query.query)
   {
      //S32 serverId_int = -1;
      StoreQueryResult results = query.query->store(sql.c_str(), sql.length());

      S32 rows = results.num_rows();

      for(S32 i = 0; i < rows; i++)
      {
         values.push_back(Vector<string>());     // Add another row

         for(S32 j = 0; j < cols; j++)
            values[i].push_back(string(results[i][j]));
      }
   }
   else
#endif
   if(query.sqliteDb)
   {
      char *err = 0;
      char **results;
      int rows, cols;

      sqlite3_get_table(query.sqliteDb, sql.c_str(), &results, &rows, &cols, &err);

      // results[0]...results[cols] contain the col headers ==> http://www.sqlite.org/c3ref/free_table.html
      for(S32 i = 0; i < rows * cols; i += cols)
      {
         values.push_back(Vector<string>());     // Add another row

         for(S32 j = 0; j < cols; j++)
            values[i].push_back(results[cols + i + j]);
      }

      sqlite3_free_table(results);
   }
}


void DatabaseWriter::createStatsDatabase() 
{
   DbQuery query(mDb, mServer, mUser, mPassword);

   // Create empty file on file system
   logprintf("Creating stats database file %s", mDb);
   ofstream dbFile;
   dbFile.open(mDb);
   dbFile.close();

   // Import schema
   logprintf("Building stats database schema");
   sqlite3 *sqliteDb = NULL;

   sqlite3_open(mDb, &sqliteDb);

   query.runQuery(getSqliteSchema());

   if(sqliteDb)
      sqlite3_close(sqliteDb);
}


void DatabaseWriter::setDumpSql(bool dump)
{
   DbQuery::dumpSql = dump;
}

////////////////////////////////////////
////////////////////////////////////////

bool DbQuery::dumpSql = false;


// Constructor
DbQuery::DbQuery(const char *db, const char *server, const char *user, const char *password)
{
   query = NULL;
   sqliteDb = NULL;
   isValid = true;

   TNLAssert(db && db[0] != 0, "must have a database");

#ifdef BF_WRITE_TO_MYSQL

   if(server && server[0] != 0) // mysql have a server to connect to
   {
      TNLAssert(server, "const char * server is NULL");
      TNLAssert(user, "const char * user is NULL");
      TNLAssert(password, "const char * password is NULL");
      try
      {
         conn.connect(db, server, user, password);    // Will throw error if it fails
         query = new Query(&conn);
      }
      catch (const Exception &ex) 
      {
         logprintf("Failure opening mysql database: %s", ex.what());
         isValid = false;
      }
   }
   else
#endif
      if(sqlite3_open(db, &sqliteDb))    // Returns true if an error occurred
      {
         logprintf("ERROR: Can't open stats database %s: %s", db, sqlite3_errmsg(sqliteDb));
         sqlite3_close(sqliteDb);
         isValid = false;
      }
}

// Destructor
DbQuery::~DbQuery()
{
   if(query)
      delete query;

   if(sqliteDb)
      sqlite3_close(sqliteDb);
}


// Run the passed query on the appropriate database -- throws exceptions!
U64 DbQuery::runQuery(const string &sql) const
{
   if(!isValid)
      return U64_MAX;

   if(dumpSql)
      logprintf("SQL: %s", sql.c_str());

   if(query)
      // Should only get here when mysql has been compiled in
#ifdef BF_WRITE_TO_MYSQL
         return query->execute(sql).insert_id();
#else
         throw std::exception();    // Should be impossible
#endif

   if(sqliteDb)
   {
      char *err = 0;
      sqlite3_exec(sqliteDb, sql.c_str(), NULL, 0, &err);

      if(err)
         logprintf("Database error accessing sqlite databse: %s", err);


      return sqlite3_last_insert_rowid(sqliteDb);  
   }

   return U64_MAX;
}


////////////////////////////////////////
////////////////////////////////////////

// Put this at the end where it's easy to find

string DatabaseWriter::getSqliteSchema() {
   string schema =
      /* bitfighter sqlite3 database structure */
      /* turn on foreign keys */
      "PRAGMA foreign_keys = ON;"

      /* schema */
      "DROP TABLE IF EXISTS schema;"
      "CREATE  TABLE schema (version INTEGER NOT NULL);"
      "INSERT INTO schema VALUES(1);"

      /* server */
      "DROP TABLE IF EXISTS server;"
      "CREATE TABLE server (server_id INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL, server_name TEXT, ip_address TEXT  NOT NULL);"

      "CREATE UNIQUE INDEX server_name_ip_unique ON server(server_name COLLATE BINARY, ip_address COLLATE BINARY);"


      /*  stats_game */
      "DROP TABLE IF EXISTS stats_game;"
      "CREATE TABLE stats_game ("
      "   stats_game_id INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL,"
      "   server_id INTEGER NOT NULL,"
      "   game_type TEXT NOT NULL,"
      "   is_official BOOL NOT NULL,"
      "   player_count INTEGER NOT NULL,"
      "   duration_seconds INTEGER NOT NULL,"
      "   level_name TEXT NOT NULL,"
      "   is_team_game BOOL NOT NULL,"
      "   team_count BOOL NULL,"
      "   insertion_date DATETIME NOT NULL  DEFAULT CURRENT_TIMESTAMP,"
      "   FOREIGN KEY(server_id) REFERENCES server(server_id));"

      "CREATE INDEX stats_game_server_id ON stats_game(server_id);"
      "CREATE INDEX stats_game_is_official ON stats_game(is_official);"
      "CREATE INDEX stats_game_player_count ON stats_game(player_count);"
      "CREATE INDEX stats_game_is_team_game ON stats_game(is_team_game);"
      "CREATE INDEX stats_game_team_count ON stats_game(team_count);"
      "CREATE INDEX stats_game_game_type ON stats_game(game_type COLLATE BINARY);"
      "CREATE INDEX stats_game_insertion_date ON stats_game(insertion_date);"


      /* stats_team */
      "DROP TABLE IF EXISTS stats_team;"
      "CREATE TABLE stats_team ("
      "   stats_team_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "   stats_game_id INTEGER NOT NULL,"
      "   team_name TEXT,"
      "   color_hex TEXT NOT NULL,"
      "   team_score INTEGER NULL,"
      "   result CHAR NULL,"
      "   insertion_date DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
      "   FOREIGN KEY(stats_game_id) REFERENCES stats_game(stats_game_id));"


      "CREATE INDEX stats_team_stats_game_id ON stats_team(stats_game_id);"
      "CREATE INDEX stats_team_result  ON stats_team(result COLLATE BINARY);"
      "CREATE INDEX stats_team_insertion_date ON stats_team(insertion_date);"


      /* stats_player */
      "DROP TABLE IF EXISTS stats_player;"
      "CREATE TABLE stats_player ("
      "   stats_player_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "   stats_game_id INTEGER NOT NULL,"
      "   player_name TEXT NOT NULL,"
      "   is_authenticated BOOL NOT NULL,"
      "   is_robot BOOL NOT NULL,"
      "   result CHAR  NOT NULL,"
      "   points INTEGER NOT NULL,"
      "   death_count INTEGER NOT NULL,"
      "   suicide_count INTEGER NOT NULL,"
      "   kill_count INTEGER NOT NULL,"
      "   asteroid_crashes INTEGER  NOT NULL,"
      "   flag_drops INTEGER  NOT NULL,"
      "   flag_pickups INTEGER  NOT NULL,"
      "   flag_returns INTEGER  NOT NULL,"
      "   flag_scores INTEGER  NOT NULL,"
      "   teleport_uses INTEGER  NOT NULL,"
      "   switched_team_count INTEGER  NULL,"
      "   stats_team_id INTEGER NULL,"
      "   insertion_date DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
      "   FOREIGN KEY(stats_game_id) REFERENCES stats_game(stats_game_id),"
      "   FOREIGN KEY(stats_team_id) REFERENCES stats_team(stats_team_id));"


      "CREATE INDEX stats_player_is_authenticated ON stats_player(is_authenticated);"
      "CREATE INDEX stats_player_result ON stats_player(result COLLATE BINARY);"
      "CREATE INDEX stats_player_stats_team_id ON stats_player(stats_team_id);"
      "CREATE INDEX stats_player_insertion_date ON stats_player(insertion_date);"
      "CREATE INDEX stats_player_player_name ON stats_player(player_name COLLATE BINARY);"
      "CREATE INDEX stats_player_is_robot ON stats_player(is_robot);"
      "CREATE INDEX stats_player_stats_game_id ON stats_player(stats_game_id);"

      /* stats_player_shots */
      "DROP TABLE IF EXISTS stats_player_shots;"
      "CREATE TABLE  stats_player_shots ("
      "   stats_player_shots_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "   stats_player_id INTEGER NOT NULL,"
      "   weapon TEXT NOT NULL,"
      "   shots INTEGER NOT NULL,"
      "   shots_struck INTEGER NOT NULL,"
      "   FOREIGN KEY(stats_player_id) REFERENCES stats_player(stats_player_id));"
         
      "   CREATE UNIQUE INDEX stats_player_shots_player_id_weapon on stats_player_shots(stats_player_id, weapon COLLATE BINARY);"

      /* stats_player_shots */
      "DROP TABLE IF EXISTS stats_player_loadout;"
      "CREATE TABLE  stats_player_loadout ("
      "   stats_player_loadout_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "   stats_player_id INTEGER NOT NULL,"
      "   loadout INTEGER NOT NULL,"
      "   FOREIGN KEY(stats_player_id) REFERENCES stats_player(stats_player_id));"

      "CREATE INDEX stats_player_loadout_stats_player_id ON stats_player_loadout(stats_player_id);"

      /* level info */

      "DROP TABLE IF EXISTS stats_level;"
      "CREATE TABLE stats_level ("
      "   stats_level_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "   level_name VARCHAR(255) default NULL,"
      "   creator VARCHAR(255) NOT NULL,"
      "   hash VARCHAR(32) NOT NULL,"
      "   game_type VARCHAR(32) NOT NULL,"
      "   has_levelgen TINYINT NOT NULL,"
      "   team_count INTEGER NOT NULL,"
      "   winning_score INTEGER NOT NULL,"
      "   game_duration INTEGER NOT NULL"
      ");"


      /* achievements */

      "DROP TABLE IF EXISTS player_achievements;"
      "CREATE TABLE player_achievements ("
      "   player_name TEXT NOT NULL,"
      "   achievement_id INTEGER NOT NULL,"
      "   server_id INTEGER NOT NULL,"
      "   date_awarded DATETIME NOT NULL  DEFAULT CURRENT_TIMESTAMP );"

      "   CREATE UNIQUE INDEX player_achievements_accomplishment_id on player_achievements(achievement_id, player_name COLLATE BINARY);";

   return schema;
}
