//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------


#include "BotNavMeshZone.h"
#include "FileList.h"
#include "LevelInfoDatabaseMapping.h"
#include "LevelSource.h"

#include "gtest/gtest.h"
#include "ConsoleLogConsumer.h"

namespace Zap
{
   void cleanString(string &str)
   {
      // Strip whitespace to make comparison less brittle
      // http://stackoverflow.com/questions/83439/remove-spaces-from-stdstring-in-c
      str.erase(remove_if(str.begin(), str.end(), isspace), str.end());

      // And convert it to lowercase
      transform(str.begin(), str.end(), str.begin(), tolower);
   }


   // These tests are pretty brittle, and may have outlived their utility.  If they break, and fixing them is a pain,
   // they can be deleted.  The main thing they were intended to test is the x-macro expansions used to generate the 
   // column lists and such... the rest is just boilerplate.
   TEST(DatabaseTest, GenerateSql)
   {
      LevelInfo levelInfo;

      char *results[] = {"col_header_hash", "col_header_level_name", "col_header_game_type", "minplayers", "maxPlayers", "script", "levelId",
                         "8f0e423dd3cc5f1ef2d48a4781fb98bf", "Level Name", "NexusGameType", "1", "5", "killemall", "999" };

      levelInfo.populateFromDatabaseResults(results);
      string sql = levelInfo.toSql();
      string expectedSql = "insert into " + Sqlite::LEVEL_INFO_TABLE_NAME + "(hash,level_name,game_type,min_players,max_players,script_file,last_seen) "
                           "values ('8f0e423dd3cc5f1ef2d48a4781fb98bf','Level Name','NexusGameType','1','5','killemall',datetime('now'));";

      cleanString(sql);
      cleanString(expectedSql);

      ASSERT_TRUE(sql == expectedSql) << "|got|" << sql << "|expected|" << expectedSql << "||";

      sql = Sqlite::getCreateLevelInfoTableSql(1);
      expectedSql = "DROP TABLE IF EXISTS " + Sqlite::SCHEMA_TABLE_NAME + ";"
                    "CREATE TABLE " + Sqlite::SCHEMA_TABLE_NAME + " (version INTEGER NOT NULL);"
                    "INSERT INTO " + Sqlite::SCHEMA_TABLE_NAME + " VALUES(1);"
                    "DROP TABLE IF EXISTS " + Sqlite::LEVEL_INFO_TABLE_NAME + ";"
                    "CREATE TABLE " + Sqlite::LEVEL_INFO_TABLE_NAME +
                    "(level_info_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                    "hash              TEXT NOT NULL,"
                    "level_name TEXT NOT NULL,"
                    "game_type TEXT NOT NULL, min_players INT NOT NULL, max_players INT NOT NULL,"
                    "script_file TEXT NOT NULL, last_seen   DATETIME NOT NULL   DEFAULT CURRENT_TIMESTAMP);"
                    "DROP TABLE IF EXISTS " + Sqlite::ZONE_TABLE_NAME + "; "
                    "CREATE TABLE " + Sqlite::ZONE_TABLE_NAME + " (level_info_id INTEGER NOT NULL, zone_id INTEGER NOT NULL, zone_geom BLOB NOT NULL); "
                    "DROP TABLE IF EXISTS " + Sqlite::NEIGHBOR_TABLE_NAME + "; "
                    "CREATE TABLE " + Sqlite::NEIGHBOR_TABLE_NAME + "("
                    "level_info_id     INTEGER NOT NULL, "
                    "write_order       INTEGER NOT NULL, "
                    "origin_zone_id    INTEGER NOT NULL, "
                    "dest_zone_id      INTEGER NOT NULL, "
                    "border_start_geom BLOB NOT NULL, "
                    "border_end_geom   BLOB NOT NULL);";

      cleanString(sql);
      cleanString(expectedSql);

      ASSERT_TRUE(sql == expectedSql) << "|got|" << sql << "|expected|" << expectedSql << "||";


      sql = levelInfo.getSelectSql("8f0e423dd3cc5f1ef2d48a4781fb98bf");
      expectedSql = "SELECT hash, level_name, game_type, min_players, max_players, script_file, level_info_id"
                    "FROM " + Sqlite::LEVEL_INFO_TABLE_NAME + 
                    "WHERE hash = '8f0e423dd3cc5f1ef2d48a4781fb98bf';";

      cleanString(sql);
      cleanString(expectedSql);

      ASSERT_TRUE(sql == expectedSql) << "|got|" << sql << "|expected|" << expectedSql << "||";
   }
   

