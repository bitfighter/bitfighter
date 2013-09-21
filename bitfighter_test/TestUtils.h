#ifndef _TEST_UTILS_H
#define _TEST_UTILS_H

#include <string>

namespace Zap
{

using namespace std;

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
	ClientGame* client;
	ServerGame* server;
};


};

#endif