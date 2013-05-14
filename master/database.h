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

#ifndef DATABASE_H
#define DATABASE_H

#include "../zap/gameWeapons.h"     // For WeaponType enum
#include "../zap/gameStats.h"
#include "../zap/SharedConstants.h"

#include "tnlTypes.h"
#include "tnlVector.h"
#include "tnlNonce.h"
#include "../sqlite/sqlite3.h"
#include <string>

#ifdef BF_WRITE_TO_MYSQL
#include "mysql++.h"
using namespace mysqlpp;
#endif


#ifndef BF_WRITE_TO_MYSQL
struct Query {};  // Dummy object for SQLite
#endif

using namespace TNL;
using namespace Zap;
using namespace std;


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

   Int<BADGE_COUNT> getAchievements(const char *name);
};


#endif
