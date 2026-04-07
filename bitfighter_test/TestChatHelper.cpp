//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ChatHelper.h"
#include "ClientGame.h"
#include "UIGame.h"
#include "UIManager.h"
#include "gameConnection.h"
#include "LevelSource.h"

#include "TestUtils.h"
#include "LevelFilesForTesting.h"

#include "gtest/gtest.h"

namespace Zap
{

static ChatHelper *enterCommandChat(GameUserInterface *gameUI)
{
   gameUI->activateHelper(HelperMenu::ChatHelperType);

   if(!gameUI->isHelperActive(HelperMenu::ChatHelperType))
   {
      ADD_FAILURE() << "Expected ChatHelper to be active";
      return NULL;
   }

   const HelperMenu *activeHelper = gameUI->getActiveHelper();
   if(activeHelper == NULL)
   {
      ADD_FAILURE() << "Expected non-null active helper";
      return NULL;
   }

   const ChatHelper *chatHelperConst = dynamic_cast<const ChatHelper *>(activeHelper);
   if(chatHelperConst == NULL)
   {
      ADD_FAILURE() << "Expected active helper to be ChatHelper";
      return NULL;
   }

   ChatHelper *chatHelper = const_cast<ChatHelper *>(chatHelperConst);
   chatHelper->activate(ChatHelper::CmdChat);

   return chatHelper;
}



static void clearChat(ChatHelper *chatHelper)
{
   S32 len = (S32)strlen(chatHelper->getChatMessage());
   for(S32 i = 0; i < len; i++)
      chatHelper->processInputCode(KEY_BACKSPACE);
}


static void typeInChat(ChatHelper *chatHelper, const string &text)
{
   for(char c : text)
      chatHelper->onTextInput(c);
}


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


TEST(ChatHelperTest, GetCandidateListCompletesCommandNames)
{
   GamePair gamePair;
   gamePair.idle(10, 5);

   ClientGame *clientGame = gamePair.getClient(0);
   GameUserInterface *gameUI = clientGame->getUIManager()->getUI<GameUserInterface>();
   ChatHelper *chatHelper = enterCommandChat(gameUI);
   ASSERT_TRUE(chatHelper != NULL);

   typeInChat(chatHelper, "dlm");

   EXPECT_TRUE(chatHelper->processInputCode(KEY_TAB));
   EXPECT_STREQ("dlmap", chatHelper->getChatMessage());
}


TEST(ChatHelperTest, GetCandidateListCompletesPlayerNameArguments)
{
   GamePair gamePair;
   gamePair.addClient("AlphaTester");
   gamePair.addClient("111_BetaTester");     // Starts with number
   gamePair.idle(10, 5);

   ClientGame *clientGame = gamePair.getClient(0);
   GameUserInterface *gameUI = clientGame->getUIManager()->getUI<GameUserInterface>();
   ChatHelper *chatHelper = enterCommandChat(gameUI);
   ASSERT_TRUE(chatHelper != NULL);

   typeInChat(chatHelper, "pm Al");
   EXPECT_TRUE(chatHelper->processInputCode(KEY_TAB));
   EXPECT_STREQ("pm AlphaTester", chatHelper->getChatMessage());

   // Test that player names that start with numbers are also completed correctly
   clearChat(chatHelper);
   typeInChat(chatHelper, "pm 1");
   EXPECT_TRUE(chatHelper->processInputCode(KEY_TAB));
   EXPECT_STREQ("pm 111_BetaTester", chatHelper->getChatMessage());
}


TEST(ChatHelperTest, GetCandidateListCompletesTeamArguments)
{
   GamePair gamePair(getLevelCode1());
   gamePair.idle(10, 5);

   ClientGame *clientGame = gamePair.getClient(0);
   GameUserInterface *gameUI = clientGame->getUIManager()->getUI<GameUserInterface>();
   ChatHelper *chatHelper = enterCommandChat(gameUI);
   ASSERT_TRUE(chatHelper != NULL);

   typeInChat(chatHelper, "addbot bot Bl");

   EXPECT_TRUE(chatHelper->processInputCode(KEY_TAB));
   EXPECT_STREQ("addbot bot Bluey", chatHelper->getChatMessage());
}


TEST(ChatHelperTest, GetCandidateListCompletesLevelArgumentsAndWrapsQuotes)
{
   GamePair gamePair;
   gamePair.idle(10, 5);

   ClientGame *clientGame = gamePair.getClient(0);
   GameUserInterface *gameUI = clientGame->getUIManager()->getUI<GameUserInterface>();
   ChatHelper *chatHelper = enterCommandChat(gameUI);
   ASSERT_TRUE(chatHelper != NULL);

   // Ensure a deterministic level list candidate for LEVEL argument completion.
   GameConnection *connection = clientGame->getConnectionToServer();
   ASSERT_TRUE(connection != NULL);

   connection->mLevelInfos.clear();
   LevelInfo levelInfo;
   levelInfo.mLevelName = StringTableEntry("Test Level", false);
   connection->mLevelInfos.push_back(levelInfo);

   typeInChat(chatHelper, "map Te");

   EXPECT_TRUE(chatHelper->processInputCode(KEY_TAB));
   EXPECT_STREQ("map \"Test Level\"", chatHelper->getChatMessage());
}


TEST(ChatHelperTest, GetCandidateListLeavesNumericArgumentsUnchanged)
{
   GamePair gamePair;
   gamePair.idle(10, 5);

   ClientGame *clientGame = gamePair.getClient(0);
   GameUserInterface *gameUI = clientGame->getUIManager()->getUI<GameUserInterface>();
   ChatHelper *chatHelper = enterCommandChat(gameUI);
   ASSERT_TRUE(chatHelper != NULL);

   typeInChat(chatHelper, "svol 1");

   EXPECT_TRUE(chatHelper->processInputCode(KEY_TAB));
   EXPECT_STREQ("svol 1", chatHelper->getChatMessage());
}

};
