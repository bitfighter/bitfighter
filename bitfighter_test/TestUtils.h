//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _TEST_UTILS_H
#define _TEST_UTILS_H

#include "../zap/GameSettings.h"    // For GameSettingsPtr def
#include <tnl.h>

#include <string>

namespace Zap
{

using namespace std;
using namespace TNL;

class ServerGame;
class ClientGame;

ClientGame *newClientGame();
ClientGame *newClientGame(const GameSettingsPtr &settings);

ServerGame *newServerGame();

/**
 * POD struct to hold a pair of connected games
 */
struct GamePair
{
	GamePair(const string &levelCode = "", S32 clients = 1);
	~GamePair();
	static void idle(U32 timeDelta, U32 cycles = 1);
	ClientGame *client;
	ServerGame *server;
};


};

#endif