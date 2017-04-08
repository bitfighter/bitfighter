//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _TEST_UTILS_H
#define _TEST_UTILS_H

#include "GameSettings.h"    // For GameSettingsPtr def
#include "TeamConstants.h"

#include "../zap/ClientInfo.h"

#include <tnlGhostConnection.h>

#include <string>

namespace Zap
{

using namespace std;
using namespace TNL;

class ServerGame;
class ClientGame;
class GameUserInterface;

ClientGame *newClientGame();
ClientGame *newClientGame(const GameSettingsPtr &settings);

ServerGame *newServerGame();
ServerGame *newServerGame(const string &levelCode);

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
	explicit GamePair(const string &levelCode = "", S32 clients = 1);    // <-- No param constructor
   explicit GamePair(const Vector<string> &levelCode, S32 clients = 1);
   explicit GamePair(GameSettingsPtr settings);
   explicit GamePair(GameSettingsPtr settings, const string &levelCode);
   explicit GamePair(GameSettingsPtr serverSettings, GameSettingsPtr clientSettings, LevelSourcePtr serverLevelSource);

	~GamePair();

   void initialize(GameSettingsPtr settings, const string &levelcode, S32 clientCount);
   void initialize(GameSettingsPtr settings, const Vector<string> &levelCode, S32 clientCount);
   void initialize(GameSettingsPtr serverSettings, GameSettingsPtr clientSettings, 
                   LevelSourcePtr serverLevelSource, S32 clientCount, bool skipInitialIdle);

	static void idle(U32 timeDelta, U32 cycles = 1);
	ServerGame *server;

   ClientGame *addClient(const string &name) const;
   ClientGame *addClient(const string &name, GameSettingsPtr settings) const;
   static ClientGame *addClient(ClientGame *clientGame);

   
   ClientGame *addClientAndSetTeam(const string &name, S32 teamIndex);
   ClientGame *addClientAndSetRole(const string &name, ClientInfo::ClientRole role);

   void addBotClientAndSetTeam(const string &name, S32 teamIndex = NO_TEAM);

   S32 getClientCount() const;
   ClientGame *getClient(S32 index);
   S32 getClientIndex(const string &name);

   static void disconnectClient(S32 index);

   GameUserInterface *getGameUI(S32 clientIndex);
   void sendKeyPress(S32 clientIndex, InputCode inputCode);
   void runChatCmd(S32 clientIndex, const string &command);

   void removeClient(const string &name);
   void removeClient(S32 index);
   void removeAllClients();
};


};

#endif