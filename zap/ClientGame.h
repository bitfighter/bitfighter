//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _CLIENTGAME_H_
#define _CLIENTGAME_H_

#ifdef ZAP_DEDICATED
#error "ClientGame.h shouldn't be included in dedicated build"
#endif

#include "game.h"

#include "gameConnection.h"
#include "dataConnection.h"      // For DataSender

#ifdef TNL_OS_WIN32
#include <windows.h>   // For screensaver... windows only feature, I'm afraid!
#endif

using namespace std;

namespace Zap
{



class ClientGame : public Game
{
   typedef Game Parent;

private:
   enum {
      NumStars = 256,               // 256 stars should be enough for anybody!   -- Bill Gates
      CommanderMapZoomTime = 350,   // Transition time between regular map and commander's map; in ms, higher = slower
   };

   Point mStars[NumStars];

   SafePtr<GameConnection> mConnectionToServer; // If this is a client game, this is the connection to the server
   bool mInCommanderMap;

   U32 mCommanderZoomDelta;

   Timer mScreenSaverTimer;
   void supressScreensaver();

   UIManager *mUIManager;
   string mRemoteLevelDownloadFilename;

   bool mDebugShowShipCoords;       // Show coords on ship?
   bool mDebugShowMeshZones;        // Show bot nav mesh zones?

   Vector<string> mMuteList;        // List of players we aren't listening to anymore because they've annoyed us!


public:
   ClientGame(const Address &bindAddress);
   virtual ~ClientGame();

   UserInterfaceData *mUserInterfaceData;

   bool hasValidControlObject();
   bool isConnectedToServer();

   GameConnection *getConnectionToServer();
   void setConnectionToServer(GameConnection *connection);

   UIManager *getUIManager() { return mUIManager; }


   bool getInCommanderMap() { return mInCommanderMap; }
   void setInCommanderMap(bool inCommanderMap) { mInCommanderMap = inCommanderMap; }

   F32 getCommanderZoomFraction() const { return mCommanderZoomDelta / F32(CommanderMapZoomTime); }
   Point worldToScreenPoint(const Point *p) const;
   void drawStars(F32 alphaFrac, Point cameraPos, Point visibleExtent);

   void render();             // Delegates to renderNormal, renderCommander, or renderSuspended, as appropriate
   void renderNormal();       // Render game in normal play mode
   void renderCommander();    // Render game in commander's map mode
   void renderSuspended();    // Render suspended game

   void renderOverlayMap();   // Render the overlay map in normal play mode
   void resetZoomDelta() { mCommanderZoomDelta = CommanderMapZoomTime; } 
   void clearZoomDelta() { mCommanderZoomDelta = 0; }
   bool isServer() { return false; }
   void idle(U32 timeDelta);
   void zoomCommanderMap();

   bool isShowingDebugShipCoords() const { return mDebugShowShipCoords; }     // Show coords on ship?
   void toggleShowingShipCoords() { mDebugShowShipCoords = !mDebugShowShipCoords; }

   bool isShowingDebugMeshZones()  const { return mDebugShowMeshZones; }      // Show bot nav mesh zones?
   void toggleShowingMeshZones() { mDebugShowMeshZones = !mDebugShowMeshZones; }

   void gotServerListFromMaster(const Vector<IPAddress> &serverList);
   void gotChatMessage(const char *playerNick, const char *message, bool isPrivate, bool isSystem);
   void setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks);
   void playerJoinedGlobalChat(const StringTableEntry &playerNick);
   void playerLeftGlobalChat(const StringTableEntry &playerNick);

   // Check for permissions
   bool hasAdmin(const char *failureMessage);
   bool hasLevelChange(const char *failureMessage);

   void enterMode(UIMode mode);


   void addToMuteList(const string &name) { mMuteList.push_back(name); }
   bool isOnMuteList(const string &name);


   void connectionToServerRejected();
   void setMOTD(const char *motd);
   void setNeedToUpgrade(bool needToUpgrade);

   string getRemoteLevelDownloadFilename() const { return mRemoteLevelDownloadFilename; }
   void setRemoteLevelDownloadFilename(const string &filename) { mRemoteLevelDownloadFilename = filename; }

   void changePassword(GameConnection::ParamType type, const Vector<string> &words, bool required);
   void changeServerNameDescr(GameConnection::ParamType type, const Vector<string> &words);
   bool checkName(const string &name);    // Make sure name is valid, and correct case of name if otherwise correct




   // Alert users when they get a reply to their request for elevated permissions
   void gotAdminPermissionsReply(bool granted);
   void gotLevelChangePermissionsReply(bool granted);

   void gotPingResponse(const Address &address, const Nonce &nonce, U32 clientIdentityToken);
   void gotQueryResponse(const Address &address, const Nonce &nonce, const char *serverName, const char *serverDescr, 
                         U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired);

   void shutdownInitiated(U16 time, const StringTableEntry &name, const StringPtr &reason, bool originator);
   void cancelShutdown();

   void displayMessageBox(const StringTableEntry &title, const StringTableEntry &instr, const Vector<StringTableEntry> &message);
   void displayMessage(const Color &msgColor, const char *format, ...);

   void onConnectionTerminated(const Address &serverAddress, NetConnection::TerminationReason reason, const char *reasonStr);
   void onConnectionToMasterTerminated(NetConnection::TerminationReason reason, const char *reasonStr);

   void onConnectTerminated(const Address &serverAddress, NetConnection::TerminationReason reason);
   void runCommand(const char *command);
   void setVolume(VolumeType volType, const Vector<string> &words);


   const Color *getTeamColor(S32 teamIndex) const;

   //const char *getRemoteLevelDownloadFilename();

   U32 getPlayerAndRobotCount();    // Returns number of human and robot players
   U32 getPlayerCount();            // Returns number of human players


   string getRequestedServerName();
   string getServerPassword();
   string getHashedServerPassword();

   void displayErrorMessage(const char *format, ...);
   void displaySuccessMessage(const char *format, ...);

   void suspendGame()   { mGameSuspended = true; }
   void unsuspendGame() { mGameSuspended = false; }

   boost::shared_ptr<AbstractTeam> getNewTeam();

   bool processPseudoItem(S32 argc, const char **argv, const string &levelFileName);        // For loading levels in editor

};

////////////////////////////////////////
////////////////////////////////////////


extern ClientGame *gClientGame;

};


#endif