   TEST(DatabaseTest, GeometryRoundTrip)
   {
      // This geometry makes no sense, but at least we can see if it makes it to the database and back!
      Vector<BotNavMeshZone *> zonesIn, zonesOut;
      Vector<Point> points1, points2;

      BotNavMeshZone zone1(0), zone2(1);     // Pass zoneIds
      points1.push_back(Point(30.5, 100));
      points1.push_back(Point(30, -100));
      points1.push_back(Point(80, -100));
      points1.push_back(Point(30, -900));
      zone1.setGeometry(points1);

      points2.push_back(Point(90.5,  100));
      points2.push_back(Point(90,    100));
      points2.push_back(Point(90,    100));
      points2.push_back(Point(60, -2.9876));
      zone2.setGeometry(points2);

      NeighboringZone neighbor1(0, points1[0], points1[1], points1[2] * 2, points1[3] * 3);
      NeighboringZone neighbor2(1, points2[0], points2[1], points2[2] * 2, points2[3] * 3);


      zone1.addNeighbor(neighbor1);
      zone2.addNeighbor(neighbor2);

      zonesIn.push_back(&zone1);
      zonesIn.push_back(&zone2);

      U64 levelId = S32_MAX - 1; 
      string dbName = "test_levelinfo.db";

      DbWriter::DbQuery query(LevelInfo::LEVEL_INFO_DATABASE_NAME.c_str());

      // Create empty file on file system
      DbWriter::DatabaseWriter::createLevelDatabase(dbName, LevelInfo::LEVEL_DATABASE_SCHEMA_VERSION);

      BotNavMeshZone::saveBotZonesToSqlite(dbName, zonesIn, levelId);
      // Time passes
      ASSERT_TRUE(BotNavMeshZone::tryToLoadZonesFromSqlite(dbName, levelId, zonesOut));

      ASSERT_EQ(zonesIn.size(), zonesOut.size());


      //for(S32 i = 0; i < zonesIn.size(); i++)
      //   for(S32 j = 0; j < zonesIn[i]->getNeighbors().size(); j++)
      //   {
      //      printf("%d,%d, %s,%s,%s,%s,%s,%s\n", zonesIn[i]->getNeighbors()[j].zoneID, zonesOut[i]->getNeighbors()[j].zoneID,
      //         zonesIn[i]->getNeighbors()[j].center.toString().c_str(), zonesOut[i]->getNeighbors()[j].center.toString().c_str(),
      //         zonesIn[i]->getNeighbors()[j].borderStart.toString().c_str(), zonesOut[i]->getNeighbors()[j].borderStart.toString().c_str(),
      //         zonesIn[i]->getNeighbors()[j].borderEnd.toString().c_str(), zonesOut[i]->getNeighbors()[j].borderEnd.toString().c_str());
      //   }
      for(S32 i = 0; i < zonesIn.size(); i++)
      {
         ASSERT_EQ(zonesIn[i] ->getGeometry().getGeometry()->getOutline()->size(),
                   zonesOut[i]->getGeometry().getGeometry()->getOutline()->size());

         for(S32 j = 0; j < zonesIn[i]->getGeometry().getGeometry()->getOutline()->size(); j++)
            ASSERT_EQ(zonesIn[i] ->getGeometry().getGeometry()->getOutline()->get(j),
                      zonesOut[i]->getGeometry().getGeometry()->getOutline()->get(j));

         ASSERT_EQ(zonesIn[i]->getNeighbors().size(), zonesOut[i]->getNeighbors().size());
         for(S32 j = 0; j < zonesIn[i]->getNeighbors().size(); j++)
         {
            ASSERT_TRUE(zonesIn[i]->getNeighbors()[j].zoneID       == zonesOut[i]->getNeighbors()[j].zoneID);
            ASSERT_TRUE(zonesIn[i]->getNeighbors()[j].borderStart  == zonesOut[i]->getNeighbors()[j].borderStart);
            ASSERT_TRUE(zonesIn[i]->getNeighbors()[j].borderEnd    == zonesOut[i]->getNeighbors()[j].borderEnd);
         }
      }

      sqlite3 *sqliteDb = DbWriter::DatabaseWriter::openSqliteDatabase(dbName, SQLITE_OPEN_READWRITE);
      ASSERT_TRUE(sqliteDb != NULL);

      BotNavMeshZone::clearZonesFromDatabase(sqliteDb, levelId);
      ASSERT_EQ(sqlite3_close_v2(sqliteDb), SQLITE_OK);

      // Delete the database file
      remove(dbName.c_str());
   }
}
