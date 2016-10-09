//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelInfoDatabaseMapping.h"
#include "stringUtils.h"

namespace Sqlite
{ 

const string SCHEMA_TABLE_NAME = "schema";
const string LEVEL_INFO_TABLE_NAME = "levelinfo";


string getCreateLevelInfoTableSql(S32 schemaVersion)
{
   return string("") +     // Coerce what follows into a string expression
      "DROP TABLE IF EXISTS " + SCHEMA_TABLE_NAME + ";"
      "CREATE TABLE " + SCHEMA_TABLE_NAME + " (version INTEGER NOT NULL);"
      "INSERT INTO " + SCHEMA_TABLE_NAME + " VALUES(" + Zap::itos(schemaVersion) + ");"
      "DROP TABLE IF EXISTS " + LEVEL_INFO_TABLE_NAME + ";"
      "CREATE TABLE " + LEVEL_INFO_TABLE_NAME + " (" +
      "level_info_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
#  define LEVEL_INFO_ITEM(index, column, type, d, e, f)  column + " " + type + "," +
      LEVEL_INFO_DATABASE_MAPPING_TABLE
#  undef LEVEL_INFO_ITEM
      "last_seen DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP);"

      "DROP TABLE IF EXISTS zones; "
      "CREATE TABLE zones (level_info_id INTEGER NOT NULL, zone_id INTEGER NOT NULL, zone_geom BLOB NOT NULL); "

      "DROP TABLE IF EXISTS zone_neighbors; "
      "CREATE TABLE zone_neighbors(level_info_id     INTEGER NOT NULL, "
      "write_order       INTEGER NOT NULL, "
      "origin_zone_id    INTEGER NOT NULL, "
      "dest_zone_id      INTEGER NOT NULL, "
      "border_start_geom BLOB NOT NULL, "
      "border_end_geom   BLOB NOT NULL);";


   // Generates something like this:
   // DROP TABLE IF EXISTS levels;
   // CREATE TABLE levels (
   //    level_info_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
   //    level_name  TEXT     NOT NULL,
   //    game_type   TEXT     NOT NULL,
   //    min_players INTEGER  NOT NULL,
   //    max_players INTEGER  NOT NULL,
   //    script_file TEXT     NOT NULL,
   //    last_seen   DATETIME NOT NULL   DEFAULT CURRENT_TIMESTAMP
   //  );
   //  DROP TABLE IF EXISTS ...
}

}
