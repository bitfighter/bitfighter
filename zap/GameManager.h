//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAME_MANAGER_H_
#define _GAME_MANAGER_H_

#include "tnlVector.h"

using namespace TNL;

namespace Zap
{

class ServerGame;
class ClientGame;

// Singleton class for keeping track of various Game objects
class GameManager
{
private:
   static ServerGame *mServerGame;
   static Vector<ClientGame *> mClientGames;

public:
   GameManager();
   virtual ~GameManager();

   static void setServerGame(ServerGame *serverGame);
   static ServerGame *getServerGame();
   static void deleteServerGame();
};

} 

#endif 
