//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LEVEL_INFO_DATABASE_H_
#define _LEVEL_INFO_DATABASE_H_

#include "GameTypesEnum.h"
#include "tnlTypes.h"

#include <string>
#include <map>
#include <iostream>        


using namespace TNL;
using namespace std;
using namespace Zap;

namespace LevelInfoDb
{


struct LevelInfoDatabase {
   map<string, string> levelName;
   map<string, GameTypeId> gameTypeId;
   map<string, S32> minPlayers;
   map<string, S32> maxPlayers;
   map<string, S32> levelSize;
   map<string, string> scriptName;
};


void readCsvFromStream(istream &stream, LevelInfoDatabase &levelDb);
bool readCsv(const string &filename);

} 

#endif 
