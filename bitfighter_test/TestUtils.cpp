#include "TestUtils.h"
#include "../zap/gameType.h"
#include "../zap/ServerGame.h"
#include "../zap/ClientGame.h"
#include "../zap/FontManager.h"
#include "../zap/UIManager.h"
#include "../zap/SystemFunctions.h"

#include <string>

using namespace std;

namespace Zap
{

// Create a new ClientGame with one dummy team -- be sure to clean up settings somewhere!
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


GamePair::GamePair(const string &levelCode)
{
   client = newClientGame();

   GameSettingsPtr settings = client->getSettingsPtr();

   client->userEnteredLoginCredentials("TestUser", "password", false);    // Simulates entry from NameEntryUserInterface

   LevelSourcePtr levelSource = LevelSourcePtr(new StringLevelSource(levelCode));
   server = initHosting(settings, levelSource, true, false);

   GameType *gt = new GameType();    // Cleaned up by database
   gt->addToGame(server, server->getGameObjDatabase());

   server->startHosting();     // This will load levels and wipe out any teams
   client->joinLocalGame(server->getNetInterface());
}


GamePair::~GamePair()
{
	delete client;
	delete server;
}


void GamePair::idle(U32 timeDelta)
{
   client->idle(timeDelta);
   server->idle(timeDelta);
}


};