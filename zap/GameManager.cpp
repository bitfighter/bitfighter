//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "GameManager.h"
#include "ServerGame.h" 

namespace Zap
{

// Declare statics
ServerGame *GameManager::mServerGame = NULL;
Vector<ClientGame *> GameManager::mClientGames;


// Constructor
GameManager::GameManager()
{
   // Do nothing
}


// Destructor
GameManager::~GameManager()
{
   // Do nothing
}


ServerGame *GameManager::getServerGame()
{
   return mServerGame;
}


void GameManager::setServerGame(ServerGame *serverGame)
{
   TNLAssert(serverGame, "Expect a valid serverGame here!");
   TNLAssert(!mServerGame, "Already have a ServerGame!");

   mServerGame = serverGame;
}


void GameManager::deleteServerGame()
{
   TNLAssert(mServerGame, "Expect a valid ServerGame here!");

   delete mServerGame;     // Kill the serverGame (leaving the clients running)
   mServerGame = NULL;
}


const Vector<ClientGame *> *GameManager::getClientGames()
{
   return &mClientGames;
}


void GameManager::deleteClientGames()
{
   mClientGames.deleteAndClear();
}


void GameManager::addClientGame(ClientGame *clientGame)
{
   mClientGames.push_back(clientGame);
}


} 
