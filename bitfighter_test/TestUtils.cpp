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

#include <string>

using namespace std;

namespace Zap
{

// Create a new ClientGame with one dummy team -- be sure to delete this somewhere!
ClientGame *newClientGame()
{
   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());

   // Need to initialize FontManager to use ClientGame... use false to avoid hassle of locating font files.
   // False will tell the FontManager to only use internally defined fonts; any TTF fonts will be replaced with Roman.
   FontManager::initialize(settings.get(), false);   
   ClientGame *game = new ClientGame(addr, settings, new UIManager());    // ClientGame destructor will clean up UIManager

   game->addTeam(new Team());     // Teams will be deleted by ClientGame destructor

   return game;
}


// Create a new ServerGame with one dummy team
ServerGame *newServerGame()
{
   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());
   LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(""));

   ServerGame *game = new ServerGame(addr, settings, levelSource, false, false);
   game->addTeam(new Team());    // Team will be cleaned up when game is deleted

   return game;
}


// Create a pair of games suitable for testing client/server interaction.  Provide some levelcode to get things started.
GamePair::GamePair(const string &levelCode)
{
   client = newClientGame();
   GameManager::addClientGame(client);

   GameSettingsPtr settings = client->getSettingsPtr();

   LuaScriptRunner::setScriptingDir(settings->getFolderManager()->luaDir);
   LuaScriptRunner::startLua();

   client->userEnteredLoginCredentials("TestPlayerOne", "password", false);    // Simulates entry from NameEntryUserInterface
   LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(levelCode));
   initHosting(settings, levelSource, true, false);

   server = GameManager::getServerGame();

   GameType *gt = new GameType();    // Cleaned up by database
   gt->addToGame(server, server->getGameObjDatabase());

   server->startHosting();     // This will load levels and wipe out any teams
   client->joinLocalGame(server->getNetInterface(), server->getHostingModePhase());

   // This is a bit hacky, but we need to turn off TNL's bandwidth controls so our tests can run faster.  FASTER!!@!
   client->getConnectionToServer()->useZeroLatencyForTesting();

   for(S32 i = 0; i < server->getClientCount(); i++)
      server->getClientInfo(i)->getConnection()->useZeroLatencyForTesting();
}


GamePair::~GamePair()
{
   LuaScriptRunner::clearScriptCache();
   LuaScriptRunner::shutdown();

   // Disconnect all ClientGames before deleting
   const Vector<ClientGame *> *clientGames = GameManager::getClientGames();

   for(S32 i = 0; i < clientGames->size(); i++)
      clientGames->get(i)->closeConnectionToGameServer();

   // Clean up GameManager
   GameManager::deleteServerGame();    
   GameManager::deleteClientGames();
}


// Idle a pair of games for a specified number of cycles
void GamePair::idle(U32 timeDelta, U32 cycles)
{
   for(U32 i = 0; i < cycles; i++)
   {
      //Platform::sleep(1);
      client->idle(timeDelta);
      server->idle(timeDelta);
   }
}


};