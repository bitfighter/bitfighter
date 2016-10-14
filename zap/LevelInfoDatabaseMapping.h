//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LEVELINFO_DATABASE_MAPPING_H_
#define _LEVELINFO_DATABASE_MAPPING_H_

#include "tnlTypes.h"
#include <string>

using std::string;
using namespace TNL;

#define LEVEL_INFO_DATABASE_MAPPING_TABLE \
   /*           Index,  Db column,     DbField Type                 LevelInfo field   Db to LevelInfo transformation   LevelInfo to Db                */  \
   LEVEL_INFO_ITEM(0,   "hash",        "TEXT NOT NULL",             mLevelHash,       ,                                                                )  \
   LEVEL_INFO_ITEM(1,   "level_name",  "TEXT NOT NULL",             mLevelName,       ,                                STE2String                      )  \
   LEVEL_INFO_ITEM(2,   "game_type",   "TEXT NOT NULL",             mLevelType,       GameType::getGameTypeIdFromName, GameType::getGameTypeClassName  )  \
   LEVEL_INFO_ITEM(3,   "min_players", "INT  NOT NULL",             minRecPlayers,    atoi,                                                            )  \
   LEVEL_INFO_ITEM(4,   "max_players", "INT  NOT NULL",             maxRecPlayers,    atoi,                                                            )  \
   LEVEL_INFO_ITEM(5,   "script_file", "TEXT NOT NULL",             mScriptFileName,  ,                                                                )  \


namespace Sqlite
{

extern const string SCHEMA_TABLE_NAME;
extern const string LEVEL_INFO_TABLE_NAME;
extern const string ZONE_TABLE_NAME;
extern const string NEIGHBOR_TABLE_NAME;

string getCreateLevelInfoTableSql(TNL::S32 schemaVersion);
string getClearOutOldLevelsSql();

}

#endif
