//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LEVEL_FILES_FOR_TESTING_H_
#define _LEVEL_FILES_FOR_TESTING_H_

#include "LevelSource.h"

//#include "tnlVector.h"
#include <string>

namespace Zap
{

using namespace std;
using namespace TNL;

string getLevelCode1();
string getLevelCodeForTestingEngineer1();
string getLevelCodeForEmptyLevelWithBots(const string &botspec);

pair<Vector<string>, Vector<LevelInfo> > getLevels();

};

#endif