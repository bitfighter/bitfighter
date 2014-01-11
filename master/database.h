//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _DATABASE_H_
#define _DATABASE_H_

#include "../zap/gameWeapons.h"     // For WeaponType enum
#include "../zap/gameStats.h"
#include "../zap/SharedConstants.h"

#include "tnlTypes.h"
#include "tnlVector.h"
#include "tnlNonce.h"
#include "../sqlite/sqlite3.h"
#include <string>


#ifdef BF_WRITE_TO_MYSQL
#  include "mysql++.h"
   using namespace mysqlpp;
#endif


#ifndef BF_WRITE_TO_MYSQL
   struct Query {};  // Dummy object for SQLite
#endif

using namespace TNL;
using namespace Zap;
using namespace std;

namespace Master {
   class MasterSettings;
}


namespace DbWriter
{

struct ServerInfo
{
   U64 id;
   string name;
   string ip;

   // Quickie constructor
   ServerInfo(U64 id, const string name, const string ip) 
   { 
      this->id = id; 
      this->name = name; 
      this->ip = ip;
   }
};


////////////////////////////////////////
////////////////////////////////////////

class DbQuery
{
#ifdef BF_WRITE_TO_MYSQL
   Connection conn;
#endif

public:
   Query *query;
   sqlite3 *sqliteDb;

   bool isValid;
   static bool dumpSql;
   
   DbQuery(const char *db, const char *server = NULL, const char *user = NULL, const char *password = NULL);     // Constructor
   ~DbQuery();                      // Destructor

   U64 runQuery(const string &sql) const;
};


////////////////////////////////////////
////////////////////////////////////////


class DatabaseWriter 
{
private:
   char mServer[64];   // was const char *, but problems when data in pointer dies.
   char mDb[64];
   char mUser[64];
   char mPassword[64];
   Vector<ServerInfo> cachedServers;

   S32 lastGameID;

   void initialize(const char *server, const char *db, const char *user, const char *password);
   void createStatsDatabase();
   string getSqliteSchema();

   U64 getServerID(const DbQuery &query, const string &serverName, const string &serverIP);

   void addToServerCache(U64 id, const string &serverName, const string &serverIPAddr);         // Add database ID to our cache
   U64 getServerIDFromCache(const string &serverName, const string &serverIPAddr);              // And get it back out again

   S32 getServerIdFromDatabase(const DbQuery &query, const string &serverName, const string &serverIP);

public:
   DatabaseWriter();

   // MySQL constructors
   DatabaseWriter(const char *server, const char *db, const char *user, const char *password);     // Constructor
   DatabaseWriter(const char *db, const char *user, const char *password);                         // Constructor

   // SQLite constructor
   DatabaseWriter(const char *db);

   void selectHandler(const string &sql, S32 cols, Vector<Vector<string> > &values);

   void setDumpSql(bool dump);

   void insertStats(const GameStats &gameStats);
   void insertAchievement(U8 achievementId, const StringTableEntry &playerNick, const string &serverName, const string &serverIP);
   void insertLevelInfo(const string &hash, const string &levelName, const string &creator, 
                        const string &gameType, bool hasLevelGen, U8 teamCount, S32 winningScore, S32 gameDurationInSeconds);

   void getTopPlayers(const string &table, const string &col2, S32 count, Vector<string> &names, Vector<string> &scores);
   Vector<string> getGameJoltCredentialStrings(const string &phpbbDatabase, const string &nameList, S32 nameCount);

   S16 getLevelRating(U32 databaseId);                                  // Returns average rating of the specified level
   S32 getLevelRating(U32 databaseId, const StringTableEntry &name);    // Returns player's rating of the specified level

   Int<BADGE_COUNT> getAchievements(const char *name);
   U16 getGamesPlayed(const char *name);
};


DatabaseWriter getDatabaseWriter(const Master::MasterSettings *settings);

}


#endif
