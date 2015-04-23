//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelFilesForTesting.h"
#include "LevelSource.h"

#include "stringUtils.h"
#include "tnlVector.h"

namespace Zap
{

using namespace std;


static Vector<string>    levelCodes;
static Vector<LevelInfo> levelInfos;

// Lazy initialization
static void initialize()
{
   if(levelCodes.size() > 0)
      return;

   // levelCodes[0]
   levelCodes.push_back(
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
   );
   levelInfos.push_back(LevelInfo("Test Level", BitmatchGame, 0, 0, "" ));

   // levelCodes[1]
   // This level has a spawn in a LoadoutZone, with a ResourceItem directly south of the spawn
   levelCodes.push_back(
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
   );
   levelInfos.push_back(LevelInfo("Engineer Test Bed One", BitmatchGame, 0, 0, ""));

   /////
   // levelCodes[2]
   // Test some missing name permutations
   levelCodes.push_back(
      "GameType 10 92\n"
      "LevelName\n"     // No name, space
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
   );
   levelInfos.push_back(LevelInfo("", BitmatchGame, 0, 0, ""));


   // levelCodes[3]
   levelCodes.push_back(
      "GameType 10 92\n"
      "LevelName \n"     // No name, one space
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
   );
   levelInfos.push_back(LevelInfo("", BitmatchGame, 0, 0, ""));

   // levelCodes[4]
   // Test some missing name permutations
   levelCodes.push_back(
      "GameType 10 92\n"
      "LevelName  \n"     // No name, two spaces space
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
   );
   levelInfos.push_back(LevelInfo("", BitmatchGame, 0, 0, ""));

   // levelCodes[5]
   // Test engineered item snapping
   levelCodes.push_back(
      "LevelFormat 2\n"
      "NexusGameType 9 1 10 1000\n"
      "LevelName NexusTestGame\n"
      "LevelDescription Level for testing snapping\n"
      "LevelCredits Bitfighter Snapper\n"
      "Team White 1 1 1\n"
      "Specials\n"
      "MinPlayers 2\n"
      "MaxPlayers 4\n"
      "BarrierMaker 20 0 0 100 0\n" // Horizontal wall, 20 thick
      "Turret 0 30 1\n"             // Turret slightly off-center, so should snap to top of wall
   );
   levelInfos.push_back(LevelInfo("NexusTestGame", NexusGame, 2, 4, ""));


   // levelCodes[6]
   // Test engineered item snapping 2
   levelCodes.push_back(
      "LevelFormat 2\n"
      "GameType 8 8\n"
      "LevelName \"Snapping Testing Level Part II!!!\"\n"
      "LevelDescription \"This is a very simple level!\"\n"
      "LevelCredits Fortran1234\n"
      "Team Blue 0 0 1\n"
      "Specials\n"
      "MinPlayers\n"
      "MaxPlayers 999\n"
      "PolyWall -255 -76.5 -255 0 -25.5 0 -25.5 -76.5\n"          // Wall wound in default order
      "Turret!1 0 -128 -0.1 0\n"                                  // Turret <Team> <X> <Y> [HealRate]
      "PolyWall -25.5 -176.5 -25.5 -100 -255 -100 -255 -176.5\n"  // Wall wound in reverse order
      "Turret!2 0 -128 -100.1 0\n"                              
   );
   levelInfos.push_back(LevelInfo("Snapping Testing Level Part II!!!", BitmatchGame, 0, 999, ""));

   // levelCodes[7]
   // Test different item propagation scenarios
   levelCodes.push_back(
      "LevelFormat 2\n"
      "GameType 10 8\n"
      "LevelName Item propagation test level\n"
      "LevelDescription \"\"\n"
      "LevelCredits Invisible\n"
      "Team Blue 0 0 1\n"
      "Team Red 1 0 0\n"
      "Specials\n"
      "MinPlayers 1\n"
      "MaxPlayers 3\n"
   );
   levelInfos.push_back(LevelInfo("Item propagation test level", BitmatchGame, 1, 3, ""));
}


// Note that the following functions need to be declared in LevelFilesForTesting.h
string getLevelCodeForEngineeredItemSnapping()
{
   initialize();
   return levelCodes[5];
}


string getLevelCodeForEngineeredItemSnapping2()
{
   initialize();
   return levelCodes[6];
}


string getLevelCode1()
{
   initialize();
   return levelCodes[0];
}

string getLevelCode3()
{
   initialize();
   return levelCodes[2];
}

string getLevelCodeForTestingEngineer1()
{
   initialize();
   return levelCodes[1];
}


string getLevelCodeForItemPropagationTests(const string &object)
{
   initialize();
   return levelCodes[7] + object + "\n";
}


pair<Vector<string>, Vector<LevelInfo> > getLevels()
{
   initialize();
   return pair<Vector<string>, Vector<LevelInfo> >(levelCodes, levelInfos);
}


string getGenericHeader()
{
   return "GameType 8 15\n"
          "LevelName \"Bitmatch01\"\n"
          "LevelDescription \"\"\n"
          "LevelCredits \n"
          "GridSize 255\n"
          "Team Blue 0 0 1\n"
          "Specials\n"
          "MinPlayers\n"
          "MaxPlayers\n";
}


// Botspec looks like this:
// BBB BB  for 2 teams with 3 bots on first team, 2 on second
// Use 0 for a team with no bots
string getLevelCodeForEmptyLevelWithBots(const string &botSpec)
{
   Vector<string> words = parseString(botSpec);
   S32 teams = words.size();

   string level =
      "LevelFormat 2\n"
      "GameType 10 8\n"
      "LevelName TwoBots\n"
      "LevelDescription\n"
      "LevelCredits Tyler Derden\n";

   for(S32 i = 0; i < teams; i++)
      level += "Team team" + itos(i) + " 0 0 0\n";

   level +=
      "Specials\n"
      "MinPlayers\n"
      "MaxPlayers\n";

   for(S32 i = 0; i < teams; i++)
      if(words[i] != "0")
         for(string::size_type j = 0; j < words[i].size(); j++)
            level += "Robot " + itos(i) + " s_bot\n";

   return level;
}


// Generate an emtpy level with the specified number of teams
string getMultiTeamLevelCode(S32 teams)
{
   string code = 
      "LevelFormat 2\n"
      "GameType 8 8\n"
      "LevelName LevelShell\n"
      "LevelDescription Test level with a specified number of levels\n"
      "LevelCredits AutoGenerated Level\n";

   for(S32 i = 0; i < teams; i++)
      code += "Team Team" + itos(i) + " " + itos(i) + " " + itos(i) + " " + itos(i) + "\n";

   code +=
      "Specials\n"
      "MinPlayers\n"
      "MaxPlayers 999\n";

   return code;
}

};