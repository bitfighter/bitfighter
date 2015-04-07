//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TestUtils.h"
#include "gameType.h"
#include "GameManager.h"
#include "UIGame.h"
#include "ServerGame.h"
#include "ClientGame.h"
#include "ChatHelper.h"
#include "helperMenu.h"
#include "FontManager.h"
#include "DisplayManager.h"
#include "UIManager.h"
#include "SystemFunctions.h"
#include "Level.h"
#include "tnlAssert.h"

#include "../zap/stringUtils.h"
#include "gtest/gtest.h"

#include <string>

using namespace std;

namespace Zap
{

   static GameManager gameManager;

// Create a new ClientGame with one dummy team -- be sure to delete this somewhere!
ClientGame *newClientGame()
{
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   return newClientGame(settings);
}


ClientGame *newClientGame(const GameSettingsPtr &settings)
{
   Address addr;
   
   ClientGame *game = new ClientGame(addr, settings, new UIManager());    // ClientGame destructor will clean up UIManager

   //game->addTeam(new Team());     // Teams will be deleted by ClientGame destructor

   return game;
}


// Create a new ServerGame with one dummy team -- be sure to delete this somewhere!
ServerGame *newServerGame()
{
   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());

   LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(""));

   Level *level = new Level();
   level->loadLevelFromString("");

   ServerGame *game = new ServerGame(addr, settings, levelSource, false, false);
   game->setLevel(level);
   game->addTeam(new Team());    // Team will be cleaned up when game is deleted

   return game;
}


GamePair::GamePair(GameSettingsPtr settings)
{
   initialize(settings, "", 0);
}


GamePair::GamePair(GameSettingsPtr settings, const string &levelCode)
{
   initialize(settings, levelCode, 0);
}


// Create a pair of games suitable for testing client/server interaction.  Provide some levelcode to get things started.
GamePair::GamePair(const string &levelCode, S32 clientCount)
{
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());

   initialize(settings, levelCode, clientCount);
}


// Create a pair of games suitable for testing client/server interaction.  Provide some levelcode to get things started.
GamePair::GamePair(const Vector<string> &levelCode, S32 clientCount)
{
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());

   initialize(settings, levelCode, clientCount);
}


void GamePair::initialize(GameSettingsPtr settings, const string &levelCode, S32 clientCount)
{
   Vector<string> levelCodeVec;
   levelCodeVec.push_back(levelCode);

   initialize(settings, levelCodeVec, clientCount);
}


void GamePair::initialize(GameSettingsPtr settings, const Vector<string> &levelCode, S32 clientCount)
{
   // Need to start Lua before we add any clients.  Might as well do it now.
   LuaScriptRunner::startLua(settings->getFolderManager()->getLuaDir());

   LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(levelCode));
   initHosting(settings, levelSource, true, false);      // Creates a game and adds it to GameManager

   server = GameManager::getServerGame();                // Get the game created in initHosting

   // Give the host name something meaningful... in this case the name of the test
   if(::testing::UnitTest::GetInstance()->current_test_case())
   {
      const char *name = ::testing::UnitTest::GetInstance()->current_test_case()->name();
      const char *name2 = ::testing::UnitTest::GetInstance()->current_test_info()->name();
      server->getSettings()->setHostName(string(name) + "_" + name2, false);
   }

   server->startHosting();          // This will load levels and wipe out any teams

   for(S32 i = 0; i < clientCount; i++)
      addClient("TestPlayer" + itos(i));

   idle(1, 5);    // Give GameType and game objects time to propagate to client(s)
}


GamePair::~GamePair()
{
   // Disconnect all ClientGames before deleting
   const Vector<ClientGame *> *clientGames = GameManager::getClientGames();

   GameManager::getServerGame()->setAutoLeveling(false);    // No need to run this while we're shutting down

   for(S32 i = 0; i < clientGames->size(); i++)
      if(clientGames->get(i)->getConnectionToServer())
         clientGames->get(i)->getConnectionToServer()->disconnect(NetConnection::ReasonSelfDisconnect, "");

   // Note that when the client disconnects, all local ghosted objects, including GameType, are deleted
   idle(10, 5);

   // Clean up GameManager
   GameManager::deleteClientGames();
   GameManager::deleteServerGame();
 
   LuaScriptRunner::clearScriptCache();
   LuaScriptRunner::shutdown();
}


// Idle a pair of games for a specified number of cycles, static method
void GamePair::idle(U32 timeDelta, U32 cycles)
{
   for(U32 i = 0; i < cycles; i++)
      GameManager::idle(timeDelta);
}


ClientGame *GamePair::addClientAndSetRole(const string &name, ClientInfo::ClientRole role)
{
   ClientGame *client = addClient(name);
   ClientInfo *clientInfo = server->findClientInfo(name);
   clientInfo->setRole(role);

   return client;
}


ClientGame *GamePair::addClientAndSetTeam(const string &name, S32 teamIndex)
{
   ClientGame *clientGame = addClient(name);

   TNLAssert(teamIndex != NO_TEAM, "Invalid team!");
   TNLAssert(teamIndex < server->getTeamCount(), "Bad team!");

   ClientInfo *clientInfo = server->findClientInfo(name);
   server->getGameType()->changeClientTeam(clientInfo, teamIndex);

   return clientGame;
}


