//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "GameManager.h"
#include "ServerGame.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "ClientInfo.h"
#endif

namespace Zap
{

// Declare statics
ServerGame *GameManager::mServerGame = NULL;
Vector<ClientGame *> GameManager::mClientGames;
GameManager::HostingModePhase GameManager::mHostingModePhase = GameManager::NotHosting;


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
   // mServerGame might be NULL here; for example when quitting after losing a connection to the game server
   delete mServerGame;     // Kill the serverGame (leaving the clients running)
   mServerGame = NULL;
}


void GameManager::idleServerGame(U32 timeDelta)
{
   if(mServerGame)
      mServerGame->idle(timeDelta);
}


/////


const Vector<ClientGame *> *GameManager::getClientGames()
{
   return &mClientGames;
}


void GameManager::deleteClientGames()
{
   mClientGames.deleteAndClear();
}


void GameManager::deleteClientGame(S32 index)
{
   mClientGames.deleteAndErase(index);
}


void GameManager::addClientGame(ClientGame *clientGame)
{
   mClientGames.push_back(clientGame);
}


void GameManager::idleClientGames(U32 timeDelta)
{
#ifndef ZAP_DEDICATED
   for(S32 i = 0; i < mClientGames.size(); i++)
      mClientGames[i]->idle(timeDelta);
#endif
}


/////


void GameManager::idle(U32 timeDelta)
{
   idleServerGame(timeDelta);
   idleClientGames(timeDelta);
}


void GameManager::setHostingModePhase(HostingModePhase phase)
{
   mHostingModePhase = phase;
}


GameManager::HostingModePhase GameManager::getHostingModePhase()
{
   return mHostingModePhase;
}


} 
