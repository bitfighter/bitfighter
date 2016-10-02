//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "FileList.h"

#include "gtest/gtest.h"
#include "LevelSource.h"

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


   TEST(DatabaseTest, GenerateSql)
   {
      LevelInfo levelInfo;

      char *results[] = { "8f0e423dd3cc5f1ef2d48a4781fb98bf", "Level Name", "NexusGameType", "1", "5", "killemall" };

      levelInfo.populateFromDatabaseResults(results);
      string sql = levelInfo.toSql();
      string expectedSql = "insert into " + LevelInfo::LEVEL_INFO_TABLE_NAME + "(hash,level_name,game_type,min_players,max_players,script_file,last_seen) "
                           "values ('8f0e423dd3cc5f1ef2d48a4781fb98bf','Level Name','NexusGameType','1','5','killemall',datetime('now'));";

      cleanString(sql);
      cleanString(expectedSql);

      ASSERT_TRUE(sql == expectedSql) << "|got|" << sql << "|expected|" << expectedSql << "||";

      sql = levelInfo.getCreateTableSql(1);
      expectedSql = "DROP TABLE IF EXISTS " + LevelInfo::SCHEMA_TABLE_NAME + ";"
                    "CREATE TABLE " + LevelInfo::SCHEMA_TABLE_NAME + " (version INTEGER NOT NULL);"
                    "INSERT INTO " + LevelInfo::SCHEMA_TABLE_NAME + " VALUES(1);"
                    "DROP TABLE IF EXISTS " + LevelInfo::LEVEL_INFO_TABLE_NAME + ";"
                    "CREATE TABLE " + LevelInfo::LEVEL_INFO_TABLE_NAME + 
                    "(hash TEXT NOT NULL PRIMARY KEY, level_name TEXT NOT NULL,"
                    "game_type TEXT NOT NULL, min_players INT NOT NULL, max_players INT NOT NULL,"
                    "script_file TEXT NOT NULL, last_seen   DATETIME NOT NULL   DEFAULT CURRENT_TIMESTAMP);";

      cleanString(sql);
      cleanString(expectedSql);

      ASSERT_TRUE(sql == expectedSql) << "|got|" << sql << "|expected|" << expectedSql << "||";


      sql = levelInfo.getSelectSql("8f0e423dd3cc5f1ef2d48a4781fb98bf");
      expectedSql = "SELECT hash, level_name, game_type, min_players, max_players, script_file, 1"
                    "FROM " + LevelInfo::LEVEL_INFO_TABLE_NAME + 
                    "WHERE hash = '8f0e423dd3cc5f1ef2d48a4781fb98bf';";

      cleanString(sql);
      cleanString(expectedSql);

      ASSERT_TRUE(sql == expectedSql) << "|got|" << sql << "|expected|" << expectedSql << "||";

   }
}
