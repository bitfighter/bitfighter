#include "gameType.h"
#include "ServerGame.h"

#include "gtest/gtest.h"

#include <string>

namespace Zap
{

class ServerGameTest: public testing::Test
{

};

TEST_F(ServerGameTest, processEmptyLevelLine)
{
   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());

   ServerGame g(addr, settings, false, false);
   GameType gt;
   gt.addToGame(&g, g.getGameObjDatabase());
   g.loadLevelFromString(g.toLevelCode() + "\r\n\r\n", g.getGameObjDatabase());
}

};
