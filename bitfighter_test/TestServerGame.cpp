#include "gameType.h"
#include "ServerGame.h"

#include "gtest/gtest.h"

#include <string>

namespace Zap
{

class ServerGameTest: public testing::Test
{

};


// What does this function actually do?  There are no tests here!
TEST_F(ServerGameTest, processEmptyLevelLine)
{
   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(""));

   ServerGame g(addr, settings, levelSource, false, false);
   GameType gt;
   gt.addToGame(&g, g.getGameObjDatabase());
   g.loadLevelFromString(g.toLevelCode() + "\r\n\r\n", g.getGameObjDatabase());
}

};
