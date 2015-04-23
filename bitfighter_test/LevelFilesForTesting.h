//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LEVEL_FILES_FOR_TESTING_H_
#define _LEVEL_FILES_FOR_TESTING_H_

#include "LevelSource.h"

#include <string>

namespace Zap
{

using namespace std;
using namespace TNL;

string getLevelCode1();
string getLevelCode3();
string getLevelCodeForTestingEngineer1();
string getLevelCodeForEmptyLevelWithBots(const string &botspec);
string getLevelCodeForEngineeredItemSnapping();
string getLevelCodeForEngineeredItemSnapping2();
string getLevelCodeForItemPropagationTests(const string &object);
string getMultiTeamLevelCode(S32 teams);


string getGenericHeader();

pair<Vector<string>, Vector<LevelInfo> > getLevels();

};

#endif