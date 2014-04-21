//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TestUtils.h"
#include "../zap/gameType.h"
#include "../zap/GameManager.h"
#include "../zap/ServerGame.h"
#include "../zap/ClientGame.h"
#include "../zap/FontManager.h"
#include "../zap/UIManager.h"
#include "../zap/SystemFunctions.h"

#include "../zap/stringUtils.h"
#include "gtest/gtest.h"

#include <string>

using namespace std;

namespace Zap
{

// Create a new ClientGame with one dummy team -- be sure to delete this somewhere!
ClientGame *newClientGame()
{
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   return newClientGame(settings);
}


ClientGame *newClientGame(const GameSettingsPtr &settings)
{
   Address addr;
   
   // Need to initialize FontManager to use ClientGame... use false to avoid hassle of locating font files.
   // False will tell the FontManager to only use internally defined fonts; any TTF fonts will be replaced with Roman.
   FontManager::initialize(settings.get(), false);   
   ClientGame *game = new ClientGame(addr, settings, new UIManager());    // ClientGame destructor will clean up UIManager

   game->addTeam(new Team());     // Teams will be deleted by ClientGame destructor

   return game;
}


// Create a new ServerGame with one dummy team -- be sure to delete this somewhere!
ServerGame *newServerGame()
{
   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(""));

   ServerGame *game = new ServerGame(addr, settings, levelSource, false, false);
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


void GamePair::initialize(GameSettingsPtr settings, const string &levelCode, S32 clientCount)
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
}


GamePair::~GamePair()
{
   // Disconnect all ClientGames before deleting
   const Vector<ClientGame *> *clientGames = GameManager::getClientGames();

   GameManager::getServerGame()->setAutoLeveling(false);    // No need to run this while we're shutting down

   for(S32 i = 0; i < clientGames->size(); i++)
      if(clientGames->get(i)->getConnectionToServer())
         clientGames->get(i)->getConnectionToServer()->disconnect(NetConnection::ReasonSelfDisconnect, "");

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


// Simulates player joining game from new client
void GamePair::addClient(const string &name, S32 team)
{
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());

   ServerGame *server = GameManager::getServerGame();
   ClientGame *client = newClientGame(settings);
   GameManager::addClientGame(client);

   client->userEnteredLoginCredentials(name, "password", false);    // Simulates entry from NameEntryUserInterface

   client->joinLocalGame(server->getNetInterface());

   // We need to turn off TNL's bandwidth controls so our tests can run faster.  FASTER!!@!
   client->getConnectionToServer()->useZeroLatencyForTesting();

   ClientInfo *clientInfo = server->findClientInfo(name.c_str());

   if(!clientInfo->isRobot())
      clientInfo->getConnection()->useZeroLatencyForTesting();

   if(team != NO_TEAM)
   {
      TNLAssert(team < server->getTeamCount(), "Bad team!");
      server->getGameType()->changeClientTeam(clientInfo, team);
   }
}


ClientGame *GamePair::getClient(S32 index)
{
   return GameManager::getClientGames()->get(index);
}


void GamePair::removeClient(S32 index)
{
   // Disconnect before deleting
   ClientGame *clientGame = GameManager::getClientGames()->get(index);

   clientGame->getConnectionToServer()->disconnect(NetConnection::ReasonSelfDisconnect, "");

   this->idle(10, 5);      // Let things propagate

   GameManager::deleteClientGame(index);
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

   TNLAssert(index >= 0, "Could not find specified player!");
   removeClient(index);
}


void GamePair::addBotClient(const string &name, S32 team)
{
   ServerGame *server = GameManager::getServerGame();

   server->addBot(Vector<const char *>(), ClientInfo::ClassRobotAddedByAutoleveler);
   // Get most recently added clientInfo
   ClientInfo *clientInfo = server->getClientInfo(server->getClientInfos()->size() - 1);
   ASSERT_TRUE(clientInfo->isRobot()) << "This is supposed to be a robot!";

   // Normally, in a game, a ship or bot would be destroyed and would respawn when their team changes, and upon
   // respawning the BfObject representing that ship would be on the correct team.  Not so here (where we are
   // taking lots of shortcuts); here we need to manually assign a new team to the robot object in addition to
   // it's more "official" setting on the ClientInfo.
   clientInfo->setTeamIndex(team);
   clientInfo->getShip()->setTeam(team);
}


};
