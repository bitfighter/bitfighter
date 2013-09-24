#ifndef _TEST_UTILS_H
#define _TEST_UTILS_H

#include <tnl.h>

#include <string>

namespace Zap
{

using namespace std;
using namespace TNL;

class ServerGame;
class ClientGame;

ClientGame *newClientGame();
ServerGame *newServerGame();

/**
 * POD struct to hold a pair of connected games
 */
struct GamePair
{
	GamePair(const string &levelCode = "");
	~GamePair();
	void idle(U32 timeDelta, U32 cycles = 1);
	ClientGame *client;
	ServerGame *server;
};


};

#endif