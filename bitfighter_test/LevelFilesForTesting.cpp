//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelFilesForTesting.h"
#include <string>

namespace Zap
{

using namespace std;

string getLevelCode1()
{
   return
      "GameType 10 8\n"
      "LevelName \"Test Level\"\n"                             // Has quotes
      "LevelDescription \"This is a basic test level\"\n"      // Has quotes
      "LevelCredits level creator\n"                           // No quotes
      "GridSize 255\n"
      "Team Bluey 0 0 1\n"
      "Specials\n"
      "MinPlayers\n"
      "MaxPlayers\n"
      "BarrierMaker 40 -1 -1 -1 1\n"
      "RepairItem 0 1 10\n"
      "Spawn 0 -0.6 0\n"
      "Teleporter 5 5 10 10\n"
      "TestItem 1 1\n"
   ;
}


// This level has a spawn in a LoadoutZone, with a ResourceItem directly south of the spawn
string getLevelCodeForTestingEngineer1()
{
   return
      "GameType 10 92\n"
      "LevelName Engineer Test Bed One\n"
      "LevelDescription Level for testing Engineer\n"
      "LevelCredits Bitfighter Test Engineer #42445\n"
      "GridSize 255\n"
      "Team Blue 0 0 1\n"
      "Specials Engineer\n"
      "MinPlayers\n"
      "MaxPlayers\n"
      "LoadoutZone 0   1 0   1 1   0 1   0 0\n"
      "Spawn 0   .5 .5\n"
      "ResourceItem   0.5 1\n"
   ;
}

};