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
public:
   enum HostingModePhase
   {
      NotHosting,
      LoadingLevels,
      DoneLoadingLevels,
      Hosting
   };

private:
   static ServerGame *mServerGame;
   static Vector<ClientGame *> mClientGames;

   static HostingModePhase mHostingModePhase;

public:
   GameManager();
   virtual ~GameManager();

   // ServerGame related
   static void setServerGame(ServerGame *serverGame);
   static ServerGame *getServerGame();
   static void deleteServerGame();
   static void idleServerGame(U32 timeDelta);

   // ClientGame related
   static const Vector<ClientGame *> *getClientGames();
   static void addClientGame(ClientGame *clientGame);
   static void deleteClientGame(S32 index);     // Delete specified game
   static void deleteClientGames();             // Delete all games

   static void idleClientGames(U32 timeDelta);

   // Other
   static void idle(U32 timeDelta);
   static void setHostingModePhase(HostingModePhase);
   static HostingModePhase getHostingModePhase();
};

} 

#endif 
