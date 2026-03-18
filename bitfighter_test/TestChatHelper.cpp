//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ChatHelper.h"
#include "ClientGame.h"
#include "UIGame.h"
#include "UIManager.h"

#include "TestUtils.h"

#include "gtest/gtest.h"

namespace Zap
{

TEST(ChatHelperTest, RunCommandHandlesSlashAndCaseInsensitiveCommandNames)
{
   GamePair gamePair;
   gamePair.idle(10, 5);

   ClientGame *clientGame = gamePair.getClient(0);
   GameUserInterface *gameUI = clientGame->getUIManager()->getUI<GameUserInterface>();

   EXPECT_FALSE(gameUI->isShowingDebugShipCoords());  // Off at the start

   ChatHelper::runCommand(clientGame, "/SHOWCOORDS");	 
   EXPECT_TRUE(gameUI->isShowingDebugShipCoords());

   ChatHelper::runCommand(clientGame, "showcoords");
   EXPECT_FALSE(gameUI->isShowingDebugShipCoords());
}


TEST(ChatHelperTest, RunCommandSfxVolumeClampsAndRejectsInvalidInput)
{
   GamePair gamePair;
   gamePair.idle(10, 5);

   ClientGame *clientGame = gamePair.getClient(0);
   IniSettings *iniSettings = clientGame->getSettings()->getIniSettings();

   iniSettings->sfxVolLevel = 0.3f;

   ChatHelper::runCommand(clientGame, "/svol 15");
   EXPECT_FLOAT_EQ(1.0f, iniSettings->sfxVolLevel);

   ChatHelper::runCommand(clientGame, "/svol nope");
   EXPECT_FLOAT_EQ(1.0f, iniSettings->sfxVolLevel);

   ChatHelper::runCommand(clientGame, "/svol 0");
   EXPECT_FLOAT_EQ(0.0f, iniSettings->sfxVolLevel);
}


TEST(ChatHelperTest, RunCommandMaxFpsRequiresPositiveInteger)
{
   GamePair gamePair;
   gamePair.idle(10, 5);

   ClientGame *clientGame = gamePair.getClient(0);
   IniSettings *iniSettings = clientGame->getSettings()->getIniSettings();

   iniSettings->maxFPS = 100;

   ChatHelper::runCommand(clientGame, "/maxfps 72");
   EXPECT_EQ(72u, iniSettings->maxFPS);

   ChatHelper::runCommand(clientGame, "/maxfps 0");
   EXPECT_EQ(72u, iniSettings->maxFPS);

   ChatHelper::runCommand(clientGame, "/maxfps -5");
   EXPECT_EQ(72u, iniSettings->maxFPS);
}


TEST(ChatHelperTest, RunCommandIgnoresEmptyInput)
{
   GamePair gamePair;
   gamePair.idle(10, 5);

   ClientGame *clientGame = gamePair.getClient(0);
   GameUserInterface *gameUI = clientGame->getUIManager()->getUI<GameUserInterface>();
   IniSettings *iniSettings = clientGame->getSettings()->getIniSettings();

   const bool showingCoordsAtStart = gameUI->isShowingDebugShipCoords();
   const F32 sfxVolAtStart = iniSettings->sfxVolLevel;
   const U32 maxFpsAtStart = iniSettings->maxFPS;

   ChatHelper::runCommand(clientGame, "/");
   ChatHelper::runCommand(clientGame, "    ");

   EXPECT_EQ(showingCoordsAtStart, gameUI->isShowingDebugShipCoords());
   EXPECT_FLOAT_EQ(sfxVolAtStart, iniSettings->sfxVolLevel);
   EXPECT_EQ(maxFpsAtStart, iniSettings->maxFPS);
}

};