// Simulates player joining game from new client
ClientGame *GamePair::addClient(const string &name)
{
   ClientGame *clientGame = newClientGame();
   clientGame->userEnteredLoginCredentials(name, "password", false);    // Simulates entry from NameEntryUserInterface

   // Get a base UI going, so if we enter the game, and exit again, we'll have a place to land
   clientGame->activateMainMenuUI();

   GameManager::addClientGame(clientGame);

   return addClient(clientGame);
}


// teamIndex is optional
ClientGame *GamePair::addClient(ClientGame *clientGame)
{
   ServerGame *server = GameManager::getServerGame();

   clientGame->joinLocalGame(server->getNetInterface());     // Client will have owner privs!

   // We need to turn off TNL's bandwidth controls so our tests can run faster.  FASTER!!@!
   clientGame->getConnectionToServer()->useZeroLatencyForTesting();

   ClientInfo *clientInfo = server->findClientInfo(clientGame->getClientInfo()->getName().getString());

   if(!clientInfo->isRobot())
      clientInfo->getConnection()->useZeroLatencyForTesting();

   return clientGame;
}


S32 GamePair::getClientCount() const
{
   return GameManager::getClientGames()->size();
}


ClientGame *GamePair::getClient(S32 index)
{
   return GameManager::getClientGames()->get(index);
}


S32 GamePair::getClientIndex(const string &name)
{
   for(S32 i = 0; i < getClientCount(); i++)
      if(strcmp(getClient(i)->getClientInfo()->getName().getString(), name.c_str()) == 0)
         return i;

   return NONE;
}


void GamePair::removeClient(S32 index)
{
   // Disconnect before deleting
   ClientGame *clientGame = GameManager::getClientGames()->get(index);

   if(clientGame->getConnectionToServer())
      clientGame->getConnectionToServer()->disconnect(NetConnection::ReasonSelfDisconnect, "");

   this->idle(5, 5);      // Let things settle

   GameManager::deleteClientGame(index);
}


void GamePair::removeAllClients()
{
   while(GameManager::getClientGames()->size() > 0)
      removeClient(GameManager::getClientGames()->size() - 1);
}


GameUserInterface *GamePair::getGameUI(S32 clientIndex)
{
   EXPECT_TRUE(clientIndex >= 0 && clientIndex < getClientCount()) << "Index out of bounds!";
   ClientGame *client = getClient(clientIndex);
   GameUserInterface *ui = dynamic_cast<GameUserInterface *>(client->getUIManager()->getCurrentUI());
   EXPECT_TRUE(ui != NULL) << "Are we in the game?";

   return ui;
}


void GamePair::sendKeyPress(S32 clientIndex, InputCode inputCode)
{
   GameUserInterface *ui = getGameUI(clientIndex);
   ui->onKeyDown(inputCode);
}


// Properly set up and execute a chat command for the given client.  This simulates the user hitting enter after
// typing their command, including creating and dismissing the chat helper.
void GamePair::runChatCmd(S32 clientIndex, const string &command)
{
   GameUserInterface *ui = getGameUI(clientIndex);

   ui->activateHelper(HelperMenu::ChatHelperType, false);      // Need this active when entering chat cmd
   ChatHelper *helper = dynamic_cast<ChatHelper *>(ui->mHelperManager.mHelperStack.last());
   ASSERT_TRUE(helper) << "Where is our helper?";
   helper->mLineEditor.setString(command);
   helper->mCurrentChatType = ChatHelper::GlobalChat;
   helper->issueChat();
}


void GamePair::removeClient(const string &name)
{
   S32 index = -1;

   const Vector<ClientGame *> *clients = GameManager::getClientGames();

   for(S32 i = 0; i < clients->size(); i++)
   {
      if(clients->get(i)->getClientInfo() && string(clients->get(i)->getClientInfo()->getName().getString()) == name)
      {
         index = i;
         break;
      }
   }

   ASSERT_TRUE(index >= 0) << "Could not find specified player!";
   removeClient(index);
}


void GamePair::addBotClient(const string &name, S32 teamIndex)
{
   ServerGame *server = GameManager::getServerGame();

   server->addBot(Vector<string>(), ClientInfo::ClassRobotAddedByAutoleveler);
   // Get most recently added clientInfo
   ClientInfo *clientInfo = server->getClientInfo(server->getClientInfos()->size() - 1);
   ASSERT_TRUE(clientInfo->isRobot()) << "This is supposed to be a robot!";

   // Normally, in a game, a ship or bot would be destroyed and would respawn when their team changes, and upon
   // respawning the BfObject representing that ship would be on the correct team.  Not so here (where we are
   // taking lots of shortcuts); here we need to manually assign a new team to the robot object in addition to
   // it's more "official" setting on the ClientInfo.
   clientInfo->setTeamIndex(teamIndex);
   clientInfo->getShip()->setTeam(teamIndex);
}


};
