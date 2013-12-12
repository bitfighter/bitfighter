//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _TEST_UTILS_H
#define _TEST_UTILS_H

#include "GameSettings.h"    // For GameSettingsPtr def
#include <tnl.h>
#include <tnlGhostConnection.h>

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

// Generic pack/unpack function -- feed it any class that supports pack/unpack
template <class T>
void packUnpack(T input, T &output, U32 mask = 0xFFFFFFFF)
{
   BitStream stream;       
   GhostConnection conn;
   
   output.markAsGhost(); 

   input.packUpdate(&conn, mask, &stream);   // Write the object
   stream.setBitPosition(0);                 // Move the stream's pointer back to the beginning
   output.unpackUpdate(&conn, &stream);      // Read the object back
}

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